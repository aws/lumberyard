/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

/*
Perlin Noise
https://mrl.nyu.edu/~perlin/doc/oscar.html

Copyright Ken Perlin

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
*/

#include "StdAfx.h"

#include <vector>
#include <math.h>

#define B 0x100
#define BM 0xff
#define N 0x1000
#define NP 12   /* 2^N */
#define NM 0xfff
#define s_curve(t) (t * t * (3. - 2. * t))
#define lerp(t, a, b) (a + t * (b - a))
#define setup(i, b0, b1, r0, r1) \
        t = vec[i] + N;                  \
        b0 = ((int)t) & BM;              \
        b1 = (b0 + 1) & BM;              \
        r0 = t - (int)t;                 \
        r1 = r0 - 1.;
#define at2(rx, ry) (rx * q[0] + ry * q[1])
#define at3(rx, ry, rz) (rx * q[0] + ry * q[1] + rz * q[2])

namespace CloudsGem
{
    namespace Noise
    {
        //////////////////////////////////////////////////////////////////////////
        // Coherent noise function over 1, 2 or 3 dimensions
        // (copyright Ken Perlin)
        void Normalize3(double v[3]);

        static int pp[B + B + 2];
        static double g3[B + B + 2][3];
        static int start = 1;

        void Init()
        {
            int i, j, k;

            for (i = 0; i < B; i++)
            {
                pp[i] = i;
                for (j = 0; j < 3; j++)
                {
                    g3[i][j] = cry_random(-1.0f, 1.0f);
                }
                Normalize3(g3[i]);
            }

            while (--i)
            {
                k = pp[i];
                pp[i] = pp[j = cry_random(0, B - 1)];
                pp[j] = k;
            }

            for (i = 0; i < B + 2; i++)
            {
                pp[B + i] = pp[i];
                for (j = 0; j < 3; j++)
                {
                    g3[B + i][j] = g3[i][j];
                }
            }
        }

        double Noise3(double vec[3])
        {
            int bx0, bx1, by0, by1, bz0, bz1, b00, b10, b01, b11;
            double rx0, rx1, ry0, ry1, rz0, rz1, *q, sy, sz, a, b, c, d, t, u, v;
            int i, j;

            if (start)
            {
                start = 0;
                Init();
            }

            setup(0, bx0, bx1, rx0, rx1);
            setup(1, by0, by1, ry0, ry1);
            setup(2, bz0, bz1, rz0, rz1);

            i = pp[bx0];
            j = pp[bx1];

            b00 = pp[i + by0];
            b10 = pp[j + by0];
            b01 = pp[i + by1];
            b11 = pp[j + by1];

            t = s_curve(rx0);
            sy = s_curve(ry0);
            sz = s_curve(rz0);

            q = g3[b00 + bz0];
            u = at3(rx0, ry0, rz0);
            q = g3[b10 + bz0];
            v = at3(rx1, ry0, rz0);
            a = lerp(t, u, v);

            q = g3[b01 + bz0];
            u = at3(rx0, ry1, rz0);
            q = g3[b11 + bz0];
            v = at3(rx1, ry1, rz0);
            b = lerp(t, u, v);

            c = lerp(sy, a, b);

            q = g3[b00 + bz1];
            u = at3(rx0, ry0, rz1);
            q = g3[b10 + bz1];
            v = at3(rx1, ry0, rz1);
            a = lerp(t, u, v);

            q = g3[b01 + bz1];
            u = at3(rx0, ry1, rz1);
            q = g3[b11 + bz1];
            v = at3(rx1, ry1, rz1);
            b = lerp(t, u, v);

            d = lerp(sy, a, b);

            return lerp(sz, c, d);
        }

        void Normalize3(double v[3])
        {
            double s = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
            v[0] = v[0] / s;
            v[1] = v[1] / s;
            v[2] = v[2] / s;
        }

        float PerlinNoise3D(double x, double y, double z, double alpha, double beta, int n)
        {
            int i;
            double val, sum = 0;
            double p[3], scale = 1;

            p[0] = x;
            p[1] = y;
            p[2] = z;
            for (i = 0; i < n; i++)
            {
                val = Noise3(p);
                sum += val / scale;
                scale *= alpha;
                p[0] *= beta;
                p[1] *= beta;
                p[2] *= beta;
            }
            return aznumeric_cast<float>(sum);
        }
    }
}

#undef B
#undef BM
#undef N
#undef NP
#undef NM
#undef lerp
