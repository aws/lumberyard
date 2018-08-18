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
#include "NavigationComponent.h"
#include "EditorNavigationUtil.h"

#include <IPathfinder.h>
#include <MathConversion.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Physics/CryCharacterPhysicsBus.h>
#include <LmbrCentral/Physics/CryPhysicsComponentRequestBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#ifdef LMBR_CENTRAL_EDITOR
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#endif

namespace LmbrCentral
{
    // Behavior Context forwarder for NavigationComponentNotificationBus
    class BehaviorNavigationComponentNotificationBusHandler : public NavigationComponentNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorNavigationComponentNotificationBusHandler,"{6D060202-06BA-470E-8F6B-E1982360C752}",AZ::SystemAllocator,
            OnSearchingForPath, OnTraversalStarted, OnTraversalInProgress, OnTraversalComplete, OnTraversalCancelled);

        void OnSearchingForPath(PathfindRequest::NavigationRequestId requestId) override
        {
            Call(FN_OnSearchingForPath, requestId);
        }

        void OnTraversalStarted(PathfindRequest::NavigationRequestId requestId) override
        {
            Call(FN_OnTraversalStarted, requestId);
        }

        void OnTraversalInProgress(PathfindRequest::NavigationRequestId requestId, float distanceRemaining) override
        {
            Call(FN_OnTraversalInProgress, requestId, distanceRemaining);
        }

        void OnTraversalComplete(PathfindRequest::NavigationRequestId requestId) override
        {
            Call(FN_OnTraversalComplete, requestId);
        }

        void OnTraversalCancelled(PathfindRequest::NavigationRequestId requestId) override
        {
            Call(FN_OnTraversalCancelled, requestId);
        }
    };


    PathfindRequest::NavigationRequestId PathfindResponse::s_nextRequestId = kInvalidRequestId;

    //////////////////////////////////////////////////////////////////////////

    void NavigationComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<NavigationComponent, AZ::Component>()
                ->Version(2)
                ->Field("Agent Type", &NavigationComponent::m_agentType)
                ->Field("Agent Speed", &NavigationComponent::m_agentSpeed)
                ->Field("Agent Radius", &NavigationComponent::m_agentRadius)
                ->Field("Arrival Distance Threshold", &NavigationComponent::m_arrivalDistanceThreshold)
                ->Field("Repath Threshold", &NavigationComponent::m_repathThreshold)
                ->Field("Move Physically", &NavigationComponent::m_movesPhysically);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<NavigationComponent>(
                    "Navigation", "The Navigation component provides basic pathfinding and pathfollowing services to an entity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "AI")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Navigation.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Navigation.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-navigation.html")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &NavigationComponent::m_agentSpeed, "Agent Speed",
                        "The speed of the agent while navigating ")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &NavigationComponent::m_agentType, "Agent Type",
                        "Describes the type of the Entity for navigation purposes. ")
#ifdef LMBR_CENTRAL_EDITOR
                        ->Attribute(AZ::Edit::Attributes::StringList, &NavigationComponent::PopulateAgentTypeList)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &NavigationComponent::HandleAgentTypeChanged)
#endif

                    ->DataElement(AZ::Edit::UIHandlers::Default, &NavigationComponent::m_agentRadius, "Agent Radius",
                        "Radius of this Navigation Agent")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                        ->Attribute("Suffix", " m")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &NavigationComponent::m_arrivalDistanceThreshold,
                        "Arrival Distance Threshold", "Describes the distance from the end point that an entity needs to be before its movement is to be stopped and considered complete")
                        ->Attribute("Suffix", " m")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &NavigationComponent::m_repathThreshold,
                        "Repath Threshold", "Describes the distance from its previously known location that a target entity needs to move before a new path is calculated")
                        ->Attribute("Suffix", " m")

                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &NavigationComponent::m_movesPhysically,
                        "Move Physically", "Indicates whether the entity moves under physics or by modifying the Entity Transform");
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<NavigationComponentRequestBus>("NavigationComponentRequestBus")
                ->Event("FindPathToEntity", &NavigationComponentRequestBus::Events::FindPathToEntity)
                ->Event("Stop", &NavigationComponentRequestBus::Events::Stop)
                ->Event("GetAgentSpeed", &NavigationComponentRequestBus::Events::GetAgentSpeed)
                ->Event("SetAgentSpeed", &NavigationComponentRequestBus::Events::SetAgentSpeed);

            behaviorContext->EBus<NavigationComponentNotificationBus>("NavigationComponentNotificationBus")
                ->Handler<BehaviorNavigationComponentNotificationBusHandler>();
        }
    }

    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Pathfind response
    //! @param navComponent The navigation component being serviced by this response
    PathfindResponse::PathfindResponse()
        : m_requestId(kInvalidRequestId)
        , m_pathfinderRequestId(kInvalidRequestId)
        , m_responseStatus(PathfindResponse::Status::Uninitialized)
        , m_navigationComponent(nullptr)
        , m_previousAgentVelocity(AZ::Vector3::CreateZero())
        , m_pathFollower(nullptr)
    {
    }

    void PathfindResponse::SetupForNewRequest(NavigationComponent* ownerComponent, const PathfindRequest& request)
    {
        AZ_Assert(ownerComponent, "Invalid parent component.");

        m_navigationComponent = ownerComponent;
        m_request = request;
        m_requestId = ++s_nextRequestId;
        m_currentDestination = request.GetDestinationLocation();
        m_previousAgentVelocity = AZ::Vector3::CreateZero();

        // Reset State information
        m_pathfinderRequestId = kInvalidRequestId;
        m_currentPath.reset();

        // Setup pathfollower instance.
        PathFollowerParams params;
        params.endAccuracy = m_navigationComponent->GetArrivalDistance();
        params.normalSpeed = m_navigationComponent->GetAgentSpeed();
        params.passRadius = m_navigationComponent->GetAgentRadius();
        params.minSpeed = params.normalSpeed * 0.8f;
        params.maxSpeed = params.normalSpeed * 1.2f;
        params.stopAtEnd = true;
        m_pathFollower = gEnv->pAISystem ? gEnv->pAISystem->CreateAndReturnNewDefaultPathFollower(params, m_pathObstacles) : nullptr;

        // Disconnect from any notifications from earlier requests
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AZ::EntityBus::Handler::BusDisconnect();

        SetStatus(Status::Initialized);

        // If this request is to follow a moving entity then connect to the transform notification bus for the target
        if (m_request.HasTargetEntity())
        {
            SetStatus(Status::WaitingForTargetEntity);
            AZ::TransformNotificationBus::Handler::BusConnect(m_request.GetTargetEntityId());
            AZ::EntityBus::Handler::BusConnect(m_request.GetTargetEntityId());
        }
    }

    void PathfindResponse::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        if (!m_navigationComponent)
        {
            return;
        }

        if ((m_responseStatus == Status::TraversalStarted) || (m_responseStatus == Status::TraversalInProgress))
        {
            auto delta = (world.GetPosition() - GetCurrentDestination()).GetLength();

            if (delta > m_navigationComponent->m_repathThreshold)
            {
                m_currentDestination = world.GetPosition();
                SetPathfinderRequestId(m_navigationComponent->RequestPath());
            }
        }
    }

    void PathfindResponse::OnEntityActivated(const AZ::EntityId&)
    {
        // Get the target entity's position
        AZ::Transform entityTransform = AZ::Transform::CreateIdentity();
        EBUS_EVENT_ID_RESULT(entityTransform, m_request.GetTargetEntityId(), AZ::TransformBus, GetWorldTM);
        m_currentDestination = entityTransform.GetPosition();

        if (m_responseStatus == Status::WaitingForTargetEntity)
        {
            AZ::EntityBus::Handler::BusDisconnect();
            m_navigationComponent->FindPathImpl();
        }
    }

    void PathfindResponse::OnEntityDeactivated(const AZ::EntityId&)
    {
        AZ::EntityBus::Handler::BusDisconnect();
    }

    void PathfindResponse::SetOwningComponent(NavigationComponent* navComponent)
    {
        m_navigationComponent = navComponent;
    }

    const PathfindRequest& PathfindResponse::GetRequest() const
    {
        return m_request;
    }

    PathfindRequest::NavigationRequestId PathfindResponse::GetRequestId() const
    {
        return m_requestId;
    }

    PathfindResponse::PathfinderRequestId PathfindResponse::GetPathfinderRequestId() const
    {
        return m_pathfinderRequestId;
    }

    void PathfindResponse::SetPathfinderRequestId(PathfinderRequestId pathfinderRequestId)
    {
        m_pathfinderRequestId = pathfinderRequestId;
    }

    const AZ::Vector3& PathfindResponse::GetCurrentDestination() const
    {
        return m_currentDestination;
    }

    PathfindResponse::Status PathfindResponse::GetStatus() const
    {
        return m_responseStatus;
    }

    void PathfindResponse::SetStatus(Status status)
    {
        m_responseStatus = status;

        // If the traversal was cancelled or completed and the request was following an entity
        if ((status >= Status::TraversalComplete) && m_request.HasTargetEntity())
        {
            // Disconnect from any notifications on the transform bust
            AZ::TransformNotificationBus::Handler::BusDisconnect();
        }
    }

    void PathfindResponse::SetCurrentPath(const INavPathPtr& currentPath)
    {
        m_currentPath = currentPath;

        if (m_pathFollower)
        {
            m_pathFollower->AttachToPath(m_currentPath.get());
        }
    }

    INavPathPtr PathfindResponse::GetCurrentPath()
    {
        return m_currentPath;
    }

    void PathfindResponse::Reset()
    {
        PathfindResponse::Status lastResponseStatus = GetStatus();

        // If there is already a Request being serviced
        if (lastResponseStatus > PathfindResponse::Status::Initialized
            && lastResponseStatus < PathfindResponse::Status::TraversalComplete)
        {
            // If the pathfinding request was still being serviced by the pathfinder
            if (lastResponseStatus >= PathfindResponse::Status::SearchingForPath
                && lastResponseStatus <= PathfindResponse::Status::TraversalInProgress)
            {
                // and If the request was a valid one
                if (GetRequestId() != PathfindResponse::kInvalidRequestId)
                {
                    // Cancel that request with the pathfinder
                    if (gEnv->pAISystem)
                    {
                        IMNMPathfinder* pathFinder = gEnv->pAISystem->GetMNMPathfinder();
                        pathFinder->CancelPathRequest(GetPathfinderRequestId());
                    }
                }
            }

            // Indicate that traversal on this request was cancelled
            SetStatus(PathfindResponse::Status::TraversalCancelled);

            // Inform every listener on this entity that traversal was cancelled.
            EBUS_EVENT_ID(m_navigationComponent->GetEntityId(), NavigationComponentNotificationBus,
                OnTraversalCancelled, GetRequestId());
        }

        m_pathFollower.reset();
    }

    const AZ::Vector3& PathfindResponse::GetLastKnownAgentVelocity() const
    {
        return m_previousAgentVelocity;
    }

    void PathfindResponse::SetLastKnownAgentVelocity(const AZ::Vector3& newVelocity)
    {
        m_previousAgentVelocity = newVelocity;
    }

    IPathFollowerPtr PathfindResponse::GetPathFollower()
    {
        return m_pathFollower;
    }

    //////////////////////////////////////////////////////////////////////////

    NavigationComponent::NavigationComponent()
        : m_agentSpeed(1.f)
        , m_agentRadius(4.f)
        , m_arrivalDistanceThreshold(0.25f)
        , m_repathThreshold(1.f)
        , m_movesPhysically(true)
    {
    }

    //////////////////////////////////////////////////////////////////////////
    // AZ::Component interface implementation
    void NavigationComponent::Init()
    {
        m_lastResponseCache.SetOwningComponent(this);
        if (gEnv->pAISystem)
        {
            m_agentTypeId = gEnv->pAISystem->GetNavigationSystem()->GetAgentTypeID(m_agentType.c_str());
        }
    }

    void NavigationComponent::Activate()
    {
        NavigationComponentRequestBus::Handler::BusConnect(m_entity->GetId());
        AZ::TransformNotificationBus::Handler::BusConnect(m_entity->GetId());

        EBUS_EVENT_ID_RESULT(m_entityTransform, m_entity->GetId(), AZ::TransformBus, GetWorldTM);
    }

    void NavigationComponent::Deactivate()
    {
        NavigationComponentRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();

        Reset();
    }
#ifdef LMBR_CENTRAL_EDITOR
    float NavigationComponent::CalculateAgentNavigationRadius(const char* agentTypeName)
    {
        float agentRadius = -1.0f;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(
            agentRadius, &AzToolsFramework::EditorRequests::Bus::Events::CalculateAgentNavigationRadius
            , agentTypeName);

        return agentRadius;
    }

    const char* NavigationComponent::GetDefaultAgentNavigationTypeName()
    {
        const char* agentTypeName = "";
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(
            agentTypeName, &AzToolsFramework::EditorRequests::Bus::Events::GetDefaultAgentNavigationTypeName);

        return agentTypeName;
    }

    AZStd::vector<AZStd::string> NavigationComponent::PopulateAgentTypeList()
    {
        if (m_agentType.size() == 0)
        {
            // If no previously stored agent type select a default one (usually on component added)
            m_agentType = GetDefaultAgentNavigationTypeName();
        }
        HandleAgentTypeChanged();
        return LmbrCentral::PopulateAgentTypeList();
    }

    AZ::u32 NavigationComponent::HandleAgentTypeChanged()
    {
        float agentRadius = CalculateAgentNavigationRadius(m_agentType.c_str());
        if (agentRadius >= 0.0f)
        {
            m_agentRadius = agentRadius;
        }
        else
        {
            AZ_Error("Editor", false, "Unable to find navigation radius data for agent type '%s'", m_agentType.c_str());
        }
        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }
#endif
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // NavigationComponentRequestBus::Handler interface implementation

    PathfindRequest::NavigationRequestId NavigationComponent::FindPath(const PathfindRequest& request)
    {
        // If neither the position nor the destination has been set
        if (!(request.HasTargetEntity() || request.HasTargetLocation()))
        {
            // Return an invalid id to indicate that the request is bad
            return PathfindResponse::kInvalidRequestId;
        }

        // Reset the Navigation component to deal with a new pathfind request
        Reset();

        m_lastResponseCache.SetupForNewRequest(this, request);

        if (!request.HasTargetEntity())
        {
            FindPathImpl();
        }

        return m_lastResponseCache.GetRequestId();
    }

    void NavigationComponent::FindPathImpl()
    {
        // Request for a path
        PathfindResponse::PathfinderRequestId pathfinderRequestID = RequestPath();
        m_lastResponseCache.SetPathfinderRequestId(pathfinderRequestID);

        if (pathfinderRequestID != MNM::Constants::eQueuedPathID_InvalidID)
        {
            // Indicate that the path is being looked for
            m_lastResponseCache.SetStatus(PathfindResponse::Status::SearchingForPath);

            // Inform every listener on this entity about the "Searching For Path" event
            EBUS_EVENT_ID(m_entity->GetId(), NavigationComponentNotificationBus,
                          OnSearchingForPath, m_lastResponseCache.GetRequestId());
        }
        else
        {
            m_lastResponseCache.SetStatus(PathfindResponse::Status::TraversalCancelled);

            // Inform every listener on this entity about the "Traversal cancelled" event
            EBUS_EVENT_ID(m_entity->GetId(), NavigationComponentNotificationBus,
                OnTraversalCancelled, m_lastResponseCache.GetRequestId());
        }
    }

    PathfindResponse::PathfinderRequestId NavigationComponent::RequestPath()
    {
        // Create a new pathfind request
        MNMPathRequest pathfinderRequest;

        // 1. Set the current entity's position as the start location
        pathfinderRequest.startLocation = AZVec3ToLYVec3(m_entityTransform.GetPosition());

        // 2. Set the requested destination
        pathfinderRequest.endLocation = AZVec3ToLYVec3(m_lastResponseCache.GetCurrentDestination());

        // 3. Set the type of the Navigation agent
        pathfinderRequest.agentTypeID = m_agentTypeId;

        // 4. Set the callback
        pathfinderRequest.resultCallback = functor(*this, &NavigationComponent::OnPathResult);

        // 5. Request the path.
        IMNMPathfinder* pathFinder = gEnv->pAISystem ? gEnv->pAISystem->GetMNMPathfinder() : nullptr;
        return pathFinder ? pathFinder->RequestPathTo(this, pathfinderRequest) : 0;
    }

    void NavigationComponent::OnPathResult(const MNM::QueuedPathID& pathfinderRequestId, MNMPathRequestResult& result)
    {
        // If the pathfinding result is for the latest pathfinding request (Otherwise ignore)
        if (pathfinderRequestId == m_lastResponseCache.GetPathfinderRequestId())
        {
            if (result.HasPathBeenFound() &&
                (m_lastResponseCache.GetRequestId() != PathfindResponse::kInvalidRequestId))
            {
                m_lastResponseCache.SetCurrentPath(result.pPath->Clone());

                // If this request was in fact looking for a path (and this isn't just a path update request)
                if (m_lastResponseCache.GetStatus() == PathfindResponse::Status::SearchingForPath)
                {
                    // Set the status of this request
                    m_lastResponseCache.SetStatus(PathfindResponse::Status::PathFound);

                    // Inform every listener on this entity that a path has been found
                    bool shouldPathBeTraversed = true;

                    EBUS_EVENT_ID_RESULT(shouldPathBeTraversed, m_entity->GetId(),
                        NavigationComponentNotificationBus,
                        OnPathFound, m_lastResponseCache.GetRequestId(), m_lastResponseCache.GetCurrentPath());

                    if (shouldPathBeTraversed)
                    {
                        // Connect to tick bus
                        AZ::TickBus::Handler::BusConnect();

                        //  Set the status of this request
                        m_lastResponseCache.SetStatus(PathfindResponse::Status::TraversalStarted);

                        // Inform every listener on this entity that traversal is in progress
                        EBUS_EVENT_ID(m_entity->GetId(), NavigationComponentNotificationBus,
                            OnTraversalStarted, m_lastResponseCache.GetRequestId());
                    }
                    else
                    {
                        // Set the status of this request
                        m_lastResponseCache.SetStatus(PathfindResponse::Status::TraversalCancelled);

                        // Inform every listener on this entity that a path could not be found
                        EBUS_EVENT_ID(m_entity->GetId(), NavigationComponentNotificationBus,
                            OnTraversalCancelled, m_lastResponseCache.GetRequestId());
                    }
                }
            }
            else
            {
                // Set the status of this request
                m_lastResponseCache.SetStatus(PathfindResponse::Status::TraversalCancelled);

                // Inform every listener on this entity that a path could not be found
                EBUS_EVENT_ID(m_entity->GetId(), NavigationComponentNotificationBus,
                    OnTraversalCancelled, m_lastResponseCache.GetRequestId());
            }
        }
    }

    void NavigationComponent::Stop(PathfindRequest::NavigationRequestId requestId)
    {
        if ((m_lastResponseCache.GetRequestId() == requestId)
            && requestId != PathfindResponse::kInvalidRequestId)
        {
            Reset();
        }
    }

    float NavigationComponent::GetAgentSpeed()
    {
        return m_agentSpeed;
    }

    void NavigationComponent::SetAgentSpeed(float agentSpeed)
    {
        m_agentSpeed = agentSpeed;

        IPathFollowerPtr pathFollower = m_lastResponseCache.GetPathFollower();

        if (pathFollower)
        {
            PathFollowerParams currentParams = pathFollower->GetParams();
            currentParams.normalSpeed = agentSpeed;
            currentParams.minSpeed = currentParams.normalSpeed * 0.8f;
            currentParams.maxSpeed = currentParams.normalSpeed * 1.2f;

            pathFollower->SetParams(currentParams);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // AZ::TickBus::Handler implementation

    void NavigationComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        // If there isn't a valid path
        if (!m_lastResponseCache.GetCurrentPath())
        {
            // Come back next frame
            return;
        }

        pe_status_dynamics dynamics;
        if (m_movesPhysically)
        {
            EBUS_EVENT_ID(GetEntityId(), CryPhysicsComponentRequestBus, GetPhysicsStatus, dynamics);
            m_lastResponseCache.SetLastKnownAgentVelocity(LYVec3ToAZVec3(dynamics.v));
        }

        // Update path-following and extract desired velocity.
        AZ::Vector3 velocity = AZ::Vector3::CreateZero();
        float distanceToEnd = 0.f;
        auto pathFollower = m_lastResponseCache.GetPathFollower();
        if (pathFollower)
        {
            const AZ::Vector3 agentPosition = m_entityTransform.GetPosition();
            const AZ::Vector3 agentVelocity = m_lastResponseCache.GetLastKnownAgentVelocity();

            PathFollowResult result;

            const bool arrived = pathFollower->Update(
                result,
                AZVec3ToLYVec3(agentPosition),
                AZVec3ToLYVec3(agentVelocity),
                deltaTime);

            velocity = LYVec3ToAZVec3(result.velocityOut);
            distanceToEnd = result.distanceToEnd;
        }

        if (velocity == AZ::Vector3::CreateZero())
        {
            if (m_movesPhysically)
            {
                if (CryCharacterPhysicsRequestBus::FindFirstHandler(GetEntityId()))
                {
                    EBUS_EVENT_ID(GetEntityId(), LmbrCentral::CryCharacterPhysicsRequestBus, RequestVelocity, AZ::Vector3::CreateZero(), 0);
                }
                else if (CryPhysicsComponentRequestBus::FindFirstHandler(GetEntityId()))
                {
                    pe_action_set_velocity setVel;
                    setVel.v = ZERO;
                    EBUS_EVENT_ID(GetEntityId(), CryPhysicsComponentRequestBus, ApplyPhysicsAction, setVel, false);
                }
            }

            // Set the status of this request
            m_lastResponseCache.SetStatus(PathfindResponse::Status::TraversalComplete);
            // Reset the pathfinding component
            Reset();

            // Inform every listener on this entity that the path has been finished
            EBUS_EVENT_ID(m_entity->GetId(), NavigationComponentNotificationBus,
                OnTraversalComplete, m_lastResponseCache.GetRequestId());
        }
        else
        {
            if (m_movesPhysically)
            {
                if (CryCharacterPhysicsRequestBus::FindFirstHandler(GetEntityId()))
                {
                    EBUS_EVENT_ID(GetEntityId(), LmbrCentral::CryCharacterPhysicsRequestBus, RequestVelocity, velocity, 0);
                }
                else if (CryPhysicsComponentRequestBus::FindFirstHandler(GetEntityId()))
                {
                    float entityMass = dynamics.mass;
                    AZ::Vector3 currentVelocity = LYVec3ToAZVec3(dynamics.v);

                    AZ::Vector3 forceRequired = ((velocity - currentVelocity) * entityMass);
                    forceRequired.SetZ(0);
                    pe_action_impulse applyImpulse;
                    applyImpulse.impulse = AZVec3ToLYVec3(forceRequired);
                    EBUS_EVENT_ID(GetEntityId(), CryPhysicsComponentRequestBus, ApplyPhysicsAction, applyImpulse, false);
                }
                else
                {
                    AZ_WarningOnce("NavigationComponent", false, "Entity cannot be moved physically; No physics component %llu", static_cast<AZ::u64>(GetEntityId()));
                }
            }
            else
            {
                // Set the position of the entity
                AZ::Transform newEntityTransform = m_entityTransform;
                AZ::Vector3 movementDelta = velocity * deltaTime;
                AZ::Vector3 currentPosition = m_entityTransform.GetPosition();
                AZ::Vector3 newPosition = m_entityTransform.GetPosition() + movementDelta;
                newEntityTransform.SetPosition(newPosition);
                EBUS_EVENT_ID(m_entity->GetId(), AZ::TransformBus, SetWorldTM, newEntityTransform);

                m_lastResponseCache.SetLastKnownAgentVelocity(velocity);
            }

            m_lastResponseCache.SetStatus(PathfindResponse::Status::TraversalInProgress);

            // Inform every listener on this entity that the path has been finished
            EBUS_EVENT_ID(m_entity->GetId(), NavigationComponentNotificationBus,
                OnTraversalInProgress, m_lastResponseCache.GetRequestId(), distanceToEnd);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // AZ::TransformNotificationBus::Handler implementation
    // If the transform on the entity has changed
    void NavigationComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_entityTransform = world;
    }

    //////////////////////////////////////////////////////////////////////////
    // NavigationComponent implementation

    void NavigationComponent::Reset()
    {
        m_lastResponseCache.Reset();

        // Disconnect from tick bus
        AZ::TickBus::Handler::BusDisconnect();
    }

    PathfindRequest::NavigationRequestId NavigationComponent::FindPathToEntity(AZ::EntityId targetEntityId)
    {
        PathfindRequest request;
        request.SetTargetEntityId(targetEntityId);
        return FindPath(request);
    }
} // namespace LmbrCentral
