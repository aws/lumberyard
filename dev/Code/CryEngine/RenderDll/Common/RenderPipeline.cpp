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
#include "Shadow_Renderer.h"
#include "RenderView.h"
#include "CompiledRenderObject.h"
#include <CryEngineAPI.h>

ENGINE_API int SRendItem::m_RecurseLevel[RT_COMMAND_BUF_COUNT];
int SRendItem::m_StartFrust[RT_COMMAND_BUF_COUNT][MAX_REND_LIGHTS + MAX_DEFERRED_LIGHTS];
int SRendItem::m_EndFrust[RT_COMMAND_BUF_COUNT][MAX_REND_LIGHTS + MAX_DEFERRED_LIGHTS];
int SRendItem::m_ShadowsStartRI[RT_COMMAND_BUF_COUNT][MAX_SHADOWMAP_FRUSTUMS];
int SRendItem::m_ShadowsEndRI[RT_COMMAND_BUF_COUNT][MAX_SHADOWMAP_FRUSTUMS];

CRenderObjectsPools* CRenderObjectImpl::s_pPools;

//=================================================================
//////////////////////////////////////////////////////////////////////////
CRenderObjectImpl::~CRenderObjectImpl()
{
    if (m_pCompiled)
    {
        CCompiledRenderObject::FreeToPool(m_pCompiled);
        m_pCompiled = nullptr;
    }
    if (m_pNextSubObject)
    {
        FreeToPool(m_pNextSubObject);
        m_pNextSubObject = nullptr;
    }
}

CRenderObject* CRenderObjectImpl::AllocateFromPool()
{
    CRenderObjectImpl* pObject = s_pPools->m_permanentRenderObjectsPool.New();
    pObject->Init();
    return pObject;
}

void CRenderObjectImpl::FreeToPool(CRenderObject* pObj)
{
    s_pPools->m_permanentRenderObjectsPool.Delete(static_cast<CRenderObjectImpl*>(pObj));
}

