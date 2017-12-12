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

#ifndef DYNAMICRESPONSESYSTEM_PROXY_H_
#define DYNAMICRESPONSESYSTEM_PROXY_H_

#include <IDynamicResponseSystem.h>

//////////////////////////////////////////////////////////////////////////
class CComponentDynamicResponse
    : public IComponentDynamicResponse
{
public:
    CComponentDynamicResponse();
    virtual ~CComponentDynamicResponse();

    //////////////////////////////////////////////////////////////////////////
    // IComponent
    void Initialize(const SComponentInitializer& init) override;
    void ProcessEvent(SEntityEvent& event) override;
    void UpdateComponent(SEntityUpdateContext& ctx) override;
    void Reload(IEntity* pEntity, SEntitySpawnParams& params) override;
    void GetMemoryUsage(ICrySizer* pSizer) const override
    {
        pSizer->AddObject(this, sizeof(*this));
    }
    //////////////////////////////////////////////////////////////////////////
    // IComponent Serialization Related
    bool NeedSerialize();
    bool GetSignature(TSerialize signature);
    void Serialize(TSerialize ser);
    void SerializeXML(XmlNodeRef& entityNode, bool bLoading);
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // IComponentDynamicResponse
    void QueueSignal(const char* pSignalName, DRS::IVariableCollection* pSignalContext /* = 0 */, float delayBeforeFiring /* = 0.0f */, bool autoReleaseCollection = true) override;
    DRS::IVariableCollection* GetLocalVariableCollection() const override;
    DRS::IResponseActor* GetResponseActor() const override;
    //////////////////////////////////////////////////////////////////////////

private:
    DRS::IResponseActor* m_pResponseActor;
};

#endif // DYNAMICRESPONSESYSTEM_PROXY_H_
