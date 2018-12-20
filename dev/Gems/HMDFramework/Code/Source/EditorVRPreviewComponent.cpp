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
#include "HMDFramework_precompiled.h"
#include "EditorVRPreviewComponent.h"
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h> // For IEditor
#include <AzCore/Component/TransformBus.h>

namespace AZ
{
    namespace VR
    {
        //////////////////////////////////////////////////////////////////////////

        void EditorVRPreviewComponent::Reflect(ReflectContext* context)
        {
            auto serializeContext = azrtti_cast<SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<EditorVRPreviewComponent, Component>()
                    ->Version(1)
                    ->Field("FirstActivate", &EditorVRPreviewComponent::m_firstActivate);

                EditContext* editContext = serializeContext->GetEditContext();

                if (editContext)
                {
                    editContext->Class<EditorVRPreviewComponent>(
                        "VR Preview", "The VR Preview component creates a user-editable navigation mesh that defines valid areas that users can teleport to")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::Category, "VR")
                        ->Attribute(Edit::Attributes::Icon, "Editor/Icons/Components/VRPreview.png")
                        ->Attribute(Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/VRPreview.png")
                        ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-vrpreview-component.html")
                        ->Attribute(Edit::Attributes::AutoExpand, true);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // Component interface implementation
        void EditorVRPreviewComponent::Init()
        {
            EditorComponentBase::Init();

            // connect here for slice created callbacks
            AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        }

        void EditorVRPreviewComponent::Activate()
        {
            // if we are being activated for the first time, generate the navigation area
            if (m_firstActivate)
            {
                GenerateEditorNavigationArea();
                m_firstActivate = false;
            }

            EditorComponentBase::Activate();
        }

        void EditorVRPreviewComponent::Deactivate()
        {
            EditorComponentBase::Deactivate();
            AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
        }
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorEntityContextNotificationBus interface implementation
        void EditorVRPreviewComponent::OnSliceInstantiated(const Data::AssetId& sliceAssetId, const SliceComponent::SliceInstanceAddress& sliceAddress, const AzFramework::SliceInstantiationTicket& ticket)
        {
            const SliceComponent::EntityList& entities = sliceAddress.GetInstance()->GetInstantiated()->m_entities;
            const EntityId entityId = m_entity->GetId();

            for (const Entity* entity : entities)
            {
                if (entity->GetId() == entityId)
                {
                    // the slice instantiated us!
                    GenerateEditorNavigationArea();
                    break;
                }
            }

            // whether or not we were the ones just instantiated, stop listening
            AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
        }
        //////////////////////////////////////////////////////////////////////////

        void EditorVRPreviewComponent::GenerateEditorNavigationArea()
        {
            const Entity* entity = GetEntity();
            AZ_Assert(entity, "Entity pointer invalid during EditorVRPreviewComponent::Init()!");

            Transform worldTM;
            EBUS_EVENT_ID_RESULT(worldTM, GetEntityId(), TransformBus, GetWorldTM);

            const Vector3 halfBounds = m_bounds * 0.5f;
            const Vector3 position = worldTM.GetPosition() - Vector3(0,0,halfBounds.GetZ());
            

            // Construct a volume from the position of the Entity Transform and the desired Bounds
            const size_t NUM_VOLUME_VERTS = 4;
            Vector3 vertices[NUM_VOLUME_VERTS];

            vertices[0] = Vector3(halfBounds.GetX(), halfBounds.GetY(), 0);
            vertices[1] = Vector3(-halfBounds.GetX(), halfBounds.GetY(), 0);
            vertices[2] = Vector3(-halfBounds.GetX(), -halfBounds.GetY(), 0);
            vertices[3] = Vector3(halfBounds.GetX(), -halfBounds.GetY(), 0);

            EBUS_EVENT(AzToolsFramework::EditorRequests::Bus,
                GenerateNavigationArea,
                entity->GetName() + "_NavMesh",
                position,
                vertices,
                NUM_VOLUME_VERTS,
                m_bounds.GetZ());
        }
    } // namespace VR
} // namespace AZ
