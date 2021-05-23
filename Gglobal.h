#ifndef _GGLOBAL_H
#define _GGLOBAL_H
#pragma once

/// \brief Gglbal.h
///
/// 本头文件用于预定义本项目中所有宏信息


#include <iostream>

// 用于提示dbg信息，在编译时使用

#ifdef DEBUG
#define dbginfo
#define dbgFunTrace(x) std::cout << x << "@[" << __FILE__ << ":" << __FUNCTION__ << "]" << std::endl
#else
#define dbginfo 0 &&
#define dbgFunTrace(x)
#endif

// 用于提示API弃用情况
#define G_DEPRECATED(theMsg) __attribute__((deprecated(theMsg)))



#define SHAPE_MAXHASHCODE 50000


/// \brief 用于在(u,v)面参数化采样时，设置合适的surface细分度，单位: 采样点数目/米
#define SAMPLE_RESOLUTIONS 10
#endif    // _GGLOBAL_H
