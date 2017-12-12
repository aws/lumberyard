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
#include "StdAfx.h"
#include "CompoundShapeComponent.h"
#include "EditorCompoundShapeComponent.h"

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace LmbrCentral
{
    void EditorCompoundShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorCompoundShapeComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1)
                ->Field("Configuration", &EditorCompoundShapeComponent::m_configuration)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorCompoundShapeComponent>(
                    "Compound Shape", "The Compound Shape component allows two or more shapes to be combined to create more complex shapes")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/ColliderSphere.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Sphere.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-shapes.html")
                    ->DataElement(0, &EditorCompoundShapeComponent::m_configuration, "Configuration", "Compound Shape Configuration")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
                        ;
            }
        }
    }

    void EditorCompoundShapeComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        auto component = gameEntity->CreateComponent<CompoundShapeComponent>();
        if (component)
        {
            component->m_configuration = m_configuration;
        }
    }

} // namespace LmbrCentral
