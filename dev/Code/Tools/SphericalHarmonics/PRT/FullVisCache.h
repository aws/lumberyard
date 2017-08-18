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

#ifndef CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_FULLVISCACHE_H
#define CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_FULLVISCACHE_H
#pragma once

#if defined(OFFLINE_COMPUTATION)


#include <PRT/PRTTypes.h>
#include "RayCache.h"

namespace NSH
{
    static const uint32 gscHemisphereSamplesPerTheta = 7;   //!< samples per hemisphere for full visibility check
    static const uint32 gscHemisphereSamplesPerPhi = 14;    //!< samples per hemisphere for full visibility check
    static const uint32 gscFailThreshold = 0;                   //!< as soon as more than 4 rays hit something, it does not count as fully visible

    //!< cache manager for ray casting, since there are multiple call possibilities, we need a central class here
    //!< it quickly computes whether there is a full visibility for a certain ray casting pos or not
    //!< split into upper and lower hemisphere, lower hemisphere is only computed if required
    class CFullVisCache
    {
    public:
        CFullVisCache();    //!< standard constructor
        //!< generates the cache for a new origin
        void GenerateFullVisInfo
        (
            const NSH::TRasterCubeImpl& crRasterCube,
            const TVec& crOrigin,
            const float cRayLen,
            const TVec& cNormal,
            const float cRayTracingBias,
            const bool cOnlyUpperHemisphere = true
        );
        const bool IsUpperHemisphereVisible() const; //!< returns the result of each GenerateFullVisInfo call for upper hemisphere
        const bool IsLowerHemisphereVisible() const; //!< returns the result of each GenerateFullVisInfo call for lower hemisphere
        const bool IsFullyVisible(const TVec& crDir) const;  //!< returns the result of GenerateFullVisInfo call this vector(just determines on which half of the sphere)

    private:

        /************************************************************************************************************************************************/

        //!< breaks after the first valid hit
        class CSimpleHit
            : public NSH::CEveryObjectOnlyOnce
        {
        public:

            void SetupRay(const Vec3& crRayDir, const Vec3& crRayOrigin, const float cRayLen, const float cSign, const Vec3& crNormal, const float cBias);
            const bool IsIntersecting() const;
            const NSH::EReturnSinkValue ReturnElement(const RasterCubeUserT& crInObject, float& rInoutfRayMaxDist);

        protected:
            Vec3    m_PNormal;  //!< plane normal
            float   m_D;                //!< sign of plane normal
            bool m_HasHit;      //!< records the result of the most recent ray casting
        };

        /************************************************************************************************************************************************/


        TVecDVec    m_UpperDirections; //!< directions to query for upper hemisphere
        TVecDVec    m_LowerDirections; //!< directions to query for lower hemisphere
        bool m_UpperHemisphereIsVisible;         //!< result of each GenerateFullVisInfo call for upper hemisphere
        bool m_LowerHemisphereIsVisible;         //!< result of each GenerateFullVisInfo call for lower hemisphere
    };
}

#include "FullVisCache.inl"

#endif
#endif // CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_FULLVISCACHE_H
