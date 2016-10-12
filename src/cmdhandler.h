#ifndef CMDHANDLER_H
#define CMDHANDLER_H

class ClientPacket;

void onRawSetCommand(ClientPacket*, void*);

void onAppendCommand(ClientPacket*, void*);
void onDecrCommand(ClientPacket*, void*);
void onDecrByCommand(ClientPacket*, void*);
void onGetRangeCommand(ClientPacket*, void*);
void onGetSetCommand(ClientPacket*, void*);
void onIncrCommand(ClientPacket*, void*);
void onIncrByCommand(ClientPacket*, void*);
void onIncrByFloatCommand(ClientPacket*, void*);
void onMGetCommand(ClientPacket*, void*);
void onMSetCommand(ClientPacket*, void*);
void onExistsCommand(ClientPacket*, void *);
void onMSetNXCommand(ClientPacket*, void*);
void onPSetEXCommand(ClientPacket*, void*);
void onSetEXCommand(ClientPacket*, void*);
void onSetNXCommand(ClientPacket*, void*);
void onSetRangeCommand(ClientPacket*, void*);
void onStrlenCommand(ClientPacket*, void*);
void onGetCommand(ClientPacket*, void*);
void onSetCommand(ClientPacket*, void*);
void onDelCommand(ClientPacket*, void*);
void onExpireCommand(ClientPacket*, void*);

void onFlushdbCommand(ClientPacket*, void*);
void onPingCommand(ClientPacket*, void*);

void onShowCommand(ClientPacket*, void*);
void onSyncCommand(ClientPacket*, void*);       //__sync [filename] [last update pos]
void onCopyCommand(ClientPacket*, void*);       //__copy [local onevalue port]
void onSyncFromCommand(ClientPacket*, void*);   //syncfrom [dest ip] [dest port]

#endif
