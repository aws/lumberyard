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
#include "AIBubblesSystem.h"
#include "AIBubblesNotifier/AIBubblesNotifier.h"

bool AIQueueBubbleMessage(const char* messageName, const EntityId entityID,
    const char* message, uint32 flags, float duration /*= 0*/, SAIBubbleRequest::ERequestType requestType /* = eRT_ErrorMessage */)
{
#ifdef CRYAISYSTEM_DEBUG
    if (IAIBubblesSystem* pAIBubblesSystem = gAIEnv.pBubblesSystem)
    {
        SAIBubbleRequest request(entityID, message, flags, duration, requestType);
        pAIBubblesSystem->QueueMessage(messageName, request);
        return true;
    }
#endif
    return false;
}

#ifdef CRYAISYSTEM_DEBUG

CAIBubblesSystem::~CAIBubblesSystem()
{
    Dispose();
}

void CAIBubblesSystem::Init()
{
    CreateBubblesNotifier();

    if (ISoftCodeMgr* pMgr = gEnv->pSoftCodeMgr)
    {
        pMgr->AddListener("BubblesSystem", this, "CAIBubblesSystem");
    }
}

void CAIBubblesSystem::Dispose()
{
    if (ISoftCodeMgr* pMgr = gEnv->pSoftCodeMgr)
    {
        pMgr->RemoveListener("BubblesSystem", this);
    }
}

void CAIBubblesSystem::InstanceReplaced(void* pOldInstance, void* pNewInstance)
{
    if (m_pBubblesNotifier == pOldInstance)
    {
        m_pBubblesNotifier = static_cast<IAIBubblesNotifier*>(pNewInstance);
    }
}

void CAIBubblesSystem::CreateBubblesNotifier()
{
    IAIBubblesNotifier::TLibrary* pLib = IAIBubblesNotifier::TLibrary::Instance();
    if (m_pBubblesNotifier = pLib->CreateInstance("CAIBubblesNotifier"))
    {
        m_pBubblesNotifier->Init();
    }
}

void CAIBubblesSystem::Reset()
{
    SOFTCODE_RETRY(m_pBubblesNotifier, m_pBubblesNotifier->Reset());
}

void CAIBubblesSystem::Update()
{
    SOFTCODE_RETRY(m_pBubblesNotifier, m_pBubblesNotifier->Update());
}

void CAIBubblesSystem::QueueMessage(const char* messageName, const SAIBubbleRequest& request)
{
    SOFTCODE_RETRY(m_pBubblesNotifier, m_pBubblesNotifier->QueueMessage(messageName, request));
}

#endif // CRYAISYSTEM_DEBUG