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
#include "LayerNodeAnimator.h"
#include "TrackViewTrack.h"
#include "Objects/ObjectLayerManager.h"

//-----------------------------------------------------------------------------
void CLayerNodeAnimator::Animate(CTrackViewAnimNode* pNode, const SAnimContext& ac)
{
    if (GetIEditor()->IsInGameMode())
    {
        return;
    }

    CTrackViewTrack* pTrack = pNode->GetTrackForParameter(eAnimParamType_Visibility);
    if (pTrack)
    {
        bool visible = true;
        pTrack->GetValue(ac.time, visible);

        CObjectLayerManager* pLayerManager =
            GetIEditor()->GetObjectManager()->GetLayersManager();

        if (pLayerManager)
        {
            CObjectLayer* pLayer = pLayerManager->FindLayerByName(pNode->GetName());
            if (pLayer)
            {
                pLayer->SetVisible(visible);
            }
        }
    }
}