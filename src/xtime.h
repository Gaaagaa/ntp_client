/**
 * @file xtime.h
 * Copyright (c) 2015 Gaaagaa. All rights reserved.
 * 
 * @author  : Gaaagaa
 * @date    : 2015-12-07
 * @version : 1.0.0.0
 * @brief   : 声明 时间的相关辅助 数据类型 与 函数接口。
 */

/**
 * The MIT License (MIT)
 * Copyright (c) Gaaagaa. All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __XTIME_H__
#define __XTIME_H__

#include "xtypes.h"

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////

//====================================================================

// 
// 相关的数据类型
// 

/** 以 100纳秒 为单位，1970-01-01 00:00:00 至今的 时间计量值 类型 */
typedef x_uint64_t xtime_meter_t;

/**
 * @struct xtime_cntxt_t
 * @brief  时间描述信息结构体（共计 64 位）。
 */
typedef union xtime_cntxt_t
{
    /** 64位值 */
    x_uint64_t ctx_value;

    /** 详细时间信息 */
    struct
    {
    x_uint32_t _year   : 16;  ///< 年
    x_uint32_t _month  :  6;  ///< 月
    x_uint32_t _day    :  6;  ///< 日
    x_uint32_t _week   :  4;  ///< 周几
    x_uint32_t _hour   :  6;  ///< 时
    x_uint32_t _minute :  6;  ///< 分
    x_uint32_t _second :  6;  ///< 秒
    x_uint32_t _msec   : 14;  ///< 毫秒
    } ctx;

#define ctx_year   ctx._year
#define ctx_month  ctx._month
#define ctx_day    ctx._day
#define ctx_week   ctx._week
#define ctx_hour   ctx._hour
#define ctx_minute ctx._minute
#define ctx_second ctx._second
#define ctx_msec   ctx._msec
} xtime_cntxt_t;

/** 无效的时间计量值 */
extern const xtime_meter_t XTIME_INVALID_METER;

/** 判断 时间计量值 是否为 无效 */
#define XTIME_METER_INVALID(xmeter)  (XTIME_INVALID_METER == (xmeter))

/** 无效的时间描述信息 */
extern const xtime_cntxt_t XTIME_INVALID_CNTXT;

/** 判断 时间描述信息 是否为 无效 */
#define XTIME_CNTXT_INVALID(xcntxt)  (XTIME_INVALID_CNTXT.ctx_value == (xcntxt).ctx_value)

//====================================================================

// 
// 相关的操作接口
// 

/**********************************************************/
/**
 * @brief 获取当前系统的 时间计量值。
 */
xtime_meter_t time_meter(void);

/**********************************************************/
/**
 * @brief 获取当前系统的 时间描述信息。
 */
xtime_cntxt_t time_cntxt(void);

/**********************************************************/
/**
 * @brief 将 时间描述信息 转换为 时间计量值。
 * 
 * @param [in ] xtm_cntxt : 待转换的 时间描述信息。
 * 
 * @return xtime_meter_t : 
 * 返回 时间计量值，可用 XTIME_METER_INVALID() 判断其是否为无效值。
 */
xtime_meter_t time_ctm(xtime_cntxt_t xtm_cntxt);

/**********************************************************/
/**
 * @brief 将 时间描述信息 转换为 时间计量值。
 * 
 * @param [in ] xtm_meter : 待转换的 时间计量值。
 * 
 * @return xtime_cntxt_t : 
 * 返回 时间描述信息，可用 XTIME_CNTXT_INVALID() 判断其是否为无效。
 */
xtime_cntxt_t time_mtc(xtime_meter_t xtm_meter);

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}; // extern "C"
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////

#endif // __XTIME_H__
