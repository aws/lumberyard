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

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
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
} // namespace AzFramework
