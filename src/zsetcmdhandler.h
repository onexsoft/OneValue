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

#ifndef ZSETCMDHANDLER_H
#define ZSETCMDHANDLER_H

class ClientPacket;

//SET
void onSAddCommand(ClientPacket* packet, void*);
void onSCardCommand(ClientPacket* packet, void*);
void onSDiffCommand(ClientPacket* packet, void*);
void onSDiffStoreCommand(ClientPacket* packet, void*);
void onSInterCommand(ClientPacket* packet, void*);
void onSInterStoreCommand(ClientPacket* packet, void*);
void onSIsMemberCommand(ClientPacket* packet, void*);

void onSMembersCommand(ClientPacket* packet, void*);
void onSMoveCommand(ClientPacket* packet, void*);
void onSPopCommand(ClientPacket* packet, void*);
void onSRandMember(ClientPacket* packet, void*);
void onSRemCommand(ClientPacket* packet, void*);
void onSUnionCommand(ClientPacket* packet, void*);
void onSUnionStoreCommand(ClientPacket* packet, void*);
void onSClearCommand(ClientPacket* packet, void*);


//ZSET
void onZAddCommand(ClientPacket* packet, void*);
void onZRemCommand(ClientPacket* packet, void*);
void onZIncrbyCommand(ClientPacket* packet, void*);
void onZRankCommand(ClientPacket* packet, void*);
void onZRevRankCommand(ClientPacket* packet, void*);
void onZRangeCommand(ClientPacket* packet, void*);
void onZRevRangeCommand(ClientPacket* packet, void*);
void onZRangeByScoreCommand(ClientPacket* packet, void*);
void onZRevRangeByScoreCommand(ClientPacket* packet, void*);
void onZCountCommand(ClientPacket* packet, void*);
void onZCardCommand(ClientPacket* packet, void*);
void onZSCoreCommand(ClientPacket* packet, void*);
void onZRemRangeByRankCommand(ClientPacket* packet, void*);
void onZRemRangeByScoreCommand(ClientPacket*packet, void*);
void onZClear(ClientPacket*packet, void*);

#endif
