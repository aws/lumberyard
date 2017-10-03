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

#include <ScriptCanvas/Core/NativeDatumNode.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Math
        {
            const bool k_TransformNodeHasProperties = true;

            class Transform
                : public NativeDatumNode<Transform, Data::TransformType, k_TransformNodeHasProperties>
            {
            public:
                using ParentType = NativeDatumNode<Transform, Data::TransformType, k_TransformNodeHasProperties>;
                AZ_COMPONENT(Transform, "{B74F127B-72E0-486B-86FF-2233767C2804}", ParentType);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<Transform, PureData>()
                            ->Version(0)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<Transform>("Transform", "A 3D transform value")
                                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Transform.png")
                                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                                ;
                        }
                    }
                }

                void AddProperties()
                {
                    AddProperty(&AZ::Transform::GetBasisX, static_cast<void(AZ::Transform::*)(const AZ::Vector3&)>(&AZ::Transform::SetBasisX), "basisX");
                    AddProperty(&AZ::Transform::GetBasisY, static_cast<void(AZ::Transform::*)(const AZ::Vector3&)>(&AZ::Transform::SetBasisY), "basisY");
                    AddProperty(&AZ::Transform::GetBasisZ, static_cast<void(AZ::Transform::*)(const AZ::Vector3&)>(&AZ::Transform::SetBasisZ), "basisZ");
                    AddProperty(&AZ::Transform::GetPosition, static_cast<void(AZ::Transform::*)(const AZ::Vector3&)>(&AZ::Transform::SetPosition), "position");
                }

                void Visit(NodeVisitor& visitor) const override
                {
                    visitor.Visit(*this);
                }
            };
        }
    }
}