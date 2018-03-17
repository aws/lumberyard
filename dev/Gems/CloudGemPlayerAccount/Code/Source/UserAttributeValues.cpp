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
#include "CloudGemPlayerAccount_precompiled.h"
#include "UserAttributeValues.h"

#include <AzCore/base.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace CloudGemPlayerAccount
{
    void UserAttributeValues::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<UserAttributeValues>()
                ->Version(1);
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<UserAttributeValues>()
                ->Method("SetAttribute", &UserAttributeValues::SetAttribute)
                ->Method("HasAttribute", &UserAttributeValues::HasAttribute)
                ->Method("GetAttribute", &UserAttributeValues::GetAttribute)
                ;
        }
    }

    void UserAttributeValues::SetAttribute(const AZStd::string& name, const AZStd::string& value)
    {
        m_attributeMap[name] = value;
    }

    bool UserAttributeValues::HasAttribute(const AZStd::string& name) const
    {
        return m_attributeMap.count(name) > 0;
    }

    AZStd::string UserAttributeValues::GetAttribute(const AZStd::string& name)
    {
        auto&& iter = m_attributeMap.find(name);
        if (iter != m_attributeMap.end())
        {
            return iter->second;
        }
        return "";
    }

    const AttributeMap& UserAttributeValues::GetData() const 
    { 
        return m_attributeMap;
    }
}   // namespace CloudGemPlayerAccount