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

#pragma once

// include the required files
#include "StandardHeaders.h"


namespace MCore
{
    /**
     * The class containing methods to solve quadric, cubic and quartic equations.
     */
    class MCORE_API RootSolver
    {
    public:
        /**
         * Solve a quadric equation.
         * @param c The coefficients.
         * @param s The values that will contain the solutions (output).
         * @result The number of roots.
         */
        static uint32 SolveQuadric(float c[3], float s[2]);

        /**
         * Solve a cubic equation.
         * @param c The coefficients.
         * @param s The values that will contain the solutions (output).
         * @result The number of roots.
         */
        static uint32 SolveCubic(float c[4], float s[3]);

        /**
         * Solve a quartic equation.
         * @param c The coefficients.
         * @param s The values that will contain the solutions (output).
         * @result The number of roots.
         */
        static uint32 SolveQuartic(float c[5], float s[4]);
    };
} // namespace MCore
