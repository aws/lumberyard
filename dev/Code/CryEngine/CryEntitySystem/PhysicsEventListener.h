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

#ifndef CRYINCLUDE_CRYENTITYSYSTEM_PHYSICSEVENTLISTENER_H
#define CRYINCLUDE_CRYENTITYSYSTEM_PHYSICSEVENTLISTENER_H
#pragma once

// forward declaration.
class CEntitySystem;
struct IPhysicalWorld;

//////////////////////////////////////////////////////////////////////////
class CPhysicsEventListener
{
public:
    CPhysicsEventListener(CEntitySystem* pEntitySystem, IPhysicalWorld* pPhysics);
    ~CPhysicsEventListener();

    static int OnBBoxOverlap(const EventPhys* pEvent);
    static int OnStateChange(const EventPhys* pEvent);
    static int OnPostStep(const EventPhys* pEvent);
    static int OnUpdateMesh(const EventPhys* pEvent);
    static int OnCreatePhysEntityPart(const EventPhys* pEvent);
    static int OnRemovePhysEntityParts(const EventPhys* pEvent);
    static int OnRevealPhysEntityPart(const EventPhys* pEvent);
    static int OnPreUpdateMesh(const EventPhys* pEvent);
    static int OnPreCreatePhysEntityPart(const EventPhys* pEvent);
    static int OnCollision(const EventPhys* pEvent);
    static int OnJointBreak(const EventPhys* pEvent);
    static int OnPostPump(const EventPhys* pEvent);

    void RegisterPhysicCallbacks();
    void UnregisterPhysicCallbacks();

private:
    static IEntity* GetEntity(void* pForeignData, int iForeignData);
    static IEntity* GetEntity(IPhysicalEntity* pPhysEntity);

    CEntitySystem* m_pEntitySystem;
    IPhysicalWorld* m_pPhysics;

    struct PhysVisAreaUpdate
    {
        PhysVisAreaUpdate(IRenderNode* pRndNode, IPhysicalEntity* pEntity) { m_pRndNode = pRndNode; m_pEntity = pEntity; }
        IRenderNode* m_pRndNode;
        IPhysicalEntity* m_pEntity;
    };
    static std::vector<PhysVisAreaUpdate> m_physVisAreaUpdateVector;
};

#endif // CRYINCLUDE_CRYENTITYSYSTEM_PHYSICSEVENTLISTENER_H

