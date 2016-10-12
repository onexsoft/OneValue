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

#ifndef T_REDIS_H
#define T_REDIS_H

#include <string>
#include <list>

#include "util/string.h"

enum {
    T_KV = 100,
    T_List,
    T_ListElement,
    T_Set,
    T_ZSet,
    T_Hash,
    T_Ttl
};

typedef std::list<std::string> stringlist;

class TRedisHelper
{
public:
    static bool isInteger(const char* buff, int len);
    static bool isInteger(const std::string& s)
    { return isInteger(s.data(), s.size()); }
    static bool isDouble(const char* buff, int len);
    static bool isDouble(const std::string& s)
    { return isDouble(s.data(), s.size()); }
    static int doubleToString(char* buff, double f, bool clearZero = true);
};

#endif
