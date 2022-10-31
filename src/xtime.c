/**
 * @file xtime.c
 * Copyright (c) 2015 Gaaagaa. All rights reserved.
 * 
 * @author  : Gaaagaa
 * @date    : 2015-12-07
 * @version : 1.0.0.0
 * @brief   : 实现 时间的相关辅助 函数接口。
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

#include "xtime.h"

#if (defined(_WIN32) || defined(_WIN64))
#include <windows.h>
#include <time.h>
#elif (defined(__linux__) || defined(__unix__))
#include <sys/time.h>
#include <time.h>
#else // UNKNOW
#error "unknow platform!"
#endif // PLATFORM

////////////////////////////////////////////////////////////////////////////////

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif // __GNUC__

////////////////////////////////////////////////////////////////////////////////

//====================================================================

// 
// 内部相关数据类型与常量
// 

/** 1900 ~ 1970 年之间的时间 秒数 */
#define JAN_1970     0x83AA7E80

/** 1601 ~ 1970 年之间的时间 百纳秒数 */
#define NS100_1970   116444736000000000LL

/** 无效的时间计量值 */
const xtime_meter_t XTIME_INVALID_METER = 0ULL;

/** 无效的时间描述信息 */
const xtime_cntxt_t XTIME_INVALID_CNTXT = { 0 };

//====================================================================

// 
// 外部相关操作接口
// 

/**********************************************************/
/**
 * @brief 获取当前系统的 时间计量值。
 */
xtime_meter_t time_meter(void)
{
    xtime_meter_t xtime_meter = XTIME_INVALID_METER;

#if (defined(_WIN32) || defined(_WIN64))

    FILETIME       xtime_file;
    ULARGE_INTEGER xtime_value;

    GetSystemTimeAsFileTime(&xtime_file);
    xtime_value.LowPart  = xtime_file.dwLowDateTime;
    xtime_value.HighPart = xtime_file.dwHighDateTime;

    xtime_meter = (xtime_meter_t)(xtime_value.QuadPart - NS100_1970);

#elif (defined(__linux__) || defined(__unix__))

    struct timeval tmval;
    gettimeofday(&tmval, X_NULL);

    xtime_meter = (xtime_meter_t)(10000000ULL * tmval.tv_sec + 10ULL * tmval.tv_usec);

#else // UNKNOW
#error "unknow platform!"
#endif // PLATFORM

    return xtime_meter;
}

/**********************************************************/
/**
 * @brief 获取当前系统的 时间描述信息。
 */
xtime_cntxt_t time_cntxt(void)
{
    return time_mtc(time_meter());
}

/**********************************************************/
/**
 * @brief 将 时间描述信息 转换为 时间计量值。
 * 
 * @param [in ] xtm_cntxt : 待转换的 时间描述信息。
 * 
 * @return xtime_meter_t : 
 *      - 成功，返回 时间计量值；
 *      - 失败，返回 XTIME_INVALID_METER。
 */
xtime_meter_t time_ctm(xtime_cntxt_t xtm_cntxt)
{
    xtime_meter_t xtm_meter = XTIME_INVALID_METER;

#if 0
    if ((xtm_cntxt.ctx_year   < 1970) ||
        (xtm_cntxt.ctx_month  <    1) || (xtm_cntxt.ctx_month > 12) ||
        (xtm_cntxt.ctx_day    <    1) || (xtm_cntxt.ctx_day   > 31) ||
        (xtm_cntxt.ctx_hour   >   23) ||
        (xtm_cntxt.ctx_minute >   59) ||
        (xtm_cntxt.ctx_second >   59) ||
        (xtm_cntxt.ctx_msec   >  999))
    {
        return XTIME_INVALID_METER;
    }
#endif

#if (defined(_WIN32) || defined(_WIN64))

    ULARGE_INTEGER xtime_value;
    FILETIME       xtime_sfile;
    FILETIME       xtime_lfile;
    SYSTEMTIME     xtime_system;

    xtime_system.wYear         = xtm_cntxt.ctx_year  ;
    xtime_system.wMonth        = xtm_cntxt.ctx_month ;
    xtime_system.wDay          = xtm_cntxt.ctx_day   ;
    xtime_system.wDayOfWeek    = xtm_cntxt.ctx_week  ;
    xtime_system.wHour         = xtm_cntxt.ctx_hour  ;
    xtime_system.wMinute       = xtm_cntxt.ctx_minute;
    xtime_system.wSecond       = xtm_cntxt.ctx_second;
    xtime_system.wMilliseconds = xtm_cntxt.ctx_msec  ;

    if (SystemTimeToFileTime(&xtime_system, &xtime_lfile))
    {
        if (LocalFileTimeToFileTime(&xtime_lfile, &xtime_sfile))
        {
            xtime_value.LowPart  = xtime_sfile.dwLowDateTime ;
            xtime_value.HighPart = xtime_sfile.dwHighDateTime;

            xtm_meter = (xtime_meter_t)(xtime_value.QuadPart - NS100_1970);
        }
    }

#elif (defined(__linux__) || defined(__unix__))

    struct tm      xtm_system;
    struct timeval xtm_value;

    xtm_system.tm_sec   = xtm_cntxt.ctx_second;
    xtm_system.tm_min   = xtm_cntxt.ctx_minute;
    xtm_system.tm_hour  = xtm_cntxt.ctx_hour  ;
    xtm_system.tm_mday  = xtm_cntxt.ctx_day   ;
    xtm_system.tm_mon   = xtm_cntxt.ctx_month - 1   ;
    xtm_system.tm_year  = xtm_cntxt.ctx_year  - 1900;
    xtm_system.tm_wday  = 0;
    xtm_system.tm_yday  = 0;
    xtm_system.tm_isdst = 0;

    xtm_value.tv_sec  = mktime(&xtm_system);
    xtm_value.tv_usec = xtm_cntxt.ctx_msec * 1000;
    if (-1 != xtm_value.tv_sec)
    {
        xtm_meter = (xtime_meter_t)(xtm_value.tv_sec * 10000000ULL + xtm_value.tv_usec * 10ULL);
    }

#else // UNKNOW
#error "unknow platform!"
#endif // PLATFORM

    return xtm_meter;
}

/**********************************************************/
/**
 * @brief 将 时间描述信息 转换为 时间计量值。
 * 
 * @param [in ] xtm_meter : 待转换的 时间计量值。
 * 
 * @return xtime_cntxt_t : 
 *      - 成功，返回 时间描述信息；
 *      - 失败，返回 XTIME_INVALID_CNTXT。
 */
xtime_cntxt_t time_mtc(xtime_meter_t xtm_meter)
{
    xtime_cntxt_t xtm_cntxt = XTIME_INVALID_CNTXT;

#if (defined(_WIN32) || defined(_WIN64))

    ULARGE_INTEGER xtime_value;
    FILETIME       xtime_sfile;
    FILETIME       xtime_lfile;
    SYSTEMTIME     xtime_system;

    xtime_value.QuadPart       = xtm_meter + NS100_1970;
    xtime_sfile.dwLowDateTime  = xtime_value.LowPart;
    xtime_sfile.dwHighDateTime = xtime_value.HighPart;
    if (!FileTimeToLocalFileTime(&xtime_sfile, &xtime_lfile))
    {
        return xtm_cntxt;
    }

    if (!FileTimeToSystemTime(&xtime_lfile, &xtime_system))
    {
        return xtm_cntxt;
    }

    xtm_cntxt.ctx_year   = xtime_system.wYear        ;
    xtm_cntxt.ctx_month  = xtime_system.wMonth       ;
    xtm_cntxt.ctx_day    = xtime_system.wDay         ;
    xtm_cntxt.ctx_week   = xtime_system.wDayOfWeek   ;
    xtm_cntxt.ctx_hour   = xtime_system.wHour        ;
    xtm_cntxt.ctx_minute = xtime_system.wMinute      ;
    xtm_cntxt.ctx_second = xtime_system.wSecond      ;
    xtm_cntxt.ctx_msec   = xtime_system.wMilliseconds;

#elif (defined(__linux__) || defined(__unix__))

    struct tm xtm_system;
    time_t xtm_time = (time_t)(xtm_meter / 10000000ULL);
    localtime_r(&xtm_time, &xtm_system);

    xtm_cntxt.ctx_year   = xtm_system.tm_year + 1900;
    xtm_cntxt.ctx_month  = xtm_system.tm_mon  + 1   ;
    xtm_cntxt.ctx_day    = xtm_system.tm_mday       ;
    xtm_cntxt.ctx_week   = xtm_system.tm_wday       ;
    xtm_cntxt.ctx_hour   = xtm_system.tm_hour       ;
    xtm_cntxt.ctx_minute = xtm_system.tm_min        ;
    xtm_cntxt.ctx_second = xtm_system.tm_sec        ;
    xtm_cntxt.ctx_msec   = (x_uint32_t)((xtm_meter % 10000000ULL) / 10000L);

#else // UNKNOW
#error "unknow platform!"
#endif // PLATFORM

    return xtm_cntxt;
}

////////////////////////////////////////////////////////////////////////////////

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif // __GNUC__

////////////////////////////////////////////////////////////////////////////////

