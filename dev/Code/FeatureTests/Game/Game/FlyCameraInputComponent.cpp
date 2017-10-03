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

#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>

#include <MathConversion.h>

#include <LyShine/IDraw2d.h>

using namespace AzFramework;
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
const AZ::Crc32 FlyCameraInputComponent::UnknownInputChannelId("unknown_input_channel_id");

//////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
{
    required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
}

//////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
{
    provided.push_back(AZ_CRC("InputService", 0xd41af40c));
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
            editContext->Class<FlyCameraInputComponent>("Fly Camera Input", "Controls the camera")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute("Category", "Game")
                ->Attribute("Icon", "Editor/Icons/Components/CameraRig")
                ->Attribute("ViewportIcon", "Editor/Icons/Components/Viewport/CameraRig.png")
                ->Attribute("AutoExpand", true)
                ->Attribute("AppearsInAddComponentMenu", true)
                ->DataElement(0, &FlyCameraInputComponent::m_moveSpeed, "Move Speed", "Speed at which the camera moves")
                ->Attribute("Min", 1.0f)
                ->Attribute("Max", 100.0f)
                ->Attribute("ChangeNotify", AZ_CRC("RefreshValues", 0x28e720d4))
                ->DataElement(0, &FlyCameraInputComponent::m_rotationSpeed, "Rotation Speed", "Speed at which the camera rotates")
                ->Attribute("Min", 1.0f)
                ->Attribute("Max", 100.0f)
                ->Attribute("ChangeNotify", AZ_CRC("RefreshValues", 0x28e720d4))
                ->DataElement(0, &FlyCameraInputComponent::m_mouseSensitivity, "Mouse Sensitivity", "Mouse sensitivity factor")
                ->Attribute("Min", 0.0f)
                ->Attribute("Max", 1.0f)
                ->Attribute("ChangeNotify", AZ_CRC("RefreshValues", 0x28e720d4))
                ->DataElement(0, &FlyCameraInputComponent::m_InvertRotationInputAxisX, "Invert Rotation Input X", "Invert rotation input x-axis")
                ->Attribute("ChangeNotify", AZ_CRC("RefreshValues", 0x28e720d4))
                ->DataElement(0, &FlyCameraInputComponent::m_InvertRotationInputAxisY, "Invert Rotation Input Y", "Invert rotation input y-axis")
                ->Attribute("ChangeNotify", AZ_CRC("RefreshValues", 0x28e720d4))
                ->DataElement(AZ::Edit::UIHandlers::CheckBox, &FlyCameraInputComponent::m_isEnabled,
                    "Is Initially Enabled", "When checked, the fly cam input is enabled on activate, else it has to be specifically enabled.");
        }
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
    InputChannelEventListener::Connect();
    AZ::TickBus::Handler::BusConnect();
    LYGame::FlyCameraInputBus::Handler::BusConnect(GetEntityId());
}

//////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::Deactivate()
{
    LYGame::FlyCameraInputBus::Handler::BusDisconnect();
    AZ::TickBus::Handler::BusDisconnect();
    InputChannelEventListener::Disconnect();
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
bool FlyCameraInputComponent::OnInputChannelEventFiltered(const InputChannel& inputChannel)
{
    if (!m_isEnabled)
    {
        return false;
    }

    const InputDeviceId& deviceId = inputChannel.GetInputDevice().GetInputDeviceId();
    if (deviceId == InputDeviceMouse::Id)
    {
        OnMouseEvent(inputChannel);
    }
    else if (deviceId == InputDeviceKeyboard::Id)
    {
        OnKeyboardEvent(inputChannel);
    }
    else if (deviceId == InputDeviceTouch::Id)
    {
        const InputChannel::PositionData2D* positionData2D = inputChannel.GetCustomData<InputChannel::PositionData2D>();
        if (positionData2D)
        {
            const Vec2 screenPosition(positionData2D->m_normalizedPosition.GetX() * gEnv->pRenderer->GetWidth(),
                                      positionData2D->m_normalizedPosition.GetY() * gEnv->pRenderer->GetHeight());
            OnTouchEvent(inputChannel, screenPosition);
        }
    }
    else if (AzFramework::InputDeviceGamepad::IsGamepad(deviceId)) // Any gamepad
    {
        OnGamepadEvent(inputChannel);
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
void FlyCameraInputComponent::OnMouseEvent(const InputChannel& inputChannel)
{
    const InputChannelId& channelId = inputChannel.GetInputChannelId();
    if (channelId == InputDeviceMouse::Movement::X)
    {
        m_rotation.x = Snap_s360(inputChannel.GetValue() * m_mouseSensitivity);
    }
    else if (channelId == InputDeviceMouse::Movement::Y)
    {
        m_rotation.y = Snap_s360(inputChannel.GetValue() * m_mouseSensitivity);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::OnKeyboardEvent(const InputChannel& inputChannel)
{
    if (gEnv && gEnv->pConsole && gEnv->pConsole->IsOpened())
    {
        return;
    }

    const InputChannelId& channelId = inputChannel.GetInputChannelId();
    if (channelId == InputDeviceKeyboard::Key::AlphanumericW)
    {
        m_movement.y = inputChannel.GetValue();
    }
    else if (channelId == InputDeviceKeyboard::Key::AlphanumericA)
    {
        m_movement.x = -inputChannel.GetValue();
    }
    else if (channelId == InputDeviceKeyboard::Key::AlphanumericS)
    {
        m_movement.y = -inputChannel.GetValue();
    }
    else if (channelId == InputDeviceKeyboard::Key::AlphanumericD)
    {
        m_movement.x = inputChannel.GetValue();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::OnGamepadEvent(const InputChannel& inputChannel)
{
    const InputChannelId& channelId = inputChannel.GetInputChannelId();
    if (channelId == InputDeviceGamepad::ThumbStickAxis1D::LX)
    {
        m_movement.x = inputChannel.GetValue();
    }
    else if (channelId == InputDeviceGamepad::ThumbStickAxis1D::LY)
    {
        m_movement.y = inputChannel.GetValue();
    }
    else if (channelId == InputDeviceGamepad::ThumbStickAxis1D::RX)
    {
        m_rotation.x = inputChannel.GetValue();
    }
    else if (channelId == InputDeviceGamepad::ThumbStickAxis1D::RY)
    {
        m_rotation.y = inputChannel.GetValue();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::OnTouchEvent(const InputChannel& inputChannel, const Vec2& screenPosition)
{
    if (inputChannel.IsStateBegan())
    {
        const float screenCentreX = static_cast<float>(gEnv->pRenderer->GetWidth()) * 0.5f;
        if (screenPosition.x <= screenCentreX)
        {
            if (m_leftFingerId == UnknownInputChannelId)
            {
                // Initiate left thumb-stick (movement)
                m_leftDownPosition = screenPosition;
                m_leftFingerId = inputChannel.GetInputChannelId().GetNameCrc32();
                DrawThumbstick(m_leftDownPosition, screenPosition, m_thumbstickTextureId);
            }
        }
        else
        {
            if (m_rightFingerId == UnknownInputChannelId)
            {
                // Initiate right thumb-stick (rotation)
                m_rightDownPosition = screenPosition;
                m_rightFingerId = inputChannel.GetInputChannelId().GetNameCrc32();
                DrawThumbstick(m_rightDownPosition, screenPosition, m_thumbstickTextureId);
            }
        }
    }
    else if (inputChannel.GetInputChannelId().GetNameCrc32() == m_leftFingerId)
    {
        // Update left thumb-stick (movement)
        OnVirtualLeftThumbstickEvent(inputChannel, screenPosition);
    }
    else if (inputChannel.GetInputChannelId().GetNameCrc32() == m_rightFingerId)
    {
        // Update right thumb-stick (rotation)
        OnVirtualRightThumbstickEvent(inputChannel, screenPosition);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::OnVirtualLeftThumbstickEvent(const InputChannel& inputChannel, const Vec2& screenPosition)
{
    if (inputChannel.GetInputChannelId().GetNameCrc32() != m_leftFingerId)
    {
        return;
    }

    switch (inputChannel.GetState())
    {
        case InputChannel::State::Ended:
        {
            // Stop movement
            m_leftFingerId = UnknownInputChannelId;
            m_movement = ZERO;
        }
        break;

        case InputChannel::State::Updated:
        {
            // Calculate movement
            const float discRadius = (float)gEnv->pRenderer->GetWidth() * m_virtualThumbstickRadiusAsPercentageOfScreenWidth;
            const float distScalar = 1.0f / discRadius;

            Vec2 dist = screenPosition - m_leftDownPosition;
            dist *= distScalar;

            m_movement.x = CLAMP(dist.x, -1.0f, 1.0f);
            m_movement.y = CLAMP(-dist.y, -1.0f, 1.0f);

            DrawThumbstick(m_leftDownPosition, screenPosition, m_thumbstickTextureId);
        }
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::OnVirtualRightThumbstickEvent(const InputChannel& inputChannel, const Vec2& screenPosition)
{
    if (inputChannel.GetInputChannelId().GetNameCrc32() != m_rightFingerId)
    {
        return;
    }

    switch (inputChannel.GetState())
    {
        case InputChannel::State::Ended:
        {
            // Stop rotation
            m_rightFingerId = UnknownInputChannelId;
            m_rotation = ZERO;
        }
        break;

        case InputChannel::State::Updated:
        {
            // Calculate rotation
            const float discRadius = (float)gEnv->pRenderer->GetWidth() * m_virtualThumbstickRadiusAsPercentageOfScreenWidth;
            const float distScalar = 1.0f / discRadius;

            Vec2 dist = screenPosition - m_rightDownPosition;
            dist *= distScalar;

            m_rotation.x = CLAMP(dist.x, -1.0f, 1.0f);
            m_rotation.y = CLAMP(dist.y, -1.0f, 1.0f);

            DrawThumbstick(m_rightDownPosition, screenPosition, m_thumbstickTextureId);
        }
        break;
    }
}
