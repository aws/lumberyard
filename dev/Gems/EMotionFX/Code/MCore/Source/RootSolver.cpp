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

// include the required files
#include "StandardHeaders.h"
#include "RootSolver.h"
#include "FastMath.h"
#include "Algorithms.h"


namespace MCore
{
    // solve a quadratic equation
    uint32 RootSolver::SolveQuadric(float c[3], float s[2])
    {
        float p, q, D;

        MCORE_ASSERT(Math::Abs(c[2]) > Math::epsilon);

        // normal form: x^2 + px + q = 0
        p = c[1] / (2 * c[2]);
        q = c[0] / c[2];
        D = p * p - q;

        if (MCore::InRange<float>(D, -0.000001f, +0.000001f))
        {
            s[0] = -p;
            return 1;
        }
        else if (D < 0)
        {
            return 0;
        }
        else if (D > 0)
        {
            float sqrt_D = Math::SafeSqrt(D);
            s[0] = sqrt_D - p;
            s[1] = -sqrt_D - p;
            return 2;
        }

        return 0;
    }


    uint32 RootSolver::SolveCubic(float c[4], float s[3])
    {
        uint32 i, num;
        float  sub;
        float  A, B, C;
        float  sq_A, p, q;
        float  cb_p, D;

        MCORE_ASSERT(Math::Abs(c[3]) > Math::epsilon);

        // normal form: x^3 + Ax^2 + Bx + C = 0
        A = c[2] / c[3];
        B = c[1] / c[3];
        C = c[0] / c[3];

        // substitute x = y - A/3 to eliminate quadric term: x^3 +px + q = 0
        sq_A = A * A;
        p = 0.33333333333333333333333333333333f * (-0.33333333333333333333333333333333f * sq_A + B);
        q = 0.5f * (0.074074074074074074074074074074074f * A * sq_A - 0.33333333333333333333333333333333f * A * B + C);

        // use Cardano's formula
        cb_p = p * p * p;
        D = q * q + cb_p;

        if (MCore::InRange<float>(D, -0.000001f, +0.000001f))
        {
            // one triple solution
            if (MCore::InRange<float>(q, -0.000001f, +0.000001f))
            {
                s[0] = 0;
                num  = 1;
            }
            else // one single and one float solution
            {
                float u = CubeRoot(-q);
                s[0] = 2 * u;
                s[1] = -u;
                num  = 2;
            }
        }
        else if (D < 0) // Casus irreducibilis: three real solutions
        {
            const float sqrt1 = Math::SafeSqrt(-cb_p);
            const float cosValue = (sqrt1 > Math::epsilon) ? -q / sqrt1 : 0.0f;
            float phi = 0.33333333333333333333333333333333f * Math::ACos(cosValue);
            float t = 2 * Math::SafeSqrt(-p);

            s[0] =  t * Math::Cos(phi);
            s[1] = -t* Math::Cos(phi + Math::pi * 0.33333333333333333333333333333333f);
            s[2] = -t* Math::Cos(phi - Math::pi * 0.33333333333333333333333333333333f);
            num = 3;
        }
        else // one real solution
        {
            float sqrt_D = Math::SafeSqrt(D);
            float u = CubeRoot(sqrt_D - q);
            float v = -CubeRoot(sqrt_D + q);

            s[0] = u + v;
            num  = 1;
        }

        // resubstitute
        sub = 0.33333333333333333333333333333333f * A;

        for (i = 0; i < num; ++i)
        {
            s[i] -= sub;
        }

        return num;
    }


    uint32 RootSolver::SolveQuartic(float c[5], float s[4])
    {
        float  coeffs[4];
        float  z, u, v, sub;
        float  A, B, C, D;
        float  sq_A, p, q, r;
        uint32 i, num;

        // normal form: x^4 + Ax^3 + Bx^2 + Cx + D = 0
        A = c[3] / c[4];
        B = c[2] / c[4];
        C = c[1] / c[4];
        D = c[0] / c[4];

        // substitute x = y - A/4 to eliminate cubic term: x^4 + px^2 + qx + r = 0
        sq_A = A * A;
        p = -0.375f * sq_A + B;
        q = 0.125f * sq_A * A - 0.5f * A * B + C;
        r = -0.01171875f * sq_A * sq_A + 0.0625f * sq_A * B - 0.25f * A * C + D;

        if (MCore::InRange<float>(r, -0.000001f, +0.000001f))
        {
            // no absolute term: y(y^3 + py + q) = 0
            coeffs[0] = q;
            coeffs[1] = p;
            coeffs[2] = 0;
            coeffs[3] = 1;
            num       = RootSolver::SolveCubic(coeffs, s);
            s[num++]  = 0;
        }
        else
        {
            // solve the resolvent cubic
            coeffs[0] = 0.5f * r * p - 0.125f * q * q;
            coeffs[1] = -r;
            coeffs[2] = -0.5f * p;
            coeffs[3] = 1;

            RootSolver::SolveCubic(coeffs, s);

            // take the one real solution
            z = s[0];

            // to build two quadric equations
            u = z * z - r;
            v = 2 * z - p;

            if (MCore::InRange<float>(u, -0.000001f, +0.000001f))
            {
                u = 0;
            }
            else if (u > 0)
            {
                u = Math::SafeSqrt(u);
            }
            else
            {
                return 0;
            }

            if (MCore::InRange<float>(v, -0.000001f, +0.000001f))
            {
                v = 0;
            }
            else if (v > 0)
            {
                v = Math::Sqrt(v);
            }
            else
            {
                return 0;
            }

            coeffs[0] = z - u;
            coeffs[1] = q < 0 ? -v : v;
            coeffs[2] = 1;

            num = RootSolver::SolveQuadric(coeffs, s);

            coeffs[ 0 ] = z + u;
            coeffs[ 1 ] = q < 0 ? v : -v;
            coeffs[ 2 ] = 1;

            num += RootSolver::SolveQuadric(coeffs, s + num);
        }

        // resubstitute
        sub = 0.25f * A;

        for (i = 0; i < num; ++i)
        {
            s[i] -= sub;
        }

        return num;
    }
}   // namespace MCore
