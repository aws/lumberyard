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

#if defined(OFFLINE_COMPUTATION)

#include "RasterCube.h"


inline NSH::CRayCache::CRayCache()
{
}

inline void NSH::CRayCache::Reset()
{
    //mark with no hit storage
    m_LastIntersectionObjects.resize(0);
}

inline void NSH::CRayCache::RecordHit(const RasterCubeUserT* cpInObject)
{
    m_LastIntersectionObjects.push_back(cpInObject);
}

template <class THitSink>
const bool NSH::CRayCache::CheckCache(THitSink& rSink) const
{
    if (!m_LastIntersectionObjects.empty())
    {
        float dist = std::numeric_limits<float>::max();
        const TRasterCubeUserTPtrVec::const_iterator cEnd = m_LastIntersectionObjects.end();
        for (TRasterCubeUserTPtrVec::const_iterator iter = m_LastIntersectionObjects.begin(); iter != cEnd; ++iter)
        {
            //return only true if all recorded hits intersect
            if (rSink.ReturnElement(*(*iter), dist) == RETURN_SINK_FAIL)
            {
                //didnt hit
                rSink.ResetHits();//reset to as if nothing was tested
                return false;
            }
        }
        return true;//all hits have hit again
    }
    return false;
}

#endif