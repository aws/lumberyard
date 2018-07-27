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

#ifndef CRYINCLUDE_EDITOR_PARTICLES_PARTICLEITEM_H
#define CRYINCLUDE_EDITOR_PARTICLES_PARTICLEITEM_H
#pragma once

#include "BaseLibraryItem.h"
#include <I3DEngine.h>
#include <IParticles.h>
#include <Util/smartptr.h>
#include <Util/Variable.h>

/*! CParticleItem contain definition of particle system spawning parameters.
 *
 */
class EDITOR_CORE_API CParticleItem
    : public CBaseLibraryItem
{
public:
    CParticleItem();
    CParticleItem(IParticleEffect* pEffect);
    ~CParticleItem();

    virtual EDataBaseItemType GetType() const { return EDB_TYPE_PARTICLE; };

    virtual void SetName(const QString& name);
    void Serialize(SerializeContext& ctx) override { Serialize(ctx, nullptr); } 
    void Serialize(SerializeContext& ctx, const ParticleParams* defaultParams);

    //////////////////////////////////////////////////////////////////////////
    // Child particle systems.
    //////////////////////////////////////////////////////////////////////////
    //! Get number of sub Particles childs.
    int GetChildCount() const override;
    //! Get sub Particles child by index.
    CBaseLibraryItem* GetChild(int index) const override;
    //! Remove all sub Particles.
    void ClearChilds();
    //! Insert sub particles in between other particles.
    //void InsertChild( int slot,CParticleItem *mtl );
    //! Find slot where sub Particles stored.
    //! @retun slot index if Particles found, -1 if Particles not found.
    int  FindChild(CParticleItem* mtl);
    //! Sets new parent of item; may be 0.
    void SetParent(CParticleItem* pParent);
    //! Returns parent Particles.
    CParticleItem* GetParent() const;

    //! Called after particle parameters where updated.
    void Update();

    //! Get particle effect assigned to this particle item.
    IParticleEffect* GetEffect() const;

    bool GetIsEnabled() override;


    void DebugEnable(int iEnable = -1);
    int GetEnabledState() const;

    virtual void GatherUsedResources(CUsedResources& resources);

    void AddAllChildren();

    // file resolving
    void ResolveAssets();
    void ResolveAsset(const char* sAsset, IVariable::EDataType type);
    void CancelResolve();
    void OnResolved(uint32 id, bool success, const char* orgName, const char* newName);

    /*! Reset the particle item's parameter to default parameters or parent's if its inherientance set to parent
    *   curLod is for resetting to specific lod
    */
    void ResetToDefault(SLodInfo *curLod = nullptr);

private:
    _smart_ptr<IParticleEffect> m_pEffect;

    //! Parent of this particle (if this is a sub particle).
    CParticleItem* m_pParentParticles;
    //! Array of sub particle items.
    std::vector<TSmartPtr<CParticleItem> > m_childs;
    bool m_bSaveEnabled, m_bDebugEnabled;

    // asset resolving
    typedef std::map<IVariable::EDataType, uint32> TResolveReq;
    TResolveReq m_ResolveRequests;
};

#endif // CRYINCLUDE_EDITOR_PARTICLES_PARTICLEITEM_H

