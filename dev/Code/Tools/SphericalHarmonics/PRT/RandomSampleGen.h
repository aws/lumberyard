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

#ifndef CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_RANDOMSAMPLEGEN_H
#define CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_RANDOMSAMPLEGEN_H
#pragma once

#include "ISampleGenerator.h"
#include <time.h>

namespace NSH
{
    //!< configuration enum for sample generator
    namespace NSampleGen
    {
        //!< naive sample generation by just distributing the samples via Random
        struct SSampleGenPolicyNaive
            : public ISampleGenPolicy
        {
            virtual void GenerateSamples(TRandomSampleVec& rSamples, const uint32 cMaxSamples)  //!< sample generation
            {
                // generate simple random 2D samples with high variance
                rSamples.resize(cMaxSamples);
                for (uint32 i = 0; i < cMaxSamples; i++)
                {
                    rSamples[i].x = Random();
                    rSamples[i].y = Random();
                }
            }
        };

        /************************************************************************************************************************************************/

        //!< stratisfied sample generation by randomly distributing the samples in a regular grid
        struct SSampleGenPolicyStratisfied
            : public ISampleGenPolicy
        {
            virtual void GenerateSamples(TRandomSampleVec& rSamples, const uint32 cMaxSamples)//!< sample generation
            {
                rSamples.clear();
                // reduce the variance of the generated samples by breaking the sample space into an NxN grid,
                // iterating through each grid cell, and generating a random sample within that
                const uint32 cSqrtSamples = (uint32)Sqrt((double)cMaxSamples);
                const double oneoverN = 1.0f / (double)cSqrtSamples;
                for (uint32 a = 0; a < cSqrtSamples; a++)
                {
                    for (uint32 b = 0; b < cSqrtSamples; b++)
                    {
                        // generate unbiased distribution of spherical coords
                        double x = ((double)a + Random()) * oneoverN;
                        double y = ((double)b + Random()) * oneoverN;
                        rSamples.push_back(TVec2D(x, y));
                    }
                }
            }
        };

        /************************************************************************************************************************************************/

        //!< hammersley sample generation by randomly distributing the samples in a regular grid
        struct SSampleGenPolicyHammersley
            : public ISampleGenPolicy
        {
            virtual void GenerateSamples(TRandomSampleVec& rSamples, const uint32 cMaxSamples)//!< sample generation
            {
                const double cInvCount = 1. / (double)cMaxSamples;
                rSamples.clear();
                const double cStart = (rand() % 1000) * 0.001f;
                double p, t;
                uint32 k, kk, pos;
                for (k = 0, pos = 0; k < cMaxSamples; k++)
                {
                    t = cStart;
                    for (p = 0.5, kk = k; kk; p *= 0.5, kk >>= 1)
                    {
                        if (kk & 1)                           // kk mod 2 == 1
                        {
                            t += p;
                        }
                    }
                    if (t > 1)
                    {
                        t--;
                    }
                    rSamples.push_back(TVec2D(t, ((k + 0.5f) * cInvCount)));
                }
            }
        };
    }
}









#endif // CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_RANDOMSAMPLEGEN_H
