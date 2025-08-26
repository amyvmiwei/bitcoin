// Copyright (c) 2014-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TIMEDATA_H
#define BITCOIN_TIMEDATA_H

#include <algorithm>
#include <assert.h>
#include <stdint.h>
#include <vector>


// 文件概述
//timedata.h 是 Bitcoin Core 中的时间数据管理模块，主要负责：
//网络时间同步：与对等节点的时间同步
//时间偏移计算：计算本地时间与网络时间的差异
//中位数过滤：使用统计方法过滤异常时间数据


static const int64_t DEFAULT_MAX_TIME_ADJUSTMENT = 70 * 60;     // 70分钟

class CNetAddr;

/** 
 * 对数据流进行中位数过滤，返回最近 N 个数值的中位数
 * 
 * Median filter over a stream of values.
 * Returns the median of the last N numbers
 */
template <typename T>
class CMedianFilter
{
private:
    std::vector<T> vValues;  // 原始值序列
    std::vector<T> vSorted;  // 排序后的值序列
    unsigned int nSize;      // 过滤器大小

public:
    CMedianFilter(unsigned int _size, T initial_value) : nSize(_size)
    {
        vValues.reserve(_size);                 // 预分配内存
        vValues.push_back(initial_value);        // 添加初始值
        vSorted = vValues;                       // 初始化排序序列
    }

    void input(T value)
    {
        //维护固定大小的滑动窗口
        if (vValues.size() == nSize) {
            vValues.erase(vValues.begin());
        }
        vValues.push_back(value);                // 添加新值

        vSorted.resize(vValues.size());          // 调整排序序列大小
        std::copy(vValues.begin(), vValues.end(), vSorted.begin());
        std::sort(vSorted.begin(), vSorted.end());
    }

    T median() const
    {
        int vSortedSize = vSorted.size();
        assert(vSortedSize > 0);
        if (vSortedSize & 1) // 奇数个元素
        {
            return vSorted[vSortedSize / 2];
        } else // 偶数个元素
        {
            return (vSorted[vSortedSize / 2 - 1] + vSorted[vSortedSize / 2]) / 2;
        }
    }

    int size() const
    {
        return vValues.size();
    }

    std::vector<T> sorted() const
    {
        return vSorted;
    }
};

/** Functions to keep track of adjusted P2P time */
int64_t GetTimeOffset();                //获取本地时间与网络时间的偏移量
int64_t GetAdjustedTime();              //获取调整后的时间（本地时间 + 偏移量）
void AddTimeData(const CNetAddr& ip, int64_t nTime); //添加来自特定 IP 节点的时间数据

#endif // BITCOIN_TIMEDATA_H
