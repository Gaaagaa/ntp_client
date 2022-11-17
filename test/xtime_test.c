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
    xtime_vnsec_t xtm_vnsec = time_vnsec();
    xtime_descr_t xtm_descr = time_descr();
    xtime_vnsec_t xtm_ucnvt = time_dtov(xtm_descr);
    xtime_descr_t xtm_dcnvt = time_vtod(xtm_vnsec);

    printf("sizeof(xtime_vnsec_t) = %d\n", (x_int32_t)sizeof(xtime_vnsec_t));
    printf("sizeof(xtime_descr_t) = %d\n", (x_int32_t)sizeof(xtime_descr_t));

    printf("[%llu, 0x%016llX] %04d-%02d-%02d %d %02d:%02d:%02d.%03d\n",
           xtm_vnsec,
           xtm_descr.ctx_value,
           xtm_descr.ctx_year,
           xtm_descr.ctx_month,
           xtm_descr.ctx_day,
           xtm_descr.ctx_week,
           xtm_descr.ctx_hour,
           xtm_descr.ctx_minute,
           xtm_descr.ctx_second,
           xtm_descr.ctx_msec);

    printf("DTOU: 0x%016llX - 0x%016llX = %llu\n",
           xtm_vnsec, xtm_ucnvt, xtm_vnsec - xtm_ucnvt);

    printf("UTOD: [0x%016llX] %04d-%02d-%02d %d %02d:%02d:%02d.%03d\n",
           xtm_vnsec,
           xtm_dcnvt.ctx_year,
           xtm_dcnvt.ctx_month,
           xtm_dcnvt.ctx_day,
           xtm_dcnvt.ctx_week,
           xtm_dcnvt.ctx_hour,
           xtm_dcnvt.ctx_minute,
           xtm_dcnvt.ctx_second,
           xtm_dcnvt.ctx_msec);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////

