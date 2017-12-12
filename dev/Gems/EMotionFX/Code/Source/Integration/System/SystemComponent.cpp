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

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <EMotionFX/Source/SingleThreadScheduler.h>

#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>

#include <Integration/EMotionFXBus.h>
#include <Integration/Assets/ActorAsset.h>
#include <Integration/Assets/MotionAsset.h>
#include <Integration/Assets/MotionSetAsset.h>
#include <Integration/Assets/AnimGraphAsset.h>

#include <Integration/System/SystemComponent.h>

#if defined(EMOTIONFXANIMATION_EDITOR) // EMFX tools / editor includes
// Qt
#   include <QtGui/QSurfaceFormat>
// EMStudio tools and main window registration
#   include <LyViewPaneNames.h>
#   include <AzToolsFramework/API/ViewPaneOptions.h>
#   include <QApplication.h>
#   include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#   include <EMotionStudio/EMStudioSDK/Source/MainWindow.h>
// EMStudio plugins
#   include <EMotionStudio/Plugins/StandardPlugins/Source/LogWindow/LogWindowPlugin.h>
#   include <EMotionStudio/Plugins/StandardPlugins/Source/Outliner/OutlinerPlugin.h>
#   include <EMotionStudio/Plugins/StandardPlugins/Source/CommandBar/CommandBarPlugin.h>
#   include <EMotionStudio/Plugins/StandardPlugins/Source/ActionHistory/ActionHistoryPlugin.h>
#   include <EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionWindowPlugin.h>
#   include <EMotionStudio/Plugins/StandardPlugins/Source/MorphTargetsWindow/MorphTargetsWindowPlugin.h>
#   include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeViewPlugin.h>
#   include <EMotionStudio/Plugins/StandardPlugins/Source/Attachments/AttachmentsPlugin.h>
#   include <EMotionStudio/Plugins/StandardPlugins/Source/SceneManager/SceneManagerPlugin.h>
#   include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/NodeWindowPlugin.h>
#   include <EMotionStudio/Plugins/StandardPlugins/Source/MotionEvents/MotionEventsPlugin.h>
#   include <EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/MotionSetsWindowPlugin.h>
#   include <EMotionStudio/Plugins/StandardPlugins/Source/NodeGroups/NodeGroupsPlugin.h>
#   include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#   include <EMotionStudio/Plugins/RenderPlugins/Source/OpenGLRender/OpenGLRenderPlugin.h>
#endif // EMOTIONFXANIMATION_EDITOR

#include <ISystem.h>

// include required AzCore headers
#include <AzCore/IO/FileIO.h>
#include <AzFramework/API/ApplicationAPI.h>

namespace EMotionFX
{
    namespace Integration
    {
        //////////////////////////////////////////////////////////////////////////
        class EMotionFXEventHandler
            : public EMotionFX::EventHandler
        {
        public:
            AZ_CLASS_ALLOCATOR(EMotionFXEventHandler, EMotionFXAllocator, 0);

            /// Dispatch motion events to listeners via ActorNotificationBus::OnMotionEvent.
            void OnEvent(const EMotionFX::EventInfo& emfxInfo) override
            {
                ActorInstance* actor = emfxInfo.mActorInstance;
                if (actor)
                {
                    const AZ::EntityId owningEntityId(reinterpret_cast<AZ::u64>(actor->GetCustomData()));

                    // Fill engine-compatible structure to dispatch to game code.
                    MotionEvent motionEvent;
                    motionEvent.m_entityId = owningEntityId;
                    motionEvent.m_actorInstance = emfxInfo.mActorInstance;
                    motionEvent.m_motionInstance = emfxInfo.mMotionInstance;
                    motionEvent.m_time = emfxInfo.mTimeValue;
                    motionEvent.m_eventType = emfxInfo.mEvent->GetEventTypeID();
                    motionEvent.m_eventTypeName = EMotionFX::GetEventManager().GetEventTypeString(emfxInfo.mEvent->GetEventTypeID());
                    motionEvent.m_globalWeight = emfxInfo.mGlobalWeight;
                    motionEvent.m_localWeight = emfxInfo.mLocalWeight;
                    motionEvent.m_isEventStart = emfxInfo.mIsEventStart;

                    // Copy parameter string, and truncate if it doesn't fit in fixed storage.
                    if (emfxInfo.mParameters && !emfxInfo.mParameters->GetIsEmpty())
                    {
                        motionEvent.SetParameterString(emfxInfo.mParameters->AsChar(), emfxInfo.mParameters->GetLength());
                    }

                    // Queue the event to flush on the main thread.
                    ActorNotificationBus::QueueEvent(owningEntityId, &ActorNotificationBus::Events::OnMotionEvent, AZStd::move(motionEvent));
                }
            }

            void OnHasLooped(EMotionFX::MotionInstance* motionInstance) override
            {
                ActorInstance* actor = motionInstance->GetActorInstance();
                if (actor)
                {
                    const AZ::EntityId owningEntityId(reinterpret_cast<AZ::u64>(actor->GetCustomData()));
                    ActorNotificationBus::QueueEvent(owningEntityId, &ActorNotificationBus::Events::OnMotionLoop, motionInstance->GetMotion()->GetName());
                }
            }

            void OnStateEntering(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphNode* state)
            {
                ActorInstance* actor = animGraphInstance->GetActorInstance();
                if (actor && state)
                {
                    const AZ::EntityId owningEntityId(reinterpret_cast<AZ::u64>(actor->GetCustomData()));
                    ActorNotificationBus::QueueEvent(owningEntityId, &ActorNotificationBus::Events::OnStateEntering, state->GetName());
                }
            }

            void OnStateEnter(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphNode* state) override
            {
                ActorInstance* actor = animGraphInstance->GetActorInstance();
                if (actor && state)
                {
                    const AZ::EntityId owningEntityId(reinterpret_cast<AZ::u64>(actor->GetCustomData()));
                    ActorNotificationBus::QueueEvent(owningEntityId, &ActorNotificationBus::Events::OnStateEntered, state->GetName());
                }
            }

            void OnStateEnd(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphNode* state)
            {
                ActorInstance* actor = animGraphInstance->GetActorInstance();
                if (actor && state)
                {
                    const AZ::EntityId owningEntityId(reinterpret_cast<AZ::u64>(actor->GetCustomData()));
                    ActorNotificationBus::QueueEvent(owningEntityId, &ActorNotificationBus::Events::OnStateExiting, state->GetName());
                }

            }

            void OnStateExit(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphNode* state) override
            {
                ActorInstance* actor = animGraphInstance->GetActorInstance();
                if (actor && state)
                {
                    const AZ::EntityId owningEntityId(reinterpret_cast<AZ::u64>(actor->GetCustomData()));
                    ActorNotificationBus::QueueEvent(owningEntityId, &ActorNotificationBus::Events::OnStateExited, state->GetName());
                }
            }

            void OnStartTransition(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphStateTransition* transition) override
            {
                ActorInstance* actor = animGraphInstance->GetActorInstance();
                if (actor)
                {
                    const AZ::EntityId owningEntityId(reinterpret_cast<AZ::u64>(actor->GetCustomData()));
                    const char* sourceName = transition->GetSourceNode() ? transition->GetSourceNode()->GetName() : "";
                    const char* targetName = transition->GetTargetNode() ? transition->GetTargetNode()->GetName() : "";
                    ActorNotificationBus::QueueEvent(owningEntityId, &ActorNotificationBus::Events::OnStateTransitionStart, sourceName, targetName);
                }
            }

            void OnEndTransition(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphStateTransition* transition) override
            {
                ActorInstance* actor = animGraphInstance->GetActorInstance();
                if (actor)
                {
                    const AZ::EntityId owningEntityId(reinterpret_cast<AZ::u64>(actor->GetCustomData()));
                    const char* sourceName = transition->GetSourceNode() ? transition->GetSourceNode()->GetName() : "";
                    const char* targetName = transition->GetTargetNode() ? transition->GetTargetNode()->GetName() : "";
                    ActorNotificationBus::QueueEvent(owningEntityId, &ActorNotificationBus::Events::OnStateTransitionEnd, sourceName, targetName);
                }
            }

        };

        //////////////////////////////////////////////////////////////////////////
        class ActorNotificationBusHandler
            : public ActorNotificationBus::Handler
            , public AZ::BehaviorEBusHandler
        {
        public:
            AZ_EBUS_BEHAVIOR_BINDER(ActorNotificationBusHandler, "{D2CD62E7-5FCF-4DC2-85DF-C205D5AB1E8B}", AZ::SystemAllocator,
                OnMotionEvent,
                OnMotionLoop,
                OnStateEntering,
                OnStateEntered,
                OnStateExiting,
                OnStateExited,
                OnStateTransitionStart,
                OnStateTransitionEnd);

            void OnMotionEvent(MotionEvent motionEvent) override
            {
                Call(FN_OnMotionEvent, motionEvent);
            }

            void OnMotionLoop(const char* motionName) override
            {
                Call(FN_OnMotionLoop, motionName);
            }

            void OnStateEntering(const char* stateName) override
            {
                Call(FN_OnStateEntering, stateName);
            }

            void OnStateEntered(const char* stateName) override
            {
                Call(FN_OnStateEntered, stateName);
            }

            void OnStateExiting(const char* stateName) override
            {
                Call(FN_OnStateExiting, stateName);
            }

            void OnStateExited(const char* stateName) override
            {
                Call(FN_OnStateExited, stateName);
            }

            void OnStateTransitionStart(const char* fromState, const char* toState) override
            {
                Call(FN_OnStateTransitionStart, fromState, toState);
            }

            void OnStateTransitionEnd(const char* fromState, const char* toState) override
            {
                Call(FN_OnStateTransitionEnd, fromState, toState);
            }
        };

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::Reflect(AZ::ReflectContext* context)
        {
            // Reflect component for serialization.
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<SystemComponent, AZ::Component>()
                    ->Version(1)
                    ->Field("NumThreads", &SystemComponent::m_numThreads)
                    ;

                if (AZ::EditContext* ec = serializeContext->GetEditContext())
                {
                    ec->Class<SystemComponent>("EMotion FX Animation", "Enables the EMotion FX animation solution")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SystemComponent::m_numThreads, "Number of threads", "Number of threads used internally by EMotion FX")
                        ;
                }
            }

            // Reflect system-level types and EBuses to behavior context.
            AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->EBus<SystemRequestBus>("SystemRequestBus")
                    ;

                behaviorContext->EBus<SystemNotificationBus>("SystemNotificationBus")
                    ;

                behaviorContext->Class<MotionEvent>("MotionEvent")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::Preview)
                    ->Property("entityId", BehaviorValueGetter(&MotionEvent::m_entityId), nullptr)
                    ->Property("parameter", BehaviorValueGetter(&MotionEvent::m_parameter), nullptr)
                    ->Property("eventType", BehaviorValueGetter(&MotionEvent::m_eventType), nullptr)
                    ->Property("eventTypeName", BehaviorValueGetter(&MotionEvent::m_eventTypeName), nullptr)
                    ->Property("time", BehaviorValueGetter(&MotionEvent::m_time), nullptr)
                    ->Property("globalWeight", BehaviorValueGetter(&MotionEvent::m_globalWeight), nullptr)
                    ->Property("localWeight", BehaviorValueGetter(&MotionEvent::m_localWeight), nullptr)
                    ->Property("isEventStart", BehaviorValueGetter(&MotionEvent::m_isEventStart), nullptr)
                    ;

                behaviorContext->EBus<ActorNotificationBus>("ActorNotificationBus")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::Preview)
                    ->Handler<ActorNotificationBusHandler>()
                    ->Event("OnMotionEvent", &ActorNotificationBus::Events::OnMotionEvent)
                    ->Event("OnMotionLoop", &ActorNotificationBus::Events::OnMotionLoop)
                    ->Event("OnStateEntering", &ActorNotificationBus::Events::OnStateEntering)
                    ->Event("OnStateEntered", &ActorNotificationBus::Events::OnStateEntered)
                    ->Event("OnStateExiting", &ActorNotificationBus::Events::OnStateExiting)
                    ->Event("OnStateExited", &ActorNotificationBus::Events::OnStateExited)
                    ->Event("OnStateTransitionStart", &ActorNotificationBus::Events::OnStateTransitionStart)
                    ->Event("OnStateTransitionEnd", &ActorNotificationBus::Events::OnStateTransitionEnd)
                    ;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("EMotionFXAnimationService", 0x3f8a6369));
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("EMotionFXAnimationService", 0x3f8a6369));
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("AssetDatabaseService", 0x3abf5601));
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC("AssetCatalogService", 0xc68ffc57));
        }

        //////////////////////////////////////////////////////////////////////////
        SystemComponent::SystemComponent()
            : m_numThreads(1)
        {
            AZ::TickBus::Handler::m_tickOrder = AZ::TICK_ANIMATION;
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::Init()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::Activate()
        {
            // Start EMotionFX allocator.
            EMotionFXAllocator::Descriptor allocatorDescriptor;
            allocatorDescriptor.m_custom = &AZ::AllocatorInstance<AZ::SystemAllocator>::Get();
            AZ::AllocatorInstance<EMotionFXAllocator>::Create();

            // Initialize MCore, which is EMotionFX's standard library of containers and systems.
            MCore::Initializer::InitSettings coreSettings;
            coreSettings.mMemAllocFunction = &EMotionFXAlloc;
            coreSettings.mMemReallocFunction = &EMotionFXRealloc;
            coreSettings.mMemFreeFunction = &EMotionFXFree;
            coreSettings.mJobExecutionFunction = MCore::JobListExecuteMCoreJobSystem;
            coreSettings.mNumThreads = m_numThreads;
            if (!MCore::Initializer::Init(&coreSettings))
            {
                AZ_Error("EMotion FX Animation", false, "Failed to initialize EMotion FX SDK Core");
                return;
            }

            // Initialize EMotionFX runtime.
            EMotionFX::Initializer::InitSettings emfxSettings;
            emfxSettings.mUnitType = MCore::Distance::UNITTYPE_METERS;

            if (!EMotionFX::Initializer::Init(&emfxSettings))
            {
                AZ_Error("EMotion FX Animation", false, "Failed to initialize EMotion FX SDK Runtime");
                return;
            }

            // Initialize media root to asset cache.
            // \todo We're in touch with the Systems team. Seems like the aliases are initialized pretty late and we can't access it when activating the system component.
            // Until this is resolved, we're going to init everything in the OnCrySystemInitialized().
            SetMediaRoot("@assets@");

            // Register EMotionFX event handler. Ownership of the object is handled by EMotionFX.
            EMotionFX::GetEventManager().AddEventHandler(aznew EMotionFXEventHandler);

            // Setup asset types.
            RegisterAssetTypesAndHandlers();

            SystemRequestBus::Handler::BusConnect();
            AZ::TickBus::Handler::BusConnect();
            CrySystemEventBus::Handler::BusConnect();
            EMotionFXRequestBus::Handler::BusConnect();

#if defined (EMOTIONFXANIMATION_EDITOR)
            AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
#endif // EMOTIONFXANIMATION_EDITOR
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::Deactivate()
        {
#if defined(EMOTIONFXANIMATION_EDITOR)
            if (EMStudio::GetManager())
            {
                EMStudio::Initializer::Shutdown();
                MysticQt::Initializer::Shutdown();
            }

            {
                using namespace AzToolsFramework;
                EditorRequests::Bus::Broadcast(&EditorRequests::UnregisterViewPane, EMStudio::MainWindow::GetEMotionFXPaneName());
            }
#endif // EMOTIONFXANIMATION_EDITOR

            AZ::TickBus::Handler::BusDisconnect();
            CrySystemEventBus::Handler::BusDisconnect();
            EMotionFXRequestBus::Handler::BusDisconnect();

            if (SystemRequestBus::Handler::BusIsConnected())
            {
                SystemRequestBus::Handler::BusDisconnect();

                m_assetHandlers.resize(0);

                EMotionFX::Initializer::Shutdown();
                MCore::Initializer::Shutdown();
            }

            // Memory leaks will be reported.
            AZ::AllocatorInstance<EMotionFXAllocator>::Destroy();
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::OnCrySystemInitialized(ISystem& system, const SSystemInitParams&)
        {
#if !defined(AZ_MONOLITHIC_BUILD)
            // When module is linked dynamically, we must set our gEnv pointer.
            // When module is linked statically, we'll share the application's gEnv pointer.
            gEnv = system.GetGlobalEnvironment();
#endif
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::OnCryEditorInitialized()
        {
            // \todo Right now we're pointing at the @devassets@ location (source) and working from there, because .actor and .motion (motion) aren't yet processed through
            // the FBX pipeline. Once they are, we'll need to update various segments of the Tool to always read from the @assets@ cache, but write to the @devassets@ data/metadata.
            SetMediaRoot("@assets@");

            EMotionFX::GetEMotionFX().InitAssetFolderPaths();
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::OnCrySystemShutdown(ISystem&)
        {
#if !defined(AZ_MONOLITHIC_BUILD)
            gEnv = nullptr;
#endif
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::OnTick(float delta, AZ::ScriptTimePoint timePoint)
        {
            (void)timePoint;

            // Flush events prior to update EMotionFX.
            ActorNotificationBus::ExecuteQueuedEvents();

            // Main EMotionFX runtime update.
            GetEMotionFX().Update(delta);
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::RegisterAnimGraphNodeType(EMotionFX::AnimGraphNode* nodeTemplate)
        {
            EMotionFX::GetAnimGraphManager().GetObjectFactory()->RegisterObjectType(nodeTemplate);
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::RegisterAssetTypesAndHandlers()
        {
            // Initialize asset handlers.
            m_assetHandlers.emplace_back(aznew ActorAssetHandler);
            m_assetHandlers.emplace_back(aznew MotionAssetHandler);
            m_assetHandlers.emplace_back(aznew MotionSetAssetHandler);
            m_assetHandlers.emplace_back(aznew AnimGraphAssetHandler);

            // Add asset types and extensions to AssetCatalog.
            auto assetCatalog = AZ::Data::AssetCatalogRequestBus::FindFirstHandler();
            if (assetCatalog)
            {
                assetCatalog->EnableCatalogForAsset(AZ::AzTypeInfo<ActorAsset>::Uuid());
                assetCatalog->EnableCatalogForAsset(AZ::AzTypeInfo<MotionAsset>::Uuid());
                assetCatalog->EnableCatalogForAsset(AZ::AzTypeInfo<MotionSetAsset>::Uuid());
                assetCatalog->EnableCatalogForAsset(AZ::AzTypeInfo<AnimGraphAsset>::Uuid());

                assetCatalog->AddExtension("actor");        // Actor
                assetCatalog->AddExtension("motion");       // Motion
                assetCatalog->AddExtension("motionset");    // Motion set
                assetCatalog->AddExtension("animgraph");    // Anim graph
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::SetMediaRoot(const char* alias)
        {
            const char* rootPath = AZ::IO::FileIOBase::GetInstance()->GetAlias(alias);
            if (rootPath)
            {
                AZStd::string mediaRootPath = rootPath;
                EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, mediaRootPath);
                EMotionFX::GetEMotionFX().SetMediaRootFolder(mediaRootPath.c_str());
            }
            else
            {
                AZ_Warning("EMotionFX", false, "Failed to set media root because alias \"%s\" could not be resolved.", alias);
            }
        }

#if defined (EMOTIONFXANIMATION_EDITOR)

        //////////////////////////////////////////////////////////////////////////
        void InitializeEMStudioPlugins()
        {
            // Register EMFX plugins.
            EMStudio::PluginManager* pluginManager = EMStudio::GetPluginManager();
            pluginManager->RegisterPlugin(new EMStudio::LogWindowPlugin());
            pluginManager->RegisterPlugin(new EMStudio::OutlinerPlugin());
            pluginManager->RegisterPlugin(new EMStudio::CommandBarPlugin());
            pluginManager->RegisterPlugin(new EMStudio::ActionHistoryPlugin());
            pluginManager->RegisterPlugin(new EMStudio::MotionWindowPlugin());
            pluginManager->RegisterPlugin(new EMStudio::MorphTargetsWindowPlugin());
            pluginManager->RegisterPlugin(new EMStudio::TimeViewPlugin());
            pluginManager->RegisterPlugin(new EMStudio::AttachmentsPlugin());
            pluginManager->RegisterPlugin(new EMStudio::SceneManagerPlugin());
            pluginManager->RegisterPlugin(new EMStudio::NodeWindowPlugin());
            pluginManager->RegisterPlugin(new EMStudio::MotionEventsPlugin());
            pluginManager->RegisterPlugin(new EMStudio::MotionSetsWindowPlugin());
            pluginManager->RegisterPlugin(new EMStudio::NodeGroupsPlugin());
            pluginManager->RegisterPlugin(new EMStudio::AnimGraphPlugin());
            pluginManager->RegisterPlugin(new EMStudio::OpenGLRenderPlugin());
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::NotifyRegisterViews()
        {
            using namespace AzToolsFramework;

            // Construct data folder that is used by the tool for loading assets (images etc.).
            AZStd::string devRootPath = AZ::IO::FileIOBase::GetInstance()->GetAlias("@devroot@");
            devRootPath += "/Gems/EMotionFX/Assets/Editor/";
            EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, devRootPath);

            // Re-initialize EMStudio.
            int argc = 0;
            char** argv = nullptr;

            MysticQt::Initializer::Init("", devRootPath.c_str());
            EMStudio::Initializer::Init(qApp, argc, argv);

            InitializeEMStudioPlugins();

            EMStudio::GetManager()->ExecuteApp();

            AZStd::function<QWidget*()> windowCreationFunc = []()
            {
                return EMStudio::GetMainWindow();
            };

            // Register EMotionFX window with the main editor.
            AzToolsFramework::ViewPaneOptions emotionFXWindowOptions;
            emotionFXWindowOptions.isPreview = true;
            emotionFXWindowOptions.isDeletable = false;
            emotionFXWindowOptions.isDockable = false;
            emotionFXWindowOptions.optionalMenuText = "Animation Editor (PREVIEW)";
            EditorRequests::Bus::Broadcast(&EditorRequests::RegisterViewPane, EMStudio::MainWindow::GetEMotionFXPaneName(), LyViewPane::CategoryTools, emotionFXWindowOptions, windowCreationFunc);
        }

#endif // EMOTIONFXANIMATION_EDITOR
    }
}
