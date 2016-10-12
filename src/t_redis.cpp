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

#include "t_redis.h"

bool TRedisHelper::isInteger(const char *buff, int len)
{
    for (int i = 0; i < len; ++i) {
        char ch = buff[i];
        if (ch == '-' && i == 0) {
            continue;
        } else if (ch == '+' && i == 0) {
            continue;
        } else {
            if (ch < '0' || ch > '9') {
                return false;
            }
        }
    }
    return true;
}

bool TRedisHelper::isDouble(const char *buff, int len)
{
    bool isFindPoint = false;
    for (int i = 0; i < len; ++i) {
        char ch = buff[i];
        if (ch == '-' && i == 0) {
            continue;
        } else if (ch == '+' && i == 0) {
            continue;
        } else if (ch == '.') {
            if (isFindPoint) {
                return false;
            }
            isFindPoint = true;
        } else {
            if (ch < '0' || ch > '9') {
                return false;
            }
        }
    }
    return true;
}

int TRedisHelper::doubleToString(char *buff, double f, bool clearZero)
{
    int len = sprintf(buff, "%lf", f);
    if (clearZero) {
        int _len = len - 1;
        for (int i = _len; i != 0; --i) {
            if (buff[i] == '0') {
                buff[i] = 0;
                --len;
            } else {
                if (buff[i] == '.') {
                    buff[i] = 0;
                    --len;
                }
                break;
            }
        }
    }
    return len;
}

