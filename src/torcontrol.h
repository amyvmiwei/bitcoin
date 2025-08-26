// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Functionality for communicating with Tor.
 */
#ifndef BITCOIN_TORCONTROL_H
#define BITCOIN_TORCONTROL_H

#include "scheduler.h"



/*
torcontrol.h 是 Bitcoin Core 中的Tor 网络控制模块，主要负责：
Tor 隐藏服务管理：创建和管理 .onion 地址
Tor 控制协议：与 Tor 守护进程通信
匿名网络集成：让 Bitcoin 节点可以通过 Tor 网络运行

*/

extern const std::string DEFAULT_TOR_CONTROL;       // 默认 Tor 控制端口
static const bool DEFAULT_LISTEN_ONION = true;      // 默认启用洋葱地址监听


/*
功能：启动 Tor 控制服务
创建事件循环基础结构
启动专门的 Tor 控制线程
初始化与 Tor 守护进程的连接
*/

void StartTorControl(boost::thread_group& threadGroup, CScheduler& scheduler);



/*
中断 Tor 控制服务
停止事件循环
允许优雅关闭
*/
void InterruptTorControl();


/*
停止 Tor 控制服务
等待线程完成
清理资源
*/ 
void StopTorControl();

#endif /* BITCOIN_TORCONTROL_H */
