# 使用 NTP 协议获取网络时间戳

最近接手的一个客户端项目，需要获取网络时间戳进行超时判断，想到了使用NTP协议来实现。
在网上参看了不少博文，大多数人提供的C/C++代码过于杂乱，不宜在正式项目中使用（拒绝野代码）。
在此我重写了一遍，并在 Windows 与 Linux 两大平台上测试通过。

## 1. NTP 工作原理

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

## 2. NTP 报文格式

NTP有两种不同类型的报文，一种是时钟同步报文，另一种是控制报文。控制报文仅用于需要网络管理的场合，它对于时钟同步功能来说并不是必需的，这里不做介绍。时钟同步报文封装在UDP报文中，其格式如图所示。

图 时钟同步报文格式

![时钟同步报文格式](https://img2018.cnblogs.com/blog/1494489/201811/1494489-20181116180232768-1346055939.png)

主要字段的解释如下：

- LI（Leap Indicator，闰秒提示）：长度为2比特，值为“11”时表示告警状态，时钟未被同步。为其他值时NTP本身不做处理。
- VN（Version Number，版本号）：长度为3比特，表示NTP的版本号，目前的最新版本为4。
- Mode：长度为3比特，表示NTP的工作模式。不同的值所表示的含义分别是：

>
> 0 未定义；
> 1 表示主动对等体模式；
> 2 表示被动对等体模式；
> 3 表示客户模式；
> 4 表示服务器模式；
> 5 表示广播模式或组播模式；
> 6 表示此报文为NTP控制报文；
> 7 预留给内部使用。
>

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

## 3. 一些常用的 NTP 服务器

>
> ntp.tencent.com
> ntp1.tencent.com
> ntp2.tencent.com
> ntp3.tencent.com
> ntp4.tencent.com
> ntp5.tencent.com
> ntp.aliyun.com
> ntp1.aliyun.com
> ntp2.aliyun.com
> ntp3.aliyun.com
> ntp4.aliyun.com
> ntp5.aliyun.com
> ntp6.aliyun.com
> ntp7.aliyun.com
> time.edu.cn
> s2c.time.edu.cn
> s2f.time.edu.cn
>

## 4. C/C++代码实现的主要流程

### 4.1 NTP通信相关的数据结构体

```c++

/**
 * @struct xtime_stamp_t
 * @brief  NTP所使用的时间戳。
 */
typedef struct xtime_stamp_t
{
    x_uint32_t xut_seconds;  ///< 从 1900年至今所经过的秒数
    x_uint32_t xut_fraction; ///< 小数部份，其单位是 百纳秒数 的 (2^32 / 10^7) 倍
} xtime_stamp_t;

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
    x_uchar_t     xct_lvmflag  ;  ///< 2 bits，飞跃指示器；3 bits，版本号；3 bits，NTP工作模式（参看 xntp_mode_t 相关枚举值）
    x_uchar_t     xct_stratum  ;  ///< 系统时钟的层数，取值范围为1~16，它定义了时钟的准确度。层数为1的时钟准确度最高，准确度从1到16依次递减，层数为16的时钟处于未同步状态，不能作为参考时钟
    x_uchar_t     xct_ppoll    ;  ///< 轮询时间，即两个连续NTP报文之间的时间间隔
    x_char_t      xct_percision;  ///< 系统时钟的精度

    x_uint32_t    xut_rootdelay;  ///< 本地到主参考时钟源的往返时间
    x_uint32_t    xut_rootdisp ;  ///< 系统时钟相对于主参考时钟的最大误差
    x_uint32_t    xut_refid    ;  ///< 参考时钟源的标识

    /**
     * T1，客户端发送请求时的 本地系统时间戳；
     * T2，服务端接收到客户端请求时的 本地系统时间戳；
     * T3，服务端发送应答数据包时的 本地系统时间戳；
     * T4，客户端接收到服务端应答数据包时的 本地系统时间戳。
     */
    xtime_stamp_t xtms_reference; ///< 系统时钟最后一次被设定或更新的时间
    xtime_stamp_t xtms_originate; ///< 服务端应答时，将客户端请求时的 T1 返送回去
    xtime_stamp_t xtms_receive  ; ///< 服务端接收到客户端请求时的 本地系统时间戳 T2
    xtime_stamp_t xtms_transmit ; ///< 客户端请求时 发送 T1，服务端应答时 回复 T3
} xntp_pack_t;

/**
 * @struct xntp_client_t
 * @brief  NTP 客户端工作对象的结构体描述信息。
 */
typedef struct xntp_client_t
{
    x_sockfd_t    xfdt_sockfd;              ///< 网络通信使用的 套接字
    x_char_t      xszt_host[TEXT_LEN_256];  ///< 存储提供 NTP 服务的 服务端 地址
    x_uint16_t    xut_port;                 ///< 存储提供 NTP 服务的 服务端 端口号
    xtime_vnsec_t xtm_4time[4];             ///< 完成 NTP 请求后，所得到的 4 个相关时间戳
} xntp_client_t;

```

### 4.2 NTP请求的操作流程

```c++

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
 * @param [in ] xntp_this : NTP 客户端工作对象。
 * @param [in ] xszt_host : NTP 服务器的 IP（四段式 IP 地址）。
 * @param [in ] xut_tmout : 超时时间（单位 毫秒）。
 * 
 * @return x_int32_t : 成功，返回 0；失败，返回 错误码。
 */
static x_int32_t ntpcli_get_4T(
                    xntp_cliptr_t xntp_this,
                    x_cstring_t xszt_host,
                    x_uint32_t xut_tmout)
{
    x_int32_t xit_errno = EPERM;

    xntp_pack_t        xnpt_pack;
    x_int32_t          xit_alen;
    struct sockaddr_in xin_addr;
    fd_set             xfds_rset;
    struct timeval     xtm_value;

    do 
    {
        //======================================

        if ((X_NULL == xntp_this) || (X_NULL == xszt_host))
        {
            xit_errno = EINVAL;
            break;
        }

        xntp_this->xtm_4time[0] = XTIME_INVALID_VNSEC;
        xntp_this->xtm_4time[1] = XTIME_INVALID_VNSEC;
        xntp_this->xtm_4time[2] = XTIME_INVALID_VNSEC;
        xntp_this->xtm_4time[3] = XTIME_INVALID_VNSEC;

        //======================================

        // 服务端主机地址
        memset(&xin_addr, 0, sizeof(struct sockaddr_in));
        xin_addr.sin_family = AF_INET;
        xin_addr.sin_port   = htons(xntp_this->xut_port);
        inet_pton(AF_INET, xszt_host, &xin_addr.sin_addr.s_addr);

        //======================================

        // 初始化请求数据包
        ntp_init_req_packet(&xnpt_pack);

        // T1
        xntp_this->xtm_4time[0] = time_vnsec();

        // NTP请求报文离开发送端时发送端的本地时间
        XTIME_UTOS(xntp_this->xtm_4time[0], xnpt_pack.xtms_transmit);

        // 转成网络字节序
        ntp_hton_packet(&xnpt_pack);

        // 发送 NTP 请求
        xit_errno = sendto(
                        xntp_this->xfdt_sockfd,
                        (x_char_t *)&xnpt_pack,
                        sizeof(xntp_pack_t),
                        0,
                        (struct sockaddr *)&xin_addr,
                        sizeof(struct sockaddr_in));
        if (xit_errno < 0)
        {
            xit_errno = sockfd_errno();
#if (defined(_WIN32) || defined(_WIN64))
            if (WSAEWOULDBLOCK != xit_errno)
#elif (defined(__linux__) || defined(__unix__))
            if ((EAGAIN != xit_errno) && (EWOULDBLOCK != xit_errno))
#else // UNKNOW
#endif // PLATFORM
            {
                break;
            }
        }

        //======================================
        // 使用 select() 检测套接字可读

        FD_ZERO(&xfds_rset);
        FD_SET(xntp_this->xfdt_sockfd, &xfds_rset);

        // 超时时间
        if (xut_tmout > 0)
        {
            xtm_value.tv_sec  = (xut_tmout / 1000);
            xtm_value.tv_usec = (xut_tmout % 1000) * 1000;
        }

        xit_errno = select(
                        (x_int32_t)(xntp_this->xfdt_sockfd + 1),
                        &xfds_rset,
                        X_NULL,
                        X_NULL,
                        (xut_tmout > 0) ? &xtm_value : X_NULL);
        if (xit_errno <= 0)
        {
            xit_errno = (0 == xit_errno) ? ETIMEDOUT : sockfd_errno();
            break;
        }

        if (!FD_ISSET(xntp_this->xfdt_sockfd, &xfds_rset))
        {
            xit_errno = EBADF;
            break;
        }

        //======================================

        memset(&xnpt_pack, 0, sizeof(xntp_pack_t));

        // 接收应答
        xit_alen  = sizeof(struct sockaddr_in);
        xit_errno = recvfrom(
                        xntp_this->xfdt_sockfd,
                        (x_char_t *)&xnpt_pack,
                        sizeof(xntp_pack_t),
                        0,
                        (struct sockaddr *)&xin_addr,
                        (socklen_t *)&xit_alen);
        // T4
        xntp_this->xtm_4time[3] = time_vnsec();

        if (xit_errno < 0)
        {
            xit_errno = sockfd_errno();
            break;
        }

        // 判断数据包长度是否有效
        if (sizeof(xntp_pack_t) != xit_errno)
        {
            xit_errno = ENODATA;
            break;
        }

        // 转成主机字节序
        ntp_ntoh_packet(&xnpt_pack);

        XTIME_STOU(xnpt_pack.xtms_receive , xntp_this->xtm_4time[1]); // T2
        XTIME_STOU(xnpt_pack.xtms_transmit, xntp_this->xtm_4time[2]); // T3

        if (!XTMVNSEC_IS_VALID(xntp_this->xtm_4time[1]) ||
            !XTMVNSEC_IS_VALID(xntp_this->xtm_4time[2]))
        {
            xit_errno = ETIME;
            break;
        }

        //======================================
#ifdef XNTP_DBG_OUTPUT
        printf("========================================\n"
               "%s : %s\n", xntp_this->xszt_host, xszt_host);
        output_tm("\tNTP RT", &xnpt_pack.xtms_reference);
        output_tm("\tNTP T1", &xnpt_pack.xtms_originate);
        output_tm("\tNTP T2", &xnpt_pack.xtms_receive  );
        output_tm("\tNTP T3", &xnpt_pack.xtms_transmit );
        output_tu("\tSYS T1", xntp_this->xtm_4time[0]);
        output_tu("\tSYS T2", xntp_this->xtm_4time[1]);
        output_tu("\tSYS T3", xntp_this->xtm_4time[2]);
        output_tu("\tSYS T4", xntp_this->xtm_4time[3]);
        printf("\n");
#endif // XNTP_DBG_OUTPUT
        //======================================
        xit_errno = 0;
    } while (0);

    return xit_errno;
}

```

### 4.3 项目中实际使用到的接口

项目实际应用的场景，分两种情况：

1. 只做单次调用，从服务端获得 NTP 时间戳之后，不再进行 NTP 的其它相关操作。此情况选择如下调用如下 `ntpcli_get_time()` 接口即可：

```c++

/**********************************************************/
/**
 * @brief 
 * 向 NTP 服务器发送 NTP 请求，获取服务器时间戳。
 * @note 
 * 该接口内部自动 创建/销毁 NTP 客户端对象，执行完整的 NTP 请求流程。
 * 这适用于只执行单次请求操作。
 *
 * @param [in ] xszt_host : NTP 服务器的 IP（四段式 IP 地址） 或 域名（如 3.cn.pool.ntp.org）。
 * @param [in ] xut_port  : NTP 服务器的 端口号（可取默认的端口号 NTP_PORT : 123）。
 * @param [in ] xut_tmout : 网络请求的超时时间（单位为毫秒）。
 *
 * @return xtime_vnsec_t : 
 * 返回 时间计量值，可用 XTMVNSEC_IS_VALID() 判断是否为有效值；
 * 若值无效，则可通过 errno 获知错误码。
 */
xtime_vnsec_t ntpcli_get_time(
                    x_cstring_t xszt_host,
                    x_uint16_t xut_port,
                    x_uint32_t xut_tmout);

```

其中，返回的时间戳值 `xtime_vnsec_t` 由计算公式 `T = T4 + ((T2 - T1) + (T3 - T4)) / 2;` 所得。若要将该时间戳值转换为其他易描述（或实际应用）的数据信息，可调用 `xtime.h` 头文件种所声明的接口 `xtime_descr_t time_vtod(xtime_vnsec_t xtm_vnsec)` 实现，`xtime_descr_t` 数据类型的详细说明，参考 `xtime.h` 中所列代码：

```c++
// xtime.h

/** 以 100纳秒 为单位，1970-01-01 00:00:00 至今的 时间计量值 类型 */
typedef x_uint64_t xtime_vnsec_t;

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

/**********************************************************/
/**
 * @brief 将 时间计量值 转换为 时间描述信息。
 * 
 * @param [in ] xtm_vnsec : 待转换的 时间计量值。
 * 
 * @return xtime_descr_t : 
 * 返回 时间描述信息，可用 XTMDESCR_IS_VALID() 判断其是否为有效。
 */
xtime_descr_t time_vtod(xtime_vnsec_t xtm_vnsec);

```

2. 需要重复发送请求 NTP，借此不断获得 NTP 时间戳，则可创建 `xntp_cliptr_t` 工作对象去实现该应用需求，具体可参考 `ntp_test.c` 源码的操作流程，这里就不再赘述。

## 5. 源码下载

在我的 Gitee 上下载：[https://gitee.com/Gaaagaa/ntp_client](https://gitee.com/Gaaagaa/ntp_client "ntp_client")

## 6. 参看资料

- NTP工作原理：[http://ntp.neu.edu.cn/archives/92](http://ntp.neu.edu.cn/archives/92)
- NTP的报文格式：[http://ntp.neu.edu.cn/archives/95](http://ntp.neu.edu.cn/archives/95)
