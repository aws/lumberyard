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
#include "Math.h"

#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Random.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <Libraries/Libraries.h>
#include <algorithm>
#include <random>

#pragma warning (disable:4503) // decorated name length exceeded, name was truncated

namespace ScriptCanvas
{
    namespace MathRandom
    {
        struct RandomDetails
        {
            AZ_CLASS_ALLOCATOR(RandomDetails, AZ::SystemAllocator, 0);

            std::random_device m_randomDevice;
            std::mt19937 m_randomEngine;

            RandomDetails()
            {
                m_randomEngine = std::mt19937(m_randomDevice());
            }
        };

        static RandomDetails* s_RandomDetails = nullptr;

        std::mt19937& GetRandomEngine()
        {
            if (s_RandomDetails == nullptr)
            {
                s_RandomDetails = aznew RandomDetails;
            }
            return s_RandomDetails->m_randomEngine;
        }

        template<>
        AZ::s8 GetRandomIntegral<AZ::s8>(AZ::s8 leftNumber, AZ::s8 rightNumber)
        {
            AZ::s32 randVal = GetRandomIntegral(static_cast<AZ::s32>(leftNumber), static_cast<AZ::s32>(rightNumber));
            return static_cast<AZ::s8>(randVal);
        }

        template<>
        AZ::u8 GetRandomIntegral<AZ::u8>(AZ::u8 leftNumber, AZ::u8 rightNumber)
        {
            AZ::u32 randVal = GetRandomIntegral(static_cast<AZ::u32>(leftNumber), static_cast<AZ::u32>(rightNumber));
            return static_cast<AZ::u8>(randVal);
        }

        template<>
        char GetRandomIntegral<char>(char leftNumber, char rightNumber)
        {
            AZ::s32 randVal = GetRandomIntegral(static_cast<AZ::s32>(leftNumber), static_cast<AZ::s32>(rightNumber));
            return static_cast<char>(randVal);
        }

        template<typename NumberType>
        NumberType GetRandomReal(NumberType leftNumber, NumberType rightNumber)
        {
            if (AZ::IsClose(leftNumber, rightNumber, std::numeric_limits<NumberType>::epsilon()))
            {
                return leftNumber;
            }
            if (s_RandomDetails == nullptr)
            {
                s_RandomDetails = aznew RandomDetails;
            }
            NumberType min = AZ::GetMin(leftNumber, rightNumber);
            NumberType max = AZ::GetMax(leftNumber, rightNumber);
            std::uniform_real_distribution<> dis(min, max);
            return static_cast<NumberType>(dis(s_RandomDetails->m_randomEngine));
        }
    }

    namespace Nodes
	{
        namespace Math
        {
            Data::NumberType GetRandom(Data::NumberType lhs, Data::NumberType rhs)
            {
                return MathRandom::GetRandomReal(lhs, rhs);
            }
        }
    }
	
    namespace Library
    {
        void Math::Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<Math, LibraryDefinition>()
                    ->Version(1)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<Math>("Math", "")->
                        ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                        Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Libraries/Math.png")->
                        Attribute(AZ::Edit::Attributes::CategoryStyle, ".math")->
                        Attribute(ScriptCanvas::Attributes::Node::TitlePaletteOverride, "MathNodeTitlePalette")
                        ;
                }
            }
        }

        void Math::InitNodeRegistry(NodeRegistry& nodeRegistry)
        {
            using namespace ScriptCanvas::Nodes::Math;
            AddNodeToRegistry<Math, AABB>(nodeRegistry);
            AddNodeToRegistry<Math, CRC>(nodeRegistry);
            AddNodeToRegistry<Math, Color>(nodeRegistry);
            AddNodeToRegistry<Math, Divide>(nodeRegistry);
            AddNodeToRegistry<Math, Matrix3x3>(nodeRegistry);
            AddNodeToRegistry<Math, Matrix4x4>(nodeRegistry);
            AddNodeToRegistry<Math, Multiply>(nodeRegistry);
            AddNodeToRegistry<Math, Number>(nodeRegistry);
            AddNodeToRegistry<Math, OBB>(nodeRegistry);
            AddNodeToRegistry<Math, Plane>(nodeRegistry);           
            AddNodeToRegistry<Math, Random>(nodeRegistry);
            AddNodeToRegistry<Math, Quaternion>(nodeRegistry);
            AddNodeToRegistry<Math, Subtract>(nodeRegistry);
            AddNodeToRegistry<Math, Sum>(nodeRegistry);
            AddNodeToRegistry<Math, Transform>(nodeRegistry);
            AddNodeToRegistry<Math, Vector2>(nodeRegistry);
            AddNodeToRegistry<Math, Vector3>(nodeRegistry);
            AddNodeToRegistry<Math, Vector4>(nodeRegistry);
            MathRegistrar::AddToRegistry<Math>(nodeRegistry);
            RandomNodes::Registrar::AddToRegistry<Math>(nodeRegistry);
            AABBNodes::Registrar::AddToRegistry<Math>(nodeRegistry);
            ColorNodes::Registrar::AddToRegistry<Math>(nodeRegistry);
            CrcNodes::Registrar::AddToRegistry<Math>(nodeRegistry);
            Matrix3x3Nodes::Registrar::AddToRegistry<Math>(nodeRegistry);
            Matrix4x4Nodes::Registrar::AddToRegistry<Math>(nodeRegistry);
            OBBNodes::Registrar::AddToRegistry<Math>(nodeRegistry);
            PlaneNodes::Registrar::AddToRegistry<Math>(nodeRegistry);
            QuaternionNodes::Registrar::AddToRegistry<Math>(nodeRegistry);
            TransformNodes::Registrar::AddToRegistry<Math>(nodeRegistry);
            Vector2Nodes::Registrar::AddToRegistry<Math>(nodeRegistry);
            Vector3Nodes::Registrar::AddToRegistry<Math>(nodeRegistry);
            Vector4Nodes::Registrar::AddToRegistry<Math>(nodeRegistry);
        }
        
        AZStd::vector<AZ::ComponentDescriptor*> Math::GetComponentDescriptors()
        {
            AZStd::vector<AZ::ComponentDescriptor*> descriptors = 
                {
                    ScriptCanvas::Nodes::Math::AABB::CreateDescriptor(),
                    ScriptCanvas::Nodes::Math::CRC::CreateDescriptor(),
                    ScriptCanvas::Nodes::Math::Color::CreateDescriptor(),
                    ScriptCanvas::Nodes::Math::Divide::CreateDescriptor(),
                    ScriptCanvas::Nodes::Math::Matrix3x3::CreateDescriptor(),
                    ScriptCanvas::Nodes::Math::Matrix4x4::CreateDescriptor(),
                    ScriptCanvas::Nodes::Math::Multiply::CreateDescriptor(),
                    ScriptCanvas::Nodes::Math::Number::CreateDescriptor(),
                    ScriptCanvas::Nodes::Math::OBB::CreateDescriptor(),
                    ScriptCanvas::Nodes::Math::Plane::CreateDescriptor(),
                    ScriptCanvas::Nodes::Math::Random::CreateDescriptor(),
                    ScriptCanvas::Nodes::Math::Quaternion::CreateDescriptor(),
                    ScriptCanvas::Nodes::Math::Subtract::CreateDescriptor(),
                    ScriptCanvas::Nodes::Math::Sum::CreateDescriptor(),
                    ScriptCanvas::Nodes::Math::Transform::CreateDescriptor(),
                    ScriptCanvas::Nodes::Math::Vector2::CreateDescriptor(),
                    ScriptCanvas::Nodes::Math::Vector3::CreateDescriptor(),
                    ScriptCanvas::Nodes::Math::Vector4::CreateDescriptor(),
            };

            MathRegistrar::AddDescriptors(descriptors);
            RandomNodes::Registrar::AddDescriptors(descriptors);
            AABBNodes::Registrar::AddDescriptors(descriptors);
            ColorNodes::Registrar::AddDescriptors(descriptors);
            CrcNodes::Registrar::AddDescriptors(descriptors);
            Matrix3x3Nodes::Registrar::AddDescriptors(descriptors);
            Matrix4x4Nodes::Registrar::AddDescriptors(descriptors);
            OBBNodes::Registrar::AddDescriptors(descriptors);
            PlaneNodes::Registrar::AddDescriptors(descriptors);
            QuaternionNodes::Registrar::AddDescriptors(descriptors);
            TransformNodes::Registrar::AddDescriptors(descriptors);
            Vector2Nodes::Registrar::AddDescriptors(descriptors);
            Vector3Nodes::Registrar::AddDescriptors(descriptors);
            Vector4Nodes::Registrar::AddDescriptors(descriptors);
            return descriptors;
        }
    
    } // namespace Math

} // namespace ScriptCavas
