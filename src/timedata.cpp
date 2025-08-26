// Copyright (c) 2014-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "timedata.h"

#include "netaddress.h"
#include "sync.h"
#include "ui_interface.h"
#include "util.h"
#include "utilstrencodings.h"
#include "warnings.h"


/*
// 创建时间偏移过滤器
CMedianFilter<int64_t> timeFilter(11, 0);  // 11个样本，初始值0

// 添加时间数据
timeFilter.input(120);   // +2分钟
timeFilter.input(-60);   // -1分钟
timeFilter.input(180);   // +3分钟

// 获取中位数
int64_t median = timeFilter.median();  // 返回 +2分钟

// 获取调整后的时间
int64_t adjustedTime = GetAdjustedTime();
 */








 
static CCriticalSection cs_nTimeOffset;
static int64_t nTimeOffset = 0;

/**
 * 三源时间策略：
 * 系统时钟：本地时间
 * 中位数：其他节点的时间
 * 用户：用户手动设置时间
 * 避免依赖单一时间源，使用多个时间源进行交叉验证
 * 
 * "Never go to sea with two chronometers; take one or three."
 * Our three time sources are:
 *  - System clock
 *  - Median of other nodes clocks
 *  - The user (asking the user to fix the system clock if the first two disagree)
 */
int64_t GetTimeOffset()
{
    LOCK(cs_nTimeOffset);
    return nTimeOffset;
}

int64_t GetAdjustedTime()
{
    return GetTime() + GetTimeOffset();
}

static int64_t abs64(int64_t n)
{
    return (n >= 0 ? n : -n);
}

#define BITCOIN_TIMEDATA_MAX_SAMPLES 200

void AddTimeData(const CNetAddr& ip, int64_t nOffsetSample)
{
    LOCK(cs_nTimeOffset);
    // Ignore duplicates  // 1. 去重：每个 IP 只记录一次
    static std::set<CNetAddr> setKnown;
    if (setKnown.size() == BITCOIN_TIMEDATA_MAX_SAMPLES)
        return;
    if (!setKnown.insert(ip).second)
        return;

    // Add data  // 2. 添加到中位数过滤器
    static CMedianFilter<int64_t> vTimeOffsets(BITCOIN_TIMEDATA_MAX_SAMPLES, 0);
    vTimeOffsets.input(nOffsetSample);
    LogPrint(BCLog::NET,"added time data, samples %d, offset %+d (%+d minutes)\n", vTimeOffsets.size(), nOffsetSample, nOffsetSample/60);

    // There is a known issue here (see issue #4521):
    //
    // - The structure vTimeOffsets contains up to 200 elements, after which
    // any new element added to it will not increase its size, replacing the
    // oldest element.
    //
    // - The condition to update nTimeOffset includes checking whether the
    // number of elements in vTimeOffsets is odd, which will never happen after
    // there are 200 elements.
    //
    // But in this case the 'bug' is protective against some attacks, and may
    // actually explain why we've never seen attacks which manipulate the
    // clock offset.
    //
    // So we should hold off on fixing this and clean it up as part of
    // a timing cleanup that strengthens it in a number of other ways.
    //
    if (vTimeOffsets.size() >= 5 && vTimeOffsets.size() % 2 == 1)
    {
        int64_t nMedian = vTimeOffsets.median();
        std::vector<int64_t> vSorted = vTimeOffsets.sorted();
        // Only let other nodes change our time by so much   // 限制调整幅度
        if (abs64(nMedian) <= std::max<int64_t>(0, GetArg("-maxtimeadjustment", DEFAULT_MAX_TIME_ADJUSTMENT)))
        {
            nTimeOffset = nMedian;   // 更新偏移量


            /* 更新条件：
至少收集 5 个样本
样本数量为奇数（确保中位数计算准确）
偏移量在允许范围内
             */
        }
        else
        {
            nTimeOffset = 0;

            static bool fDone;
            if (!fDone)
            {
                // If nobody has a time different than ours but within 5 minutes of ours, give a warning
                // 如果所有节点时间都与本地差异很大，发出警告
                bool fMatch = false;
                for (int64_t nOffset : vSorted)
                    if (nOffset != 0 && abs64(nOffset) < 5 * 60)
                        fMatch = true;

                if (!fMatch)
                {
                    // 显示警告：请检查系统时间
                    fDone = true;
                    std::string strMessage = strprintf(_("Please check that your computer's date and time are correct! If your clock is wrong, %s will not work properly."), _(PACKAGE_NAME));
                    SetMiscWarning(strMessage);
                    uiInterface.ThreadSafeMessageBox(strMessage, "", CClientUIInterface::MSG_WARNING);
                }
            }
        }

        if (LogAcceptCategory(BCLog::NET)) {
            for (int64_t n : vSorted) {
                LogPrint(BCLog::NET, "%+d  ", n);
            }
            LogPrint(BCLog::NET, "|  ");

            LogPrint(BCLog::NET, "nTimeOffset = %+d  (%+d minutes)\n", nTimeOffset, nTimeOffset/60);
        }
    }
}
