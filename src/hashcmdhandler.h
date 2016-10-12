/*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*/

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


