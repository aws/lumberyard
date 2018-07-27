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

// Description : Manages AI proxies

#ifndef CRYINCLUDE_CRYACTION_AI_AIPROXYMANAGER_H
#define CRYINCLUDE_CRYACTION_AI_AIPROXYMANAGER_H
#pragma once

#include "IEntitySystem.h"
#include "IAIActorProxy.h"
#include "IAIGroupProxy.h"

class CAIProxyManager
    : public IEntitySystemSink
    , public IAIActorProxyFactory
    , public IAIGroupProxyFactory
{
public:
    CAIProxyManager();
    virtual ~CAIProxyManager();

    void Init();
    void Shutdown();

    IAIActorProxy* GetAIActorProxy(EntityId entityid) const;
    void OnAIProxyDestroyed(IAIActorProxy* pProxy);

    // IAIActorProxyFactory
    virtual IAIActorProxy* CreateActorProxy(EntityId entityID);
    // ~IAIActorProxyFactory

    // IAIGroupProxyFactory
    virtual IAIGroupProxy* CreateGroupProxy(int groupID);
    // ~IAIGroupProxyFactory

    // IEntitySystemSink
    virtual bool OnBeforeSpawn(SEntitySpawnParams& params);
    virtual void OnSpawn(IEntity* pEntity, SEntitySpawnParams& params);
    virtual bool OnRemove(IEntity* pEntity);
    virtual void OnReused(IEntity* pEntity, SEntitySpawnParams& params);
    virtual void OnEvent(IEntity* pEntity, SEntityEvent& event);
    //~IEntitySystemSink

private:
    typedef std::map< EntityId, CAIProxy* > TAIProxyMap;
    TAIProxyMap m_aiProxyMap;
};

#endif // CRYINCLUDE_CRYACTION_AI_AIPROXYMANAGER_H
