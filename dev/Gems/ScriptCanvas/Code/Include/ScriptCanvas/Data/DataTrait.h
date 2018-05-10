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

#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvas/Data/BehaviorContextObjectPtr.h>

namespace ScriptCanvas
{
    namespace Data
    {
        template<typename t_Type>
        struct Traits
        {
            using Type = AZStd::decay_t<t_Type>;
            static const bool s_isAutoBoxed = false;
            static const bool s_isNative = false;
            static const eType s_type = eType::Invalid;

            static AZ::Uuid GetAZType(const Data::Type& = {}) { return azrtti_typeid<t_Type>(); }
            static Data::Type GetSCType(const AZ::TypeId& = AZ::TypeId::CreateNull()) { return Data::FromAZType(GetAZType()); }
            static const char* GetName(const Data::Type& = {}) { return Data::GetName(Data::FromAZType(GetAZType())); }
            // The static_assert needs to rely on the template parameter in order to avoid the clang frontend from asserting when parsing the template declaration
            static Type GetDefault(const Data::Type& = {}) { AZ_STATIC_ASSERT((!AZStd::is_same<t_Type, t_Type>::value), "implement in the typed function"); return {}; }
            static bool IsDefault(const AZStd::any&, const Data::Type& = {}) { AZ_STATIC_ASSERT((!AZStd::is_same<t_Type, t_Type>::value), "implement in the typed function"); return {}; }
        };

        // a compile time map of eType back to underlying AZ type and traits
        template<eType>
        struct eTraits
        {
        };

        struct TypeErasedDataTraits
        {
            AZ_CLASS_ALLOCATOR(TypeErasedDataTraits, AZ::SystemAllocator, 0);

            TypeErasedDataTraits() = default;

            template<typename t_Traits>
            explicit TypeErasedDataTraits(t_Traits)
            {
                m_isAutoBoxed = t_Traits::s_isAutoBoxed;
                m_isNative = t_Traits::s_isNative;
                m_type = t_Traits::s_type;
                m_getAZTypeCB = &t_Traits::GetAZType;
                m_getSCTypeCB = &t_Traits::GetSCType;
                m_getNameCB = &t_Traits::GetName;
                m_getDefaultCB = [](const Data::Type& scType) -> AZStd::any
                {
                    return AZStd::make_any<typename t_Traits::Type>(t_Traits::GetDefault(scType));
                };
                m_isDefaultCB = [](const AZStd::any& value, const Data::Type& scType) -> bool
                {
                    return value.is<typename t_Traits::Type>() ? t_Traits::IsDefault(AZStd::any_cast<typename t_Traits::Type>(value), scType) : false;
                };
            }

            AZ::Uuid GetAZType(const Data::Type& scType = {}) const { return m_getAZTypeCB ? m_getAZTypeCB(scType) : AZ::Uuid::CreateNull(); }
            Data::Type GetSCType(const AZ::TypeId& typeId = AZ::TypeId::CreateNull()) const { return m_getSCTypeCB ? m_getSCTypeCB(typeId) : Data::Type::Invalid(); }
            const char* GetName(const Data::Type& scType = {}) const { return m_getNameCB ? m_getNameCB(scType) : ""; }
            AZStd::any GetDefault(const Data::Type& scType= {}) const { return m_getDefaultCB ? m_getDefaultCB(scType) : AZStd::any{}; }
            bool IsDefault(const AZStd::any& value, const Data::Type& scType = {}) const { return m_isDefaultCB ? m_isDefaultCB(value, scType) : false; }

            bool m_isAutoBoxed = false;
            bool m_isNative = false;
            eType m_type = eType::Invalid;

            using GetAZTypeCB = AZ::Uuid(*)(const Data::Type&);
            using GetSCTypeCB = Data::Type(*)(const AZ::TypeId&);
            using GetNameCB = const char*(*)(const Data::Type&);
            using GetDefaultCB = AZStd::any(*)(const Data::Type&);
            using IsDefaultCB = bool(*)(const AZStd::any&, const Data::Type&);
            GetAZTypeCB m_getAZTypeCB{};
            GetSCTypeCB m_getSCTypeCB{};
            GetNameCB m_getNameCB{};
            GetDefaultCB m_getDefaultCB{};
            IsDefaultCB m_isDefaultCB{};
        };

        template<eType scTypeValue>
        static TypeErasedDataTraits MakeTypeErasedDataTraits()
        {
            return TypeErasedDataTraits(eTraits<scTypeValue>{});
        }

        template<typename t_Type>
        static TypeErasedDataTraits MakeTypeErasedDataTraits()
        {
            return TypeErasedDataTraits(Traits<t_Type>{});
        }

        template<>
        struct Traits<AABBType>
        {
            using Type = AABBType;
            static const bool s_isAutoBoxed = true;
            static const bool s_isNative = true;
            static const eType s_type = eType::AABB;

            static AZ::Uuid GetAZType(const Data::Type& = {}) { return azrtti_typeid<AABBType>(); }
            static Data::Type GetSCType(const AZ::TypeId& = AZ::TypeId::CreateNull()) { return Data::Type::AABB(); }
            static const char* GetName(const Data::Type& = {}) { return "AABB"; }
            static Type GetDefault(const Data::Type& = {}) { return Data::AABBType::CreateFromMinMax(Data::Vector3Type(-.5f, -.5f, -.5f), Data::Vector3Type(.5f, .5f, .5f)); }
            static bool IsDefault(const Type& value, const Data::Type& = {}) { return value != GetDefault(); }
        };

        template<>
        struct Traits<BooleanType>
        {
            using Type = BooleanType;
            static const bool s_isAutoBoxed = false;
            static const bool s_isNative = true;
            static const eType s_type = eType::Boolean;

            static AZ::Uuid GetAZType(const Data::Type& = {}) { return azrtti_typeid<BooleanType>(); }
            static Data::Type GetSCType(const AZ::TypeId& = AZ::TypeId::CreateNull()) { return Data::Type::Boolean(); }
            static const char* GetName(const Data::Type& = {}) { return "Boolean"; }
            static Type GetDefault(const Data::Type& = {}) { return false; }
            static bool IsDefault(const Type& value, const Data::Type& = {}) { return value != GetDefault(); }
        };

        template<>
        struct Traits<ColorType>
        {
            using Type = ColorType;
            static const bool s_isAutoBoxed = true;
            static const bool s_isNative = true;
            static const eType s_type = eType::Color;

            static AZ::Uuid GetAZType(const Data::Type& = {}) { return azrtti_typeid<ColorType>(); }
            static Data::Type GetSCType(const AZ::TypeId& = AZ::TypeId::CreateNull()) { return Data::Type::Color(); }
            static const char* GetName(const Data::Type& = {}) { return "Color"; }
            static Type GetDefault(const Data::Type& = {}) { return ColorType::CreateZero(); }
            static bool IsDefault(const Type& value, const Data::Type& = {}) { return value != GetDefault(); }
        };

        template<>
        struct Traits<CRCType>
        {
            using Type = CRCType;
            static const bool s_isAutoBoxed = true;
            static const bool s_isNative = true;
            static const eType s_type = eType::CRC;

            static AZ::Uuid GetAZType(const Data::Type& = {}) { return azrtti_typeid<CRCType>(); }
            static Data::Type GetSCType(const AZ::TypeId& = AZ::TypeId::CreateNull()) { return Data::Type::CRC(); }
            static const char* GetName(const Data::Type& = {}) { return "CRC"; }
            static Type GetDefault(const Data::Type& = {}) { return CRCType(); }
            static bool IsDefault(const Type& value, const Data::Type& = {}) { return value != GetDefault(); }
        };

        template<>
        struct Traits<EntityIDType>
        {
            using Type = EntityIDType;
            static const bool s_isAutoBoxed = false;
            static const bool s_isNative = true;
            static const eType s_type = eType::EntityID;

            static AZ::Uuid GetAZType(const Data::Type& = {}) { return azrtti_typeid<EntityIDType>(); }
            static Data::Type GetSCType(const AZ::TypeId& = AZ::TypeId::CreateNull()) { return Data::Type::EntityID(); }
            static const char* GetName(const Data::Type& = {}) { return "EntityID"; }
            static Type GetDefault(const Data::Type& = {}) { return SelfReferenceId; }
            static bool IsDefault(const Type& value, const Data::Type& = {}) { return value != GetDefault(); }
        };

        template<>
        struct Traits<Matrix3x3Type>
        {
            using Type = Matrix3x3Type;
            static const bool s_isAutoBoxed = true;
            static const bool s_isNative = true;
            static const eType s_type = eType::Matrix3x3;

            static AZ::Uuid GetAZType(const Data::Type& = {}) { return azrtti_typeid<Matrix3x3Type>(); }
            static Data::Type GetSCType(const AZ::TypeId& = AZ::TypeId::CreateNull()) { return Data::Type::Matrix3x3(); }
            static const char* GetName(const Data::Type& = {}) { return "Matrix3x3"; }
            static Type GetDefault(const Data::Type& = {}) { return Matrix3x3Type::CreateIdentity(); }
            static bool IsDefault(const Type& value, const Data::Type& = {}) { return value != GetDefault(); }
        };

        template<>
        struct Traits<Matrix4x4Type>
        {
            using Type = Matrix4x4Type;
            static const bool s_isAutoBoxed = true;
            static const bool s_isNative = true;
            static const eType s_type = eType::Matrix4x4;

            static AZ::Uuid GetAZType(const Data::Type& = {}) { return azrtti_typeid<Matrix4x4Type>(); }
            static Data::Type GetSCType(const AZ::TypeId& = AZ::TypeId::CreateNull()) { return Data::Type::Matrix4x4(); }
            static const char* GetName(const Data::Type& = {}) { return "Matrix4x4"; }
            static Type GetDefault(const Data::Type& = {}) { return Matrix4x4Type::CreateIdentity(); }
            static bool IsDefault(const Type& value, const Data::Type& = {}) { return value != GetDefault(); }
        };

        template<>
        struct Traits<NumberType>
        {
            using Type = NumberType;
            static const bool s_isAutoBoxed = false;
            static const bool s_isNative = true;
            static const eType s_type = eType::Number;

            static AZ::Uuid GetAZType(const Data::Type& = {}) { return azrtti_typeid<NumberType>(); }
            static Data::Type GetSCType(const AZ::TypeId& = AZ::TypeId::CreateNull()) { return Data::Type::Number(); }
            static const char* GetName(const Data::Type& = {}) { return "Number"; }
            static Type GetDefault(const Data::Type& = {}) { return 0.0; }
            static bool IsDefault(const Type& value, const Data::Type& = {}) { return value != GetDefault(); }
        };

        template<>
        struct Traits<OBBType>
        {
            using Type = OBBType;
            static const bool s_isAutoBoxed = true;
            static const bool s_isNative = true;
            static const eType s_type = eType::OBB;

            static AZ::Uuid GetAZType(const Data::Type& = {}) { return azrtti_typeid<OBBType>(); }
            static Data::Type GetSCType(const AZ::TypeId& = AZ::TypeId::CreateNull()) { return Data::Type::OBB(); }
            static const char* GetName(const Data::Type& = {}) { return "OBB"; }
            static Type GetDefault(const Data::Type& = {}) {
                return OBBType::CreateFromPositionAndAxes
                (Vector3Type::CreateZero()
                    , Vector3Type(1.f, 0.f, 0.f), 0.5f
                    , Vector3Type(0.f, 1.f, 0.f), 0.5f
                    , Vector3Type(0.f, 0.f, 1.f), 0.5f);
            }

            static bool IsDefault(const Type& value, const Data::Type& = {}) { return value != GetDefault(); }
        };

        template<>
        struct Traits<PlaneType>
        {
            using Type = PlaneType;
            static const bool s_isAutoBoxed = true;
            static const bool s_isNative = true;
            static const eType s_type = eType::Plane;

            static AZ::Uuid GetAZType(const Data::Type& = {}) { return azrtti_typeid<PlaneType>(); }
            static Data::Type GetSCType(const AZ::TypeId& = AZ::TypeId::CreateNull()) { return Data::Type::Plane(); }
            static const char* GetName(const Data::Type& = {}) { return "Plane"; }
            static Type GetDefault(const Data::Type& = {}) { return PlaneType::CreateFromNormalAndPoint(Vector3Type(0.f, 0.f, 1.f), Vector3Type::CreateZero()); }
            static bool IsDefault(const Type& value, const Data::Type& = {}) { return value != GetDefault(); }
        };

        template<>
        struct Traits<QuaternionType>
        {
            using Type = QuaternionType;
            static const bool s_isAutoBoxed = true;
            static const bool s_isNative = true;
            static const eType s_type = eType::Quaternion;

            static AZ::Uuid GetAZType(const Data::Type& = {}) { return azrtti_typeid<QuaternionType>(); }
            static Data::Type GetSCType(const AZ::TypeId& = AZ::TypeId::CreateNull()) { return Data::Type::Quaternion(); }
            static const char* GetName(const Data::Type& = {}) { return "Quaternion"; }
            static Type GetDefault(const Data::Type& = {}) { return QuaternionType::CreateIdentity(); }
            static bool IsDefault(const Type& value, const Data::Type& = {}) { return value != GetDefault(); }
        };

        template<>
        struct Traits<StringType>
        {
            using Type = StringType;
            static const bool s_isAutoBoxed = false;
            static const bool s_isNative = true;
            static const eType s_type = eType::String;

            static AZ::Uuid GetAZType(const Data::Type& = {}) { return azrtti_typeid<StringType>(); }
            static Data::Type GetSCType(const AZ::TypeId& = AZ::TypeId::CreateNull()) { return Data::Type::String(); }
            static const char* GetName(const Data::Type& = {}) { return "String"; }
            static Type GetDefault(const Data::Type& = {}) { return StringType(); }
            static bool IsDefault(const Type& value, const Data::Type& = {}) { return value != GetDefault(); }
        };

        template<>
        struct Traits<TransformType>
        {
            using Type = TransformType;
            static const bool s_isAutoBoxed = false;
            static const bool s_isNative = true;
            static const eType s_type = eType::Transform;

            static AZ::Uuid GetAZType(const Data::Type& = {}) { return azrtti_typeid<TransformType>(); }
            static Data::Type GetSCType(const AZ::TypeId& = AZ::TypeId::CreateNull()) { return Data::Type::Transform(); }
            static const char* GetName(const Data::Type& = {}) { return "Transform"; }
            static Type GetDefault(const Data::Type& = {}) { return TransformType::CreateIdentity(); }
            static bool IsDefault(const Type& value, const Data::Type& = {}) { return value != GetDefault(); }
        };

        template<>
        struct Traits<Vector2Type>
        {
            using Type = Vector2Type;
            static const bool s_isAutoBoxed = true;
            static const bool s_isNative = true;
            static const eType s_type = eType::Vector2;

            static AZ::Uuid GetAZType(const Data::Type& = {}) { return azrtti_typeid<Vector2Type>(); }
            static Data::Type GetSCType(const AZ::TypeId& = AZ::TypeId::CreateNull()) { return Data::Type::Vector2(); }
            static const char* GetName(const Data::Type& = {}) { return "Vector2"; }
            static Type GetDefault(const Data::Type& = {}) { return Vector2Type::CreateZero(); }
            static bool IsDefault(const Type& value, const Data::Type& = {}) { return value != GetDefault(); }
        };

        template<>
        struct Traits<Vector3Type>
        {
            using Type = Vector3Type;
            static const bool s_isAutoBoxed = true;
            static const bool s_isNative = true;
            static const eType s_type = eType::Vector3;

            static AZ::Uuid GetAZType(const Data::Type& = {}) { return azrtti_typeid<Vector3Type>(); }
            static Data::Type GetSCType(const AZ::TypeId& = AZ::TypeId::CreateNull()) { return Data::Type::Vector3(); }
            static const char* GetName(const Data::Type& = {}) { return "Vector3"; }
            static Type GetDefault(const Data::Type& = {}) { return Vector3Type::CreateZero(); }
            static bool IsDefault(const Type& value, const Data::Type& = {}) { return value != GetDefault(); }
        };

        template<>
        struct Traits<Vector4Type>
        {
            using Type = Vector4Type;
            static const bool s_isAutoBoxed = true;
            static const bool s_isNative = true;
            static const eType s_type = eType::Vector4;

            static AZ::Uuid GetAZType(const Data::Type& = {}) { return azrtti_typeid<Vector4Type>(); }
            static Data::Type GetSCType(const AZ::TypeId& = AZ::TypeId::CreateNull()) { return Data::Type::Vector4(); }
            static const char* GetName(const Data::Type& = {}) { return "Vector4"; }
            static Type GetDefault(const Data::Type& = {}) { return Vector4Type::CreateZero(); }
            static bool IsDefault(const Type& value, const Data::Type& = {}) { return value != GetDefault(); }
        };

        /**
        * Special Case Traits specialization for the string_view type
        * The C++ string_view class uses the StringType traits so that in ScriptCanvas string_view maps as
        * a native string type in ScriptCanvas. This to allow the common C++ concepts of a "string" to be treated
        * as a string in ScriptCanvas according to the ScriptCanvas spec
        */
        template<>
        struct Traits<AZStd::string_view> : public Traits<StringType>
        {};

        /**
        * Special Case Traits specialization for the const char* type
        * The C++ const char* class uses the StringType traits so that in ScriptCanvas string_view maps as
        * a native string type in ScriptCanvas. This to allow the common C++ concepts of a "string" to be treated
        * as a string in ScriptCanvas according to the ScriptCanvas spec
        */
        template<>
        struct Traits<const char*> : public Traits<StringType>
        {};

        template<>
        struct eTraits<eType::AABB> : Traits<AABBType> {};

        template<>
        struct eTraits<eType::Boolean> : Traits<BooleanType> {};

        template<>
        struct eTraits<eType::BehaviorContextObject>
        {
            using Type = BehaviorContextObjectPtr;
            static const bool s_isAutoBoxed = false;
            static const bool s_isNative = false;
            static const eType s_type = eType::BehaviorContextObject;

            static AZ::Uuid GetAZType(const Data::Type& scType) { return scType.GetAZType(); }
            static Data::Type GetSCType(const AZ::TypeId& typeId = AZ::TypeId::CreateNull()) { return Data::Type::BehaviorContextObject(typeId); }
            static const char* GetName(const Data::Type& scType)
            {
                const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(scType.GetAZType());
                return behaviorClass ? behaviorClass->m_name.c_str() : "Invalid";
            }
            
            static Type GetDefault(const Data::Type& scType);
            static bool IsDefault(const Type& value, const Data::Type& scType);
        };

        template<>
        struct eTraits<eType::Color> : Traits<ColorType> {};

        template<>
        struct eTraits<eType::CRC> : Traits<CRCType> {};

        template<>
        struct eTraits<eType::EntityID> : Traits<EntityIDType> {};

        template<>
        struct eTraits<eType::Matrix3x3> : Traits<Matrix3x3Type> {};

        template<>
        struct eTraits<eType::Matrix4x4> : Traits<Matrix4x4Type> {};

        template<>
        struct eTraits<eType::Number> : Traits<NumberType> {};

        template<>
        struct eTraits<eType::OBB> : Traits<OBBType> {};

        template<>
        struct eTraits<eType::Plane> : Traits<PlaneType> {};

        template<>
        struct eTraits<eType::Quaternion> : Traits<QuaternionType> {};

        template<>
        struct eTraits<eType::String> : Traits<StringType> {};

        template<>
        struct eTraits<eType::Transform> : Traits<TransformType> {};

        template<>
        struct eTraits<eType::Vector2> : Traits<Vector2Type> {};

        template<>
        struct eTraits<eType::Vector3> : Traits<Vector3Type> {};

        template<>
        struct eTraits<eType::Vector4> : Traits<Vector4Type> {};

    } // namespace Data
} // namespace ScriptCanvas