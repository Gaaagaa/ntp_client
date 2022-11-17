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

/** 常用的 NTP 服务器地址列表 */
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

/** 网络地址（IP 或 域名）类型 */
typedef x_char_t x_host_t[TEXT_LEN_256];

/**
 * @struct xopt_args_t
 * @brief  命令选项的各个参数。
 */
typedef struct xopt_args_t
{
    x_bool_t   xbt_usage; ///< 是否显示帮助信息
    x_uint16_t xut_port;  ///< NTP 服务器端口号（默认值为 123）
    x_host_t   xntp_host; ///< NTP 服务器地址
    x_int32_t  xit_rept;  ///< 请求重复次数（默认值为 1）
    x_uint32_t xut_tmout; ///< 网络请求的超时时间（单位为 毫秒，默认值取 3000）
} xopt_args_t;

/** 简单的判断 xopt_args_t 的有效性 */
#define XOPT_VALID(xopt) (('\0' != (xopt).xntp_host[0]) && ((xopt).xit_rept > 0))

////////////////////////////////////////////////////////////////////////////////

/**********************************************************/
/**
 * @brief 字符串忽略大小写的比对操作。
 *
 * @param [in ] xszt_lcmp : 比较操作的左值字符串。
 * @param [in ] xszt_rcmp : 比较操作的右值字符串。
 *
 * @return x_int32_t
 *         - xszt_lcmp <  xszt_rcmp，返回 <= -1；
 *         - xszt_lcmp == xszt_rcmp，返回 ==  0；
 *         - xszt_lcmp >  xszt_rcmp，返回 >=  1；
 */
static x_int32_t xstr_icmp(x_cstring_t xszt_lcmp, x_cstring_t xszt_rcmp)
{
    x_int32_t xit_lvalue = 0;
    x_int32_t xit_rvalue = 0;

    if (xszt_lcmp == xszt_rcmp)
        return 0;
    if (X_NULL == xszt_lcmp)
        return -1;
    if (X_NULL == xszt_rcmp)
        return 1;

    do
    {
        if (((xit_lvalue = (*(xszt_lcmp++))) >= 'A') && (xit_lvalue <= 'Z'))
            xit_lvalue -= ('A' - 'a');

        if (((xit_rvalue = (*(xszt_rcmp++))) >= 'A') && (xit_rvalue <= 'Z'))
            xit_rvalue -= ('A' - 'a');

    } while (xit_lvalue && (xit_lvalue == xit_rvalue));

    return (xit_lvalue - xit_rvalue);
}

/**********************************************************/
/**
 * @brief 显示应用程序的 命令行格式。
 */
x_void_t usage(x_cstring_t xszt_app)
{
    x_int32_t xit_iter = 1;

    printf("Usage:\n %s [-h] [-n <number>] -s <host> [-p <port>] [-t <msec>]\n", xszt_app);
    printf("\t-h          Output usage.\n");
    printf("\t-n <number> The times of repetition.\n");
    printf("\t-s <host>   The host of NTP server, IP or domain.\n");
    printf("\t-p <port>   The port of NTP server, default 123.\n");
    printf("\t-t <msec>   Network request timeout in milliseconds, default 3000.\n");

    printf("\nCommon NTP server:\n");

    for (xit_iter = 0; X_NULL != xszt_host[xit_iter]; ++xit_iter)
    {
        printf("\t%s\n", xszt_host[xit_iter]);
    }

    printf("\n");
}

/**********************************************************/
/**
 * @brief 从命令行中，提取工作的选项参数信息。
 */
x_void_t get_opt(x_int32_t xit_argc, x_cstring_t xszt_argv[], xopt_args_t * xopt_args)
{
    x_int32_t xit_iter  = 1;

    memset(xopt_args, 0, sizeof(xopt_args_t));
    xopt_args->xbt_usage = X_FALSE;
    xopt_args->xut_port  = NTP_PORT;
    xopt_args->xit_rept  = 1;
    xopt_args->xut_tmout = 3000;

    for (; xit_iter < xit_argc; ++xit_iter)
    {
        if (!xopt_args->xbt_usage && (0 == xstr_icmp("-h", xszt_argv[xit_iter])))
        {
            xopt_args->xbt_usage = X_TRUE;
        }
        else if (0 == xstr_icmp("-s", xszt_argv[xit_iter]))
        {
            if ((xit_iter + 1) < xit_argc)
            {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996)
#endif // _MSC_VER
                strcpy(xopt_args->xntp_host, xszt_argv[++xit_iter]);
#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER
            }
        }
        else if (0 == xstr_icmp("-n", xszt_argv[xit_iter]))
        {
            if ((xit_iter + 1) < xit_argc)
                xopt_args->xit_rept = atoi(xszt_argv[++xit_iter]);
        }
        else if (0 == xstr_icmp("-p", xszt_argv[xit_iter]))
        {
            if ((xit_iter + 1) < xit_argc)
                xopt_args->xut_port = (x_uint16_t)atoi(xszt_argv[++xit_iter]);
        }
        else if (0 == xstr_icmp("-t", xszt_argv[xit_iter]))
        {
            if ((xit_iter + 1) < xit_argc)
                xopt_args->xut_tmout = (x_uint32_t)atoi(xszt_argv[++xit_iter]);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

// 
// 程序入口的主函数
// 

int main(int argc, char * argv[])
{
    xopt_args_t   xopt_args;
    x_int32_t     xit_iter  = 0;
    xntp_cliptr_t xntp_this = X_NULL;

    xtime_vnsec_t xtm_vnsec = XTIME_INVALID_VNSEC;
    xtime_vnsec_t xtm_ltime = XTIME_INVALID_VNSEC;
    xtime_descr_t xtm_descr = { 0 };
    xtime_descr_t xtm_local = { 0 };

#if defined(_WIN32) || defined(_WIN64)
    WSADATA xwsa_data;
    WSAStartup(MAKEWORD(2, 0), &xwsa_data);
#endif // defined(_WIN32) || defined(_WIN64)

    //======================================

    do
    {
        //======================================

        get_opt(argc, argv, &xopt_args);
        if (!XOPT_VALID(xopt_args))
        {
            usage(argv[0]);
            break;
        }

        if (xopt_args.xbt_usage)
        {
            usage(argv[0]);
        }

        //======================================

        xntp_this = ntpcli_open();
        if (X_NULL == xntp_this)
        {
            printf("ntpcli_open() return X_NULL, errno : %d\n", errno);
            break;
        }

        ntpcli_config(xntp_this, xopt_args.xntp_host, xopt_args.xut_port);

        for (xit_iter = 0; xit_iter < xopt_args.xit_rept; ++xit_iter)
        {
            xtm_vnsec = ntpcli_req_time(xntp_this, xopt_args.xut_tmout);
            if (XTMVNSEC_IS_VALID(xtm_vnsec))
            {
                xtm_ltime = time_vnsec();
                xtm_descr = time_vtod(xtm_vnsec);
                xtm_local = time_vtod(xtm_ltime);

                printf("\n[%d] %s:%d : \n",
                       xit_iter + 1,
                       xopt_args.xntp_host,
                       xopt_args.xut_port);
                printf("\tNTP response : [ %04d-%02d-%02d %d %02d:%02d:%02d.%03d ]\n",
                       xtm_descr.ctx_year  ,
                       xtm_descr.ctx_month ,
                       xtm_descr.ctx_day   ,
                       xtm_descr.ctx_week  ,
                       xtm_descr.ctx_hour  ,
                       xtm_descr.ctx_minute,
                       xtm_descr.ctx_second,
                       xtm_descr.ctx_msec  );

                printf("\tLocal time   : [ %04d-%02d-%02d %d %02d:%02d:%02d.%03d ]\n",
                       xtm_local.ctx_year  ,
                       xtm_local.ctx_month ,
                       xtm_local.ctx_day   ,
                       xtm_local.ctx_week  ,
                       xtm_local.ctx_hour  ,
                       xtm_local.ctx_minute,
                       xtm_local.ctx_second,
                       xtm_local.ctx_msec  );

                printf("\tDeviation    : %lld us\n",
                       ((x_int64_t)(xtm_ltime - xtm_vnsec)) / 10LL);
            }
            else
            {
                printf("\n[%d] %s:%d : errno = %d\n",
                       xit_iter + 1,
                       xopt_args.xntp_host,
                       xopt_args.xut_port,
                       errno);
            }
        }

        //======================================
    } while (0);

    if (X_NULL != xntp_this)
    {
        ntpcli_close(xntp_this);
        xntp_this = X_NULL;
    }

    //======================================

#if defined(_WIN32) || defined(_WIN64)
    WSACleanup();
#endif // defined(_WIN32) || defined(_WIN64)

    //======================================

    return 0;
}
