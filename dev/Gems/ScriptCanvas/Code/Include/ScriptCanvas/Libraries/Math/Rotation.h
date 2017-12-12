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
            const bool k_RotationNodeHasProperties = true;

            class Rotation
                : public NativeDatumNode<Rotation, Data::RotationType, k_RotationNodeHasProperties>
            {
            public:
                using ParentType = NativeDatumNode<Rotation, Data::RotationType, k_RotationNodeHasProperties>;
                AZ_COMPONENT(Rotation, "{E17FE11D-69F2-4746-B582-778B48D0BF47}", ParentType);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<Rotation, PureData>()
                            ->Version(0)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<Rotation>("Rotation", "Pitch/Roll/Yaw (generally applied in Yaw, Pitch, Roll order)")
                                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Rotation.png")
                                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                                ;
                        }
                    }
                }

                void AddProperties()
                {
                    // \todo these will be in radians, but we will want to present them in degrees
                    AddProperty(&Data::RotationType::GetX, &Data::RotationType::SetX, "x");
                    AddProperty(&Data::RotationType::GetY, &Data::RotationType::SetY, "y");
                    AddProperty(&Data::RotationType::GetZ, &Data::RotationType::SetZ, "z");
                    AddProperty(&Data::RotationType::GetW, &Data::RotationType::SetW, "w");
                }

                void Visit(NodeVisitor& visitor) const override
                {
                    visitor.Visit(*this);
                }
            };
        }
    }
}