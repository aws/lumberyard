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

#ifndef CRYINCLUDE_EDITOR_LIVEMOCAP_LIVEMOCAPSCENE_H
#define CRYINCLUDE_EDITOR_LIVEMOCAP_LIVEMOCAPSCENE_H
#pragma once


#include "../../SDKs/LiveMocap/LiveMocap.h"

#include "LiveMocapActor.h"

class CLiveMocapScene;

class ILiveMocapSceneListener
{
public:
    virtual void OnCreateActor(const CLiveMocapScene& scene, const CLiveMocapActor& actor) = 0;
};

class CLiveMocapScene
    : public ILMScene
{
public:
    CLiveMocapScene();
    ~CLiveMocapScene();

public:
    uint32 GetActorCount() const { return (uint32)m_actors.size(); }
    CLiveMocapActor* GetActor(uint32 index) { return m_actors[index]; }
    CLiveMocapActor* GetActor(const char* name);
    const CLiveMocapActor* GetActor(uint32 index) const { return m_actors[index]; }
    void DeleteActors();

    void AddListener(ILiveMocapSceneListener* pListener);
    void RemoveListener(ILiveMocapSceneListener* pListener);

    void SetEntity(IEntity* pEntity) { m_pEntity = pEntity; }
    IEntity* GetEntity() { return m_pEntity; }

    void Reset();

    void Update();

    // ILCScene
public:
    ILMActor* CreateActor(LMString name, const LMSet* pSet);

private:
    std::vector<CLiveMocapActor*> m_actors;
    std::vector<ILiveMocapSceneListener*> m_listeners;

    IEntity* m_pEntity;
};

#endif // CRYINCLUDE_EDITOR_LIVEMOCAP_LIVEMOCAPSCENE_H
