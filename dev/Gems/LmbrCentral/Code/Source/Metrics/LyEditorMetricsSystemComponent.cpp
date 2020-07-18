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

#ifdef METRICS_SYSTEM_COMPONENT_ENABLED

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>

#include "LyEditorMetricsSystemComponent.h"
#include "LyScopedMetricsEvent.h"
#include "ActionMetricsTracker.h"

namespace MetricsEventData
{
    // Event Names
    static const char* Navigation = "ComponentEntityNavigationEvent";
    static const char* InstantiateSlice = "InstantiateSliceEvent";
    static const char* CreateEntity = "CreateEntityEvent";
    static const char* DeleteEntity = "DeleteEntityEvent";
    static const char* AddComponent = "AddComponentEvent";
    static const char* RemoveComponent = "RemoveComponentEvent";
    static const char* UpdateTransformParentEntity = "UpdateTransformParentEntityEvent";
    static const char* Undo = "UndoEvent";
    static const char* Redo = "RedoEvent";
    static const char* Clone = "CloneEvent";
    static const char* LegacyEntityCreated = "LegacyEntityCreated";
    static const char* EditorOperationEvent = "EditorOperation";
    static const char* EnterComponentMode = "EnterComponentModeEvent";
    static const char* LeaveComponentMode = "LeaveComponentModeEvent";
    static const char* EnableNewViewportInteractionModel = "EnableNewViewportInteractionModel";
    static const char* DisableNewViewportInteractionModel = "DisableNewViewportInteractionModel";

    // Attribute Keys
    static const char* SliceIdentifier = "SliceId";
    static const char* EntityId = "EntityId";
    static const char* NewParentId = "NewParentId";
    static const char* OldParentId = "OldParentId";
    static const char* SelectedComponent = "SelectedComponent";
    static const char* NavigationBehaviour = "NavigationBehaviour";
    static const char* NavigationActionGroupId = "NavigationActionGroupId";
    static const char* LegacyEntityType = "EntityType";
    static const char* LegacyEntityScriptType = "EntityScriptType";
    static const char* SourceUuid = "Source";
    static const char* SourceExtension = "Extension";
    static const char* SourceNumberOfProducts = "NumberOfProducts";
    static const char* EditorOperationName = "Operation";


    const char* GetMetricNameForEvent(EEditorNotifyEvent aEventId)
    {
        struct EventNameInfo
        {
            EEditorNotifyEvent event;
            const char* name;
        };
        static const EventNameInfo s_eventNameMap[] =
        {
            { eNotify_OnInit, "OnInit" },
            { eNotify_OnBeginNewScene, "OnBeginNewScene" },
            { eNotify_OnEndNewScene, "OnEndNewScene" },
            { eNotify_OnBeginSceneOpen, "OnBeginSceneOpen" },
            { eNotify_OnEndSceneOpen, "OnEndSceneOpen" },
            { eNotify_OnBeginSceneSave, "OnBeginSceneSave" },
            { eNotify_OnBeginLayerExport, "OnBeginLayerExport" },
            { eNotify_OnEndLayerExport, "OnEndLayerExport" },
            { eNotify_OnCloseScene, "OnCloseScene" },
            { eNotify_OnMissionChange, "OnMissionChange" },
            { eNotify_OnBeginLoad, "OnBeginLoad" },
            { eNotify_OnEndLoad, "OnEndLoad" },
            { eNotify_OnBeginCreate, "OnBeginCreate" },
            { eNotify_OnEndCreate, "OnEndCreate" },
            { eNotify_OnExportToGame, "OnExportToGame" },
            { eNotify_OnEditModeChange, "OnEditModeChange" },
            { eNotify_OnEditToolChange, "OnEditToolChange" },
            { eNotify_OnBeginGameMode, "OnBeginGameMode" },
            { eNotify_OnEndGameMode, "OnEndGameMode" },
            { eNotify_OnSelectionChange, "OnSelectionChange" },
            { eNotify_OnPlaySequence, "OnPlaySequence" },
            { eNotify_OnStopSequence, "OnStopSequence" },
            { eNotify_OnOpenGroup, "OnOpenGroup" },
            { eNotify_OnCloseGroup, "OnCloseGroup" },
            { eNotify_OnTerrainRebuild, "OnTerrainRebuild" },
            { eNotify_OnBeginTerrainRebuild, "OnBeginTerrainRebuild" },
            { eNotify_OnEndTerrainRebuild, "OnEndTerrainRebuild" },
            { eNotify_OnLayerImportBegin, "OnLayerImportBegin" },
            { eNotify_OnLayerImportEnd, "OnLayerImportEnd" },
            { eNotify_OnAddAWSProfile, "OnAddAWSProfile" },
            { eNotify_OnSwitchAWSProfile, "OnSwitchAWSProfile" },
            { eNotify_OnSwitchAWSDeployment, "OnSwitchAWSDeployment" },
            { eNotify_OnFirstAWSUse, "OnFirstAWSUse" }
        };

        for (const EventNameInfo& eventNameInfo : s_eventNameMap)
        {
            if (eventNameInfo.event == aEventId)
            {
                return eventNameInfo.name;
            }
        }

        return nullptr;
    }
}


namespace LyEditorMetrics
{
    static const char* s_navigationTriggerStrings[AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::Count]
        = {
        "RightClickMenu",
        "ButtonClick",
        "DragAndDrop",
        "Shortcut",

        "ButtonClickToolbar",
        "LeftClickMenu",
        };

    void LyEditorMetricsSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<LyEditorMetricsSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<LyEditorMetricsSystemComponent>("LyEditorMetrics", "Optionally send Lumberyard usage data to Amazon so that we can create a better user experience.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ;
            }
        }
    }

    void LyEditorMetricsSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("LyEditorMetricsService", 0xb55bcc16));
    }

    void LyEditorMetricsSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("LyEditorMetricsService", 0xb55bcc16));
    }

    void LyEditorMetricsSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void LyEditorMetricsSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    LyEditorMetricsSystemComponent::LyEditorMetricsSystemComponent()
    {
        InitializeLegacyEntityList();
        InitializeLegacyScriptEntityList();
        InitializeActionBrowserData();
    }

    void LyEditorMetricsSystemComponent::OnEditorNotifyEvent(EEditorNotifyEvent event)
    {
        if ((event == eNotify_OnSelectionChange) && (m_batchingSelectionStackSize != 0))
        {
            m_queuedSelectionChangedEvents++;

            return;
        }

        switch (event)
        {
            case eNotify_OnBeginGameMode:
                // stop listening to entity related events while in game mode
                AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusDisconnect();
                break;

            case eNotify_OnEndGameMode:
                // start listening to entity related events while in game mode again
                AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusConnect();
                break;

            default:
                break;
        }

        const char* eventMetricName = MetricsEventData::GetMetricNameForEvent(event);
        if (eventMetricName)
        {
            auto metricId = LyMetrics_CreateEvent(MetricsEventData::EditorOperationEvent);
            LyMetrics_AddAttribute(metricId, MetricsEventData::EditorOperationName, eventMetricName);
            LyMetrics_SubmitEvent(metricId);
        }
    }

    void LyEditorMetricsSystemComponent::BeginUserAction(LyEditorMetricsSystemComponent::NavigationTrigger behaviour)
    {
        // We want to allow this function to be called multiple times, so that UI events
        // can be trapped at multiple levels and not overridden.
        // For instance, keyboard shortcut handlers get hit first, but then end up
        // calling through to the same code as the menu handlers.
        // By making this function callable from multiple levels, the
        // code for trapping shortcuts can fire first, then the menu trap can fire again and be ignored
        if (m_needToFireNavigationEvent)
        {
            return;
        }

        assert(behaviour < EditorMetricsEventsBusTraits::NavigationTrigger::Count);

        // starting a new set of actions here (give them a unique identifier for this session of the Editor)
        m_actionId++;

        // create a string here once so that we can send it to the
        m_actionIdString = AZStd::string::format("%llu", m_actionId);

        // store the navigation information, and we'll fire it when we get an actual event
        m_navigationBehaviour = behaviour;

        // flag that we need to send the navigation data when we get an event
        m_needToFireNavigationEvent = true;
    }

    void LyEditorMetricsSystemComponent::EndUserAction()
    {
        m_needToFireNavigationEvent = false;
    }

    void LyEditorMetricsSystemComponent::EntityCreated(const AZ::EntityId& entityId)
    {
        SendNavigationEventIfNeeded();

        SendEntitiesMetricsEvent(MetricsEventData::CreateEntity, entityId);

        AZ::EntityId parentId = AZ::EntityId();
        EBUS_EVENT_ID_RESULT(parentId, entityId, AZ::TransformBus, GetParentId);
        if (parentId.IsValid())
        {
            SendParentIdMetricsEvent(MetricsEventData::UpdateTransformParentEntity, entityId, parentId, AZ::EntityId());
        }
    }

    void LyEditorMetricsSystemComponent::EntityDeleted(const AZ::EntityId& entityId)
    {
        SendNavigationEventIfNeeded();

        SendEntitiesMetricsEvent(MetricsEventData::DeleteEntity, entityId);
    }

    void LyEditorMetricsSystemComponent::SliceInstantiated(const AZ::Crc32& sliceIdentifier)
    {
        SendNavigationEventIfNeeded();

        SendSliceInstantiatedMetricsEvent(MetricsEventData::InstantiateSlice, sliceIdentifier);
    }

    void LyEditorMetricsSystemComponent::ComponentAdded(const AZ::EntityId& entityId, const AZ::Uuid& componentTypeId)
    {
        SendNavigationEventIfNeeded();

        SendComponentsMetricsEvent(MetricsEventData::AddComponent, entityId, componentTypeId);
    }

    void LyEditorMetricsSystemComponent::ComponentRemoved(const AZ::EntityId& entityId, const AZ::Uuid& componentTypeId)
    {
        SendNavigationEventIfNeeded();

        SendComponentsMetricsEvent(MetricsEventData::RemoveComponent, entityId, componentTypeId);
    }

    void LyEditorMetricsSystemComponent::UpdateTransformParentEntity(const AZ::EntityId& entityId, const AZ::EntityId& newParentId, const AZ::EntityId& oldParentId)
    {
        SendNavigationEventIfNeeded();

        SendParentIdMetricsEvent(MetricsEventData::UpdateTransformParentEntity, entityId, newParentId, oldParentId);
    }

    void LyEditorMetricsSystemComponent::LegacyEntityCreated(const char* entityType, const char* scriptEntityType)
    {
        // check if the entity type is in our white list of entity
        // types that we can send back plain text for
        if (m_legacyEntityNameWhiteList.find(entityType) == m_legacyEntityNameWhiteList.end())
        {
            entityType = "Unknown";
        }

        if ((scriptEntityType != nullptr) && (scriptEntityType[0] != 0))
        {
            if (m_legacyScriptEntityNameWhiteList.find(scriptEntityType) == m_legacyScriptEntityNameWhiteList.end())
            {
                scriptEntityType = "Unknown";
            }
        }

        LyMetrics::LyScopedMetricsEvent scopeMetricsEvent(MetricsEventData::LegacyEntityCreated,
            {
                // Entity Type
                {
                    MetricsEventData::LegacyEntityType, entityType
                },
                {
                    MetricsEventData::LegacyEntityScriptType, scriptEntityType
                }
            });
    }

    void LyEditorMetricsSystemComponent::Undo()
    {
        SendNavigationEventIfNeeded();

        SendUndoRedoMetricsEvent(MetricsEventData::Undo);
    }

    void LyEditorMetricsSystemComponent::Redo()
    {
        SendNavigationEventIfNeeded();

        SendUndoRedoMetricsEvent(MetricsEventData::Redo);
    }

    void LyEditorMetricsSystemComponent::EntitiesCloned()
    {
        SendNavigationEventIfNeeded();

        LyMetrics::LyScopedMetricsEvent cloneMetricsEvent(MetricsEventData::Clone,
            {
                // Navigation unique action group id
                {
                    MetricsEventData::NavigationActionGroupId, m_actionIdString.c_str()
                }
            });
    }

    void LyEditorMetricsSystemComponent::MenuTriggered(const char* menuIdentifier, AzToolsFramework::MetricsActionTriggerType triggerType)
    {
        m_actionTracker->SendMetrics(menuIdentifier, triggerType);
    }

    void LyEditorMetricsSystemComponent::AssetBrowserAction(AzToolsFramework::AssetBrowserActionType actionType, const AZ::Uuid& sourceUuid, const char* extension, int numberOfProducts)
    {
        const char* eventName = m_assetBrowserActionMap[actionType].c_str();

        char hash[AZ::Uuid::MaxStringBuffer];
        sourceUuid.ToString(hash, sizeof(hash));
        const char* sourceUuidStr = hash;

        if (m_extensionWhiteList.find(extension) == m_extensionWhiteList.end())
        {
            // if extension is whitelisted send it as is
            extension = "Unknown";
        }

        LyMetrics::LyScopedMetricsEvent scopeMetricsEvent(eventName,
        {
            // Source uuid
            {
                MetricsEventData::SourceUuid, sourceUuidStr
            },

            // Source extension
            {
                MetricsEventData::SourceExtension, extension
            },

            // Number of products
            {
                MetricsEventData::SourceNumberOfProducts, AZStd::string::format("%u", numberOfProducts)
            }
        });
    }

    void LyEditorMetricsSystemComponent::RegisterAction(QAction* action, const QString& metricsText)
    {
        if (m_actionTracker != nullptr)
        {
            m_actionTracker->RegisterAction(action, metricsText);
        }
    }

    void LyEditorMetricsSystemComponent::UnregisterAction(QAction* action)
    {
        if (m_actionTracker != nullptr)
        {
            m_actionTracker->UnregisterAction(action);
        }
    }

    void LyEditorMetricsSystemComponent::BeginSelectionChange()
    {
        BeforeEntitySelectionChanged();
    }

    void LyEditorMetricsSystemComponent::EndSelectionChange()
    {
        const AzToolsFramework::EntityIdList emptyList = AzToolsFramework::EntityIdList();
        AfterEntitySelectionChanged(emptyList, emptyList);
    }

    void LyEditorMetricsSystemComponent::EnteredComponentMode(
        const AZStd::vector<AZ::EntityId>& entityIds, const AZStd::vector<AZ::Uuid>& componentTypeIds)
    {
        SendNavigationEventIfNeeded();

        SendComponentsMetricsEvent(MetricsEventData::EnterComponentMode, entityIds, componentTypeIds);
    }

    void LyEditorMetricsSystemComponent::LeftComponentMode(
        const AZStd::vector<AZ::EntityId>& entityIds, const AZStd::vector<AZ::Uuid>& componentTypeIds)
    {
        SendNavigationEventIfNeeded();

        SendComponentsMetricsEvent(MetricsEventData::LeaveComponentMode, entityIds, componentTypeIds);
    }

    void LyEditorMetricsSystemComponent::EnabledNewViewportInteractionModel()
    {
        SendNavigationEventIfNeeded();

        LyMetrics::LyScopedMetricsEvent scopeMetricsEvent(MetricsEventData::EnableNewViewportInteractionModel, {});
    }

    void LyEditorMetricsSystemComponent::DisabledNewViewportInteractionModel()
    {
        SendNavigationEventIfNeeded();

        LyMetrics::LyScopedMetricsEvent scopeMetricsEvent(MetricsEventData::DisableNewViewportInteractionModel, {});
    }

    void LyEditorMetricsSystemComponent::BeforeEntitySelectionChanged()
    {
        m_batchingSelectionStackSize++;
    }

    void LyEditorMetricsSystemComponent::AfterEntitySelectionChanged(const AzToolsFramework::EntityIdList&, const AzToolsFramework::EntityIdList&)
    {
        assert(m_batchingSelectionStackSize > 0);

        // check if we had any and deal with them
        if (m_batchingSelectionStackSize > 0)
        {
            m_batchingSelectionStackSize--;

            if ((m_batchingSelectionStackSize == 0) && (m_queuedSelectionChangedEvents > 0))
            {
                OnEditorNotifyEvent(eNotify_OnSelectionChange);
                m_queuedSelectionChangedEvents = 0;
            }
        }
    }

    void LyEditorMetricsSystemComponent::BeforeUndoRedo()
    {
        BeforeEntitySelectionChanged();
    }

    void LyEditorMetricsSystemComponent::AfterUndoRedo()
    {
        const AzToolsFramework::EntityIdList emptyList = AzToolsFramework::EntityIdList();
        AfterEntitySelectionChanged(emptyList, emptyList);
    }

    void LyEditorMetricsSystemComponent::InitializeLegacyEntityList()
    {
        // This list is generated by hand, but we're not developing any new CryEntities, so
        // easiest way to do this.
        const char* entityWhiteListNames[] = {
            "AIAnchor",
            "AIHorizontalOcclusionPlane",
            "AIPath",
            "AIPerceptionModifier",
            "AIPoint",
            "AIReinforcementSpot",
            "AIShape",
            "AreaBox",
            "AreaSolid",
            "AreaSphere",
            "AudioAreaAmbience",
            "AudioAreaEntity",
            "AudioAreaRandom",
            "AudioTriggerSpot",
            "Brush",
            "Camera",
            "CameraTarget",
            "CharAttachHelper",
            "ClipVolume",
            "Cloud",
            "CloudVolume",
            "Comment",
            "ComponentEntity",
            "CoverSurface",
            "Decal",
            "Designer",
            "DistanceCloud",
            "Entity",
            "Entity::AITerritory",
            "Entity::AIWave",
            "Entity::Constraint",
            "Entity::GeomCache",
            "Entity::WindArea",
            "EntityArchetype",
            "EnvironmentProbe",
            "GameVolume",
            "GeomEntity",
            "GravityVolume",
            "Group",
            "Ledge",
            "LedgeStatic",
            "NavigationArea",
            "NavigationSeedPoint",
            "OccluderArea",
            "OccluderPlane",
            "ParticleEntity",
            "Portal",
            "Prefab",
            "ReferencePicture",
            "River",
            "Road",
            "Rope",
            "SequenceObject",
            "Shape",
            "SimpleEntity",
            "SmartObject",
            "SmartObjectHelper",
            "Solid",
            "SplineDistributor",
            "StdEntity",
            "StdSoundObject",
            "StdTagPoint",
            "StdVolume",
            "TagPoint",
            "VehicleComponent",
            "VehicleHelper",
            "VehiclePart",
            "VehiclePrototype",
            "VehicleSeat",
            "VehicleWeapon",
            "VisArea",
            "WaterVolume"
        };

        for (auto name : entityWhiteListNames)
        {
            m_legacyEntityNameWhiteList.insert(name);
        }
    }

    void LyEditorMetricsSystemComponent::InitializeLegacyScriptEntityList()
    {
        // This list is generated by hand, but we're not developing any new CryEntities, so
        // easiest way to do this.
        const char* scriptEntityWhiteListNames[] = {
            "AIAlertness",
            "AIAnchor",
            "AICorpse",
            "AIReinforcementSpot",
            "AISpawner",
            "AITerritory",
            "AIWave",
            "Abrams",
            "ActorEntity",
            "AdvancedDoor",
            "AmmoCrate",
            "AmmoCrateMP",
            "AnimDoor",
            "AnimObject",
            "AreaBezierVolume",
            "AreaBox",
            "AreaRiver",
            "AreaRoad",
            "AreaShape",
            "AreaSolid",
            "AreaSphere",
            "AreaTrigger",
            "Assault",
            "AssaultIntel",
            "AssaultScope",
            "AudioAreaAmbience",
            "AudioAreaEntity",
            "AudioAreaRandom",
            "AudioListener",
            "AudioTriggerSpot",
            "BTBBombBase",
            "BTBBombSite",
            "BasicEntity",
            "BreakableObject",
            "Breakage",
            "CTFBase",
            "CTFFlag",
            "CameraPoint",
            "CameraShake",
            "CameraSource",
            "CameraTarget",
            "CaptureTheFlag",
            "CharacterAttachHelper",
            "CinematicTrigger",
            "ClipVolume",
            "CloneFactory",
            "Cloth",
            "Cloud",
            "CoaxialGun",
            "Comment",
            "Constraint",
            "Corpse",
            "CrashSite",
            "DamageTestEnt",
            "DangerousRigidBody",
            "DeadBody",
            "DecalPlacer",
            "Default",
            "DefaultVehicle",
            "DeflectorShield",
            "DeployableBarrier",
            "DestroyableLight",
            "DestroyableObject",
            "DestroyedVehicle",
            "Door",
            "DoorPanel",
            "DummyPlayer",
            "Elevator",
            "ElevatorSwitch",
            "EntityFlashTag",
            "EnvironmentLight",
            "EnvironmentalWeapon",
            "Explosion",
            "ExtendedClip",
            "Extraction",
            "ExtractionPoint",
            "ExtractionWeaponSpawn",
            "Fan",
            "Flash",
            "Flashlight",
            "FlowgraphEntity",
            "Fog",
            "FogVolume",
            "ForbiddenArea",
            "FragGrenades",
            "GeomCache",
            "GeomEntity",
            "Gladiator",
            "GravityBox",
            "GravitySphere",
            "GravityStream",
            "GravityStreamCap",
            "GravityValve",
            "HMG",
            "HMMWV",
            "HUD_MessagePlane",
            "HUD_Object",
            "Hazard",
            "HealthPack",
            "Human",
            "InstantAction",
            "InteractiveEntity",
            "InteractiveObjectEx",
            "Ironsight",
            "IronsightRifle",
            "Ladder",
            "LedgeObject",
            "LedgeObjectStatic",
            "Light",
            "LightBox",
            "Lightning",
            "LightningArc",
            "LivingEntity",
            "MGbullet",
            "MH60_Blackhawk",
            "MPPath",
            "MannequinObject",
            "MapIconEntity",
            "Mine",
            "MineField",
            "MissionObjective",
            "NavigationSeedPoint",
            "NoAttachmentBarrel",
            "NoAttachmentBottom",
            "NoWeapon",
            "NullAI",
            "ParticleEffect",
            "ParticlePhysics",
            "PickAndThrowWeapon",
            "PickableAmmo",
            "Pistol",
            "PistolBullet",
            "PistolBulletAmmo",
            "PistolBulletIncendiary",
            "PistolIncendiaryAmmo",
            "Player",
            "PlayerHeavy",
            "PlayerModelChanger",
            "PlayerProximityTrigger",
            "PowerStruggle",
            "PrecacheCamera",
            "PressurizedObject",
            "PrismObject",
            "ProceduralObject",
            "ProximityTrigger",
            "Rain",
            "RaisingWater",
            "ReplayActor",
            "ReplayObject",
            "Rifle",
            "RifleBullet",
            "RigidBody",
            "RigidBodyEx",
            "RigidBodyLight",
            "Rocket",
            "RocketLauncher",
            "RopeEntity",
            "ScanSpot",
            "Shake",
            "ShootingTarget",
            "Shotgun",
            "ShotgunAmmo",
            "ShotgunShell",
            "ShotgunShellSolid",
            "ShotgunSolidAmmo",
            "Silencer",
            "SinglePlayer",
            "SmartMine",
            "SmartObject",
            "SmartObjectCondition",
            "SmokeGrenades",
            "Snake",
            "SniperScope",
            "Snow",
            "SpawnGroup",
            "SpawnPoint",
            "SpectatorPoint",
            "SpeedBoat",
            "SpotterScannable",
            "Switch",
            "TacticalEntity",
            "TagNamePoint",
            "TagPoint",
            "Tank125",
            "TankCannon",
            "TeamInstantAction",
            "Tornado",
            "Turret",
            "TurretGun",
            "UIEntity",
            "UiCanvasRefEntity",
            "VTOLSpawnPoint",
            "VehiclePartDetached",
            "VehicleSeatSerializer",
            "VicinityDependentObjectMover",
            "ViewDist",
            "VolumeObject",
            "WaterPuddle",
            "WaterRipplesGenerator",
            "WaterVolume",
            "Wind",
            "WindArea",
            "explosivegrenade",
            "smokegrenade",
            "swat_van"
        };

        for (auto name : scriptEntityWhiteListNames)
        {
            m_legacyScriptEntityNameWhiteList.insert(name);
        }
    }

    void LyEditorMetricsSystemComponent::InitializeActionBrowserData()
    {
        using namespace AzToolsFramework;

        const char* extensionWhiteListNames[] = {
            ".fbx",
            ".cgf",
            ".mtl",
            ".png",
            ".jpg",
            ".tif",
            ".bmp"
        };

        for (auto name : extensionWhiteListNames)
        {
            m_extensionWhiteList.insert(name);
        }

        m_assetBrowserActionMap[AssetBrowserActionType::SourceExpanded] = "AssetBrowserSourceExpandedEvent";
        m_assetBrowserActionMap[AssetBrowserActionType::SourceCollapsed] = "AssetBrowserSourceCollapsedEvent";
        m_assetBrowserActionMap[AssetBrowserActionType::SourceDragged] = "AssetBrowserSourceDraggedEvent";
        m_assetBrowserActionMap[AssetBrowserActionType::ProductDragged] = "AssetBrowserProductDraggedEvent";
    }

    void LyEditorMetricsSystemComponent::SendSliceInstantiatedMetricsEvent(const char* eventName, const AZ::Crc32& sliceIdentifier)
    {
        AZStd::string identifier = AZStd::string::format("%u", static_cast<AZ::u32>(sliceIdentifier));
        LyMetrics::LyScopedMetricsEvent entityMetricsEvent(eventName,
            {
                // Slice identifier
                {
                    MetricsEventData::SliceIdentifier, identifier.c_str()
                },

                // Navigation unique action group id
                {
                    MetricsEventData::NavigationActionGroupId, m_actionIdString.c_str()
                }
            });
    }

    void LyEditorMetricsSystemComponent::SendEntitiesMetricsEvent(const char* eventName, const AZ::EntityId& entityId)
    {
        LyMetrics::LyScopedMetricsEvent entityMetricsEvent(eventName,
            {
                // Entity ID
                {
                    MetricsEventData::EntityId, entityId.ToString().c_str()
                },

                // Navigation unique action group id
                {
                    MetricsEventData::NavigationActionGroupId, m_actionIdString.c_str()
                }
            });
    }

    AZStd::string FindComponentName(const AZ::Uuid& componentTypeId)
    {
        // check if we can send the component name as plain text
        bool canSendPlainName = false;
        AzFramework::MetricsPlainTextNameRegistrationBus::BroadcastResult(
            canSendPlainName, &AzFramework::MetricsPlainTextNameRegistrationBusTraits::IsTypeRegisteredForNameSending, componentTypeId);

        if (canSendPlainName)
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(
                serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

            if (serializeContext != nullptr)
            {
                // if we can get find the class data from the id, return the component type name
                if (const auto classData = serializeContext->FindClassData(componentTypeId))
                {
                    return AZStd::string(classData->m_name);
                }
            }
        }

        // if we cannot set the plain name, or trying to find the plain name failed,
        // set the hash of the UUID for the component as the name
        char hash[AZ::Uuid::MaxStringBuffer];
        componentTypeId.ToString(hash, sizeof(hash));

        return AZStd::string(hash);
    }

    void LyEditorMetricsSystemComponent::SendComponentsMetricsEvent(
        const char* eventName, const AZ::EntityId& entityId, const AZ::Uuid& componentTypeId)
    {
        LyMetrics::LyScopedMetricsEvent componentMetricsEvent(eventName,
            {
                // Entity ID
                {
                    MetricsEventData::EntityId, entityId.ToString().c_str()
                },

                // Component Name
                {
                    MetricsEventData::SelectedComponent, FindComponentName(componentTypeId).c_str()
                },

                // Navigation unique action group id
                {
                    MetricsEventData::NavigationActionGroupId, m_actionIdString.c_str()
                }
            });
    }

    template<typename Container>
    static AZStd::string BuildCommaSeparatedString(
        const Container& container, AZStd::function<AZStd::string(const typename Container::value_type& type)> toString)
    {
        // build a vector of strings
        AZStd::vector<AZStd::string> names;
        names.reserve(container.size());
        for (const auto& type : container)
        {
            names.push_back(toString(type));
        }

        // build a single comma separated string
        AZStd::string combinedNames;
        AzFramework::StringFunc::Join(combinedNames, names.begin(), names.end(), ", ");

        return combinedNames;
    }

    void LyEditorMetricsSystemComponent::SendComponentsMetricsEvent(
        const char* eventName, const AZStd::vector<AZ::EntityId>& entityIds, const AZStd::vector<AZ::Uuid>& componentTypeIds)
    {
        const AZStd::string combinedComponentNames =
            BuildCommaSeparatedString(componentTypeIds, [](const AZ::Uuid& componentTypeId)
            {
                return FindComponentName(componentTypeId);
            });

        const AZStd::string combinedEntityIdStrings =
            BuildCommaSeparatedString(entityIds, [](const AZ::EntityId& entityId)
            {
                return entityId.ToString();
            });

        LyMetrics::LyScopedMetricsEvent componentMetricsEvent(eventName,
            {
                // Entity Ids
                {
                    MetricsEventData::EntityId, combinedEntityIdStrings.c_str()
                },

                // Component names
                {
                    MetricsEventData::SelectedComponent, combinedComponentNames.c_str()
                },

                // Navigation unique action group id
                {
                    MetricsEventData::NavigationActionGroupId, m_actionIdString.c_str()
                }
            });
    }

    void LyEditorMetricsSystemComponent::SendParentIdMetricsEvent(const char* eventName, const AZ::EntityId& entityId, const AZ::EntityId& newParentId, const AZ::EntityId& oldParentId)
    {
        LyMetrics::LyScopedMetricsEvent scopeMetricsEvent(eventName,
            {
                // Entity ID
                {
                    MetricsEventData::EntityId, entityId.ToString().c_str()
                },

                // New Parent ID
                {
                    MetricsEventData::NewParentId, newParentId.IsValid() ? newParentId.ToString().c_str() : ""
                },

                // Old Parent ID
                {
                    MetricsEventData::OldParentId, oldParentId.IsValid() ? oldParentId.ToString().c_str() : ""
                },

                // Navigation unique action group id
                {
                    MetricsEventData::NavigationActionGroupId, m_actionIdString.c_str()
                }
            });
    }

    void LyEditorMetricsSystemComponent::SendNavigationEventIfNeeded()
    {
        if (!m_needToFireNavigationEvent)
        {
            return;
        }

        LyMetrics::LyScopedMetricsEvent navigationMetricsEvent(MetricsEventData::Navigation,
            {
                // Navigation Behavior
                {
                    MetricsEventData::NavigationBehaviour, s_navigationTriggerStrings[m_navigationBehaviour]
                },

                // Navigation unique action group id
                {
                    MetricsEventData::NavigationActionGroupId, m_actionIdString.c_str()
                }
            });

        m_needToFireNavigationEvent = false;
    }

    void LyEditorMetricsSystemComponent::SendUndoRedoMetricsEvent(const char* eventName)
    {
        LyMetrics::LyScopedMetricsEvent undoRedoMetricsEvent(eventName,
            {
                // Navigation unique action group id
                {
                    MetricsEventData::NavigationActionGroupId, m_actionIdString.c_str()
                }
            });
    }

    void LyEditorMetricsSystemComponent::Init()
    {
    }

    void LyEditorMetricsSystemComponent::Activate()
    {
        AzToolsFramework::EditorMetricsEventsBus::Handler::BusConnect();
        AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusConnect();

        m_actionTracker = new ActionMetricsTracker();

        IEditor* editor = nullptr;
        EBUS_EVENT_RESULT(editor, AzToolsFramework::EditorRequests::Bus, GetEditor);
        if (editor != nullptr)
        {
            editor->RegisterNotifyListener(this);
            m_notifierRegistered = true;
        }
        else
        {
            AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        }
    }

    void LyEditorMetricsSystemComponent::Deactivate()
    {
        if (m_notifierRegistered)
        {
            IEditor* editor = nullptr;
            EBUS_EVENT_RESULT(editor, AzToolsFramework::EditorRequests::Bus, GetEditor);
            if (editor != nullptr)
            {
                editor->UnregisterNotifyListener(this);
            }

            m_notifierRegistered = false;
        }

        delete m_actionTracker;
        m_actionTracker = nullptr;

        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorMetricsEventsBus::Handler::BusDisconnect();
    }

    void LyEditorMetricsSystemComponent::NotifyIEditorAvailable(IEditor* editor)
    {
        if (!m_notifierRegistered)
        {
            editor->RegisterNotifyListener(this);
            m_notifierRegistered = true;

            AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        }
    }
}

#endif // #ifdef METRICS_SYSTEM_COMPONENT_ENABLED
