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
#include "FlyCameraInputComponent.h"

#include <AzCore/Component/Component.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <MathConversion.h>

#include <LyShine/IDraw2d.h>

using namespace LYGame;

//////////////////////////////////////////////////////////////////////////////
namespace
{
    //////////////////////////////////////////////////////////////////////////
    int GenerateThumbstickTexture()
    {
        if (!gEnv || !gEnv->pRenderer)
        {
            return 0;
        }

        const uint32 clear = 0x00000000;
        const uint32 white = 0xffffffff;

        const int textureSize = 128;
        const int textureSizeOn2 = textureSize / 2;
        const int circleRadiusSquared = textureSizeOn2 * textureSizeOn2;

        uint32 pixelData[textureSize * textureSize];

        for (int y = 0; y < textureSize; ++y)
        {
            const int yOffsetIntoFlatIndex = y * textureSize;
            const int yDistanceFromCenter = textureSizeOn2 - y;
            const int yDistanceFromCenterSquared = yDistanceFromCenter * yDistanceFromCenter;

            for (int x = 0; x < textureSize; ++x)
            {
                const int xDistanceFromCenter = textureSizeOn2 - x;
                const int xDistanceFromCenterSquared = xDistanceFromCenter * xDistanceFromCenter;
                const int distanceFromCenterSquared = xDistanceFromCenterSquared + yDistanceFromCenterSquared;

                const int flatIndex = yOffsetIntoFlatIndex + x;
                pixelData[flatIndex] = ((distanceFromCenterSquared > circleRadiusSquared) ? clear : white);
            }
        }

        return gEnv->pRenderer->DownLoadToVideoMemory((uint8*)pixelData, textureSize, textureSize, eTF_R8G8B8A8, eTF_R8G8B8A8, 1);
    }

    //////////////////////////////////////////////////////////////////////////
    void ReleaseThumbstickTexture(int textureId)
    {
        if (textureId && gEnv && gEnv->pRenderer)
        {
            gEnv->pRenderer->RemoveTexture(textureId);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void DrawThumbstick(Vec2 initialPosition,
        Vec2 currentPosition,
        int textureId)
    {
        IDraw2d* draw2d = Draw2dHelper::GetDraw2d();
        if (!draw2d)
        {
            return;
        }

        draw2d->BeginDraw2d(true);
        {
            const float percentageOfScreenHeightToUseAsThumbstickRadius = 0.05f;
            const float largeCircleRadius = (float)gEnv->pRenderer->GetHeight() * percentageOfScreenHeightToUseAsThumbstickRadius;
            const float largeCircleDiameter = largeCircleRadius * 2.0f;
            const AZ::Vector2 largeCirclePosition(initialPosition.x - largeCircleRadius, initialPosition.y - largeCircleRadius);
            const AZ::Vector2 largeCircleSize(largeCircleDiameter, largeCircleDiameter);
            const float alpha = 0.5f;
            draw2d->DrawImage(textureId, largeCirclePosition, largeCircleSize, alpha);

            Vec2 delta = currentPosition - initialPosition;
            delta.Normalize();
            delta *= largeCircleRadius;

            const float smallCircleRadius = largeCircleRadius * 0.5f;
            const float smallCircleDiameter = smallCircleRadius * 2.0f;
            const AZ::Vector2 smallCirclePosition(initialPosition.x + delta.x - smallCircleRadius,
                initialPosition.y + delta.y - smallCircleRadius);
            const AZ::Vector2 smallCircleSize(smallCircleDiameter, smallCircleDiameter);
            draw2d->DrawImage(textureId, smallCirclePosition, smallCircleSize, 0.5f);
        }
        draw2d->EndDraw2d();
    }
}

//////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
{
    required.push_back(AZ_CRC("TransformService"));
}

//////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
{
    provided.push_back(AZ_CRC("InputService"));
}

//////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::Reflect(AZ::ReflectContext* reflection)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
    if (serializeContext)
    {
        serializeContext->Class<FlyCameraInputComponent, AZ::Component>()
            ->Version(1)
            ->Field("Move Speed", &FlyCameraInputComponent::m_moveSpeed)
            ->Field("Rotation Speed", &FlyCameraInputComponent::m_rotationSpeed)
            ->Field("Mouse Sensitivity", &FlyCameraInputComponent::m_mouseSensitivity)
            ->Field("Invert Rotation Input X", &FlyCameraInputComponent::m_InvertRotationInputAxisX)
            ->Field("Invert Rotation Input Y", &FlyCameraInputComponent::m_InvertRotationInputAxisY)
            ->Field("Is enabled", &FlyCameraInputComponent::m_isEnabled);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext)
        {
            editContext->Class<FlyCameraInputComponent>("Fly Camera Input", "The Fly Camera Input allows you to control the camera")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute("Category", "Game")
                ->Attribute("Icon", "Editor/Icons/Components/CameraRig")
                ->Attribute("ViewportIcon", "Editor/Icons/Components/Viewport/CameraRig.png")
                ->Attribute("AutoExpand", true)
                ->Attribute("AppearsInAddComponentMenu", true)
                ->DataElement(0, &FlyCameraInputComponent::m_moveSpeed, "Move Speed", "Speed at which the camera moves")
                ->Attribute("Min", 1.0f)
                ->Attribute("Max", 100.0f)
                ->Attribute("ChangeNotify", AZ_CRC("RefreshValues"))
                ->DataElement(0, &FlyCameraInputComponent::m_rotationSpeed, "Rotation Speed", "Speed at which the camera rotates")
                ->Attribute("Min", 1.0f)
                ->Attribute("Max", 100.0f)
                ->Attribute("ChangeNotify", AZ_CRC("RefreshValues"))
                ->DataElement(0, &FlyCameraInputComponent::m_mouseSensitivity, "Mouse Sensitivity", "Mouse sensitivity factor")
                ->Attribute("Min", 0.0f)
                ->Attribute("Max", 1.0f)
                ->Attribute("ChangeNotify", AZ_CRC("RefreshValues"))
                ->DataElement(0, &FlyCameraInputComponent::m_InvertRotationInputAxisX, "Invert Rotation Input X", "Invert rotation input x-axis")
                ->Attribute("ChangeNotify", AZ_CRC("RefreshValues"))
                ->DataElement(0, &FlyCameraInputComponent::m_InvertRotationInputAxisY, "Invert Rotation Input Y", "Invert rotation input y-axis")
                ->Attribute("ChangeNotify", AZ_CRC("RefreshValues"))
                ->DataElement(AZ::Edit::UIHandlers::CheckBox, &FlyCameraInputComponent::m_isEnabled,
                    "Is Initially Enabled", "When checked, the fly cam input is enabled on activate, else it has to be specifically enabled.");
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);
    if (behaviorContext)
    {
        behaviorContext->EBus<FlyCameraInputBus>("FlyCameraInputBus")
            ->Event("SetIsEnabled", &FlyCameraInputBus::Events::SetIsEnabled)
            ->Event("GetIsEnabled", &FlyCameraInputBus::Events::GetIsEnabled);
    }
}

//////////////////////////////////////////////////////////////////////////////
FlyCameraInputComponent::~FlyCameraInputComponent()
{
    ReleaseThumbstickTexture(m_thumbstickTextureId);
}

//////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::Init()
{
    m_thumbstickTextureId = GenerateThumbstickTexture();
}

//////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::Activate()
{
    if (gEnv && gEnv->pInput)
    {
        gEnv->pInput->AddEventListener(this);
    }

    AZ::TickBus::Handler::BusConnect();
    LYGame::FlyCameraInputBus::Handler::BusConnect(GetEntityId());
}

//////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::Deactivate()
{
    LYGame::FlyCameraInputBus::Handler::BusDisconnect();
    AZ::TickBus::Handler::BusDisconnect();

    if (gEnv && gEnv->pInput)
    {
        gEnv->pInput->RemoveEventListener(this);
    }
}

//////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::OnTick(float deltaTime, AZ::ScriptTimePoint /*time*/)
{
    if (!m_isEnabled)
    {
        return;
    }

    AZ::Transform worldTransform = AZ::Transform::Identity();
    EBUS_EVENT_ID_RESULT(worldTransform, GetEntityId(), AZ::TransformBus, GetWorldTM);

    // Update movement
    const float moveSpeed = m_moveSpeed * deltaTime;
    const AZ::Vector3 right = worldTransform.GetColumn(0);
    const AZ::Vector3 forward = worldTransform.GetColumn(1);
    const AZ::Vector3 movement = (forward * m_movement.y) + (right * m_movement.x);
    const AZ::Vector3 newPosition = worldTransform.GetPosition() + (movement * moveSpeed);
    worldTransform.SetPosition(newPosition);

    const Vec2 invertedRotation(m_InvertRotationInputAxisX ? m_rotation.x : -m_rotation.x,
        m_InvertRotationInputAxisY ? m_rotation.y : -m_rotation.y);

    // Update rotation (not sure how to do this properly using just AZ::Quaternion)
    const AZ::Quaternion worldOrientation = AZ::Quaternion::CreateFromTransform(worldTransform);
    const Ang3 rotation(AZQuaternionToLYQuaternion(worldOrientation));
    const Ang3 newRotation = rotation + Ang3(DEG2RAD(invertedRotation.y), 0.f, DEG2RAD(invertedRotation.x)) * m_rotationSpeed;
    const AZ::Quaternion newOrientation = LYQuaternionToAZQuaternion(Quat(newRotation));
    worldTransform.SetRotationPartFromQuaternion(newOrientation);

    EBUS_EVENT_ID(GetEntityId(), AZ::TransformBus, SetWorldTM, worldTransform);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool FlyCameraInputComponent::OnInputEvent(const SInputEvent& inputEvent)
{
    if (!m_isEnabled)
    {
        return false;
    }

    switch (inputEvent.deviceType)
    {
    case eIDT_Mouse:
    {
        OnMouseEvent(inputEvent);
    }
    break;
    case eIDT_Keyboard:
    {
        OnKeyboardEvent(inputEvent);
    }
    break;
    case eIDT_TouchScreen:
    {
        OnTouchEvent(inputEvent);
    }
    break;
    case eIDT_Gamepad:
    {
        OnGamepadEvent(inputEvent);
    }
    break;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::SetIsEnabled(bool isEnabled)
{
    m_isEnabled = isEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool FlyCameraInputComponent::GetIsEnabled()
{
    return m_isEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::OnMouseEvent(const SInputEvent& inputEvent)
{
    switch (inputEvent.keyId)
    {
    case eKI_MouseX:
    {
        m_rotation.x = Snap_s360(inputEvent.value * m_mouseSensitivity);
    }
    break;
    case eKI_MouseY:
    {
        m_rotation.y = Snap_s360(inputEvent.value * m_mouseSensitivity);
    }
    break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::OnKeyboardEvent(const SInputEvent& inputEvent)
{
    if (gEnv && gEnv->pConsole && gEnv->pConsole->IsOpened())
    {
        return;
    }

    const float value = (inputEvent.state == eIS_Pressed || inputEvent.state == eIS_Down) ? 1.0f : 0.0f;
    switch (inputEvent.keyId)
    {
    case eKI_W:
    {
        m_movement.y = value;
    }
    break;
    case eKI_A:
    {
        m_movement.x = -value;
    }
    break;
    case eKI_S:
    {
        m_movement.y = -value;
    }
    break;
    case eKI_D:
    {
        m_movement.x = value;
    }
    break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::OnGamepadEvent(const SInputEvent& inputEvent)
{
    switch (inputEvent.keyId)
    {
    case eKI_XI_ThumbLX:
    case eKI_Orbis_StickLX:
    {
        m_movement.x = inputEvent.value;
    }
    break;
    case eKI_XI_ThumbLY:
    case eKI_Orbis_StickLY:
    {
        m_movement.y = inputEvent.value;
    }
    break;
    case eKI_XI_ThumbRX:
    case eKI_Orbis_StickRX:
    {
        m_rotation.x = inputEvent.value;
    }
    break;
    case eKI_XI_ThumbRY:
    case eKI_Orbis_StickRY:
    {
        m_rotation.y = inputEvent.value;
    }
    break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::OnTouchEvent(const SInputEvent& inputEvent)
{
    if (inputEvent.state == eIS_Pressed)
    {
        const float screenCentreX = static_cast<float>(gEnv->pRenderer->GetWidth()) * 0.5f;
        if (inputEvent.screenPosition.x <= screenCentreX)
        {
            if (m_leftFingerId == eKI_Unknown)
            {
                // Initiate left thumb-stick (movement)
                m_leftDownPosition = inputEvent.screenPosition;
                m_leftFingerId = inputEvent.keyId;
                DrawThumbstick(m_leftDownPosition, inputEvent.screenPosition, m_thumbstickTextureId);
            }
        }
        else
        {
            if (m_rightFingerId == eKI_Unknown)
            {
                // Initiate right thumb-stick (rotation)
                m_rightDownPosition = inputEvent.screenPosition;
                m_rightFingerId = inputEvent.keyId;
                DrawThumbstick(m_rightDownPosition, inputEvent.screenPosition, m_thumbstickTextureId);
            }
        }
    }
    else if (inputEvent.keyId == m_leftFingerId)
    {
        // Update left thumb-stick (movement)
        OnVirtualLeftThumbstickEvent(inputEvent);
    }
    else if (inputEvent.keyId == m_rightFingerId)
    {
        // Update right thumb-stick (rotation)
        OnVirtualRightThumbstickEvent(inputEvent);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::OnVirtualLeftThumbstickEvent(const SInputEvent& inputEvent)
{
    if (inputEvent.keyId != m_leftFingerId)
    {
        return;
    }

    switch (inputEvent.state)
    {
    case eIS_Released:
    {
        // Stop movement
        m_leftFingerId = eKI_Unknown;
        m_movement = ZERO;
    }
    break;

    case eIS_Down:
    {
        DrawThumbstick(m_leftDownPosition, inputEvent.screenPosition, m_thumbstickTextureId);
    }
    break;

    case eIS_Changed:
    {
        // Calculate movement
        const float discRadius = (float)gEnv->pRenderer->GetWidth() * m_virtualThumbstickRadiusAsPercentageOfScreenWidth;
        const float distScalar = 1.0f / discRadius;

        Vec2 dist = inputEvent.screenPosition - m_leftDownPosition;
        dist *= distScalar;

        m_movement.x = CLAMP(dist.x, -1.0f, 1.0f);
        m_movement.y = CLAMP(-dist.y, -1.0f, 1.0f);
    }
    break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::OnVirtualRightThumbstickEvent(const SInputEvent& inputEvent)
{
    if (inputEvent.keyId != m_rightFingerId)
    {
        return;
    }

    switch (inputEvent.state)
    {
    case eIS_Released:
    {
        // Stop rotation
        m_rightFingerId = eKI_Unknown;
        m_rotation = ZERO;
    }
    break;

    case eIS_Down:
    {
        DrawThumbstick(m_rightDownPosition, inputEvent.screenPosition, m_thumbstickTextureId);
    }
    break;

    case eIS_Changed:
    {
        // Calculate rotation
        const float discRadius = (float)gEnv->pRenderer->GetWidth() * m_virtualThumbstickRadiusAsPercentageOfScreenWidth;
        const float distScalar = 1.0f / discRadius;

        Vec2 dist = inputEvent.screenPosition - m_rightDownPosition;
        dist *= distScalar;

        m_rotation.x = CLAMP(dist.x, -1.0f, 1.0f);
        m_rotation.y = CLAMP(dist.y, -1.0f, 1.0f);
    }
    break;
    }
}
