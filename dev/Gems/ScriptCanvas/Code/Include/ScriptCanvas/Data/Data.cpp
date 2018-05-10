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

#include "precompiled.h"
#include "Data.h"
#include <ScriptCanvas/Data/DataTrait.h>

#include <AzCore/Component/EntityBus.h>
#include <AzCore/Math/VectorFloat.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace DataCpp
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Data;

    AZ_INLINE AZStd::pair<bool, Type> FromAZTypeHelper(const AZ::Uuid& type)
    {
        if (type.IsNull())
        {
            return { true, Type::Invalid() };
        }
        else if (IsAABB(type))
        {
            return { true, Type::AABB() };
        }
        else if (IsBoolean(type))
        {
            return { true, Type::Boolean() };
        }
        else if (IsColor(type))
        {
            return { true, Type::Color() };
        }
        else if (IsCRC(type))
        {
            return { true, Type::CRC() };
        }
        else if (IsEntityID(type))
        {
            return { true, Type::EntityID() };
        }
        else if (IsMatrix3x3(type))
        {
            return { true, Type::Matrix3x3() };
        }
        else if (IsMatrix4x4(type))
        {
            return { true, Type::Matrix4x4() };
        }
        else if (IsNumber(type))
        {
            return { true, Type::Number() };
        }
        else if (IsOBB(type))
        {
            return { true, Type::OBB() };
        }
        else if (IsPlane(type))
        {
            return { true, Type::Plane() };
        }
        else if (IsQuaternion(type))
        {
            return { true, Type::Quaternion() };
        }
        else if (IsString(type))
        {
            return { true, Type::String() };
        }
        else if (IsTransform(type))
        {
            return { true, Type::Transform() };
        }
        else if (IsVector2(type))
        {
            return { true, Type::Vector2() };
        }
        else if (IsVector3(type))
        {
            return { true, Type::Vector3() };
        }
        else if (IsVector4(type))
        {
            return { true, Type::Vector4() };
        }
        else
        {
            return { false, Type::Invalid() };
        }
    }

    AZ_INLINE AZStd::pair<bool, Type> FromBehaviorContextTypeHelper(const AZ::Uuid& type)
    {
        if (type.IsNull())
        {
            return { true, Type::Invalid() };
        }
        else if (IsBoolean(type))
        {
            return { true, Type::Boolean() };
        }
        else if (IsEntityID(type))
        {
            return { true, Type::EntityID() };
        }
        else if (IsNumber(type))
        {
            return { true, Type::Number() };
        }
        else if (IsString(type))
        {
            return { true, Type::String() };
        }
        else
        {
            return { false, Type::Invalid() };
        }
    }

    AZ_INLINE const char* GetBehaviorClassName(const AZ::Uuid& typeID)
    {
        const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(typeID);
        return behaviorClass ? behaviorClass->m_name.c_str() : "Invalid";
    }

    AZ_INLINE bool IsSupportedBehaviorContextObject(const AZ::Uuid& typeID)
    {
        return AZ::BehaviorContextHelper::GetClass(typeID) != nullptr;
    }
}

namespace ScriptCanvas
{
    namespace Data
    {
        Type FromAZType(const AZ::Uuid& type)
        {
            AZStd::pair<bool, Type> help = DataCpp::FromAZTypeHelper(type);
            return help.first ? help.second : Type::BehaviorContextObject(type);
        }
        
        Type FromAZTypeChecked(const AZ::Uuid& type)
        {
            AZStd::pair<bool, Type> help = DataCpp::FromAZTypeHelper(type);
            return help.first 
                ? help.second 
                : DataCpp::IsSupportedBehaviorContextObject(type)
                    ? Type::BehaviorContextObject(type)
                    : Type::Invalid();
        }
        
        const char* GetName(const Type& type)
        {
            switch (type.GetType())
            {
            case eType::AABB:
                return eTraits<eType::AABB>::GetName();

            case eType::BehaviorContextObject:
                return DataCpp::GetBehaviorClassName(type.GetAZType());

            case eType::Boolean:
                return eTraits<eType::Boolean>::GetName();

            case eType::Color:
                return eTraits<eType::Color>::GetName();

            case eType::CRC:
                return eTraits<eType::CRC>::GetName();

            case eType::EntityID:
                return eTraits<eType::EntityID>::GetName();

            case eType::Invalid:
                return "Invalid";

            case eType::Matrix3x3:
                return eTraits<eType::Matrix3x3>::GetName();

            case eType::Matrix4x4:
                return eTraits<eType::Matrix4x4>::GetName();

            case eType::Number:
                return eTraits<eType::Number>::GetName();

            case eType::OBB:
                return eTraits<eType::OBB>::GetName();

            case eType::Plane:
                return eTraits<eType::Plane>::GetName();

            case eType::Quaternion:
                return eTraits<eType::Quaternion>::GetName();

            case eType::String:
                return eTraits<eType::String>::GetName();

            case eType::Transform:
                return eTraits<eType::Transform>::GetName();

            case eType::Vector2:
                return eTraits<eType::Vector2>::GetName();

            case eType::Vector3:
                return eTraits<eType::Vector3>::GetName();

            case eType::Vector4:
                return eTraits<eType::Vector4>::GetName();

            default:
                AZ_Assert(false, "Invalid type!");
                return "Invalid";
            }
        }

        const char* GetBehaviorContextName(const AZ::Uuid& azType)
        {
            const Type& type = ScriptCanvas::Data::FromAZType(azType);

            switch (type.GetType())
            {
            case eType::Boolean:
                return eTraits<eType::Boolean>::GetName();

            case eType::EntityID:
                return eTraits<eType::EntityID>::GetName();

            case eType::Invalid:
                return "Invalid";

            case eType::Number:
                return eTraits<eType::Number>::GetName();

            case eType::String:
                return eTraits<eType::String>::GetName();

            case eType::AABB:
            case eType::BehaviorContextObject:
            case eType::Color:
            case eType::CRC:
            case eType::Matrix3x3:
            case eType::Matrix4x4:
            case eType::OBB:
            case eType::Plane:
            case eType::Quaternion:
            case eType::Transform:
            case eType::Vector3:
            case eType::Vector2:
            case eType::Vector4:
            default:
                return DataCpp::GetBehaviorClassName(azType);
            }
        }

        void Type::Reflect(AZ::ReflectContext* reflection)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<Type>()
                    ->Version(2)
                    ->Field("m_type", &Type::m_type)
                    ->Field("m_azType", &Type::m_azType)
                    ;
            }
        }

        bool Type::operator==(const Type& other) const
        {
            return IS_EXACTLY_A(other);
        }

        bool Type::operator!=(const Type& other) const
        {
            return !((*this) == other);
        }

    }
}