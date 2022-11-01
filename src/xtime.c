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

/** 1601 ~ 1970 年之间的时间 百纳秒数 */
#define XTIME_UNSEC_1601_1970 116444736000000000LL

//====================================================================

// 
// 外部相关操作接口
// 

/**********************************************************/
/**
 * @brief 获取当前系统的 时间计量值。
 */
xtime_unsec_t time_unsec(void)
{
    xtime_unsec_t xtm_unsec = XTIME_INVALID_UNSEC;

#if (defined(_WIN32) || defined(_WIN64))

    FILETIME       xtime_file;
    ULARGE_INTEGER xtime_value;

    GetSystemTimeAsFileTime(&xtime_file);
    xtime_value.LowPart  = xtime_file.dwLowDateTime;
    xtime_value.HighPart = xtime_file.dwHighDateTime;

    xtm_unsec = (xtime_unsec_t)(xtime_value.QuadPart - XTIME_UNSEC_1601_1970);

#elif (defined(__linux__) || defined(__unix__))

    struct timeval xtm_value;
    gettimeofday(&xtm_value, X_NULL);

    xtm_unsec = (xtime_unsec_t)(xtm_value.tv_sec * 10000000ULL + xtm_value.tv_usec * 10ULL);

#else // UNKNOW
#error "unknow platform!"
#endif // PLATFORM

    return xtm_unsec;
}

/**********************************************************/
/**
 * @brief 获取当前系统的 时间描述信息。
 */
xtime_descr_t time_descr(void)
{
    return time_utod(time_unsec());
}

/**********************************************************/
/**
 * @brief 将 时间描述信息 转换为 时间计量值。
 * 
 * @param [in ] xtm_descr : 待转换的 时间描述信息。
 * 
 * @return xtime_unsec_t : 
 * 返回 时间计量值，可用 XTIME_UNSEC_INVALID() 判断其是否为无效值。
 */
xtime_unsec_t time_dtou(xtime_descr_t xtm_descr)
{
    xtime_unsec_t xtm_unsec = XTIME_INVALID_UNSEC;

#if 0
    if ((xtm_descr.ctx_year   < 1970) ||
        (xtm_descr.ctx_month  <    1) || (xtm_descr.ctx_month > 12) ||
        (xtm_descr.ctx_day    <    1) || (xtm_descr.ctx_day   > 31) ||
        (xtm_descr.ctx_hour   >   23) ||
        (xtm_descr.ctx_minute >   59) ||
        (xtm_descr.ctx_second >   59) ||
        (xtm_descr.ctx_msec   >  999))
    {
        return XTIME_INVALID_METER;
    }
#endif

#if (defined(_WIN32) || defined(_WIN64))

    ULARGE_INTEGER xtime_value;
    FILETIME       xtime_sfile;
    FILETIME       xtime_lfile;
    SYSTEMTIME     xtime_system;

    xtime_system.wYear         = xtm_descr.ctx_year  ;
    xtime_system.wMonth        = xtm_descr.ctx_month ;
    xtime_system.wDay          = xtm_descr.ctx_day   ;
    xtime_system.wDayOfWeek    = xtm_descr.ctx_week  ;
    xtime_system.wHour         = xtm_descr.ctx_hour  ;
    xtime_system.wMinute       = xtm_descr.ctx_minute;
    xtime_system.wSecond       = xtm_descr.ctx_second;
    xtime_system.wMilliseconds = xtm_descr.ctx_msec  ;

    if (SystemTimeToFileTime(&xtime_system, &xtime_lfile))
    {
        if (LocalFileTimeToFileTime(&xtime_lfile, &xtime_sfile))
        {
            xtime_value.LowPart  = xtime_sfile.dwLowDateTime ;
            xtime_value.HighPart = xtime_sfile.dwHighDateTime;

            xtm_unsec = (xtime_unsec_t)(xtime_value.QuadPart - XTIME_UNSEC_1601_1970);
        }
    }

#elif (defined(__linux__) || defined(__unix__))

    struct tm      xtm_system;
    struct timeval xtm_value;

    xtm_system.tm_sec   = xtm_descr.ctx_second;
    xtm_system.tm_min   = xtm_descr.ctx_minute;
    xtm_system.tm_hour  = xtm_descr.ctx_hour  ;
    xtm_system.tm_mday  = xtm_descr.ctx_day   ;
    xtm_system.tm_mon   = xtm_descr.ctx_month - 1   ;
    xtm_system.tm_year  = xtm_descr.ctx_year  - 1900;
    xtm_system.tm_wday  = 0;
    xtm_system.tm_yday  = 0;
    xtm_system.tm_isdst = 0;

    xtm_value.tv_sec  = mktime(&xtm_system);
    xtm_value.tv_usec = xtm_descr.ctx_msec * 1000;
    if (-1 != xtm_value.tv_sec)
    {
        xtm_unsec = (xtime_unsec_t)(xtm_value.tv_sec * 10000000ULL + xtm_value.tv_usec * 10ULL);
    }

#else // UNKNOW
#error "unknow platform!"
#endif // PLATFORM

    return xtm_unsec;
}

/**********************************************************/
/**
 * @brief 将 时间计量值 转换为 时间描述信息。
 * 
 * @param [in ] xtm_unsec : 待转换的 时间计量值。
 * 
 * @return xtime_descr_t : 
 * 返回 时间描述信息，可用 XTIME_DESCR_INVALID() 判断其是否为无效。
 */
xtime_descr_t time_utod(xtime_unsec_t xtm_unsec)
{
    xtime_descr_t xtm_descr = { XTIME_INVALID_DESCR };

#if (defined(_WIN32) || defined(_WIN64))

    ULARGE_INTEGER xtime_value;
    FILETIME       xtime_sfile;
    FILETIME       xtime_lfile;
    SYSTEMTIME     xtime_system;

    xtime_value.QuadPart       = xtm_unsec + XTIME_UNSEC_1601_1970;
    xtime_sfile.dwLowDateTime  = xtime_value.LowPart;
    xtime_sfile.dwHighDateTime = xtime_value.HighPart;
    if (!FileTimeToLocalFileTime(&xtime_sfile, &xtime_lfile))
    {
        return xtm_descr;
    }

    if (!FileTimeToSystemTime(&xtime_lfile, &xtime_system))
    {
        return xtm_descr;
    }

    xtm_descr.ctx_year   = xtime_system.wYear        ;
    xtm_descr.ctx_month  = xtime_system.wMonth       ;
    xtm_descr.ctx_day    = xtime_system.wDay         ;
    xtm_descr.ctx_week   = xtime_system.wDayOfWeek   ;
    xtm_descr.ctx_hour   = xtime_system.wHour        ;
    xtm_descr.ctx_minute = xtime_system.wMinute      ;
    xtm_descr.ctx_second = xtime_system.wSecond      ;
    xtm_descr.ctx_msec   = xtime_system.wMilliseconds;

#elif (defined(__linux__) || defined(__unix__))

    struct tm xtm_system;
    time_t xtm_time = (time_t)(xtm_unsec / 10000000ULL);
    localtime_r(&xtm_time, &xtm_system);

    xtm_descr.ctx_year   = xtm_system.tm_year + 1900;
    xtm_descr.ctx_month  = xtm_system.tm_mon  + 1   ;
    xtm_descr.ctx_day    = xtm_system.tm_mday       ;
    xtm_descr.ctx_week   = xtm_system.tm_wday       ;
    xtm_descr.ctx_hour   = xtm_system.tm_hour       ;
    xtm_descr.ctx_minute = xtm_system.tm_min        ;
    xtm_descr.ctx_second = xtm_system.tm_sec        ;
    xtm_descr.ctx_msec   = (x_uint32_t)((xtm_unsec % 10000000ULL) / 10000L);

#else // UNKNOW
#error "unknow platform!"
#endif // PLATFORM

    return xtm_descr;
}

////////////////////////////////////////////////////////////////////////////////

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif // __GNUC__

////////////////////////////////////////////////////////////////////////////////

