# ntp_client #
使用NTP协议获取网络时间戳，提供的 C/C++ 源码支持 Windows 和 Linux(CentOS) 两大平台。

## Winodws 平台上编译与测试 ##
在 VC 的命令行环境下，使用 **nmake /f Makefile.mk** 命令进行编译，输出的测试程序为 
**ntp\_cli\_test.exe** ，执行该程序，即可看到测试结果。

## Linux 平台上编译与测试 ##
    # make
    # ./ntp_cli_test
依次执行上面的命令，即可看到测试结果。

## 源码说明 ##
- VxDType.h: 定义通用数据类型的头文件。
- VxNtpHelper.h：使用NTP协议获取网络时间戳所提供的 API 与相关数据定义的头文件。
- VxNtpHelper.cpp：VxNtpHelper.h中提供的操 API 实现源码文件。
- main.cpp：接口调用流程（测试程序）。

具体项目中的应用，参看 main.cpp 中的调用流程。

