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
#include "EditorOccluderAreaComponent.h"
#include "EditorOccluderAreaComponentMode.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzToolsFramework/Viewport/VertexContainerDisplay.h>
#include <Editor/Objects/BaseObject.h>
#include <Editor/Objects/VisAreaShapeObject.h>
#include <Objects/EntityObject.h>

// Include files needed for writing DisplayEntity functions that access the DisplayContext directly.
#include <EditorCoreAPI.h>

#include <LegacyEntityConversion/LegacyEntityConversion.h>
#include "MathConversion.h"

namespace Visibility
{
    void EditorOccluderAreaComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides)
    {
        provides.push_back(AZ_CRC("EditorOccluderAreaService", 0xf943e16a));
        provides.push_back(AZ_CRC("OccluderAreaService", 0x2fefad66));
        provides.push_back(AZ_CRC("FixedVertexContainerService", 0x83f1bbf2));
    }

    void EditorOccluderAreaComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires)
    {
        requires.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    void EditorOccluderAreaComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC("QuadShapeService", 0xe449b0fc));
    }

    void EditorOccluderAreaComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("EditorOccluderAreaService", 0xf943e16a));
        incompatible.push_back(AZ_CRC("OccluderAreaService", 0x2fefad66));
        incompatible.push_back(AZ_CRC("FixedVertexContainerService", 0x83f1bbf2));
    }

    void EditorOccluderAreaConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorOccluderAreaConfiguration, OccluderAreaConfiguration>()
                ->Version(1);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorOccluderAreaConfiguration>("OccluderArea Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editContext->Class<OccluderAreaConfiguration>("OccluderArea Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OccluderAreaConfiguration::m_displayFilled, "DisplayFilled", "Display the Occlude Area as a filled quad.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &OccluderAreaConfiguration::OnChange)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OccluderAreaConfiguration::m_cullDistRatio, "CullDistRatio", "The range of the culling effect.")
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &OccluderAreaConfiguration::OnChange)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OccluderAreaConfiguration::m_useInIndoors, "UseInIndoors", "Should this occluder work inside VisAreas.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &OccluderAreaConfiguration::OnChange)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OccluderAreaConfiguration::m_doubleSide, "DoubleSide", "Should this occlude from both sides.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &OccluderAreaConfiguration::OnChange)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OccluderAreaConfiguration::m_vertices, "Vertices", "Points that make up the OccluderArea.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &OccluderAreaConfiguration::OnVerticesChange)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
            }
        }
    }

    void EditorOccluderAreaComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorOccluderAreaComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(2)
                ->Field("m_config", &EditorOccluderAreaComponent::m_config)
                ->Field("ComponentMode", &EditorOccluderAreaComponent::m_componentModeDelegate)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorOccluderAreaComponent>("OccluderArea", "An area that blocks objects behind it from rendering.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Rendering")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/OccluderArea.png")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/OccluderArea.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/console/lumberyard/userguide/occluder-area-component")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorOccluderAreaComponent::m_config, "m_config", "No Description")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorOccluderAreaComponent::m_componentModeDelegate, "Component Mode", "OccluderArea Component Mode")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<EditorOccluderAreaRequestBus>("EditorOccluderAreaRequestBus")
                ->Event("SetDisplayFilled", &EditorOccluderAreaRequestBus::Events::SetDisplayFilled)
                ->Event("GetDisplayFilled", &EditorOccluderAreaRequestBus::Events::GetDisplayFilled)
                ->VirtualProperty("DisplayFilled", "GetDisplayFilled", "SetDisplayFilled")

                ->Event("SetCullDistRatio", &EditorOccluderAreaRequestBus::Events::SetCullDistRatio)
                ->Event("GetCullDistRatio", &EditorOccluderAreaRequestBus::Events::GetCullDistRatio)
                ->VirtualProperty("CullDistRatio", "GetCullDistRatio", "SetCullDistRatio")

                ->Event("SetUseInIndoors", &EditorOccluderAreaRequestBus::Events::SetUseInIndoors)
                ->Event("GetUseInIndoors", &EditorOccluderAreaRequestBus::Events::GetUseInIndoors)
                ->VirtualProperty("UseInIndoors", "GetUseInIndoors", "SetUseInIndoors")

                ->Event("SetDoubleSide", &EditorOccluderAreaRequestBus::Events::SetDoubleSide)
                ->Event("GetDoubleSide", &EditorOccluderAreaRequestBus::Events::GetDoubleSide)
                ->VirtualProperty("DoubleSide", "GetDoubleSide", "SetDoubleSide")
                ;

            behaviorContext->Class<EditorOccluderAreaComponent>()->RequestBus("EditorOccluderAreaRequestBus");
        }

        EditorOccluderAreaConfiguration::Reflect(context);
    }

    void EditorOccluderAreaConfiguration::OnChange()
    {
        EditorOccluderAreaRequestBus::Event(m_entityId, &EditorOccluderAreaRequests::UpdateOccluderAreaObject);
    }

    void EditorOccluderAreaConfiguration::OnVerticesChange()
    {
        EditorOccluderAreaRequestBus::Event(
            m_entityId, &EditorOccluderAreaRequests::UpdateOccluderAreaObject);
        EditorOccluderAreaNotificationBus::Event(
            m_entityId, &EditorOccluderAreaNotifications::OnVerticesChangedInspector);
    }

    void EditorOccluderAreaConfiguration::SetEntityId(const AZ::EntityId entityId)
    {
        m_entityId = entityId;
    }

    EditorOccluderAreaComponent::~EditorOccluderAreaComponent()
    {
        if (m_area)
        {
            GetIEditor()->Get3DEngine()->DeleteVisArea(m_area);
            m_area = nullptr;
        }
    }

    void EditorOccluderAreaComponent::Activate()
    {
        Base::Activate();

        const AZ::EntityId entityId = GetEntityId();
        m_config.SetEntityId(entityId);

#ifndef AZ_TESTS_ENABLED
        // NOTE: We create the vis-area here at activate, but destroy it in the destructor.
        // We have to do this, otherwise the vis-area is not saved into the level.
        // Unfortunately, at this time we cannot create the vis-areas at game runtime.
        // This means that dynamic slices cannot effectively contain vis areas until we fix
        // the core rendering system to allow that.
        const auto visGUID = static_cast<AZ::u64>(entityId);
        m_area = GetIEditor()->Get3DEngine()->CreateVisArea(visGUID);
#endif

        m_componentModeDelegate.ConnectWithSingleComponentMode<
            EditorOccluderAreaComponent, EditorOccluderAreaComponentMode>(
                AZ::EntityComponentIdPair(entityId, GetId()), this);

        EditorOccluderAreaRequestBus::Handler::BusConnect(entityId);
        AZ::FixedVerticesRequestBus<AZ::Vector3>::Handler::BusConnect(entityId);
        AZ::TransformNotificationBus::Handler::BusConnect(entityId);
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(entityId);
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(GetEntityId());

        UpdateOccluderAreaObject();
    }

    void EditorOccluderAreaComponent::Deactivate()
    {
        m_componentModeDelegate.Disconnect();

        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AZ::FixedVerticesRequestBus<AZ::Vector3>::Handler::BusDisconnect();
        EditorOccluderAreaRequestBus::Handler::BusDisconnect();

        Base::Deactivate();
    }

    /// Update the object runtime after changes to the Configuration.
    /// Called by the default RequestBus SetXXX implementations,
    /// and used to initially set up the object the first time the
    /// Configuration are set.
    void EditorOccluderAreaComponent::UpdateOccluderAreaObject()
    {
        if (m_area)
        {
            AZStd::array<Vec3, 4> verts;

            const Matrix34& wtm = AZTransformToLYTransform(GetWorldTM());
            for (size_t i = 0; i < m_config.m_vertices.size(); ++i)
            {
                verts[i] = wtm.TransformPoint(AZVec3ToLYVec3(m_config.m_vertices[i]));
            }

            SVisAreaInfo info;
            info.fHeight = 0;
            info.vAmbientColor = Vec3(0, 0, 0);
            info.bAffectedByOutLights = false;
            info.bSkyOnly = false;
            info.fViewDistRatio = m_config.m_cullDistRatio;
            info.bDoubleSide = m_config.m_doubleSide;
            info.bUseDeepness = false;
            info.bUseInIndoors = m_config.m_useInIndoors;
            info.bOceanIsVisible = false;
            info.fPortalBlending = -1.0f;

            const AZStd::string name = AZStd::string("OcclArea_") + GetEntity()->GetName();
            GetIEditor()->Get3DEngine()->UpdateVisArea(m_area, &verts[0], verts.size(), name.c_str(), info, false);
        }
    }

    void EditorOccluderAreaComponent::SetDisplayFilled(const bool value)
    {
        m_config.m_displayFilled = value;
        UpdateOccluderAreaObject();
    }

    bool EditorOccluderAreaComponent::GetDisplayFilled()
    {
        return m_config.m_displayFilled;
    }

    void EditorOccluderAreaComponent::SetCullDistRatio(const float value)
    {
        m_config.m_cullDistRatio = value;
        UpdateOccluderAreaObject();
    }

    float EditorOccluderAreaComponent::GetCullDistRatio()
    {
        return m_config.m_cullDistRatio;
    }

    void EditorOccluderAreaComponent::SetUseInIndoors(const bool value)
    {
        m_config.m_useInIndoors = value;
        UpdateOccluderAreaObject();
    }

    bool EditorOccluderAreaComponent::GetUseInIndoors()
    {
        return m_config.m_useInIndoors;
    }

    void EditorOccluderAreaComponent::SetDoubleSide(const bool value)
    {
        m_config.m_doubleSide = value;
        UpdateOccluderAreaObject();
    }

    bool EditorOccluderAreaComponent::GetDoubleSide()
    {
        return m_config.m_doubleSide;
    }

    bool EditorOccluderAreaComponent::GetVertex(const size_t index, AZ::Vector3& vertex) const
    {
        if (index < m_config.m_vertices.size())
        {
            vertex = m_config.m_vertices[index];
            return true;
        }

        return false;
    }

    bool EditorOccluderAreaComponent::UpdateVertex(const size_t index, const AZ::Vector3& vertex)
    {
        if (index < m_config.m_vertices.size())
        {
            m_config.m_vertices[index] = vertex;
            return true;
        }

        return false;
    }

    void EditorOccluderAreaComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        UpdateOccluderAreaObject();
    }

    void EditorOccluderAreaComponent::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        const AZ::Transform worldFromLocal = GetWorldTM();
        const AZ::Vector4 color(0.5f, 0.25f, 0.0f, 1.0f);
        const AZ::Vector4 selectedColor(1.0f, 0.5f, 0.0f, 1.0f);
        const float previousLineWidth = debugDisplay.GetLineWidth();

        debugDisplay.DepthWriteOff();
        debugDisplay.PushMatrix(worldFromLocal);
        debugDisplay.SetColor(IsSelected() ? selectedColor : color);
        debugDisplay.SetLineWidth(5.0f);
        debugDisplay.SetAlpha(0.8f);

        for (size_t i = 2; i < 4; i++)
        {
            // draw the plane
            if (m_config.m_displayFilled)
            {
                debugDisplay.SetAlpha(0.3f);
                debugDisplay.CullOff();
                debugDisplay.DrawTri(m_config.m_vertices[0], m_config.m_vertices[i - 1], m_config.m_vertices[i]);
                debugDisplay.CullOn();
                debugDisplay.SetAlpha(0.8f);
            }

            debugDisplay.DrawLine(m_config.m_vertices[i - 2], m_config.m_vertices[i - 1]);
            debugDisplay.DrawLine(m_config.m_vertices[i - 1], m_config.m_vertices[i]);
        }

        // draw the closing line
        debugDisplay.DrawLine(m_config.m_vertices[3], m_config.m_vertices[0]);

        if (m_componentModeDelegate.AddedToComponentMode())
        {
            AzToolsFramework::VertexContainerDisplay::DisplayVertexContainerIndices(
                debugDisplay, AzToolsFramework::FixedVerticesArray<AZ::Vector3, 4>(m_config.m_vertices),
                GetWorldTM(), IsSelected());
        }

        debugDisplay.DepthWriteOn();
        debugDisplay.SetLineWidth(previousLineWidth);
        debugDisplay.PopMatrix();
    }

    void EditorOccluderAreaComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<OccluderAreaComponent>(m_config);
    }

    AZ::Aabb EditorOccluderAreaComponent::GetEditorSelectionBoundsViewport(
        const AzFramework::ViewportInfo& /*viewportInfo*/)
    {
        AZ::Aabb bbox = AZ::Aabb::CreateNull();
        for (const auto& vertex : m_config.m_vertices)
        {
            bbox.AddPoint(vertex);
        }
        bbox.ApplyTransform(GetWorldTM());
        return bbox;
    }

    bool EditorOccluderAreaComponent::EditorSelectionIntersectRayViewport(
        const AzFramework::ViewportInfo& /*viewportInfo*/, const AZ::Vector3& src,
        const AZ::Vector3& dir, AZ::VectorFloat& distance)
    {
        const AZ::VectorFloat rayLength = AZ::VectorFloat(1000.0f);
        AZ::Vector3 scaledDir = dir * rayLength;
        AZ::Vector3 end = src + scaledDir;
        AZ::VectorFloat t;
        AZ::VectorFloat intermediateT = FLT_MAX;
        bool didHit = false;

        // Transform verts to world space for tris test
        AZStd::array<AZ::Vector3, 4> verts;
        const AZ::Transform& wtm = GetWorldTM();
        for (size_t i = 0; i < m_config.m_vertices.size(); ++i)
        {
            verts[i] = wtm * m_config.m_vertices[i];
        }

        AZ::Vector3 normal;
        for (AZ::u8 i = 2; i < 4; ++i)
        {
            if (AZ::Intersect::IntersectSegmentTriangleCCW(src, end, verts[0], verts[i - 1], verts[i], normal, t) > 0)
            {
                intermediateT = AZStd::GetMin(t, intermediateT);
                didHit = true;
            }
            //Else if here as we shouldn't successfully ccw and cw intersect at the same time
            else if (AZ::Intersect::IntersectSegmentTriangle(src, end, verts[0], verts[i - 1], verts[i], normal, t) > 0)
            {
                intermediateT = AZStd::GetMin(t, intermediateT);
                didHit = true;
            }
        }

        if (didHit)
        {
            distance = AZ::VectorFloat(intermediateT) * rayLength;
        }
        return didHit;
    }

    AZ::LegacyConversion::LegacyConversionResult OccluderAreaConverter::ConvertEntity(CBaseObject* entityToConvert)
    {
        if ((AZStd::string(entityToConvert->metaObject()->className()) == "COccluderAreaObject"
            && AZStd::string(entityToConvert->metaObject()->className()) != "CEntityObject")
            || (entityToConvert->inherits("CEntityObject")
                && static_cast<CEntityObject *>(entityToConvert)->GetEntityClass() == "OccluderArea"))
        {
            // Have a OccluderArea.
            return ConvertArea(entityToConvert);

        }
        else if ((AZStd::string(entityToConvert->metaObject()->className()) == "COccluderPlaneObject"
            && AZStd::string(entityToConvert->metaObject()->className()) != "CEntityObject")
            || (entityToConvert->inherits("CEntityObject")
                && static_cast<CEntityObject *>(entityToConvert)->GetEntityClass() == "OccluderPlane"))
        {
            // Have a OccluderPlane.
            return ConvertPlane(entityToConvert);
        }
        else
        {
            return AZ::LegacyConversion::LegacyConversionResult::Ignored;
        }
    }

    AZ::LegacyConversion::LegacyConversionResult OccluderAreaConverter::ConvertArea(CBaseObject* entityToConvert)
    {
        if (!((AZStd::string(entityToConvert->metaObject()->className()) == "COccluderAreaObject"
            && AZStd::string(entityToConvert->metaObject()->className())  != "CEntityObject")
            || (entityToConvert->inherits("CEntityObject")
                && static_cast<CEntityObject *>(entityToConvert)->GetEntityClass() == "OccluderArea")))
        {
            // We don't know how to convert this entity, whatever it is
            return AZ::LegacyConversion::LegacyConversionResult::Ignored;
        }
        // Have a OccluderArea.

        // Step 1, create an entity with no components.

        // if we have a parent, and the parent is not yet converted, we ignore this entity!
        // this is not ALWAYS the case - there may be types you wish to create anyway and just put them in the world, so we cannot enforce this
        // inside the CreateConvertedEntity function.
        AZ::Outcome<AZ::Entity*, AZ::LegacyConversion::CreateEntityResult> result(nullptr);

        const AZ::ComponentTypeList componentsToAdd {};
        AZ::LegacyConversion::LegacyConversionRequestBus::BroadcastResult(result, &AZ::LegacyConversion::LegacyConversionRequests::CreateConvertedEntity, entityToConvert, true, componentsToAdd);

        if (!result.IsSuccess())
        {
            // if we failed due to no parent, we keep processing
            return (result.GetError() == AZ::LegacyConversion::CreateEntityResult::FailedNoParent) ? AZ::LegacyConversion::LegacyConversionResult::Ignored : AZ::LegacyConversion::LegacyConversionResult::Failed;
        }
        AZ::Entity* entity = result.GetValue();
        entity->Deactivate();

        auto component = entity->CreateComponent<EditorOccluderAreaComponent>();

        if (!component)
        {
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        // Step 2: extract the parameter data from the underlying CBaseObject into
        // the component.

        //Turn the entity back on
        //Otherwise all the next bus calls will fail
        entity->Activate();

        //Now we can take all the params from the previous entity and shove them into the
        //config for this new component.
        bool conversionSuccess = false;

        const AZ::EntityId newEntityId = entity->GetId();

        const auto* occluderEntity = static_cast<const COccluderAreaObject*>(entityToConvert);
        if (const CVarBlock* varBlock = occluderEntity->GetVarBlock())
        {
            using namespace AZ::LegacyConversion::Util;

            conversionSuccess = true;

            conversionSuccess &= ConvertVarBus<EditorOccluderAreaRequestBus, bool>("DisplayFilled", varBlock, &EditorOccluderAreaRequestBus::Events::SetDisplayFilled, newEntityId);
            conversionSuccess &= ConvertVarBus<EditorOccluderAreaRequestBus, float>("CullDistRatio", varBlock, &EditorOccluderAreaRequestBus::Events::SetCullDistRatio, newEntityId);
            conversionSuccess &= ConvertVarBus<EditorOccluderAreaRequestBus, bool>("UseInIndoors", varBlock, &EditorOccluderAreaRequestBus::Events::SetUseInIndoors, newEntityId);
        }

        if (!conversionSuccess)
        {
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        //Tell the editor component to update vertices, since EBus is connected after the entity is activated.
        for (int i = 0; i < occluderEntity->GetPointCount(); ++i)
        {
            component->m_config.m_vertices[i] = LYVec3ToAZVec3(occluderEntity->GetPoint(i));
        }

        // Call Update to redraw the shape
        component->UpdateOccluderAreaObject();

        // finally, we can delete the old entity since we handled this completely!
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, result.GetValue()->GetId());
        return AZ::LegacyConversion::LegacyConversionResult::HandledDeleteEntity;
    }

    AZ::LegacyConversion::LegacyConversionResult OccluderAreaConverter::ConvertPlane(CBaseObject* entityToConvert)
    {
        // Step 1, create an entity with no components.

        // if we have a parent, and the parent is not yet converted, we ignore this entity!
        // this is not ALWAYS the case - there may be types you wish to create anyway and just put them in the world, so we cannot enforce this
        // inside the CreateConvertedEntity function.
        AZ::Outcome<AZ::Entity*, AZ::LegacyConversion::CreateEntityResult> result(nullptr);

        AZ::ComponentTypeList componentsToAdd {};
        AZ::LegacyConversion::LegacyConversionRequestBus::BroadcastResult(result, &AZ::LegacyConversion::LegacyConversionRequests::CreateConvertedEntity, entityToConvert, true, AZ::ComponentTypeList(componentsToAdd));

        if (!result.IsSuccess())
        {
            // if we failed due to no parent, we keep processing
            return (result.GetError() == AZ::LegacyConversion::CreateEntityResult::FailedNoParent) ? AZ::LegacyConversion::LegacyConversionResult::Ignored : AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        AZ::Entity* entity = result.GetValue();
        entity->Deactivate();

        auto component = entity->CreateComponent<EditorOccluderAreaComponent>();

        if (!component)
        {
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        // Step 2: extract the parameter data from the underlying CBaseObject into
        // the component.

        //Turn the entity back on
        //Otherwise all the next bus calls will fail
        entity->Activate();

        //Now we can take all the params from the previous entity and shove them into the
        //config for this new component.
        bool conversionSuccess = false;

        const AZ::EntityId newEntityId = entity->GetId();

        const auto* occluderEntity = static_cast<const COccluderPlaneObject*>(entityToConvert);
        const float planeHeight = occluderEntity->GetHeight();

        if (const CVarBlock* varBlock = occluderEntity->GetVarBlock())
        {
            using namespace AZ::LegacyConversion::Util;

            conversionSuccess = true;

            conversionSuccess &= ConvertVarBus<EditorOccluderAreaRequestBus, bool>("DisplayFilled", varBlock, &EditorOccluderAreaRequestBus::Events::SetDisplayFilled, newEntityId);
            conversionSuccess &= ConvertVarBus<EditorOccluderAreaRequestBus, float>("CullDistRatio", varBlock, &EditorOccluderAreaRequestBus::Events::SetCullDistRatio, newEntityId);
            conversionSuccess &= ConvertVarBus<EditorOccluderAreaRequestBus, bool>("UseInIndoors", varBlock, &EditorOccluderAreaRequestBus::Events::SetUseInIndoors, newEntityId);
            conversionSuccess &= ConvertVarBus<EditorOccluderAreaRequestBus, bool>("DoubleSide", varBlock, &EditorOccluderAreaRequestBus::Events::SetDoubleSide, newEntityId);
        }

        if (!conversionSuccess)
        {
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        //Tell the editor component to update vertices, since EBus is connected after the entity is activated.

        const int vertexCount = occluderEntity->GetPointCount();
        AZ_Assert(vertexCount == 2, "There Should be 2 points for Occlude Plane");

        /// The way CryEngine is doing is that using the 2 vertices and add height to these vertices to create the other 2 vertices

        //The plane has two points but we need 4 for the occluder area
        AZStd::fixed_vector<AZ::Vector3, 4> vertices(4);
        AZ::Transform transform = component->GetLocalTM();
        const AZ::Quaternion originQuat = AZ::Quaternion::CreateFromTransform(transform);
        for (int i = 0; i < vertexCount; i++)
        {
            //Get the actual vertex position based on the local rotation
            const AZ::Vector3 point = originQuat * LYVec3ToAZVec3(occluderEntity->GetPoint(i));
            vertices[i] = point;

            //The legacy plane only has 2 points so we need to generate the other two points in the quad
            // On point 0 we generate point 3
            // On point 1 we generate point 2
            const size_t generatedPointIndex = vertexCount * 2 - 1 - i;

            //get the generated vertex position based on the plane height
            const AZ::Vector3 generatedPoint = vertices[i] + AZ::Vector3(0, 0, planeHeight);
            vertices[generatedPointIndex] = generatedPoint;
        }

        for (size_t i = 0; i < vertices.size(); ++i)
        {
            component->m_config.m_vertices[i] = vertices[i];
        }

        // Remove the rotation of the entity, since it has been applied to the points
        transform.SetRotationPartFromQuaternion(AZ::Quaternion::CreateIdentity());
        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetLocalTM, transform);

        // Call Update to redraw the shape
        component->UpdateOccluderAreaObject();

        // finally, we can delete the old entity since we handled this completely!
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, result.GetValue()->GetId());
        return AZ::LegacyConversion::LegacyConversionResult::HandledDeleteEntity;
    }

    bool OccluderAreaConverter::BeforeConversionBegins()
    {
        return true;
    }

    bool OccluderAreaConverter::AfterConversionEnds()
    {
        return true;
    }
} // namespace Visibility
