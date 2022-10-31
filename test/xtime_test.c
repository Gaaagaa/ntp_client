/**
 * @file xtime_test.c
 * Copyright (c) 2022 Gaaagaa. All rights reserved.
 * 
 * @author  : Gaaagaa
 * @date    : 2022-10-31
 * @version : 1.0.0.0
 * @brief   : 测试 xtime 提供的接口。
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

#include "xtime.h"
#include <stdio.h>

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char * argv[])
{
    xtime_meter_t xtime_meter = time_meter();
    xtime_cntxt_t xtime_cntxt = time_cntxt();

    printf("sizeof(xtime_meter_t) = %d\n", (x_int32_t)sizeof(xtime_meter_t));
    printf("sizeof(xtime_cntxt_t) = %d\n", (x_int32_t)sizeof(xtime_cntxt_t));

    printf("[%llu, %llu] %04d-%02d-%02d %d %02d:%02d:%02d.%03d\n",
           xtime_meter,
           xtime_cntxt.ctx_value,
           xtime_cntxt.ctx_year,
           xtime_cntxt.ctx_month,
           xtime_cntxt.ctx_day,
           xtime_cntxt.ctx_week,
           xtime_cntxt.ctx_hour,
           xtime_cntxt.ctx_minute,
           xtime_cntxt.ctx_second,
           xtime_cntxt.ctx_msec);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////

