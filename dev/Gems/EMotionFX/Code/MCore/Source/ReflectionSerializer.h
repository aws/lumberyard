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

#pragma once

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/string.h>
#include <MCore/Source/CommandLine.h>

namespace MCore
{
    class ReflectionSerializer
    {
    public:
        template <typename TClass>
        static AZ::Outcome<AZStd::string> SerializeMember(const TClass* classPtr, const char* memberName)
        {
            return SerializeMember(azrtti_typeid(classPtr), classPtr, memberName);
        }

        template <typename TClass>
        static bool DeserializeIntoMember(TClass* classPtr, const char* memberName, const AZStd::string& value)
        {
            return DeserializeIntoMember(azrtti_typeid(classPtr), classPtr, memberName, value);
        }

        template <typename TClass>
        static AZ::Outcome<AZStd::string> Serialize(const TClass* classPtr)
        {
            return Serialize(azrtti_typeid(classPtr), classPtr);
        }

        template <typename TClass>
        static bool Deserialize(TClass* classPtr, const AZStd::string& sourceBuffer)
        {
            return Deserialize(azrtti_typeid(classPtr), classPtr, sourceBuffer);
        }

        template <typename TClass>
        static AZ::Outcome<AZStd::string> SerializeIntoCommandLine(const TClass* classPtr)
        {
            return SerializeIntoCommandLine(azrtti_typeid(classPtr), classPtr);
        }

        template <typename TClass>
        static bool Deserialize(TClass* classPtr, const MCore::CommandLine& sourceCommandLine)
        {
            return Deserialize(azrtti_typeid(classPtr), classPtr, sourceCommandLine);
        }

        template <typename TClass>
        static TClass* Clone(const TClass* classPtr)
        {
            return reinterpret_cast<TClass*>(Clone(azrtti_typeid(classPtr), classPtr));
        }

        template <typename TClass, typename TValue>
        static bool SetIntoMember(AZ::SerializeContext* context, TClass* classPtr, const char* memberName, const TValue& value)
        {
            void* ptrToMember = GetPointerToMember(context, azrtti_typeid(classPtr), classPtr, memberName);
            if (ptrToMember)
            {
                *reinterpret_cast<TValue*>(ptrToMember) = value;
                return true;
            }
            return false;
        }

    private:
        static AZ::Outcome<AZStd::string> SerializeMember(const AZ::TypeId& classTypeId, const void* classPtr, const char* memberName);

        static bool DeserializeIntoMember(const AZ::TypeId& classTypeId, void* classPtr, const char* memberName, const AZStd::string& value);

        static AZ::Outcome<AZStd::string> Serialize(const AZ::TypeId& classTypeId, const void* classPtr);

        static bool Deserialize(const AZ::TypeId& classTypeId, void* classPtr, const AZStd::string& sourceBuffer);

        static AZ::Outcome<AZStd::string> SerializeIntoCommandLine(const AZ::TypeId& classTypeId, const void* classPtr);

        static bool Deserialize(const AZ::TypeId& classTypeId, void* classPtr, const MCore::CommandLine& sourceCommandLine);

        static void* Clone(const AZ::TypeId& classTypeId, const void* classPtr);

        static void* GetPointerToMember(AZ::SerializeContext* context, const AZ::TypeId& classTypeId, void* classPtr, const char* memberName);
    };
}
