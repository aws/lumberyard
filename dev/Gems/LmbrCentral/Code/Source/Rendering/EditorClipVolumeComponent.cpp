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
#include "EditorClipVolumeComponent.h"

#include <AzCore/Math/Geometry2DUtils.h>

#include <Shape/PolygonPrismShape.h>

#include <MathConversion.h>
#include <I3DEngine.h>
#include "IEntitySystem.h"

namespace LmbrCentral
{    

    void EditorClipVolumeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorClipVolumeComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorClipVolumeComponent>("Clip volume", "Controls clip volumes.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Rendering")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Component_Placeholder.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/ComponentPlaceholder.png")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ;
            }
        }
    }

    void EditorClipVolumeComponent::Init()
    {
    }

    void EditorClipVolumeComponent::Activate()
    {
        m_clipVolume.AttachToEntity(GetEntityId());
        m_clipVolume.CreateClipVolume();

        AzToolsFramework::Components::EditorComponentBase::Activate();

        PolygonPrismShapeComponentNotificationBus::Handler::BusConnect(GetEntityId());
        ClipVolumeComponentRequestBus::Handler::BusConnect(GetEntityId());

        // Get the initial shape
        ShapeChanged();
    }

    void EditorClipVolumeComponent::Deactivate()
    {
        m_clipVolume.Cleanup();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();

        ClipVolumeComponentRequestBus::Handler::BusDisconnect();
        PolygonPrismShapeComponentNotificationBus::Handler::BusDisconnect();
    }

    void EditorClipVolumeComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (auto clipVolumeComponent = gameEntity->CreateComponent<ClipVolumeComponent>())
        {
            m_clipVolume.CopyPropertiesTo(clipVolumeComponent->m_clipVolume);
            // ensure we do not copy across the edit time entity id
            clipVolumeComponent->m_clipVolume.m_entityId = AZ::EntityId();
        }
    }

    void EditorClipVolumeComponent::ShapeChanged()
    {
        AZ::ConstPolygonPrismPtr polygonPrismPtr = nullptr;
        PolygonPrismShapeComponentRequestBus::EventResult(polygonPrismPtr, GetEntityId(), &PolygonPrismShapeComponentRequests::GetPolygonPrism);

        if (!polygonPrismPtr)
        {
            AZ_Error("Clip Volume Component", false, "Polygon prism does not exist for clip volume.");
            return;
        }

        // Turn shape into vertices using Vec3 for use with the BSP system
        const AZ::PolygonPrism& polygonPrism = *polygonPrismPtr;

        const size_t initialVertexCount = polygonPrism.m_vertexContainer.Size();
        const AZStd::vector<AZ::Vector2>& verticesLocal = polygonPrism.m_vertexContainer.GetVertices();

        if (initialVertexCount < 3)
        {
            AZ_Error("Clip Volume Component", false, "Invalid polygon prism for entity \"%s\""
                " - must have at least 3 vertices.",
                GetEntity()->GetName().c_str());

            return;
        }

        if (!AZ::Geometry2DUtils::IsSimplePolygon(verticesLocal))
        {
            AZ_Error("Clip Volume Component", false, "Invalid polygon prism for entity \"%s\""
                " - must be a simple polygon (no self intersection or duplicate vertices).",
                GetEntity()->GetName().c_str());

            return;
        }

        if (!AZ::Geometry2DUtils::IsConvex(verticesLocal))
        {
            AZ_Error("Clip Volume Component", false, "Invalid polygon prism for entity \"%s\""
                " - must be convex.",
                GetEntity()->GetName().c_str());

            return;
        }

        AZStd::vector<Vec3> prismVertices;
        const size_t totalVertexCount = initialVertexCount * 2;
        prismVertices.resize(totalVertexCount);
        for (size_t vertexNum = 0; vertexNum < totalVertexCount; ++vertexNum)
        {
            prismVertices[vertexNum] =
                Vec3(verticesLocal[vertexNum % initialVertexCount].GetX(),
                    verticesLocal[vertexNum % initialVertexCount].GetY(),
                    (vertexNum < initialVertexCount) ? 0 : polygonPrism.GetHeight());
        }

        AZStd::vector<AZ::Vector3> shapeVertices;

        // bottom face
        for (int i = 0; i < (initialVertexCount - 2); ++i)
        {
            shapeVertices.push_back(LYVec3ToAZVec3(prismVertices[0]));
            shapeVertices.push_back(LYVec3ToAZVec3(prismVertices[i + 2]));
            shapeVertices.push_back(LYVec3ToAZVec3(prismVertices[i + 1]));
        }
        // top face
        for (int i = 0; i < (initialVertexCount - 2); ++i)
        {
            shapeVertices.push_back(LYVec3ToAZVec3(prismVertices[initialVertexCount]));
            shapeVertices.push_back(LYVec3ToAZVec3(prismVertices[initialVertexCount + i + 1]));
            shapeVertices.push_back(LYVec3ToAZVec3(prismVertices[initialVertexCount + i + 2]));
        }

        // side faces
        for (int i = 0; i < initialVertexCount; ++i)
        {
            shapeVertices.push_back(LYVec3ToAZVec3(prismVertices[i]));
            shapeVertices.push_back(LYVec3ToAZVec3(prismVertices[((i + 1) % initialVertexCount) + initialVertexCount]));
            shapeVertices.push_back(LYVec3ToAZVec3(prismVertices[i + initialVertexCount]));

            shapeVertices.push_back(LYVec3ToAZVec3(prismVertices[i]));
            shapeVertices.push_back(LYVec3ToAZVec3(prismVertices[(i + 1) % initialVertexCount]));
            shapeVertices.push_back(LYVec3ToAZVec3(prismVertices[((i + 1) % initialVertexCount) + initialVertexCount]));
        }

        m_clipVolume.UpdatedVertices(shapeVertices);
    }

} //namespace LmbrCentral
