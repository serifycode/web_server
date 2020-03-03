#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include "copyable.h"
#include <string>
#include <stdint.h>

class Timestamp :public copyable
{
public:
    Timestamp()
    :microSecondsSinceEpoch_(0)
    {}
    explicit Timestamp(int64_t microSecondsSinceEpochArg)
        : microSecondsSinceEpoch_(microSecondsSinceEpochArg)
    {}
    static Timestamp now();
    std::string toFormattedString(bool showMicroseconds = true) const;//返回年月时间
    static const int kMicroSecondsPerSecond = 1000 * 1000;
private:
    int64_t microSecondsSinceEpoch_; 
};


#endif 