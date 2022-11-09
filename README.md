# ntp_client

使用NTP协议获取网络时间戳，提供的 C/C++ 源码支持 Windows 和 Linux(CentOS) 两大平台。

## 1. Winodws 平台上编译与测试

系统需要安装并配置好 cmake 工具，命令行中执行如下命令：

```bat
mkdir build
cd build
cmake ..
```

然后进入 **build** 目录则可看到 **sln** 解决方案文件，使用 **Visual Studio** 打开，则可进行 编译/调试 等工作。

## 2. Linux 平台上编译与测试

```bat
mkdir build
cd build
cmake ..
make
```

依次执行上面的命令，即可看到编译结果。

## 3. 源码说明

核心代码（**src** 目录下）：

- **xtypes.h** : 定义通用数据类型的头文件。
- **xtime.h**、**xtime.c** ：系统时间相关操作 API 与 相关数据定义 的 头文件 和 实现文件。
- **ntp_client.h**、**ntp_client.c** ：使用NTP协议获取网络时间戳所提供的 API 与 相关数据定义 的 头文件 和 实现文件。

测试程序代码（**test** 目录下）：

- **xtime.c** : xtime 主要接口的测试程序。
- **ntp_test.c** : 使用 NTP 协议获取网络时间戳的测试程序。
