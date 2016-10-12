#include "util/logger.h"
#include "command.h"
#include "cmdhandler.h"
#include "non-portable.h"
#include "sync.h"
#include "redisproxy.h"

void ClientPacket::setFinishedState(State state)
{
    switch (state) {
    case ClientPacket::Unknown:
        sendBuff.append("-Unknown state\r\n");
        break;
    case ClientPacket::ProtoError:
        sendBuff.append("-Proto error\r\n");
        break;
    case ClientPacket::ProtoNotSupport:
        sendBuff.append("-Proto not support\r\n");
        break;
    case ClientPacket::WrongNumberOfArguments:
        sendBuff.append("-Wrong number of arguments\r\n");
        break;
    case ClientPacket::RequestError:
        sendBuff.append("-Request error\r\n");
        break;
    case ClientPacket::RequestFinished:
        break;
    default:
        break;
    }
    server->writeReply(this);
}



static Monitor dummy;
RedisProxy::RedisProxy(void)
{
    m_monitor = &dummy;
    m_leveldbCluster = NULL;
    m_syncThread = NULL;
    m_vipAddress[0] = 0;
    m_vipName[0] = 0;
    m_unixSocketFileName[0] = 0;
    m_vipEnabled = false;
    m_threadPoolRefCount = 0;
    m_eventLoopThreadPool = NULL;
}

RedisProxy::~RedisProxy(void)
{
}

void RedisProxy::setMonitor(Monitor *monitor)
{
    if (monitor != NULL) {
        m_monitor = monitor;
    }
}

bool RedisProxy::run(const HostAddress &addr)
{
    if (isRunning()) {
        return false;
    }

    if (m_vipEnabled) {
        TcpSocket sock = TcpSocket::createTcpSocket();
        Logger::log(Logger::Message, "connect to vip address(%s:%d)...", m_vipAddress, addr.port());
        if (!sock.connect(HostAddress(m_vipAddress, addr.port()))) {
            Logger::log(Logger::Message, "set VIP [%s,%s]...", m_vipName, m_vipAddress);
            int ret = NonPortable::setVipAddress(m_vipName, m_vipAddress, 0);
            Logger::log(Logger::Message, "set_vip_address return %d", ret);
        } else {
            m_vipSocket = sock;
            m_vipEvent.set(eventLoop(), sock.socket(), EV_READ, vipHandler, this);
            m_vipEvent.active();
        }
    }

    if (strlen(m_unixSocketFileName) > 0) {
        remove(m_unixSocketFileName);
        socket_t sockfile = NonPortable::createUnixSocketFile(m_unixSocketFileName);
        if (sockfile != -1) {
            Logger::log(Logger::Message, "Unix socket file created. path='%s'", m_unixSocketFileName);
            m_sockfile = TcpSocket(sockfile);
            m_sockfileEvent.set(eventLoop(), sockfile, EV_READ | EV_PERSIST, onAcceptHandler, this);
            m_sockfileEvent.active();
        } else {
            Logger::log(Logger::Error, "Create unix socket file '%s' failed", m_unixSocketFileName);
        }
    }

    RedisCommandTable* cmdtable = RedisCommandTable::instance();
    cmdtable->registerCommand("__SYNC", RedisCommand::PrivType, onSyncCommand, this);
    cmdtable->registerCommand("__COPY", RedisCommand::PrivType, onCopyCommand, NULL);
    cmdtable->registerCommand("SYNCFROM", -1, onSyncFromCommand, NULL);

    m_monitor->proxyStarted(this);
    Logger::log(Logger::Message, "Start the %s on port %d", APP_NAME, addr.port());

    return TcpServer::run(addr);
}

void RedisProxy::stop(void)
{
    TcpServer::stop();
    if (m_vipEnabled) {
        if (!m_vipSocket.isNull()) {
            Logger::log(Logger::Message, "delete vip...");
            int ret = NonPortable::setVipAddress(m_vipName, m_vipAddress, 1);
            Logger::log(Logger::Message, "delete vip return %d", ret);

            m_vipEvent.remove();
            m_vipSocket.close();
        }
    }
    if (!m_sockfile.isNull()) {
        m_sockfileEvent.remove();
        m_sockfile.close();
    }
    Logger::log(Logger::Message, "%s has stopped", APP_NAME);
}

Context *RedisProxy::createContextObject(void)
{
    ClientPacket* packet = new ClientPacket;

    EventLoop* loop;
    if (m_eventLoopThreadPool) {
        int threadCount = m_eventLoopThreadPool->size();
        EventLoopThread* loopThread = m_eventLoopThreadPool->thread(m_threadPoolRefCount % threadCount);
        ++m_threadPoolRefCount;
        loop = loopThread->eventLoop();
    } else {
        loop = eventLoop();
    }

    packet->eventLoop = loop;
    return packet;
}

void RedisProxy::destroyContextObject(Context *c)
{
    delete c;
}

void RedisProxy::closeConnection(Context* c)
{
    ClientPacket* packet = (ClientPacket*)c;
    m_monitor->clientDisconnected(packet);
    TcpServer::closeConnection(c);
}

void RedisProxy::clientConnected(Context* c)
{
    ClientPacket* packet = (ClientPacket*)c;
    m_monitor->clientConnected(packet);
}

TcpServer::ReadStatus RedisProxy::readingRequest(Context *c)
{
    ClientPacket* packet = (ClientPacket*)c;
    packet->recvParseResult.reset();
    RedisProto::ParseState state = RedisProto::parse(packet->recvBuff.data() + packet->recvBufferOffset,
                                                     packet->recvBuff.size() - packet->recvBufferOffset,
                                                     &packet->recvParseResult);
    switch (state) {
    case RedisProto::ProtoError:
        return ReadError;
    case RedisProto::ProtoIncomplete:
        return ReadIncomplete;
    case RedisProto::ProtoOK:
        packet->recvBufferOffset += packet->recvParseResult.protoBuffLen;
        return ReadFinished;
    default:
        return ReadError;
    }
}

void RedisProxy::readRequestFinished(Context *c)
{
    ClientPacket* packet = (ClientPacket*)c;

    RedisProtoParseResult& r = packet->recvParseResult;
    char* cmd = r.tokens[0].s;
    int len = r.tokens[0].len;

    RedisCommand* command = RedisCommandTable::instance()->findCommand(cmd, len);
    if (!command) {
        packet->setFinishedState(ClientPacket::ProtoNotSupport);
        return;
    }

    packet->commandType = command->type;
    command->handler(packet, command->arg);
}

void RedisProxy::writeReply(Context *c)
{
    ClientPacket* packet = (ClientPacket*)c;
    if (packet->recvBufferOffset != packet->recvBuff.size()) {
        packet->recvParseResult.reset();
        RedisProto::ParseState state = RedisProto::parse(packet->recvBuff.data() + packet->recvBufferOffset,
                                                         packet->recvBuff.size() - packet->recvBufferOffset,
                                                         &packet->recvParseResult);
        switch (state) {
        case RedisProto::ProtoError:
            closeConnection(c);
            break;
        case RedisProto::ProtoIncomplete:
            waitRequest(c);
            break;
        case RedisProto::ProtoOK:
            packet->recvBufferOffset += packet->recvParseResult.protoBuffLen;
            readRequestFinished(c);
            break;
        default:
            break;
        }
    } else {
        TcpServer::writeReply(c);
    }
}

void RedisProxy::writeReplyFinished(Context *c)
{
    ClientPacket* packet = (ClientPacket*)c;
    m_monitor->replyClientFinished(packet);
    packet->commandType = -1;
    packet->sendBuff.clear();
    packet->recvBuff.clear();
    packet->sendBytes = 0;
    packet->recvBytes = 0;
    packet->recvBufferOffset = 0;
    packet->recvParseResult.reset();
    waitRequest(c);
}

void RedisProxy::vipHandler(socket_t sock, short, void* arg)
{
    char buff[64];
    RedisProxy* proxy = (RedisProxy*)arg;
    int ret = ::recv(sock, buff, 64, 0);
    if (ret == 0) {
        Logger::log(Logger::Message, "disconnected from VIP. change vip address...");
        int ret = NonPortable::setVipAddress(proxy->m_vipName, proxy->m_vipAddress, 0);
        Logger::log(Logger::Message, "set_vip_address return %d", ret);
        proxy->m_vipSocket.close();
    }
}

