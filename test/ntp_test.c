﻿/**
 * @file ntp_test.c
 * Copyright (c) 2018 Gaaagaa. All rights reserved.
 * 
 * @author  : Gaaagaa
 * @date    : 2018-10-19
 * @version : 1.0.0.0
 * @brief   : 使用 NTP 协议获取网络时间戳的测试程序。
 */

#include "ntp_client.h"

#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#endif // defined(_WIN32) || defined(_WIN64)

#include <string.h>
#include <stdio.h>
#include <errno.h>

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char * argv[])
{
    xtime_unsec_t xtm_unsec = XTIME_INVALID_UNSEC;
    xtime_descr_t xtm_descr = { XTIME_INVALID_DESCR };
    xtime_descr_t xtm_system = { XTIME_INVALID_DESCR };

    //======================================
    // 常用的 NTP 服务器地址列表

    x_int32_t   xit_iter = 0;
    x_cstring_t xszt_host[] =
    {
        "ntp.tencent.com" ,
        "ntp1.tencent.com",
        "ntp2.tencent.com",
        "ntp3.tencent.com",
        "ntp4.tencent.com",
        "ntp5.tencent.com",
        "ntp1.aliyun.com" ,
        "ntp2.aliyun.com" ,
        "ntp3.aliyun.com" ,
        "ntp4.aliyun.com" ,
        "ntp5.aliyun.com" ,
        "ntp6.aliyun.com" ,
        "ntp7.aliyun.com" ,
        "time.edu.cn"     ,
        "s2c.time.edu.cn" ,
        "s2f.time.edu.cn" ,
        "s2k.time.edu.cn" ,
        X_NULL
    };

    //======================================

#if defined(_WIN32) || defined(_WIN64)
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 0), &wsaData);
#endif // defined(_WIN32) || defined(_WIN64)

    //======================================

    for (xit_iter = 0; X_NULL != xszt_host[xit_iter]; ++xit_iter)
    {
        xtm_unsec = ntp_get_time(xszt_host[xit_iter], NTP_PORT, 5000);
        if (!XTIME_UNSEC_INVALID(xtm_unsec))
        {
            xtm_system = time_descr();
            xtm_descr = time_utod(xtm_unsec);

            printf("%-16s : [0x%016llX, 0x%016llX][%04d-%02d-%02d %d %02d:%02d:%02d.%03d] "
                                               "- [%04d-%02d-%02d %d %02d:%02d:%02d.%03d]\n",
                   xszt_host[xit_iter]  ,
                   xtm_unsec           ,
                   xtm_descr.ctx_value ,
                   xtm_descr.ctx_year  ,
                   xtm_descr.ctx_month ,
                   xtm_descr.ctx_day   ,
                   xtm_descr.ctx_week  ,
                   xtm_descr.ctx_hour  ,
                   xtm_descr.ctx_minute,
                   xtm_descr.ctx_second,
                   xtm_descr.ctx_msec  ,
                   xtm_system.ctx_year  ,
                   xtm_system.ctx_month ,
                   xtm_system.ctx_day   ,
                   xtm_system.ctx_week  ,
                   xtm_system.ctx_hour  ,
                   xtm_system.ctx_minute,
                   xtm_system.ctx_second,
                   xtm_system.ctx_msec  );
        }
        else
        {
            printf("%-16s : errno = %d\n", xszt_host[xit_iter], errno);
        }
    }

    //======================================

#if defined(_WIN32) || defined(_WIN64)
    WSACleanup();
#endif // defined(_WIN32) || defined(_WIN64)

    //======================================

    return 0;
}