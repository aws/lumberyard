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

#include "Water_precompiled.h"
#include "EditorWaterVolumeComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>

#include "MathConversion.h"
#include <AzCore/Math/VectorConversions.h>

#include <LmbrCentral/Shape/PolygonPrismShapeComponentBus.h>

#include <LegacyEntityConversion/LegacyEntityConversion.h>

//Legacy for conversion
#include "Objects/WaterShapeObject.h"
#include "Material/Material.h"

namespace Water
{
    AZ::Color EditorWaterVolumeCommon::s_waterAreaColor = AZ::Color(0.3f, 0.5f, 9.0f, 1.0f);

    void EditorWaterVolumeCommon::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorWaterVolumeCommon, WaterVolumeCommon>()
                ->Version(1)
                ->Field("DisplayFilled", &EditorWaterVolumeCommon::m_displayFilled)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorWaterVolumeCommon>("Editor Water Volume Common Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorWaterVolumeCommon::m_displayFilled, "Display Filled", "Renders the area as a filled volume.")
                    ;

                editContext->Class<WaterVolumeCommon>("Water Volume Common Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterVolumeCommon::m_materialAsset, "Material", "The water material to use. Given material must have a water shader.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnMaterialAssetChange)

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &WaterVolumeCommon::m_minSpec, "Minimum spec", "The minimum engine spec for this volume to render.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnMinSpecChange)
                    ->EnumAttribute(EngineSpec::Never, "Never")
                    ->EnumAttribute(EngineSpec::VeryHigh, "Very high")
                    ->EnumAttribute(EngineSpec::High, "High")
                    ->EnumAttribute(EngineSpec::Medium, "Medium")
                    ->EnumAttribute(EngineSpec::Low, "Low")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterVolumeCommon::m_surfaceUScale, "Surface U Scale", "How much the water surface texture tiles on the U axis.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnWaterAreaParamChange)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterVolumeCommon::m_surfaceVScale, "Surface V Scale", "How much the water surface texture tiles on the V axis.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnWaterAreaParamChange)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterVolumeCommon::m_viewDistanceMultiplier, "View Distance Multiplier", "Adjusts max view distance. If 1.0 then default is used. 1.1 would be 10% further than default.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnViewDistanceMultiplierChange)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Fog")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterVolumeCommon::m_fogDensity, "Density", "The more dense the water fog, the harder it is to see through.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnFogDensityChange)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Color, &WaterVolumeCommon::m_fogColor, "Color", "Color of the water fog.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnFogColorChange)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterVolumeCommon::m_fogColorMultiplier, "Color Multiplier", "Multiplied against the Color parameter.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnFogColorChange)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterVolumeCommon::m_fogColorAffectedBySun, "Color Affected by Sun", "Should sky color affect the water's fog color. Useful if this volume is outdoors.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnFogColorAffectedBySunChange)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterVolumeCommon::m_fogShadowing, "Shadowing", "How much shadows should affect the water's fog color.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnFogShadowingChange)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterVolumeCommon::m_capFogAtVolumeDepth, "Cap at Volume Depth", "Should fog still render if the player is underneath the volume.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnCapFogAtVolumeDepthChange)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Caustics")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterVolumeCommon::m_causticsEnabled, "Enabled", "Enables water caustics.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnCausticsEnableChange)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterVolumeCommon::m_causticIntensity, "Intensity", "Affects the brightness and visibility of caustics.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnCausticIntensityChange)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterVolumeCommon::m_causticTiling, "Tiling", "Affects how many caustics are rendered.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnCausticTilingChange)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterVolumeCommon::m_causticHeight, "Height", "Affects how high off the surface that caustics are rendered.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnCausticHeightChange)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Advanced")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterVolumeCommon::m_spillableVolume, "Spillable Volume", "How much volume can spill into a container below.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnPhysicsParamChange)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterVolumeCommon::m_volumeAccuracy, "Volume Accuracy", "How accurate the surface level of the spilled volume is.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnPhysicsParamChange)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterVolumeCommon::m_extrudeBorder, "Extrude Border", "How much additional border to add to the spilled volume.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnPhysicsParamChange)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterVolumeCommon::m_convexBorder, "Convex Border", "Should the convex hull of the container be taken into account.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnPhysicsParamChange)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterVolumeCommon::m_objectSizeLimit, "Object Size Limit", "The minimum volume that an object must have to affect the water surface.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnPhysicsParamChange)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Wave Simulation")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterVolumeCommon::m_waveSurfaceCellSize, "Surface Cell Size", "How large each cell of the water volume is.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnPhysicsParamChange)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterVolumeCommon::m_waveSpeed, "Speed", "How fast each wave moves.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnPhysicsParamChange)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterVolumeCommon::m_waveDampening, "Dampening", "How much dampening force is applied during simulation.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnPhysicsParamChange)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterVolumeCommon::m_waveTimestep, "Timestep", "How often the wave simulation ticks.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnPhysicsParamChange)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.001f)		// Above 1 millisec updates - closer to 0 will create major slowdown, and 0 will freeze the editor

                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterVolumeCommon::m_waveSleepThreshold, "Sleep Threshold", "The lowest velocity for a cell to rest.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnPhysicsParamChange)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterVolumeCommon::m_waveDepthCellSize, "Depth Cell Size", "How large each depth cell is.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnPhysicsParamChange)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterVolumeCommon::m_waveHeightLimit, "Height Limit", "The highest and lowest that the surface can deform.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnPhysicsParamChange)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterVolumeCommon::m_waveForce, "Force", "How strong the wave force is.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnPhysicsParamChange)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterVolumeCommon::m_waveSimulationAreaGrowth, "Simulation Area Growth", "Additional space added if the water simulation will cause expansion.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaterVolumeCommon::OnPhysicsParamChange)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ;
            }
        }
    }

    void EditorWaterVolumeCommon::Activate()
    {
        WaterVolumeCommon::Activate();

        EditorWaterVolumeComponentRequestBus::Handler::BusConnect(m_entityId);
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void EditorWaterVolumeCommon::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        EditorWaterVolumeComponentRequestBus::Handler::BusDisconnect(m_entityId);

        WaterVolumeCommon::Deactivate();
    }

    void EditorWaterVolumeCommon::DrawWaterVolume(AzFramework::EntityDebugDisplayRequests* dc)
    {
        const AZStd::vector<AZ::Vector3>& vertices = m_azVerts;
        size_t vertCount = vertices.size();

        dc->SetColor(s_waterAreaColor.GetAsVector4());

        //Draw walls for every line segment
        for (size_t i = 0; i < vertCount; ++i)
        {
            AZ::Vector3 lowerLeft = AZ::Vector3();
            AZ::Vector3 lowerRight = AZ::Vector3();
            AZ::Vector3 upperRight = AZ::Vector3();
            AZ::Vector3 upperLeft = AZ::Vector3();

            upperLeft = m_currentWorldTransform * vertices[i];
            upperRight = m_currentWorldTransform * vertices[(i + 1) % vertCount];
            //The mod with vertCount ensures that for the last vert it will connect back with vert 0
            //If vert count is 10, the last vert will be i = 9 and we want that to create a surface with vert 0

            //Lower points are just upper points moved down
            lowerRight = AZ::Vector3(upperRight.GetX(), upperRight.GetY(), upperRight.GetZ() - m_waterDepthScaled);
            lowerLeft = AZ::Vector3(upperLeft.GetX(), upperLeft.GetY(), upperLeft.GetZ() - m_waterDepthScaled);

            if (m_displayFilled)
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
    }

    void EditorWaterVolumeCommon::SetMinSpec(EngineSpec minSpec)
    {
        m_minSpec = minSpec;
        OnMinSpecChange();
    }

    void EditorWaterVolumeCommon::SetViewDistanceMultiplier(float viewDistanceMultiplier)
    {
        m_viewDistanceMultiplier = viewDistanceMultiplier;
        OnViewDistanceMultiplierChange();
    }

    void EditorWaterVolumeCommon::OnEditorSpecChange()
    {
        //Trigger the min spec to update and refresh
        OnMinSpecChange();
    }

    void EditorWaterVolumeComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides)
    {
        provides.push_back(AZ_CRC("WaterVolumeService", 0x895e29b1));
    }
    void EditorWaterVolumeComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        //Only compatible with Box, Cylinder and PolygonPrism shapes 
        incompatible.push_back(AZ_CRC("CapsuleShapeService", 0x9bc1122c));
        incompatible.push_back(AZ_CRC("SphereShapeService", 0x90c8dc80));
        incompatible.push_back(AZ_CRC("CompoundShapeService", 0x4f7c640a));
    }
    void EditorWaterVolumeComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires)
    {
        requires.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        requires.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
    }

    void EditorWaterVolumeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorWaterVolumeComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1)
                ->Field("Common", &EditorWaterVolumeComponent::m_common)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorWaterVolumeComponent>("Water Volume", "A volume that represents a small body of water.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Environment")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/WaterVolume.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/WaterVolume.png")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/console/lumberyard/userguide/water-volume-component")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorWaterVolumeComponent::m_common, "Common", "No Description")
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<EditorWaterVolumeComponent>()->RequestBus("WaterVolumeComponentRequestBus");
        }

        EditorWaterVolumeCommon::Reflect(context);
    }

    void EditorWaterVolumeComponent::Init()
    {
        m_common.Init(GetEntityId());
    }

    void EditorWaterVolumeComponent::Activate()
    {
        const AZ::EntityId& entityId = GetEntityId();

        m_common.Activate();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(entityId);

    }

    void EditorWaterVolumeComponent::Deactivate()
    {
        const AZ::EntityId& entityId = GetEntityId();

        m_common.Deactivate();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect(entityId);
    }

    void EditorWaterVolumeComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<WaterVolumeComponent>(&m_common);
    }

    void EditorWaterVolumeComponent::DisplayEntity(bool& handled)
    {
        auto* dc = AzFramework::EntityDebugDisplayRequestBus::FindFirstHandler();
        if (dc == nullptr)
        {
            handled = false;
            return;
        }

        m_common.DrawWaterVolume(dc);

        handled = true;
    }

    AZ::LegacyConversion::LegacyConversionResult WaterConverter::ConvertEntity(CBaseObject* entityToConvert)
    {
        const AZStd::string className = AZStd::string(entityToConvert->metaObject()->className());

        if (!(className == "CWaterShapeObject" && className != "CEntityObject"))
        {
            // We don't know how to convert this entity, whatever it is
            return AZ::LegacyConversion::LegacyConversionResult::Ignored;
        }

        //Get the underlying entity object
        const CWaterShapeObject* waterShapeObject = static_cast<CWaterShapeObject*>(entityToConvert);
        if (!waterShapeObject)
        {
            //Object was not an water shape object but we did not ignore the object
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        // if we have a parent, and the parent is not yet converted, we ignore this entity!
        // this is not ALWAYS the case - there may be types you wish to create anyway and just put them in the world, so we cannot enforce this
        // inside the CreateConvertedEntity function.
        AZ::Outcome<AZ::Entity*, AZ::LegacyConversion::CreateEntityResult> result(nullptr);

        const AZ::Uuid waterVolumeId = "{41EE10CD-11CF-40D3-BDF4-B70FC23EB44C}";
        AZ::ComponentTypeList componentList = { waterVolumeId, "{5368F204-FE6D-45C0-9A4F-0F933D90A785}" }; //EditorWaterVolumeComponent, EditorPolygonPrismComponent
        AZ::LegacyConversion::LegacyConversionRequestBus::BroadcastResult(result, &AZ::LegacyConversion::LegacyConversionRequests::CreateConvertedEntity, entityToConvert, true, componentList);

        if (!result.IsSuccess())
        {
            // if we failed due to no parent, we keep processing
            return (result.GetError() == AZ::LegacyConversion::CreateEntityResult::FailedNoParent) ? AZ::LegacyConversion::LegacyConversionResult::Ignored : AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        const AZ::Entity* entity = result.GetValue();
        const AZ::EntityId entityId = entity->GetId();

        //Material needs to be set on the material owner request bus like this
        CMaterial* material = waterShapeObject->GetMaterial();
        if (material)
        {
            LmbrCentral::MaterialOwnerRequestBus::Event(entityId, &LmbrCentral::MaterialOwnerRequestBus::Events::SetMaterial, material->GetMatInfo());
        }

        //Set water volume parameters
        bool conversionSuccess = false;

        if (const CVarBlock* varBlock = waterShapeObject->GetVarBlock())
        {
            using namespace AZ::LegacyConversion::Util;
            using namespace Water;

            conversionSuccess = true;

            //Set some specific values directly with the EditorWaterVolumeComponentRequestBus
            //These values only matter at edit time

            //MinSpec is kind of special as it's not really a serialized var
            EditorWaterVolumeComponentRequestBus::Event(entityId, &EditorWaterVolumeComponentRequestBus::Events::SetMinSpec, SpecConversion(static_cast<ESystemConfigSpec>(waterShapeObject->GetMinSpec())));

            conversionSuccess &= ConvertVarBus<EditorWaterVolumeComponentRequestBus, bool>("DisplayFilled", varBlock, &EditorWaterVolumeComponentRequestBus::Events::SetDisplayFilled, entityId);
            conversionSuccess &= ConvertVarBus<EditorWaterVolumeComponentRequestBus, float>("ViewDistancemMultiplier", varBlock, &EditorWaterVolumeComponentRequestBus::Events::SetViewDistanceMultiplier, entityId);

            conversionSuccess &= ConvertVarBus<WaterVolumeComponentRequestBus, float>("UScale", varBlock, &WaterVolumeComponentRequestBus::Events::SetSurfaceUScale, entityId);
            conversionSuccess &= ConvertVarBus<WaterVolumeComponentRequestBus, float>("VScale", varBlock, &WaterVolumeComponentRequestBus::Events::SetSurfaceVScale, entityId);

            conversionSuccess &= ConvertVarBus<WaterVolumeComponentRequestBus, float>("FogDensity", varBlock, &WaterVolumeComponentRequestBus::Events::SetFogDensity, entityId);
            conversionSuccess &= ConvertVarBus<WaterVolumeComponentRequestBus, Vec3, AZ::Vector3>("FogColor", varBlock, &WaterVolumeComponentRequestBus::Events::SetFogColor, entityId, LYVec3ToAZVec3);
            conversionSuccess &= ConvertVarBus<WaterVolumeComponentRequestBus, float>("FogColorMultiplier", varBlock, &WaterVolumeComponentRequestBus::Events::SetFogColorMultiplier, entityId);
            conversionSuccess &= ConvertVarBus<WaterVolumeComponentRequestBus, bool>("FogColorAffectedBySun", varBlock, &WaterVolumeComponentRequestBus::Events::SetFogColorAffectedBySun, entityId);
            conversionSuccess &= ConvertVarBus<WaterVolumeComponentRequestBus, float>("FogShadowing", varBlock, &WaterVolumeComponentRequestBus::Events::SetFogShadowing, entityId);
            conversionSuccess &= ConvertVarBus<WaterVolumeComponentRequestBus, bool>("CapFogAtVolumeDepth", varBlock, &WaterVolumeComponentRequestBus::Events::SetCapFogAtVolumeDepth, entityId);

            conversionSuccess &= ConvertVarBus<WaterVolumeComponentRequestBus, bool>("Caustics", varBlock, &WaterVolumeComponentRequestBus::Events::SetCausticsEnabled, entityId);
            conversionSuccess &= ConvertVarBus<WaterVolumeComponentRequestBus, float>("CausticIntensity", varBlock, &WaterVolumeComponentRequestBus::Events::SetCausticIntensity, entityId);
            conversionSuccess &= ConvertVarBus<WaterVolumeComponentRequestBus, float>("CausticTiling", varBlock, &WaterVolumeComponentRequestBus::Events::SetCausticTiling, entityId);
            conversionSuccess &= ConvertVarBus<WaterVolumeComponentRequestBus, float>("CausticHeight", varBlock, &WaterVolumeComponentRequestBus::Events::SetCausticHeight, entityId);

            conversionSuccess &= ConvertVarBus<WaterVolumeComponentRequestBus, float>("Volume", varBlock, &WaterVolumeComponentRequestBus::Events::SetSpillableVolume, entityId);
            conversionSuccess &= ConvertVarBus<WaterVolumeComponentRequestBus, float>("VolumeAcc", varBlock, &WaterVolumeComponentRequestBus::Events::SetVolumeAccuracy, entityId);
            conversionSuccess &= ConvertVarBus<WaterVolumeComponentRequestBus, float>("BorderPad", varBlock, &WaterVolumeComponentRequestBus::Events::SetExtrudeBorder, entityId);
            conversionSuccess &= ConvertVarBus<WaterVolumeComponentRequestBus, bool>("BorderConvex", varBlock, &WaterVolumeComponentRequestBus::Events::SetConvexBorder, entityId);
            conversionSuccess &= ConvertVarBus<WaterVolumeComponentRequestBus, float>("ObjVolThresh", varBlock, &WaterVolumeComponentRequestBus::Events::SetObjectSizeLimit, entityId);

            conversionSuccess &= ConvertVarBus<WaterVolumeComponentRequestBus, float>("SimCell", varBlock, &WaterVolumeComponentRequestBus::Events::SetWaveSurfaceCellSize, entityId);
            conversionSuccess &= ConvertVarBus<WaterVolumeComponentRequestBus, float>("WaveSpeed", varBlock, &WaterVolumeComponentRequestBus::Events::SetWaveSpeed, entityId);
            conversionSuccess &= ConvertVarBus<WaterVolumeComponentRequestBus, float>("WaveDamping", varBlock, &WaterVolumeComponentRequestBus::Events::SetWaveDampening, entityId);
            conversionSuccess &= ConvertVarBus<WaterVolumeComponentRequestBus, float>("WaveTimestep", varBlock, &WaterVolumeComponentRequestBus::Events::SetWaveTimestep, entityId);
            conversionSuccess &= ConvertVarBus<WaterVolumeComponentRequestBus, float>("MinWaveVel", varBlock, &WaterVolumeComponentRequestBus::Events::SetWaveSleepThreshold, entityId);
            conversionSuccess &= ConvertVarBus<WaterVolumeComponentRequestBus, float>("DepthCells", varBlock, &WaterVolumeComponentRequestBus::Events::SetWaveDepthCellSize, entityId);
            conversionSuccess &= ConvertVarBus<WaterVolumeComponentRequestBus, float>("HeightLimit", varBlock, &WaterVolumeComponentRequestBus::Events::SetWaveHeightLimit, entityId);
            conversionSuccess &= ConvertVarBus<WaterVolumeComponentRequestBus, float>("Resistance", varBlock, &WaterVolumeComponentRequestBus::Events::SetWaveForce, entityId);
            conversionSuccess &= ConvertVarBus<WaterVolumeComponentRequestBus, float>("SimAreaGrowth", varBlock, &WaterVolumeComponentRequestBus::Events::SetWaveSimulationAreaGrowth, entityId);
        }

        if (!conversionSuccess)
        {
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        //Retrieve shape points

        /*
        We're going to pull a bit of a switcheroo here.
        The current water plane defined by the legacy system is the top of the
        water volume.
        This is going to move that top plane to the bottom of the volume since
        the PolygonPrism's points define a floor.
        In this case depth turns into height.
        The resulting volume is the same.

        It's worth noting that all legacy areas primarily exist on the global XY plane
        with a Z position and a height. This is no different for water volumes. Each point
        on the legacy area has a Z value but under the hood that value will not really matter.
        The Z position is just the Z position of the entity, individual points only matter on the
        XY plane.
        */
        const size_t pointCount = static_cast<size_t>(waterShapeObject->GetPointCount());

        AZStd::vector<AZ::Vector2> floorPoints;
        floorPoints.resize(pointCount);

        float height = waterShapeObject->GetWaterVolumeDepth();

        //New points are also on the XY plane so just get all points as Vector2s
        for (size_t i = 0; i < pointCount; ++i)
        {
            const AZ::Vector3 azPoint = LYVec3ToAZVec3(waterShapeObject->GetPoint(i));
            floorPoints[i] = AZ::Vector3ToVector2(azPoint);
        }

        /*
        We need the new position of all these points to line up with all the old ones
        after rotation. This is tricky because the new points don't line up 1:1.
        Imagine the old points get pulled straight down by -height. If there was no
        rotation that would be sufficient to match with the new floor.

        However if rotation is involved, the new points will yield a surface that doesn't match
        up. If you imagine the rotated old points pulled down and you extrude that shape along the
        normal of that plane you get the surface of the new points. We need some offset so that these
        new points produce a surface that matches up with the position of the old points.

        If you rotate a water volume component you'll get a better visualization of this.

        The solution:
        Take a vector of (0,0,-height) and apply the legacy entity's rotation to it.
        This vector will put our new entity's position in place to make sure that it's new
        surface lines up with the old one.
        */
        const AZ::Quaternion azRot = LYQuaternionToAZQuaternion(waterShapeObject->GetRotation());
        const AZ::Vector3 offset = azRot * AZ::Vector3(0, 0, -height);

        //Apply the offset
        AZ::Vector3 worldPos = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(worldPos, entityId, &AZ::TransformBus::Events::GetWorldTranslation);

        worldPos += offset;
        AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetWorldTranslation, worldPos);

        //Now just apply the points and the height
        LmbrCentral::PolygonPrismShapeComponentRequestBus::Event(entityId, &LmbrCentral::PolygonPrismShapeComponentRequestBus::Events::SetVertices, floorPoints);
        LmbrCentral::PolygonPrismShapeComponentRequestBus::Event(entityId, &LmbrCentral::PolygonPrismShapeComponentRequestBus::Events::SetHeight, height);

        return AZ::LegacyConversion::LegacyConversionResult::HandledDeleteEntity;
    }

    bool WaterConverter::BeforeConversionBegins()
    {
        return true;
    }

    bool  WaterConverter::AfterConversionEnds()
    {
        return true;
    }
} // namespace Water
