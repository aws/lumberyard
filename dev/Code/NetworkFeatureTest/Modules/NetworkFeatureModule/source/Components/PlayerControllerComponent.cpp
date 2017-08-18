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
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Math/MathUtils.h>
#include <IGameObjectSystem.h>
#include <LmbrCentral/Physics/PhysicsComponentBus.h>
#include <MathConversion.h>

#include "Components/PlayerControllerComponent.h"

namespace NetworkFeatureTest
{
    PlayerControllerComponent::PlayerControllerComponent()
        : m_forwardScalar(0.0f)
        , m_horizontalScalar(0.0f)
        , m_translationSpeed(1000.0f)
        , m_rotationSpeed(10.0f)
        , m_isRotationScalar(false)
        , m_yawRotation(0.0f)
        , m_pitchRotation(0.0f)
        , m_allowJump(false)
        , m_shouldJump(false)
    {
    }

    PlayerControllerComponent::~PlayerControllerComponent()
    {
    }

    void PlayerControllerComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<PlayerControllerComponent, AZ::Component>()
                ->Version(1)
                ->Field("ForwardScalar", &PlayerControllerComponent::m_forwardScalar)
                ->Field("HorizonatlScalar", &PlayerControllerComponent::m_horizontalScalar)
                ->Field("TranslationSpeed", &PlayerControllerComponent::m_translationSpeed)
                ->Field("IsRotationScalar", &PlayerControllerComponent::m_isRotationScalar)
                ->Field("RotateYawScalar", &PlayerControllerComponent::m_yawRotation)
                ->Field("RotatePitchScalar", &PlayerControllerComponent::m_pitchRotation)
                ->Field("RotationSpeed", &PlayerControllerComponent::m_rotationSpeed)
                ->Field("AllowJump", &PlayerControllerComponent::m_allowJump)
                ->Field("ShouldJump", &PlayerControllerComponent::m_shouldJump)
            ;

            AZ::EditContext* edit = serialize->GetEditContext();

            if (edit)
            {
                edit->Class<PlayerControllerComponent>("Player Controller", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "NetworkFeatureTest")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerControllerComponent::m_translationSpeed, "Translation Speed", "How fast the player should move.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &PlayerControllerComponent::m_rotationSpeed, "Rotation Speed", "How fast the player should rotate.")
                ;
            }
        }
    }

    void PlayerControllerComponent::Init()
    {
    }

    void PlayerControllerComponent::Activate()
    {
        AZ::TickBus::Handler::BusConnect();

        IGameFramework* pGameFramework = gEnv->pGame ? gEnv->pGame->GetIGameFramework() : nullptr;
        IActionMapManager* actionMapManager = pGameFramework ? pGameFramework->GetIActionMapManager() : nullptr;

        if (actionMapManager)
        {
            actionMapManager->AddExtraActionListener(this, "player");
        }
    }

    void PlayerControllerComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();

        IGameFramework* pGameFramework = gEnv->pGame ? gEnv->pGame->GetIGameFramework() : nullptr;
        IActionMapManager* actionMapManager = pGameFramework ? pGameFramework->GetIActionMapManager() : nullptr;

        if (actionMapManager)
        {
            actionMapManager->RemoveExtraActionListener(this, "player");
        }
    }

    void PlayerControllerComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        pe_status_pos findRotation;
        EBUS_EVENT_ID(GetEntityId(), LmbrCentral::CryPhysicsComponentRequestBus, GetPhysicsStatus, findRotation);

        AZ::Quaternion rotationQuat = LYQuaternionToAZQuaternion(findRotation.q);
        AZ::Transform transform = AZ::Transform::CreateFromQuaternion(rotationQuat);

        if (!AZ::IsClose(m_yawRotation, 0.0f, 0.001f))
        {
            if (m_isRotationScalar)
            {
                float angleDiff = m_yawRotation * m_rotationSpeed * deltaTime;
                rotationQuat *= AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(0.0f, 0.0f, 1.0f), AZ::DegToRad(angleDiff));
            }
            else
            {
                rotationQuat *= AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(0.0f, 0.0f, 1.0f), AZ::DegToRad(m_yawRotation));
            }

            pe_params_pos setRotationParams;
            setRotationParams.q = AZQuaternionToLYQuaternion(rotationQuat);

            EBUS_EVENT_ID(GetEntityId(), LmbrCentral::CryPhysicsComponentRequestBus, SetPhysicsParameters, setRotationParams);
        }

        AZ::Vector3 forward = transform.GetBasisY();
        forward.Normalize();

        AZ::Vector3 right = transform.GetBasisX();
        right.Normalize();

        pe_action_impulse impulseAction;
        impulseAction.impulse = AZVec3ToLYVec3(forward * m_forwardScalar * m_translationSpeed * deltaTime + right * m_horizontalScalar * m_translationSpeed * deltaTime);

        impulseAction.impulse.z = 0.0f;
        EBUS_EVENT_ID(GetEntityId(), LmbrCentral::CryPhysicsComponentRequestBus, ApplyPhysicsAction, impulseAction, true);
    }

    void PlayerControllerComponent::OnAction(const ActionId& action, int activationMode, float value)
    {
        if (activationMode & eAAM_OnPress)
        {
            OnActionPressed(action, value);
        }
        else if (activationMode & eAAM_OnHold)
        {
            OnActionHeld(action, value);
        }
        else if (activationMode & eAAM_OnRelease)
        {
            OnActionReleased(action, value);
        }
        else if (activationMode & eAAM_Always)
        {
            OnActionAlways(action, value);
        }
    }

    void PlayerControllerComponent::OnActionPressed(const ActionId& action, float value)
    {
        if (strcmpi(action.c_str(), "jump") == 0)
        {
            if (m_allowJump)
            {
                m_shouldJump = true;
            }
        }
        else if (strcmpi(action.c_str(), "moveleft") == 0)
        {
            m_horizontalScalar = -1.0f;
        }
        else if (strcmpi(action.c_str(), "moveright") == 0)
        {
            m_horizontalScalar = 1.0f;
        }
        else if (strcmpi(action.c_str(), "moveforward") == 0)
        {
            m_forwardScalar = 1.0f;
        }
        else if (strcmpi(action.c_str(), "moveback") == 0)
        {
            m_forwardScalar = -1.0f;
        }
    }

    void PlayerControllerComponent::OnActionHeld(const ActionId& action, float value)
    {
    }

    void PlayerControllerComponent::OnActionReleased(const ActionId& action, float value)
    {
        if (strcmpi(action.c_str(), "moveleft") == 0)
        {
            m_horizontalScalar = 0.0f;
        }
        else if (strcmpi(action.c_str(), "moveright") == 0)
        {
            m_horizontalScalar = 0.0f;
        }
        else if (strcmpi(action.c_str(), "moveforward") == 0)
        {
            m_forwardScalar = 0.0f;
        }
        else if (strcmpi(action.c_str(), "moveback") == 0)
        {
            m_forwardScalar = 0.0f;
        }
    }

    void PlayerControllerComponent::OnActionAlways(const ActionId& action, float value)
    {
        if (strcmpi(action.c_str(), "xi_movey") == 0)
        {
            m_forwardScalar = value;
        }
        else if (strcmpi(action.c_str(), "xi_movex") == 0)
        {
            m_horizontalScalar = value;
        }
        else if (strcmpi(action.c_str(), "xi_rotateyaw") == 0)
        {
            m_isRotationScalar = true;
            m_yawRotation = -value;
        }
        else if (strcmpi(action.c_str(), "xi_rotatepitch") == 0)
        {
            m_isRotationScalar = true;
            m_pitchRotation = value;
        }
        else if (strcmpi(action.c_str(), "rotateyaw") == 0)
        {
            m_isRotationScalar = false;
            m_yawRotation = -value * 0.1f;
        }
        else if (strcmpi(action.c_str(), "rotatepitch") == 0)
        {
            m_isRotationScalar = false;
            m_pitchRotation = value * 0.1f;
        }
    }
}