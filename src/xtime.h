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
typedef x_uint64_t xtime_unsec_t;

/**
 * @struct xtime_descr_t
 * @brief  时间描述信息的联合体类型（共计 64 位）。
 * @note   该结构体用于描述 1970-01-01 00:00:00 往后的时间。
 */
typedef union xtime_descr_t
{
    /** 对齐的 64位 整数值 */
    x_uint64_t ctx_value;

    /** 描述信息的上下文描述结构体 */
    struct
    {
    x_uint32_t ctx_year   : 16;  ///< 年（1970 ~ ）
    x_uint32_t ctx_month  :  6;  ///< 月（1 ~ 12）
    x_uint32_t ctx_day    :  6;  ///< 日（1 ~ 31）
    x_uint32_t ctx_week   :  4;  ///< 周几（0 ~ 6）
    x_uint32_t ctx_hour   :  6;  ///< 时（0 ~ 23）
    x_uint32_t ctx_minute :  6;  ///< 分（0 ~ 59）
    x_uint32_t ctx_second :  6;  ///< 秒（0 ~ 59）
    x_uint32_t ctx_msec   : 14;  ///< 毫秒（0 ~ 999）
    };
} xtime_descr_t;

#define XTMDESCR_LEAP_YEAR(Y)       ((0 == (Y) % 400) || ((0 == (Y) % 4) && (0 != (Y) % 100)))

/** 定义无效的 时间计量值 */
#define XTIME_INVALID_UNSEC         ((xtime_unsec_t)~0ULL)

/** 判断 时间计量值 是否为 有效 */
#define XTMUNSEC_IS_VALID(xunsec)   (XTIME_INVALID_UNSEC != (xunsec))

/** 判断 时间描述信息 是否为 有效 */
#define XTMDESCR_IS_VALID(xdescr)   time_descr_valid(xdescr)

//====================================================================

// 
// 相关的操作接口
// 

/**********************************************************/
/**
 * @brief 获取当前系统的 时间计量值。
 */
xtime_unsec_t time_unsec(void);

/**********************************************************/
/**
 * @brief 获取当前系统的 时间描述信息。
 */
xtime_descr_t time_descr(void);

/**********************************************************/
/**
 * @brief 将 时间描述信息 转换为 时间计量值。
 * 
 * @param [in ] xtm_descr : 待转换的 时间描述信息。
 * 
 * @return xtime_unsec_t : 
 * 返回 时间计量值，可用 XTMUNSEC_IS_VALID() 判断其是否为有效。
 */
xtime_unsec_t time_dtou(xtime_descr_t xtm_descr);

/**********************************************************/
/**
 * @brief 将 时间计量值 转换为 时间描述信息。
 * 
 * @param [in ] xtm_unsec : 待转换的 时间计量值。
 * 
 * @return xtime_descr_t : 
 * 返回 时间描述信息，可用 XTMDESCR_IS_VALID() 判断其是否为有效。
 */
xtime_descr_t time_utod(xtime_unsec_t xtm_unsec);

/**********************************************************/
/**
 * @brief 判断 时间描述信息 是否有效。
 */
x_bool_t time_descr_valid(xtime_descr_t xtm_descr);

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}; // extern "C"
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////

#endif // __XTIME_H__
