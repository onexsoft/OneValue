#ifndef LISTCMDHANDLER_H
#define LISTCMDHANDLER_H

class ClientPacket;

void onLindexCommand(ClientPacket* packet, void*);
void onLlenCommand(ClientPacket* packet, void*);
void onLinsertCommand(ClientPacket* packet, void*);
void onLpopCommand(ClientPacket* packet, void*);
void onLpushCommand(ClientPacket* packet, void*);
void onLpushxCommand(ClientPacket* packet, void*);
void onLRangeCommand(ClientPacket* packet, void*);

void onLRemCommand(ClientPacket* packet, void*);
void onLSetCommand(ClientPacket* packet, void*);
void onLTrimCommand(ClientPacket* packet, void*);
void onRPopCommand(ClientPacket* packet, void*);
void onRPopLPushCommand(ClientPacket* packet, void*);
void onRPushCommand(ClientPacket* packet, void*);
void onRpushxCommand(ClientPacket* packet, void*);
void onLClearCommand(ClientPacket* packet, void*);

#endif
