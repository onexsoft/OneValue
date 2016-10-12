#include <signal.h>
#include <algorithm>

#include "util/logger.h"
#include "redisproxy.h"
#include "onevaluecfg.h"
#include "monitor.h"
#include "sync.h"
#include "non-portable.h"

RedisProxy* currentProxy = NULL;
void handler(int)
{
    exit(0);
}

void setupSignal(void)
{
#ifndef WIN32
    struct sigaction sig;

    sig.sa_handler = handler;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags = 0;

    sigaction(SIGTERM, &sig, NULL);
    sigaction(SIGINT, &sig, NULL);
#endif
}

void startOneValue(void)
{
    COneValueCfg* cfg = COneValueCfg::instance();

    setupSignal();

    RedisProxy proxy;
    EventLoopThreadPool pool;
    currentProxy = &proxy;
    pool.start(cfg->threadNum());
    proxy.setEventLoopThreadPool(&pool);
    proxy.setUnixSockFileName(cfg->unixSocketFile());

    //Set logger handler
    FileLogger fileLogger;
    if (fileLogger.setFileName(cfg->logFile())) {
        Logger::log(Logger::Message, "Using the log file(%s) output", fileLogger.fileName());
        Logger::setDefaultLogger(&fileLogger);
    }

    //Set leveldb cluster
    COption* opt = cfg->dbOption();

    LeveldbCluster cluster;
    LeveldbCluster::Option clusterOption;

    CBinLog* binlogCfg = cfg->binlog();
    clusterOption.maxBinlogSize = binlogCfg->maxBinlogSize();
    clusterOption.binlogEnabled = binlogCfg->enabled();
    clusterOption.workdir = cfg->workDir();
    clusterOption.maxhash = cfg->hashMax();
    clusterOption.sync = opt->sync();
    clusterOption.leveldbopt.compress = opt->compress();
    clusterOption.leveldbopt.cacheSize = opt->lruCacheSize();
    clusterOption.leveldbopt.writeBufferSize = opt->writeBufSize();
    for (int i = 0; i < cfg->dbCnt(); ++i) {
        CDbNode* dbcfg = cfg->dbIndex(i);
        clusterOption.dbnames.push_back(dbcfg->db_name);
    }

    //Start cluster and set mapping
    if (!cluster.start(clusterOption)) {
        Logger::log(Logger::Error, "Start failed. Stop");
        return;
    }
    for (int i = 0; i < cfg->dbCnt(); ++i) {
        CDbNode* dbcfg = cfg->dbIndex(i);
        for (int h = dbcfg->hash_min; h <= dbcfg->hash_max; ++h) {
            cluster.setMapping(h, dbcfg->db_name);
        }
    }
    proxy.setLeveldbCluster(&cluster);

    //Start sync service
    SMaster* masterInfo = cfg->master();
    if (masterInfo->ip[0] != 0) {
        Sync* sync = new Sync(&proxy, masterInfo->ip, masterInfo->port);
        sync->setSyncInterval(masterInfo->syncInterval);
        sync->start();
        proxy.setSyncThread(sync);
    }

    //Set monitor
    CProxyMonitor monitor;
    proxy.setMonitor(&monitor);

    int port = cfg->port();
    if (port <= 0) {
        port = RedisProxy::DefaultPort;
    }

    //Run proxy
    proxy.run(HostAddress(port));
}

void createPidFile(const char* pidFile)
{
    FILE* fp = fopen(pidFile, "w");
    if (fp) {
#ifdef WIN32
        fprintf(fp, "%d\n", ::GetCurrentProcessId());
#else
        fprintf(fp, "%d\n", (int)getpid());
#endif
        fclose(fp);
    }
}

int main(int argc, char** argv)
{
    printf("%s %s, Copyright 2015 by OneXSoft\n\n", APP_NAME, APP_VERSION);

    const char* cfgFile = "onevalue.xml";
    if (argc == 1) {
        Logger::log(Logger::Message, "No config file specified. using the default config(%s)", cfgFile);
    } if (argc >= 2) {
        cfgFile = argv[1];
    }

    COneValueCfg* cfg = COneValueCfg::instance();
    if (!cfg->loadCfg(cfgFile)) {
        Logger::log(Logger::Error, "Failed to read config file");
        return 1;
    }

    const char* errInfo;
    if (!COneValueCfgChecker::isValid(cfg, errInfo)) {
        Logger::log(Logger::Error, "Invalid configuration: %s", errInfo);
        return 1;
    }

    const char* pidfile = cfg->pidFile();
    bool pidfile_enabled = (strlen(pidfile) > 0);
    if (pidfile_enabled) {
        createPidFile(pidfile);
    }

    //Background
    if (cfg->daemonize()) {
        NonPortable::daemonize();
    }

    //Guard
    if (cfg->guard()) {
        NonPortable::guard(startOneValue, APP_EXIT_KEY);
    } else {
        startOneValue();
    }

    if (pidfile_enabled) {
        remove(pidfile);
    }

    return 0;
}

















