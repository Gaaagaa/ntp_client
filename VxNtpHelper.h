/**
 * @file    VxNtpHelper.h
 * <pre>
 * Copyright (c) 2018, Gaaagaa All rights reserved.
 *
 * 文件名称：VxNtpHelper.h
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

#ifndef __VXNTPHELPER_H__
#define __VXNTPHELPER_H__

#include "VxDType.h"

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////

#ifndef NTP_OUTPUT
#define NTP_OUTPUT 1    ///< 接口调用过程是否输出调试信息的宏开关
#endif // NTP_OUTPUT

#define NTP_PORT   123  ///< NTP 专用端口号

/**
 * @struct x_ntp_timestamp_t
 * @brief  NTP 时间戳。
 */
typedef struct x_ntp_timestamp_t
{
    x_uint32_t  xut_seconds;    ///< 从 1900年至今所经过的秒数
    x_uint32_t  xut_fraction;   ///< 小数部份，单位是微秒数的4294.967296( = 2^32 / 10^6 )倍
} x_ntp_timestamp_t;

/**
 * @struct x_ntp_timeval_t
 * @brief  对系统的 timeval 结构体进行重定义结构体。
 */
typedef struct x_ntp_timeval_t
{
    x_long_t    tv_sec;    ///< 秒
    x_long_t    tv_usec;   ///< 微秒
} x_ntp_timeval_t;

/**
 * @struct x_ntp_time_context_t
 * @brief  时间描述信息结构体。
 */
typedef struct x_ntp_time_context_t
{
    x_uint32_t   xut_year   : 16;  ///< 年
    x_uint32_t   xut_month  :  6;  ///< 月
    x_uint32_t   xut_day    :  6;  ///< 日
    x_uint32_t   xut_week   :  4;  ///< 周几
    x_uint32_t   xut_hour   :  6;  ///< 时
    x_uint32_t   xut_minute :  6;  ///< 分
    x_uint32_t   xut_second :  6;  ///< 秒
    x_uint32_t   xut_msec   : 14;  ///< 毫秒
} x_ntp_time_context_t;

////////////////////////////////////////////////////////////////////////////////

/**********************************************************/
/**
 * @brief 获取当前系统的 时间值（以 100纳秒 为单位，1970年1月1日到现在的时间）。
 */
x_uint64_t ntp_gettimevalue(void);

/**********************************************************/
/**
 * @brief 获取当前系统的 timeval 值（1970年1月1日到现在的时间）。
 */
x_void_t ntp_gettimeofday(x_ntp_timeval_t * xtm_value);

/**********************************************************/
/**
 * @brief 将 x_ntp_time_context_t 转换为 以 100纳秒 
 *        为单位的时间值（1970年1月1日到现在的时间）。
 */
x_uint64_t ntp_time_value(x_ntp_time_context_t * xtm_context);

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
x_bool_t ntp_tmctxt_bv(x_uint64_t xut_time, x_ntp_time_context_t * xtm_context);

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
x_bool_t ntp_tmctxt_tv(const x_ntp_timeval_t * const xtm_value, x_ntp_time_context_t * xtm_context);

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
x_bool_t ntp_tmctxt_ts(const x_ntp_timestamp_t * const xtm_timestamp, x_ntp_time_context_t * xtm_context);

////////////////////////////////////////////////////////////////////////////////

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
x_int32_t ntp_get_time(x_cstring_t xszt_host, x_uint16_t xut_port, x_uint32_t xut_tmout, x_uint64_t * xut_timev);

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}; // extern "C"
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////

#endif // __VXNTPHELPER_H__
