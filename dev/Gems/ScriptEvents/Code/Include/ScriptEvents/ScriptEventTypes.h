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

#include <ScriptEvents/Internal/VersionedProperty.h>

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix4x4.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/sort.h>

namespace ScriptEvents
{
    namespace Types
    {
        using SupportedTypes = AZStd::vector< AZStd::pair< AZ::Uuid, AZStd::string> >;
        using VersionedTypes = AZStd::vector<AZStd::pair< ScriptEventData::VersionedProperty, AZStd::string>>;

        static VersionedTypes GetValidAddressTypes()
        {
            using namespace ScriptEventData;

            static VersionedTypes validAddressTypes;

            if (validAddressTypes.empty())
            {
                validAddressTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<void>(), "None"), "None"));
                validAddressTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZStd::string>(), "String"), "String"));
                validAddressTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::EntityId>(), "Entity Id"), "Entity Id"));
                validAddressTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Crc32>(), "Tag"), "Tag"));
            }

            return validAddressTypes;
        }

        // Returns the list of the valid Script Event method parameter type Ids, this is used 
        static AZStd::vector<AZ::Uuid> GetSupportedParameterTypes()
        {
            static AZStd::vector<AZ::Uuid> supportedTypes;
            if (supportedTypes.empty())
            {
                supportedTypes.push_back(azrtti_typeid<bool>());
                supportedTypes.push_back(azrtti_typeid<double>());
                supportedTypes.push_back(azrtti_typeid<AZ::EntityId>());
                supportedTypes.push_back(azrtti_typeid<AZStd::string>());
                supportedTypes.push_back(azrtti_typeid<AZ::Vector2>());
                supportedTypes.push_back(azrtti_typeid<AZ::Vector3>());
                supportedTypes.push_back(azrtti_typeid<AZ::Vector4>());
                supportedTypes.push_back(azrtti_typeid<AZ::Matrix3x3>());
                supportedTypes.push_back(azrtti_typeid<AZ::Matrix4x4>());
                supportedTypes.push_back(azrtti_typeid<AZ::Transform>());
                supportedTypes.push_back(azrtti_typeid<AZ::Quaternion>());
                supportedTypes.push_back(azrtti_typeid<AZ::Crc32>());
            }

            return supportedTypes;
        }

        // Returns the list of the valid Script Event method parameters, this is used to populate the ReflectedPropertyEditor's combobox
        static VersionedTypes GetValidParameterTypes()
        {
            using namespace ScriptEventData;

            static VersionedTypes validParamTypes;

            if (validParamTypes.empty())
            {
                validParamTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<bool>(), "Boolean"), "Boolean"));
                validParamTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<double>(), "Number"), "Number"));
                validParamTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZStd::string>(), "String"), "String"));
                validParamTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::EntityId>(), "Entity Id"), "Entity Id"));
                validParamTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Vector2>(), "Vector2"), "Vector2")); 
                validParamTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Vector3>(), "Vector3"), "Vector3"));
                validParamTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Vector4>(), "Vector4"), "Vector4"));
                validParamTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Matrix3x3>(), "Matrix3x3"), "Matrix3x3"));
                validParamTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Matrix4x4>(), "Matrix4x4"), "Matrix4x4"));
                validParamTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Transform>(), "Transform"), "Transform"));
                validParamTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Quaternion>(), "Quaternion"), "Quaternion"));
                validParamTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Crc32>(), "Tag"), "Tag"));
            }

            return validParamTypes;
        }

        // Determines whether the specified type is a valid parameter on a Script Event method argument list
        static bool IsValidParameterType(const AZ::Uuid& typeId)
        {
            for (const auto& supportedParam : GetSupportedParameterTypes())
            {
                if (supportedParam == typeId)
                {
                    return true;
                }
            }

            return false;
        }

        // Supported return types for Script Event methods
        static AZStd::vector<AZ::Uuid> GetSupportedReturnTypes()
        {
            static AZStd::vector<AZ::Uuid> supportedTypes;
            if (supportedTypes.empty())
            {
                supportedTypes.push_back(azrtti_typeid<void>());
                supportedTypes.push_back(azrtti_typeid<bool>());
                supportedTypes.push_back(azrtti_typeid<double>());
                supportedTypes.push_back(azrtti_typeid<AZ::EntityId>());
                supportedTypes.push_back(azrtti_typeid<AZStd::string>());
                supportedTypes.push_back(azrtti_typeid<AZ::Vector2>());
                supportedTypes.push_back(azrtti_typeid<AZ::Vector3>());
                supportedTypes.push_back(azrtti_typeid<AZ::Vector4>());
                supportedTypes.push_back(azrtti_typeid<AZ::Matrix3x3>());
                supportedTypes.push_back(azrtti_typeid<AZ::Matrix4x4>());
                supportedTypes.push_back(azrtti_typeid<AZ::Transform>());
                supportedTypes.push_back(azrtti_typeid<AZ::Quaternion>());
                supportedTypes.push_back(azrtti_typeid<AZ::Crc32>());
            }

            return supportedTypes;
        }

        static VersionedTypes GetValidReturnTypes()
        {
            using namespace ScriptEventData;

            static VersionedTypes validReturnTypes;

            if (validReturnTypes.empty())
            {
                validReturnTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<void>(), "None"), "None"));
                validReturnTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<bool>(), "Boolean"), "Boolean"));
                validReturnTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<double>(), "Number"), "Number"));
                validReturnTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZStd::string>(), "String"), "String"));
                validReturnTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::EntityId>(), "Entity Id"), "Entity Id"));
                validReturnTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Vector2>(), "Vector2"), "Vector2"));
                validReturnTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Vector3>(), "Vector3"), "Vector3"));
                validReturnTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Vector4>(), "Vector4"), "Vector4"));
                validReturnTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Matrix3x3>(), "Matrix3x3"), "Matrix3x3"));
                validReturnTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Matrix4x4>(), "Matrix4x4"), "Matrix4x4"));
                validReturnTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Transform>(), "Transform"), "Transform"));
                validReturnTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Quaternion>(), "Quaternion"), "Quaternion"));
                validReturnTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Crc32>(), "Tag"), "Tag"));
            }

            return validReturnTypes;
        }

        static bool IsValidReturnType(const AZ::Uuid& typeId)
        {
            for (const auto& supportedType: GetSupportedReturnTypes())
            {
                if (supportedType == typeId)
                {
                    return true;
                }
            }

            return false;
        }


        static AZ::BehaviorMethod* FindBehaviorOperatorMethod(const AZ::BehaviorClass* behaviorClass, AZ::Script::Attributes::OperatorType operatorType)
        {
            AZ_Assert(behaviorClass, "Invalid AZ::BehaviorClass submitted to FindBehaviorOperatorMethod");
            for (auto&& equalMethodCandidatePair : behaviorClass->m_methods)
            {
                const AZ::AttributeArray& attributes = equalMethodCandidatePair.second->m_attributes;
                for (auto&& attributePair : attributes)
                {
                    if (attributePair.second->RTTI_IsTypeOf(AZ::AzTypeInfo<AZ::AttributeData<AZ::Script::Attributes::OperatorType>>::Uuid()))
                    {
                        auto&& attributeData = AZ::RttiCast<AZ::AttributeData<AZ::Script::Attributes::OperatorType>*>(attributePair.second);
                        if (attributeData->Get(nullptr) == operatorType)
                        {
                            return equalMethodCandidatePair.second;
                        }
                    }
                }
            }
            return nullptr;
        }

        static bool IsAddressableType(const AZ::Uuid& uuid)
        {
            return !uuid.IsNull() && !AZ::BehaviorContext::IsVoidType(uuid);
        }

        static bool ValidateAddressType(const AZ::Uuid& addressTypeId)
        {
            bool isValid = true;
            if (IsAddressableType(addressTypeId))
            {
                AZ::BehaviorContext* behaviorContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationBus::Events::GetBehaviorContext);

                if (auto&& behaviorClass = behaviorContext->m_typeToClassMap[addressTypeId])
                {
                    if (!behaviorClass->m_valueHasher)
                    {
                        AZ_Warning("Script Events", false, AZStd::string::format("Class %s with id %s must have an AZStd::hash<T> specialization to be a bus id", behaviorClass->m_name.c_str(), addressTypeId.ToString<AZStd::string>().c_str()).c_str());
                        return false;
                    }

                    if (!FindBehaviorOperatorMethod(behaviorClass, AZ::Script::Attributes::OperatorType::Equal) && !behaviorClass->m_equalityComparer)
                    {
                        AZ_Warning("Script Events", false, AZStd::string::format("Class %s with id %s does not have an operator equal defined to be a bus id", behaviorClass->m_name.c_str(), addressTypeId.ToString<AZStd::string>().c_str()).c_str());
                        return false;
                    }
                }
                else
                {
                    AZ::SerializeContext* serializeContext = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                    if (auto&& classData = serializeContext->FindClassData(addressTypeId))
                    {
                        AZ_Warning("Script Events", false, AZStd::string::format("Type %s with id %s not found in behavior context", classData->m_name, addressTypeId.ToString<AZStd::string>().c_str()).c_str());
                        return false;
                    }
                    else
                    {
                        AZ_Warning("Script Events", false, AZStd::string::format("Type with id %s not found in behavior context", addressTypeId.ToString<AZStd::string>().c_str()).c_str());
                        return false;
                    }
                }
            }
            return true;
        }
    }
}
