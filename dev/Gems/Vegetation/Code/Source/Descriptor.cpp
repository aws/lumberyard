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

#include "Vegetation_precompiled.h"

#include <Vegetation/Descriptor.h>
#include <SurfaceData/SurfaceTag.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/StringFunc/StringFunc.h>

//////////////////////////////////////////////////////////////////////
// #pragma inline_depth(0)

namespace Vegetation
{
    namespace DescriptorUtil
    {
        static bool UpdateVersion(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() < 4)
            {
                AZ::Vector3 positionMin(-0.3f, -0.3f, 0.0f);
                if (classElement.GetChildData(AZ_CRC("PositionMin", 0x1abef6a6), positionMin))
                {
                    classElement.RemoveElementByName(AZ_CRC("PositionMin", 0x1abef6a6));
                    classElement.AddElementWithData(context, "PositionMinX", (float)positionMin.GetX());
                    classElement.AddElementWithData(context, "PositionMinY", (float)positionMin.GetY());
                    classElement.AddElementWithData(context, "PositionMinZ", (float)positionMin.GetZ());
                }

                AZ::Vector3 positionMax(0.3f, 0.3f, 0.0f);
                if (classElement.GetChildData(AZ_CRC("PositionMax", 0x26b3c9ff), positionMax))
                {
                    classElement.RemoveElementByName(AZ_CRC("PositionMax", 0x26b3c9ff));
                    classElement.AddElementWithData(context, "PositionMaxX", (float)positionMax.GetX());
                    classElement.AddElementWithData(context, "PositionMaxY", (float)positionMax.GetY());
                    classElement.AddElementWithData(context, "PositionMaxZ", (float)positionMax.GetZ());
                }

                AZ::Vector3 rotationMin(0.0f, 0.0f, -180.0f);
                if (classElement.GetChildData(AZ_CRC("RotationMin", 0xf556391b), rotationMin))
                {
                    classElement.RemoveElementByName(AZ_CRC("RotationMin", 0xf556391b));
                    classElement.AddElementWithData(context, "RotationMinX", (float)rotationMin.GetX());
                    classElement.AddElementWithData(context, "RotationMinY", (float)rotationMin.GetY());
                    classElement.AddElementWithData(context, "RotationMinZ", (float)rotationMin.GetZ());
                }

                AZ::Vector3 rotationMax(0.0f, 0.0f, 180.0f);
                if (classElement.GetChildData(AZ_CRC("RotationMax", 0xc95b0642), rotationMax))
                {
                    classElement.RemoveElementByName(AZ_CRC("RotationMax", 0xc95b0642));
                    classElement.AddElementWithData(context, "RotationMaxX", (float)rotationMax.GetX());
                    classElement.AddElementWithData(context, "RotationMaxY", (float)rotationMax.GetY());
                    classElement.AddElementWithData(context, "RotationMaxZ", (float)rotationMax.GetZ());
                }
            }
            if (classElement.GetVersion() < 5)
            {
                classElement.RemoveElementByName(AZ_CRC("RadiusMax", 0x5e90f2ea));
            }
            return true;
        }
    }

    void Descriptor::Reflect(AZ::ReflectContext* context)
    {
        // Don't reflect again if we're already reflected to the passed in context
        if (context->IsTypeReflected(VegetationDescriptorTypeId))
        {
            return;
        }

        SurfaceTagDistance::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<Descriptor>()
                ->Version(5, &DescriptorUtil::UpdateVersion)
                ->Field("MeshAsset", &Descriptor::m_meshAsset)
                ->Field("MaterialAsset", &Descriptor::m_materialAsset)
                ->Field("Weight", &Descriptor::m_weight)
                ->Field("AutoMerge", &Descriptor::m_autoMerge)
                ->Field("UseTerrainColor", &Descriptor::m_useTerrainColor)
                ->Field("Advanced", &Descriptor::m_advanced)
                ->Field("PositionOverrideEnabled", &Descriptor::m_positionOverrideEnabled)
                ->Field("PositionMinX", &Descriptor::m_positionMinX)
                ->Field("PositionMaxX", &Descriptor::m_positionMaxX)
                ->Field("PositionMinY", &Descriptor::m_positionMinY)
                ->Field("PositionMaxY", &Descriptor::m_positionMaxY)
                ->Field("PositionMinZ", &Descriptor::m_positionMinZ)
                ->Field("PositionMaxZ", &Descriptor::m_positionMaxZ)
                ->Field("RotationOverrideEnabled", &Descriptor::m_rotationOverrideEnabled)
                ->Field("RotationMinX", &Descriptor::m_rotationMinX)
                ->Field("RotationMaxX", &Descriptor::m_rotationMaxX)
                ->Field("RotationMinY", &Descriptor::m_rotationMinY)
                ->Field("RotationMaxY", &Descriptor::m_rotationMaxY)
                ->Field("RotationMinZ", &Descriptor::m_rotationMinZ)
                ->Field("RotationMaxZ", &Descriptor::m_rotationMaxZ)
                ->Field("ScaleOverrideEnabled", &Descriptor::m_scaleOverrideEnabled)
                ->Field("ScaleMin", &Descriptor::m_scaleMin)
                ->Field("ScaleMax", &Descriptor::m_scaleMax)
                ->Field("AltitudeFilterOverrideEnabled", &Descriptor::m_altitudeFilterOverrideEnabled)
                ->Field("AltitudeFilterMin", &Descriptor::m_altitudeFilterMin)
                ->Field("AltitudeFilterMax", &Descriptor::m_altitudeFilterMax)
                ->Field("WindBending", &Descriptor::m_windBending)
                ->Field("AirResistance", &Descriptor::m_airResistance)
                ->Field("Stiffness", &Descriptor::m_stiffness)
                ->Field("Damping", &Descriptor::m_damping)
                ->Field("Variance", &Descriptor::m_variance)
                ->Field("RadiusOverrideEnabled", &Descriptor::m_radiusOverrideEnabled)
                ->Field("BoundMode", &Descriptor::m_boundMode)
                ->Field("RadiusMin", &Descriptor::m_radiusMin)
                ->Field("SurfaceAlignmentOverrideEnabled", &Descriptor::m_surfaceAlignmentOverrideEnabled)
                ->Field("SurfaceAlignmentMin", &Descriptor::m_surfaceAlignmentMin)
                ->Field("SurfaceAlignmentMax", &Descriptor::m_surfaceAlignmentMax)
                ->Field("SlopeFilterOverrideEnabled", &Descriptor::m_slopeFilterOverrideEnabled)
                ->Field("SlopeFilterMin", &Descriptor::m_slopeFilterMin)
                ->Field("SlopeFilterMax", &Descriptor::m_slopeFilterMax)
                ->Field("SurfaceFilterOverrideMode", &Descriptor::m_surfaceFilterOverrideMode)
                ->Field("InclusiveSurfaceFilterTags", &Descriptor::m_inclusiveSurfaceFilterTags)
                ->Field("ExclusiveSurfaceFilterTags", &Descriptor::m_exclusiveSurfaceFilterTags)
                ->Field("SurfaceTagDistance", &Descriptor::m_surfaceTagDistance)
                ->Field("ViewDistRatio", &Descriptor::m_viewDistanceRatio)
                ->Field("LodDistRatio", &Descriptor::m_lodDistanceRatio)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<Descriptor>(
                    "Vegetation Descriptor", "Details used to create vegetation instances")
                    ->DataElement(0, &Descriptor::m_meshAsset, "Mesh Asset", "Mesh asset (CGF)")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Descriptor::MeshAssetChanged)
                    ->DataElement(0, &Descriptor::m_materialAsset, "Material Asset", "Material asset")
                    ->DataElement(0, &Descriptor::m_weight, "Weight", "Weight counted against the total density of the placed vegetation sector")
                    ->DataElement(0, &Descriptor::m_autoMerge, "AutoMerge", "Use in the auto merge mesh system or a stand alone vegetation instance")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(0, &Descriptor::m_useTerrainColor, "Use Terrain Color", "")
                    ->DataElement(0, &Descriptor::m_advanced, "Use Advanced Settings", "Display advanced settings")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Position Modifier")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->DataElement(0, &Descriptor::m_positionOverrideEnabled, "Position Modifier Override Enabled", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_positionMinX, "Position Min X", "Minimum position offset on X axis.")
                            ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -2.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 2.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_positionMaxX, "Position Max X", "Maximum position offset on X axis.")
                            ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -2.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 2.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_positionMinY, "Position Min Y", "Minimum position offset on Y axis.")
                            ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -2.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 2.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_positionMaxY, "Position Max Y", "Maximum position offset on Y axis.")
                            ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -2.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 2.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_positionMinZ, "Position Min Z", "Minimum position offset on Z axis.")
                            ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -2.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 2.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_positionMaxZ, "Position Max Z", "Maximum position offset on Z axis.")
                            ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -2.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 2.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Rotation Modifier")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->DataElement(0, &Descriptor::m_rotationOverrideEnabled, "Rotation Modifier Override Enabled", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_rotationMinX, "Rotation Min X", "Minimum rotation offset on X axis.")
                            ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -180.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 180.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_rotationMaxX, "Rotation Max X", "Maximum rotation offset on X axis.")
                            ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -180.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 180.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_rotationMinY, "Rotation Min Y", "Minimum rotation offset on Y axis.")
                            ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -180.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 180.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_rotationMaxY, "Rotation Max Y", "Maximum rotation offset on Y axis.")
                            ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -180.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 180.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_rotationMinZ, "Rotation Min Z", "Minimum rotation offset on Z axis.")
                            ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -180.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 180.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_rotationMaxZ, "Rotation Max Z", "Maximum rotation offset on Z axis.")
                            ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -180.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 180.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Scale Modifier")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->DataElement(0, &Descriptor::m_scaleOverrideEnabled, "Scale Modifier Override Enabled", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_scaleMin, "Scale Min", "")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 10.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.125f)
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_scaleMax, "Scale Max", "")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 10.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.125f)
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Altitude Filter")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->DataElement(0, &Descriptor::m_altitudeFilterOverrideEnabled, "Altitude Filter Override Enabled", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(0, &Descriptor::m_altitudeFilterMin, "Altitude Filter Min", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(0, &Descriptor::m_altitudeFilterMax, "Altitude Filter Max", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Bending Influence")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        // The Bending value controls the procedural bending deformation of the vegetation objects; this works based off the amount of environment wind (WindVector) in the level.
                        ->DataElement(0, &Descriptor::m_windBending, "Wind Bending", "Controls the wind bending deformation of the instances. No effect if AutoMerge is on.") 
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::m_autoMerge)
                        // Specifically for AutoMerged vegetation - we make them readonly when not using AutoMerged.
                        ->DataElement(0, &Descriptor::m_airResistance, "Air Resistance", "Decreases resistance to wind influence. Tied to the Wind vector and Breeze generation. No effect if AutoMerge is off.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::AutoMergeIsDisabled)
                        ->DataElement(0, &Descriptor::m_stiffness, "Stiffness", "Controls the stiffness, how much it reacts to physics interaction, for AutoMerged vegetation. Controls how much force it takes to bend the asset. No effect if AutoMerge is off.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::AutoMergeIsDisabled)
                        ->DataElement(0, &Descriptor::m_damping, "Damping", "Physics damping for AutoMerged vegetation. Reduces oscillation amplitude of bending objects. No effect if AutoMerge is off.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::AutoMergeIsDisabled)
                        ->DataElement(0, &Descriptor::m_variance, "Variance", "Applies and increases noise movement. No effect if AutoMerge is off.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::AutoMergeIsDisabled)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Distance Between Filter (Radius)")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->DataElement(0, &Descriptor::m_radiusOverrideEnabled, "Distance Between Filter Override Enabled", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &Descriptor::m_boundMode, "Bound Mode", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                             ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                            ->EnumAttribute(BoundMode::Radius, "Radius")
                            ->EnumAttribute(BoundMode::MeshRadius, "MeshRadius")
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_radiusMin, "Radius Min", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::IsRadiusReadOnly)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 16.0f) // match current default sector size in meters.

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Surface Slope Alignment")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->DataElement(0, &Descriptor::m_surfaceAlignmentOverrideEnabled, "Surface Slope Alignment Override Enabled", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_surfaceAlignmentMin, "Surface Slope Alignment Min", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_surfaceAlignmentMax, "Surface Slope Alignment Max", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Slope Filter")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->DataElement(0, &Descriptor::m_slopeFilterOverrideEnabled, "Slope Filter Override Enabled", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_slopeFilterMin, "Slope Filter Min", "")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 180.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_slopeFilterMax, "Slope Filter Max", "")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 180.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Surface Mask Filter")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &Descriptor::m_surfaceFilterOverrideMode, "Override Mode", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->EnumAttribute(OverrideMode::Disable, "Disable")
                            ->EnumAttribute(OverrideMode::Replace, "Replace")
                            ->EnumAttribute(OverrideMode::Extend, "Extend")
                        ->DataElement(0, &Descriptor::m_inclusiveSurfaceFilterTags, "Inclusion Tags", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(0, &Descriptor::m_exclusiveSurfaceFilterTags, "Exclusion Tags", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "")
                        ->DataElement(0, &Descriptor::m_surfaceTagDistance, "Surface Mask Depth Filter", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "View Settings")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->DataElement(0, &Descriptor::m_viewDistanceRatio, "Distance Ratio", "Controls the maximum view distance for vegetation instances.  No effect if AutoMerge is on.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::Min, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1024.0f)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::m_autoMerge)
                        ->DataElement(0, &Descriptor::m_lodDistanceRatio, "LOD Ratio", "Controls the distance where the vegetation LOD use less vegetation models.  No effect if AutoMerge is on.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::Min, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1024.0f)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::m_autoMerge)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<Descriptor>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("weight", BehaviorValueProperty(&Descriptor::m_weight))
                ->Property("autoMerge", BehaviorValueProperty(&Descriptor::m_autoMerge))
                ->Property("surfaceTagDistance", BehaviorValueProperty(&Descriptor::m_surfaceTagDistance))
                ->Property("surfaceFilterOverrideMode", 
                    [](Descriptor* descriptor) { return (AZ::u8)(descriptor->m_surfaceFilterOverrideMode); },
                    [](Descriptor* descriptor, const AZ::u8& i) { descriptor->m_surfaceFilterOverrideMode = (OverrideMode)i; })
                ->Property("radiusOverrideEnabled", BehaviorValueProperty(&Descriptor::m_radiusOverrideEnabled))
                ->Property("radiusMin", BehaviorValueProperty(&Descriptor::m_radiusMin))
                ->Property("boundMode",
                    [](Descriptor* descriptor) { return (AZ::u8)(descriptor->m_boundMode); },
                    [](Descriptor* descriptor, const AZ::u8& i) { descriptor->m_boundMode = (BoundMode)i; })
                ->Property("surfaceAlignmentOverrideEnabled", BehaviorValueProperty(&Descriptor::m_surfaceAlignmentOverrideEnabled))
                ->Property("surfaceAlignmentMin", BehaviorValueProperty(&Descriptor::m_surfaceAlignmentMin))
                ->Property("surfaceAlignmentMax", BehaviorValueProperty(&Descriptor::m_surfaceAlignmentMax))
                ->Property("rotationOverrideEnabled", BehaviorValueProperty(&Descriptor::m_rotationOverrideEnabled))
                ->Property("rotationMinX", BehaviorValueProperty(&Descriptor::m_rotationMinX))
                ->Property("rotationMaxX", BehaviorValueProperty(&Descriptor::m_rotationMaxX))
                ->Property("rotationMinY", BehaviorValueProperty(&Descriptor::m_rotationMinY))
                ->Property("rotationMaxY", BehaviorValueProperty(&Descriptor::m_rotationMaxY))
                ->Property("rotationMinZ", BehaviorValueProperty(&Descriptor::m_rotationMinZ))
                ->Property("rotationMaxZ", BehaviorValueProperty(&Descriptor::m_rotationMaxZ))
                ->Property("positionOverrideEnabled", BehaviorValueProperty(&Descriptor::m_positionOverrideEnabled))
                ->Property("positionMinX", BehaviorValueProperty(&Descriptor::m_positionMinX))
                ->Property("positionMaxX", BehaviorValueProperty(&Descriptor::m_positionMaxX))
                ->Property("positionMinY", BehaviorValueProperty(&Descriptor::m_positionMinY))
                ->Property("positionMaxY", BehaviorValueProperty(&Descriptor::m_positionMaxY))
                ->Property("positionMinZ", BehaviorValueProperty(&Descriptor::m_positionMinZ))
                ->Property("positionMaxZ", BehaviorValueProperty(&Descriptor::m_positionMaxZ))
                ->Property("scaleOverrideEnabled", BehaviorValueProperty(&Descriptor::m_scaleOverrideEnabled))
                ->Property("scaleMin", BehaviorValueProperty(&Descriptor::m_scaleMin))
                ->Property("scaleMax", BehaviorValueProperty(&Descriptor::m_scaleMax))
                ->Property("altitudeFilterOverrideEnabled", BehaviorValueProperty(&Descriptor::m_altitudeFilterOverrideEnabled))
                ->Property("altitudeFilterMin", BehaviorValueProperty(&Descriptor::m_altitudeFilterMin))
                ->Property("altitudeFilterMax", BehaviorValueProperty(&Descriptor::m_altitudeFilterMax))
                ->Property("slopeFilterOverrideEnabled", BehaviorValueProperty(&Descriptor::m_slopeFilterOverrideEnabled))
                ->Property("slopeFilterMin", BehaviorValueProperty(&Descriptor::m_slopeFilterMin))
                ->Property("slopeFilterMax", BehaviorValueProperty(&Descriptor::m_slopeFilterMax))
                ->Property("windBending", BehaviorValueProperty(&Descriptor::m_windBending))
                ->Property("airResistance", BehaviorValueProperty(&Descriptor::m_airResistance))
                ->Property("stiffness", BehaviorValueProperty(&Descriptor::m_stiffness))
                ->Property("damping", BehaviorValueProperty(&Descriptor::m_damping))
                ->Property("variance", BehaviorValueProperty(&Descriptor::m_variance))
                ->Property("useTerrainColor", BehaviorValueProperty(&Descriptor::m_useTerrainColor))
                ->Method("GetMeshAssetPath", &Descriptor::GetMeshAssetPath)
                ->Method("SetMeshAssetPath", &Descriptor::SetMeshAssetPath)
                ->Method("GetMaterialAssetPath", &Descriptor::GetMaterialAssetPath)
                ->Method("SetMaterialAssetPath", &Descriptor::SetMaterialAssetPath)
                ->Method("GetNumInclusiveSurfaceFilterTags", &Descriptor::GetNumInclusiveSurfaceFilterTags)
                ->Method("GetInclusiveSurfaceFilterTag", &Descriptor::GetInclusiveSurfaceFilterTag)
                ->Method("RemoveInclusiveSurfaceFilterTag", &Descriptor::RemoveInclusiveSurfaceFilterTag)
                ->Method("AddInclusiveSurfaceFilterTag", &Descriptor::AddInclusiveSurfaceFilterTag)
                ->Method("GetNumExclusiveSurfaceFilterTags", &Descriptor::GetNumExclusiveSurfaceFilterTags)
                ->Method("GetExclusiveSurfaceFilterTag", &Descriptor::GetExclusiveSurfaceFilterTag)
                ->Method("RemoveExclusiveSurfaceFilterTag", &Descriptor::RemoveExclusiveSurfaceFilterTag)
                ->Method("AddExclusiveSurfaceFilterTag", &Descriptor::AddExclusiveSurfaceFilterTag)
                ;
        }
    }

    void SurfaceTagDistance::Reflect(AZ::ReflectContext * context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceTagDistance>()
                ->Version(0)
                ->Field("SurfaceTag", &SurfaceTagDistance::m_tags)
                ->Field("UpperDistanceInMeters", &SurfaceTagDistance::m_upperDistanceInMeters)
                ->Field("LowerDistanceInMeters", &SurfaceTagDistance::m_lowerDistanceInMeters)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<SurfaceTagDistance>(
                    "VegetationSurfaceTagDepth", "Describes depth information for a vegetation object based on a tag to match with a surface mask")
                    ->DataElement(0, &SurfaceTagDistance::m_tags, "Surface Tags", "The surface tags to compare the distance from the planting tag to.")
                    ->DataElement(0, &SurfaceTagDistance::m_upperDistanceInMeters, "Upper Distance Range (m)", "Upper Distance in meters from comparison surface, negative for below")
                    ->DataElement(0, &SurfaceTagDistance::m_lowerDistanceInMeters, "Lower Distance Range (m)", "Lower Distance in meters from comparison surface, negative for below")
                    ;
            }
        }
        
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SurfaceTagDistance>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("upperDistanceInMeters", BehaviorValueProperty(&SurfaceTagDistance::m_upperDistanceInMeters))
                ->Property("lowerDistanceInMeters", BehaviorValueProperty(&SurfaceTagDistance::m_lowerDistanceInMeters))
                ->Method("GetNumTags", &SurfaceTagDistance::GetNumTags)
                ->Method("GetTag", &SurfaceTagDistance::GetTag)
                ->Method("RemoveTag", &SurfaceTagDistance::RemoveTag)
                ->Method("AddTag", &SurfaceTagDistance::AddTag)
                ;
        }
    }

    size_t SurfaceTagDistance::GetNumTags() const
    {
        return m_tags.size();
    }

    AZ::Crc32 SurfaceTagDistance::GetTag(int tagIndex) const
    {
        if (tagIndex < m_tags.size() && tagIndex >= 0)
        {
            return m_tags[tagIndex];
        }

        return AZ::Crc32();
    }

    void SurfaceTagDistance::RemoveTag(int tagIndex)
    {
        if (tagIndex < m_tags.size() && tagIndex >= 0)
        {
            m_tags.erase(m_tags.begin() + tagIndex);
        }
    }

    void SurfaceTagDistance::AddTag(const AZStd::string& tag)
    {
        m_tags.push_back(SurfaceData::SurfaceTag(tag));
    }

    bool SurfaceTagDistance::operator==(const SurfaceTagDistance& rhs) const
    {
        return m_tags == rhs.m_tags && m_lowerDistanceInMeters == rhs.m_lowerDistanceInMeters && m_upperDistanceInMeters == rhs.m_upperDistanceInMeters;
    }

    Descriptor::Descriptor()
    {
        ResetAssets();
    }

    Descriptor::~Descriptor()
    {
        ResetAssets();
    }

    bool Descriptor::operator==(const Descriptor& rhs) const
    {
        return
            m_meshAsset == rhs.m_meshAsset &&
            m_materialAsset.GetAssetPath() == rhs.m_materialAsset.GetAssetPath() &&
            m_weight == rhs.m_weight &&
            m_autoMerge == rhs.m_autoMerge &&
            m_viewDistanceRatio == rhs.m_viewDistanceRatio &&
            m_lodDistanceRatio == rhs.m_lodDistanceRatio &&
            m_surfaceTagDistance == rhs.m_surfaceTagDistance &&
            m_surfaceFilterOverrideMode == rhs.m_surfaceFilterOverrideMode &&
            m_inclusiveSurfaceFilterTags == rhs.m_inclusiveSurfaceFilterTags &&
            m_exclusiveSurfaceFilterTags == rhs.m_exclusiveSurfaceFilterTags &&
            m_radiusOverrideEnabled == rhs.m_radiusOverrideEnabled &&
            m_radiusMin == rhs.m_radiusMin &&
            m_boundMode == rhs.m_boundMode &&
            m_surfaceAlignmentOverrideEnabled == rhs.m_surfaceAlignmentOverrideEnabled &&
            m_surfaceAlignmentMin == rhs.m_surfaceAlignmentMin &&
            m_surfaceAlignmentMax == rhs.m_surfaceAlignmentMax &&
            m_rotationOverrideEnabled == rhs.m_rotationOverrideEnabled &&
            m_rotationMinX == rhs.m_rotationMinX &&
            m_rotationMaxX == rhs.m_rotationMaxX &&
            m_rotationMinY == rhs.m_rotationMinY &&
            m_rotationMaxY == rhs.m_rotationMaxY &&
            m_rotationMinZ == rhs.m_rotationMinZ &&
            m_rotationMaxZ == rhs.m_rotationMaxZ &&
            m_positionOverrideEnabled == rhs.m_positionOverrideEnabled &&
            m_positionMinX == rhs.m_positionMinX &&
            m_positionMaxX == rhs.m_positionMaxX &&
            m_positionMinY == rhs.m_positionMinY &&
            m_positionMaxY == rhs.m_positionMaxY &&
            m_positionMinZ == rhs.m_positionMinZ &&
            m_positionMaxZ == rhs.m_positionMaxZ &&
            m_scaleOverrideEnabled == rhs.m_scaleOverrideEnabled &&
            m_scaleMin == rhs.m_scaleMin &&
            m_scaleMax == rhs.m_scaleMax &&
            m_altitudeFilterOverrideEnabled == rhs.m_altitudeFilterOverrideEnabled &&
            m_altitudeFilterMin == rhs.m_altitudeFilterMin &&
            m_altitudeFilterMax == rhs.m_altitudeFilterMax &&
            m_slopeFilterOverrideEnabled == rhs.m_slopeFilterOverrideEnabled &&
            m_slopeFilterMin == rhs.m_slopeFilterMin &&
            m_slopeFilterMax == rhs.m_slopeFilterMax &&
            m_windBending == rhs.m_windBending &&
            m_airResistance == rhs.m_airResistance &&
            m_stiffness == rhs.m_stiffness &&
            m_damping == rhs.m_damping &&
            m_variance == rhs.m_variance &&
            m_useTerrainColor == rhs.m_useTerrainColor
            ;
    }

    void Descriptor::ResetAssets(bool resetMaterialOverride)
    {
        m_meshLoaded = false;
        m_meshRadius = 0.0f;
        m_meshAsset.Release();
        m_meshAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::QueueLoad);
        if (resetMaterialOverride)
        {
            m_materialOverride = nullptr;
        }
    }

    void Descriptor::LoadAssets()
    {
        ResetAssets();
        m_meshAsset.QueueLoad();

        auto system = GetISystem();
        auto engine = system ? system->GetI3DEngine() : nullptr;
        if (engine)
        {
            const AZStd::string& materialPath = m_materialAsset.GetAssetPath();

            auto materialManager = engine->GetMaterialManager();
            if (!materialPath.empty() && materialManager)
            {
                m_materialOverride = materialManager->LoadMaterial(materialPath.c_str());
                AZ_Warning("vegetation", m_materialOverride != materialManager->GetDefaultMaterial(), "Failed to load override Material \"%s\".", materialPath.c_str());
            }
        }
    }

    void Descriptor::UpdateAssets()
    {
        m_meshLoaded = m_meshAsset.IsReady() && m_meshAsset.Get()->m_statObj;
        m_meshRadius = m_meshLoaded ? m_meshAsset.Get()->m_statObj->GetRadius() : 0.0f;
    }

    AZStd::string Descriptor::GetMeshAssetPath() const
    {
        AZStd::string assetPathString;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPathString, &AZ::Data::AssetCatalogRequests::GetAssetPathById, m_meshAsset.GetId());
        return assetPathString;
    }

    void Descriptor::SetMeshAssetPath(const AZStd::string& assetPath)
    {
        AZ::Data::AssetId assetId;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, assetPath.c_str(), AZ::Data::s_invalidAssetType, false);
        if (assetId.IsValid())
        {
            m_meshAsset.Create(assetId, false);
        }
    }

    AZStd::string Descriptor::GetMaterialAssetPath() const
    {
        return m_materialAsset.GetAssetPath();
    }

    void Descriptor::SetMaterialAssetPath(const AZStd::string& path)
    {
        m_materialAsset.SetAssetPath(path.c_str());
    }

    size_t Descriptor::GetNumInclusiveSurfaceFilterTags() const
    {
        return m_inclusiveSurfaceFilterTags.size();
    }

    AZ::Crc32 Descriptor::GetInclusiveSurfaceFilterTag(int tagIndex) const
    {
        if (tagIndex < m_inclusiveSurfaceFilterTags.size() && tagIndex >= 0)
        {
            return m_inclusiveSurfaceFilterTags[tagIndex];
        }

        return AZ::Crc32();
    }

    void Descriptor::RemoveInclusiveSurfaceFilterTag(int tagIndex)
    {
        if (tagIndex < m_inclusiveSurfaceFilterTags.size() && tagIndex >= 0)
        {
            m_inclusiveSurfaceFilterTags.erase(m_inclusiveSurfaceFilterTags.begin() + tagIndex);
        }
    }

    void Descriptor::AddInclusiveSurfaceFilterTag(const AZStd::string& tag)
    {
        m_inclusiveSurfaceFilterTags.push_back(SurfaceData::SurfaceTag(tag));
    }

    size_t Descriptor::GetNumExclusiveSurfaceFilterTags() const
    {
        return m_exclusiveSurfaceFilterTags.size();
    }

    AZ::Crc32 Descriptor::GetExclusiveSurfaceFilterTag(int tagIndex) const
    {
        if (tagIndex < m_exclusiveSurfaceFilterTags.size() && tagIndex >= 0)
        {
            return m_exclusiveSurfaceFilterTags[tagIndex];
        }

        return AZ::Crc32();
    }

    void Descriptor::RemoveExclusiveSurfaceFilterTag(int tagIndex)
    {
        if (tagIndex < m_exclusiveSurfaceFilterTags.size() && tagIndex >= 0)
        {
            m_exclusiveSurfaceFilterTags.erase(m_exclusiveSurfaceFilterTags.begin() + tagIndex);
        }
    }

    void Descriptor::AddExclusiveSurfaceFilterTag(const AZStd::string& tag)
    {
        m_exclusiveSurfaceFilterTags.push_back(SurfaceData::SurfaceTag(tag));
    }

    const char* Descriptor::GetMeshName()
    {
        UpdateMeshAssetName();
        return m_meshAssetName.c_str();
    }

    AZ::u32 Descriptor::MeshAssetChanged()
    {
        UpdateMeshAssetName(true);
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    bool Descriptor::AutoMergeIsDisabled() const
    {
        return !m_autoMerge;
    }

    AZ::u32 Descriptor::GetAdvancedGroupVisibility() const
    {
        return m_advanced ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    void Descriptor::UpdateMeshAssetName(bool forceUpdate)
    {
        if (!forceUpdate && !m_meshAssetName.empty())
        {
            return;
        }

        m_meshAssetName = "<asset name>";

        if (m_meshAsset.GetId().IsValid())
        {
            // Get the asset file name
            m_meshAssetName = m_meshAsset.GetHint();
            if (!m_meshAsset.GetHint().empty())
            {
                AzFramework::StringFunc::Path::GetFileName(m_meshAsset.GetHint().c_str(), m_meshAssetName);
            }
        }
    }
}

