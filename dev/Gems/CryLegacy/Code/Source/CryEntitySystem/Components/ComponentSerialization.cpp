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
#include "ISerialize.h"
#include "ComponentSerialization.h"

DECLARE_DEFAULT_COMPONENT_FACTORY(ComponentSerialization, IComponentSerialization)

void ComponentSerialization::Unregister(const ComponentType& componentType)
{
    m_serializationFunctionMap.erase(componentType);
    for (auto it = m_order.begin(); it != m_order.end(); ++it)
    {
        if ((*it).second == componentType)
        {
            m_order.erase(it);
            return;
        }
    }
}

void ComponentSerialization::Serialize(TSerialize s)
{
    for (auto& it : m_order)
    {
        SerializationRegistration::iterator componentIt = m_serializationFunctionMap.find(it.second);
        if (componentIt != m_serializationFunctionMap.end())
        {
            auto bind = componentIt->second;
            auto& function = std::get<SerializationFunctionIndex::Serialize>(bind);
            if (function)
            {
                (function)(s);
            }
        }
    }
}

void ComponentSerialization::SerializeOnly(TSerialize s, const std::vector<ComponentType>& componentsToSerialize)
{
    for (auto& it : m_order)
    {
        auto component = std::find(componentsToSerialize.begin(), componentsToSerialize.end(), it.second);
        if (component != componentsToSerialize.end())
        {
            SerializationRegistration::iterator componentIt = m_serializationFunctionMap.find(*component);
            if (componentIt != m_serializationFunctionMap.end())
            {
                auto bind = componentIt->second;
                auto& function = std::get<SerializationFunctionIndex::Serialize>(bind);
                if (function)
                {
                    (function)(s);
                }
            }
        }
    }
}

void ComponentSerialization::SerializeXML(XmlNodeRef& node, bool loading, const std::vector<ComponentType>& componentsToSkip /*= {}*/)
{
    for (auto& it : m_order)
    {
        if (!componentsToSkip.empty() && std::find(componentsToSkip.begin(), componentsToSkip.end(), it.second) != componentsToSkip.end())
        {
            continue;
        }

        SerializationRegistration::iterator componentIt = m_serializationFunctionMap.find(it.second);
        if (componentIt != m_serializationFunctionMap.end())
        {
            auto bind = componentIt->second;
            auto& function = std::get<SerializationFunctionIndex::SerializeXML>(bind);
            if (function)
            {
                (function)(node, loading);
            }
        }
    }
}

void ComponentSerialization::SerializeXMLOnly(XmlNodeRef& node, bool loading, const std::vector<ComponentType>& componentsToSerialize /*= {}*/)
{
    for (auto& it : m_order)
    {
        auto component = std::find(componentsToSerialize.begin(), componentsToSerialize.end(), it.second);
        if (component != componentsToSerialize.end())
        {
            SerializationRegistration::const_iterator componentIt = m_serializationFunctionMap.find(*component);

            auto bind = componentIt->second;
            auto& function = std::get<SerializationFunctionIndex::SerializeXML>(bind);
            if (function)
            {
                (function)(node, loading);
            }
        }
    }
}

bool ComponentSerialization::NeedSerialize() const
{
    for (auto& it : m_order)
    {
        SerializationRegistration::const_iterator componentIt = m_serializationFunctionMap.find(it.second);
        if (componentIt != m_serializationFunctionMap.end())
        {
            const auto bind = componentIt->second;
            const auto& function = std::get<SerializationFunctionIndex::NeedsSerialize>(bind);
            if (function && (function)())
            {
                return true;
            }
        }
    }
    return false;
}

bool ComponentSerialization::GetSignature(TSerialize signature)
{
    bool gotSignature = true;

    for (auto& it : m_order)
    {
        SerializationRegistration::const_iterator componentIt = m_serializationFunctionMap.find(it.second);
        if (componentIt != m_serializationFunctionMap.end())
        {
            const auto bind = componentIt->second;
            const auto& function = std::get<SerializationFunctionIndex::GetSignature>(bind);
            if (function)
            {
                gotSignature &= (function)(signature);
            }
        }
    }
    return gotSignature;
}

void ComponentSerialization::RegisterInternal(uint32 serializationOrder, const ComponentType& componentType, SerializeFunction serialize, SerializeXMLFunction serializeXML, NeedSerializeFunction needSerialize, GetSignatureFunction getSignature)
{
    if (m_serializationFunctionMap.find(componentType) == m_serializationFunctionMap.end())
    {
        m_serializationFunctionMap[componentType] = SerializeFunctions(serialize, serializeXML, needSerialize, getSignature);
        m_order.insert(std::pair<uint32, ComponentType>(serializationOrder, componentType));
    }
}
