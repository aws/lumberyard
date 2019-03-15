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

#include <AzCore/Math/Crc.h>
#include <AzCore/Math/VectorConversions.h>
#include <AzCore/RTTI/BehaviorContext.h>
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
    }

    void EditorOccluderAreaConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
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

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OccluderAreaConfiguration::m_displayFilled, "DisplayFilled", "Display the Occlude Area as a filled quad.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &OccluderAreaConfiguration::OnChange)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OccluderAreaConfiguration::m_cullDistRatio, "CullDistRatio", "The range of the culling effect.")
                    ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &OccluderAreaConfiguration::OnChange)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OccluderAreaConfiguration::m_useInIndoors, "UseInIndoors", "Should this occluder work inside VisAreas.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &OccluderAreaConfiguration::OnChange)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OccluderAreaConfiguration::m_doubleSide, "DoubleSide", "Should this occlude from both sides.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &OccluderAreaConfiguration::OnChange)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &OccluderAreaConfiguration::m_vertices, "Vertices", "Points that make up the Occluder Area.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &OccluderAreaConfiguration::OnVerticesChange)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ;
            }
        }
    }

    void EditorOccluderAreaComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorOccluderAreaComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1)
                ->Field("m_config", &EditorOccluderAreaComponent::m_config)
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
                ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<EditorOccluderAreaComponent>()->RequestBus("OccluderAreaRequestBus");
        }

        EditorOccluderAreaConfiguration::Reflect(context);
    }

    void EditorOccluderAreaConfiguration::OnChange()
    {
        m_component->UpdateObject();
    }

    void EditorOccluderAreaConfiguration::OnVerticesChange()
    {
        m_component->UpdateObject();
        m_component->m_vertexSelection.RefreshLocal();
    }

    EditorOccluderAreaComponent::EditorOccluderAreaComponent()
    {
        m_config.m_component = this;
    }

    EditorOccluderAreaComponent::~EditorOccluderAreaComponent()
    {
        m_vertexSelection.Destroy();

        if (m_area)
        {
            GetIEditor()->Get3DEngine()->DeleteVisArea(m_area);
            m_area = nullptr;
        }
    }

    void EditorOccluderAreaComponent::Activate()
    {
        // NOTE: We create the visarea here at activated, but destroy it in the destructor.
        // We have to do this, otherwise the visarea is not saved into the level.
        // Unfortunately, at this time we cannot create the visareas at game runtime.
        // This means that dynamic slices cannot effectively contain vis areas until we fix the core rendering system to allow that.
        const AZ::u64 visGUID = static_cast<AZ::u64>(GetEntityId());
        m_area = GetIEditor()->Get3DEngine()->CreateVisArea(visGUID);

        Base::Activate();
        OccluderAreaRequestBus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        EntitySelectionEvents::Bus::Handler::BusConnect(GetEntityId());

        UpdateObject();
    }

    void EditorOccluderAreaComponent::Deactivate()
    {
        m_vertexSelection.Destroy();

        EntitySelectionEvents::Bus::Handler::BusDisconnect(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect(GetEntityId());
        OccluderAreaRequestBus::Handler::BusDisconnect(GetEntityId());
        Base::Deactivate();
    }

    void EditorOccluderAreaComponent::UpdateObject()
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

            AZStd::string name = AZStd::string("OcclArea_") + GetEntity()->GetName();

            GetIEditor()->Get3DEngine()->UpdateVisArea(m_area, &verts[0], verts.size(), name.c_str(), info, false);
        }
    }

    void EditorOccluderAreaComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_vertexSelection.RefreshSpace(world);
        UpdateObject();
    }

    void EditorOccluderAreaComponent::DisplayEntity(bool& handled)
    {
        const AZ::Transform world = GetWorldTM();
        const AZ::Vector4 color(0.5f, 0.25f, 0.0f, 1.0f);
        const AZ::Vector4 selectedColor(1.0f, 0.5f, 0.0f, 1.0f);

        auto displayInterface = AzFramework::EntityDebugDisplayRequestBus::FindFirstHandler();
        AZ_Assert(displayInterface, "Invalid display context.");

        displayInterface->DepthWriteOff();
        displayInterface->PushMatrix(world);
        displayInterface->SetColor(IsSelected() ? selectedColor : color);
        displayInterface->SetLineWidth(5.0f);
        displayInterface->SetAlpha(0.8f);

        for (AZ::u8 i = 2; i < 4; i++)
        {
            //Draw plane
            if (m_config.m_displayFilled)
            {
                displayInterface->SetAlpha(0.3f);
                displayInterface->CullOff();
                displayInterface->DrawTri(m_config.m_vertices[0], m_config.m_vertices[i - 1], m_config.m_vertices[i]);
                displayInterface->CullOn();
                displayInterface->SetAlpha(0.8f);
            }
            displayInterface->DrawLine(m_config.m_vertices[i - 2], m_config.m_vertices[i - 1]);
            displayInterface->DrawLine(m_config.m_vertices[i - 1], m_config.m_vertices[i]);
        }
        //Draw the closing line
        displayInterface->DrawLine(m_config.m_vertices[3], m_config.m_vertices[0]);

        AzToolsFramework::EditorVertexSelectionUtil::DisplayVertexContainerIndices(*displayInterface, m_vertexSelection, GetWorldTM(), IsSelected());

        displayInterface->DepthWriteOn();
        displayInterface->PopMatrix();

        handled = true;
    }

    void EditorOccluderAreaComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        OccluderAreaComponent* component = gameEntity->CreateComponent<OccluderAreaComponent>(&m_config);
    }

    void EditorOccluderAreaComponent::OnSelected()
    {
        m_vertexSelection.Destroy();
        CreateManipulators();
    }

    void EditorOccluderAreaComponent::OnDeselected()
    {
        m_vertexSelection.Destroy();
    }

    void EditorOccluderAreaComponent::CreateManipulators()
    {
        m_vertexSelection.m_hoverSelection = AZStd::make_unique<AzToolsFramework::NullHoverSelection>();

        // create interface wrapping internal AZStd::array for use by vertex selection
        m_vertexSelection.m_vertices =
            AZStd::make_unique<AzToolsFramework::FixedVerticesArray<AZ::Vector3, 4>>(m_config.m_vertices);

        m_vertexSelection.Create(GetEntityId(), AzToolsFramework::ManipulatorManagerId(1),
            AzToolsFramework::TranslationManipulators::Dimensions::Three,
            AzToolsFramework::ConfigureTranslationManipulatorAppearance3d);

        m_vertexSelection.m_onVertexPositionsUpdated = [this]()
        {
            UpdateObject();
        };
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

        AZ::ComponentTypeList componentsToAdd {};
        AZ::LegacyConversion::LegacyConversionRequestBus::BroadcastResult(result, &AZ::LegacyConversion::LegacyConversionRequests::CreateConvertedEntity, entityToConvert, true, componentsToAdd);

        if (!result.IsSuccess())
        {
            // if we failed due to no parent, we keep processing
            return (result.GetError() == AZ::LegacyConversion::CreateEntityResult::FailedNoParent) ? AZ::LegacyConversion::LegacyConversionResult::Ignored : AZ::LegacyConversion::LegacyConversionResult::Failed;
        }
        AZ::Entity* entity = result.GetValue();
        entity->Deactivate();

        EditorOccluderAreaComponent* component = entity->CreateComponent<EditorOccluderAreaComponent>();

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

        AZ::EntityId newEntityId = entity->GetId();

        COccluderAreaObject* occluderEntity = static_cast<COccluderAreaObject *>(entityToConvert);

        if (const CVarBlock* varBlock = occluderEntity->GetVarBlock())
        {
            using namespace AZ::LegacyConversion::Util;

            conversionSuccess = true;

            conversionSuccess &= ConvertVarBus<OccluderAreaRequestBus, bool>("DisplayFilled", varBlock, &OccluderAreaRequestBus::Events::SetDisplayFilled, newEntityId);
            conversionSuccess &= ConvertVarBus<OccluderAreaRequestBus, float>("CullDistRatio", varBlock, &OccluderAreaRequestBus::Events::SetCullDistRatio, newEntityId);
            conversionSuccess &= ConvertVarBus<OccluderAreaRequestBus, bool>("UseInIndoors", varBlock, &OccluderAreaRequestBus::Events::SetUseInIndoors, newEntityId);
        }

        if (!conversionSuccess)
        {
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        //Tell the editor component to update vertices, since EBus is connected after the entity is activated.
        for (size_t i = 0; i < occluderEntity->GetPointCount(); ++i)
        {
            component->m_config.m_vertices[i] = LYVec3ToAZVec3(occluderEntity->GetPoint(i));
        }

        // Call Update to redraw the shape
        component->UpdateObject();

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
        AZ::Entity *entity = result.GetValue();
        entity->Deactivate();

        EditorOccluderAreaComponent *component = entity->CreateComponent<EditorOccluderAreaComponent>();

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

        AZ::EntityId newEntityId = entity->GetId();

        COccluderPlaneObject* occluderEntity = static_cast<COccluderPlaneObject *>(entityToConvert);
        float planeHeight = occluderEntity->GetHeight();

        if (const CVarBlock* varBlock = occluderEntity->GetVarBlock())
        {
            using namespace AZ::LegacyConversion::Util;

            conversionSuccess = true;

            conversionSuccess &= ConvertVarBus<OccluderAreaRequestBus, bool>("DisplayFilled", varBlock, &OccluderAreaRequestBus::Events::SetDisplayFilled, newEntityId);
            conversionSuccess &= ConvertVarBus<OccluderAreaRequestBus, float>("CullDistRatio", varBlock, &OccluderAreaRequestBus::Events::SetCullDistRatio, newEntityId);
            conversionSuccess &= ConvertVarBus<OccluderAreaRequestBus, bool>("UseInIndoors", varBlock, &OccluderAreaRequestBus::Events::SetUseInIndoors, newEntityId);
            conversionSuccess &= ConvertVarBus<OccluderAreaRequestBus, bool>("DoubleSide", varBlock, &OccluderAreaRequestBus::Events::SetDoubleSide, newEntityId);
        }

        if (!conversionSuccess)
        {
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        //Tell the editor component to update vertices, since EBus is connected after the entity is activated.

        const int vertexCount = occluderEntity->GetPointCount();
        AZ_Assert(vertexCount == 2, "There Should be 2 points for Occlude Plane");

        /**
        * The way CryEngine is doing is that using the 2 vertices and add height to these vertices to create the other 2 vertices
        */

        //The plane has two points but we need 4 for the occluder area
        AZStd::fixed_vector<AZ::Vector3, 4> vertices(4);
        AZ::Transform transform = component->GetLocalTM();
        AZ::Quaternion originQuat = AZ::Quaternion::CreateFromTransform(transform);
        for (size_t i = 0; i < vertexCount; i++)
        {
            //Get the actual vertex position based on the local rotation
            const AZ::Vector3 point = originQuat * LYVec3ToAZVec3(occluderEntity->GetPoint(i));
            vertices[i] = point;

            //The legacy plane only has 2 points so we need to generate the other two points in the quad
            // On point 0 we generate point 3
            // On point 1 we generate point 2
            size_t generatedPointIndex = vertexCount * 2 - 1 - i;

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
        component->UpdateObject();

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
