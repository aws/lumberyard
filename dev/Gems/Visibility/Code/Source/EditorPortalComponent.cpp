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

#include <AzCore/RTTI/BehaviorContext.h>

// Include files needed for writing DisplayEntity functions that access the DisplayContext directly.
#include <EditorCoreAPI.h>

#include <AzCore/Math/Crc.h>
#include <AzFramework/Math/MathUtils.h>
#include <Editor/Objects/BaseObject.h>
#include <Editor/Objects/VisAreaShapeObject.h>
#include <LegacyEntityConversion/LegacyEntityConversion.h>
#include <MathConversion.h>

namespace Visibility
{
    void EditorPortalConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
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
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorPortalComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1)
                ->Field("m_config", &EditorPortalComponent::m_config)
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
                ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<EditorPortalComponent>()->RequestBus("PortalRequestBus");
        }

        EditorPortalConfiguration::Reflect(context);
    }

    void EditorPortalConfiguration::OnChange()
    {
        m_component->UpdateObject();
    }

    void EditorPortalConfiguration::OnVerticesChange()
    {
        m_component->UpdateObject();
        m_component->m_vertexSelection.Refresh();
    }

    EditorPortalComponent::EditorPortalComponent()
    {
        m_config.m_component = this;
    }

    EditorPortalComponent::~EditorPortalComponent()
    {
        m_vertexSelection.Destroy();

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
        // NOTE: We create the vis-area here at activated, but destroy it in the destructor.
        // We have to do this, otherwise the vis-area is not saved into the level.
        // Unfortunately, at this time we cannot create the vis-areas at game runtime.
        // This means that dynamic slices cannot effectively contain vis-areas until we fix the core rendering system to allow that.
        const AZ::u64 visGUID = static_cast<AZ::u64>(GetEntityId());
        m_area = GetIEditor()->Get3DEngine()->CreateVisArea(visGUID);

        m_AZCachedWorldTransform = AZ::Transform::CreateIdentity();
        m_cryCachedWorldTransform = Matrix34::CreateIdentity();

        Base::Activate();
        PortalRequestBus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        EntitySelectionEvents::Bus::Handler::BusConnect(GetEntityId());

        //Call OnTransformChanged manually to cache current transform since it won't be called
        //automatically for us when the level starts up.
        AZ::Transform worldTM;
        AZ::TransformBus::EventResult(worldTM, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        //Use an identity transform for localTM because the
        //OnTransformChanged impl for this class doesn't need it
        OnTransformChanged(AZ::Transform::CreateIdentity(), worldTM);
    }

    void EditorPortalComponent::Deactivate()
    {
        m_vertexSelection.Destroy();

        EntitySelectionEvents::Bus::Handler::BusDisconnect(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusDisconnect(GetEntityId());
        PortalRequestBus::Handler::BusDisconnect();
        Base::Deactivate();
    }

    void EditorPortalComponent::UpdateObject()
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
        //Cache the transform so that we don't have to retrieve it every time UpdateObject is called
        m_AZCachedWorldTransform = world;
        m_cryCachedWorldTransform = AZTransformToLYTransform(m_AZCachedWorldTransform);

        UpdateObject();
    }

    void EditorPortalComponent::DisplayEntity(bool& handled)
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

        for (size_t i = 0; i < quadPointCount; ++i)
        {
            //Remove translation from quad points so we can use them with the DisplayContext's usage of the transform
            transformedQuadPoints[i] -= translation;

            const float height = transformedQuadPoints[i].GetZ();
            if (height < minHeight)
            {
                minHeight = height;
            }
            if (height > maxHeight)
            {
                maxHeight = height;
            }
        }

        const AZ::Vector3 floorLeftFront = AZ::Vector3(transformedQuadPoints[0].GetX(), transformedQuadPoints[0].GetY(), minHeight);
        const AZ::Vector3 floorRightFront = AZ::Vector3(transformedQuadPoints[1].GetX(), transformedQuadPoints[1].GetY(), minHeight);
        const AZ::Vector3 floorLeftBack = AZ::Vector3(transformedQuadPoints[2].GetX(), transformedQuadPoints[2].GetY(), minHeight);
        const AZ::Vector3 floorRightBack = AZ::Vector3(transformedQuadPoints[3].GetX(), transformedQuadPoints[3].GetY(), minHeight);

        const AZ::Vector3 quadUpperLeftFront = AZ::Vector3(transformedQuadPoints[0].GetX(), transformedQuadPoints[0].GetY(), maxHeight);
        const AZ::Vector3 quadUpperRightFront = AZ::Vector3(transformedQuadPoints[1].GetX(), transformedQuadPoints[1].GetY(), maxHeight);
        const AZ::Vector3 quadUpperLeftBack = AZ::Vector3(transformedQuadPoints[2].GetX(), transformedQuadPoints[2].GetY(), maxHeight);
        const AZ::Vector3 quadUpperRightBack = AZ::Vector3(transformedQuadPoints[3].GetX(), transformedQuadPoints[3].GetY(), maxHeight);

        const AZ::Vector3 portalUpperLeftFront = AZ::Vector3(transformedQuadPoints[0].GetX(), transformedQuadPoints[0].GetY(), maxHeight + m_config.m_height);
        const AZ::Vector3 portalUpperRightFront = AZ::Vector3(transformedQuadPoints[1].GetX(), transformedQuadPoints[1].GetY(), maxHeight + m_config.m_height);
        const AZ::Vector3 portalUpperLeftBack = AZ::Vector3(transformedQuadPoints[2].GetX(), transformedQuadPoints[2].GetY(), maxHeight + m_config.m_height);
        const AZ::Vector3 portalUpperRightBack = AZ::Vector3(transformedQuadPoints[3].GetX(), transformedQuadPoints[3].GetY(), maxHeight + m_config.m_height);

        auto* dc = AzFramework::EntityDebugDisplayRequestBus::FindFirstHandler();
        AZ_Assert(dc, "Invalid display context.");

        //Draw the outline of the OBB of the Portal's quad
        AZ::Color color(0.000f, 1.0f, 0.000f, 1.0f);
        dc->SetColor(AZ::Vector4(color.GetR(), color.GetG(), color.GetB(), 1.f));

        //Remove all rotation from the transform
        const AZ::Quaternion rotation = AZ::Quaternion::CreateIdentity();

        AZ::Transform worldTMOnlyZRot = m_AZCachedWorldTransform;
        worldTMOnlyZRot.SetRotationPartFromQuaternion(rotation);

        dc->PushMatrix(worldTMOnlyZRot);

        //Draw the outline of the OBB of the portal quad

        //Bottom
        dc->DrawLine(floorLeftFront, floorRightFront);
        dc->DrawLine(floorRightFront, floorRightBack);
        dc->DrawLine(floorRightBack, floorLeftBack);
        dc->DrawLine(floorLeftBack, floorLeftFront);
        //Top
        dc->DrawLine(quadUpperLeftFront, quadUpperRightFront);
        dc->DrawLine(quadUpperRightFront, quadUpperRightBack);
        dc->DrawLine(quadUpperRightBack, quadUpperLeftBack);
        dc->DrawLine(quadUpperLeftBack, quadUpperLeftFront);
        //Left
        dc->DrawLine(floorLeftFront, quadUpperLeftFront);
        dc->DrawLine(quadUpperLeftFront, quadUpperLeftBack);
        dc->DrawLine(quadUpperLeftBack, floorLeftBack);
        dc->DrawLine(floorLeftBack, floorLeftFront);
        //Right
        dc->DrawLine(floorRightFront, quadUpperRightFront);
        dc->DrawLine(quadUpperRightFront, quadUpperRightBack);
        dc->DrawLine(quadUpperRightBack, floorRightBack);
        dc->DrawLine(floorRightBack, floorRightFront);
        //Front
        dc->DrawLine(floorLeftFront, floorRightFront);
        dc->DrawLine(floorRightFront, quadUpperRightFront);
        dc->DrawLine(quadUpperRightFront, quadUpperLeftFront);
        dc->DrawLine(quadUpperLeftFront, floorLeftFront);
        //Back
        dc->DrawLine(floorLeftBack, floorRightBack);
        dc->DrawLine(floorRightBack, quadUpperRightBack);
        dc->DrawLine(quadUpperRightBack, quadUpperLeftBack);
        dc->DrawLine(quadUpperLeftBack, floorLeftBack);

        //Now draw the entire portal volume (Previous OBB + extra height)
        if (m_config.m_displayFilled)
        {
            //Draw whole portal with less alpha
            dc->SetColor(AZ::Vector4(color.GetR(), color.GetG(), color.GetB(), 0.1f));

            //Draw both winding orders for quads so they appear solid from all angles
            //Not drawing boxes because the corners of the quad may not be hit if the bounds are rotated oddly

            //Bottom
            dc->DrawQuad(floorLeftFront, floorRightFront, floorRightBack, floorLeftBack);
            dc->DrawQuad(floorLeftFront, floorLeftBack, floorRightBack, floorRightFront);
            //Top
            dc->DrawQuad(portalUpperLeftFront, portalUpperRightFront, portalUpperRightBack, portalUpperLeftBack);
            dc->DrawQuad(portalUpperLeftFront, portalUpperLeftBack, portalUpperRightBack, portalUpperRightFront);
            //Left
            dc->DrawQuad(floorLeftFront, portalUpperLeftFront, portalUpperLeftBack, floorLeftBack);
            dc->DrawQuad(floorLeftFront, floorLeftBack, portalUpperLeftBack, portalUpperLeftFront);
            //Right
            dc->DrawQuad(floorRightFront, portalUpperRightFront, portalUpperRightBack, floorRightBack);
            dc->DrawQuad(floorRightFront, floorRightBack, portalUpperRightBack, portalUpperRightFront);
            //Front
            dc->DrawQuad(floorLeftFront, floorRightFront, portalUpperRightFront, portalUpperLeftFront);
            dc->DrawQuad(floorLeftFront, portalUpperLeftFront, portalUpperRightFront, floorRightFront);
            //Back
            dc->DrawQuad(floorLeftBack, floorRightBack, portalUpperRightBack, portalUpperLeftBack);
            dc->DrawQuad(floorLeftBack, portalUpperLeftBack, portalUpperRightBack, floorRightBack);
        }
        else
        {
            //Bottom
            dc->DrawLine(floorLeftFront, floorRightFront);
            dc->DrawLine(floorRightFront, floorRightBack);
            dc->DrawLine(floorRightBack, floorLeftBack);
            dc->DrawLine(floorLeftBack, floorLeftFront);
            //Top
            dc->DrawLine(portalUpperLeftFront, portalUpperRightFront);
            dc->DrawLine(portalUpperRightFront, portalUpperRightBack);
            dc->DrawLine(portalUpperRightBack, portalUpperLeftBack);
            dc->DrawLine(portalUpperLeftBack, portalUpperLeftFront);
            //Left
            dc->DrawLine(floorLeftFront, portalUpperLeftFront);
            dc->DrawLine(portalUpperLeftFront, portalUpperLeftBack);
            dc->DrawLine(portalUpperLeftBack, floorLeftBack);
            dc->DrawLine(floorLeftBack, floorLeftFront);
            //Right
            dc->DrawLine(floorRightFront, portalUpperRightFront);
            dc->DrawLine(portalUpperRightFront, portalUpperRightBack);
            dc->DrawLine(portalUpperRightBack, floorRightBack);
            dc->DrawLine(floorRightBack, floorRightFront);
            //Front
            dc->DrawLine(floorLeftFront, floorRightFront);
            dc->DrawLine(floorRightFront, portalUpperRightFront);
            dc->DrawLine(portalUpperRightFront, portalUpperLeftFront);
            dc->DrawLine(portalUpperLeftFront, floorLeftFront);
            //Back
            dc->DrawLine(floorLeftBack, floorRightBack);
            dc->DrawLine(floorRightBack, portalUpperRightBack);
            dc->DrawLine(portalUpperRightBack, portalUpperLeftBack);
            dc->DrawLine(portalUpperLeftBack, floorLeftBack);
        }

        AzToolsFramework::EditorVertexSelectionUtil::DisplayVertexContainerIndices(*dc, m_vertexSelection, GetWorldTM(), IsSelected());

        dc->PopMatrix();

        handled = true;
    }

    void EditorPortalComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        PortalComponent* component = gameEntity->CreateComponent<PortalComponent>(&m_config);
    }

    void EditorPortalComponent::OnSelected()
    {
        m_vertexSelection.Destroy();
        CreateManipulators();
    }
    void EditorPortalComponent::OnDeselected()
    {
        m_vertexSelection.Destroy();
    }

    // Manipulator handling
    void EditorPortalComponent::CreateManipulators()
    {
        AZStd::unique_ptr<AzToolsFramework::NullHoverSelection> nullHoverSelection =
            AZStd::make_unique<AzToolsFramework::NullHoverSelection>();
        m_vertexSelection.m_hoverSelection = AZStd::move(nullHoverSelection);

        // create interface wrapping internal AZStd::array for use by vertex selection
        m_vertexSelection.m_vertices =
            AZStd::make_unique<AzToolsFramework::FixedVerticesArray<AZ::Vector3, 4>>(m_config.m_vertices);

        const AzToolsFramework::ManipulatorManagerId managerId = AzToolsFramework::ManipulatorManagerId(1);
        m_vertexSelection.Create(GetEntityId(), managerId,
            AzToolsFramework::TranslationManipulator::Dimensions::Three,
            AzToolsFramework::ConfigureTranslationManipulatorAppearance3d);
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

        using namespace LmbrCentral;

        EditorPortalComponent* portalComponent = entity->CreateComponent<EditorPortalComponent>();

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

        CPortalShapeObject* portalEntity = static_cast<CPortalShapeObject *>(entityToConvert);
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

            conversionSuccess &= ConvertVarBus<PortalRequestBus, float>("Height", varBlock, &PortalRequestBus::Events::SetHeight, newEntityId);
            conversionSuccess &= ConvertVarBus<PortalRequestBus, bool>("DisplayFilled", varBlock, &PortalRequestBus::Events::SetDisplayFilled, newEntityId);
            conversionSuccess &= ConvertVarBus<PortalRequestBus, bool>("AffectedBySun", varBlock, &PortalRequestBus::Events::SetAffectedBySun, newEntityId);
            conversionSuccess &= ConvertVarBus<PortalRequestBus, float>("ViewDistRatio", varBlock, &PortalRequestBus::Events::SetViewDistRatio, newEntityId);
            conversionSuccess &= ConvertVarBus<PortalRequestBus, bool>("SkyOnly", varBlock, &PortalRequestBus::Events::SetSkyOnly, newEntityId);
            conversionSuccess &= ConvertVarBus<PortalRequestBus, bool>("OceanIsVisible", varBlock, &PortalRequestBus::Events::SetOceanIsVisible, newEntityId);
            conversionSuccess &= ConvertVarBus<PortalRequestBus, bool>("UseDeepness", varBlock, &PortalRequestBus::Events::SetUseDeepness, newEntityId);
            conversionSuccess &= ConvertVarBus<PortalRequestBus, bool>("DoubleSide", varBlock, &PortalRequestBus::Events::SetDoubleSide, newEntityId);
            conversionSuccess &= ConvertVarBus<PortalRequestBus, bool>("LightBlending", varBlock, &PortalRequestBus::Events::SetLightBlending, newEntityId);
            conversionSuccess &= ConvertVarBus<PortalRequestBus, float>("LightBlendValue", varBlock, &PortalRequestBus::Events::SetLightBlendValue, newEntityId);
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
