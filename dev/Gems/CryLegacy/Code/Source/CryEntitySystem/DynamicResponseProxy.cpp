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

#include "CryLegacy_precompiled.h"
#include "DynamicResponseProxy.h"
#include "Components/IComponentSerialization.h"

#include <IDynamicResponseSystem.h>
#include <ISerialize.h>


DECLARE_DEFAULT_COMPONENT_FACTORY(CComponentDynamicResponse, IComponentDynamicResponse)

//////////////////////////////////////////////////////////////////////////
CComponentDynamicResponse::CComponentDynamicResponse()
    : m_pResponseActor(NULL)
{
}

//////////////////////////////////////////////////////////////////////////
CComponentDynamicResponse::~CComponentDynamicResponse()
{
    gEnv->pDynamicResponseSystem->ReleaseResponseActor(m_pResponseActor);
    m_pResponseActor = NULL;
    delete this;
}

//////////////////////////////////////////////////////////////////////////
void CComponentDynamicResponse::Initialize(SComponentInitializer const& init)
{
    assert(init.m_pEntity);

    //i set this to script.... I guess
    init.m_pEntity->GetComponent<IComponentSerialization>()->Register<CComponentDynamicResponse>(SerializationOrder::Script, *this, &CComponentDynamicResponse::Serialize, &CComponentDynamicResponse::SerializeXML, &CComponentDynamicResponse::NeedSerialize, &CComponentDynamicResponse::GetSignature);

    m_pResponseActor = gEnv->pDynamicResponseSystem->CreateResponseActor(init.m_pEntity->GetName(), init.m_pEntity->GetId());
    assert(m_pResponseActor);
    m_pResponseActor->GetLocalVariables()->SetVariableValue("Name", init.m_pEntity->GetName());
}

//////////////////////////////////////////////////////////////////////////
void CComponentDynamicResponse::Reload(IEntity* pEntity, SEntitySpawnParams& params)
{
}

//////////////////////////////////////////////////////////////////////////
void CComponentDynamicResponse::UpdateComponent(SEntityUpdateContext& ctx)
{
}

//////////////////////////////////////////////////////////////////////////
void CComponentDynamicResponse::ProcessEvent(SEntityEvent& event)
{
    if (event.event == ENTITY_EVENT_RESET)
    {
        m_pResponseActor->GetLocalVariables()->SetVariableValue("Name", m_pResponseActor->GetName());
    }
}

//////////////////////////////////////////////////////////////////////////
bool CComponentDynamicResponse::NeedSerialize()
{
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CComponentDynamicResponse::GetSignature(TSerialize signature)
{
    signature.BeginGroup("ComponentDynamicResponse");
    signature.EndGroup();
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CComponentDynamicResponse::Serialize(TSerialize ser)
{
}

//////////////////////////////////////////////////////////////////////////
void CComponentDynamicResponse::SerializeXML(XmlNodeRef& entityNode, bool bLoading)
{
}

//////////////////////////////////////////////////////////////////////////
void CComponentDynamicResponse::QueueSignal(const char* pSignalName, DRS::IVariableCollection* pSignalContext /* = 0 */, float delayBeforeFiring /* = 0.0f */, bool autoReleaseCollection)
{
    gEnv->pDynamicResponseSystem->QueueSignal(pSignalName, m_pResponseActor, pSignalContext, delayBeforeFiring, (pSignalContext) ? autoReleaseCollection : false);
}

//////////////////////////////////////////////////////////////////////////
DRS::IVariableCollection* CComponentDynamicResponse::GetLocalVariableCollection() const
{
    assert(m_pResponseActor && "Proxy without Actor detected...");
    return m_pResponseActor->GetLocalVariables();
}

//////////////////////////////////////////////////////////////////////////
DRS::IResponseActor* CComponentDynamicResponse::GetResponseActor() const
{
    return m_pResponseActor;
}


//////////////////////////////////////////////////////////////////////////
