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

#include "CryLegacy_precompiled.h"
#include "CryAction.h"
#include "IGameRulesSystem.h"
#include "ActionGame.h"
#include "DelayedPlaneBreak.h"
#include "ParticleParams.h"
#include "IBreakableManager.h"

void CDelayedPlaneBreak::OnUpdate()
{
    m_islandIn.bCreateIsle = (m_islandIn.processFlags & ePlaneBreak_AutoSmash) == 0;
    if (!m_bMeshPrepOnly)
    {
        gEnv->pEntitySystem->GetBreakableManager()->ExtractPlaneMeshIsland(m_islandIn, m_islandOut);
    }
    else
    {
        m_islandOut.pStatObj = m_islandIn.pStatObj;
        m_islandOut.pStatObj->GetIndexedMesh(true);
    }
    if (m_threadTaskInfo.m_pThread)
    {
        gEnv->pSystem->GetIThreadTaskManager()->UnregisterTask(this);
    }

    CDelayedPlaneBreak* pdpb = this - m_idx;
    if (!(m_islandIn.pStatObj->GetFlags() & STATIC_OBJECT_CLONE) &&
        (m_islandOut.pStatObj->GetFlags() & STATIC_OBJECT_CLONE))
    {
        for (int i = 0; i < m_count; ++i)
        {
            if (pdpb[i].m_status != NONE &&
                i != m_idx &&
                pdpb[i].m_islandOut.pStatObj == m_islandIn.pStatObj &&
                pdpb[i].m_epc.pEntity[1] == m_epc.pEntity[1] &&
                pdpb[i].m_epc.partid[1] == m_epc.partid[1])
            {
                IStatObj* oldSrc = pdpb[i].m_islandIn.pStatObj;
                pdpb[i].m_islandIn.pStatObj = m_islandOut.pStatObj;
                pdpb[i].m_islandOut.pStatObj = m_islandOut.pStatObj;
                m_islandOut.pStatObj->AddRef();
                oldSrc->Release();
            }
        }
    }
    m_status = DONE;
}
