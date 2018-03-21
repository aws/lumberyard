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

#include "Visibility_precompiled.h"
#include "EditorVisAreaComponent.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <Editor/Objects/BaseObject.h>
#include <Editor/Objects/VisAreaShapeObject.h>

// Include files needed for writing DisplayEntity functions that access the DisplayContext directly.
#include <EditorCoreAPI.h>

#include <LegacyEntityConversion/LegacyEntityConversion.h>
#include "MathConversion.h"

namespace Visibility
{
    /*static*/ AZ::Color EditorVisAreaComponent::s_visAreaColor = AZ::Color(1.0f, 0.5f, 0.0f, 1.0f);

    void EditorVisAreaComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides)
    {
        provides.push_back(AZ_CRC("VisAreaService"));
    }
    void EditorVisAreaComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires)
    {
        requires.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    void EditorVisAreaConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorVisAreaConfiguration, VisAreaConfiguration>()
                ->Version(1);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorVisAreaConfiguration>("VisArea Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editContext->Class<VisAreaConfiguration>("VisArea Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &VisAreaConfiguration::m_Height, "Height", "How tall the VisArea is.")
                    ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &VisAreaConfiguration::ChangeHeight)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &VisAreaConfiguration::m_DisplayFilled, "DisplayFilled", "Display the VisArea as a filled volume.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &VisAreaConfiguration::ChangeDisplayFilled)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &VisAreaConfiguration::m_AffectedBySun, "AffectedBySun", "Allows sunlight to affect objects inside the VisArea.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &VisAreaConfiguration::ChangeAffectedBySun)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &VisAreaConfiguration::m_ViewDistRatio, "ViewDistRatio", "Specifies how far the VisArea is rendered.")
                    ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &VisAreaConfiguration::ChangeViewDistRatio)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &VisAreaConfiguration::m_OceanIsVisible, "OceanIsVisible", "Ocean will be visible when looking outside the VisArea.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &VisAreaConfiguration::ChangeOceanIsVisible)

                    //Note: This will not work as expected. See Activate where we set the callbacks on the vertex container directly
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &VisAreaConfiguration::m_vertexContainer, "Vertices", "Points that make up the floor of the VisArea.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &VisAreaConfiguration::ChangeVertexContainer)
                    ;
            }
        }
    }

    void EditorVisAreaComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorVisAreaComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1)
                ->Field("m_config", &EditorVisAreaComponent::m_config)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorVisAreaComponent>("VisArea", "An area where only objects inside the area will be visible.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Rendering")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/VisArea.png")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/VisArea.png")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/console/lumberyard/userguide/vis-area-component")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorVisAreaComponent::m_config, "m_config", "No Description")
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<EditorVisAreaComponent>()->RequestBus("VisAreaComponentRequestBus");
        }

        EditorVisAreaConfiguration::Reflect(context);
    }

    void EditorVisAreaConfiguration::ChangeHeight()
    {
        m_component->UpdateVisArea();
    }

    void EditorVisAreaConfiguration::ChangeDisplayFilled()
    {
        m_component->UpdateVisArea();
    }

    void EditorVisAreaConfiguration::ChangeAffectedBySun()
    {
        m_component->UpdateVisArea();
    }

    void EditorVisAreaConfiguration::ChangeViewDistRatio()
    {
        m_component->UpdateVisArea();
    }

    void EditorVisAreaConfiguration::ChangeOceanIsVisible()
    {
        m_component->UpdateVisArea();
    }

    void EditorVisAreaConfiguration::ChangeVertexContainer()
    {
        m_component->UpdateVisArea();
    }

    EditorVisAreaComponent::EditorVisAreaComponent()
        : m_area(nullptr)
    {
        m_config.m_component = this;
    }

    EditorVisAreaComponent::~EditorVisAreaComponent()
    {
        m_vertexSelection.Destroy();

        if (m_area)
        {
            // Reset the listener vis area in the unlucky case that we are deleting the
            // vis area where the listener is currently in
            GetIEditor()->Get3DEngine()->DeleteVisArea(m_area);
            m_area = nullptr;
        }
    }

    void EditorVisAreaComponent::Activate()
    {
        // NOTE: We create the visarea here at activated, but destroy it in the destructor.
        // We have to do this, otherwise the visarea is not saved into the level.
        // Unfortunately, at this time we cannot create the visareas at game runtime.
        // This means that dynamic slices cannot effectively contain vis areas until we fix the core rendering system to allow that.
        const AZ::u64 visGUID = AZ::u64(GetEntityId());
        m_area = GetIEditor()->Get3DEngine()->CreateVisArea(visGUID);

        //Give default values to the vertices if needed
        if (m_config.m_vertexContainer.Size() == 0)
        {
            m_config.m_vertexContainer.AddVertex(AZ::Vector3(-1.0f, -1.0f, 0.0f));
            m_config.m_vertexContainer.AddVertex(AZ::Vector3(+1.0f, -1.0f, 0.0f));
            m_config.m_vertexContainer.AddVertex(AZ::Vector3(+1.0f, +1.0f, 0.0f));
            m_config.m_vertexContainer.AddVertex(AZ::Vector3(-1.0f, +1.0f, 0.0f));
        }

        Base::Activate();

        VisAreaComponentRequestBus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::EditorEntityInfoNotificationBus::Handler::BusConnect();
        EntitySelectionEvents::Bus::Handler::BusConnect(GetEntityId());

        //Apply callbacks to VertexContainer directly
        auto containerChanged = [this]()
        {
            UpdateVisArea();

            bool selected = false;
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
                selected, GetEntityId(), &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSelected);

            if (selected)
            {
                m_vertexSelection.Destroy();
                CreateManipulators();
            }
        };

        auto vertexAdded = [this, containerChanged](size_t index)
        {
            bool selected = false;
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
                selected, GetEntityId(), &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSelected);

            if (selected)
            {
                containerChanged();

                AzToolsFramework::ManipulatorManagerId managerId = AzToolsFramework::ManipulatorManagerId(1);
                m_vertexSelection.CreateTranslationManipulator(GetEntityId(), managerId,
                    AzToolsFramework::TranslationManipulator::Dimensions::Three,
                    m_config.m_vertexContainer.GetVertices()[index], index,
                    AzToolsFramework::ConfigureTranslationManipulatorAppearance3d);
            }
        };

        auto vertexChanged = [this]()
        {
            UpdateVisArea();
            m_vertexSelection.Refresh();
        };

        m_config.m_vertexContainer.SetCallbacks(
            vertexAdded, [containerChanged](size_t) { containerChanged(); },
            vertexChanged, containerChanged, containerChanged);

        //We can rely on the transform component so nab the most up to date
        //transform here
        AZ::TransformBus::EventResult(m_currentWorldTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        //Make the initial vis area with the data we just parsed
        UpdateVisArea();
    }

    void EditorVisAreaComponent::Deactivate()
    {
        m_vertexSelection.Destroy();

        EntitySelectionEvents::Bus::Handler::BusDisconnect(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEntityInfoNotificationBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect(GetEntityId());
        VisAreaComponentRequestBus::Handler::BusDisconnect(GetEntityId());

        Base::Deactivate();
    }

    void EditorVisAreaComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentWorldTransform = world;
        UpdateVisArea();
    }

    void EditorVisAreaComponent::UpdateVisArea()
    {
        if (m_area)
        {
            std::vector<Vec3> points;

            const AZStd::vector<AZ::Vector3>& vertices = m_config.m_vertexContainer.GetVertices();
            const size_t vertexCount = vertices.size();

            if (vertexCount > 3)
            {
                const Matrix34& wtm = AZTransformToLYTransform(GetWorldTM());
                points.resize(vertexCount);
                for (size_t i = 0; i < vertexCount; i++)
                {
                    points[i] = wtm.TransformPoint(AZVec3ToLYVec3(vertices[i]));
                }

                SVisAreaInfo info;
                info.fHeight = GetHeight();
                info.bAffectedByOutLights = m_config.m_AffectedBySun;
                info.fViewDistRatio = m_config.m_ViewDistRatio;
                info.bOceanIsVisible = m_config.m_OceanIsVisible;

                //Unconfigurable; these values are used by other area types
                //We set them just so that debugging later it's clear that these
                //aren't being used because this is a VisArea.
                info.fPortalBlending = -1;
                info.bDoubleSide = true;
                info.bUseDeepness = false;
                info.bUseInIndoors = false;

                const AZStd::string name = AZStd::string("visarea_") + GetEntity()->GetName();

                GetIEditor()->Get3DEngine()->UpdateVisArea(m_area, &points[0], points.size(), name.c_str(), info, true);
            }
        }
    }

    void EditorVisAreaComponent::DisplayEntity(bool& handled)
    {
        /*
            The VisArea and Portal share a common strangeness with how they are displayed.
            The Legacy visualization is actually incorrect! It's important to know that
            the vis volumes essentially act as a points on an XY plane with a known Z
            position and a height. The volumes that actually affect rendition are planar
            quads with a height. The height is calculated by the largest height value on
            a local point + the given height value on the component.

            Also note that this visualization does not display the floors or ceilings of the vis area
            volumes. There is currently no way with the display context to easily draw a filled polygon.
            We could try to draw some triangles but it would take up a great deal of rendering
            time and could potentially slow down the editor more than we want if there are a lot
            of volumes.
        */
        auto* dc = AzFramework::EntityDebugDisplayRequestBus::FindFirstHandler();
        if (dc != nullptr)
        {
            const AZStd::vector<AZ::Vector3>& vertices = m_config.m_vertexContainer.GetVertices();
            const size_t vertexCount = vertices.size();

            //We do not want to push a transform with scale or rotation as the
            //vis area is always snapped to the XY plane with a height.
            //Scale will be applied during flattening
            AZ::Transform translation = AZ::Transform::CreateIdentity();
            translation.SetTranslation(m_currentWorldTransform.GetTranslation());

            dc->PushMatrix(translation);
            dc->SetColor(s_visAreaColor.GetAsVector4());

            AZStd::vector<AZ::Vector3> transformedPoints;
            transformedPoints.resize(vertexCount);

            //Min and max Z value (in local space)
            float minZ = FLT_MAX;
            float maxZ = FLT_MIN;

            //Apply rotation and scale before removing translation.
            //We want translation to apply with the matrix to make things easier
            //but we need to calculate a difference in Z after rotation and scaling.
            //During the next loop we'll flatten all these points down to a common XY plane
            for (size_t i = 0; i < vertexCount; ++i)
            {
                AZ::Vector3 transformedPoint = m_currentWorldTransform * vertices[i];

                //Back into local space
                transformedPoints[i] = transformedPoint - m_currentWorldTransform.GetTranslation();

                const float transformedZ = transformedPoints[i].GetZ();

                minZ = AZ::GetMin(transformedZ, minZ);
                maxZ = AZ::GetMax(transformedZ, maxZ);
            }

            //The height of the vis area + the max local height
            const float actualHeight = m_config.m_Height + maxZ;

            //Draw walls for every line segment
            size_t transformedPointCount = transformedPoints.size();
            for (size_t i = 0; i < transformedPointCount; ++i)
            {
                AZ::Vector3 lowerLeft   = AZ::Vector3();
                AZ::Vector3 lowerRight  = AZ::Vector3();
                AZ::Vector3 upperRight  = AZ::Vector3();
                AZ::Vector3 upperLeft   = AZ::Vector3();

                lowerLeft = transformedPoints[i];
                lowerRight = transformedPoints[(i + 1) % transformedPointCount];
                //The mod with transformedPointCount ensures that for the last vert it will connect back with vert 0
                //If vert count is 10, the last vert will be i = 9 and we want that to create a surface with vert 0

                //Make all lower points planar
                lowerLeft.SetZ(minZ);
                lowerRight.SetZ(minZ);

                upperRight = AZ::Vector3(lowerRight.GetX(), lowerRight.GetY(), actualHeight);
                upperLeft = AZ::Vector3(lowerLeft.GetX(), lowerLeft.GetY(), actualHeight);

                if (m_config.m_DisplayFilled)
                {
                    dc->SetAlpha(0.3f);
                    //Draw filled quad with two winding orders to make it double sided
                    dc->DrawQuad(lowerLeft, lowerRight, upperRight, upperLeft);
                    dc->DrawQuad(lowerLeft, upperLeft, upperRight, lowerRight);
                }

                dc->SetAlpha(1.0f);
                dc->DrawLine(lowerLeft, lowerRight);
                dc->DrawLine(lowerRight, upperRight);
                dc->DrawLine(upperRight, upperLeft);
                dc->DrawLine(upperLeft, lowerLeft);
            }

            dc->PopMatrix();

            handled = true;
        }
    }

    void EditorVisAreaComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        VisAreaComponent* component = gameEntity->CreateComponent<VisAreaComponent>(&m_config);
    }

    void EditorVisAreaComponent::OnSelected()
    {
        m_vertexSelection.Destroy();
        CreateManipulators();
    }

    void EditorVisAreaComponent::OnDeselected()
    {
        m_vertexSelection.Destroy();
    }

    void EditorVisAreaComponent::CreateManipulators()
    {
        AZStd::unique_ptr<AzToolsFramework::LineSegmentHoverSelection<AZ::Vector3>> visAreaHoverSelection =
            AZStd::make_unique<AzToolsFramework::LineSegmentHoverSelection<AZ::Vector3>>();

        // create interface wrapping internal vertex container for use by vertex selection
        visAreaHoverSelection->m_vertices = AZStd::make_unique<AzToolsFramework::VariableVerticesVertexContainer<AZ::Vector3>>(
            m_config.m_vertexContainer);
        m_vertexSelection.m_hoverSelection = AZStd::move(visAreaHoverSelection);

        m_vertexSelection.m_vertices =
            AZStd::make_unique<AzToolsFramework::VariableVerticesVertexContainer<AZ::Vector3>>(
                m_config.m_vertexContainer);

        const AzToolsFramework::ManipulatorManagerId managerId = AzToolsFramework::ManipulatorManagerId(1);
        m_vertexSelection.Create(GetEntityId(), managerId,
            AzToolsFramework::TranslationManipulator::Dimensions::Three,
            AzToolsFramework::ConfigureTranslationManipulatorAppearance3d);
    }

    AZ::LegacyConversion::LegacyConversionResult VisAreaConverter::ConvertEntity(CBaseObject* entityToConvert)
    {
        if (AZStd::string(entityToConvert->metaObject()->className()) != "CVisAreaShapeObject")
        {
            // We don't know how to convert this entity, whatever it is
            return AZ::LegacyConversion::LegacyConversionResult::Ignored;
        }

        // if we have a parent, and the parent is not yet converted, we ignore this entity!
        // this is not ALWAYS the case - there may be types you wish to create anyway and just put them in the world, so we cannot enforce this
        // inside the CreateConvertedEntity function.
        AZ::Outcome<AZ::Entity*, AZ::LegacyConversion::CreateEntityResult> result(nullptr);

        // Create an entity with a VisArea component.
        const AZ::Uuid editorVisAreaId = "{F4EC32D8-D4DD-54F7-97A8-D195497D5F2C}";
        AZ::ComponentTypeList componentsToAdd { editorVisAreaId };

        AZ::LegacyConversion::LegacyConversionRequestBus::BroadcastResult(result, &AZ::LegacyConversion::LegacyConversionRequests::CreateConvertedEntity, entityToConvert, true, componentsToAdd);

        if (!result.IsSuccess())
        {
            // if we failed due to no parent, we keep processing
            return (result.GetError() == AZ::LegacyConversion::CreateEntityResult::FailedNoParent) ? AZ::LegacyConversion::LegacyConversionResult::Ignored : AZ::LegacyConversion::LegacyConversionResult::Failed;
        }
        AZ::Entity *entity = result.GetValue();

        //The original vis area entity that we want to convert
        CVisAreaShapeObject* visAreaEntity = static_cast<CVisAreaShapeObject*>(entityToConvert);

        const size_t pointCount = static_cast<size_t>(visAreaEntity->GetPointCount());
        if (pointCount < 3)
        {
            //No legacy vis area should exist with less than 3 points
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        float minZ = FLT_MAX;
        float maxZ = FLT_MIN;

        //The first point of the visarea is where the entity's transform is
        bool firstPointIsLowest = false;
        float firstPointHeight = 0.0f;

        //We need to retrieve the planar points of this vis area as they project onto the XY plane.
        //It makes conversion easier.
        AZ::Transform worldTransform;
        AZ::TransformBus::EventResult(worldTransform, entity->GetId(), &AZ::TransformBus::Events::GetWorldTM);
        AZ::Vector3 worldPosition = worldTransform.GetTranslation();

        //Collection of points for the vis area
        AZStd::vector<AZ::Vector3> visAreaPoints;
        visAreaPoints.resize(pointCount);

        for (size_t i = 0; i < pointCount; i++)
        {
            const AZ::Vector3 point = worldTransform * LYVec3ToAZVec3(visAreaEntity->GetPoint(i));
            const float pointZ = point.GetZ();

            if (i == 0)
            {
                firstPointHeight = pointZ;
            }

            if (pointZ < minZ)
            {
                minZ = pointZ;

                //If the first point is the lowest, we don't need to adjust the transform
                if (i == 0)
                {
                    firstPointIsLowest = true;
                }
                else
                {
                    firstPointIsLowest = false;
                }
            }
            if (pointZ > maxZ)
            {
                maxZ = pointZ;
            }

            AZ::Vector3 objectSpacePoint = point;
            objectSpacePoint -= worldPosition;
            //Force Z 0 to make point planar
            visAreaPoints[i] = AZ::Vector3(objectSpacePoint.GetX(), objectSpacePoint.GetY(), 0);
        }

        //Because we are flattening the points as we convert the entity, we need to apply
        //this height difference. If we don't than we will actually be making the VisArea
        //shorter. This is because any difference in height between points is applied to
        //the vis area after it is used in the visibility system (where it is viewed only
        //on the XY plane).
        const float heightMod = maxZ - minZ;

        //Now we can take all the params from the previous entity and shove them into the
        //config for this new component.
        bool conversionSuccess = false;

        const AZ::EntityId newEntityId = entity->GetId();

        if (const CVarBlock* varBlock = visAreaEntity->GetVarBlock())
        {
            using namespace AZ::LegacyConversion::Util;

            conversionSuccess = true;

            conversionSuccess &= ConvertVarBus<VisAreaComponentRequestBus, float>("Height", varBlock, &VisAreaComponentRequestBus::Events::SetHeight, newEntityId);
            conversionSuccess &= ConvertVarBus<VisAreaComponentRequestBus, bool>("DisplayFilled", varBlock, &VisAreaComponentRequestBus::Events::SetDisplayFilled, newEntityId);
            conversionSuccess &= ConvertVarBus<VisAreaComponentRequestBus, bool>("AffectedBySun", varBlock, &VisAreaComponentRequestBus::Events::SetAffectedBySun, newEntityId);
            conversionSuccess &= ConvertVarBus<VisAreaComponentRequestBus, float>("ViewDistRatio", varBlock, &VisAreaComponentRequestBus::Events::SetViewDistRatio, newEntityId);
            conversionSuccess &= ConvertVarBus<VisAreaComponentRequestBus, bool>("OceanIsVisible", varBlock, &VisAreaComponentRequestBus::Events::SetOceanIsVisible, newEntityId);
        }

        if (!conversionSuccess)
        {
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        //Apply the height mod to the vis area height
        float height = 0.0f;
        VisAreaComponentRequestBus::EventResult(height, newEntityId, &VisAreaComponentRequestBus::Events::GetHeight);
        height += heightMod;
        VisAreaComponentRequestBus::Event(newEntityId, &VisAreaComponentRequestBus::Events::SetHeight, height);

        //Set the points on the component
        VisAreaComponentRequestBus::Event(newEntityId, &VisAreaComponentRequestBus::Events::SetVertices, visAreaPoints);

        //Offset the transform to make sure that the entity's transform is brought down to the lowest point
        //Only need to do this if the first point is not the lowest
        if (!firstPointIsLowest)
        {
            worldPosition.SetZ(worldPosition.GetZ() - (firstPointHeight - minZ));
        }

        //Remove all rotation and scale from the entity
        worldTransform = AZ::Transform::CreateIdentity();
        worldTransform.SetTranslation(worldPosition);

        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTM, worldTransform);

        //Finally, we can delete the old entity since we handled this completely!
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, result.GetValue()->GetId());
        return AZ::LegacyConversion::LegacyConversionResult::HandledDeleteEntity;
    }

    bool VisAreaConverter::BeforeConversionBegins()
    {
        return true;
    }

    bool VisAreaConverter::AfterConversionEnds()
    {
        return true;
    }
} //namespace Visibility