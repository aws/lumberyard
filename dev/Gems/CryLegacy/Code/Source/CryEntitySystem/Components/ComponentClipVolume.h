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
#ifndef CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTCLIPVOLUME_H
#define CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTCLIPVOLUME_H
#pragma once

#include "EntitySystem.h"
#include "IEntityClass.h"
#include "ISerialize.h"
#include <IRenderMesh.h>
#include "Components/IComponentClipVolume.h"

//! Implementation of the entity's clip-volume component.
//! A clip-volume can be used to define limits for things like
//! a light's illumination.
class CComponentClipVolume
    : public IComponentClipVolume
{
public:
    CComponentClipVolume();
    ~CComponentClipVolume() override;

    //////////////////////////////////////////////////////////////////////////
    // IComponent interface implementation.
    virtual void ProcessEvent(SEntityEvent& event) override;
    bool InitComponent(IEntity* pEntity, SEntitySpawnParams& params) override;
    void Reload(IEntity* pEntity, SEntitySpawnParams& params) override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    //IComponentClipVolume
    void UpdateRenderMesh(IRenderMesh* pRenderMesh, const DynArray<Vec3>& meshFaces) override;
    IClipVolume* GetClipVolume() const override { return m_pClipVolume; }
    IBSPTree3D* GetBspTree() const override { return m_pBspTree; }
    //////////////////////////////////////////////////////////////////////////

    virtual void Serialize(TSerialize serialize) {};
    virtual void SerializeXML(XmlNodeRef& entityNodeXML, bool loading);
    virtual void Release();
    virtual bool NeedSerialize() { return false; }
    virtual bool GetSignature(TSerialize signature);
    virtual void GetMemoryUsage(ICrySizer* pSizer) const override;

private:
    bool LoadFromFile(const char* szFilePath);

private:
    // Host entity.
    IEntity* m_pEntity;

    // Engine clip volume
    IClipVolume* m_pClipVolume;

    // Render Mesh
    _smart_ptr<IRenderMesh> m_pRenderMesh;

    // BSP tree
    IBSPTree3D* m_pBspTree;

    // In-game stat obj
    string m_GeometryFileName;
};

DECLARE_SMART_POINTERS(CComponentClipVolume)

#endif // CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTCLIPVOLUME_H