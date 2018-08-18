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
#include "LiveMocapScene.h"

/*

  CLiveMocapScene

*/

CLiveMocapScene::CLiveMocapScene()
{
    m_pEntity = NULL;
}

CLiveMocapScene::~CLiveMocapScene()
{
    while (!m_actors.empty())
    {
        m_actors.back()->Release();
        m_actors.pop_back();
    }
}

//

CLiveMocapActor* CLiveMocapScene::GetActor(const char* name)
{
    size_t actorCount = m_actors.size();
    for (size_t i = 0; i < actorCount; ++i)
    {
        if (::_stricmp(m_actors[i]->GetName(), name) == 0)
        {
            return m_actors[i];
        }
    }

    return NULL;
}

void CLiveMocapScene::DeleteActors()
{
    size_t actorCount = m_actors.size();
    for (size_t i = 0; i < actorCount; ++i)
    {
        m_actors[i]->Release();
    }
    m_actors.clear();
}

void CLiveMocapScene::AddListener(ILiveMocapSceneListener* pListener)
{
    size_t listenerCount = m_listeners.size();
    for (size_t i = 0; i < listenerCount; ++i)
    {
        if (m_listeners[i] == pListener)
        {
            return;
        }
    }

    m_listeners.push_back(pListener);
}

void CLiveMocapScene::RemoveListener(ILiveMocapSceneListener* pListener)
{
    size_t listenerCount = m_listeners.size();
    for (size_t i = 0; i < listenerCount; ++i)
    {
        if (m_listeners[i] != pListener)
        {
            continue;
        }

        m_listeners[i] = m_listeners.back();
        m_listeners.pop_back();
        return;
    }
}

void CLiveMocapScene::Reset()
{
    m_pEntity = NULL;

    size_t actorCount = m_actors.size();
    for (size_t i = 0; i < actorCount; ++i)
    {
        m_actors[i]->Reset();
    }
}

void CLiveMocapScene::Update()
{
    if (!m_pEntity)
    {
        return;
    }

    QuatT origin(IDENTITY);
    origin.t = m_pEntity->GetPos();
    origin.q = m_pEntity->GetRotation();

    size_t actorCount = m_actors.size();
    for (size_t i = 0; i < actorCount; ++i)
    {
        m_actors[i]->Update(origin);
    }
}

// ILMScene

ILMActor* CLiveMocapScene::CreateActor(LMString name, const LMSet* pSet)
{
    CLiveMocapActor* pActor = GetActor(name);
    if (pActor)
    {
        return pActor;
    }

    pActor = CLiveMocapActor::Create(name, pSet);
    if (!pActor)
    {
        return NULL;
    }

    m_actors.push_back(pActor);

    size_t listenerCount = m_listeners.size();
    for (size_t i = 0; i < listenerCount; ++i)
    {
        m_listeners[i]->OnCreateActor(*this, *pActor);
    }

    return pActor;
}