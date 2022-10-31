/**
 * @file ntp_client.c
 * Copyright (c) 2018 Gaaagaa. All rights reserved.
 * 
 * @author  : Gaaagaa
 * @date    : 2018-10-19
 * @version : 1.0.0.0
 * @brief   : 使用 NTP 协议时的一些辅助函数接口以及相关的数据定义。
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

#include "ntp_client.h"

#if (defined(_WIN32) || defined(_WIN64))
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#elif (defined(__linux__) || defined(__unix__))
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
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
#define XNTP_SEC_1970 0x83AA7E80

/**
 * @struct xtime_stamp_t
 * @brief  NTP所使用的时间戳。
 */
typedef struct xtime_stamp_t
{
    x_uint32_t xut_seconds;  ///< 从 1900年至今所经过的秒数
    x_uint32_t xut_fraction; ///< 小数部份，单位是微秒数的 (2^32 / 10^6) 倍
} xtime_stamp_t;

/** 将 时间计量值 转为 NTP 时间戳 */
#define XTIME_MTOS(xmeter, xstamp)                                                        \
do                                                                                        \
{                                                                                         \
    (xstamp).xut_seconds  = (x_uint32_t)((((xmeter) / 10000000ULL) + XNTP_SEC_1970));     \
    (xstamp).xut_fraction = (x_uint32_t)((((xmeter) % 10000000ULL) << 32) / 10000000ULL); \
} while (0)

/** 将 NTP 时间戳 转为 时间计量值 */
#define XTIME_STOM(xstamp, xmeter)                                          \
do                                                                          \
{                                                                           \
    if (((xstamp).xut_seconds) > XNTP_SEC_1970)                             \
        (xmeter) = (((xstamp).xut_seconds - XNTP_SEC_1970) * 10000000ULL) + \
                   (((xstamp).xut_fraction * 10000000ULL) >> 32);           \
    else                                                                    \
        (xmeter) = XTIME_INVALID_METER;                                     \
} while (0)

/**
 * @enum  xntp_mode_t
 * @brief NTP工作模式的相关枚举值。
 */
typedef enum xntp_mode_t
{
    ntp_mode_unknow     = 0,  ///< 未定义
    ntp_mode_initiative = 1,  ///< 主动对等体模式
    ntp_mode_passive    = 2,  ///< 被动对等体模式
    ntp_mode_client     = 3,  ///< 客户端模式
    ntp_mode_server     = 4,  ///< 服务器模式
    ntp_mode_broadcast  = 5,  ///< 广播模式或组播模式
    ntp_mode_control    = 6,  ///< 报文为 NTP 控制报文
    ntp_mode_reserved   = 7,  ///< 预留给内部使用
} xntp_mode_t;

/**
 * @struct xntp_pack_t
 * @brief  NTP 报文格式。
 */
typedef struct xntp_pack_t
{
    x_uchar_t     xct_li_ver_mode;      ///< 2 bits，飞跃指示器；3 bits，版本号；3 bits，NTP工作模式（参看 xntp_mode_t 相关枚举值）
    x_uchar_t     xct_stratum    ;      ///< 系统时钟的层数，取值范围为1~16，它定义了时钟的准确度。层数为1的时钟准确度最高，准确度从1到16依次递减，层数为16的时钟处于未同步状态，不能作为参考时钟
    x_uchar_t     xct_poll       ;      ///< 轮询时间，即两个连续NTP报文之间的时间间隔
    x_uchar_t     xct_percision  ;      ///< 系统时钟的精度

    x_uint32_t    xut_root_delay     ;  ///< 本地到主参考时钟源的往返时间
    x_uint32_t    xut_root_dispersion;  ///< 系统时钟相对于主参考时钟的最大误差
    x_uint32_t    xut_ref_indentifier;  ///< 参考时钟源的标识

    xtime_stamp_t xtmst_reference;      ///< 系统时钟最后一次被设定或更新的时间（应答完成后，用于存储 T1）
    xtime_stamp_t xtmst_originate;      ///< NTP请求报文离开发送端时发送端的本地时间（应答完成后，用于存储 T4）
    xtime_stamp_t xtmst_receive  ;      ///< NTP请求报文到达接收端时接收端的本地时间（应答完成后，用于存储 T2）
    xtime_stamp_t xtmst_transmit ;      ///< NTP应答报文离开应答者时应答者的本地时间（应答完成后，用于存储 T3）
} xntp_pack_t;

////////////////////////////////////////////////////////////////////////////////

/**********************************************************/
/**
 * @brief 判断字符串是否为有效的 4 段式 IP 地址格式。
 *
 * @param [in ] xszt_vptr : 判断的字符串。
 * @param [out] xut_value : 若入参不为 X_NULL，则操作成功时，返回对应的 IP 地址值。
 *
 * @return x_bool_t : 成功，返回 X_TRUE；失败，返回 X_FALSE。
 */
static x_bool_t ntp_ipv4_valid(x_cstring_t xszt_vptr, x_uint32_t * xut_value)
{
    x_uchar_t xct_ipv[4] = { 0, 0, 0, 0 };

    x_int32_t xit_itv = 0;
    x_int32_t xit_sum = 0;
    x_bool_t  xbt_okv = X_FALSE;

    x_char_t         xct_iter = '\0';
    const x_char_t * xct_iptr = xszt_vptr;

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
        else if (xbt_okv                                   && 
                 (('.' == xct_iter) || ('\0' == xct_iter)) && 
                 (xit_itv < (x_int32_t)sizeof(xct_ipv))    && 
                 (xit_sum <= 0xFF))
        {
            xct_ipv[xit_itv++] = (x_uchar_t)xit_sum;
            xit_sum = 0;
            xbt_okv = X_FALSE;
        }
        else
        {
            break;
        }

        if ('\0' == xct_iter)
        {
            break;
        }
    }

    xbt_okv = (xit_itv == sizeof(xct_ipv)) ? X_TRUE : X_FALSE;

    //======================================

#define MAKE_IPV4_VALUE(b1, b2, b3, b4)      \
    ((x_uint32_t)(((x_uint32_t)(b1) << 24) + \
                  ((x_uint32_t)(b2) << 16) + \
                  ((x_uint32_t)(b3) <<  8) + \
                  ((x_uint32_t)(b4) <<  0)))

    if (X_NULL != xut_value)
    {
        if (xbt_okv)
            *xut_value = MAKE_IPV4_VALUE(xct_ipv[0], xct_ipv[1], xct_ipv[2], xct_ipv[3]);
        else
            *xut_value = 0xFFFFFFFF;
    }

#undef MAKE_IPV4_VALUE

//======================================

    return xbt_okv;
}

/**********************************************************/
/**
 * @brief 返回套接字当前操作失败的错误码。
 */
static x_int32_t ntp_sockfd_errno()
{
#ifdef _MSC_VER
    return (x_int32_t)WSAGetLastError();
#else // !_MSC_VER
    return errno;
#endif // _MSC_VER
}

/**********************************************************/
/**
 * @brief 设置套接字的 数据收发超时时间。
 * 
 * @param [in ] xut_tmout : 超时时间（单位为 毫秒）。
 * 
 */
static x_void_t ntp_sockfd_tmout(x_sockfd_t xfdt_sockfd, x_uint32_t xut_tmout)
{
        // 设置 发送/接收 超时时间
#if (defined(_WIN32) || defined(_WIN64))

    setsockopt(
        xfdt_sockfd,
        SOL_SOCKET,
        SO_SNDTIMEO,
        (x_char_t *)&xut_tmout,
        sizeof(x_uint32_t));

    setsockopt(
        xfdt_sockfd,
        SOL_SOCKET,
        SO_RCVTIMEO,
        (x_char_t *)&xut_tmout,
        sizeof(x_uint32_t));

#elif (defined(__linux__) || defined(__unix__))

    struct timeval xtm_value;

    xtm_value.tv_sec  = (x_long_t)((xut_tmout / 1000));
    xtm_value.tv_usec = (x_long_t)((xut_tmout % 1000) * 1000);

    setsockopt(
        xfdt_sockfd,
        SOL_SOCKET,
        SO_SNDTIMEO,
        (x_char_t *)&xtm_value,
        sizeof(struct timeval));

    setsockopt(
        xfdt_sockfd,
        SOL_SOCKET,
        SO_RCVTIMEO,
        (x_char_t *)&xtm_value,
        sizeof(struct timeval));

#else // UNKNOW
#endif // PLATFORM
}

/**********************************************************/
/**
 * @brief 关闭套接字。
 */
static x_int32_t ntp_sockfd_close(x_sockfd_t xfdt_sockfd)
{
#if (defined(_WIN32) || defined(_WIN64))
    return closesocket(xfdt_sockfd);
#else // !(defined(_WIN32) || defined(_WIN64))
    return close(xfdt_sockfd);
#endif // (defined(_WIN32) || defined(_WIN64))
}

/**********************************************************/
/**
 * @brief 初始化 NTP 的请求数据包。
 */
static x_void_t ntp_init_request_packet(xntp_pack_t * xnpt_dptr)
{
    const x_uchar_t xct_leap = 0;
    const x_uchar_t xct_ver  = 3;
    const x_uchar_t xct_mode = ntp_mode_client;

    xnpt_dptr->xct_li_ver_mode = (xct_leap << 6) | (xct_ver << 3) | (xct_mode << 0);
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
 * @brief 将 xntp_pack_t 中的 网络字节序 字段转换为 主机字节序。
 */
static x_void_t ntp_ntoh_packet(xntp_pack_t * xnpt_nptr)
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
 * @brief 将 xntp_pack_t 中的 主机字节序 字段转换为 网络字节序。
 */
static x_void_t ntp_hton_packet(xntp_pack_t * xnpt_nptr)
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
 *  1. 客户端 发送一个NTP报文给 服务端，该报文带有它离开 客户端 时的时间戳，该时间戳为 T1。
 *  2. 当此NTP报文到达 服务端 时，服务端 加上自己的时间戳，该时间戳为 T2。
 *  3. 当此NTP报文离开 服务端 时，服务端 再加上自己的时间戳，该时间戳为 T3。
 *  4. 当 客户端 接收到该应答报文时，客户端 的本地时间戳，该时间戳为 T4。
 * </pre>
 *
 * @param [in ] xszt_host : NTP 服务器的 IP（四段式 IP 地址）。
 * @param [in ] xut_port  : NTP 服务器的 端口号（可取默认的端口号 NTP_PORT : 123）。
 * @param [in ] xut_tmout : 超时时间（单位 毫秒）。
 * @param [out] xtm_meter : 操作成功返回的相关计算所需的时间戳（T1、T2、T3、T4）。
 *
 * @return x_int32_t : 成功，返回 0；失败，返回 错误码。
 */
static x_int32_t ntp_get_4meter(
                    x_cstring_t xszt_host,
                    x_uint16_t xut_port,
                    x_uint32_t xut_tmout,
                    xtime_meter_t xtm_meter[4])
{
    x_int32_t xit_errno = EPERM;

    x_sockfd_t  xfdt_sockfd = X_INVALID_SOCKFD;
    xntp_pack_t xnpt_packet;

    x_int32_t xit_addrlen = sizeof(struct sockaddr_in);
    struct sockaddr_in skaddr_host;

    do 
    {
        //======================================

        if ((X_NULL == xszt_host) || (xut_tmout <= 0))
        {
            xit_errno = EINVAL;
            break;
        }

        //======================================

        xfdt_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (X_INVALID_SOCKFD == xfdt_sockfd)
        {
            xit_errno = ntp_sockfd_errno();
            break;
        }

        // 设置 发送/接收 超时时间
        ntp_sockfd_tmout(xfdt_sockfd, xut_tmout);

        // 服务端主机地址
        memset(&skaddr_host, 0, sizeof(struct sockaddr_in));
        skaddr_host.sin_family = AF_INET;
        skaddr_host.sin_port   = htons(xut_port);
        inet_pton(AF_INET, xszt_host, &skaddr_host.sin_addr.s_addr);

        //======================================

        // 初始化请求数据包
        ntp_init_request_packet(&xnpt_packet);

        // T1
        xtm_meter[0] = time_meter();

        // NTP请求报文离开发送端时发送端的本地时间
        XTIME_MTOS(xtm_meter[0], xnpt_packet.xtmst_originate);

        // 转成网络字节序
        ntp_hton_packet(&xnpt_packet);

        // 发送 NTP 请求
        xit_errno = sendto(xfdt_sockfd,
                         (x_char_t *)&xnpt_packet,
                         sizeof(xntp_pack_t),
                         0,
                         (struct sockaddr *)&skaddr_host,
                         sizeof(struct sockaddr_in));
        if (xit_errno < 0)
        {
            xit_errno = ntp_sockfd_errno();
            break;
        }

        //======================================

        memset(&xnpt_packet, 0, sizeof(xntp_pack_t));

        // 接收应答
        xit_errno = recvfrom(xfdt_sockfd,
                           (x_char_t *)&xnpt_packet,
                           sizeof(xntp_pack_t),
                           0,
                           (struct sockaddr *)&skaddr_host,
                           (socklen_t *)&xit_addrlen);
        if (xit_errno < 0)
        {
            errno = ntp_sockfd_errno();
            break;
        }

        // 判断数据包长度是否有效
        if (sizeof(xntp_pack_t) != xit_errno)
        {
            xit_errno = ENODATA;
            break;
        }

        // T4
        xtm_meter[3] = time_meter();

        // 转成主机字节序
        ntp_ntoh_packet(&xnpt_packet);

        XTIME_STOM(xnpt_packet.xtmst_receive , xtm_meter[1]); // T2
        XTIME_STOM(xnpt_packet.xtmst_transmit, xtm_meter[2]); // T3

        if (XTIME_METER_INVALID(xtm_meter[1]) ||
            XTIME_METER_INVALID(xtm_meter[2]))
        {
            xit_errno = ETIME;
            break;
        }

        //======================================
        xit_errno = 0;
    } while (0);

    if (X_INVALID_SOCKFD != xfdt_sockfd)
    {
        ntp_sockfd_close(xfdt_sockfd);
        xfdt_sockfd = X_INVALID_SOCKFD;
    }

    return xit_errno;
}

/**********************************************************/
/**
 * @brief 向 NTP 服务器发送 NTP 请求，获取相关计算所需的时间戳。
 *
 * @param [in ] xszt_name : NTP 服务器的 域名（如 3.cn.pool.ntp.org）。
 * @param [in ] xut_port  : NTP 服务器的 端口号（可取默认的端口号 NTP_PORT : 123）。
 * @param [in ] xut_tmout : 网络请求的超时时间（单位为毫秒）。
 * @param [out] xtm_meter : 操作成功返回的相关计算所需的时间戳（T1、T2、T3、T4）。
 *
 * @return x_int32_t : 成功，返回 0；失败，返回 错误码。
 */
static x_int32_t ntp_get_4meter_by_name(
                    x_cstring_t xszt_name,
                    x_uint16_t xut_port,
                    x_uint32_t xut_tmout,
                    xtime_meter_t xtm_meter[4])
{
    x_int32_t xit_errno = EPERM;

    struct addrinfo   xai_hint;
    struct addrinfo * xai_rptr = X_NULL;
    struct addrinfo * xai_iptr = X_NULL;

    x_char_t xszt_host[TEXT_LEN_256] = { 0 };

    do
    {
        //======================================

        if (X_NULL == xszt_name)
        {
            xit_errno = EINVAL;
            break;
        }

        memset(&xai_hint, 0, sizeof(xai_hint));
        xai_hint.ai_family   = AF_INET;
        xai_hint.ai_socktype = SOCK_DGRAM;

        xit_errno = getaddrinfo(xszt_name, X_NULL, &xai_hint, &xai_rptr);
        if (0 != xit_errno)
        {
            break;
        }

        //======================================

        for (xai_iptr = xai_rptr; X_NULL != xai_iptr; xai_iptr = xai_iptr->ai_next)
        {
            if (AF_INET != xai_iptr->ai_family)
            {
                continue;
            }

            memset(xszt_host, 0, TEXT_LEN_256);
            if (X_NULL == inet_ntop(
                            AF_INET,
                            &(((struct sockaddr_in *)(xai_iptr->ai_addr))->sin_addr),
                            xszt_host,
                            TEXT_LEN_256))
            {
                continue;
            }

            xit_errno = ntp_get_4meter(xszt_host, xut_port, xut_tmout, xtm_meter);
            if (0 == xit_errno)
            {
                break;
            }
        }

        //======================================
    } while (0);

    if (X_NULL != xai_rptr)
    {
        freeaddrinfo(xai_rptr);
        xai_rptr = X_NULL;
    }

    return xit_errno;
}

//====================================================================

// 
// 外部相关操作接口
// 

/**********************************************************/
/**
 * @brief 向 NTP 服务器发送 NTP 请求，获取服务器时间戳。
 *
 * @param [in ] xszt_host : NTP 服务器的 IP（四段式 IP 地址） 或 域名（如 3.cn.pool.ntp.org）。
 * @param [in ] xut_port  : NTP 服务器的 端口号（可取默认的端口号 NTP_PORT : 123）。
 * @param [in ] xut_tmout : 网络请求的超时时间（单位为毫秒）。
 *
 * @return xtime_meter_t : 
 * 返回 时间计量值，可用 XTIME_METER_INVALID() 判断是否为无效值；
 * 若值无效，则可通过 ntp_errno() 获知错误码。
 */
xtime_meter_t ntp_get_time(
                x_cstring_t xszt_host,
                x_uint16_t xut_port,
                x_uint32_t xut_tmout)
{
    x_int32_t xit_errno = EPERM;

    xtime_meter_t xtm_meter[4] =
    {
        XTIME_INVALID_METER,
        XTIME_INVALID_METER,
        XTIME_INVALID_METER,
        XTIME_INVALID_METER
    };

    //======================================
    // 参数验证

    if ((X_NULL == xszt_host) || (xut_tmout <= 0))
    {
        errno = EINVAL;
        return XTIME_INVALID_METER;
    }

    //======================================

    if (ntp_ipv4_valid(xszt_host, X_NULL))
        xit_errno = ntp_get_4meter(xszt_host, xut_port, xut_tmout, xtm_meter);
    else
        xit_errno = ntp_get_4meter_by_name(xszt_host, xut_port, xut_tmout, xtm_meter);

    if (0 != xit_errno)
    {
        errno = xit_errno;
        return XTIME_INVALID_METER;
    }

    //======================================

/** 最后结果的计算公式：T = T4 + ((T2 - T1) + (T3 - T4)) / 2; */
#define XNTP_METER(x4meter) \
    (x4meter[3] + ((x4meter[1] - x4meter[0]) + (x4meter[2] - x4meter[3])) / 2)

    return XNTP_METER(xtm_meter);

#undef XNTP_METER

    //======================================
}

////////////////////////////////////////////////////////////////////////////////

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif // __GNUC__

////////////////////////////////////////////////////////////////////////////////
