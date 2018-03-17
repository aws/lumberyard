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

#ifndef CRYINCLUDE_CRY3DENGINE_PHYSCALLBACKS_H
#define CRYINCLUDE_CRY3DENGINE_PHYSCALLBACKS_H
#pragma once

#include <IDeferredCollisionEvent.h>
#include <AzCore/Jobs/LegacyJobExecutor.h>

class CPhysCallbacks
    : public Cry3DEngineBase
{
public:
    static void Init();
    static void Done();

    static int OnFoliageTouched(const EventPhys* pEvent);
    static int OnPhysStateChange(const EventPhys* pEvent);
    static int OnPhysAreaChange(const EventPhys* pEvent);
};

class CPhysStreamer
    : public IPhysicsStreamer
{
public:
    virtual int CreatePhysicalEntity(PhysicsForeignData pForeignData, int iForeignData, int iForeignFlags);
    virtual int DestroyPhysicalEntity(IPhysicalEntity* pent) { return 1; }
    virtual int CreatePhysicalEntitiesInBox(const Vec3& boxMin, const Vec3& boxMax);
    virtual int DestroyPhysicalEntitiesInBox(const Vec3& boxMin, const Vec3& boxMax);
};

// Deferred physics event object implementing CPhysCallbacks::OnCollision
class CDeferredCollisionEventOnPhysCollision
    : public IDeferredPhysicsEvent
    , public Cry3DEngineBase
{
public:
    virtual ~CDeferredCollisionEventOnPhysCollision();

    // Factory create function to pass to CDeferredPhysicsManager::HandleEvent
    static IDeferredPhysicsEvent* CreateCollisionEvent(const EventPhys* pCollisionEvent);

    // Entry function to register as event handler with physics
    static int OnCollision(const EventPhys* pEvent);


    // == IDeferredPhysicsEvent interface == //
    virtual void Start();
    virtual int Result(EventPhys*);
    virtual void Sync();
    virtual bool HasFinished();

    virtual IDeferredPhysicsEvent::DeferredEventType GetType() const { return PhysCallBack_OnCollision; }
    virtual EventPhys* PhysicsEvent(){ return &m_CollisionEvent; }


    // == IThreadTask interface == //
    virtual void OnUpdate();
    virtual void Stop();
    virtual SThreadTaskInfo* GetTaskInfo();

    void* operator new(size_t);
    void operator delete(void*);

private:
    // Private constructor, only allow creating by the factory function(which is used by the deferred physics event manager
    CDeferredCollisionEventOnPhysCollision(const EventPhysCollision* pCollisionEvent);

    // == Functions implementing the event logic == //
    void RayTraceVegetation();
    void TestCollisionWithRenderMesh();
    void FinishTestCollisionWithRenderMesh();
    void PostStep();
    void AdjustBulletVelocity();
    void UpdateFoliage();

    // == utility functions == //
    void MarkFinished(int nResult);

    // == state variables to sync the asynchron execution == //
    AZ::LegacyJobExecutor m_jobCompletion;

    volatile bool m_bTaskRunning;

    // == members for internal state of the event == //
    bool m_bHit;                                                            // did  the RayIntersection hit something
    bool m_bFinished;                                                   // did an early out happen and we are finished with computation
    bool m_bPierceable;                                             // remeber if we are hitting a pierceable object
    int m_nResult;                                                      // result value of the event(0 or 1)
    CDeferredCollisionEventOnPhysCollision* m_pNextEvent;
    int m_nWaitForPrevEvent;

    // == internal data == //
    EventPhysCollision m_CollisionEvent;            // copy of the original physics event
    SRayHitInfo m_HitInfo;                                      // RayIntersection result data
    _smart_ptr<IRenderMesh> m_pRenderMesh;      // Rendermesh to use for RayIntersection
    bool m_bDecalPlacementTestRequested;            // check if decal can be placed here
    _smart_ptr<IMaterial> m_pMaterial;              // Material for IMaterial
    bool m_bNeedMeshThreadUnLock;                           // remeber if we need to unlock a rendermesh

    // == members to store values over functions == //
    Matrix34 m_worldTM;
    Matrix33 m_worldRot;
    int* m_pMatMapping;
    int m_nMats;

    // == States for ThreadTask and AsyncRayIntersection == //
    SThreadTaskInfo m_threadTaskInfo;
    SIntersectionData m_RayIntersectionData;
};

#endif // CRYINCLUDE_CRY3DENGINE_PHYSCALLBACKS_H
