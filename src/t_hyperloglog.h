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

#ifndef T_HYPERLOGLOG_H
#define T_HYPERLOGLOG_H

#include <iostream>
#include <cstdint>
#include <cmath>
#include <vector>
#include <map>
#include <algorithm>


class THyperLogLog
{
public:
    THyperLogLog(uint8_t precision, std::string origin_register);
    ~THyperLogLog();
    
    double Estimate() const;
    double FirstEstimate() const;
    int CountZero() const;
    double Alpha() const;
    uint8_t Nclz(uint32_t x, int b);
    
    std::string Add(const char * str, uint32_t len);
    std::string Merge(const THyperLogLog & hll);
    
protected:
    uint32_t m_;                    // register bit width
    uint8_t b_;                     // register size
    double alpha_;
    char * register_;               // registers
};

#endif

