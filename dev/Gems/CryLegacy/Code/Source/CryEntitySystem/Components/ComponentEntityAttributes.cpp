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
#include "CryLegacy_precompiled.h"

#include "ComponentEntityAttributes.h"

#include "Serialization/IArchive.h"
#include "Serialization/IArchiveHost.h"
#include <Components/IComponentSerialization.h>

DECLARE_DEFAULT_COMPONENT_FACTORY(CComponentEntityAttributes, IComponentEntityAttributes)

namespace
{
    struct SEntityAttributesSerializer
    {
        SEntityAttributesSerializer(TEntityAttributeArray& _attributes)
            : attributes(_attributes)
        {}

        void Serialize(Serialization::IArchive& archive)
        {
            for (size_t iAttribute = 0, attributeCount = attributes.size(); iAttribute < attributeCount; ++iAttribute)
            {
                IEntityAttribute*   pAttribute = attributes[iAttribute].get();
                if (pAttribute != NULL)
                {
                    archive(*pAttribute, pAttribute->GetName(), pAttribute->GetLabel());
                }
            }
        }

        TEntityAttributeArray&  attributes;
    };
}

//////////////////////////////////////////////////////////////////////////
void CComponentEntityAttributes::ProcessEvent(SEntityEvent& event) {}

//////////////////////////////////////////////////////////////////////////
void CComponentEntityAttributes::Initialize(SComponentInitializer const& inititializer) {}

//////////////////////////////////////////////////////////////////////////
void CComponentEntityAttributes::Done() {}

//////////////////////////////////////////////////////////////////////////
void CComponentEntityAttributes::UpdateComponent(SEntityUpdateContext& context) {}

//////////////////////////////////////////////////////////////////////////
bool CComponentEntityAttributes::InitComponent(IEntity* pEntity, SEntitySpawnParams& params)
{
    if (m_attributes.empty() == true)
    {
        EntityAttributeUtils::CloneAttributes(params.pClass->GetEntityAttributes(), m_attributes);
    }

    pEntity->GetComponent<IComponentSerialization>()->Register<CComponentEntityAttributes>(SerializationOrder::Attributes, *this, &CComponentEntityAttributes::Serialize, &CComponentEntityAttributes::SerializeXML, &CComponentEntityAttributes::NeedSerialize, &CComponentEntityAttributes::GetSignature);

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CComponentEntityAttributes::Reload(IEntity* pEntity, SEntitySpawnParams& params) {}

//////////////////////////////////////////////////////////////////////////
void CComponentEntityAttributes::SerializeXML(XmlNodeRef& entityNodeXML, bool loading)
{
    if (loading == true)
    {
        if (XmlNodeRef attributesNodeXML = entityNodeXML->findChild("Attributes"))
        {
            SEntityAttributesSerializer serializer(m_attributes);
            Serialization::LoadXmlNode(serializer, attributesNodeXML);
        }
    }
    else
    {
        if (!m_attributes.empty())
        {
            SEntityAttributesSerializer serializer(m_attributes);
            if (XmlNodeRef attributesNodeXML = Serialization::SaveXmlNode(serializer, "Attributes"))
            {
                entityNodeXML->addChild(attributesNodeXML);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentEntityAttributes::Serialize(TSerialize serialize) {}

//////////////////////////////////////////////////////////////////////////
bool CComponentEntityAttributes::NeedSerialize()
{
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CComponentEntityAttributes::GetSignature(TSerialize signature)
{
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CComponentEntityAttributes::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(this, sizeof(*this));
}

//////////////////////////////////////////////////////////////////////////
void CComponentEntityAttributes::SetAttributes(const TEntityAttributeArray& attributes)
{
    const size_t    attributeCount = attributes.size();
    m_attributes.resize(attributeCount);
    for (size_t iAttribute = 0; iAttribute < attributeCount; ++iAttribute)
    {
        IEntityAttribute*   pSrc = attributes[iAttribute].get();
        IEntityAttribute*   pDst = m_attributes[iAttribute].get();
        if ((pDst != NULL) && (strcmp(pSrc->GetName(), pDst->GetName()) == 0))
        {
            Serialization::CloneBinary(*pDst, *pSrc);
        }
        else if (pSrc != NULL)
        {
            m_attributes[iAttribute] = pSrc->Clone();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
TEntityAttributeArray& CComponentEntityAttributes::GetAttributes()
{
    return m_attributes;
}

//////////////////////////////////////////////////////////////////////////
const TEntityAttributeArray& CComponentEntityAttributes::GetAttributes() const
{
    return m_attributes;
}