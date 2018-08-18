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

// Description : Crytek Common render helper functions and structures declarations.


#include "StdAfx.h"


// Resource manager internal variables.
ResourceClassMap CBaseResource::m_sResources;
CryCriticalSection CBaseResource::s_cResLock;

CBaseResource& CBaseResource::operator=(const CBaseResource& Src)
{
    return *this;
}

bool CBaseResource::IsValid()
{
    AUTO_LOCK(s_cResLock); // Not thread safe without this

    SResourceContainer* pContainer = GetResourcesForClass(m_ClassName);
    if (!pContainer)
    {
        return false;
    }

    ResourceClassMapItor itRM = m_sResources.find(m_ClassName);

    if (itRM == m_sResources.end())
    {
        return false;
    }
    if (itRM->second != pContainer)
    {
        return false;
    }
    ResourcesMapItor itRL = itRM->second->m_RMap.find(m_NameCRC);
    if (itRL == itRM->second->m_RMap.end())
    {
        return false;
    }
    if (itRL->second != this)
    {
        return false;
    }

    return true;
}

SResourceContainer* CBaseResource::GetResourcesForClass(const CCryNameTSCRC& className)
{
    ResourceClassMapItor itRM = m_sResources.find(className);
    if (itRM == m_sResources.end())
    {
        return NULL;
    }
    return itRM->second;
}

CBaseResource* CBaseResource::GetResource(const CCryNameTSCRC& className, int nID, bool bAddRef)
{
    FUNCTION_PROFILER_RENDER_FLAT
        AUTO_LOCK(s_cResLock); // Not thread safe without this

    SResourceContainer* pRL = GetResourcesForClass(className);
    if (!pRL)
    {
        return NULL;
    }

    int nIndex = RListIndexFromId(nID);

    //assert(pRL->m_RList.size() > nID);
    if (nIndex >= (int)pRL->m_RList.size() || nIndex < 0)
    {
        return NULL;
    }
    CBaseResource* pBR = pRL->m_RList[nIndex];
    if (pBR)
    {
        if (bAddRef)
        {
            pBR->AddRef();
        }
        return pBR;
    }
    return NULL;
}

CBaseResource* CBaseResource::GetResource(const CCryNameTSCRC& className, const CCryNameTSCRC& Name, bool bAddRef)
{
    FUNCTION_PROFILER_RENDER_FLAT
        AUTO_LOCK(s_cResLock); // Not thread safe without this

    SResourceContainer* pRL = GetResourcesForClass(className);
    if (!pRL)
    {
        return NULL;
    }

    CBaseResource* pBR = NULL;
    ResourcesMapItor itRL = pRL->m_RMap.find(Name);
    if (itRL != pRL->m_RMap.end())
    {
        pBR = itRL->second;
        if (bAddRef)
        {
            pBR->AddRef();
        }
        return pBR;
    }
    return NULL;
}

bool CBaseResource::Register(const CCryNameTSCRC& className, const CCryNameTSCRC& Name)
{
    AUTO_LOCK(s_cResLock); // Not thread safe without this

    SResourceContainer* pRL = GetResourcesForClass(className);
    if (!pRL)
    {
        pRL = new SResourceContainer;
        m_sResources.insert(ResourceClassMapItor::value_type(className, pRL));
    }

    assert(pRL);
    if (!pRL)
    {
        return false;
    }

    ResourcesMapItor itRL = pRL->m_RMap.find(Name);
    if (itRL != pRL->m_RMap.end())
    {
        return false;
    }
    pRL->m_RMap.insert(ResourcesMapItor::value_type(Name, this));
    int nIndex;
    if (pRL->m_AvailableIDs.size())
    {
        ResourceIds::iterator it = pRL->m_AvailableIDs.end() - 1;
        nIndex = RListIndexFromId(*it);
        pRL->m_AvailableIDs.erase(it);
        assert(nIndex < (int)pRL->m_RList.size());
        pRL->m_RList[nIndex] = this;
    }
    else
    {
        nIndex = pRL->m_RList.size();
        pRL->m_RList.push_back(this);
    }
    m_nID = IdFromRListIndex(nIndex);
    m_NameCRC = Name;
    m_ClassName = className;
    m_nRefCount = 1;

    return true;
}

bool CBaseResource::UnRegister()
{
    AUTO_LOCK(s_cResLock); // Not thread safe without this

    if (IsValid())
    {
        SResourceContainer* pContainer = GetResourcesForClass(m_ClassName);
        assert(pContainer);
        if (pContainer)
        {
            pContainer->m_RMap.erase(m_NameCRC);
            pContainer->m_RList[RListIndexFromId(m_nID)] = NULL;
            pContainer->m_AvailableIDs.push_back(m_nID);
        }
        return true;
    }
    return false;
}

int32 CBaseResource::Release()
{
    IF (m_nRefCount > 0, 1)
    {
        int32 nRef = CryInterlockedDecrement(&m_nRefCount);
        if (nRef < 0)
        {
            CryFatalError("CBaseResource::Release() called more than once!");
        }

        if (nRef <= 0)
        {
            UnRegister();
            if (gRenDev && gRenDev->m_pRT)
            {
                gRenDev->m_pRT->RC_ReleaseBaseResource(this);
            }
            return 0;
        }
        return nRef;
    }
    return 0;
}

//=================================================================

SResourceContainer::~SResourceContainer()
{
    for (ResourcesMapItor it = m_RMap.begin(); it != m_RMap.end(); )
    {
        CBaseResource* pRes = it->second;
        if (pRes && CRenderer::CV_r_printmemoryleaks)
        {
            iLog->Log("Warning: ~SResourceContainer: Resource %d was not deleted (%d)", pRes->GetID(), pRes->GetRefCounter());
        }
        ++it; // advance "it" here because the safe release below usually invalidates "it" (calls m_RMap.erase(it))
        SAFE_RELEASE(pRes);
    }
    m_RMap.clear();
    m_RList.clear();
    m_AvailableIDs.clear();
}

void CBaseResource::ShutDown()
{
    if (CRenderer::CV_r_releaseallresourcesonexit)
    {
        ResourceClassMapItor it;
        for (it = m_sResources.begin(); it != m_sResources.end(); it++)
        {
            SResourceContainer* pCN = it->second;
            SAFE_DELETE(pCN);
        }
        m_sResources.clear();
    }
}

#ifdef MEMREPLAY_WRAP_D3D11

const GUID MemReplayD3DAnnotation::s_guid = {
    0xff23dad0, 0xd47b, 0x4d80, {0xaf, 0x63, 0x7e, 0xf5, 0xea, 0x2c, 0x4a, 0x9d}
};

MemReplayD3DAnnotation::MemReplayD3DAnnotation(ID3D11DeviceChild* pRes, size_t sz)
    : m_nRefCount(0)
    , m_pRes(pRes)
{
    MEMREPLAY_SCOPE(EMemReplayAllocClass::C_D3DManaged, 0);
    MEMREPLAY_SCOPE_ALLOC(pRes, sz, 0);
}

MemReplayD3DAnnotation::~MemReplayD3DAnnotation()
{
    MEMREPLAY_SCOPE(EMemReplayAllocClass::C_D3DManaged, 0);
    MEMREPLAY_SCOPE_FREE(m_pRes);
}

void MemReplayD3DAnnotation::AddMap(UINT nSubRes, void* pData, size_t sz)
{
    MapDesc desc;
    desc.nSubResource = nSubRes;
    desc.pData = pData;
    m_maps.push_back(desc);

    MEMREPLAY_SCOPE(EMemReplayAllocClass::C_UserPointer, EMemReplayUserPointerClass::C_CryMalloc);
    MEMREPLAY_SCOPE_ALLOC(pData, sz, 0);
}

void MemReplayD3DAnnotation::RemoveMap(UINT nSubRes)
{
    for (std::vector<MapDesc>::reverse_iterator it = m_maps.rbegin(), itEnd = m_maps.rend(); it != itEnd; ++it)
    {
        if (it->nSubResource == nSubRes)
        {
            MEMREPLAY_SCOPE(EMemReplayAllocClass::C_UserPointer, EMemReplayUserPointerClass::C_CryMalloc);
            MEMREPLAY_SCOPE_FREE(it->pData);

            m_maps.erase(m_maps.begin() + std::distance(&*m_maps.begin(), &*it));
            if (m_maps.empty())
            {
                stl::free_container(m_maps);
            }
            break;
        }
    }
}

HRESULT MemReplayD3DAnnotation::QueryInterface(REFIID riid, void** ppvObject)
{
    if (riid == IID_IUnknown)
    {
        *reinterpret_cast<IUnknown**>(ppvObject) = this;
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

ULONG MemReplayD3DAnnotation::AddRef()
{
    return ++m_nRefCount;
}

ULONG MemReplayD3DAnnotation::Release()
{
    ULONG nr = --m_nRefCount;

    if (!nr)
    {
        delete this;
    }

    return nr;
}

#endif