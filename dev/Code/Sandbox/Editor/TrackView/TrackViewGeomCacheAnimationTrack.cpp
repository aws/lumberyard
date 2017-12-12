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

#include "StdAfx.h"
#if defined(USE_GEOM_CACHES)
#include "TrackViewGeomCacheAnimationTrack.h"
#include "TrackViewSequence.h"

#include <IGeomCache.h>

CTrackViewKeyHandle CTrackViewGeomCacheAnimationTrack::CreateKey(const float time)
{
    CTrackViewSequence* pSequence = GetSequence();
    CTrackViewSequenceNotificationContext context(GetSequence());

    CTrackViewKeyHandle keyHandle = CTrackViewTrack::CreateKey(time);

    // Find editor object who owns this node.
    IEntity* pEntity = keyHandle.GetTrack()->GetAnimNode()->GetEntity();
    if (pEntity)
    {
        const IGeomCacheRenderNode* pGeomCacheRenderNode = pEntity->GetGeomCacheRenderNode(0);
        if (pGeomCacheRenderNode)
        {
            const IGeomCache* pGeomCache = pGeomCacheRenderNode->GetGeomCache();

            if (pGeomCache)
            {
                ITimeRangeKey timeRangeKey;
                keyHandle.GetKey(&timeRangeKey);
                timeRangeKey.m_duration = pGeomCache->GetDuration();
                timeRangeKey.m_endTime = timeRangeKey.m_duration;
                keyHandle.SetKey(&timeRangeKey);
                GetSequence()->OnKeysChanged();
            }
        }
    }

    return keyHandle;
}
#endif