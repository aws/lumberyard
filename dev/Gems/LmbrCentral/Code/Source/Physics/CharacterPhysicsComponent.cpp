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
#include "CharacterPhysicsComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/Entity.h>
#include <MathConversion.h>
#include <ISystem.h>
#include <IPhysics.h>
#include <AzCore/EBus/EBus.h>

namespace LmbrCentral
{
    using AzFramework::PhysicsComponentRequestBus;
    using AzFramework::PhysicsComponentNotificationBus;

    void CharacterPhysicsComponent::CryPlayerPhysicsConfiguration::PlayerDimensions::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<PlayerDimensions>()
                ->Version(1)
                ->Field("Use Capsule", &PlayerDimensions::m_useCapsule)
                ->Field("Collider Radius", &PlayerDimensions::m_colliderRadius)
                ->Field("Collider Half-Height", &PlayerDimensions::m_colliderHalfHeight)
                ->Field("Height Collider", &PlayerDimensions::m_heightCollider)
                ->Field("Height Pivot", &PlayerDimensions::m_heightPivot)
                ->Field("Height Eye", &PlayerDimensions::m_heightEye)
                ->Field("Height Head", &PlayerDimensions::m_heightHead)
                ->Field("Head Radius", &PlayerDimensions::m_headRadius)
                ->Field("Unprojection Direction", &PlayerDimensions::m_unprojectionDirection)
                ->Field("Max Unprojection", &PlayerDimensions::m_maxUnprojection)
                ->Field("Ground Contact Epsilon", &PlayerDimensions::m_groundContactEpsilon)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<PlayerDimensions>(
                    "Player dimensions", "Player dimensions used by CryPhysics")

                    ->DataElement(0, &PlayerDimensions::m_useCapsule,
                    "Use capsule", "Switches between capsule and cylinder collider geometry")

                    ->DataElement(0, &PlayerDimensions::m_colliderRadius,
                    "Collider radius", "Radius of collision cylinder/capsule")
                        ->Attribute("Suffix", " m")
                        ->Attribute("Min", 0.f)

                    ->DataElement(0, &PlayerDimensions::m_colliderHalfHeight,
                    "Collider half-height", "Half-height of straight section of collision cylinder/capsule")
                        ->Attribute("Suffix", " m")
                        ->Attribute("Min", 0.f)

                    ->DataElement(0, &PlayerDimensions::m_heightCollider,
                    "Height collider", "Vertical offset of collision geometry center")
                        ->Attribute("Suffix", " m")

                    ->DataElement(0, &PlayerDimensions::m_heightPivot,
                    "Height pivot", "Offset from central ground position that is considered entity center")
                        ->Attribute("Suffix", " m")

                    ->DataElement(0, &PlayerDimensions::m_heightEye,
                    "Height eye", "Vertical offset of camera")
                        ->Attribute("Suffix", " m")

                    ->DataElement(0, &PlayerDimensions::m_heightHead,
                    "Height head", "Center.z of the head geometry")
                        ->Attribute("Suffix", " m")

                    ->DataElement(0, &PlayerDimensions::m_headRadius,
                    "Head radius", "Radius of the 'head' geometry (used for camera offset)")
                        ->Attribute("Suffix", " m")
                        ->Attribute("Min", 0.f)

                    ->DataElement(0, &PlayerDimensions::m_unprojectionDirection,
                    "Unprojection direction", "Unprojection direction to test in case the new position overlaps with the environment (can be 0 for 'auto')")

                    ->DataElement(0, &PlayerDimensions::m_maxUnprojection,
                    "Max unprojection", "Maximum allowed unprojection")
                        ->Attribute("Suffix", " m")

                    ->DataElement(0, &PlayerDimensions::m_groundContactEpsilon,
                    "Ground contact epsilon", "The amount that the living needs to move upwards before ground contact is lost.")
                        ->Attribute("Suffix", " m")
                        ->Attribute("Min", 0.f);
                ;
            }
        }
    }

    void CharacterPhysicsComponent::CryPlayerPhysicsConfiguration::PlayerDynamics::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<PlayerDynamics>()
                ->Version(1)
                ->Field("Mass", &PlayerDynamics::m_mass)
                ->Field("Inertia", &PlayerDynamics::m_inertia)
                ->Field("Inertia Acceleration", &PlayerDynamics::m_inertiaAcceleration)
                ->Field("Time Impulse Recover", &PlayerDynamics::m_timeImpulseRecover)
                ->Field("Air Control", &PlayerDynamics::m_airControl)
                ->Field("Air Resistance", &PlayerDynamics::m_airResistance)
                ->Field("Use Custom Gravity", &PlayerDynamics::m_useCustomGravity)
                ->Field("Gravity", &PlayerDynamics::m_gravity)
                ->Field("Nod Speed", &PlayerDynamics::m_nodSpeed)
                ->Field("Is Active", &PlayerDynamics::m_isActive)
                ->Field("Release Ground Collider When Not Active", &PlayerDynamics::m_releaseGroundColliderWhenNotActive)
                ->Field("Is Swimming", &PlayerDynamics::m_isSwimming)
                ->Field("Surface Index", &PlayerDynamics::m_surfaceIndex)
                ->Field("Min Fall Angle", &PlayerDynamics::m_minFallAngle)
                ->Field("Min Slide Angle", &PlayerDynamics::m_minSlideAngle)
                ->Field("Max Climb Angle", &PlayerDynamics::m_maxClimbAngle)
                ->Field("Max Jump Angle", &PlayerDynamics::m_maxJumpAngle)
                ->Field("Max Velocity Ground", &PlayerDynamics::m_maxVelocityGround)
                ->Field("Collide With Terrain", &PlayerDynamics::m_collideWithTerrain)
                ->Field("Collide With Static", &PlayerDynamics::m_collideWithStatic)
                ->Field("Collide With Rigid", &PlayerDynamics::m_collideWithRigid)
                ->Field("Collide With Sleeping Rigid", &PlayerDynamics::m_collideWithSleepingRigid)
                ->Field("Collide With Living", &PlayerDynamics::m_collideWithLiving)
                ->Field("Collide With Independent", &PlayerDynamics::m_collideWithIndependent)
                ->Field("RecordCollisions", &PlayerDynamics::m_recordCollisions)
                ->Field("MaxRecordedCollisions", &PlayerDynamics::m_maxRecordedCollisions)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<PlayerDynamics>(
                    "Player Dynamics", "Player movement dynamics used by CryPhysics")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerDynamics::m_mass,
                    "Mass", "Mass in kg")
                        ->Attribute("Suffix", " kg")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerDynamics::m_inertia,
                    "Inertia", "Inertia coefficient, the more it is, the less inertia is; 0 means no inertia")
                        ->Attribute("Min", 0.f)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerDynamics::m_inertiaAcceleration,
                    "Inertia acceleration", "Inertia on acceleration")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerDynamics::m_timeImpulseRecover,
                    "Time impulse recover", "Forcefully turns on inertia for that duration after receiving an impulse")
                        ->Attribute("Min", 0.f)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerDynamics::m_airControl,
                    "Air control", "Air control coefficient 0..1, 1 - special value (total control of movement)")
                        ->Attribute("Min", 0.f)
                        ->Attribute("Max", 1.f)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerDynamics::m_airResistance,
                    "Air resistance", "Standard air resistance")
                        ->Attribute("Min", 0.f)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerDynamics::m_useCustomGravity,
                    "Use custom gravity", "True: use custom gravity. False: use world gravity.")
                        ->Attribute("ChangeNotify", AZ_CRC("RefreshEntireTree", 0xefbc823c))

                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerDynamics::m_gravity,
                    "Gravity", "Custom gravity vector for this object (ignored when 'Use custom gravity' is false)")
                        ->Attribute("Visibility", &PlayerDynamics::m_useCustomGravity)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerDynamics::m_nodSpeed,
                    "Nod speed", "Vertical camera shake speed after landings")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerDynamics::m_isActive,
                    "Is active", "Setting false disables all simulation for the character, apart from moving along the requested velocity")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerDynamics::m_releaseGroundColliderWhenNotActive,
                    "Release ground collider when not active", "When true, if the living entity is not active, the ground collider, if any, will be explicitly released during the simulation step.")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerDynamics::m_isSwimming,
                    "Is swimming", "Whether entity is swimming (is not bound to ground plane)")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerDynamics::m_surfaceIndex,
                    "Surface index", "Surface identifier for collisions")
                        ->Attribute("Min", 0)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Limits")
                        ->Attribute("AutoExpand", true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerDynamics::m_minFallAngle,
                    "Min fall angle", "Player starts falling when slope is steeper than this")
                        ->Attribute("Suffix", " deg")
                        ->Attribute("Min", 0.f)
                        ->Attribute("Max", 90.f)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerDynamics::m_minSlideAngle,
                    "Min slide angle", "If surface slope is more than this angle, player starts sliding")
                        ->Attribute("Suffix", " deg")
                        ->Attribute("Min", 0.f)
                        ->Attribute("Max", 90.f)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerDynamics::m_maxClimbAngle,
                    "Max climb angle", "Player cannot climb surface which slope is steeper than this angle")
                        ->Attribute("Suffix", " deg")
                        ->Attribute("Min", 0.f)
                        ->Attribute("Max", 90.f)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerDynamics::m_maxJumpAngle,
                    "Max jump angle", "Player is not allowed to jump towards ground if this angle is exceeded")
                        ->Attribute("Suffix", " deg")
                        ->Attribute("Min", 0.f)
                        ->Attribute("Max", 90.f)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerDynamics::m_maxVelocityGround,
                    "Max ground velocity", "Player cannot stand on surfaces that are moving faster than this")
                        ->Attribute("Suffix", " m/s")
                        ->Attribute("Min", 0.f)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Collides with types:")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerDynamics::m_collideWithTerrain,
                    "Terrain", "Collide with terrain")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerDynamics::m_collideWithStatic,
                    "Static", "Collide with static entities")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerDynamics::m_collideWithRigid,
                    "Rigid body (active)", "Collide with active rigid bodies")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerDynamics::m_collideWithSleepingRigid,
                    "Rigid body (sleeping)", "Collide with sleeping rigid bodies")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerDynamics::m_collideWithLiving,
                    "Living", "Collide with living entities")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerDynamics::m_collideWithIndependent,
                    "Independent", "Collide with independent entities")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerDynamics::m_recordCollisions, "Record collisions", "Whether or not to record and report collisions with this entity")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerDynamics::m_maxRecordedCollisions, "Number of collisions", "Maximum number of collisions to record and report per frame")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &PlayerDynamics::m_recordCollisions)
                        ->Attribute(AZ::Edit::Attributes::Min, 0)
                    ;
            }
        }
    }

    void CharacterPhysicsComponent::CryPlayerPhysicsConfiguration::Reflect(AZ::ReflectContext* context)
    {
        PlayerDimensions::Reflect(context);
        PlayerDynamics::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<CryPlayerPhysicsConfiguration>()
                ->Version(1)
                ->Field("Player Dimensions", &CryPlayerPhysicsConfiguration::m_dimensions)
                ->Field("Player Dynamics", &CryPlayerPhysicsConfiguration::m_dynamics)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<CryPlayerPhysicsConfiguration>(
                    "CryPhysics Player Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Physics (Legacy)")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CryPlayerPhysicsConfiguration::m_dimensions,
                    "Player Dimensions", "Player dimensions used by CryPhysics")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CryPlayerPhysicsConfiguration::m_dynamics,
                    "Player Dynamics", "Player movement dynamics used by CryPhysics")
                    ;
            }
        }
    }

    pe_player_dimensions CharacterPhysicsComponent::CryPlayerPhysicsConfiguration::PlayerDimensions::GetParams() const
    {
        pe_player_dimensions out;
        out.heightPivot = m_heightPivot;
        out.heightEye = m_heightEye;
        out.sizeCollider.Set(m_colliderRadius, m_colliderRadius, m_colliderHalfHeight);
        out.heightCollider = m_heightCollider;
        out.headRadius = m_headRadius;
        out.heightHead = m_heightHead;
        out.dirUnproj = AZVec3ToLYVec3(m_unprojectionDirection);
        out.maxUnproj = m_maxUnprojection;
        out.bUseCapsule = m_useCapsule;
        out.groundContactEps = m_groundContactEpsilon;

        return out;
    }

    pe_player_dynamics CharacterPhysicsComponent::CryPlayerPhysicsConfiguration::PlayerDynamics::GetParams() const
    {
        pe_player_dynamics out;
        out.kInertia = m_inertia;
        out.kInertiaAccel = m_inertiaAcceleration;
        out.kAirControl = m_airControl;
        out.kAirResistance = m_airResistance;
        if (m_useCustomGravity)
        {
            out.gravity = AZVec3ToLYVec3(m_gravity);
        }
        out.nodSpeed = m_nodSpeed;
        out.bSwimming = m_isSwimming;
        out.mass = m_mass;
        out.surface_idx = m_surfaceIndex;
        out.minSlideAngle = m_minSlideAngle;
        out.maxClimbAngle = m_maxClimbAngle;
        out.maxJumpAngle = m_maxJumpAngle;
        out.minFallAngle = m_minFallAngle;
        out.maxVelGround = m_maxVelocityGround;
        out.timeImpulseRecover = m_timeImpulseRecover;

        out.collTypes = (m_collideWithTerrain ? ent_terrain : 0)
            | (m_collideWithStatic ? ent_static : 0)
            | (m_collideWithRigid ? ent_rigid : 0)
            | (m_collideWithSleepingRigid ? ent_sleeping_rigid : 0)
            | (m_collideWithLiving ? ent_living : 0)
            | (m_collideWithIndependent ? ent_independent : 0)
            ;

        out.bActive = m_isActive;
        out.bReleaseGroundColliderWhenNotActive = m_releaseGroundColliderWhenNotActive;

        return out;
    }

    void CharacterPhysicsComponent::Reflect(AZ::ReflectContext* context)
    {
        CryPlayerPhysicsConfiguration::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<CharacterPhysicsComponent, AZ::Component>()
                ->Version(1)
                ->Field("Configuration", &CharacterPhysicsComponent::m_configuration)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<CharacterPhysicsComponent>(
                    "Character Physics", "The Character Physics component adds physical behavior to and configures simulation characteristics for character entities, such as players and enemies")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Physics (Legacy)")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/CharacterPhysics.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/CharacterPhysics.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-physics-character.html")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CharacterPhysicsComponent::m_configuration,
                    "Configuration", "Character Physics Configuration")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
                ;
            }
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->EBus<CryCharacterPhysicsRequestBus>("CryCharacterPhysicsRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Event("RequestVelocity", &CryCharacterPhysicsRequestBus::Events::RequestVelocity);
        }
    }

    void CharacterPhysicsComponent::Init()
    {
    }

    void CharacterPhysicsComponent::Activate()
    {
#ifdef AZ_ENABLE_TRACING
        bool isStaticTransform = false;
        AZ::TransformBus::EventResult(isStaticTransform, GetEntityId(), &AZ::TransformBus::Events::IsStaticTransform);
        AZ_Warning("Character Physics Component", !isStaticTransform,
            "Character Physics needs to move, but entity '%s' %s has a static transform.", GetEntity()->GetName().c_str(), GetEntityId().ToString().c_str());
#endif

        EnablePhysics();

        PhysicsComponentRequestBus::Handler::BusConnect(GetEntityId());
    }

    void CharacterPhysicsComponent::Deactivate()
    {
        PhysicsComponentRequestBus::Handler::BusDisconnect();

        DisablePhysics();
    }

    bool CharacterPhysicsComponent::IsPhysicsEnabled()
    {
        return m_physicalEntity != nullptr;
    }

    bool CharacterPhysicsComponent::ConfigurePhysicalEntity()
    {
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        EBUS_EVENT_ID_RESULT(transform, GetEntityId(), AZ::TransformBus, GetWorldTM);

        // set player-specific parameters
        pe_player_dimensions dimensions = m_configuration.m_dimensions.GetParams();
        if (!m_physicalEntity->SetParams(&dimensions))
        {
            AZ_Warning("CharacterPhysicsComponent", false, "Setting dimensions failed.  The character may not have enough room.");
            return false;
        }

        pe_player_dynamics dynamics = m_configuration.m_dynamics.GetParams();
        if (!m_physicalEntity->SetParams(&dynamics))
        {
            AZ_Error("CharacterPhysicsComponent", false, "Setting pe_player_dynamics only returns true, something has gone wrong");
            return false;
        }

        // Ensure that certain events bubble up from the physics system
        pe_params_flags flagParameters;
        flagParameters.flagsOR = pef_log_poststep // enable event when entity is moved by physics system
            | pef_log_collisions;                    // enable collision event
        if (!m_physicalEntity->SetParams(&flagParameters))
        {
            AZ_Error("CharacterPhysicsComponent", false, "Setting pe_params_flags only returns true, something has gone wrong");
            return false;
        }

        // Store the current transform so we know if it changes to the identity after configuration
        m_previousEntityTransform = transform;

        return true;

    }

    extern AZStd::shared_ptr<IPhysicalEntity> CreateIPhysicalEntitySharedPtr(IPhysicalEntity* rawPhysicalEntityPtr);
    
    void CharacterPhysicsComponent::EnablePhysics()
    {
        if (IsPhysicsEnabled())
        {
            return;
        }

        // Setup position params
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        EBUS_EVENT_ID_RESULT(transform, GetEntityId(), AZ::TransformBus, GetWorldTM);
        pe_params_pos positionParameters;
        Matrix34 cryTransform(AZTransformToLYTransform(transform));
        positionParameters.pMtx3x4 = &cryTransform;

        // Create physical entity
        IPhysicalEntity* rawPhysicalEntityPtr = gEnv->pPhysicalWorld->CreatePhysicalEntity(
            PE_LIVING, // type
            &positionParameters, // params
            static_cast<uint64>(GetEntityId()), // pForeignData
            PHYS_FOREIGN_ID_COMPONENT_ENTITY, // iForeignData
            -1, // id
            nullptr // IGeneralMemoryHeap
        );

        m_physicalEntity = CreateIPhysicalEntitySharedPtr(rawPhysicalEntityPtr);
        if (!m_physicalEntity)
        {
            AZ_Printf("SkinnedCharacterPhysicsComponent", "SkinnedCharacterPhysicsComponent::EnablePhysics no physical entity on entity id %s", GetEntityId().ToString().c_str());

            return;
        }

        pe_simulation_params simParams;
        m_physicalEntity->GetParams(&simParams);
        if (m_configuration.m_dynamics.m_recordCollisions)
        {
            simParams.maxLoggedCollisions = m_configuration.m_dynamics.m_maxRecordedCollisions;
        }
        m_physicalEntity->SetParams(&simParams);

        if (!m_physicalEntity)
        {
            return;
        }

        // Let behavior configure the physical entity.
        // If anything goes wrong, destroy m_physicalEntity
        if (!ConfigurePhysicalEntity())
        {
            m_physicalEntity.reset();
            return;
        }

        // Create Proximity trigger proxy
        m_proximityTriggerProxy = nullptr;
        EBUS_EVENT_RESULT(m_proximityTriggerProxy, ProximityTriggerSystemRequestBus, CreateEntity, GetEntityId());

        // Connect to buses concerning the live physics object
        CryPhysicsComponentRequestBus::Handler::BusConnect(GetEntityId());
        EntityPhysicsEventBus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        CryCharacterPhysicsRequestBus::Handler::BusConnect(GetEntityId());

        // Send notification
        EBUS_EVENT_ID(GetEntityId(), PhysicsComponentNotificationBus, OnPhysicsEnabled);
    }

    void CharacterPhysicsComponent::DisablePhysics()
    {
        if (!IsPhysicsEnabled())
        {
            return;
        }

        // Disconnect from buses concerning the live physics object
        CryCharacterPhysicsRequestBus::Handler::BusDisconnect();
        CryPhysicsComponentRequestBus::Handler::BusDisconnect();
        EntityPhysicsEventBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();

        // Send notification
        EBUS_EVENT_ID(GetEntityId(), PhysicsComponentNotificationBus, OnPhysicsDisabled);

        // Remove Proximity trigger proxy
        if (m_proximityTriggerProxy)
        {
            EBUS_EVENT(ProximityTriggerSystemRequestBus, RemoveEntity, m_proximityTriggerProxy, false);
            m_proximityTriggerProxy = nullptr;
        }


        m_physicalEntity.reset();
    }

    IPhysicalEntity* CharacterPhysicsComponent::GetPhysicalEntity()
    {
        return m_physicalEntity.get();
    }

    void CharacterPhysicsComponent::GetPhysicsParameters(pe_params& outParameters)
    {
        if (m_physicalEntity)
        {
            m_physicalEntity->GetParams(&outParameters);
        }
    }

    void CharacterPhysicsComponent::SetPhysicsParameters(const pe_params& parameters)
    {
        if (m_physicalEntity)
        {
            m_physicalEntity->SetParams(&parameters);
        }
    }

    void CharacterPhysicsComponent::GetPhysicsStatus(pe_status& outStatus)
    {
        if (m_physicalEntity)
        {
            m_physicalEntity->GetStatus(&outStatus);
        }
    }

    void CharacterPhysicsComponent::ApplyPhysicsAction(const pe_action& action, bool threadSafe)
    {
        if (m_physicalEntity)
        {
            m_physicalEntity->Action(&action, threadSafe ? 1 : 0);
        }
    }

    void CharacterPhysicsComponent::OnPostStep(const EntityPhysicsEvents::PostStep& event)
    {
        AZ::Transform transform;
        pe_status_pos pos;
        if (m_physicalEntity && m_physicalEntity->GetStatus(&pos) > 0)
        {
            // Physics post-events are stale by a frame, so favor current physical snapshot.
            // This won't stall because physics has not yet been queued for the next frame.
            transform = AZ::Transform::CreateFromQuaternionAndTranslation(LYQuaternionToAZQuaternion(pos.q), LYVec3ToAZVec3(pos.pos));
        }
        else
        {
            // If unable to retrieve directly from physics, use the event.
            transform = AZ::Transform::CreateFromQuaternionAndTranslation(event.m_entityRotation, event.m_entityPosition);;
        }
        
        // Inform the TransformComponent that we've been moved by the physics system
        if (GetEntity()->GetTransform())
        {
            // Maintain scale (this must be precise).
            AZ::Transform entityTransform = GetEntity()->GetTransform()->GetWorldTM();
            transform.MultiplyByScale(entityTransform.ExtractScaleExact());
        }

        AZ_Assert(m_isApplyingPhysicsToEntityTransform == false, "two post steps received before a transform event");
        m_isApplyingPhysicsToEntityTransform = true;
        EBUS_EVENT_ID(event.m_entity, AZ::TransformBus, SetWorldTM, transform);
        m_isApplyingPhysicsToEntityTransform = false;
    }

    void CharacterPhysicsComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        // Because physics can create a change in transform through OnPostStep, we need to make sure we
        // are not creating a cycle
        if (!m_isApplyingPhysicsToEntityTransform)
        {
            const bool positionChanged = !(m_previousEntityTransform.GetTranslation().IsClose(world.GetTranslation()));

            // LivingEntity only cares about positional changes.
            pe_params_pos positionParameters;
            AZ::Quaternion worldOrientation = AZ::Quaternion::CreateFromTransform(world);
            positionParameters.q = AZQuaternionToLYQuaternion(worldOrientation).GetNormalized();

            // Tells LivingEntity to maintain movement/velocity state. This is a hardcoded value in CryPhysics.
            const AZ::u32 kCryPhysicsMaintainVelocity = 32;
            positionParameters.bRecalcBounds = kCryPhysicsMaintainVelocity;

            // Set the position before calling SetPhysicsParameters
            if (positionChanged)
            {
                positionParameters.pos = AZVec3ToLYVec3(world.GetPosition());
            }

            EBUS_EVENT_ID(GetEntityId(), CryPhysicsComponentRequestBus, SetPhysicsParameters, positionParameters);
        }
        m_previousEntityTransform = world;

        // If the proximity trigger entity proxy exists then move it
        if (m_proximityTriggerProxy)
        {
            AZ::Aabb azAabb = GetAabb();
            AABB lyAabb = AZAabbToLyAABB(azAabb);
            ProximityTriggerSystemRequestBus::Broadcast(&ProximityTriggerSystemRequests::MoveEntity, m_proximityTriggerProxy, Vec3(0), lyAabb);
        }
    }

    void CharacterPhysicsComponent::OnParentChanged(AZ::EntityId oldParent, AZ::EntityId newParent)
    {
        AZ_Warning("PhysicsComponent", !(newParent.IsValid() && m_physicalEntity),
            "Entity '%s' is using physics while parented to another entity. This may lead to unexpected behavior.",
            m_entity->GetName().c_str());
    }

    void CharacterPhysicsComponent::RequestVelocity(const AZ::Vector3& velocity, int jump)
    {
        pe_action_move move;
        move.iJump = jump;
        move.dir = AZVec3ToLYVec3(velocity);
        EBUS_EVENT_ID(GetEntityId(), CryPhysicsComponentRequestBus, ApplyPhysicsAction, move, false);
    }
    
    void CharacterPhysicsComponent::AddImpulse(const AZ::Vector3& impulse)
    {
        if (IsPhysicsEnabled())
        {
            pe_action_impulse action;
            action.impulse = AZVec3ToLYVec3(impulse);
            m_physicalEntity->Action(&action);
        }
    }

    void CharacterPhysicsComponent::AddImpulseAtPoint(const AZ::Vector3& impulse, const AZ::Vector3& worldSpacePoint)
    {
        if (IsPhysicsEnabled())
        {
            pe_action_impulse action;
            action.impulse = AZVec3ToLYVec3(impulse);
            action.point = AZVec3ToLYVec3(worldSpacePoint);
            m_physicalEntity->Action(&action);
        }
    }

    AZ::Vector3 CharacterPhysicsComponent::GetVelocity()
    {
        AZ::Vector3 velocity = AZ::Vector3::CreateZero();

        if (IsPhysicsEnabled())
        {
            pe_status_living living;
            m_physicalEntity->GetStatus(&living);
            velocity = LYVec3ToAZVec3(living.vel);
        }

        return velocity;
    }

    void CharacterPhysicsComponent::SetVelocity(const AZ::Vector3& velocity)
    {
        if (IsPhysicsEnabled())
        {
            pe_action_set_velocity action;
            action.v = AZVec3ToLYVec3(velocity);
            m_physicalEntity->Action(&action);
        }
    }

    AZ::Vector3 CharacterPhysicsComponent::GetAcceleration()
    {
        AZ::Vector3 acceleration = AZ::Vector3::CreateZero();

        if (IsPhysicsEnabled())
        {
            pe_status_dynamics status;
            m_physicalEntity->GetStatus(&status);
            acceleration = LYVec3ToAZVec3(status.a);
        }

        return acceleration;
    }

    AZ::Vector3 CharacterPhysicsComponent::GetAngularVelocity()
    {
        AZ::Vector3 angularVelocity = AZ::Vector3::CreateZero();

        if (IsPhysicsEnabled())
        {
            pe_status_dynamics status;
            m_physicalEntity->GetStatus(&status);
            angularVelocity = LYVec3ToAZVec3(status.w);
        }

        return angularVelocity;
    }

    float CharacterPhysicsComponent::GetMass()
    {
        float mass = 0.0f;

        if (IsPhysicsEnabled())
        {
            pe_status_dynamics status;
            m_physicalEntity->GetStatus(&status);
            mass = status.mass;
        }

        return mass;
    }

    void CharacterPhysicsComponent::SetMass(float mass)
    {
        if (IsPhysicsEnabled())
        {
            pe_player_dynamics params;
            params.mass = mass;
            m_physicalEntity->SetParams(&params);
        }
    }

    AZ::Aabb CharacterPhysicsComponent::GetAabb()
    {
        if (IsPhysicsEnabled())
        {
            pe_params_bbox bboxParams;
            m_physicalEntity->GetParams(&bboxParams);
            AZ::Aabb result = AZ::Aabb::CreateFromMinMax(LYVec3ToAZVec3(bboxParams.BBox[0]), LYVec3ToAZVec3(bboxParams.BBox[1]));
            return result;
        }
        else
        {
            return AZ::Aabb::CreateNull();
        }
    }
} // namespace LmbrCentral
