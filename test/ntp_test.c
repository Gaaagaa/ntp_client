/**
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
    xtime_meter_t xtm_meter = XTIME_INVALID_METER;
    xtime_cntxt_t xtm_cntxt = XTIME_INVALID_CNTXT;
    xtime_cntxt_t xtm_sctxt = XTIME_INVALID_CNTXT;

    //======================================
    // 常用的 NTP 服务器地址列表

    x_int32_t   xit_iter = 0;
    x_cstring_t xszt_host[] =
    {
        "1.cn.pool.ntp.org"         ,
        "2.cn.pool.ntp.org"         ,
        "3.cn.pool.ntp.org"         ,
        "0.cn.pool.ntp.org"         ,
        "cn.pool.ntp.org"           ,
        "tw.pool.ntp.org"           ,
        "0.tw.pool.ntp.org"         ,
        "1.tw.pool.ntp.org"         ,
        "2.tw.pool.ntp.org"         ,
        "3.tw.pool.ntp.org"         ,
        "pool.ntp.org"              ,
        "time.windows.com"          ,
        "time.nist.gov"             ,
        "time-nw.nist.gov"          ,
        "asia.pool.ntp.org"         ,
        "europe.pool.ntp.org"       ,
        "oceania.pool.ntp.org"      ,
        "north-america.pool.ntp.org",
        "south-america.pool.ntp.org",
        "africa.pool.ntp.org"       ,
        "ca.pool.ntp.org"           ,
        "uk.pool.ntp.org"           ,
        "us.pool.ntp.org"           ,
        "au.pool.ntp.org"           ,
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
        xtm_meter = ntp_get_time(xszt_host[xit_iter], NTP_PORT, 5000);
        if (!XTIME_METER_INVALID(xtm_meter))
        {
            xtm_sctxt = time_cntxt();
            xtm_cntxt = time_mtc(xtm_meter);

            printf("%-26s : [%04d-%02d-%02d %d %02d:%02d:%02d.%03d] "
                         "- [%04d-%02d-%02d %d %02d:%02d:%02d.%03d]\n",
                   xszt_host[xit_iter] ,
                   xtm_cntxt.ctx_year  ,
                   xtm_cntxt.ctx_month ,
                   xtm_cntxt.ctx_day   ,
                   xtm_cntxt.ctx_week  ,
                   xtm_cntxt.ctx_hour  ,
                   xtm_cntxt.ctx_minute,
                   xtm_cntxt.ctx_second,
                   xtm_cntxt.ctx_msec  ,
                   xtm_sctxt.ctx_year  ,
                   xtm_sctxt.ctx_month ,
                   xtm_sctxt.ctx_day   ,
                   xtm_sctxt.ctx_week  ,
                   xtm_sctxt.ctx_hour  ,
                   xtm_sctxt.ctx_minute,
                   xtm_sctxt.ctx_second,
                   xtm_sctxt.ctx_msec  );
        }
        else
        {
            printf("%-26s : errno = %d\n", xszt_host[xit_iter], errno);
        }
    }

    //======================================

#if defined(_WIN32) || defined(_WIN64)
    WSACleanup();
#endif // defined(_WIN32) || defined(_WIN64)

    //======================================

    return 0;
}
