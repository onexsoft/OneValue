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

