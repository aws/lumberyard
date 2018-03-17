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

#include "StdAfx.h"
#include "LegacyEntityConversion.h"

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>

#include <LmbrCentral/Ai/NavigationAreaBus.h>
#include <LmbrCentral/Rendering/DecalComponentBus.h>
#include <LmbrCentral/Rendering/EditorLightComponentBus.h>
#include <LmbrCentral/Rendering/LensFlareComponentBus.h>
#include <LmbrCentral/Rendering/MeshAsset.h>
#include <LmbrCentral/Rendering/MaterialOwnerBus.h>
#include <LmbrCentral/Rendering/MeshComponentBus.h>
#include <LmbrCentral/Rendering/ParticleComponentBus.h>
#include <LmbrCentral/Rendering/FogVolumeComponentBus.h>
#include <LmbrCentral/Rendering/GeomCacheComponentBus.h>
#include <LmbrCentral/Rendering/MeshAsset.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>
#include <LmbrCentral/Shape/PolygonPrismShapeComponentBus.h>

// crycommon
#include "MathConversion.h"

// CryEntities
#include "Objects/EntityObject.h"
#include "Objects/BrushObject.h"
#include "Objects/CameraObject.h"
#include "Objects/GeomEntity.h"
#include "Objects/SimpleEntity.h"
#include "Objects/PrefabObject.h"
#include "Objects/ParticleEffectObject.h"
#include "Objects/AreaBox.h"
#include "Objects/AreaSphere.h"
#include "Objects/ShapeObject.h"
#include "Objects/TagPoint.h"
#include "Objects/DecalObject.h"
#include "Objects/MiscEntities.h" //Has CGeomCacheEntity
#include "Material/Material.h"

#include <AzCore/Math/VectorConversions.h>

namespace LegacyConversionInternal
{
    // utility functions
    using namespace AZ::LegacyConversion;
    using namespace AZ::LegacyConversion::Util;

    // convert an entity without any special components.
    // you can always use the Legacy Conversion Bus to retrieve the created entity if you want to do more to it!
    LegacyConversionResult ConvertPlainEntity(CBaseObject* entityToConvert)
    {
        bool isPrefab = IsClassOf(entityToConvert, PREFAB_OBJECT_CLASS_NAME);
        bool isGroup = IsClassOf(entityToConvert, "Group");
        bool isTagPoint = IsClassOf(entityToConvert, "TagPoint");

        if ((!isPrefab) && (!isGroup) && (!isTagPoint))
        {
            return LegacyConversionResult::Ignored;
        }

        // we'll replace prefabs with slices, if possible - but we aren't going to actually make slice files here.
        // just retain the parenting.

        AZ::Outcome<AZ::EntityId, LegacyConversionResult> conversionResult = CreateEntityForConversion(entityToConvert, AZ::ComponentTypeList());
        if (!conversionResult.IsSuccess())
        {
            return conversionResult.GetError();
        }

        // finally, we can delete the old entity since we handled this completely!
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, conversionResult.GetValue());
        return LegacyConversionResult::HandledDeleteEntity;
    }

    LegacyConversionResult ConvertMeshEntity(CBaseObject* entityToConvert)
    {
        bool isBrush = IsClassOf(entityToConvert, "Brush");
        bool isGeomEntity = IsClassOf(entityToConvert, "GeomEntity");
        bool isSimpleEntity = IsClassOf(entityToConvert, "SimpleEntity");

        if ((!isBrush) && (!isGeomEntity) && (!isSimpleEntity))
        {
            return LegacyConversionResult::Ignored;
        }

        // Assign mesh asset.
        AZStd::string meshFileName;
        CMaterial* materialUsed = entityToConvert->GetMaterial();
        IStatObj* statObj = entityToConvert->GetIStatObj();
        IPhysicalEntity* physics = entityToConvert->GetCollisionEntity();
        if (statObj)
        {
            meshFileName = statObj->GetFilePath();
        }
        else
        {
            if (isBrush)
            {
                CBrushObject* castedObject = static_cast<CBrushObject*>(entityToConvert);
                meshFileName = castedObject->GetGeometryFile().toUtf8().data();
            }
            else if (isGeomEntity)
            {
                CGeomEntity* castedObject = static_cast<CGeomEntity*>(entityToConvert);
                meshFileName = castedObject->GetGeometryFile().toUtf8().data();
            }
            else if (isSimpleEntity)
            {
                CSimpleEntity* castedObject = static_cast<CSimpleEntity*>(entityToConvert);
                meshFileName = castedObject->GetGeometryFile().toUtf8().data();
            }
        }

        AZ::Data::AssetId assetId;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, meshFileName.c_str(), AZ::Data::s_invalidAssetType, false);

        AZ::Uuid meshComponentId = "{FC315B86-3280-4D03-B4F0-5553D7D08432}";
        bool isCDF = false;
        // its okay for us to not have a mesh, but if we're using a CDF file as our mesh, we need to use a skinned mesh component instead
        if (assetId.IsValid())
        {
            // is it a CDF?
            AZ::Data::AssetInfo info;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(info, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, assetId);
            if (info.m_assetType == AZ::AzTypeInfo<LmbrCentral::CharacterDefinitionAsset>::Uuid())
            {
                // switch to a skinned mesh
                meshComponentId = "{D3E1A9FC-56C9-4997-B56B-DA186EE2D62A}";
                isCDF = true;
            }
        }

        AZ::ComponentTypeList componentsToAdd {
            meshComponentId
        };

        AZ::Outcome<AZ::EntityId, LegacyConversionResult> conversionResult = CreateEntityForConversion(entityToConvert, componentsToAdd);
        if (!conversionResult.IsSuccess())
        {
            return conversionResult.GetError();
        }

        AZ::EntityId newEntityId = conversionResult.GetValue();

        if (!meshFileName.empty())
        {
            EBUS_EVENT_ID(newEntityId, LmbrCentral::MeshComponentRequestBus, SetMeshAsset, assetId);
        }
        if (materialUsed)
        {
            if ((!statObj) || (materialUsed->GetMatInfo() != statObj->GetMaterial()))
            {
                // overridden materials.
                EBUS_EVENT_ID(newEntityId, LmbrCentral::MaterialOwnerRequestBus, SetMaterial, materialUsed->GetMatInfo());
            }
        }

        // TODO: Change any other properties you'd like to change on the target (render flags, etc)
        // by calling the appropriate bus interface to do so.

        // finally, we can delete the old entity since we handled this completely!
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, newEntityId);
        return LegacyConversionResult::HandledDeleteEntity;
    }

    LegacyConversionResult ConvertBasicEntity(CBaseObject* entityToConvert)
    {
        if (!IsClassOf(entityToConvert, "Entity"))
        {
            return LegacyConversionResult::Ignored;
        }

        CEntityObject* entityObject = static_cast<CEntityObject*>(entityToConvert);

        if (entityObject->GetEntityClass() != "BasicEntity")
        {
            return LegacyConversionResult::Ignored;
        }

        AZ::Uuid meshComponentId = "{FC315B86-3280-4D03-B4F0-5553D7D08432}";
        AZ::Uuid editorRigidPhysicsComponentId = "{BD17E257-BADB-45D7-A8BA-16D6B0BE0881}";
        AZ::Uuid editorStaticPhysicsComponentId = "{C8D8C366-F7B7-42F6-8B86-E58FFF4AF984}";
        AZ::ComponentTypeList componentsToAdd;

        IPhysicalEntity* physics = entityToConvert->GetCollisionEntity();
        bool bRigidBody = false;
        if (physics)
        {
            componentsToAdd.push_back("{2D559EB0-F6FE-46E0-9FCE-E8F375177724}"); // rigid body collider (mesh) shape

            bRigidBody = entityObject->GetEntityPropertyBool("bRigidBody");
            if (bRigidBody)
            {
                componentsToAdd.push_back(editorRigidPhysicsComponentId);
            }
            else
            {
                componentsToAdd.push_back(editorStaticPhysicsComponentId);
            }
        }

        AZStd::string meshFileName = QStringAdapter(entityObject->GetEntityPropertyString("object_Model"));
        bool bHasMesh = false;
        AZ::Data::AssetId meshAssetId;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(meshAssetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, meshFileName.c_str(), AZ::Data::s_invalidAssetType, false);
        if (meshAssetId.IsValid())
        {
            componentsToAdd.push_back(meshComponentId);
            bHasMesh = true;
        }

        IStatObj* statObj = entityToConvert->GetIStatObj();
        CMaterial* materialUsed = entityToConvert->GetMaterial();

        AZ::Outcome<AZ::EntityId, LegacyConversionResult> conversionResult = CreateEntityForConversion(entityToConvert, componentsToAdd);
        if (!conversionResult.IsSuccess())
        {
            return conversionResult.GetError();
        }

        AZ::EntityId newEntityId = conversionResult.GetValue();
        AZ::Entity* newEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(newEntity, &AZ::ComponentApplicationBus::Events::FindEntity, newEntityId);

        bool conversionSuccess = true;
        if (const CVarBlock* varBlock = entityObject->GetProperties())
        {
            if (bRigidBody)
            {
                conversionSuccess &= ConvertVarHierarchy<float>(newEntity, editorRigidPhysicsComponentId, { AZ_CRC("Density", 0xc0a86afa) }, "Density", varBlock);
                conversionSuccess &= ConvertVarHierarchy<float>(newEntity, editorRigidPhysicsComponentId, { AZ_CRC("Mass", 0x6c035b66) }, "Mass", varBlock);
            }
        }

        if (bHasMesh)
        {
            EBUS_EVENT_ID(newEntityId, LmbrCentral::MeshComponentRequestBus, SetMeshAsset, meshAssetId);
        }

        if (materialUsed)
        {
            if ((!statObj) || (materialUsed->GetMatInfo() != statObj->GetMaterial()))
            {
                // overridden materials.
                EBUS_EVENT_ID(newEntityId, LmbrCentral::MaterialOwnerRequestBus, SetMaterial, materialUsed->GetMatInfo());
            }
        }

        if (!conversionSuccess)
        {
            return LegacyConversionResult::Failed;
        }

        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, newEntityId);
        return LegacyConversionResult::HandledDeleteEntity;
    }

    /*
        This is used to invert the legacy IgnoreVisAreas param
        to the new UseVisAreas param. 
     */
    bool UseVisAreaAdapter(const bool& ignoreVisAreas)
    {
        return !ignoreVisAreas;
    }

    // note: CEnvironementProbeObject -> EditorEnvProbeComponent
    LegacyConversionResult ConvertEnvironmentLightEntity(CBaseObject* entityToConvert)
    {
        bool isEnvironmentProbe = IsClassOf(entityToConvert, "EnvironmentProbe");

        if (!isEnvironmentProbe)
        {
            return LegacyConversionResult::Ignored;
        }

        AZ::Uuid editorEnvProbeId = "{8DBD6035-583E-409F-AFD9-F36829A0655D}";
        AZ::ComponentTypeList componentsToAdd {
            editorEnvProbeId
        };

        AZ::Outcome<AZ::EntityId, LegacyConversionResult> conversionResult = CreateEntityForConversion(entityToConvert, componentsToAdd);
        if (!conversionResult.IsSuccess())
        {
            return conversionResult.GetError();
        }

        AZ::EntityId newEntityId = conversionResult.GetValue();

        bool conversionSuccess = true;
        // EnvironmentProbeParams
        if (CVarBlock* varBlock = entityToConvert->GetVarBlock())
        {
            // change any other properties you'd like to change on the target by calling the appropriate bus interface to do so
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, int>("cubemap_resolution", varBlock, &LmbrCentral::EditorLightComponentRequests::SetCubemapResolution, newEntityId);
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, bool>("OutdoorOnly", varBlock, &LmbrCentral::EditorLightComponentRequests::SetIndoorOnly, newEntityId, [](const bool& var)
                    {
                        // mapping from OutdoorOnly to IndoorOnly - hence the inversion (not quite correct, but guess at intent)
                        return !var;
                    });
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, int, AZ::u32>("CastShadowMinspec", varBlock, &LmbrCentral::EditorLightComponentRequests::SetCastShadowSpec, newEntityId, [](const int&)
                    {
                        // here we want to disable shadows
                        return static_cast<AZ::u32>(EngineSpec::Never);
                    });
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, float>("ViewDistanceMultiplier", varBlock, &LmbrCentral::EditorLightComponentRequests::SetViewDistanceMultiplier, newEntityId);
        }

        // Environment Probe Properties
        CEntityObject* entityObject = static_cast<CEntityObject*>(entityToConvert);
        if (const CVarBlock* varBlock = entityObject->GetProperties())
        {
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, bool>("bActive", varBlock, &LmbrCentral::EditorLightComponentRequests::SetOnInitially, newEntityId);

            AZ::Vector3 boxDimensions;
            conversionSuccess &= FindVector3(varBlock, "BoxSizeX", "BoxSizeY", "BoxSizeZ", true, true, boxDimensions);

            if (conversionSuccess)
            {
                LmbrCentral::EditorLightComponentRequestBus::Event(newEntityId, &LmbrCentral::EditorLightComponentRequests::SetProbeArea, boxDimensions);
            }

            // convert all relevant env probe properties to new component
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, int, AZ::u32>("SortPriority", varBlock, &LmbrCentral::EditorLightComponentRequests::SetProbeSortPriority, newEntityId);
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, Vec3, AZ::Color>("clrDiffuse", varBlock, &LmbrCentral::EditorLightComponentRequests::SetColor, newEntityId, LegacyColorConverter);
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, float>("fDiffuseMultiplier", varBlock, &LmbrCentral::EditorLightComponentRequests::SetDiffuseMultiplier, newEntityId);
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, float>("fSpecularMultiplier", varBlock, &LmbrCentral::EditorLightComponentRequests::SetSpecularMultiplier, newEntityId);
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, bool>("bAffectsVolumetricFogOnly", varBlock, &LmbrCentral::EditorLightComponentRequests::SetVolumetricFogOnly, newEntityId);
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, float>("fAttenuationFalloffMax", varBlock, &LmbrCentral::EditorLightComponentRequests::SetAttenuationFalloffMax, newEntityId);
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, bool, bool>("bIgnoresVisAreas", varBlock, &LmbrCentral::EditorLightComponentRequests::SetUseVisAreas, newEntityId, UseVisAreaAdapter);
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, bool>("bVolumetricFog", varBlock, &LmbrCentral::EditorLightComponentRequests::SetVolumetricFog, newEntityId);
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, QString, AZStd::string>("texture_deferred_cubemap", varBlock, &LmbrCentral::EditorLightComponentRequests::SetCubemap, newEntityId, QStringAdapter);
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, float>("fBoxHeight", varBlock, &LmbrCentral::EditorLightComponentRequests::SetBoxHeight, newEntityId);
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, float>("fBoxWidth", varBlock, &LmbrCentral::EditorLightComponentRequests::SetBoxWidth, newEntityId);
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, float>("fBoxLength", varBlock, &LmbrCentral::EditorLightComponentRequests::SetBoxLength, newEntityId);
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, bool>("bBoxProject", varBlock, &LmbrCentral::EditorLightComponentRequests::SetBoxProjected, newEntityId);
        }

        if (!conversionSuccess)
        {
            return LegacyConversionResult::Failed;
        }

        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, newEntityId);
        return LegacyConversionResult::HandledDeleteEntity;
    }

    // note: Light -> EditorProjector/Point/AreaLightComponent
    LegacyConversionResult ConvertLightEntity(CBaseObject* entityToConvert)
    {
        if (!IsClassOf(entityToConvert, "Entity"))
        {
            return LegacyConversionResult::Ignored;
        }

        CEntityObject* entityObject = static_cast<CEntityObject*>(entityToConvert);

        if (entityObject->GetEntityClass() != "Light")
        {
            return LegacyConversionResult::Ignored;
        }

        bool objectIsProjectorLight = false;
        bool objectIsPointLight = false;
        bool objectIsAreaLight = false;
        bool objectHasLensFlare = false;

        // Find out if the object is a ProjectorLight, PointLight or AreaLight
        if (const CVarBlock* varBlock = entityObject->GetProperties())
        {
            const IVariable* textureVar = varBlock->FindVariable("texture_Texture", true, false);
            const IVariable* areaVar = varBlock->FindVariable("bAreaLight", true, false);

            if (textureVar && areaVar)
            {
                QString textureName;
                textureVar->Get(textureName);

                bool isAreaLight = false;
                areaVar->Get(isAreaLight);

                // The only difference between Projector and Point lights in CryEngine is having or not having the projector texture set.
                if (textureName.size() > 0)
                {
                    objectIsProjectorLight = true;
                }
                else if (isAreaLight)
                {
                    objectIsAreaLight = true;
                }
                else
                {
                    objectIsPointLight = true;
                }
            }
        }

        if (!objectIsProjectorLight && !objectIsPointLight && !objectIsAreaLight)
        {
            AZ_Warning("Legacy Entity Converter", "Conversion failed for %s. Unknown type of Light", entityObject->GetName().toUtf8().data());
            return LegacyConversionResult::Ignored;
        }

        AZ::ComponentTypeList componentsToAdd;

        if (objectIsProjectorLight)
        {
            componentsToAdd.push_back("{41928E34-B558-4559-82CF-8B5795A38CB4}"); // ProjectorLight component UID
        }
        else if (objectIsAreaLight)
        {
            componentsToAdd.push_back("{1DE624B1-876F-4E0A-96A6-7B248FA2076F}"); // EditorAreaLightComponent component UID
        }
        else
        {
            componentsToAdd.push_back("{00818135-138D-42AD-8657-FF3FD38D9E7A}"); // PointLight component UID
        }

        AZ::Uuid editorLensFlareComponentId = "{4B85E77D-91F9-40C5-8FCB-B494000A9E69}";
        IOpticsElementBasePtr optics = entityObject->GetOpticsElement();
        if (optics != nullptr)
        {
            // add a LensFlare component to the converted AZ::Entity
            objectHasLensFlare = true;
            componentsToAdd.push_back(editorLensFlareComponentId);
        }

        AZ::Outcome<AZ::EntityId, LegacyConversionResult> conversionResult = CreateEntityForConversion(entityToConvert, componentsToAdd);
        if (!conversionResult.IsSuccess())
        {
            return conversionResult.GetError();
        }

        AZ::EntityId newEntityId = conversionResult.GetValue();

        bool conversionSuccess = true;
        if (CVarBlock* varBlock = entityToConvert->GetVarBlock())
        {
            // change any other properties you'd like to change on the target by calling the appropriate bus interface to do so
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, bool>("OutdoorOnly", varBlock, &LmbrCentral::EditorLightComponentRequests::SetIndoorOnly, newEntityId, [](const bool& var) { return !var; });
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, float>("ViewDistanceMultiplier", varBlock, &LmbrCentral::EditorLightComponentRequests::SetViewDistanceMultiplier, newEntityId);
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, bool>("HiddenInGame", varBlock, &LmbrCentral::EditorLightComponentRequests::SetVisible, newEntityId, [](const bool& var) { return !var; });
        }

        if (const CVarBlock* varBlock = entityObject->GetProperties())
        {
            if (objectIsProjectorLight)
            {
                conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, float>("Radius", varBlock, &LmbrCentral::EditorLightComponentRequests::SetProjectorMaxDistance, newEntityId);
                conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, float>("fAttenuationBulbSize", varBlock, &LmbrCentral::EditorLightComponentRequests::SetProjectorAttenuationBulbSize, newEntityId);
                conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, float>("fProjectorFov", varBlock, &LmbrCentral::EditorLightComponentRequests::SetProjectorFOV, newEntityId);
                conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, float>("fProjectorNearPlane", varBlock, &LmbrCentral::EditorLightComponentRequests::SetProjectorNearPlane, newEntityId);
                conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, QString, AZStd::string>("texture_Texture", varBlock, &LmbrCentral::EditorLightComponentRequests::SetProjectorTexture, newEntityId, QStringAdapter);
            }

            if (objectIsPointLight)
            {
                conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, float>("Radius", varBlock, &LmbrCentral::EditorLightComponentRequests::SetPointMaxDistance, newEntityId);
                conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, float>("fAttenuationBulbSize", varBlock, &LmbrCentral::EditorLightComponentRequests::SetPointAttenuationBulbSize, newEntityId);
            }

            if (objectIsAreaLight)
            {
                conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, float>("fPlaneHeight", varBlock, &LmbrCentral::EditorLightComponentRequests::SetAreaHeight, newEntityId);
                conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, float>("fPlaneWidth", varBlock, &LmbrCentral::EditorLightComponentRequests::SetAreaWidth, newEntityId);
                conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, float>("Radius", varBlock, &LmbrCentral::EditorLightComponentRequests::SetAreaMaxDistance, newEntityId);
            }

            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, float>("fDiffuseMultiplier", varBlock, &LmbrCentral::EditorLightComponentRequests::SetDiffuseMultiplier, newEntityId);
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, float>("fSpecularMultiplier", varBlock, &LmbrCentral::EditorLightComponentRequests::SetSpecularMultiplier, newEntityId);
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, bool>("bAmbient", varBlock, &LmbrCentral::EditorLightComponentRequests::SetAmbient, newEntityId);
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, bool>("bAffectsVolumetricFogOnly", varBlock, &LmbrCentral::EditorLightComponentRequests::SetVolumetricFogOnly, newEntityId);
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, bool>("bVolumetricFog", varBlock, &LmbrCentral::EditorLightComponentRequests::SetVolumetricFog, newEntityId);
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, bool, bool>("bIgnoresVisAreas", varBlock, &LmbrCentral::EditorLightComponentRequests::SetUseVisAreas, newEntityId, UseVisAreaAdapter);
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, bool>("bAffectsThisAreaOnly", varBlock, &LmbrCentral::EditorLightComponentRequests::SetAffectsThisAreaOnly, newEntityId);

            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, bool>("bActive", varBlock, &LmbrCentral::EditorLightComponentRequests::SetOnInitially, newEntityId);

            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, Vec3, AZ::Color>("clrDiffuse", varBlock, &LmbrCentral::EditorLightComponentRequests::SetColor, newEntityId, LegacyColorConverter);

            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, float>("fShadowBias", varBlock, &LmbrCentral::EditorLightComponentRequests::SetShadowBias, newEntityId);
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, float>("fShadowSlopeBias", varBlock, &LmbrCentral::EditorLightComponentRequests::SetShadowSlopeBias, newEntityId);
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, float>("fShadowResolutionScale", varBlock, &LmbrCentral::EditorLightComponentRequests::SetShadowResScale, newEntityId);
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, float>("fShadowUpdateMinRadius", varBlock, &LmbrCentral::EditorLightComponentRequests::SetShadowUpdateMinRadius, newEntityId);
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, float>("fShadowUpdateRatio", varBlock, &LmbrCentral::EditorLightComponentRequests::SetShadowUpdateRatio, newEntityId);

            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, int, AZ::u32>("nCastShadows", varBlock, &LmbrCentral::EditorLightComponentRequests::SetCastShadowSpec, newEntityId, [](const int& var)
                    {
                        return static_cast<AZ::u32>(SpecConversion(static_cast<ESystemConfigSpec>(var)));
                    });

            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, int, AZ::u32>("nLightStyle", varBlock, &LmbrCentral::EditorLightComponentRequests::SetAnimIndex, newEntityId);

            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, float>("fAnimationSpeed", varBlock, &LmbrCentral::EditorLightComponentRequests::SetAnimSpeed, newEntityId);
            conversionSuccess &= ConvertVarBus<LmbrCentral::EditorLightComponentRequestBus, int, float>("nAnimationPhase", varBlock, &LmbrCentral::EditorLightComponentRequests::SetAnimPhase, newEntityId, [](const int& var)
                    {
                        const int max = 100;
                        int clamped = CLAMP(var, 0, max);
                        return clamped / static_cast<float>(max);
                    });
        }

        if (!conversionSuccess)
        {
            return LegacyConversionResult::Failed;
        }

        if (objectHasLensFlare)
        {
            AZ::Entity* newEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(newEntity, &AZ::ComponentApplicationBus::Events::FindEntity, newEntityId);

            // This conversion is based on the function LightInstance::LensFlareConfigToLightParams().

            AZ::u32 engineSpec = static_cast<AZ::u32>(SpecConversion(static_cast<ESystemConfigSpec>(entityToConvert->GetMinSpec())));
            conversionSuccess &= SetVarHierarchy<AZ::u32>(newEntity, editorLensFlareComponentId, { AZ_CRC("MinimumSpec", 0xb7566e46), AZ_CRC("EditorLensFlareConfiguration", 0x1293eec5) }, engineSpec);

            if (const CVarBlock* varBlock = entityObject->GetProperties())
            {
                conversionSuccess &= ConvertVarHierarchy<bool>(newEntity, editorLensFlareComponentId, { AZ_CRC("OnInitially", 0x6a5d7e6b), AZ_CRC("EditorLensFlareConfiguration", 0x1293eec5) }, "bActive", varBlock);
            }

            if (CVarBlock* varBlock = entityToConvert->GetVarBlock())
            {
                conversionSuccess &= ConvertVarHierarchy<float>(newEntity, editorLensFlareComponentId, { AZ_CRC("ViewDistanceMultiplier", 0x86a77124), AZ_CRC("EditorLensFlareConfiguration", 0x1293eec5) }, "ViewDistanceMultiplier", varBlock);
            }

            const CDLight* lightParams = entityObject->GetLightProperty();
            if (lightParams == nullptr)
            {
                return LegacyConversionResult::Failed;
            }

            const SOpticsInstanceParameters& opticsParams = lightParams->GetOpticsParams();
            AZ::Vector3 tint(opticsParams.m_color.r, opticsParams.m_color.g, opticsParams.m_color.b);
            conversionSuccess &= SetVarHierarchy<float>(newEntity, editorLensFlareComponentId, { AZ_CRC("Brightness", 0xd976d6e3), AZ_CRC("EditorLensFlareConfiguration", 0x1293eec5) }, opticsParams.m_brightness);
            conversionSuccess &= SetVarHierarchy<AZ::Vector3>(newEntity, editorLensFlareComponentId, { AZ_CRC("Tint", 0x2e09eb74), AZ_CRC("EditorLensFlareConfiguration", 0x1293eec5) }, tint);
            conversionSuccess &= SetVarHierarchy<AZ::u32>(newEntity, editorLensFlareComponentId, { AZ_CRC("TintAlpha", 0xd91a8bff), AZ_CRC("EditorLensFlareConfiguration", 0x1293eec5) }, static_cast<AZ::u32>(opticsParams.m_color.a * 255.0f));
            conversionSuccess &= SetVarHierarchy<float>(newEntity, editorLensFlareComponentId, { AZ_CRC("Size", 0xf7c0246a), AZ_CRC("EditorLensFlareConfiguration", 0x1293eec5) }, opticsParams.m_size);

            // configuration.m_syncAnimWithLight is default to false, and only when it's true we need to set LightEntity property
            conversionSuccess &= SetVarHierarchy<AZ::u32>(newEntity, editorLensFlareComponentId, { AZ_CRC("AnimIndex", 0x8465b66e), AZ_CRC("EditorLensFlareConfiguration", 0x1293eec5) }, static_cast<AZ::u32>(lightParams->m_nLightStyle));
            conversionSuccess &= SetVarHierarchy<float>(newEntity, editorLensFlareComponentId, { AZ_CRC("AnimSpeed", 0x0b302f99), AZ_CRC("EditorLensFlareConfiguration", 0x1293eec5) }, lightParams->GetAnimSpeed());
            conversionSuccess &= SetVarHierarchy<float>(newEntity, editorLensFlareComponentId, { AZ_CRC("AnimPhase", 0xb5ab07a4), AZ_CRC("EditorLensFlareConfiguration", 0x1293eec5) }, lightParams->m_nLightPhase / 255.0f);

            conversionSuccess &= SetVarHierarchy<float>(newEntity, editorLensFlareComponentId, { AZ_CRC("LensFlareFrustumAngle", 0x38f1266a), AZ_CRC("EditorLensFlareConfiguration", 0x1293eec5) }, lightParams->m_LensOpticsFrustumAngle * 360.0f / 255.0f);

            conversionSuccess &= SetVarHierarchy<bool>(newEntity, editorLensFlareComponentId, { AZ_CRC("AffectsThisAreaOnly", 0xcc6144e4), AZ_CRC("EditorLensFlareConfiguration", 0x1293eec5) }, DLF_THIS_AREA_ONLY & lightParams->m_Flags);
            conversionSuccess &= SetVarHierarchy<bool>(newEntity, editorLensFlareComponentId, { AZ_CRC("IgnoreVisAreas", 0x01823201), AZ_CRC("EditorLensFlareConfiguration", 0x1293eec5) }, DLF_IGNORES_VISAREAS & lightParams->m_Flags);
            conversionSuccess &= SetVarHierarchy<bool>(newEntity, editorLensFlareComponentId, { AZ_CRC("IndoorOnly", 0xc8ab6ddb), AZ_CRC("EditorLensFlareConfiguration", 0x1293eec5) }, DLF_INDOOR_ONLY & lightParams->m_Flags);
            conversionSuccess &= SetVarHierarchy<bool>(newEntity, editorLensFlareComponentId, { AZ_CRC("AttachToSun", 0x2b6a82fa), AZ_CRC("EditorLensFlareConfiguration", 0x1293eec5) }, DLF_ATTACH_TO_SUN & lightParams->m_Flags);

            AZStd::string lensFlareName = entityObject->GetOpticsElement()->GetName().c_str();
            AZStd::string::size_type firstDotPosition = lensFlareName.find_first_of('.');
            AZStd::string lensFlareLibName = lensFlareName.substr(0, firstDotPosition) + ".xml";
            AZStd::string lensFlareEffectName = lensFlareName.substr(firstDotPosition + 1, lensFlareName.length() - firstDotPosition - 1);
            AZStd::string fullLensFlarePath = AZStd::string(FLARE_LIBS_PATH) + lensFlareLibName;

            AZ::Data::AssetId assetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, fullLensFlarePath.c_str(), AZ::Data::s_invalidAssetType, false);
            if (assetId.IsValid())
            {
                AzToolsFramework::Components::EditorComponentBase* editorLensFlareComponent = azrtti_cast<AzToolsFramework::Components::EditorComponentBase*>(newEntity->FindComponent(editorLensFlareComponentId));
                editorLensFlareComponent->SetPrimaryAsset(assetId);
                conversionSuccess &= SetVarHierarchy<AZStd::string>(newEntity, editorLensFlareComponentId, { AZ_CRC("SelectedLensFlare", 0xa5b3daa9) }, lensFlareEffectName);
                conversionSuccess &= SetVarHierarchy<AZStd::string>(newEntity, editorLensFlareComponentId, { AZ_CRC("LensFlare", 0x9c2c65f4), AZ_CRC("EditorLensFlareConfiguration", 0x1293eec5) }, lensFlareName);

                if (!conversionSuccess)
                {
                    return LegacyConversionResult::Failed;
                }

                LmbrCentral::LensFlareComponentEditorRequestBus::Event(newEntityId, &LmbrCentral::LensFlareComponentEditorRequestBus::Events::RefreshLensFlare);
            }
        }

        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, newEntityId);
        return LegacyConversionResult::HandledDeleteEntity;
    }

    LegacyConversionResult ConvertCameraObjectEntity(CBaseObject* entityToConvert)
    {
        if (!IsClassOf(entityToConvert, "Camera"))
        {
            return LegacyConversionResult::Ignored;
        }

        CEntityObject* entityObject = static_cast<CEntityObject*>(entityToConvert);
        if (entityObject->GetEntityClass() != "CameraSource")
        {
            return LegacyConversionResult::Failed;
        }

        CCameraObject* cameraObject = static_cast<CCameraObject*>(entityToConvert);
        if (!cameraObject)
        {
            return LegacyConversionResult::Failed;
        }

        AZ::ComponentTypeList componentsToAdd {
            EditorCameraComponentTypeId
        };
        AZ::Outcome<AZ::EntityId, LegacyConversionResult> conversionResult = CreateEntityForConversion(entityToConvert, componentsToAdd);
        if (!conversionResult.IsSuccess())
        {
            return conversionResult.GetError();
        }

        AZ::EntityId newEntityId = conversionResult.GetValue();

        if (CVarBlock* varBlock = entityToConvert->GetVarBlock())
        {
            bool conversionSuccess = true;

            // change any other properties you'd like to change on the target by calling the appropriate bus interface to do so
            conversionSuccess &= ConvertVarBus<Camera::CameraRequestBus, float>("FOV", varBlock, &Camera::CameraComponentRequests::SetFov, newEntityId, [](const float& var) { return RAD2DEG(var); });
            conversionSuccess &= ConvertVarBus<Camera::CameraRequestBus, float>("NearZ", varBlock, &Camera::CameraComponentRequests::SetNearClipDistance, newEntityId);
            conversionSuccess &= ConvertVarBus<Camera::CameraRequestBus, float>("FarZ", varBlock, &Camera::CameraComponentRequests::SetFarClipDistance, newEntityId);

            if (!conversionSuccess)
            {
                return LegacyConversionResult::Failed;
            }
        }

        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, newEntityId);
        return LegacyConversionResult::HandledDeleteEntity;
    }

    LegacyConversionResult ConvertCameraObjectTargetEntity(CBaseObject* entityToConvert)
    {
        if (!IsClassOf(entityToConvert, "CameraTarget"))
        {
            return LegacyConversionResult::Ignored;
        }

        CEntityObject* entityObject = static_cast<CEntityObject*>(entityToConvert);
        if (entityObject->GetEntityClass() != "CameraTarget")
        {
            return LegacyConversionResult::Failed;
        }

        AZ::ComponentTypeList componentsToAdd;
        AZ::Outcome<AZ::EntityId, LegacyConversionResult> conversionResult = CreateEntityForConversion(entityToConvert, componentsToAdd);
        if (!conversionResult.IsSuccess())
        {
            return conversionResult.GetError();
        }

        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, conversionResult.GetValue());
        return LegacyConversionResult::HandledDeleteEntity;
    }

    LegacyConversionResult ConvertTagCommentEntity(CBaseObject* entityToConvert)
    {
        if (!IsClassOf(entityToConvert, "Comment"))
        {
            return LegacyConversionResult::Ignored;
        }

        CEntityObject* entityObject = static_cast<CEntityObject*>(entityToConvert);
        if (entityObject->GetEntityClass() != "Comment")
        {
            return LegacyConversionResult::Failed;
        }

        AZ::Uuid editorCommentComponentId = "{5181117D-CD69-4C05-8804-C1FBE5F0C00F}";
        AZ::ComponentTypeList componentsToAdd {
            editorCommentComponentId
        };
        AZ::Outcome<AZ::EntityId, LegacyConversionResult> conversionResult = CreateEntityForConversion(entityToConvert, componentsToAdd);
        if (!conversionResult.IsSuccess())
        {
            return conversionResult.GetError();
        }
        AZ::EntityId newEntityId = conversionResult.GetValue();

        AZ::Entity* newEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(newEntity, &AZ::ComponentApplicationBus::Events::FindEntity, newEntityId);

        if (CVarBlock* varBlock = entityObject->GetProperties())
        {
            bool conversionSuccess = ConvertVarHierarchy<QString, AZStd::string>(newEntity, editorCommentComponentId, { AZ_CRC("Configuration", 0xa5e2a5d7) }, "Comment", varBlock, QStringAdapter);

            if (!conversionSuccess)
            {
                conversionSuccess = ConvertVarHierarchy<QString, AZStd::string>(newEntity, editorCommentComponentId, { AZ_CRC("Configuration", 0xa5e2a5d7) }, "Text", varBlock, QStringAdapter);
            }

            if (!conversionSuccess)
            {
                return LegacyConversionResult::Failed;
            }
        }

        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, newEntityId);
        return LegacyConversionResult::HandledDeleteEntity;
    }

    LegacyConversionResult ConvertParticleEntity(CBaseObject* entityToConvert)
    {
        if (!IsClassOf(entityToConvert, "ParticleEntity"))
        {
            return LegacyConversionResult::Ignored;
        }

        CEntityObject* entityObject = static_cast<CEntityObject*>(entityToConvert);
        if (entityObject->GetEntityClass() != "ParticleEffect")
        {
            return LegacyConversionResult::Failed;
        }

        CParticleEffectObject* particleEffectObject = static_cast<CParticleEffectObject*>(entityToConvert);
        if (!particleEffectObject)
        {
            return LegacyConversionResult::Failed;
        }

        AZ::Uuid editorParticleComponentId = "{0F35739E-1B40-4497-860D-D6FF5D87A9D9}";
        AZ::ComponentTypeList componentsToAdd {
            editorParticleComponentId
        };

        AZ::Outcome<AZ::EntityId, LegacyConversionResult> conversionResult = CreateEntityForConversion(entityToConvert, componentsToAdd);
        if (!conversionResult.IsSuccess())
        {
            return conversionResult.GetError();
        }

        AZ::EntityId newEntityId = conversionResult.GetValue();

        if (CVarBlock* varBlock = particleEffectObject->GetProperties())
        {
            AZStd::string particleName = particleEffectObject->GetParticleName().toUtf8().data();
            AZStd::string emitterName = particleName.substr(particleName.find_first_of('.') + 1);
            AZStd::string libName = particleName.substr(0, particleName.find_first_of('.'));
            AZStd::string fullLibraryName = libName + ".xml";
            fullLibraryName = AZStd::string("Libs/Particles/") + fullLibraryName;

            EBUS_EVENT_ID(newEntityId, LmbrCentral::EditorParticleComponentRequestBus, SetEmitter, particleName, fullLibraryName);

            AZ::Entity* newEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(newEntity, &AZ::ComponentApplicationBus::Events::FindEntity, newEntityId);

            bool conversionSuccess = true;
            conversionSuccess &= ConvertVarHierarchy<bool>(newEntity, editorParticleComponentId, { AZ_CRC("Pre-roll", 0xdc1c1775), AZ_CRC("Particle", 0x56249fe1) }, "bPrime", varBlock);
            conversionSuccess &= ConvertVarHierarchy<float>(newEntity, editorParticleComponentId, { AZ_CRC("Particle Count Scale", 0x1ea57494) }, "CountScale", varBlock);
            conversionSuccess &= ConvertVarHierarchy<float>(newEntity, editorParticleComponentId, { AZ_CRC("Time Scale", 0x53407bec) }, "TimeScale", varBlock);
            conversionSuccess &= ConvertVarHierarchy<float>(newEntity, editorParticleComponentId, { AZ_CRC("Pulse Period", 0xf1d38aea) }, "PulsePeriod", varBlock);
            conversionSuccess &= ConvertVarHierarchy<float>(newEntity, editorParticleComponentId, { AZ_CRC("GlobalSizeScale", 0x23608075) }, "Scale", varBlock);
            conversionSuccess &= ConvertVarHierarchy<float>(newEntity, editorParticleComponentId, { AZ_CRC("Speed Scale", 0x26c0d5eb) }, "SpeedScale", varBlock);
            conversionSuccess &= ConvertVarHierarchy<float>(newEntity, editorParticleComponentId, { AZ_CRC("Strength", 0xd6b9e1d8) }, "Strength", varBlock);
            conversionSuccess &= ConvertVarHierarchy<bool>(newEntity, editorParticleComponentId, { AZ_CRC("Register by Bounding Box", 0x701b53f8) }, "bRegisterByBBox", varBlock);
            conversionSuccess &= ConvertVarHierarchy<bool>(newEntity, editorParticleComponentId, { AZ_CRC("Enable Audio", 0x020fb42d) }, "bEnableAudio", varBlock);
            conversionSuccess &= ConvertVarHierarchy<QString, AZStd::string>(newEntity, editorParticleComponentId, { AZ_CRC("Audio RTPC", 0x7c3d55d1) }, "audioRTPCRtpc", varBlock, QStringAdapter);

            if (!conversionSuccess)
            {
                return LegacyConversionResult::Failed;
            }
        }

        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, newEntityId);
        return LegacyConversionResult::HandledDeleteEntity;
    }


    LegacyConversionResult ConvertProximityTriggerEntity(CBaseObject* entityToConvert)
    {
        if (!IsClassOf(entityToConvert, "Entity"))
        {
            return LegacyConversionResult::Ignored;
        }

        CEntityObject* entityObject = static_cast<CEntityObject*>(entityToConvert);
        if (entityObject->GetEntityClass() != "ProximityTrigger")
        {
            return LegacyConversionResult::Ignored;
        }

        AZ::Uuid editorBoxShapeComponentId = "{2ADD9043-48E8-4263-859A-72E0024372BF}";
        AZ::Uuid editorTriggerAreaComponentId = "{7F983509-09FF-4990-93D0-9D52BA1C60B2}";
        AZ::ComponentTypeList componentsToAdd {
            editorBoxShapeComponentId, editorTriggerAreaComponentId
        };

        AZ::Outcome<AZ::EntityId, LegacyConversionResult> conversionResult = CreateEntityForConversion(entityToConvert, componentsToAdd);
        if (!conversionResult.IsSuccess())
        {
            return conversionResult.GetError();
        }
        AZ::EntityId newEntityId = conversionResult.GetValue();

        bool conversionSuccess = true;
        if (const CVarBlock* varBlock = entityObject->GetProperties())
        {
            AZ::Vector3 dimensions;
            conversionSuccess &= FindVector3(varBlock, "DimX", "DimY", "DimZ", true, false, dimensions);

            if (conversionSuccess)
            {
                LmbrCentral::BoxShapeComponentRequestsBus::Event(newEntityId, &LmbrCentral::BoxShapeComponentRequests::SetBoxDimensions, dimensions);

                AZ::Entity* newEntity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(newEntity, &AZ::ComponentApplicationBus::Events::FindEntity, newEntityId);

                conversionSuccess &= ConvertVarHierarchy<bool>(newEntity, editorTriggerAreaComponentId, { AZ_CRC("TriggerOnce", 0x3a10a884) }, "bTriggerOnce", varBlock);
            }
        }

        if (!conversionSuccess)
        {
            return LegacyConversionResult::Failed;
        }

        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, newEntityId);
        return LegacyConversionResult::HandledDeleteEntity;
    }

    LegacyConversionResult ConvertAreaBoxEntity(CBaseObject* entityToConvert)
    {
        if (!IsClassOf(entityToConvert, "AreaBox"))
        {
            return LegacyConversionResult::Ignored;
        }

        CEntityObject* entityObject = static_cast<CEntityObject*>(entityToConvert);
        if (entityObject->GetEntityClass() != "AreaBox")
        {
            return LegacyConversionResult::Ignored;
        }

        CAreaBox* areaBox = static_cast<CAreaBox*>(entityToConvert);
        if (!areaBox)
        {
            return LegacyConversionResult::Failed;
        }

        AZ::Uuid editorBoxShapeComponentId = "{2ADD9043-48E8-4263-859A-72E0024372BF}";
        AZ::ComponentTypeList componentsToAdd {
            editorBoxShapeComponentId
        };

        AZ::Outcome<AZ::EntityId, LegacyConversionResult> conversionResult = CreateEntityForConversion(entityToConvert, componentsToAdd);
        if (!conversionResult.IsSuccess())
        {
            return conversionResult.GetError();
        }
        AZ::EntityId newEntityId = conversionResult.GetValue();

        AZ::Vector3 dimensions(areaBox->GetWidth(), areaBox->GetLength(), areaBox->GetHeight());
        LmbrCentral::BoxShapeComponentRequestsBus::Event(newEntityId, &LmbrCentral::BoxShapeComponentRequests::SetBoxDimensions, dimensions);

        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, newEntityId);
        return LegacyConversionResult::HandledDeleteEntity;
    }

    LegacyConversionResult ConvertAreaSphereEntity(CBaseObject* entityToConvert)
    {
        if (!IsClassOf(entityToConvert, "AreaSphere"))
        {
            return LegacyConversionResult::Ignored;
        }

        CEntityObject* entityObject = static_cast<CEntityObject*>(entityToConvert);
        if (entityObject->GetEntityClass() != "AreaSphere")
        {
            return LegacyConversionResult::Ignored;
        }

        CAreaSphere* areaSphere = static_cast<CAreaSphere*>(entityToConvert);
        if (!areaSphere)
        {
            return LegacyConversionResult::Failed;
        }

        AZ::Uuid editorSphereShapeComponentId = "{2EA56CBF-63C8-41D9-84D5-0EC2BECE748E}";
        AZ::ComponentTypeList componentsToAdd { editorSphereShapeComponentId };

        AZ::Outcome<AZ::EntityId, LegacyConversionResult> conversionResult = CreateEntityForConversion(entityToConvert, componentsToAdd);
        if (!conversionResult.IsSuccess())
        {
            return conversionResult.GetError();
        }
        AZ::EntityId newEntityId = conversionResult.GetValue();

        LmbrCentral::SphereShapeComponentRequestsBus::Event(newEntityId, &LmbrCentral::SphereShapeComponentRequests::SetRadius, areaSphere->GetRadius());

        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, newEntityId);
        return LegacyConversionResult::HandledDeleteEntity;
    }

    LegacyConversionResult ConvertAreaShapeEntity(CBaseObject* entityToConvert)
    {
        if (!IsClassOf(entityToConvert, "Shape"))
        {
            return LegacyConversionResult::Ignored;
        }

        CShapeObject* areaShape = static_cast<CShapeObject*>(entityToConvert);
        if (!areaShape)
        {
            return LegacyConversionResult::Failed;
        }

        AZ::Uuid editorPolygonPrismShapeComponentId = "{5368F204-FE6D-45C0-9A4F-0F933D90A785}"; // EditorPolygonPrismShapeComponent
        AZ::ComponentTypeList componentsToAdd {
            editorPolygonPrismShapeComponentId
        };

        AZ::Outcome<AZ::EntityId, LegacyConversionResult> conversionResult = CreateEntityForConversion(entityToConvert, componentsToAdd);
        if (!conversionResult.IsSuccess())
        {
            return conversionResult.GetError();
        }

        AZ::EntityId newEntityId = conversionResult.GetValue();

        AZ::Entity* newEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(newEntity, &AZ::ComponentApplicationBus::Events::FindEntity, newEntityId);

        float height = areaShape->GetHeight();
        AZStd::vector<AZ::Vector2> vertices;
        vertices.resize(areaShape->GetPointCount());

        for (size_t i = 0; i < areaShape->GetPointCount(); ++i)
        {
            const Vec3& point = areaShape->GetPoint(i);
            vertices[i] = AZ::Vector2(point.x, point.y);
        }

        bool conversionSuccess = true;
        conversionSuccess &= SetVarHierarchy<AZStd::vector<AZ::Vector2> >(newEntity, editorPolygonPrismShapeComponentId, { AZ_CRC("Vertices", 0x4112bc6d), AZ_CRC("VertexContainer", 0xe359981a), AZ_CRC("Configuration", 0xa5e2a5d7) }, vertices);
        conversionSuccess &= SetVarHierarchy<float>(newEntity, editorPolygonPrismShapeComponentId, { AZ_CRC("Height", 0xf54de50f), AZ_CRC("Configuration", 0xa5e2a5d7) }, height);

        if (!conversionSuccess)
        {
            return LegacyConversionResult::Failed;
        }

        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, newEntityId);
        return LegacyConversionResult::HandledDeleteEntity;
    }

    LegacyConversionResult ConvertNavigationSeed(CBaseObject* entityToConvert)
    {
        if (!IsClassOf(entityToConvert, "NavigationSeedPoint"))
        {
            return LegacyConversionResult::Ignored;
        }

        const AZ::Uuid navigationSeedId = "{A836E9F7-0C5A-4397-AD01-523EBC1E41A5}";
        AZ::Outcome<AZ::EntityId, LegacyConversionResult> conversionResult = CreateEntityForConversion(entityToConvert, { navigationSeedId });

        if (!conversionResult.IsSuccess())
        {
            return conversionResult.GetError();
        }

        const AZ::EntityId newEntityId = conversionResult.GetValue();
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, newEntityId);

        return LegacyConversionResult::HandledDeleteEntity;
    }

    LegacyConversionResult ConvertWindVolumeEntity(CBaseObject* entityToConvert)
    {
        if (!IsClassOf(entityToConvert, "Entity::WindArea"))
        {
            return LegacyConversionResult::Ignored;
        }
        bool isSphereShape = static_cast<CEntityObject*>(entityToConvert)->GetEntityPropertyBool("bEllipsoidal");

        auto shapeComponentId = isSphereShape
            ? "{2EA56CBF-63C8-41D9-84D5-0EC2BECE748E}" //EditorSphereShapeComponent
            : "{2ADD9043-48E8-4263-859A-72E0024372BF}"; //EditorBoxShapeComponent
        auto windComponentId = "{61E5864D-F553-4A37-9A03-B9F836F1D3DC}"; // WindVolumeComponent
        auto conversionResult = CreateEntityForConversion(entityToConvert, {
            shapeComponentId,
            windComponentId
        });

        if (!conversionResult.IsSuccess())
        {
            return conversionResult.GetError();
        }

        auto newEntityId = conversionResult.GetValue();
        AZ::Entity* newEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(newEntity, &AZ::ComponentApplicationBus::Events::FindEntity, newEntityId);

        auto varBlock = static_cast<CEntityObject*>(entityToConvert)->GetProperties();
        auto success = true;
        Vec3 size;
        success &= ConvertVarHierarchy<float>(newEntity, windComponentId, { AZ_CRC("Air Density", 0xadc8eb2f) }, "AirDensity", varBlock);
        success &= ConvertVarHierarchy<float>(newEntity, windComponentId, { AZ_CRC("Air Resistance", 0xec64bb06) }, "AirResistance", varBlock);
        success &= ConvertVarHierarchy<float>(newEntity, windComponentId, { AZ_CRC("Speed", 0x0f26fef6) }, "Speed", varBlock);
        success &= ConvertVarHierarchy<float>(newEntity, windComponentId, { AZ_CRC("FalloffInner", 0xc87f0b6a) }, "FalloffInner", varBlock);
        success &= ConvertVarHierarchy<Vec3, AZ::Vector3>(newEntity, windComponentId, { AZ_CRC("Direction", 0x3e4ad1b3) }, "Dir", varBlock, LYVec3ToAZVec3);
        success &= GetVec3("Size", varBlock, size);
        if (isSphereShape)
        {
            // NOTE: 0.577 is a magic number from windarea.lua for converting box size to radius.
            LmbrCentral::SphereShapeComponentRequestsBus::Event(newEntity->GetId(), &LmbrCentral::SphereShapeComponentRequestsBus::Events::SetRadius, size.len() * 0.577f);
        }
        else
        {
            LmbrCentral::BoxShapeComponentRequestsBus::Event(newEntity->GetId(), &LmbrCentral::BoxShapeComponentRequestsBus::Events::SetBoxDimensions, LYVec3ToAZVec3(size * 2.f));
        }

        if (!success)
        {
            return LegacyConversionResult::Failed;
        }

        return LegacyConversionResult::HandledDeleteEntity;
    }

    LegacyConversionResult ConvertNavigationAreaEntity(CBaseObject* entityToConvert)
    {
        if (!IsClassOf(entityToConvert, "NavigationArea"))
        {
            return LegacyConversionResult::Ignored;
        }

        CNavigationAreaObject* navigationAreaObject = static_cast<CNavigationAreaObject*>(entityToConvert);
        if (!navigationAreaObject)
        {
            return LegacyConversionResult::Failed;
        }

        const auto navigationAreaComponentId = "{8391FF77-7F4E-4576-9617-37793F88C5DA}"; // EditorNavigationAreaComponent
        const auto polygonPrismComponentId = "{5368F204-FE6D-45C0-9A4F-0F933D90A785}"; // EditorPolygonPrismComponent
        const auto conversionResult = CreateEntityForConversion(entityToConvert, { navigationAreaComponentId, polygonPrismComponentId });
        if (!conversionResult.IsSuccess())
        {
            return conversionResult.GetError();
        }

        auto newEntityId = conversionResult.GetValue();
        AZ::Entity* newEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(newEntity, &AZ::ComponentApplicationBus::Events::FindEntity, newEntityId);

        const float height = navigationAreaObject->GetHeight();
        AZStd::vector<AZ::Vector2> vertices;
        vertices.resize(navigationAreaObject->GetPointCount());

        for (size_t i = 0; i < navigationAreaObject->GetPointCount(); ++i)
        {
            const Vec3& point = navigationAreaObject->GetPoint(i);
            vertices[i] = AZ::Vector2(point.x, point.y);
        }

        bool conversionSuccess = true;
        conversionSuccess &= SetVarHierarchy<AZStd::vector<AZ::Vector2> >(newEntity, polygonPrismComponentId, { AZ_CRC("Vertices", 0x4112bc6d), AZ_CRC("VertexContainer", 0xe359981a), AZ_CRC("Configuration", 0xa5e2a5d7) }, vertices);
        conversionSuccess &= SetVarHierarchy<float>(newEntity, polygonPrismComponentId, { AZ_CRC("Height", 0xf54de50f), AZ_CRC("Configuration", 0xa5e2a5d7) }, height);

        const auto varBlock = entityToConvert->GetVarBlock();
        conversionSuccess &= ConvertVarHierarchy<bool>(newEntity, navigationAreaComponentId, { AZ_CRC("Exclusion", 0x0df1686c) }, "Exclusion", varBlock);

        // find all agent types, for the ones that are enabled, add them to the agent types for this navigation area
        AZStd::vector<AZStd::string> agentTypeNames;
        if (const IVariable* agentTypes = varBlock->FindVariable("AffectedAgentTypes", true, false))
        {
            for (size_t i = 0; i < agentTypes->GetNumVariables(); ++i)
            {
                if (auto agentType = agentTypes->GetVariable(i))
                {
                    bool enabled;
                    agentType->Get(enabled);

                    if (enabled)
                    {
                        agentTypeNames.push_back(AZStd::string(agentType->GetName().toUtf8().constData()));
                    }
                }
            }
        }

        conversionSuccess &= SetVarHierarchy<AZStd::vector<AZStd::string> >(newEntity, navigationAreaComponentId, { AZ_CRC("AgentTypes", 0xa98e43c8) }, agentTypeNames);

        // refresh/update then nav mesh after the areas have been created
        LmbrCentral::NavigationAreaRequestBus::Event(newEntityId, &LmbrCentral::NavigationAreaRequests::RefreshArea);

        if (!conversionSuccess)
        {
            return LegacyConversionResult::Failed;
        }

        return LegacyConversionResult::HandledDeleteEntity;
    }

    LegacyConversionResult ConvertTagPointEntity(CBaseObject* entityToConvert)
    {
        if (!IsClassOf(entityToConvert, "TagPoint"))
        {
            return LegacyConversionResult::Ignored;
        }

        CEntityObject* entityObject = static_cast<CEntityObject*>(entityToConvert);
        if (entityObject->GetEntityClass() != "TagPoint")
        {
            return LegacyConversionResult::Ignored;
        }

        CTagPoint* tagPoint = static_cast<CTagPoint*>(entityToConvert);
        if (!tagPoint)
        {
            return LegacyConversionResult::Failed;
        }

        // Cry TagPoint entity is converted to empty AZ entity. We don't add any component.
        AZ::ComponentTypeList componentsToAdd;

        AZ::Outcome<AZ::EntityId, LegacyConversionResult> conversionResult = CreateEntityForConversion(entityToConvert, componentsToAdd);
        if (!conversionResult.IsSuccess())
        {
            return conversionResult.GetError();
        }
        AZ::EntityId newEntityId = conversionResult.GetValue();

        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, newEntityId);
        return LegacyConversionResult::HandledDeleteEntity;
    }

    LegacyConversionResult ConvertDecalEntity(CBaseObject* entityToConvert)
    {
        if (!IsClassOf(entityToConvert, "Decal"))
        {
            return LegacyConversionResult::Ignored;
        }

        CDecalObject* decalObj = static_cast<CDecalObject*>(entityToConvert);
        if (!decalObj)
        {
            return LegacyConversionResult::Failed;
        }

        AZ::Uuid editorDecalComponentId = "{BA3890BD-D2E7-4DB6-95CD-7E7D5525567A}";
        AZ::ComponentTypeList componentsToAdd { editorDecalComponentId };

        AZ::Outcome<AZ::EntityId, LegacyConversionResult> conversionResult = CreateEntityForConversion(entityToConvert, componentsToAdd);
        if (!conversionResult.IsSuccess())
        {
            return conversionResult.GetError();
        }
        AZ::EntityId newEntityId = conversionResult.GetValue();

        AZ::Entity* newEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(newEntity, &AZ::ComponentApplicationBus::Events::FindEntity, newEntityId);

        bool conversionSuccess = true;
        if (CVarBlock* varBlock = entityToConvert->GetVarBlock())
        {
            conversionSuccess &= ConvertVarHierarchy<float>(newEntity, editorDecalComponentId, { AZ_CRC("View Distance Multiplier", 0xa5643f50), AZ_CRC("EditorDecalConfiguration", 0x912ed516) }, "ViewDistanceMultiplier", varBlock);
            conversionSuccess &= ConvertVarHierarchy<int>(newEntity, editorDecalComponentId, { AZ_CRC("ProjectionType", 0x1fef2617), AZ_CRC("EditorDecalConfiguration", 0x912ed516) }, "ProjectionType", varBlock);
            conversionSuccess &= ConvertVarHierarchy<bool>(newEntity, editorDecalComponentId, { AZ_CRC("Deferred", 0xa364e89a), AZ_CRC("EditorDecalConfiguration", 0x912ed516) }, "Deferred", varBlock);
            conversionSuccess &= ConvertVarHierarchy<int, AZ::u32>(newEntity, editorDecalComponentId, { AZ_CRC("SortPriority", 0xb35a33a7), AZ_CRC("EditorDecalConfiguration", 0x912ed516) }, "SortPriority", varBlock);
            conversionSuccess &= ConvertVarHierarchy<float>(newEntity, editorDecalComponentId, { AZ_CRC("Depth", 0xfaa31c69), AZ_CRC("EditorDecalConfiguration", 0x912ed516) }, "ProjectionDepth", varBlock);
        }

        AZ::u32 engineSpec;
        if (static_cast<ESystemConfigSpec>(entityToConvert->GetMinSpec()) == CONFIG_AUTO_SPEC)
        {
            // LY Engine Spec doesn't have AUTO option, and by default, SpecConversion() converts Cry Engine Spec AUTO to LY Engine Spec NEVER.
            // We are doing a special case here to convert Cry Engine Spec AUTO to LY Engine Spec LOW for Decal Component.
            engineSpec = static_cast<AZ::u32>(EngineSpec::Low);
        }
        else
        {
            engineSpec = static_cast<AZ::u32>(SpecConversion(static_cast<ESystemConfigSpec>(entityToConvert->GetMinSpec())));
        }
        conversionSuccess &= SetVarHierarchy<AZ::u32>(newEntity, editorDecalComponentId, { AZ_CRC("Min Spec", 0x2297ea39), AZ_CRC("EditorDecalConfiguration", 0x912ed516) }, engineSpec);

        if (!conversionSuccess)
        {
            return LegacyConversionResult::Failed;
        }

        CMaterial* material = entityToConvert->GetMaterial();
        if (material != nullptr)
        {
            LmbrCentral::MaterialOwnerRequestBus::Event(newEntityId, &LmbrCentral::MaterialOwnerRequestBus::Events::SetMaterial, material->GetMatInfo());
        }
        else
        {
            LmbrCentral::DecalComponentEditorRequests::Bus::Event(newEntityId, &LmbrCentral::DecalComponentEditorRequests::RefreshDecal);
        }

        return LegacyConversionResult::HandledDeleteEntity;
    }

    LegacyConversionResult ConvertFogVolumeEntity(CBaseObject* entityToConvert)
    {
        if (!IsClassOf(entityToConvert, "Entity"))
        {
            return LegacyConversionResult::Ignored;
        }

        CEntityObject* entityObject = static_cast<CEntityObject*>(entityToConvert);
        if (entityObject->GetEntityClass() != "FogVolume")
        {
            return LegacyConversionResult::Ignored;
        }

        auto fogVolumeComponent = "{8CA5AB61-96D8-482F-B07C-DAD72ED73646}";
        auto boxShapeComponent = "{2ADD9043-48E8-4263-859A-72E0024372BF}";

        auto conversionResult = CreateEntityForConversion(entityToConvert, { fogVolumeComponent, boxShapeComponent });
        if (!conversionResult.IsSuccess())
        {
            return conversionResult.GetError();
        }

        auto newEntityId = conversionResult.GetValue();
        AZ::Entity* newEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(newEntity, &AZ::ComponentApplicationBus::Events::FindEntity, newEntityId);

        auto oldEntity = static_cast<CEntityObject*>(entityToConvert);
        auto varBlock = oldEntity->GetProperties();
        auto success = true;

        success &= ConvertVarHierarchy<Vec3, AZ::Vector3>(newEntity, boxShapeComponent, { AZ_CRC("Dimensions", 0xe27d8ba5) }, "Size", varBlock, LYVec3ToAZVec3);

        success &= ConvertVarHierarchy<int, AZ::s32>(newEntity, fogVolumeComponent, { AZ_CRC("VolumeType", 0xea31f528) }, "eiVolumeType", varBlock);
        success &= ConvertVarHierarchy<Vec3, AZ::Color>(newEntity, fogVolumeComponent, { AZ_CRC("Color", 0x665648e9) }, "color_Color", varBlock, LegacyColorConverter);
        success &= ConvertVarHierarchy<float>(newEntity, fogVolumeComponent, { AZ_CRC("HdrDynamic", 0x1887bf7d) }, "fHDRDynamic", varBlock);
        success &= ConvertVarHierarchy<bool>(newEntity, fogVolumeComponent, { AZ_CRC("UseGlobalFogColor", 0xab48bdd5) }, "bUseGlobalFogColor", varBlock);
        success &= ConvertVarHierarchy<float>(newEntity, fogVolumeComponent, { AZ_CRC("SoftEdges", 0x27b19509) }, "SoftEdges", varBlock);
        success &= ConvertVarHierarchy<float>(newEntity, fogVolumeComponent, { AZ_CRC("WindInfluence", 0x09dc636c) }, "WindInfluence", varBlock);
        success &= ConvertVarHierarchy<float>(newEntity, fogVolumeComponent, { AZ_CRC("GlobalDensity", 0xd60a9829) }, "GlobalDensity", varBlock);
        success &= ConvertVarHierarchy<float>(newEntity, fogVolumeComponent, { AZ_CRC("DensityOffset", 0x27f138e9) }, "DensityOffset", varBlock);
        success &= ConvertVarHierarchy<float>(newEntity, fogVolumeComponent, { AZ_CRC("NearCutoff", 0xd21458a8) }, "NearCutoff", varBlock);
        success &= ConvertVarHierarchy<bool>(newEntity, fogVolumeComponent, { AZ_CRC("IgnoresVisAreas", 0x9bc7210c) }, "bIgnoresVisAreas", varBlock);
        success &= ConvertVarHierarchy<bool>(newEntity, fogVolumeComponent, { AZ_CRC("AffectsThisAreaOnly", 0xcc6144e4) }, "bAffectsThisAreaOnly", varBlock);
        success &= ConvertVarHierarchy<float>(newEntity, fogVolumeComponent, { AZ_CRC("FallOffDirLong", 0xd18566a5) }, "FallOffDirLong", varBlock);
        success &= ConvertVarHierarchy<float>(newEntity, fogVolumeComponent, { AZ_CRC("FallOffDirLatitude", 0x67e74d30) }, "FallOffDirLati", varBlock);
        success &= ConvertVarHierarchy<float>(newEntity, fogVolumeComponent, { AZ_CRC("FallOffShift", 0x00453be6) }, "FallOffShift", varBlock);
        success &= ConvertVarHierarchy<float>(newEntity, fogVolumeComponent, { AZ_CRC("FallOffScale", 0x49082527) }, "FallOffScale", varBlock);
        success &= ConvertVarHierarchy<float>(newEntity, fogVolumeComponent, { AZ_CRC("RampStart", 0x9c05ddae) }, "RampStart", varBlock);
        success &= ConvertVarHierarchy<float>(newEntity, fogVolumeComponent, { AZ_CRC("RampEnd", 0xd0133b40) }, "RampEnd", varBlock);
        success &= ConvertVarHierarchy<float>(newEntity, fogVolumeComponent, { AZ_CRC("RampInfluence", 0xa558e112) }, "RampInfluence", varBlock);
        success &= ConvertVarHierarchy<float>(newEntity, fogVolumeComponent, { AZ_CRC("DensityNoiseScale", 0xb4915b4e) }, "DensityNoiseScale", varBlock);
        success &= ConvertVarHierarchy<float>(newEntity, fogVolumeComponent, { AZ_CRC("DensityNoiseOffset", 0x22e33600) }, "DensityNoiseOffset", varBlock);
        success &= ConvertVarHierarchy<float>(newEntity, fogVolumeComponent, { AZ_CRC("DensityNoiseTimeFrequency", 0xfc53e6ab) }, "DensityNoiseTimeFrequency", varBlock);
        success &= ConvertVarHierarchy<Vec3, AZ::Vector3>(newEntity, fogVolumeComponent, { AZ_CRC("DensityNoiseFrequency", 0xca297935) }, "DensityNoiseFrequency", varBlock, LYVec3ToAZVec3);

        //SpecConversion
        success &= SetVarHierarchy<AZ::u32>(newEntity, fogVolumeComponent, { AZ_CRC("EngineSpec", 0x068303d5) }, oldEntity->GetMinSpec());
        success &= SetVarHierarchy<float>(newEntity, fogVolumeComponent, { AZ_CRC("DistMult", 0xab1f5595) }, oldEntity->GetViewDistanceMultiplier());

        if (!success)
        {
            return LegacyConversionResult::Failed;
        }

        LmbrCentral::FogVolumeComponentRequestBus::Event(newEntityId, &LmbrCentral::FogVolumeComponentRequestBus::Events::RefreshFog);
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, newEntityId);
        return LegacyConversionResult::HandledDeleteEntity;
    }

    LegacyConversionResult ConvertDistanceClouds(CBaseObject* entityToConvert)
    {
        if (!((AZStd::string(entityToConvert->metaObject()->className()) == "CDistanceCloudObject" && AZStd::string(entityToConvert->metaObject()->className()) != "CEntityObject") ||
            (entityToConvert->inherits("CEntityObject") && static_cast<CEntityObject *>(entityToConvert)->GetEntityClass() == "DistanceCloud")))
        {
            // We don't know how to convert this entity, whatever it is
            return AZ::LegacyConversion::LegacyConversionResult::Ignored;
        }

        auto staticMeshComponentId = "{FC315B86-3280-4D03-B4F0-5553D7D08432}"; // Editor Static Mesh Component
        auto conversionResult = CreateEntityForConversion(entityToConvert, { staticMeshComponentId });
        if (!conversionResult.IsSuccess())
        {
            return conversionResult.GetError();
        }

        auto newEntityId = conversionResult.GetValue();
        AZ::Entity* newEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(newEntity, &AZ::ComponentApplicationBus::Events::FindEntity, newEntityId);

        //Set mesh
        AZ::Data::AssetId assetId;
        AZStd::string meshFileName = "objects/default/primitive_plane.cgf";
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, meshFileName.c_str(), AZ::Data::s_invalidAssetType, false);

        if (!assetId.IsValid())
        {
            return LegacyConversionResult::Failed;
        }

        LmbrCentral::MeshComponentRequestBus::Event(newEntityId, &LmbrCentral::MeshComponentRequestBus::Events::SetMeshAsset, assetId);

        //Set material
        CMaterial* distanceCloudMat = entityToConvert->GetMaterial();

        if (distanceCloudMat == nullptr || distanceCloudMat->GetMatInfo() == nullptr)
        {
            return LegacyConversionResult::Failed;
        }

        LmbrCentral::MaterialOwnerRequestBus::Event(newEntityId, &LmbrCentral::MaterialOwnerRequestBus::Events::SetMaterial, distanceCloudMat->GetMatInfo());

        //Multiply X and Y scale by 2!
        //This is needed because the distance cloud scale is used as "size" which goes positive and negative
        //So a scale of 100 on Distance Clouds results in a width of 200
        //The plane we use is a unit plane which has a width of 1 at scale 1 and would have a
        //width of 100 at scale 100. Distance clouds use a scale of 100 by default

        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::Transform scaleModifier = AZ::Transform::CreateScale(AZ::Vector3(2.0f, 2.0f, 1.0f));

        AZ::TransformBus::EventResult(transform, newEntityId, &AZ::TransformBus::Events::GetWorldTM);

        transform = transform * scaleModifier;

        AZ::TransformBus::Event(newEntityId, &AZ::TransformBus::Events::SetWorldTM, transform);

        return LegacyConversionResult::HandledDeleteEntity;
    }

    AZ::EntityId CreateStandin(AZ::EntityId parentId, CBaseObject* entityToConvert, IStatObj* standinObj, const AZStd::string& standinName)
    {
        if (standinObj == nullptr)
        {
            //Return invalid ID
            return AZ::EntityId();
        }

        AZStd::string filePath = standinObj->GetFilePath();
        if (filePath.empty())
        {
            //Return invalid ID
            return AZ::EntityId();
        }

        //Try to get the base mesh asset
        AZ::Data::AssetId meshAssetId;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(meshAssetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, filePath.c_str(), AZ::Data::s_invalidAssetType, false);

        if (!meshAssetId.IsValid())
        {
            //Return invalid ID
            return AZ::EntityId();
        }

        //This might be a material override or the base material, we have no way to know
        _smart_ptr<IMaterial> material = standinObj->GetMaterial();

        //Create the base entity
        //We use a CBaseObj so that we don't have to worry about transforms
        AZ::Outcome<AZ::Entity*, AZ::LegacyConversion::CreateEntityResult> result(nullptr);
        AZ::ComponentTypeList componentList = { "{FC315B86-3280-4D03-B4F0-5553D7D08432}" }; //EditorMeshComponent
        AZ::LegacyConversion::LegacyConversionRequestBus::BroadcastResult(result, &AZ::LegacyConversion::LegacyConversionRequests::CreateConvertedEntity, entityToConvert, true, componentList);

        AZ::Entity *entity = result.GetValue();
        AZ::EntityId entityId = entity->GetId();

        //Change name to something reasonable like "GeomCache2_FirstFrameStandin"
        AZStd::string name = entity->GetName();
        name += "_" + standinName;
        entity->SetName(name);

        LmbrCentral::MeshComponentRequestBus::Event(entityId, &LmbrCentral::MeshComponentRequestBus::Events::SetMeshAsset, meshAssetId);

        //Invisible by default
        LmbrCentral::MeshComponentRequestBus::Event(entityId, &LmbrCentral::MeshComponentRequestBus::Events::SetVisibility, false);

        if (material)
        {
            LmbrCentral::MaterialOwnerRequestBus::Event(entityId, &LmbrCentral::MaterialOwnerRequestBus::Events::SetMaterial, material);
        }

        AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetParent, parentId);

        return entityId;
    }

    LegacyConversionResult ConvertGeomCache(CBaseObject* entityToConvert)
    {
        if (!((AZStd::string(entityToConvert->metaObject()->className()) == "CEntityObject" &&
            AZStd::string(entityToConvert->metaObject()->className()) != "CEntityObject") ||
            (entityToConvert->inherits("CEntityObject") &&
                static_cast<CEntityObject*>(entityToConvert)->GetEntityClass() == "GeomCache")))
        {
            // We don't know how to convert this entity, whatever it is
            return AZ::LegacyConversion::LegacyConversionResult::Ignored;
        }

        //Get the underlying entity object
        CGeomCacheEntity* geomCacheEntity = static_cast<CGeomCacheEntity*>(entityToConvert);
        if (!geomCacheEntity)
        {
            //Object was not a geom cache object but we did not ignore the object
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        IEntity* legacyEntity = geomCacheEntity->GetIEntity();
        if (!legacyEntity)
        {
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        IGeomCacheRenderNode* renderNode = legacyEntity->GetGeomCacheRenderNode(0);
        if (!renderNode)
        {
            return AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        // if we have a parent, and the parent is not yet converted, we ignore this entity!
        // this is not ALWAYS the case - there may be types you wish to create anyway and just put them in the world, so we cannot enforce this
        // inside the CreateConvertedEntity function.
        AZ::Outcome<AZ::Entity*, AZ::LegacyConversion::CreateEntityResult> result(nullptr);
        AZ::ComponentTypeList componentList = { "{045C0C58-C13E-49B0-A471-D4AC5D3FC6BD}" }; //EditorGeomCacheComponent
        AZ::LegacyConversion::LegacyConversionRequestBus::BroadcastResult(result, &AZ::LegacyConversion::LegacyConversionRequests::CreateConvertedEntity, entityToConvert, true, componentList);

        if (!result.IsSuccess())
        {
            // if we failed due to no parent, we keep processing
            return (result.GetError() == AZ::LegacyConversion::CreateEntityResult::FailedNoParent) ? AZ::LegacyConversion::LegacyConversionResult::Ignored : AZ::LegacyConversion::LegacyConversionResult::Failed;
        }

        AZ::Entity *entity = result.GetValue();
        AZ::EntityId entityId = entity->GetId();

        //Retrieve geom cache asset
        AZ::Data::AssetId geomCacheAssetId;
        AZStd::string geomCacheFilePath = renderNode->GetGeomCache()->GetFilePath();
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(geomCacheAssetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, geomCacheFilePath.c_str(), AZ::Data::s_invalidAssetType, false);

        AZ::Data::Asset<LmbrCentral::GeomCacheAsset> geomCacheAsset;
        //If the asset isn't valid, just don't send anything to the component
        //This shouldn't happen unless the legacy entity is setup incorrectly
        if (geomCacheAssetId.IsValid())
        {
            geomCacheAsset.Create(geomCacheAssetId);
        }

        AZ::u32 renderFlags = renderNode->m_dwRndFlags;

        bool visible = (renderFlags & ERF_HIDDEN) == 0; //Visible if not hidden
        EngineSpec minSpec = SpecConversion(static_cast<ESystemConfigSpec>(renderNode->GetMinSpec()));
        _smart_ptr<IMaterial> materialOverride = renderNode->GetMaterialOverride();
        bool loop = renderNode->IsLooping();
        bool playOnStart = true; //We have no way to retrieve this from Lua
        float startTime = 0.0f; //Also no way to retrieve from Lua
        float standinDistance = renderNode->GetStandInDistance();
        float streamInDistance = renderNode->GetStreamInDistance();
        float viewDistanceMultiplier = renderNode->GetViewDistanceMultiplier();
        //We want the base max view dist. GetMaxViewDist from the node gets us the base max view dist * view dist multiplier;
        float maxViewDistance = renderNode->GetMaxViewDist() / viewDistanceMultiplier;
        AZ::u32 lodDistanceRatio = static_cast<AZ::u32>(renderNode->GetLodRatio());
        //The geom cache casts shadows if it has these two flags
        bool castShadows = (renderFlags & (ERF_CASTSHADOWMAPS | ERF_HAS_CASTSHADOWMAPS)) != 0;
        //The geom cache uses vis areas if it's not outdoor only
        bool useVisAreas = (renderFlags & (ERF_OUTDOORONLY)) == 0;

        //Get stand-ins
        //We have to create new entities but only if a stand-in was actually applied
        //We have no data about if the material is a material override so we will always
        //apply the material as an override to the new mesh component
        AZ::EntityId firstFrameStandin = CreateStandin(entityId, entityToConvert, renderNode->GetFirstFrameStandIn(), "FirstFrameStandin");
        AZ::EntityId lastFrameStandin = CreateStandin(entityId, entityToConvert, renderNode->GetLastFrameStandIn(), "LastFrameStandin");
        AZ::EntityId standin = CreateStandin(entityId, entityToConvert, renderNode->GetStandIn(), "Standin");

        //Actually apply everything to the object
        LmbrCentral::GeometryCacheComponentRequestBus::Event(entityId, &LmbrCentral::GeometryCacheComponentRequestBus::Events::SetVisible, visible);
        LmbrCentral::EditorGeometryCacheComponentRequestBus::Event(entityId, &LmbrCentral::EditorGeometryCacheComponentRequestBus::Events::SetMinSpec, minSpec);

        if (materialOverride)
        {
            LmbrCentral::MaterialOwnerRequestBus::Event(entityId, &LmbrCentral::MaterialOwnerRequestBus::Events::SetMaterial, materialOverride);
        }

        LmbrCentral::GeometryCacheComponentRequestBus::Event(entityId, &LmbrCentral::GeometryCacheComponentRequestBus::Events::SetLoop, loop);
        LmbrCentral::EditorGeometryCacheComponentRequestBus::Event(entityId, &LmbrCentral::EditorGeometryCacheComponentRequestBus::Events::SetPlayOnStart, playOnStart);
        LmbrCentral::GeometryCacheComponentRequestBus::Event(entityId, &LmbrCentral::GeometryCacheComponentRequestBus::Events::SetStartTime, startTime);

        LmbrCentral::GeometryCacheComponentRequestBus::Event(entityId, &LmbrCentral::GeometryCacheComponentRequestBus::Events::SetFirstFrameStandIn, firstFrameStandin);
        LmbrCentral::GeometryCacheComponentRequestBus::Event(entityId, &LmbrCentral::GeometryCacheComponentRequestBus::Events::SetLastFrameStandIn, lastFrameStandin);
        LmbrCentral::GeometryCacheComponentRequestBus::Event(entityId, &LmbrCentral::GeometryCacheComponentRequestBus::Events::SetStandIn, standin);

        LmbrCentral::GeometryCacheComponentRequestBus::Event(entityId, &LmbrCentral::GeometryCacheComponentRequestBus::Events::SetStandInDistance, standinDistance);
        LmbrCentral::GeometryCacheComponentRequestBus::Event(entityId, &LmbrCentral::GeometryCacheComponentRequestBus::Events::SetStreamInDistance, streamInDistance);
        LmbrCentral::EditorGeometryCacheComponentRequestBus::Event(entityId, &LmbrCentral::EditorGeometryCacheComponentRequestBus::Events::SetMaxViewDistance, maxViewDistance);
        LmbrCentral::EditorGeometryCacheComponentRequestBus::Event(entityId, &LmbrCentral::EditorGeometryCacheComponentRequestBus::Events::SetViewDistanceMultiplier, viewDistanceMultiplier);
        LmbrCentral::EditorGeometryCacheComponentRequestBus::Event(entityId, &LmbrCentral::EditorGeometryCacheComponentRequestBus::Events::SetLODDistanceRatio, lodDistanceRatio);
        LmbrCentral::EditorGeometryCacheComponentRequestBus::Event(entityId, &LmbrCentral::EditorGeometryCacheComponentRequestBus::Events::SetCastShadows, castShadows);
        LmbrCentral::EditorGeometryCacheComponentRequestBus::Event(entityId, &LmbrCentral::EditorGeometryCacheComponentRequestBus::Events::SetUseVisAreas, useVisAreas);

        LmbrCentral::GeometryCacheComponentRequestBus::Event(entityId, &LmbrCentral::GeometryCacheComponentRequestBus::Events::SetGeomCacheAsset, geomCacheAssetId);

        return AZ::LegacyConversion::LegacyConversionResult::HandledDeleteEntity;
    }
}

namespace AZ
{
    namespace LegacyConversion
    {
        using namespace LegacyConversionInternal;

        Converter::Converter()
        {
            BusConnect();
        }

        typedef LegacyConversionResult(* EntityConvertFunc)(CBaseObject* entityToConvert);

        LegacyConversionResult Converter::ConvertEntity(CBaseObject* entityToConvert)
        {
            if (!entityToConvert)
            {
                // this is illegal.
                return LegacyConversionResult::Failed;
            }

            EntityConvertFunc entityConvertFuncs[] = {
                ConvertPlainEntity,
                ConvertMeshEntity,
                ConvertBasicEntity,
                ConvertEnvironmentLightEntity,
                ConvertLightEntity,
                ConvertCameraObjectEntity,
                ConvertCameraObjectTargetEntity,
                ConvertTagCommentEntity,
                ConvertParticleEntity,
                ConvertProximityTriggerEntity,
                ConvertAreaShapeEntity,
                ConvertAreaBoxEntity,
                ConvertAreaSphereEntity,
                ConvertNavigationSeed,
                //ConvertWindVolumeEntity, Disabled for v1.12
                ConvertNavigationAreaEntity,
                ConvertTagPointEntity,
                ConvertDecalEntity,
                ConvertFogVolumeEntity,
                ConvertDistanceClouds,
                ConvertGeomCache,
            };

            for (EntityConvertFunc convertFunc : entityConvertFuncs)
            {
                LegacyConversionResult result = convertFunc(entityToConvert);
                if (result != LegacyConversionResult::Ignored)
                {
                    return result;
                }
            }

            return LegacyConversionResult::Ignored;
        }

        bool Converter::BeforeConversionBegins()
        {
            return true;
        }

        bool Converter::AfterConversionEnds()
        {
            return true;
        }
    }
}
