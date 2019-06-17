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
#include "EditorPortalComponent.h"
#include "EditorPortalComponentMode.h"

#include <AzCore/RTTI/BehaviorContext.h>

// Include files needed for writing DisplayEntity functions that access the DisplayContext directly.
#include <EditorCoreAPI.h>

#include <AzCore/Math/Crc.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzFramework/Math/MathUtils.h>
#include <AzToolsFramework/Viewport/VertexContainerDisplay.h>
#include <Editor/Objects/BaseObject.h>
#include <Editor/Objects/VisAreaShapeObject.h>
#include <LegacyEntityConversion/LegacyEntityConversion.h>
#include <MathConversion.h>

namespace Visibility
{
    void EditorPortalComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("EditorPortalService", 0x6ead38f6));
        provided.push_back(AZ_CRC("PortalService", 0x06076210));
        provided.push_back(AZ_CRC("FixedVertexContainerService", 0x83f1bbf2));
    }

    void EditorPortalComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    void EditorPortalComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC("QuadShapeService", 0xe449b0fc));
    }

    void EditorPortalComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("SphereShapeService", 0x90c8dc80));
        incompatible.push_back(AZ_CRC("SplineShapeService", 0x4d4b94a2));
        incompatible.push_back(AZ_CRC("PolygonPrismShapeService", 0x1cbc4ed4));
        incompatible.push_back(AZ_CRC("FixedVertexContainerService", 0x83f1bbf2));
    }

    void EditorPortalConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorPortalConfiguration, PortalConfiguration>()
                ->Version(1);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorPortalConfiguration>("Portal Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editContext->Class<PortalConfiguration>("Portal Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &PortalConfiguration::m_height, "Height", "How tall the Portal is.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PortalConfiguration::OnChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &PortalConfiguration::m_displayFilled, "DisplayFilled", "Display the Portal as a filled volume.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PortalConfiguration::OnChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &PortalConfiguration::m_affectedBySun, "AffectedBySun", "Allows sunlight to affect objects inside the Portal.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PortalConfiguration::OnChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &PortalConfiguration::m_viewDistRatio, "ViewDistRatio", "Specifies how far the Portal is rendered.")
                    ->Attribute(AZ::Edit::Attributes::Max, 100.000000)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.000000)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PortalConfiguration::OnChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &PortalConfiguration::m_skyOnly, "SkyOnly", "Only the Sky Box will render when looking outside the Portal.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PortalConfiguration::OnChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &PortalConfiguration::m_oceanIsVisible, "OceanIsVisible", "Ocean will be visible when looking outside the Portal.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PortalConfiguration::OnChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &PortalConfiguration::m_useDeepness, "UseDeepness", "Portal will be treated as an object with volume rather than a plane.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PortalConfiguration::OnChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &PortalConfiguration::m_doubleSide, "DoubleSide", "Cameras will be able to look through the portal from both sides.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PortalConfiguration::OnChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &PortalConfiguration::m_lightBlending, "LightBlending", "Light from neighboring VisAreas will blend into the Portal.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PortalConfiguration::OnChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &PortalConfiguration::m_lightBlendValue, "LightBlendValue", "How much to blend lights from neighboring VisAreas.")
                    ->Attribute(AZ::Edit::Attributes::Max, 1.000000)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.000000)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PortalConfiguration::OnChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &PortalConfiguration::m_vertices, "Vertices", "Points that make up the floor of the Portal.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PortalConfiguration::OnVerticesChange)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ;
            }
        }
    }

    void EditorPortalComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorPortalComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(2)
                ->Field("m_config", &EditorPortalComponent::m_config)
                ->Field("ComponentMode", &EditorPortalComponent::m_componentModeDelegate)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorPortalComponent>("Portal", "An area that describes a visibility portal between VisAreas.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Rendering")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Portal.png")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Portal.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/console/lumberyard/userguide/portal-component")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorPortalComponent::m_config, "m_config", "No Description")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorPortalComponent::m_componentModeDelegate, "Component Mode", "Portal Component Mode")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<EditorPortalRequestBus>("EditorPortalRequestBus")
                ->Event("SetHeight", &EditorPortalRequestBus::Events::SetHeight)
                ->Event("GetHeight", &EditorPortalRequestBus::Events::GetHeight)
                ->VirtualProperty("Height", "GetHeight", "SetHeight")

                ->Event("SetDisplayFilled", &EditorPortalRequestBus::Events::SetDisplayFilled)
                ->Event("GetDisplayFilled", &EditorPortalRequestBus::Events::GetDisplayFilled)
                ->VirtualProperty("DisplayFilled", "GetDisplayFilled", "SetDisplayFilled")

                ->Event("SetAffectedBySun", &EditorPortalRequestBus::Events::SetAffectedBySun)
                ->Event("GetAffectedBySun", &EditorPortalRequestBus::Events::GetAffectedBySun)
                ->VirtualProperty("AffectedBySun", "GetAffectedBySun", "SetAffectedBySun")

                ->Event("SetViewDistRatio", &EditorPortalRequestBus::Events::SetViewDistRatio)
                ->Event("GetViewDistRatio", &EditorPortalRequestBus::Events::GetViewDistRatio)
                ->VirtualProperty("ViewDistRatio", "GetViewDistRatio", "SetViewDistRatio")

                ->Event("SetSkyOnly", &EditorPortalRequestBus::Events::SetSkyOnly)
                ->Event("GetSkyOnly", &EditorPortalRequestBus::Events::GetSkyOnly)
                ->VirtualProperty("SkyOnly", "GetSkyOnly", "SetSkyOnly")

                ->Event("SetOceanIsVisible", &EditorPortalRequestBus::Events::SetOceanIsVisible)
                ->Event("GetOceanIsVisible", &EditorPortalRequestBus::Events::GetOceanIsVisible)
                ->VirtualProperty("OceanIsVisible", "GetOceanIsVisible", "SetOceanIsVisible")

                ->Event("SetUseDeepness", &EditorPortalRequestBus::Events::SetUseDeepness)
                ->Event("GetUseDeepness", &EditorPortalRequestBus::Events::GetUseDeepness)
                ->VirtualProperty("UseDeepness", "GetUseDeepness", "SetUseDeepness")

                ->Event("SetDoubleSide", &EditorPortalRequestBus::Events::SetDoubleSide)
                ->Event("GetDoubleSide", &EditorPortalRequestBus::Events::GetDoubleSide)
                ->VirtualProperty("DoubleSide", "GetDoubleSide", "SetDoubleSide")

                ->Event("SetLightBlending", &EditorPortalRequestBus::Events::SetLightBlending)
                ->Event("GetLightBlending", &EditorPortalRequestBus::Events::GetLightBlending)
                ->VirtualProperty("LightBlending", "GetLightBlending", "SetLightBlending")

                ->Event("SetLightBlendValue", &EditorPortalRequestBus::Events::SetLightBlendValue)
                ->Event("GetLightBlendValue", &EditorPortalRequestBus::Events::GetLightBlendValue)
                ->VirtualProperty("LightBlendValue", "GetLightBlendValue", "SetLightBlendValue")
                ;

            behaviorContext->Class<EditorPortalComponent>()->RequestBus("EditorPortalRequestBus");
        }

        EditorPortalConfiguration::Reflect(context);
    }

    void EditorPortalConfiguration::OnChange()
    {
        EditorPortalRequestBus::Event(m_entityId, &EditorPortalRequests::UpdatePortalObject);
    }

    void EditorPortalConfiguration::OnVerticesChange()
    {
        EditorPortalRequestBus::Event(m_entityId, &EditorPortalRequests::UpdatePortalObject);
        EditorPortalNotificationBus::Event(m_entityId, &EditorPortalNotifications::OnVerticesChangedInspector);
    }

    void EditorPortalConfiguration::SetEntityId(const AZ::EntityId entityId)
    {
        m_entityId = entityId;
    }

    EditorPortalComponent::~EditorPortalComponent()
    {
        if (m_area)
        {
            // reset the listener vis area in the unlucky case that we are deleting the
            // vis area where the listener is currently in
            // Audio: do we still need this?
            GetIEditor()->Get3DEngine()->DeleteVisArea(m_area);
            m_area = nullptr;
        }
    }

    void EditorPortalComponent::Activate()
    {
        Base::Activate();

        const AZ::EntityId entityId = GetEntityId();
        m_config.SetEntityId(entityId);

#ifndef AZ_TESTS_ENABLED
        // NOTE: We create the vis-area here at activated, but destroy it in the destructor.
        // We have to do this, otherwise the vis-area is not saved into the level.
        // Unfortunately, at this time we cannot create the vis-areas at game runtime.
        // This means that dynamic slices cannot effectively contain vis-areas until we fix the core rendering system to allow that.

        const auto visGUID = static_cast<AZ::u64>(entityId);
        m_area = GetIEditor()->Get3DEngine()->CreateVisArea(visGUID);
#endif

        m_AZCachedWorldTransform = AZ::Transform::CreateIdentity();
        m_cryCachedWorldTransform = Matrix34::CreateIdentity();

        m_componentModeDelegate.ConnectWithSingleComponentMode<
            EditorPortalComponent, EditorPortalComponentMode>(
                AZ::EntityComponentIdPair(entityId, GetId()), this);

        EditorPortalRequestBus::Handler::BusConnect(entityId);
        AZ::FixedVerticesRequestBus<AZ::Vector3>::Handler::BusConnect(entityId);
        AZ::TransformNotificationBus::Handler::BusConnect(entityId);
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(entityId);
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(GetEntityId());

        //Call OnTransformChanged manually to cache current transform since it won't be called
        //automatically for us when the level starts up.
        AZ::Transform worldTM;
        AZ::TransformBus::EventResult(worldTM, entityId, &AZ::TransformBus::Events::GetWorldTM);

        //Use an identity transform for localTM because the
        //OnTransformChanged impl for this class doesn't need it
        OnTransformChanged(AZ::Transform::CreateIdentity(), worldTM);
    }

    void EditorPortalComponent::Deactivate()
    {
        m_componentModeDelegate.Disconnect();

        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusDisconnect(GetEntityId());
        AZ::FixedVerticesRequestBus<AZ::Vector3>::Handler::BusDisconnect();
        EditorPortalRequestBus::Handler::BusDisconnect();

        Base::Deactivate();
    }

    void EditorPortalComponent::SetHeight(const float height)
    {
        m_config.m_height = height;
        UpdatePortalObject();
    }

    float EditorPortalComponent::GetHeight()
    {
        return m_config.m_height;
    }

    void EditorPortalComponent::SetDisplayFilled(const bool filled)
    {
        m_config.m_displayFilled = filled;
        UpdatePortalObject();
    }

    bool EditorPortalComponent::GetDisplayFilled()
    {
        return m_config.m_displayFilled;
    }

    void EditorPortalComponent::SetAffectedBySun(const bool affectedBySun)
    {
        m_config.m_affectedBySun = affectedBySun;
        UpdatePortalObject();
    }

    bool EditorPortalComponent::GetAffectedBySun()
    {
        return m_config.m_affectedBySun;
    }

    void EditorPortalComponent::SetViewDistRatio(const float viewDistRatio)
    {
        m_config.m_viewDistRatio = viewDistRatio;
        UpdatePortalObject();
    }

    float EditorPortalComponent::GetViewDistRatio()
    {
        return m_config.m_viewDistRatio;
    }

    void EditorPortalComponent::SetSkyOnly(const bool skyOnly)
    {
        m_config.m_skyOnly = skyOnly;
        UpdatePortalObject();
    }

    bool EditorPortalComponent::GetSkyOnly()
    {
        return m_config.m_skyOnly;
    }

    void EditorPortalComponent::SetOceanIsVisible(const bool oceanVisible)
    {
        m_config.m_oceanIsVisible = oceanVisible;
        UpdatePortalObject();
    }

    bool EditorPortalComponent::GetOceanIsVisible()
    {
        return m_config.m_oceanIsVisible;
    }

    void EditorPortalComponent::SetUseDeepness(const bool useDeepness)
    {
        m_config.m_useDeepness = useDeepness;
        UpdatePortalObject();
    }

    bool EditorPortalComponent::GetUseDeepness()
    {
        return m_config.m_useDeepness;
    }

    void EditorPortalComponent::SetDoubleSide(const bool doubleSided)
    {
        m_config.m_doubleSide = doubleSided;
        UpdatePortalObject();
    }

    bool EditorPortalComponent::GetDoubleSide()
    {
        return m_config.m_doubleSide;
    }

    void EditorPortalComponent::SetLightBlending(const bool lightBending)
    {
        m_config.m_lightBlending = lightBending;
        UpdatePortalObject();
    }

    bool EditorPortalComponent::GetLightBlending()
    {
        return m_config.m_lightBlending;
    }

    void EditorPortalComponent::SetLightBlendValue(const float lightBendAmount)
    {
        m_config.m_lightBlendValue = lightBendAmount;
        UpdatePortalObject();
    }

    float EditorPortalComponent::GetLightBlendValue()
    {
        return m_config.m_lightBlendValue;
    }

    bool EditorPortalComponent::GetVertex(const size_t index, AZ::Vector3& vertex) const
    {
        if (index < m_config.m_vertices.size())
        {
            vertex = m_config.m_vertices[index];
            return true;
        }

        return false;
    }

    bool EditorPortalComponent::UpdateVertex(const size_t index, const AZ::Vector3& vertex)
    {
        if (index < m_config.m_vertices.size())
        {
            m_config.m_vertices[index] = vertex;
            return true;
        }

        return false;
    }

    /// Update the object runtime after changes to the Configuration.
    /// Called by the default RequestBus SetXXX implementations,
    /// and used to initially set up the object the first time the
    /// Configuration are set.
    void EditorPortalComponent::UpdatePortalObject()
    {
        if (m_area)
        {
            SVisAreaInfo info;
            info.vAmbientColor = Vec3(ZERO);
            info.bAffectedByOutLights = m_config.m_affectedBySun;
            info.bSkyOnly = m_config.m_skyOnly;
            info.fViewDistRatio = m_config.m_viewDistRatio;
            info.bDoubleSide = m_config.m_doubleSide;
            info.bUseDeepness = m_config.m_useDeepness;
            info.bUseInIndoors = true; //Does not apply to Portals (Portals are only in VisAreas)
            info.bOceanIsVisible = m_config.m_oceanIsVisible;
            info.fPortalBlending = -1.0f;

            if (m_config.m_lightBlending)
            {
                info.fPortalBlending = m_config.m_lightBlendValue;
            }

            AZStd::string name = AZStd::string("Portal_") + GetEntity()->GetName();

            // Calculate scaled height
            // Height exists separate from plane points but we still want to scale it with the transform
            info.fHeight = m_config.m_height;

            /*
               We have to derive at least 3 points and pass them to the vis area system
               For now that means getting the 4 points of the bottom face of the box.

               If we want to send *all* points of a shape the vis system we need to make sure
               that Height is 0; otherwise it'll extend the AABB of the area upwards.
             */

            //Convert to Cry vectors and apply the transform to the given points
            AZStd::fixed_vector<Vec3, 4> verts(4);
            for (AZ::u32 i = 0; i < verts.size(); ++i)
            {
                verts[i] = AZVec3ToLYVec3(m_config.m_vertices[i]);
                verts[i] = m_cryCachedWorldTransform.TransformPoint(verts[i]);
            }

            GetIEditor()->Get3DEngine()->UpdateVisArea(m_area, &verts[0], verts.size(), name.c_str(), info, true);
        }
    }

    void EditorPortalComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        //Cache the transform so that we don't have to retrieve it every time UpdatePortalObject is called
        m_AZCachedWorldTransform = world;
        m_cryCachedWorldTransform = AZTransformToLYTransform(m_AZCachedWorldTransform);

        UpdatePortalObject();
    }

    struct PortalQuadVertices
    {
        AZ::Vector3 floorLeftFront;
        AZ::Vector3 floorRightFront;
        AZ::Vector3 floorLeftBack;
        AZ::Vector3 floorRightBack;

        AZ::Vector3 quadUpperLeftFront;
        AZ::Vector3 quadUpperRightFront;
        AZ::Vector3 quadUpperLeftBack;
        AZ::Vector3 quadUpperRightBack;

        AZ::Vector3 portalUpperLeftFront;
        AZ::Vector3 portalUpperRightFront;
        AZ::Vector3 portalUpperLeftBack;
        AZ::Vector3 portalUpperRightBack;
    };

    PortalQuadVertices EditorPortalComponent::CalculatePortalQuadVertices(VertTranslation vertTranslation)
    {
        PortalQuadVertices pqv;

        //Untransformed quad corners
        const AZ::Vector3 lowerLeftFront = m_config.m_vertices[0];
        const AZ::Vector3 lowerRightFront = m_config.m_vertices[1];
        const AZ::Vector3 lowerLeftBack = m_config.m_vertices[3];
        const AZ::Vector3 lowerRightBack = m_config.m_vertices[2];

        //Need to calculate the height of the quad after transformation
        const AZ::u32 quadPointCount = 4;
        AZ::Vector3 transformedQuadPoints[quadPointCount];

        transformedQuadPoints[0] = m_AZCachedWorldTransform * lowerLeftFront;
        transformedQuadPoints[1] = m_AZCachedWorldTransform * lowerRightFront;
        transformedQuadPoints[2] = m_AZCachedWorldTransform * lowerLeftBack;
        transformedQuadPoints[3] = m_AZCachedWorldTransform * lowerRightBack;

        const AZ::Vector3 translation = m_AZCachedWorldTransform.GetTranslation();

        float minHeight = FLT_MAX;
        float maxHeight = FLT_MIN;

        for (auto& transformedQuadPoint : transformedQuadPoints)
        {
            // remove translation from quad points so we can use them with the DisplayContext's usage of the transform
            if (vertTranslation == VertTranslation::Remove)
            {
                transformedQuadPoint -= translation;
            }

            const float height = transformedQuadPoint.GetZ();
            if (height < minHeight)
            {
                minHeight = height;
            }
            if (height > maxHeight)
            {
                maxHeight = height;
            }
        }

        pqv.floorLeftFront = AZ::Vector3(transformedQuadPoints[0].GetX(), transformedQuadPoints[0].GetY(), minHeight);
        pqv.floorRightFront = AZ::Vector3(transformedQuadPoints[1].GetX(), transformedQuadPoints[1].GetY(), minHeight);
        pqv.floorLeftBack = AZ::Vector3(transformedQuadPoints[2].GetX(), transformedQuadPoints[2].GetY(), minHeight);
        pqv.floorRightBack = AZ::Vector3(transformedQuadPoints[3].GetX(), transformedQuadPoints[3].GetY(), minHeight);

        pqv.quadUpperLeftFront = AZ::Vector3(transformedQuadPoints[0].GetX(), transformedQuadPoints[0].GetY(), maxHeight);
        pqv.quadUpperRightFront = AZ::Vector3(transformedQuadPoints[1].GetX(), transformedQuadPoints[1].GetY(), maxHeight);
        pqv.quadUpperLeftBack = AZ::Vector3(transformedQuadPoints[2].GetX(), transformedQuadPoints[2].GetY(), maxHeight);
        pqv.quadUpperRightBack = AZ::Vector3(transformedQuadPoints[3].GetX(), transformedQuadPoints[3].GetY(), maxHeight);

        pqv.portalUpperLeftFront = AZ::Vector3(transformedQuadPoints[0].GetX(), transformedQuadPoints[0].GetY(), maxHeight + m_config.m_height);
        pqv.portalUpperRightFront = AZ::Vector3(transformedQuadPoints[1].GetX(), transformedQuadPoints[1].GetY(), maxHeight + m_config.m_height);
        pqv.portalUpperLeftBack = AZ::Vector3(transformedQuadPoints[2].GetX(), transformedQuadPoints[2].GetY(), maxHeight + m_config.m_height);
        pqv.portalUpperRightBack = AZ::Vector3(transformedQuadPoints[3].GetX(), transformedQuadPoints[3].GetY(), maxHeight + m_config.m_height);

        return pqv;
    }

    void EditorPortalComponent::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        /*
           IMPORTANT NOTE: This method may seem very complicated but it is an accurate visualization of
           how portals actually work. The legacy visualization used with the legacy portal entity is
           very misleading!

           Portals always exist as a quad but if the quad becomes non-planar, from rotation or in the legacy
           system from a point being pulled up or down, the volume changes in a non-obvious way. Instead of portal
           existing as the shape defined by 4 points and extruded upwards, the portal actually remains planar.
           Any height difference that you add by making the shape non-planar is just applied to the height of the volume.

           If this is confusing, please actually look at the visualization created by this method. Make sure
           that you rotate the portal in many weird contorted ways and examine how the visualization reacts.
           The portal volume is always going to be a box rotated on only X and Y axes that stretches up along the Z axis.

           Important note on the complexity of this method:
           We cannot directly visualize the OBB of the portal with an AABB that we then transform. The OBB that's mentioned
           here is best imagined as the top plane being all points of the quad pulled up to the height of the highest quad's vert
           and the bottom plane being all points of the quad pulled down to the height of the lowest quad's vert. Trying to
           create an AABB from these points won't produce the correct visualization under complex rotations as the Min and Max
           of the AABB will either only encompass part of the bounding volume or be too large.
         */

        const PortalQuadVertices pqv = CalculatePortalQuadVertices(VertTranslation::Remove);

        //Draw the outline of the OBB of the Portal's quad
        AZ::Color color(0.000f, 1.0f, 0.000f, 1.0f);
        debugDisplay.SetColor(AZ::Vector4(color.GetR(), color.GetG(), color.GetB(), 1.f));

        //Remove all rotation from the transform
        const AZ::Quaternion rotation = AZ::Quaternion::CreateIdentity();

        AZ::Transform worldTMOnlyZRot = m_AZCachedWorldTransform;
        worldTMOnlyZRot.SetRotationPartFromQuaternion(rotation);

        debugDisplay.PushMatrix(worldTMOnlyZRot);

        //Draw the outline of the OBB of the portal quad

        //Bottom
        debugDisplay.DrawLine(pqv.floorLeftFront, pqv.floorRightFront);
        debugDisplay.DrawLine(pqv.floorRightFront, pqv.floorRightBack);
        debugDisplay.DrawLine(pqv.floorRightBack, pqv.floorLeftBack);
        debugDisplay.DrawLine(pqv.floorLeftBack, pqv.floorLeftFront);
        //Top
        debugDisplay.DrawLine(pqv.quadUpperLeftFront, pqv.quadUpperRightFront);
        debugDisplay.DrawLine(pqv.quadUpperRightFront, pqv.quadUpperRightBack);
        debugDisplay.DrawLine(pqv.quadUpperRightBack, pqv.quadUpperLeftBack);
        debugDisplay.DrawLine(pqv.quadUpperLeftBack, pqv.quadUpperLeftFront);
        //Left
        debugDisplay.DrawLine(pqv.floorLeftFront, pqv.quadUpperLeftFront);
        debugDisplay.DrawLine(pqv.quadUpperLeftFront, pqv.quadUpperLeftBack);
        debugDisplay.DrawLine(pqv.quadUpperLeftBack, pqv.floorLeftBack);
        debugDisplay.DrawLine(pqv.floorLeftBack, pqv.floorLeftFront);
        //Right
        debugDisplay.DrawLine(pqv.floorRightFront, pqv.quadUpperRightFront);
        debugDisplay.DrawLine(pqv.quadUpperRightFront, pqv.quadUpperRightBack);
        debugDisplay.DrawLine(pqv.quadUpperRightBack, pqv.floorRightBack);
        debugDisplay.DrawLine(pqv.floorRightBack, pqv.floorRightFront);
        //Front
        debugDisplay.DrawLine(pqv.floorLeftFront, pqv.floorRightFront);
        debugDisplay.DrawLine(pqv.floorRightFront, pqv.quadUpperRightFront);
        debugDisplay.DrawLine(pqv.quadUpperRightFront, pqv.quadUpperLeftFront);
        debugDisplay.DrawLine(pqv.quadUpperLeftFront, pqv.floorLeftFront);
        //Back
        debugDisplay.DrawLine(pqv.floorLeftBack, pqv.floorRightBack);
        debugDisplay.DrawLine(pqv.floorRightBack, pqv.quadUpperRightBack);
        debugDisplay.DrawLine(pqv.quadUpperRightBack, pqv.quadUpperLeftBack);
        debugDisplay.DrawLine(pqv.quadUpperLeftBack, pqv.floorLeftBack);

        //Now draw the entire portal volume (Previous OBB + extra height)
        if (m_config.m_displayFilled)
        {
            //Draw whole portal with less alpha
            debugDisplay.SetColor(AZ::Vector4(color.GetR(), color.GetG(), color.GetB(), 0.1f));

            //Draw both winding orders for quads so they appear solid from all angles
            //Not drawing boxes because the corners of the quad may not be hit if the bounds are rotated oddly

            //Bottom
            debugDisplay.DrawQuad(pqv.floorLeftFront, pqv.floorRightFront, pqv.floorRightBack, pqv.floorLeftBack);
            debugDisplay.DrawQuad(pqv.floorLeftFront, pqv.floorLeftBack, pqv.floorRightBack, pqv.floorRightFront);
            //Top
            debugDisplay.DrawQuad(pqv.portalUpperLeftFront, pqv.portalUpperRightFront, pqv.portalUpperRightBack, pqv.portalUpperLeftBack);
            debugDisplay.DrawQuad(pqv.portalUpperLeftFront, pqv.portalUpperLeftBack, pqv.portalUpperRightBack, pqv.portalUpperRightFront);
            //Left
            debugDisplay.DrawQuad(pqv.floorLeftFront, pqv.portalUpperLeftFront, pqv.portalUpperLeftBack, pqv.floorLeftBack);
            debugDisplay.DrawQuad(pqv.floorLeftFront, pqv.floorLeftBack, pqv.portalUpperLeftBack, pqv.portalUpperLeftFront);
            //Right
            debugDisplay.DrawQuad(pqv.floorRightFront, pqv.portalUpperRightFront, pqv.portalUpperRightBack, pqv.floorRightBack);
            debugDisplay.DrawQuad(pqv.floorRightFront, pqv.floorRightBack, pqv.portalUpperRightBack, pqv.portalUpperRightFront);
            //Front
            debugDisplay.DrawQuad(pqv.floorLeftFront, pqv.floorRightFront, pqv.portalUpperRightFront, pqv.portalUpperLeftFront);
            debugDisplay.DrawQuad(pqv.floorLeftFront, pqv.portalUpperLeftFront, pqv.portalUpperRightFront, pqv.floorRightFront);
            //Back
            debugDisplay.DrawQuad(pqv.floorLeftBack, pqv.floorRightBack, pqv.portalUpperRightBack, pqv.portalUpperLeftBack);
            debugDisplay.DrawQuad(pqv.floorLeftBack, pqv.portalUpperLeftBack, pqv.portalUpperRightBack, pqv.floorRightBack);
        }
        else
        {
            //Bottom
            debugDisplay.DrawLine(pqv.floorLeftFront, pqv.floorRightFront);
            debugDisplay.DrawLine(pqv.floorRightFront, pqv.floorRightBack);
            debugDisplay.DrawLine(pqv.floorRightBack, pqv.floorLeftBack);
            debugDisplay.DrawLine(pqv.floorLeftBack, pqv.floorLeftFront);
            //Top
            debugDisplay.DrawLine(pqv.portalUpperLeftFront, pqv.portalUpperRightFront);
            debugDisplay.DrawLine(pqv.portalUpperRightFront, pqv.portalUpperRightBack);
            debugDisplay.DrawLine(pqv.portalUpperRightBack, pqv.portalUpperLeftBack);
            debugDisplay.DrawLine(pqv.portalUpperLeftBack, pqv.portalUpperLeftFront);
            //Left
            debugDisplay.DrawLine(pqv.floorLeftFront, pqv.portalUpperLeftFront);
            debugDisplay.DrawLine(pqv.portalUpperLeftFront, pqv.portalUpperLeftBack);
            debugDisplay.DrawLine(pqv.portalUpperLeftBack, pqv.floorLeftBack);
            debugDisplay.DrawLine(pqv.floorLeftBack, pqv.floorLeftFront);
            //Right
            debugDisplay.DrawLine(pqv.floorRightFront, pqv.portalUpperRightFront);
            debugDisplay.DrawLine(pqv.portalUpperRightFront, pqv.portalUpperRightBack);
            debugDisplay.DrawLine(pqv.portalUpperRightBack, pqv.floorRightBack);
            debugDisplay.DrawLine(pqv.floorRightBack, pqv.floorRightFront);
            //Front
            debugDisplay.DrawLine(pqv.floorLeftFront, pqv.floorRightFront);
            debugDisplay.DrawLine(pqv.floorRightFront, pqv.portalUpperRightFront);
            debugDisplay.DrawLine(pqv.portalUpperRightFront, pqv.portalUpperLeftFront);
            debugDisplay.DrawLine(pqv.portalUpperLeftFront, pqv.floorLeftFront);
            //Back
            debugDisplay.DrawLine(pqv.floorLeftBack, pqv.floorRightBack);
            debugDisplay.DrawLine(pqv.floorRightBack, pqv.portalUpperRightBack);
            debugDisplay.DrawLine(pqv.portalUpperRightBack, pqv.portalUpperLeftBack);
            debugDisplay.DrawLine(pqv.portalUpperLeftBack, pqv.floorLeftBack);
        }

        if (m_componentModeDelegate.AddedToComponentMode())
        {
            AzToolsFramework::VertexContainerDisplay::DisplayVertexContainerIndices(
                debugDisplay, AzToolsFramework::FixedVerticesArray<AZ::Vector3, 4>(m_config.m_vertices),
                GetWorldTM(), IsSelected());
        }

        debugDisplay.PopMatrix();
    }

    void EditorPortalComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<PortalComponent>(m_config);
    }

    AZ::Aabb EditorPortalComponent::GetEditorSelectionBoundsViewport(const AzFramework::ViewportInfo& /*viewportInfo*/)
    {
        const PortalQuadVertices pqv = CalculatePortalQuadVertices(VertTranslation::Keep);

        AZ::Aabb bbox = AZ::Aabb::CreateNull();
        bbox.AddPoint(pqv.floorLeftFront);
        bbox.AddPoint(pqv.floorRightFront);
        bbox.AddPoint(pqv.floorLeftBack);
        bbox.AddPoint(pqv.floorRightBack);
        bbox.AddPoint(pqv.portalUpperLeftFront);
        return bbox;
    }

    bool EditorPortalComponent::EditorSelectionIntersectRayViewport(
        const AzFramework::ViewportInfo& /*viewportInfo*/, const AZ::Vector3& src,
        const AZ::Vector3& dir, AZ::VectorFloat& distance)
    {
        float t;
        float intermediateT = FLT_MAX;

        const PortalQuadVertices pqv = CalculatePortalQuadVertices(VertTranslation::Keep);

        //Count each quad for intersection hits, two hits implies we are intersecting the prism from outside of it (or from too far)
        AZ::u8 hits = 0;

        //Bottom
        if (AZ::Intersect::IntersectRayQuad(src, dir, pqv.floorLeftFront, pqv.floorRightFront, pqv.floorRightBack, pqv.floorLeftBack, t))
        { 
            ++hits; 
            intermediateT = AZStd::GetMin(t, intermediateT);
        }
        //Top
        if (AZ::Intersect::IntersectRayQuad(src, dir, pqv.portalUpperLeftFront, pqv.portalUpperRightFront, pqv.portalUpperRightBack, pqv.portalUpperLeftBack, t))
        { 
            ++hits; 
            intermediateT = AZStd::GetMin(t, intermediateT);
        }

        //Left
        if (AZ::Intersect::IntersectRayQuad(src, dir, pqv.floorLeftFront, pqv.portalUpperLeftFront, pqv.portalUpperLeftBack, pqv.floorLeftBack, t))
        { 
            ++hits;
            intermediateT = AZStd::GetMin(t, intermediateT);
        }
        //Right
        if (AZ::Intersect::IntersectRayQuad(src, dir, pqv.floorRightFront, pqv.portalUpperRightFront, pqv.portalUpperRightBack, pqv.floorRightBack, t))
        { 
            ++hits;
            intermediateT = AZStd::GetMin(t, intermediateT);
        }
        //Front
        if (AZ::Intersect::IntersectRayQuad(src, dir, pqv.floorLeftFront, pqv.floorRightFront, pqv.portalUpperRightFront, pqv.portalUpperLeftFront, t))
        { 
            ++hits;
            intermediateT = AZStd::GetMin(t, intermediateT);
        }
        //Back
        if (AZ::Intersect::IntersectRayQuad(src, dir, pqv.floorLeftBack, pqv.floorRightBack, pqv.portalUpperRightBack, pqv.portalUpperLeftBack, t)) 
        { 
            ++hits;
            intermediateT = AZStd::GetMin(t, intermediateT);
        }

        if (hits > 0)
        {
            distance = AZ::VectorFloat(intermediateT);
        }
        return hits >= 2;
    }

    AZ::LegacyConversion::LegacyConversionResult PortalConverter::ConvertEntity(CBaseObject* entityToConvert)
    {
        if (AZStd::string(entityToConvert->metaObject()->className()) != "CPortalShapeObject")
        {
            // We don't know how to convert this entity, whatever it is
            return AZ::LegacyConversion::LegacyConversionResult::Ignored;
        }
        // Have a Portal.

        // Step 1, create an entity with no components.

        // if we have a parent, and the parent is not yet converted, we ignore this entity!
        // this is not ALWAYS the case - there may be types you wish to create anyway and just put them in the world, so we cannot enforce this
        // inside the CreateConvertedEntity function.
        AZ::Outcome<AZ::Entity*, AZ::LegacyConversion::CreateEntityResult> result(nullptr);

        AZ::ComponentTypeList componentsToAdd {};

        AZ::LegacyConversion::LegacyConversionRequestBus::BroadcastResult(result, &AZ::LegacyConversion::LegacyConversionRequests::CreateConvertedEntity, entityToConvert, true, componentsToAdd);

        if (!result.IsSuccess())
        {
            // if we failed due to no parent, we keep processing
            return (result.GetError() == AZ::LegacyConversion::CreateEntityResult::FailedNoParent) ? AZ::LegacyConversion::LegacyConversionResult::Ignored : AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        AZ::Entity* entity = result.GetValue();
        entity->Deactivate();

        auto portalComponent = entity->CreateComponent<EditorPortalComponent>();
        if (!portalComponent)
        {
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        // Step 2: extract the parameter data from the underlying CBaseObject into
        // the component.

        AZStd::fixed_vector<AZ::Vector3, 4> portalVerts;

        /*
           NOTE: Portal conversion here not only converts to the new shape system but also
           flattens the portal points to make them planar. We force them to be planar on Z here
           because that's sort of the way it was supposed to work (though not enforced) and how
           we expect it to work in the new system. The height difference between the min and max
           Z value of all the points will be added to the height of the portal. This is to emulate
           some legacy weirdness with how non-planar points are handled.
         */

        auto portalEntity = static_cast<CPortalShapeObject*>(entityToConvert);
        float minZ = FLT_MAX;
        float maxZ = -FLT_MAX;
        float heightMod = 0.0f;

        //Some concessions need to be made because the first point of the portal is where the entity's transform is
        bool firstPointIsLowest = false;
        float firstPointHeight = 0.0f;

        if (portalEntity->GetPointCount() != 4)
        {
            //Cannot make a portal component with anything other than 4 points
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        //Retrieve transform because we want to transform points
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, entity->GetId(), &AZ::TransformBus::Events::GetWorldTM);

        AZ::Vector3 worldPosition = transform.GetPosition();

        for (int i = 0; i < portalEntity->GetPointCount(); i++)
        {
            AZ::Vector3 point = transform * LYVec3ToAZVec3(portalEntity->GetPoint(i));

            float pointZ = point.GetZ();

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
            portalVerts.push_back(AZ::Vector3(objectSpacePoint.GetX(), objectSpacePoint.GetY(), 0));
        }

        heightMod = maxZ - minZ;

        //Turn the entity back on
        //Otherwise all the next bus calls will fail
        entity->Activate();

        //Now we can take all the params from the previous entity and shove them into the
        //config for this new component.
        bool conversionSuccess = false;

        AZ::EntityId newEntityId = entity->GetId();

        if (const CVarBlock* varBlock = portalEntity->GetVarBlock())
        {
            using namespace AZ::LegacyConversion::Util;

            conversionSuccess = true;

            conversionSuccess &= ConvertVarBus<EditorPortalRequestBus, float>("Height", varBlock, &EditorPortalRequestBus::Events::SetHeight, newEntityId);
            conversionSuccess &= ConvertVarBus<EditorPortalRequestBus, bool>("DisplayFilled", varBlock, &EditorPortalRequestBus::Events::SetDisplayFilled, newEntityId);
            conversionSuccess &= ConvertVarBus<EditorPortalRequestBus, bool>("AffectedBySun", varBlock, &EditorPortalRequestBus::Events::SetAffectedBySun, newEntityId);
            conversionSuccess &= ConvertVarBus<EditorPortalRequestBus, float>("ViewDistRatio", varBlock, &EditorPortalRequestBus::Events::SetViewDistRatio, newEntityId);
            conversionSuccess &= ConvertVarBus<EditorPortalRequestBus, bool>("SkyOnly", varBlock, &EditorPortalRequestBus::Events::SetSkyOnly, newEntityId);
            conversionSuccess &= ConvertVarBus<EditorPortalRequestBus, bool>("OceanIsVisible", varBlock, &EditorPortalRequestBus::Events::SetOceanIsVisible, newEntityId);
            conversionSuccess &= ConvertVarBus<EditorPortalRequestBus, bool>("UseDeepness", varBlock, &EditorPortalRequestBus::Events::SetUseDeepness, newEntityId);
            conversionSuccess &= ConvertVarBus<EditorPortalRequestBus, bool>("DoubleSide", varBlock, &EditorPortalRequestBus::Events::SetDoubleSide, newEntityId);
            conversionSuccess &= ConvertVarBus<EditorPortalRequestBus, bool>("LightBlending", varBlock, &EditorPortalRequestBus::Events::SetLightBlending, newEntityId);
            conversionSuccess &= ConvertVarBus<EditorPortalRequestBus, float>("LightBlendValue", varBlock, &EditorPortalRequestBus::Events::SetLightBlendValue, newEntityId);
        }

        if (!conversionSuccess)
        {
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        AZ::Transform worldTransform;
        AZ::TransformBus::EventResult(worldTransform, entity->GetId(), &AZ::TransformBus::Events::GetWorldTM);
        AZ::Vector3 position = worldTransform.GetTranslation();

        //Offset the transform to make sure that the entity's transform is brought down to the lowest point
        //Only need to do this if the first point is not the lowest
        if (!firstPointIsLowest)
        {
            position.SetZ(position.GetZ() - (firstPointHeight - minZ));
        }

        //Apply the height mod to the portal height
        portalComponent->m_config.m_height += heightMod;

        //Apply verts to the portal
        for (size_t i = 0; i < portalVerts.size(); ++i)
        {
            portalComponent->m_config.m_vertices[i] = portalVerts[i];
        }

        //Remove all rotation and scale from the entity
        worldTransform = AZ::Transform::CreateIdentity();
        worldTransform.SetTranslation(position);

        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTM, worldTransform);

        // finally, we can delete the old entity since we handled this completely!
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, result.GetValue()->GetId());
        return AZ::LegacyConversion::LegacyConversionResult::HandledDeleteEntity;
    }

    bool PortalConverter::BeforeConversionBegins()
    {
        return true;
    }

    bool PortalConverter::AfterConversionEnds()
    {
        return true;
    }
} //namespace Visibility
