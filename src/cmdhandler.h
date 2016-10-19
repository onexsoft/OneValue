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

void onPFAddCommand(ClientPacket*, void*);
void onPFCountCommand(ClientPacket*, void*);
void onPFMergeCommand(ClientPacket*, void*);

void onFlushdbCommand(ClientPacket*, void*);
void onPingCommand(ClientPacket*, void*);

void onShowCommand(ClientPacket*, void*);
void onSyncCommand(ClientPacket*, void*);       //__sync [filename] [last update pos]
void onCopyCommand(ClientPacket*, void*);       //__copy [local onevalue port]
void onSyncFromCommand(ClientPacket*, void*);   //syncfrom [dest ip] [dest port]

#endif
