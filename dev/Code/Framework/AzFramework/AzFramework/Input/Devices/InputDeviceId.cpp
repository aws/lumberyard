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

#include <AzFramework/Input/Devices/InputDeviceId.h>

#include <AzCore/RTTI/BehaviorContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceId::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<InputDeviceId>()
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Category, "Input")
                ->Constructor<const char*, AZ::u32>()
                ->Property("name", BehaviorValueProperty(&InputDeviceId::m_name))
                ->Property("index", BehaviorValueProperty(&InputDeviceId::m_index))
            ;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceId::InputDeviceId(const char* name, AZ::u32 index)
        : m_name(name)
        , m_crc32(name)
        , m_index(index)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const char* InputDeviceId::GetName() const
    {
        return m_name;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const AZ::Crc32& InputDeviceId::GetNameCrc32() const
    {
        return m_crc32;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::u32 InputDeviceId::GetIndex() const
    {
        return m_index;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceId::operator==(const InputDeviceId& other) const
    {
        return (m_crc32 == other.m_crc32) && (m_index == other.m_index);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceId::operator!=(const InputDeviceId& other) const
    {
        return !(*this == other);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceId::operator<(const InputDeviceId& other) const
    {
        if (m_index == other.m_index)
        {
            return m_crc32 < other.m_crc32;
        }
        return m_index < other.m_index;
    }
} // namespace AzFramework
