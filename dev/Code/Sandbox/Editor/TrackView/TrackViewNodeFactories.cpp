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
#include "TrackViewNodeFactories.h"
#include "TrackViewAnimNode.h"
#include "TrackViewCameraNode.h"
#include "TrackViewEventNode.h"
#include "TrackViewTrack.h"
#include "TrackViewGeomCacheAnimationTrack.h"

CTrackViewAnimNode* CTrackViewAnimNodeFactory::BuildAnimNode(IAnimSequence* pSequence, IAnimNode* pAnimNode, CTrackViewNode* pParentNode)
{
    CTrackViewAnimNode* retNode = nullptr;

    if (pAnimNode->GetType() == eAnimNodeType_Camera)
    {
        retNode = new CTrackViewCameraNode(pSequence, pAnimNode, pParentNode);
    }
    else if (pAnimNode->GetType() == eAnimNodeType_Event)
    {
        retNode = new CTrackViewEventNode(pSequence, pAnimNode, pParentNode);
    }
    else
    {
        retNode = new CTrackViewAnimNode(pSequence, pAnimNode, pParentNode);
    }

    return retNode;
}

CTrackViewTrack* CTrackViewTrackFactory::BuildTrack(IAnimTrack* pTrack, CTrackViewAnimNode* pTrackAnimNode,
    CTrackViewNode* pParentNode, bool bIsSubTrack, unsigned int subTrackIndex)
{
#if defined(USE_GEOM_CACHES)
    if (pTrack->GetParameterType() == eAnimParamType_TimeRanges && pTrackAnimNode->GetType() == eAnimNodeType_GeomCache)
    {
        return new CTrackViewGeomCacheAnimationTrack(pTrack, pTrackAnimNode, pParentNode, bIsSubTrack, subTrackIndex);
    }
#endif

    return new CTrackViewTrack(pTrack, pTrackAnimNode, pParentNode, bIsSubTrack, subTrackIndex);
}