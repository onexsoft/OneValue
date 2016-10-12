#ifndef HASHCMDHANDLER_H
#define HASHCMDHANDLER_H

class ClientPacket;

void onHsetCommand(ClientPacket* packet, void*);

void onHgetCommand(ClientPacket* packet, void*);

void onHmgetCommand(ClientPacket* packet, void*);

void onHgetAllCommand(ClientPacket* packet, void*);

void onHexistsCommand(ClientPacket* packet, void*);

void onHkeysCommand(ClientPacket* packet, void*);

void onHvalsCommand(ClientPacket* packet, void*);

void onHincrbyCommand(ClientPacket* packet, void*);

void onHdelCommand(ClientPacket* packet, void*);

void onHlenCommand(ClientPacket* packet, void*);

void onHmsetCommand(ClientPacket* packet, void*);

void onHsetnxCommand(ClientPacket* packet, void*);

void onHincrbyFloatCommand(ClientPacket* packet, void*);

void onHClearCommand(ClientPacket* packet, void*);

#endif // HASHCMDHANDLER_H


