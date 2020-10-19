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
#include <Vegetation/LegacyVegetationInstanceSpawner.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <Vegetation/Ebuses/DescriptorNotificationBus.h>
#include <Vegetation/Ebuses/InstanceSystemRequestBus.h>
#include <Vegetation/InstanceData.h>

#include <MathConversion.h>
#include <I3DEngine.h>
#include <ISystem.h>
#include <Tarray.h>

namespace Vegetation
{

    LegacyVegetationInstanceSpawner::LegacyVegetationInstanceSpawner()
    {
        m_engine = GetISystem() ? GetISystem()->GetI3DEngine() : nullptr;
        m_statInstGroupId = StatInstGroupEvents::s_InvalidStatInstGroupId;

        UnloadAssets();
    }

    LegacyVegetationInstanceSpawner::~LegacyVegetationInstanceSpawner()
    {
        UnloadAssets();
    }

    void LegacyVegetationInstanceSpawner::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<LegacyVegetationInstanceSpawner, InstanceSpawner>()
                ->Version(0)
                ->Field("MeshAsset", &LegacyVegetationInstanceSpawner::m_meshAsset)
                ->Field("MaterialAsset", &LegacyVegetationInstanceSpawner::m_materialAsset)
                ->Field("AutoMerge", &LegacyVegetationInstanceSpawner::m_autoMerge)
                ->Field("UseTerrainColor", &LegacyVegetationInstanceSpawner::m_useTerrainColor)
                ->Field("WindBending", &LegacyVegetationInstanceSpawner::m_windBending)
                ->Field("AirResistance", &LegacyVegetationInstanceSpawner::m_airResistance)
                ->Field("Stiffness", &LegacyVegetationInstanceSpawner::m_stiffness)
                ->Field("Damping", &LegacyVegetationInstanceSpawner::m_damping)
                ->Field("Variance", &LegacyVegetationInstanceSpawner::m_variance)
                ->Field("ViewDistRatio", &LegacyVegetationInstanceSpawner::m_viewDistanceRatio)
                ->Field("LodDistRatio", &LegacyVegetationInstanceSpawner::m_lodDistanceRatio)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<LegacyVegetationInstanceSpawner>(
                    "Vegetation", "Vegetation Instance")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(0, &LegacyVegetationInstanceSpawner::m_meshAsset, "Mesh Asset", "Mesh asset (CGF)")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &LegacyVegetationInstanceSpawner::MeshAssetChanged)
                    ->DataElement(0, &LegacyVegetationInstanceSpawner::m_materialAsset, "Material Asset", "Material asset")
                    ->DataElement(0, &LegacyVegetationInstanceSpawner::m_autoMerge, "AutoMerge", "Use in the auto merge mesh system or a stand alone vegetation instance")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(0, &LegacyVegetationInstanceSpawner::m_useTerrainColor, "Use Terrain Color", "")

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Bending Influence")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        // The Bending value controls the procedural bending deformation of the vegetation objects; this works based off the amount of environment wind (WindVector) in the level.
                        ->DataElement(0, &LegacyVegetationInstanceSpawner::m_windBending, "Wind Bending", "Controls the wind bending deformation of the instances. No effect if AutoMerge is on.")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &LegacyVegetationInstanceSpawner::m_autoMerge)
                        // Specifically for AutoMerged vegetation - we make them readonly when not using AutoMerged.
                        ->DataElement(0, &LegacyVegetationInstanceSpawner::m_airResistance, "Air Resistance", "Decreases resistance to wind influence. Tied to the Wind vector and Breeze generation. No effect if AutoMerge is off.")
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &LegacyVegetationInstanceSpawner::AutoMergeIsDisabled)
                        ->DataElement(0, &LegacyVegetationInstanceSpawner::m_stiffness, "Stiffness", "Controls the stiffness, how much it reacts to physics interaction, for AutoMerged vegetation. Controls how much force it takes to bend the asset. No effect if AutoMerge is off.")
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &LegacyVegetationInstanceSpawner::AutoMergeIsDisabled)
                        ->DataElement(0, &LegacyVegetationInstanceSpawner::m_damping, "Damping", "Physics damping for AutoMerged vegetation. Reduces oscillation amplitude of bending objects. No effect if AutoMerge is off.")
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &LegacyVegetationInstanceSpawner::AutoMergeIsDisabled)
                        ->DataElement(0, &LegacyVegetationInstanceSpawner::m_variance, "Variance", "Applies and increases noise movement. No effect if AutoMerge is off.")
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &LegacyVegetationInstanceSpawner::AutoMergeIsDisabled)

                    // These only apply to CVegetation instances - we make them readonly when using AutoMerged.
                    ->ClassElement(AZ::Edit::ClassElements::Group, "View Settings")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->DataElement(0, &LegacyVegetationInstanceSpawner::m_viewDistanceRatio, "Distance Ratio", "Controls the maximum view distance for vegetation instances.  No effect if AutoMerge is on.")
                            ->Attribute(AZ::Edit::Attributes::Min, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1024.0f)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &LegacyVegetationInstanceSpawner::m_autoMerge)
                        ->DataElement(0, &LegacyVegetationInstanceSpawner::m_lodDistanceRatio, "LOD Ratio", "Controls the distance where the vegetation LOD use less vegetation models.  No effect if AutoMerge is on.")
                            ->Attribute(AZ::Edit::Attributes::Min, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1024.0f)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &LegacyVegetationInstanceSpawner::m_autoMerge)
                    ;
            }
        }
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<LegacyVegetationInstanceSpawner>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Attribute(AZ::Script::Attributes::Module, "vegetation")
                ->Constructor()
                ->Property("autoMerge", BehaviorValueProperty(&LegacyVegetationInstanceSpawner::m_autoMerge))
                ->Property("windBending", BehaviorValueProperty(&LegacyVegetationInstanceSpawner::m_windBending))
                ->Property("airResistance", BehaviorValueProperty(&LegacyVegetationInstanceSpawner::m_airResistance))
                ->Property("stiffness", BehaviorValueProperty(&LegacyVegetationInstanceSpawner::m_stiffness))
                ->Property("damping", BehaviorValueProperty(&LegacyVegetationInstanceSpawner::m_damping))
                ->Property("variance", BehaviorValueProperty(&LegacyVegetationInstanceSpawner::m_variance))
                ->Property("useTerrainColor", BehaviorValueProperty(&LegacyVegetationInstanceSpawner::m_useTerrainColor))
                ->Property("viewDistanceRatio", BehaviorValueProperty(&LegacyVegetationInstanceSpawner::m_viewDistanceRatio))
                ->Property("lodRatio", BehaviorValueProperty(&LegacyVegetationInstanceSpawner::m_lodDistanceRatio))
                ->Method("GetMeshAssetPath", &LegacyVegetationInstanceSpawner::GetMeshAssetPath)
                ->Method("SetMeshAssetPath", &LegacyVegetationInstanceSpawner::SetMeshAssetPath)
                ->Method("GetMaterialAssetPath", &LegacyVegetationInstanceSpawner::GetMaterialAssetPath)
                ->Method("SetMaterialAssetPath", &LegacyVegetationInstanceSpawner::SetMaterialAssetPath)
                ;
        }
    }

    AZStd::shared_ptr<InstanceSpawner> LegacyVegetationInstanceSpawner::ConvertLegacyDescriptorData(AZ::SerializeContext::DataElementNode& classElement)
    {
        auto convertedInstanceSpawner = AZStd::make_shared<LegacyVegetationInstanceSpawner>();

        if (classElement.GetChildData(AZ_CRC("AutoMerge", 0x856f374e), convertedInstanceSpawner->m_autoMerge))
        {
            classElement.RemoveElementByName(AZ_CRC("AutoMerge", 0x856f374e));
        }
        if (classElement.GetChildData(AZ_CRC("MeshAsset", 0x2e843642), convertedInstanceSpawner->m_meshAsset))
        {
            classElement.RemoveElementByName(AZ_CRC("MeshAsset", 0x2e843642));
        }
        if (classElement.GetChildData(AZ_CRC("MaterialAsset", 0x0be24772), convertedInstanceSpawner->m_materialAsset))
        {
            classElement.RemoveElementByName(AZ_CRC("MaterialAsset", 0x0be24772));
        }
        if (classElement.GetChildData(AZ_CRC("UseTerrainColor", 0xdd19ded9), convertedInstanceSpawner->m_useTerrainColor))
        {
            classElement.RemoveElementByName(AZ_CRC("UseTerrainColor", 0xdd19ded9));
        }
        if (classElement.GetChildData(AZ_CRC("WindBending", 0xe8940571), convertedInstanceSpawner->m_windBending))
        {
            classElement.RemoveElementByName(AZ_CRC("WindBending", 0xe8940571));
        }
        if (classElement.GetChildData(AZ_CRC("AirResistance", 0x24221466), convertedInstanceSpawner->m_airResistance))
        {
            classElement.RemoveElementByName(AZ_CRC("AirResistance", 0x24221466));
        }
        if (classElement.GetChildData(AZ_CRC("Stiffness", 0x89379000), convertedInstanceSpawner->m_stiffness))
        {
            classElement.RemoveElementByName(AZ_CRC("Stiffness", 0x89379000));
        }
        if (classElement.GetChildData(AZ_CRC("Damping", 0x440e3a6a), convertedInstanceSpawner->m_damping))
        {
            classElement.RemoveElementByName(AZ_CRC("Damping", 0x440e3a6a));
        }
        if (classElement.GetChildData(AZ_CRC("Variance", 0x42cf6226), convertedInstanceSpawner->m_variance))
        {
            classElement.RemoveElementByName(AZ_CRC("Variance", 0x42cf6226));
        }
        if (classElement.GetChildData(AZ_CRC("ViewDistRatio", 0x2cd2b6e2), convertedInstanceSpawner->m_viewDistanceRatio))
        {
            classElement.RemoveElementByName(AZ_CRC("ViewDistRatio", 0x2cd2b6e2));
        }
        if (classElement.GetChildData(AZ_CRC("LodDistRatio", 0xf4f78029), convertedInstanceSpawner->m_lodDistanceRatio))
        {
            classElement.RemoveElementByName(AZ_CRC("LodDistRatio", 0xf4f78029));
        }

        return convertedInstanceSpawner;
    }

    bool LegacyVegetationInstanceSpawner::DataIsEquivalent(const InstanceSpawner & baseRhs) const
    {
        // NOTE:  This specifically does *not* check m_statInstGroupId when comparing for data equivalency.
        // This is because the data equivalency determines whether or not two instance spawners *can*
        // be the same spawner.  The id will likely be unset for one of the two, and valid for the
        // other one.  The goal would be for the spawner that's equal with an unset id to go away
        // and be replaced by the already-existing-and-valid spawner.

        if (const auto* rhs = azrtti_cast<const LegacyVegetationInstanceSpawner*>(&baseRhs))
        {
            return
                m_meshAsset == rhs->m_meshAsset &&
                m_materialAsset.GetAssetPath() == rhs->m_materialAsset.GetAssetPath() &&
                m_autoMerge == rhs->m_autoMerge &&
                m_viewDistanceRatio == rhs->m_viewDistanceRatio &&
                m_lodDistanceRatio == rhs->m_lodDistanceRatio &&
                m_windBending == rhs->m_windBending &&
                m_airResistance == rhs->m_airResistance &&
                m_stiffness == rhs->m_stiffness &&
                m_damping == rhs->m_damping &&
                m_variance == rhs->m_variance &&
                m_useTerrainColor == rhs->m_useTerrainColor
                ;
        }

        // Not the same subtypes, so definitely not a data match.
        return false;
    }

    void LegacyVegetationInstanceSpawner::LoadAssets()
    {
        UnloadAssets();

        m_meshAsset.QueueLoad();
        m_materialOverride = LoadMaterialAsset(m_materialAsset.GetAssetPath());
        AZ::Data::AssetBus::MultiHandler::BusConnect(m_meshAsset.GetId());
    }

    void LegacyVegetationInstanceSpawner::UnloadAssets()
    {
        m_materialOverride = nullptr;
        ResetMeshAsset();
        ReleaseStatInstGroupId();
        NotifyOnAssetsUnloaded();
    }

    void LegacyVegetationInstanceSpawner::RequestStatInstGroupId()
    {
        if (m_meshSpawnable && (m_statInstGroupId == StatInstGroupEvents::s_InvalidStatInstGroupId))
        {
            StatInstGroupEventBus::BroadcastResult(m_statInstGroupId, &StatInstGroupEventBus::Events::GenerateStatInstGroupId);
            if (m_statInstGroupId == StatInstGroupEvents::s_InvalidStatInstGroupId)
            {
                AZ_Error("vegetation", false, "GenerateStatInstGroupId failed!");
            }
        }
    }

    void LegacyVegetationInstanceSpawner::ReleaseStatInstGroupId()
    {
        if (m_statInstGroupId != StatInstGroupEvents::s_InvalidStatInstGroupId)
        {
            if (m_engine)
            {
                IStatInstGroup group;
                m_engine->SetStatInstGroup(m_statInstGroupId, group);
            }
            StatInstGroupEventBus::Broadcast(&StatInstGroupEventBus::Events::ReleaseStatInstGroupId, m_statInstGroupId);
            m_statInstGroupId = StatInstGroupEvents::s_InvalidStatInstGroupId;
        }
    }

    _smart_ptr<IMaterial> LegacyVegetationInstanceSpawner::LoadMaterialAsset(const AZStd::string& materialPath)
    {
        _smart_ptr<IMaterial> materialAsset = nullptr;

        if (!materialPath.empty())
        {
            auto system = GetISystem();
            auto engine = system ? system->GetI3DEngine() : nullptr;
            if (engine)
            {
                auto materialManager = engine->GetMaterialManager();
                if (materialManager)
                {
                    const bool makeIfNotFound = true;
                    materialAsset = materialManager->LoadMaterial(materialPath.c_str(), !makeIfNotFound);
                    AZ_Error("vegetation", materialAsset, "Failed to load override Material \"%s\".", materialPath.c_str());
                }
            }
        }

        return materialAsset;
    }


    void LegacyVegetationInstanceSpawner::ResetMeshAsset()
    {
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();

        m_meshAsset.Release();
        UpdateCachedValues();
        m_meshAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::QueueLoad);
    }

    void LegacyVegetationInstanceSpawner::UpdateCachedValues()
    {
        // Once our assets are loaded and at the point that they're getting registered,
        // cache off the spawnable state and the mesh radius for use from multiple threads.

        // Note that "is loaded" (asset.IsReady()) is different than "is spawnable", 
        // which is that the asset is ready *and* has a valid StatObj.
        m_meshLoaded = m_meshAsset.IsReady();
        m_meshSpawnable = m_meshLoaded && m_meshAsset.Get()->m_statObj;
        m_meshRadius = m_meshSpawnable ? m_meshAsset.Get()->m_statObj->GetRadius() : 0.0f;
    }

    void LegacyVegetationInstanceSpawner::OnRegisterUniqueDescriptor()
    {
        UpdateCachedValues();
        RequestStatInstGroupId();
        if (m_statInstGroupId != StatInstGroupEvents::s_InvalidStatInstGroupId)
        {
            // Update this stat group with any changes
            IStatInstGroup group;
            group.pStatObj = m_meshAsset.Get()->m_statObj;
            group.bAutoMerged = m_autoMerge;
            group.pMaterial = m_materialOverride;
            group.fBending = m_windBending;
            group.fAirResistance = m_airResistance;
            group.fStiffness = m_stiffness;
            group.fVariance = m_variance;
            group.fDamping = m_damping;
            group.fLodDistRatio = m_lodDistanceRatio;
            group.fMaxViewDistRatio = m_viewDistanceRatio;
            group.bUseTerrainColor = m_useTerrainColor;

            if (!m_engine->SetStatInstGroup(m_statInstGroupId, group))
            {
                AZ_Error("vegetation", false, "SetStatInstGroup failed!");
                StatInstGroupEventBus::Broadcast(&StatInstGroupEventBus::Events::ReleaseStatInstGroupId, m_statInstGroupId);
                m_statInstGroupId = StatInstGroupEvents::s_InvalidStatInstGroupId;
            }
        }
    }

    void LegacyVegetationInstanceSpawner::OnReleaseUniqueDescriptor()
    {
        ReleaseStatInstGroupId();
    }

    bool LegacyVegetationInstanceSpawner::HasEmptyAssetReferences() const
    {
        // If we don't have a valid Mesh Asset, then we're spawning empty instances.
        return !m_meshAsset.GetId().IsValid();
    }

    bool LegacyVegetationInstanceSpawner::IsLoaded() const
    {
        return m_meshLoaded;
    }

    bool LegacyVegetationInstanceSpawner::IsSpawnable() const
    {
        return m_meshSpawnable;
    }

    float LegacyVegetationInstanceSpawner::GetRadius() const
    {
        return m_meshRadius;
    }

    AZStd::string LegacyVegetationInstanceSpawner::GetName() const
    {
        AZStd::string assetName;
        if (!HasEmptyAssetReferences())
        {
            // Get the asset file name
            assetName = m_meshAsset.GetHint();
            if (!m_meshAsset.GetHint().empty())
            {
                AzFramework::StringFunc::Path::GetFileName(m_meshAsset.GetHint().c_str(), assetName);
            }
        }
        else
        {
            assetName = "<asset name>";
        }

        return assetName;
    }

    void LegacyVegetationInstanceSpawner::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (m_meshAsset.GetId() == asset.GetId())
        {
            ResetMeshAsset();
            m_meshAsset = asset;
            UpdateCachedValues();
            NotifyOnAssetsLoaded();
        }
    }

    void LegacyVegetationInstanceSpawner::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        OnAssetReady(asset);
    }

    AZStd::string LegacyVegetationInstanceSpawner::GetMeshAssetPath() const
    {
        AZStd::string assetPathString;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPathString, &AZ::Data::AssetCatalogRequests::GetAssetPathById, m_meshAsset.GetId());
        return assetPathString;
    }

    void LegacyVegetationInstanceSpawner::SetMeshAssetPath(const AZStd::string& assetPath)
    {
        if (!assetPath.empty())
        {
            AZ::Data::AssetId assetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, assetPath.c_str(), AZ::Data::s_invalidAssetType, false);
            if (assetId.IsValid())
            {
                AZ::Data::AssetInfo assetInfo;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
                if (assetInfo.m_assetType == m_meshAsset.GetType())
                {
                    m_meshAsset.Create(assetId, false);
                    LoadAssets();
                }
                else
                {
                    AZ_Error("Vegetation", false, "Asset '%s' is of type %s, but expected a MeshAsset type.",
                        assetPath.c_str(), assetInfo.m_assetType.ToString<AZStd::string>().c_str());
                }
            }
            else
            {
                AZ_Error("Vegetation", false, "Asset '%s' is invalid.", assetPath.c_str());
            }
        }
        else
        {
            m_meshAsset = AZ::Data::Asset<LmbrCentral::MeshAsset>();
            LoadAssets();
        }
    }

    AZStd::string LegacyVegetationInstanceSpawner::GetMaterialAssetPath() const
    {
        return m_materialAsset.GetAssetPath();
    }

    void LegacyVegetationInstanceSpawner::SetMaterialAssetPath(const AZStd::string& path)
    {
        if (!path.empty())
        {
            // Only set the new path if we can successfully load a material from it.
            // We don't need to print an error if it fails, LoadMaterialAsset already does that.
            auto materialOverride = LoadMaterialAsset(path);
            if (materialOverride)
            {
                m_materialAsset.SetAssetPath(path.c_str());
            }
        }
        else
        {
            // Clear the material reference
            m_materialAsset.SetAssetPath(path.c_str());
        }
        LoadAssets();
    }

    bool LegacyVegetationInstanceSpawner::AutoMergeIsDisabled() const
    {
        return !m_autoMerge;
    }

    AZ::u32 LegacyVegetationInstanceSpawner::MeshAssetChanged()
    {
        // Whenever we change the mesh asset, force a refresh of the Entity Inspector
        // since we want the Descriptor List to refresh the name of the entry.
        NotifyOnAssetsUnloaded();
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    InstancePtr LegacyVegetationInstanceSpawner::CreateInstance(const InstanceData& instanceData)
    {
        InstancePtr opaqueInstanceData = nullptr;

        if (m_statInstGroupId == StatInstGroupEvents::s_InvalidStatInstGroupId)
        {
            //could not locate registered vegetation render group but it's not an error
            //an edit, asset change, or other event could have released descriptors or render groups on this or another thread
            //this should result in a composition change and refresh
            //so skip this instance
            return nullptr;
        }

        if (m_autoMerge)
        {
            IMergedMeshesManager::SInstanceSample sample;
            sample.instGroupId = m_statInstGroupId;
            sample.pos = AZVec3ToLYVec3(instanceData.m_position);
            sample.scale = (uint8)SATURATEB(instanceData.m_scale * 64.0f); // [LY-90912] Need to expose VEGETATION_CONV_FACTOR from CryEngine\Cry3DEngine\Vegetation.h for use here
            sample.q = AZQuaternionToLYQuaternion(instanceData.m_alignment * instanceData.m_rotation);
            sample.q.NormalizeSafe();

            const bool bRegister = false;
            IRenderNode* mergedMeshNode = nullptr;
            IRenderNode* instanceNode = m_engine->GetIMergedMeshesManager()->AddDynamicInstance(sample, &mergedMeshNode, bRegister);
            AZ_Assert(instanceNode, "Could not AddDynamicInstance!");

            opaqueInstanceData = instanceNode;
            InstanceSystemRequestBus::Broadcast(&InstanceSystemRequestBus::Events::RegisterMergedMeshInstance, opaqueInstanceData, mergedMeshNode);
        }
        else
        {
            IVegetation* instanceNode = (IVegetation*)m_engine->CreateRenderNode(eERType_Vegetation);
            AZ_Assert(instanceNode, "Could not CreateRenderNode(eERType_Vegetation)!");

            instanceNode->SetStatObjGroupIndex(m_statInstGroupId);
            instanceNode->SetUniformScale(instanceData.m_scale);
            instanceNode->SetPosition(AZVec3ToLYVec3(instanceData.m_position));
            instanceNode->SetRotation(Ang3(AZQuaternionToLYQuaternion(instanceData.m_alignment * instanceData.m_rotation)));
            instanceNode->SetDynamic(true); // this lets us track that this is a dynamic vegetation instance
            instanceNode->PrepareBBox();
            instanceNode->Physicalize();
            m_engine->RegisterEntity(instanceNode);
            opaqueInstanceData = instanceNode;
        }

        return opaqueInstanceData;
    }

    void LegacyVegetationInstanceSpawner::DestroyInstance(InstanceId id, InstancePtr instance)
    {
        if (instance)
        {
            IRenderNode* instanceNode = reinterpret_cast<IRenderNode*>(instance);
            instanceNode->ReleaseNode();
            InstanceSystemRequestBus::Broadcast(&InstanceSystemRequestBus::Events::ReleaseMergedMeshInstance, instance);
        }
    }

} // namespace Vegetation
