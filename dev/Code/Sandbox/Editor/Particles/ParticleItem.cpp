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
#include "ParticleItem.h"

#include "ParticleLibrary.h"
#include "BaseLibraryManager.h"
#include "IEditorParticleManager.h"

#include "ParticleParams.h"
#include "../AssetResolver/AssetResolver.h"

//////////////////////////////////////////////////////////////////////////
CParticleItem::CParticleItem()
    : m_pParentParticles(0)
    , m_bDebugEnabled(true)
    , m_bSaveEnabled(false)
{
    m_pEffect = GetIEditor()->GetEnv()->pParticleManager->CreateEffect();
    m_pEffect->AddRef();
    m_pEffect->SetEnabled(true);
}

//////////////////////////////////////////////////////////////////////////
CParticleItem::CParticleItem(IParticleEffect* pEffect)
    : m_pEffect(pEffect)
    , m_pParentParticles(0)
    , m_bDebugEnabled(true)
    , m_bSaveEnabled(false)
{
    assert(m_pEffect);

    // Set library item name to full effect name, without library prefix.
    string sEffectName = pEffect->GetFullName();
    const char* sItemName = sEffectName.c_str();
    if (const char* sFind = strchr(sItemName, '.'))
    {
        sItemName = sFind + 1;
    }
    m_name = sItemName;

    ResolveAssets();
}

//////////////////////////////////////////////////////////////////////////
CParticleItem::~CParticleItem()
{
    GetIEditor()->GetEnv()->pParticleManager->DeleteEffect(m_pEffect);
    CancelResolve();
}

//////////////////////////////////////////////////////////////////////////
void CParticleItem::SetName(const QString& name)
{
    if (m_pEffect)
    {
        if (!m_pEffect->GetParent())
        {
            QString fullname = GetLibrary()->GetName() + "." + name;
            m_pEffect->SetName(fullname.toUtf8().data());
        }
        else
        {
            m_pEffect->SetName(name.toUtf8().data());
        }
    }
    CBaseLibraryItem::SetName(name); 
}


void CParticleItem::AddAllChildren()
{
    m_childs.clear();

    // Add all children recursively.
    for (int i = 0; i < m_pEffect->GetChildCount(); i++)
    {
        CParticleItem* pItem = new CParticleItem(m_pEffect->GetChild(i));
        if (GetLibrary())
        {
            GetLibrary()->AddItem(pItem, true);
        }
        pItem->m_pParentParticles = this;
        m_childs.push_back(pItem);
        pItem->AddAllChildren();
    }
}

//////////////////////////////////////////////////////////////////////////
void CParticleItem::Serialize(SerializeContext& ctx, const ParticleParams* defaultParams)
{
    //NOTE: In the Particle Editor "Folders" are objects that are virtual so we need to skip those from being serialized out.
    // If we dont do this we will get extra unwanted items in our library when we next load from the file.
    //
    // to remove any "folder" items from the library we go through and look for any items without the isParticle identifier ...
    if (IsParticleItem)
    {
        XmlNodeRef node = ctx.node;
        if (ctx.bLoading)
        {
            if (!ctx.bIgnoreChilds)
            {
                ClearChilds();
            }

            m_pEffect->Serialize(node, true, !ctx.bIgnoreChilds, defaultParams);

            if (!ctx.bIgnoreChilds)
            {
                AddAllChildren();
            }
        }
        else
        {
            m_pEffect->Serialize(node, false, !ctx.bIgnoreChilds, defaultParams);
        }
    }
    else
    {
        XmlNodeRef node = ctx.node;
        if (ctx.bLoading)
        {
            if (!ctx.bIgnoreChilds)
            {
                ClearChilds();
            }

            m_pEffect->Serialize(node, true, !ctx.bIgnoreChilds, defaultParams);

            if (!ctx.bIgnoreChilds)
            {
                AddAllChildren();
            }
        }
        else
        {
            m_pEffect->Serialize(node, false, !ctx.bIgnoreChilds, defaultParams);
        }
    }
    m_bModified = false;
}

//////////////////////////////////////////////////////////////////////////
int CParticleItem::GetChildCount() const
{
    return m_childs.size();
}

//////////////////////////////////////////////////////////////////////////
CBaseLibraryItem* CParticleItem::GetChild(int index) const
{
    assert(index >= 0 && index < m_childs.size());
    return m_childs[index];
}

//////////////////////////////////////////////////////////////////////////
bool CParticleItem::GetIsEnabled()
{
    assert(m_pEffect);
    if (m_pEffect)
    {
        return m_pEffect->GetParticleParams().bEnabled;
    }
    return false;
}


//////////////////////////////////////////////////////////////////////////
void CParticleItem::SetParent(CParticleItem* pParent)
{
    if (pParent == m_pParentParticles)
    {
        return;
    }

    TSmartPtr<CParticleItem> refholder = this;
    if (m_pParentParticles)
    {
        stl::find_and_erase(m_pParentParticles->m_childs, this);
    }

    QString sNewName = GetShortName();
    m_pParentParticles = pParent;
    if (pParent)
    {
        pParent->m_childs.push_back(this);
        m_library = pParent->m_library;
        if (m_pEffect)
        {
            m_pEffect->SetParent(pParent->m_pEffect);
        }
        sNewName = pParent->GetName() + "." + sNewName;
    }
    else
    {
        m_pEffect->SetParent(NULL);
    }

    // Change name to be within group.
    sNewName = m_library->GetManager()->MakeUniqueItemName(sNewName);
    SetName(sNewName);
}

//////////////////////////////////////////////////////////////////////////
void CParticleItem::ClearChilds()
{
    // Also delete them from the library.
    for (int i = 0; i < m_childs.size(); i++)
    {
        m_childs[i]->m_pParentParticles = NULL;
        if (GetLibrary())
        {
            GetLibrary()->RemoveItem(m_childs[i]);
        }
        m_childs[i]->ClearChilds();
    }
    m_childs.clear();

    if (m_pEffect)
    {
        m_pEffect->ClearChilds();
    }
}

/*
//////////////////////////////////////////////////////////////////////////
void CParticleItem::InsertChild( int slot,CParticleItem *pItem )
{
    if (slot < 0)
        slot = 0;
    if (slot > m_childs.size())
        slot = m_childs.size();

    assert( pItem );
    pItem->m_pParentParticles = this;
    pItem->m_library = m_library;

    m_childs.insert( m_childs.begin() + slot,pItem );
    m_pMatInfo->RemoveAllSubMtls();
    for (int i = 0; i < m_childs.size(); i++)
    {
        m_pMatInfo->AddSubMtl( m_childs[i]->m_pMatInfo );
    }
}
*/

//////////////////////////////////////////////////////////////////////////
int CParticleItem::FindChild(CParticleItem* pItem)
{
    for (int i = 0; i < m_childs.size(); i++)
    {
        if (m_childs[i] == pItem)
        {
            return i;
        }
    }
    return -1;
}

//////////////////////////////////////////////////////////////////////////
CParticleItem* CParticleItem::GetParent() const
{
    return m_pParentParticles;
}

//////////////////////////////////////////////////////////////////////////
IParticleEffect* CParticleItem::GetEffect() const
{
    return m_pEffect;
}

/*
        E/S     0           1
        0/0     0/0     0/0
        0/1     0/1     1/1
        1/0     0/1     1/1
        1/1     0/1     1/1
*/
void CParticleItem::DebugEnable(int iEnable)
{
    if (m_pEffect)
    {
        if (iEnable < 0)
        {
            iEnable = !m_bDebugEnabled;
        }
        m_bDebugEnabled = iEnable != 0;
        m_bSaveEnabled = m_bSaveEnabled || m_pEffect->IsEnabled();
        m_pEffect->SetEnabled(m_bSaveEnabled && m_bDebugEnabled);
        for (int i = 0; i < m_childs.size(); i++)
        {
            m_childs[i]->DebugEnable(iEnable);
        }
    }
}

int CParticleItem::GetEnabledState() const
{
    int nEnabled = m_pEffect->IsEnabled();
    for (int i = 0; i < m_childs.size(); i++)
    {
        if (m_childs[i]->GetEnabledState())
        {
            nEnabled |= 2;
        }
    }
    return nEnabled;
}

//////////////////////////////////////////////////////////////////////////
void CParticleItem::Update()
{
    // Mark library as modified.
    if (NULL != GetLibrary())
    {
        GetLibrary()->SetModified();
    }
}

//////////////////////////////////////////////////////////////////////////
void CParticleItem::GatherUsedResources(CUsedResources& resources)
{
    if (m_pEffect->GetParticleParams().sTexture.length())
    {
        resources.Add(m_pEffect->GetParticleParams().sTexture.c_str());
    }
    if (m_pEffect->GetParticleParams().sMaterial.length())
    {
        resources.Add(m_pEffect->GetParticleParams().sMaterial.c_str());
    }
    if (m_pEffect->GetParticleParams().sGeometry.length())
    {
        resources.Add(m_pEffect->GetParticleParams().sGeometry.c_str());
    }
    if (m_pEffect->GetParticleParams().sStartTrigger.length())
    {
        resources.Add(m_pEffect->GetParticleParams().sStartTrigger.c_str());
    }
    if (m_pEffect->GetParticleParams().sStopTrigger.length())
    {
        resources.Add(m_pEffect->GetParticleParams().sStopTrigger.c_str());
    }
}

//////////////////////////////////////////////////////////////////////////
void CParticleItem::ResolveAssets()
{
    if (!m_pEffect)
    {
        return;
    }

    const ParticleParams& params = m_pEffect->GetParticleParams();

    ResolveAsset(params.sTexture.c_str(), IVariable::DT_TEXTURE);
    ResolveAsset(params.sMaterial.c_str(), IVariable::DT_MATERIAL);
    ResolveAsset(params.sGeometry.c_str(), IVariable::DT_OBJECT);
    ResolveAsset(params.sStartTrigger.c_str(), IVariable::DT_AUDIO_TRIGGER);
    ResolveAsset(params.sStopTrigger.c_str(), IVariable::DT_AUDIO_TRIGGER);
}

void CParticleItem::ResolveAsset(const char* sAsset, IVariable::EDataType type)
{
    if (*sAsset)
    {
        uint32 id = m_ResolveRequests[type];
        if (!id)
        {
            m_ResolveRequests[IVariable::DT_TEXTURE] = GetIEditor()->GetMissingAssetResolver()->AddResolveRequest(sAsset, functor(*this, &CParticleItem::OnResolved), type);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CParticleItem::CancelResolve()
{
    GetIEditor()->GetMissingAssetResolver()->CancelRequest(functor(*this, &CParticleItem::OnResolved));
}

//////////////////////////////////////////////////////////////////////////
void CParticleItem::OnResolved(uint32 id, bool success, const char* orgName, const char* newName)
{
    if (!success)
    {
        return;
    }

    ParticleParams params = m_pEffect->GetParticleParams();

    if (m_ResolveRequests[IVariable::DT_TEXTURE] == id)
    {
        params.sTexture = newName;
        m_ResolveRequests.erase(IVariable::DT_TEXTURE);
    }
    else if (m_ResolveRequests[IVariable::DT_MATERIAL] == id)
    {
        params.sMaterial = newName;
        m_ResolveRequests.erase(IVariable::DT_MATERIAL);
    }
    else if (m_ResolveRequests[IVariable::DT_OBJECT] == id)
    {
        params.sGeometry = newName;
        m_ResolveRequests.erase(IVariable::DT_OBJECT);
    }
    else if (m_ResolveRequests[IVariable::DT_AUDIO_TRIGGER] == id)
    {
        params.sStartTrigger = newName;
        m_ResolveRequests.erase(IVariable::DT_AUDIO_TRIGGER);
    }
    else
    {
        return;
    }

    m_pEffect->SetParticleParams(params);
    Update();
}

void CParticleItem::ResetToDefault(SLodInfo *curLod)
{
    _smart_ptr<IParticleEffect> pEffect = m_pEffect;
    if (curLod)
    {
        IParticleEffect* lodEffect = m_pEffect->GetLodParticle(curLod);
        if (lodEffect)
        {
            pEffect = lodEffect;
        }
    }

    if (pEffect->GetParticleParams().eInheritance == ParticleParams::EInheritance::Parent
        && pEffect->GetParent())
    {
        ParticleParams param = pEffect->GetParent()->GetParticleParams();
        param.eInheritance = ParticleParams::EInheritance::Parent;
        pEffect->SetParticleParams(param);
        return;
    }

    GetIEditor()->GetParticleManager()->LoadDefaultEmitter(pEffect);
}
