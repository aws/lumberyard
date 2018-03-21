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
#include "LmbrCentral_precompiled.h"
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
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorCompoundShapeComponent::ConfigurationChanged)
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

    void EditorCompoundShapeComponent::Activate()
    {
        CompoundShapeComponentRequestsBus::Handler::BusConnect(GetEntityId());
    }

    void EditorCompoundShapeComponent::Deactivate()
    {
        CompoundShapeComponentRequestsBus::Handler::BusDisconnect(GetEntityId());
    }
    
    bool EditorCompoundShapeComponent::HasShapeComponentReferencingEntityId(const AZ::EntityId& entityId)
    {
        const AZStd::list<AZ::EntityId>& childEntities = m_configuration.GetChildEntities();
        auto it = AZStd::find(childEntities.begin(), childEntities.end(), entityId);
        if (it != childEntities.end())
        {
            return true;
        }

        bool result = false;
        for (const AZ::EntityId& childEntityId : childEntities)
        {
            if (childEntityId == GetEntityId())
            {
                // This can happen when we have multiple entities selected and use the picker to set the reference to one of the selected entities
                return true;
            }

            if (childEntityId.IsValid())
            {
                CompoundShapeComponentRequestsBus::EventResult(result, childEntityId, &CompoundShapeComponentRequests::HasShapeComponentReferencingEntityId, entityId);

                if (result)
                {
                    return true;
                }
            }
        }

        return false;
    }

    AZ::u32 EditorCompoundShapeComponent::ConfigurationChanged()
    {
        AZStd::list<AZ::EntityId>& childEntities = const_cast<AZStd::list<AZ::EntityId>&>(m_configuration.GetChildEntities());
        auto selfIt = AZStd::find(childEntities.begin(), childEntities.end(), GetEntityId());
        if (selfIt != childEntities.end())
        {
            QMessageBox(QMessageBox::Warning,
                "Input Error",
                "You cannot set a compound shape component to reference itself!",
                QMessageBox::Ok, QApplication::activeWindow()).exec();

            *selfIt = AZ::EntityId();
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }

        bool result = false;
        for (auto childIt = childEntities.begin(); childIt != childEntities.end(); ++childIt)
        {
            CompoundShapeComponentRequestsBus::EventResult(result, *childIt, &CompoundShapeComponentRequests::HasShapeComponentReferencingEntityId, GetEntityId());
            if (result)
            {
                QMessageBox(QMessageBox::Warning,
                    "Endless Loop Warning",
                    "Having circular references within a compound shape results in an endless loop!  Clearing the reference.",
                    QMessageBox::Ok, QApplication::activeWindow()).exec();

                *childIt = AZ::EntityId();
                return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
            }
        }

        return AZ::Edit::PropertyRefreshLevels::None;
    }

} // namespace LmbrCentral
