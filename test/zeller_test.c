/**
 * @file zeller_test.c
 * Copyright (c) 2022 Gaaagaa. All rights reserved.
 * 
 * @author  : Gaaagaa
 * @date    : 2022-11-04
 * @version : 1.0.0.0
 * @brief   : 对 Zeller 公式的实现函数进行测试。
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

#define XTIME_DAY_STEP  (24ULL * 60ULL * 60ULL * 1000ULL * 1000ULL * 10ULL)

int main(int argc, char * argv[])
{
    xtime_vnsec_t xtm_vnsec = 0ULL;
    xtime_descr_t xtm_descr = { 0ULL };

    x_uint32_t xut_iter = 0;
    x_uint32_t xut_week = 0;
    x_uint32_t xut_nerr = 0;

    for (; xut_iter < 365000U; ++xut_iter)
    {
        xtm_descr = time_vtod(xtm_vnsec);
        xut_week  = time_week(xtm_descr.ctx_year, xtm_descr.ctx_month, xtm_descr.ctx_day);

        if (xut_week != xtm_descr.ctx_week)
        {
            printf("[ 0x%016llX ] [ %04d-%02d-%02d %d %02d:%02d:%02d.%03d ] != week[ %u ]\n",
                   xtm_vnsec           ,
                   xtm_descr.ctx_year  ,
                   xtm_descr.ctx_month ,
                   xtm_descr.ctx_day   ,
                   xtm_descr.ctx_week  ,
                   xtm_descr.ctx_hour  ,
                   xtm_descr.ctx_minute,
                   xtm_descr.ctx_second,
                   xtm_descr.ctx_msec  ,
                   xut_week            );

            xut_nerr += 1;
        }

        xtm_vnsec += XTIME_DAY_STEP;
    }

    printf("xut_nerr = %u\n", xut_nerr);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
