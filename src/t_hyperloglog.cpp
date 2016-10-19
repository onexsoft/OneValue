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

#include "t_hyperloglog.h"
#include "util/murmur3.h"

#define HLL_HASH_SEED 313

THyperLogLog::THyperLogLog(uint8_t precision, std::string origin_register)
{
    b_ = precision;
    m_ = 1 << precision;
    alpha_ = Alpha();
    register_ = new char[m_];
    for(uint32_t i = 0; i < m_; ++i)
        register_[i] = 0;
    if(origin_register != "")
        for (uint32_t i = 0; i < m_; ++i) {
            register_[i] = origin_register[i];
        }
}

THyperLogLog::~THyperLogLog()
{
    delete [] register_;
}

std::string THyperLogLog::Add(const char * str, uint32_t len)
{
    uint32_t hash;
    MurmurHash3_x86_32(str, len, HLL_HASH_SEED, (void*) &hash);
    int index = hash & ((1 << b_) - 1);
    uint8_t rank = Nclz((hash << b_), 32 - b_);
    if (rank > register_[index])
        register_[index] = rank;
    std::string result_str(m_, 0);
    for (uint32_t i = 0; i < m_; ++i) {
        result_str[i] = register_[i];
    }
    return result_str;
}

double THyperLogLog::Estimate() const
{
    double estimate = FirstEstimate();
    
    if (estimate <= 2.5* m_)
    {
        uint32_t zeros = CountZero();
        if(zeros != 0)
        {
            estimate = m_ * log((double)m_ / zeros);
        }
    }else if(estimate > pow(2, 32) / 30.0)
    {
        estimate = log1p(estimate * -1 / pow(2, 32)) * pow(2, 32) * -1;
    }
    return estimate;
}

double THyperLogLog::FirstEstimate() const
{
    double estimate, sum = 0.0;
    for (uint32_t i = 0; i < m_; i++)
        sum += 1.0 / (1 << register_[i]);
    
    estimate = alpha_ * m_ * m_ / sum;
    return estimate;
}

double THyperLogLog::Alpha() const
{
    switch (m_) {
        case 16:
            return 0.673;
        case 32:
            return 0.697;
        case 64:
            return 0.709;
        default:
            return 0.7213/(1 + 1.079 / m_);
    }
}

int THyperLogLog::CountZero() const
{
    int count = 0;
    for(uint32_t i = 0;i < m_; i++)
    {
        if (register_[i] == 0) {
            count++;
        }
    }
    return count;
}

std::string THyperLogLog::Merge(const THyperLogLog & hll)
{
    if (m_ != hll.m_) {
        // TODO: ERROR "number of registers doesn't match"
    }
    for (uint32_t r = 0; r < m_; r++) {
        if (register_[r] < hll.register_[r]) {
            register_[r] |= hll.register_[r];
        }
    }
    
    std::string result_str(m_, 0);
    for (uint32_t i = 0; i < m_; ++i) {
        result_str[i] = register_[i];
    }
    return result_str;
}

//::__builtin_clz(x): 返回左起第一个‘1’之前0的个数
uint8_t THyperLogLog::Nclz(uint32_t x, int b)
{
    return (uint8_t)std::min(b, ::__builtin_clz(x)) + 1;
}
