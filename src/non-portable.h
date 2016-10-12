#ifndef NONPORTABLE_H
#define NONPORTABLE_H

#include "util/tcpsocket.h"

namespace NonPortable {

//guard
void guard(void (*driverFunc)(), int exit_key);

//daemon
int daemonize(void);

//create unix socket file
socket_t createUnixSocketFile(const char* file);

//vip address
int setVipAddress(const char *ifname, const char *address, int isdel);

}

#endif // NONPORTABLE_H
