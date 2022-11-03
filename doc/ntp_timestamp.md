最近接手的一个客户端项目，需要获取网络时间戳进行超时判断，想到了使用NTP协议来实现。
在网上参看了不少博文，大多数人提供的C/C++代码过于杂乱，不宜在正式项目中使用（拒绝野代码）。
在此我重写了一遍，并在 Windows 与 Linux 两大平台上测试通过。

## NTP 工作原理
NTP的基本工作原理如图所示（Device A 相当于客户端，Device B 相当于 NTP 服务端）。Device A 和 Device B 通过网络相连，它们都有自己独立的系统时钟，需要通过NTP实现各自系统时钟的自动同步。为便于理解，作如下假设：

- 在Device A和Device B的系统时钟同步之前，Device A的时钟设定为10:00:00am，Device B的时钟设定为11:00:00am。
- Device B作为NTP时间服务器，即Device A将使自己的时钟与Device B的时钟同步。
- NTP报文在Device A和Device B之间单向传输所需要的时间为1秒。

图 NTP基本原理图

![NTP基本原理图](https://img2018.cnblogs.com/blog/1494489/201811/1494489-20181116180148262-1580200220.png)

获取网络时间戳的工作过程如下：

1. Device A发送一个NTP报文给Device B，该报文带有它离开Device A时的时间戳，该时间戳为10:00:00am（T1）。
2. 当此NTP报文到达Device B时，Device B加上自己的时间戳，该时间戳为11:00:01am（T2）。
3. 当此NTP报文离开Device B时，Device B再加上自己的时间戳，该时间戳为11:00:02am（T3）。
4. 当Device A接收到该响应报文时，Device A的本地时间为10:00:03am（T4）。

至此，Device A已经拥有足够的信息来计算两个重要的参数：

- NTP报文的往返时延 Delay =(T4 - T1) - (T3 - T2) = 2秒。
- Device A 相对于 Device B 的时间差 Offset = ((T2 - T1) + (T3 - T4)) / 2 = 1小时。
- Device A 同步到 Device B 的时间戳 T = T4 + ((T2 - T1) + (T3 - T4)) / 2 。

这样，Device A就能够根据这些信息来设定自己的时钟，使之与Device B的时钟同步。

以上内容只是对NTP工作原理的一个粗略描述，详细内容请参阅RFC 1305。

## NTP 报文格式
NTP有两种不同类型的报文，一种是时钟同步报文，另一种是控制报文。控制报文仅用于需要网络管理的场合，它对于时钟同步功能来说并不是必需的，这里不做介绍。时钟同步报文封装在UDP报文中，其格式如图所示。

图 时钟同步报文格式

![时钟同步报文格式](https://img2018.cnblogs.com/blog/1494489/201811/1494489-20181116180232768-1346055939.png)

主要字段的解释如下：

- LI（Leap Indicator，闰秒提示）：长度为2比特，值为“11”时表示告警状态，时钟未被同步。为其他值时NTP本身不做处理。
- VN（Version Number，版本号）：长度为3比特，表示NTP的版本号，目前的最新版本为4。
- Mode：长度为3比特，表示NTP的工作模式。不同的值所表示的含义分别是：
```
	0 未定义；
	1 表示主动对等体模式；
	2 表示被动对等体模式；
	3 表示客户模式；
	4 表示服务器模式；
	5 表示广播模式或组播模式；
	6 表示此报文为NTP控制报文；
	7 预留给内部使用。
```
- Stratum：系统时钟的层数，取值范围为1～16，它定义了时钟的准确度。层数为1的时钟准确度最高，准确度从1到16依次递减，层数为16的时钟处于未同步状态。
- Poll：轮询时间，即两个连续NTP报文之间的时间间隔。
- Precision：系统时钟的精度。
- Root Delay：本地到主参考时钟源的往返时间。
- Root Dispersion：系统时钟相对于主参考时钟的最大误差。
- Reference Identifier：参考时钟源的标识。
- Reference Timestamp：系统时钟最后一次被设定或更新的时间。
- Originate Timestamp：NTP请求报文离开发送端时发送端的本地时间。
- Receive Timestamp：NTP请求报文到达接收端时接收端的本地时间。
- Transmit Timestamp：应答报文离开应答者时应答者的本地时间。
- Authenticator：验证信息。

## 常用的 NTP 服务器
```
    1.cn.pool.ntp.org
    2.cn.pool.ntp.org
    3.cn.pool.ntp.org
    0.cn.pool.ntp.org
    cn.pool.ntp.org
    tw.pool.ntp.org
    0.tw.pool.ntp.org
    1.tw.pool.ntp.org
    2.tw.pool.ntp.org
    3.tw.pool.ntp.org
    pool.ntp.org
    time.windows.com
    time.nist.gov
    time-nw.nist.gov
    asia.pool.ntp.org
    europe.pool.ntp.org
    oceania.pool.ntp.org
    north-america.pool.ntp.org
    south-america.pool.ntp.org
    africa.pool.ntp.org
    ca.pool.ntp.org
    uk.pool.ntp.org
    us.pool.ntp.org
    au.pool.ntp.org
```

## C/C++代码实现的主要流程
- NTP通信相关的数据结构体
```
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
    x_uchar_t         xct_li_ver_mode;      ///< 2 bits，飞跃指示器；3 bits，版本号；3 bits，NTP工作    模式（参看 em_ntp_mode_t 相关枚举值）
    x_uchar_t         xct_stratum    ;      ///< 系统时钟的层数，取值范围为1~16，它定义了时钟的准确    度。层数为1的时钟准确度最高，准确度从1到16依次递减，层数为16的时钟处于未同步状态，不能作为参考时钟
    x_uchar_t         xct_poll       ;      ///< 轮询时间，即两个连续NTP报文之间的时间间隔
    x_uchar_t         xct_percision  ;      ///< 系统时钟的精度

    x_uint32_t        xut_root_delay     ;  ///< 本地到主参考时钟源的往返时间
    x_uint32_t        xut_root_dispersion;  ///< 系统时钟相对于主参考时钟的最大误差
    x_uint32_t        xut_ref_indentifier;  ///< 参考时钟源的标识

    x_ntp_timestamp_t xtmst_reference;      ///< 系统时钟最后一次被设定或更新的时间
    x_ntp_timestamp_t xtmst_originate;      ///< NTP请求报文离开发送端时发送端的本地时间
    x_ntp_timestamp_t xtmst_receive  ;      ///< NTP请求报文到达接收端时接收端的本地时间
    x_ntp_timestamp_t xtmst_transmit ;      ///< 应答报文离开应答者时应答者的本地时间
} x_ntp_packet_t;
```
- NTP请求的操作流程
```
/**********************************************************/
/**
 * @brief 向 NTP 服务器发送 NTP 请求，获取相关计算所需的时间戳（T1、T2、T3、T4如下所诉）。
 * <pre>
 *     1. 客户端 发送一个NTP报文给 服务端，该报文带有它离开 客户端 时的时间戳，该时间戳为 T1。
 *     2. 当此NTP报文到达 服务端 时，服务端 加上自己的时间戳，该时间戳为 T2。
 *     3. 当此NTP报文离开 服务端 时，服务端 再加上自己的时间戳，该时间戳为 T3。
 *     4. 当 客户端 接收到该应答报文时，客户端 的本地时间戳，该时间戳为 T4。
 * </prev>
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
```
- 项目中实际使用到的接口
```
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
x_int32_t ntp_get_time(x_cstring_t xszt_host, x_uint16_t xut_port, x_uint32_t xut_tmout,     x_uint64_t * xut_timev);
```
其中，返回的时间戳值 `xut_timev` 由计算公式 `T = T4 + ((T2 - T1) + (T3 - T4)) / 2;` 所得。若要将该时间戳值转换为其他易描述（或实际应用）的数据信息，调用 `ntp_tmctxt_bv(xut_timev, &xtm_context)` 接口即可，详细说明参考如下所列代码：
```
// VxNtpHelper.h

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
```
也可通过如下方式转换为 timeval 信息：
```
struct timeval tv;
tv.tv_sec  = (long)((xut_timev / 10000000LL));
tv.tv_usec = (long)((xut_timev % 10000000LL) / 10);
```

## 源码下载
至Github下载：[https://github.com/Gaaagaa/ntp_client](https://github.com/Gaaagaa/ntp_client "ntp_client")
## 参看资料
- NTP工作原理：[http://ntp.neu.edu.cn/archives/92](http://ntp.neu.edu.cn/archives/92)
- NTP的报文格式：[http://ntp.neu.edu.cn/archives/95](http://ntp.neu.edu.cn/archives/95)
