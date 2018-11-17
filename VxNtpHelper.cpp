/**
 * @file    VxNtpHelper.cpp
 * <pre>
 * Copyright (c) 2018, Gaaagaa All rights reserved.
 *
 * 文件名称：VxNtpHelper.cpp
 * 创建日期：2018年10月19日
 * 文件标识：
 * 文件摘要：使用 NTP 协议时的一些辅助函数接口以及相关的数据定义。
 *
 * 当前版本：1.0.0.0
 * 作    者：
 * 完成日期：2018年10月19日
 * 版本摘要：
 *
 * 取代版本：
 * 原作者  ：
 * 完成日期：
 * 版本摘要：
 * </pre>
 */

#include "VxNtpHelper.h"

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#include <time.h>
#else // !_WIN32
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#endif // _WIN32

#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
// 定义测试信息输出接口的操作宏

#if NTP_OUTPUT

#include <stdio.h>

static inline x_void_t ts_output(x_cstring_t xszt_name, const x_ntp_time_context_t * const xtm_ctxt)
{
    printf("\t%s : %04d-%02d-%02d_%02d-%02d-%02d.%03d\n",
        xszt_name           ,
        xtm_ctxt->xut_year  ,
        xtm_ctxt->xut_month ,
        xtm_ctxt->xut_day   ,
        xtm_ctxt->xut_hour  ,
        xtm_ctxt->xut_minute,
        xtm_ctxt->xut_second,
        xtm_ctxt->xut_msec  );
}

static inline x_void_t tn_output(x_cstring_t xszt_name, const x_ntp_timestamp_t * const xtm_stamp)
{
    x_ntp_time_context_t xtm_ctxt;
    ntp_tmctxt_ts(xtm_stamp, &xtm_ctxt);
    ts_output(xszt_name, &xtm_ctxt);
}

static inline x_void_t tv_output(x_cstring_t xszt_name, const x_ntp_timeval_t * const xtm_value)
{
    x_ntp_time_context_t xtm_ctxt;
    ntp_tmctxt_tv(xtm_value, &xtm_ctxt);
    ts_output(xszt_name, &xtm_ctxt);
}

static inline x_void_t bv_output(x_cstring_t xszt_name, x_uint64_t xut_time)
{
    x_ntp_time_context_t xtm_ctxt;
    ntp_tmctxt_bv(xut_time, &xtm_ctxt);
    ts_output(xszt_name, &xtm_ctxt);
}

#define XOUTLINE(szformat, ...)           do { printf((szformat), ##__VA_ARGS__); printf("\n"); } while (0)
#define TS_OUTPUT(xszt_name, xtm_ctxt )   ts_output((xszt_name), (xtm_ctxt ))
#define TN_OUTPUT(xszt_name, xtm_stamp)   tn_output((xszt_name), (xtm_stamp))
#define TV_OUTPUT(xszt_name, xtm_value)   tv_output((xszt_name), (xtm_value))
#define BV_OUTPUT(xszt_name, xut_time )   bv_output((xszt_name), (xut_time ))

#else // !NTP_OUTPUT

#define XOUTLINE(szformat, ...)
#define TS_OUTPUT(xszt_name, xtm_ctxt )
#define TN_OUTPUT(xszt_name, xtm_stamp)
#define TV_OUTPUT(xszt_name, xtm_value)
#define BV_OUTPUT(xszt_name, xut_time )

#endif // NTP_OUTPUT

////////////////////////////////////////////////////////////////////////////////

#define JAN_1970     0x83AA7E80             ///< 1900 ~ 1970 年之间的时间 秒数
#define NS100_1970   116444736000000000LL   ///< 1601 ~ 1970 年之间的时间 百纳秒数

/**
 * @enum  em_ntp_mode_t
 * @brief NTP工作模式的相关枚举值。
 */
typedef enum em_ntp_mode_t
{
    ntp_mode_unknow     = 0,  ///< 未定义
    ntp_mode_initiative = 1,  ///< 主动对等体模式
    ntp_mode_passive    = 2,  ///< 被动对等体模式
    ntp_mode_client     = 3,  ///< 客户端模式
    ntp_mode_server     = 4,  ///< 服务器模式
    ntp_mode_broadcast  = 5,  ///< 广播模式或组播模式
    ntp_mode_control    = 6,  ///< 报文为 NTP 控制报文
    ntp_mode_reserved   = 7,  ///< 预留给内部使用
} em_ntp_mode_t;

/**
 * @struct x_ntp_packet_t
 * @brief  NTP 报文格式。
 */
typedef struct x_ntp_packet_t
{
    x_uchar_t         xct_li_ver_mode;      ///< 2 bits，飞跃指示器；3 bits，版本号；3 bits，NTP工作模式（参看 em_ntp_mode_t 相关枚举值）
    x_uchar_t         xct_stratum    ;      ///< 系统时钟的层数，取值范围为1~16，它定义了时钟的准确度。层数为1的时钟准确度最高，准确度从1到16依次递减，层数为16的时钟处于未同步状态，不能作为参考时钟
    x_uchar_t         xct_poll       ;      ///< 轮询时间，即两个连续NTP报文之间的时间间隔
    x_uchar_t         xct_percision  ;      ///< 系统时钟的精度

    x_uint32_t        xut_root_delay     ;  ///< 本地到主参考时钟源的往返时间
    x_uint32_t        xut_root_dispersion;  ///< 系统时钟相对于主参考时钟的最大误差
    x_uint32_t        xut_ref_indentifier;  ///< 参考时钟源的标识

    x_ntp_timestamp_t xtmst_reference;      ///< 系统时钟最后一次被设定或更新的时间（应答完成后，用于存储 T1）
    x_ntp_timestamp_t xtmst_originate;      ///< NTP请求报文离开发送端时发送端的本地时间（应答完成后，用于存储 T4）
    x_ntp_timestamp_t xtmst_receive  ;      ///< NTP请求报文到达接收端时接收端的本地时间（应答完成后，用于存储 T2）
    x_ntp_timestamp_t xtmst_transmit ;      ///< NTP应答报文离开应答者时应答者的本地时间（应答完成后，用于存储 T3）
} x_ntp_packet_t;

////////////////////////////////////////////////////////////////////////////////

/**********************************************************/
/**
 * @brief 将 x_ntp_timeval_t 转换为 x_ntp_timestamp_t 。
 */
static inline x_void_t ntp_timeval_to_timestamp(x_ntp_timestamp_t * xtm_timestamp, const x_ntp_timeval_t * const xtm_timeval)
{
    const x_lfloat_t xlft_frac_per_ms = 4.294967296E6;  // 2^32 / 1000

    xtm_timestamp->xut_seconds  = (x_uint32_t)(xtm_timeval->tv_sec  + JAN_1970);
    xtm_timestamp->xut_fraction = (x_uint32_t)(xtm_timeval->tv_usec / 1000.0 * xlft_frac_per_ms);
}

/**********************************************************/
/**
 * @brief 将 x_ntp_timeval_t 转换为 x_ntp_timestamp_t 。
 */
static inline x_void_t ntp_timestamp_to_timeval(x_ntp_timeval_t * xtm_timeval, const x_ntp_timestamp_t * const xtm_timestamp)
{
    const x_lfloat_t xlft_frac_per_ms = 4.294967296E6;  // 2^32 / 1000

    if (xtm_timestamp->xut_seconds >= JAN_1970)
    {
        xtm_timeval->tv_sec  = (x_long_t)(xtm_timestamp->xut_seconds - JAN_1970);
        xtm_timeval->tv_usec = (x_long_t)(xtm_timestamp->xut_fraction / xlft_frac_per_ms * 1000.0);
    }
    else
    {
        xtm_timeval->tv_sec  = 0;
        xtm_timeval->tv_usec = 0;
    }
}

/**********************************************************/
/**
 * @brief 将 x_ntp_timeval_t 转换成 100纳秒为单位的值。
 */
static inline x_uint64_t ntp_timeval_ns100(const x_ntp_timeval_t * const xtm_timeval)
{
    return (10000000ULL * xtm_timeval->tv_sec + 10ULL * xtm_timeval->tv_usec);
}

/**********************************************************/
/**
 * @brief 将 x_ntp_timeval_t 转换成 毫秒值。
 */
static inline x_uint64_t ntp_timeval_ms(const x_ntp_timeval_t * const xtm_timeval)
{
    return (1000ULL * xtm_timeval->tv_sec + (x_uint64_t)(xtm_timeval->tv_usec / 1000.0 + 0.5));
}

/**********************************************************/
/**
 * @brief 将 x_ntp_timestamp_t 转换成 100纳秒为单位的值。
 */
static inline x_uint64_t ntp_timestamp_ns100(const x_ntp_timestamp_t * const xtm_timestamp)
{
    x_ntp_timeval_t xmt_timeval;
    ntp_timestamp_to_timeval(&xmt_timeval, xtm_timestamp);
    return ntp_timeval_ns100(&xmt_timeval);
}

/**********************************************************/
/**
 * @brief 将 x_ntp_timestamp_t 转换成 毫秒值。
 */
static inline x_uint64_t ntp_timestamp_ms(const x_ntp_timestamp_t * const xtm_timestamp)
{
    x_ntp_timeval_t xmt_timeval;
    ntp_timestamp_to_timeval(&xmt_timeval, xtm_timestamp);
    return ntp_timeval_ms(&xmt_timeval);
}

////////////////////////////////////////////////////////////////////////////////

/**********************************************************/
/**
 * @brief 获取当前系统的 时间值（以 100纳秒 为单位，1970年1月1日到现在的时间）。
 */
x_uint64_t ntp_gettimevalue(void)
{
#ifdef _WIN32
    FILETIME       xtime_file;
    ULARGE_INTEGER xtime_value;

    GetSystemTimeAsFileTime(&xtime_file);
    xtime_value.LowPart  = xtime_file.dwLowDateTime;
    xtime_value.HighPart = xtime_file.dwHighDateTime;

    return (x_uint64_t)(xtime_value.QuadPart - NS100_1970);
#else // !_WIN32
    struct timeval tmval;
    gettimeofday(&tmval, X_NULL);

    return (10000000ULL * tmval.tv_sec + 10ULL * tmval.tv_usec);
#endif // _WIN32
}

/**********************************************************/
/**
 * @brief 获取当前系统的 timeval 值（1970年1月1日到现在的时间）。
 */
x_void_t ntp_gettimeofday(x_ntp_timeval_t * xtm_value)
{
#ifdef _WIN32
    FILETIME       xtime_file;
    ULARGE_INTEGER xtime_value;

    GetSystemTimeAsFileTime(&xtime_file);
    xtime_value.LowPart  = xtime_file.dwLowDateTime;
    xtime_value.HighPart = xtime_file.dwHighDateTime;

    xtm_value->tv_sec  = (x_long_t)((xtime_value.QuadPart - NS100_1970) / 10000000LL); // 1970年以来的秒数
    xtm_value->tv_usec = (x_long_t)((xtime_value.QuadPart / 10LL      ) % 1000000LL ); // 微秒
#else // !_WIN32
    struct timeval tmval;
    gettimeofday(&tmval, X_NULL);

    xtm_value->tv_sec  = tmval.tv_sec ;
    xtm_value->tv_usec = tmval.tv_usec;
#endif // _WIN32
}

/**********************************************************/
/**
 * @brief 将 x_ntp_time_context_t 转换为 以 100纳秒
 *        为单位的时间值（1970年1月1日到现在的时间）。
 */
x_uint64_t ntp_time_value(x_ntp_time_context_t * xtm_context)
{
    x_uint64_t xut_time = 0ULL;

#if 0
    if ((xtm_context->xut_year   < 1970) ||
        (xtm_context->xut_month  <    1) || (xtm_context->xut_month > 12) ||
        (xtm_context->xut_day    <    1) || (xtm_context->xut_day   > 31) ||
        (xtm_context->xut_hour   >   23) ||
        (xtm_context->xut_minute >   59) ||
        (xtm_context->xut_second >   59) ||
        (xtm_context->xut_msec   >  999))
    {
        return xut_time;
    }
#endif

#ifdef _WIN32
    ULARGE_INTEGER xtime_value;
    FILETIME       xtime_sysfile;
    FILETIME       xtime_locfile;
    SYSTEMTIME     xtime_system;

    xtime_system.wYear         = xtm_context->xut_year  ;
    xtime_system.wMonth        = xtm_context->xut_month ;
    xtime_system.wDay          = xtm_context->xut_day   ;
    xtime_system.wDayOfWeek    = xtm_context->xut_week  ;
    xtime_system.wHour         = xtm_context->xut_hour  ;
    xtime_system.wMinute       = xtm_context->xut_minute;
    xtime_system.wSecond       = xtm_context->xut_second;
    xtime_system.wMilliseconds = xtm_context->xut_msec  ;

    if (SystemTimeToFileTime(&xtime_system, &xtime_locfile))
    {
        if (LocalFileTimeToFileTime(&xtime_locfile, &xtime_sysfile))
        {
            xtime_value.LowPart  = xtime_sysfile.dwLowDateTime ;
            xtime_value.HighPart = xtime_sysfile.dwHighDateTime;
            xut_time = xtime_value.QuadPart - NS100_1970;
        }
    }
#else // !_WIN32
    struct tm       xtm_system;
    x_ntp_timeval_t xtm_value;

    xtm_system.tm_sec   = xtm_context->xut_second;
    xtm_system.tm_min   = xtm_context->xut_minute;
    xtm_system.tm_hour  = xtm_context->xut_hour  ;
    xtm_system.tm_mday  = xtm_context->xut_day   ;
    xtm_system.tm_mon   = xtm_context->xut_month - 1   ;
    xtm_system.tm_year  = xtm_context->xut_year  - 1900;
    xtm_system.tm_wday  = 0;
    xtm_system.tm_yday  = 0;
    xtm_system.tm_isdst = 0;

    xtm_value.tv_sec  = mktime(&xtm_system);
    xtm_value.tv_usec = xtm_context->xut_msec * 1000;
    if (-1 != xtm_value.tv_sec)
    {
        xut_time = ntp_timeval_ns100(&xtm_value);
    }
#endif // _WIN32

    return xut_time;
}

/**********************************************************/
/**
 * @brief 转换（以 100纳秒 为单位的）时间值（1970年1月1日到现在的时间）
 *        为具体的时间描述信息（即 x_ntp_time_context_t）。
 *
 * @param [in ] xut_time    : 时间值（1970年1月1日到现在的时间）。
 * @param [out] xtm_context : 操作成功返回的时间描述信息。
 *
 * @return x_bool_t
 *         - 成功，返回 X_TRUE；
 *         - 失败，返回 X_FALSE。
 */
x_bool_t ntp_tmctxt_bv(x_uint64_t xut_time, x_ntp_time_context_t * xtm_context)
{
#ifdef _WIN32
    ULARGE_INTEGER xtime_value;
    FILETIME       xtime_sysfile;
    FILETIME       xtime_locfile;
    SYSTEMTIME     xtime_system;

    if (X_NULL == xtm_context)
    {
        return X_FALSE;
    }

    xtime_value.QuadPart = xut_time + NS100_1970;
    xtime_sysfile.dwLowDateTime  = xtime_value.LowPart;
    xtime_sysfile.dwHighDateTime = xtime_value.HighPart;
    if (!FileTimeToLocalFileTime(&xtime_sysfile, &xtime_locfile))
    {
        return X_FALSE;
    }

    if (!FileTimeToSystemTime(&xtime_locfile, &xtime_system))
    {
        return X_FALSE;
    }

    xtm_context->xut_year   = xtime_system.wYear        ;
    xtm_context->xut_month  = xtime_system.wMonth       ;
    xtm_context->xut_day    = xtime_system.wDay         ;
    xtm_context->xut_week   = xtime_system.wDayOfWeek   ;
    xtm_context->xut_hour   = xtime_system.wHour        ;
    xtm_context->xut_minute = xtime_system.wMinute      ;
    xtm_context->xut_second = xtime_system.wSecond      ;
    xtm_context->xut_msec   = xtime_system.wMilliseconds;
#else // !_WIN32
    struct tm xtm_system;
    time_t xtm_time = (time_t)(xut_time / 10000000ULL);
    localtime_r(&xtm_time, &xtm_system);

    xtm_context->xut_year   = xtm_system.tm_year + 1900;
    xtm_context->xut_month  = xtm_system.tm_mon  + 1   ;
    xtm_context->xut_day    = xtm_system.tm_mday       ;
    xtm_context->xut_week   = xtm_system.tm_wday       ;
    xtm_context->xut_hour   = xtm_system.tm_hour       ;
    xtm_context->xut_minute = xtm_system.tm_min        ;
    xtm_context->xut_second = xtm_system.tm_sec        ;
    xtm_context->xut_msec   = (x_uint32_t)((xut_time % 10000000ULL) / 10000L);
#endif // _WIN32

    return X_TRUE;
}

/**********************************************************/
/**
 * @brief 转换（x_ntp_timeval_t 类型的）时间值
 *        为具体的时间描述信息（即 x_ntp_time_context_t）。
 *
 * @param [in ] xtm_value   : 时间值。
 * @param [out] xtm_context : 操作成功返回的时间描述信息。
 *
 * @return x_bool_t
 *         - 成功，返回 X_TRUE；
 *         - 失败，返回 X_FALSE。
 */
x_bool_t ntp_tmctxt_tv(const x_ntp_timeval_t * const xtm_value, x_ntp_time_context_t * xtm_context)
{
    return ntp_tmctxt_bv(ntp_timeval_ns100(xtm_value), xtm_context);
}

/**********************************************************/
/**
 * @brief 转换（x_ntp_timeval_t 类型的）时间值
 *        为具体的时间描述信息（即 x_ntp_time_context_t）。
 *
 * @param [in ] xtm_timestamp : 时间值。
 * @param [out] xtm_context   : 操作成功返回的时间描述信息。
 *
 * @return x_bool_t
 *         - 成功，返回 X_TRUE；
 *         - 失败，返回 X_FALSE。
 */
x_bool_t ntp_tmctxt_ts(const x_ntp_timestamp_t * const xtm_timestamp, x_ntp_time_context_t * xtm_context)
{
    return ntp_tmctxt_bv(ntp_timestamp_ns100(xtm_timestamp), xtm_context);
}

////////////////////////////////////////////////////////////////////////////////

/**********************************************************/
/**
 * @brief 判断字符串是否为有效的 4 段式 IP 地址格式。
 *
 * @param [in ] xszt_vptr : 判断的字符串。
 * @param [out] xut_value : 若入参不为 X_NULL，则操作成功时，返回对应的 IP 地址值。
 *
 * @return x_bool_t
 *         - 成功，返回 X_TRUE；
 *         - 失败，返回 X_FALSE。
 */
static x_bool_t ntp_ipv4_valid(x_cstring_t xszt_vptr, x_uint32_t * xut_value)
{
    x_uchar_t xct_ipv[4] = { 0, 0, 0, 0 };

    x_int32_t xit_itv = 0;
    x_int32_t xit_sum = 0;
    x_bool_t  xbt_okv = X_FALSE;

    x_char_t    xct_iter = '\0';
    x_cchar_t * xct_iptr = xszt_vptr;

    if (X_NULL == xszt_vptr)
    {
        return X_FALSE;
    }

    for (xct_iter = *xszt_vptr; X_TRUE; xct_iter = *(++xct_iptr))
    {
        if ((xct_iter != '\0') && (xct_iter >= '0') && (xct_iter <= '9'))
        {
            xit_sum = 10 * xit_sum + (xct_iter - '0');
            xbt_okv = X_TRUE;
        }
        else if (xbt_okv && (('\0' == xct_iter) || ('.' == xct_iter)) && (xit_itv < (x_int32_t)sizeof(xct_ipv)) && (xit_sum <= 0xFF))
        {
            xct_ipv[xit_itv++] = xit_sum;
            xit_sum = 0;
            xbt_okv = X_FALSE;
        }
        else
            break;

        if ('\0' == xct_iter)
        {
            break;
        }
    }

#define MAKE_IPV4_VALUE(b1,b2,b3,b4)  ((x_uint32_t)(((x_uint32_t)(b1)<<24)+((x_uint32_t)(b2)<<16)+((x_uint32_t)(b3)<<8)+((x_uint32_t)(b4))))

    xbt_okv = (xit_itv == sizeof(xct_ipv)) ? X_TRUE : X_FALSE;
    if (X_NULL != xut_value)
    {
        *xut_value = xbt_okv ? MAKE_IPV4_VALUE(xct_ipv[0], xct_ipv[1], xct_ipv[2], xct_ipv[3]) : 0xFFFFFFFF;
    }

#undef MAKE_IPV4_VALUE

    return xbt_okv;
}

/**********************************************************/
/**
 * @brief 获取 域名下的 IP 地址表（取代系统的 gethostbyname() API 调用）。
 *
 * @param [in ] xszt_dname : 指定的域名（格式如：www.163.com）。
 * @param [in ] xit_family : 期待返回的套接口地址结构的类型。
 * @param [out] xvec_host  : 操作成功返回的地址列表。
 *
 * @return x_int32_t
 *         - 成功，返回 0；
 *         - 失败，返回 错误码。
 */
static x_int32_t ntp_gethostbyname(x_cstring_t xszt_dname, x_int32_t xit_family, std::vector< std::string > & xvec_host)
{
    x_int32_t xit_err = -1;

    struct addrinfo   xai_hint;
    struct addrinfo * xai_rptr = X_NULL;
    struct addrinfo * xai_iptr = X_NULL;

    x_char_t xszt_iphost[TEXT_LEN_256] = { 0 };

    do
    {
        //======================================

        if (X_NULL == xszt_dname)
        {
            break;
        }

        memset(&xai_hint, 0, sizeof(xai_hint));
        xai_hint.ai_family   = xit_family;
        xai_hint.ai_socktype = SOCK_DGRAM;

        xit_err = getaddrinfo(xszt_dname, X_NULL, &xai_hint, &xai_rptr);
        if (0 != xit_err)
        {
            break;
        }

        //======================================

        for (xai_iptr = xai_rptr; X_NULL != xai_iptr; xai_iptr = xai_iptr->ai_next)
        {
            if (xit_family != xai_iptr->ai_family)
            {
                continue;
            }

            memset(xszt_iphost, 0, TEXT_LEN_256);
            if (X_NULL == inet_ntop(xit_family, &(((struct sockaddr_in *)(xai_iptr->ai_addr))->sin_addr), xszt_iphost, TEXT_LEN_256))
            {
                continue;
            }

            xvec_host.push_back(std::string(xszt_iphost));
        }

        //======================================

        xit_err = (xvec_host.size() > 0) ? 0 : -3;
    } while (0);

    if (X_NULL != xai_rptr)
    {
        freeaddrinfo(xai_rptr);
        xai_rptr = X_NULL;
    }

    return xit_err;
}

/**********************************************************/
/**
 * @brief 返回套接字当前操作失败的错误码。
 */
static x_int32_t ntp_sockfd_lasterror()
{
#ifdef _WIN32
    return (x_int32_t)WSAGetLastError();
#else // !_WIN32
    return errno;
#endif // _WIN32
}

/**********************************************************/
/**
 * @brief 关闭套接字。
 */
static x_int32_t ntp_sockfd_close(x_sockfd_t xfdt_sockfd)
{
#ifdef _WIN32
    return closesocket(xfdt_sockfd);
#else // !_WIN32
    return close(xfdt_sockfd);
#endif // _WIN32
}

/**********************************************************/
/**
 * @brief 初始化 NTP 的请求数据包。
 */
static x_void_t ntp_init_request_packet(x_ntp_packet_t * xnpt_dptr)
{
    const x_uchar_t xct_leap_indicator = 0;
    const x_uchar_t xct_ntp_version    = 3;
    const x_uchar_t xct_ntp_mode       = ntp_mode_client;

    xnpt_dptr->xct_li_ver_mode = (xct_leap_indicator << 6) | (xct_ntp_version << 3) | (xct_ntp_mode << 0);
    xnpt_dptr->xct_stratum     = 0;
    xnpt_dptr->xct_poll        = 4;
    xnpt_dptr->xct_percision   = ((-6) & 0xFF);

    xnpt_dptr->xut_root_delay      = (1 << 16);
    xnpt_dptr->xut_root_dispersion = (1 << 16);
    xnpt_dptr->xut_ref_indentifier = 0;

    xnpt_dptr->xtmst_reference.xut_seconds  = 0;
    xnpt_dptr->xtmst_reference.xut_fraction = 0;
    xnpt_dptr->xtmst_originate.xut_seconds  = 0;
    xnpt_dptr->xtmst_originate.xut_fraction = 0;
    xnpt_dptr->xtmst_receive  .xut_seconds  = 0;
    xnpt_dptr->xtmst_receive  .xut_fraction = 0;
    xnpt_dptr->xtmst_transmit .xut_seconds  = 0;
    xnpt_dptr->xtmst_transmit .xut_fraction = 0;
}

/**********************************************************/
/**
 * @brief 将 x_ntp_packet_t 中的 网络字节序 字段转换为 主机字节序。
 */
static x_void_t ntp_ntoh_packet(x_ntp_packet_t * xnpt_nptr)
{
#if 0
    xnpt_nptr->xct_li_ver_mode = xnpt_nptr->xct_li_ver_mode;
    xnpt_nptr->xct_stratum     = xnpt_nptr->xct_stratum    ;
    xnpt_nptr->xct_poll        = xnpt_nptr->xct_poll       ;
    xnpt_nptr->xct_percision   = xnpt_nptr->xct_percision  ;
#endif
    xnpt_nptr->xut_root_delay               = ntohl(xnpt_nptr->xut_root_delay              );
    xnpt_nptr->xut_root_dispersion          = ntohl(xnpt_nptr->xut_root_dispersion         );
    xnpt_nptr->xut_ref_indentifier          = ntohl(xnpt_nptr->xut_ref_indentifier         );
    xnpt_nptr->xtmst_reference.xut_seconds  = ntohl(xnpt_nptr->xtmst_reference.xut_seconds );
    xnpt_nptr->xtmst_reference.xut_fraction = ntohl(xnpt_nptr->xtmst_reference.xut_fraction);
    xnpt_nptr->xtmst_originate.xut_seconds  = ntohl(xnpt_nptr->xtmst_originate.xut_seconds );
    xnpt_nptr->xtmst_originate.xut_fraction = ntohl(xnpt_nptr->xtmst_originate.xut_fraction);
    xnpt_nptr->xtmst_receive  .xut_seconds  = ntohl(xnpt_nptr->xtmst_receive  .xut_seconds );
    xnpt_nptr->xtmst_receive  .xut_fraction = ntohl(xnpt_nptr->xtmst_receive  .xut_fraction);
    xnpt_nptr->xtmst_transmit .xut_seconds  = ntohl(xnpt_nptr->xtmst_transmit .xut_seconds );
    xnpt_nptr->xtmst_transmit .xut_fraction = ntohl(xnpt_nptr->xtmst_transmit .xut_fraction);
}

/**********************************************************/
/**
 * @brief 将 x_ntp_packet_t 中的 主机字节序 字段转换为 网络字节序。
 */
static x_void_t ntp_hton_packet(x_ntp_packet_t * xnpt_nptr)
{
#if 0
    xnpt_nptr->xct_li_ver_mode = xnpt_nptr->xct_li_ver_mode;
    xnpt_nptr->xct_stratum     = xnpt_nptr->xct_stratum    ;
    xnpt_nptr->xct_poll        = xnpt_nptr->xct_poll       ;
    xnpt_nptr->xct_percision   = xnpt_nptr->xct_percision  ;
#endif
    xnpt_nptr->xut_root_delay               = htonl(xnpt_nptr->xut_root_delay              );
    xnpt_nptr->xut_root_dispersion          = htonl(xnpt_nptr->xut_root_dispersion         );
    xnpt_nptr->xut_ref_indentifier          = htonl(xnpt_nptr->xut_ref_indentifier         );
    xnpt_nptr->xtmst_reference.xut_seconds  = htonl(xnpt_nptr->xtmst_reference.xut_seconds );
    xnpt_nptr->xtmst_reference.xut_fraction = htonl(xnpt_nptr->xtmst_reference.xut_fraction);
    xnpt_nptr->xtmst_originate.xut_seconds  = htonl(xnpt_nptr->xtmst_originate.xut_seconds );
    xnpt_nptr->xtmst_originate.xut_fraction = htonl(xnpt_nptr->xtmst_originate.xut_fraction);
    xnpt_nptr->xtmst_receive  .xut_seconds  = htonl(xnpt_nptr->xtmst_receive  .xut_seconds );
    xnpt_nptr->xtmst_receive  .xut_fraction = htonl(xnpt_nptr->xtmst_receive  .xut_fraction);
    xnpt_nptr->xtmst_transmit .xut_seconds  = htonl(xnpt_nptr->xtmst_transmit .xut_seconds );
    xnpt_nptr->xtmst_transmit .xut_fraction = htonl(xnpt_nptr->xtmst_transmit .xut_fraction);
}

/**********************************************************/
/**
 * @brief 向 NTP 服务器发送 NTP 请求，获取相关计算所需的时间戳（T1、T2、T3、T4如下所诉）。
 * <pre>
 *     1. 客户端 发送一个NTP报文给 服务端，该报文带有它离开 客户端 时的时间戳，该时间戳为 T1。
 *     2. 当此NTP报文到达 服务端 时，服务端 加上自己的时间戳，该时间戳为 T2。
 *     3. 当此NTP报文离开 服务端 时，服务端 再加上自己的时间戳，该时间戳为 T3。
 *     4. 当 客户端 接收到该应答报文时，客户端 的本地时间戳，该时间戳为 T4。
 * </pre>
 *
 * @param [in ] xszt_host : NTP 服务器的 IP（四段式 IP 地址）。
 * @param [in ] xut_port  : NTP 服务器的 端口号（可取默认的端口号 NTP_PORT : 123）。
 * @param [in ] xut_tmout : 超时时间（单位 毫秒）。
 * @param [out] xit_tmlst : 操作成功返回的相关计算所需的时间戳（T1、T2、T3、T4）。
 *
 * @return x_int32_t
 *         - 成功，返回 0；
 *         - 失败，返回 错误码。
 */
static x_int32_t ntp_get_time_values(x_cstring_t xszt_host, x_uint16_t xut_port, x_uint32_t xut_tmout, x_int64_t xit_tmlst[4])
{
    x_int32_t xit_err = -1;

    x_sockfd_t      xfdt_sockfd = X_INVALID_SOCKFD;
    x_ntp_packet_t  xnpt_buffer;
    x_ntp_timeval_t xtm_value;

    x_int32_t xit_addrlen = sizeof(struct sockaddr_in);
    struct sockaddr_in skaddr_host;

    do 
    {
        //======================================

        if ((X_NULL == xszt_host) || (xut_tmout <= 0) || (X_NULL == xit_tmlst))
        {
            break;
        }

        //======================================

        xfdt_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (X_INVALID_SOCKFD == xfdt_sockfd)
        {
            break;
        }

        // 设置 发送/接收 超时时间
#ifdef _WIN32
        setsockopt(xfdt_sockfd, SOL_SOCKET, SO_SNDTIMEO, (x_char_t *)&xut_tmout, sizeof(x_uint32_t));
        setsockopt(xfdt_sockfd, SOL_SOCKET, SO_RCVTIMEO, (x_char_t *)&xut_tmout, sizeof(x_uint32_t));
#else // !_WIN32
        xtm_value.tv_sec  = (x_long_t)((xut_tmout / 1000));
        xtm_value.tv_usec = (x_long_t)((xut_tmout % 1000) * 1000);
        setsockopt(xfdt_sockfd, SOL_SOCKET, SO_SNDTIMEO, (x_char_t *)&xtm_value, sizeof(x_ntp_timeval_t));
        setsockopt(xfdt_sockfd, SOL_SOCKET, SO_RCVTIMEO, (x_char_t *)&xtm_value, sizeof(x_ntp_timeval_t));
#endif // _WIN32

        // 服务端主机地址
        memset(&skaddr_host, 0, sizeof(struct sockaddr_in));
        skaddr_host.sin_family = AF_INET;
        skaddr_host.sin_port   = htons(xut_port);
        inet_pton(AF_INET, xszt_host, &skaddr_host.sin_addr.s_addr);

        //======================================

        // 初始化请求数据包
        ntp_init_request_packet(&xnpt_buffer);

        // NTP请求报文离开发送端时发送端的本地时间
        ntp_gettimeofday(&xtm_value);
        ntp_timeval_to_timestamp(&xnpt_buffer.xtmst_originate, &xtm_value);

        // T1
        xit_tmlst[0] = (x_int64_t)ntp_timeval_ns100(&xtm_value);

        // 转成网络字节序
        ntp_hton_packet(&xnpt_buffer);

        // 投递请求
        xit_err = sendto(xfdt_sockfd,
                         (x_char_t *)&xnpt_buffer,
                         sizeof(x_ntp_packet_t),
                         0,
                         (sockaddr *)&skaddr_host,
                         sizeof(struct sockaddr_in));
        if (xit_err < 0)
        {
            xit_err = ntp_sockfd_lasterror();
            continue;
        }

        //======================================

        memset(&xnpt_buffer, 0, sizeof(x_ntp_packet_t));

        // 接收应答
        xit_err = recvfrom(xfdt_sockfd,
                           (x_char_t *)&xnpt_buffer,
                           sizeof(x_ntp_packet_t),
                           0,
                           (sockaddr *)&skaddr_host,
                           (socklen_t *)&xit_addrlen);
        if (xit_err < 0)
        {
            xit_err = ntp_sockfd_lasterror();
            break;
        }

        if (sizeof(x_ntp_packet_t) != xit_err)
        {
            xit_err = -1;
            break;
        }

        // T4
        xit_tmlst[3] = (x_int64_t)ntp_gettimevalue();

        // 转成主机字节序
        ntp_ntoh_packet(&xnpt_buffer);

        xit_tmlst[1] = (x_int64_t)ntp_timestamp_ns100(&xnpt_buffer.xtmst_receive ); // T2
        xit_tmlst[2] = (x_int64_t)ntp_timestamp_ns100(&xnpt_buffer.xtmst_transmit); // T3

        //======================================
        xit_err = 0;
    } while (0);

    if (X_INVALID_SOCKFD != xfdt_sockfd)
    {
        ntp_sockfd_close(xfdt_sockfd);
        xfdt_sockfd = X_INVALID_SOCKFD;
    }

    return xit_err;
}

/**********************************************************/
/**
 * @brief 向 NTP 服务器发送 NTP 请求，获取服务器时间戳。
 * 
 * @param [in ] xszt_host : NTP 服务器的 IP（四段式 IP 地址） 或 域名（如 3.cn.pool.ntp.org）。
 * @param [in ] xut_port  : NTP 服务器的 端口号（可取默认的端口号 NTP_PORT : 123）。
 * @param [in ] xut_tmout : 网络请求的超时时间（单位为毫秒）。
 * @param [out] xut_timev : 操作成功返回的应答时间值（以 100纳秒 为单位，1970年1月1日到现在的时间）。
 * 
 * @return x_int32_t
 *         - 成功，返回 0；
 *         - 失败，返回 错误码。
 */
x_int32_t ntp_get_time(x_cstring_t xszt_host, x_uint16_t xut_port, x_uint32_t xut_tmout, x_uint64_t * xut_timev)
{
    x_int32_t xit_err = -1;
    std::vector< std::string > xvec_host;

    x_int64_t xit_tmlst[4] = { 0 };

    //======================================
    // 参数验证

    if ((X_NULL == xszt_host) || (xut_tmout <= 0) || (X_NULL == xut_timev))
    {
        return -1;
    }

    //======================================
    // 获取 IP 地址列表

    if (ntp_ipv4_valid(xszt_host, X_NULL))
    {
        xvec_host.push_back(std::string(xszt_host));
    }
    else
    {
        xit_err = ntp_gethostbyname(xszt_host, AF_INET, xvec_host);
        if (0 != xit_err)
        {
            return xit_err;
        }
    }

    if (xvec_host.empty())
    {
        return -1;
    }

    //======================================

    for (std::vector< std::string >::iterator itvec = xvec_host.begin(); itvec != xvec_host.end(); ++itvec)
    {
        XOUTLINE("========================================");
        XOUTLINE("  %s -> %s\n", xszt_host, itvec->c_str());

        xit_err = ntp_get_time_values(itvec->c_str(), xut_port, xut_tmout, xit_tmlst);
        if (0 == xit_err)
        {
            // T = T4 + ((T2 - T1) + (T3 - T4)) / 2;
            *xut_timev = xit_tmlst[3] + ((xit_tmlst[1] - xit_tmlst[0]) + (xit_tmlst[2] - xit_tmlst[3])) / 2;

            BV_OUTPUT("time1", xit_tmlst[0]);
            BV_OUTPUT("time2", xit_tmlst[1]);
            BV_OUTPUT("time3", xit_tmlst[2]);
            BV_OUTPUT("time4", xit_tmlst[3]);
            BV_OUTPUT("timev", *xut_timev);
            BV_OUTPUT("timec", ntp_gettimevalue());

            break;
        }
    }

    //======================================

    return xit_err;
}

