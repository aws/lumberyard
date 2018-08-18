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
#ifndef CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTCAMERA_H
#define CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTCAMERA_H
#pragma once


#include "Entity.h"
#include "EntitySystem.h"
#include "Components/IComponentCamera.h"

struct SProximityElement;

//! This component is used to render from the camera's perspective
//! to the entity's material.
class CComponentCamera
    : public IComponentCamera
{
public:
    CComponentCamera();
    IEntity* GetEntity() const { return m_pEntity; };

    //////////////////////////////////////////////////////////////////////////
    // IComponent interface implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void Initialize(const SComponentInitializer& init);
    virtual void ProcessEvent(SEntityEvent& event);
    virtual void Done() {};
    virtual void UpdateComponent(SEntityUpdateContext& ctx) {};
    virtual bool InitComponent(IEntity* pEntity, SEntitySpawnParams& params);
    virtual void Reload(IEntity* pEntity, SEntitySpawnParams& params);
    virtual void SerializeXML(XmlNodeRef& entityNode, bool bLoading) {};
    virtual void Serialize(TSerialize ser);
    virtual bool NeedSerialize() { return false; };
    virtual bool GetSignature(TSerialize signature);
    //////////////////////////////////////////////////////////////////////////

    virtual void SetCamera(CCamera& cam);
    virtual CCamera& GetCamera() { return m_camera; };
    //////////////////////////////////////////////////////////////////////////

    void UpdateMaterialCamera();

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
private:
    //////////////////////////////////////////////////////////////////////////
    // Private member variables.
    //////////////////////////////////////////////////////////////////////////
    // Host entity.
    IEntity* m_pEntity;
    CCamera m_camera;
};

#endif // CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTCAMERA_H
