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

#include "HMDFramework_precompiled.h"
#include "HMDDebuggerComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

// For debug rendering
#include "../CryAction/IViewSystem.h"
#include <IGame.h>
#include <IGameFramework.h>
#include <IRenderAuxGeom.h>
#include <MathConversion.h>

// for input
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>

// debug constants
static const float kDebugCameraMoveSpeed = 5.0f;
static const float kDebugCameraRotateScale = -0.01f; // negative value to reverse direction of pitch and yaw

static const AzFramework::InputChannelId kDebugCameraInputMap[HMDDebuggerComponent::EHMDDebugCameraKeys::Count] = // maps to EHMDDebugCameraKeys
{
    AzFramework::InputDeviceKeyboard::Key::AlphanumericW, // Forward
    AzFramework::InputDeviceKeyboard::Key::AlphanumericS, // Back
    AzFramework::InputDeviceKeyboard::Key::AlphanumericA, // Left
    AzFramework::InputDeviceKeyboard::Key::AlphanumericD  // Right
};

void HMDDebuggerComponent::Reflect(AZ::ReflectContext* context)
{
    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->Class<HMDDebuggerComponent, AZ::Component>()
            ->Version(1)
            ->SerializerForEmptyClass()
            ;

        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
        {
            editContext->Class<HMDDebuggerComponent>(
                "HMD Debugger", "")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "VR")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                ;
        }
    }
}

void HMDDebuggerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
{
    provided.push_back(AZ_CRC("HMDDebugger"));
}

void HMDDebuggerComponent::Init()
{
    for (int i = 0; i < EHMDDebugCameraKeys::Count; ++i)
    {
        m_debugCameraInputState[i] = false;
    }
}

void HMDDebuggerComponent::Activate()
{
    AZ::VR::HMDDebuggerRequestBus::Handler::BusConnect();
}

void HMDDebuggerComponent::Deactivate()
{
    AZ::VR::HMDDebuggerRequestBus::Handler::BusDisconnect();
}

void HMDDebuggerComponent::EnableInfo(bool enable)
{
    m_debugFlags.set(EHMDDebugFlags::Info, enable);
    EBUS_EVENT(AZ::VR::HMDDeviceRequestBus, EnableDebugging, enable);
    OnDebugFlagsChanged();
}

void HMDDebuggerComponent::EnableCamera(bool enable)
{
    m_debugFlags.set(EHMDDebugFlags::Camera, enable);

    if (enable)
    {
        InputChannelEventListener::Connect();
    }
    else
    {
        InputChannelEventListener::Disconnect();
    }

    OnDebugFlagsChanged();
}

void HMDDebuggerComponent::OnDebugFlagsChanged()
{
    if (m_debugFlags.any())
    {
        AZ::TickBus::Handler::BusConnect();
    }
    else
    {
        AZ::TickBus::Handler::BusDisconnect();
    }
}

void DrawHMDController(AZ::VR::ControllerIndex id, const SViewParams* viewParameters, IRenderAuxGeom* auxGeom)
{
    // The HMD and controllers are tracked in their own local spaces which are converted into a world-space offset
    // internally in the device. This offset is relative to the re-centered pose of the HMD. To actually render something
    // like a controller is world space, we need to get the current entity that is attached to the view system and use its
    // transformations (viewParameters).

    bool connected = false;
    EBUS_EVENT_RESULT(connected, AZ::VR::ControllerRequestBus, IsConnected, id);

    if (connected)
    {
        // Draw a cross with a line pointing backwards along the controller. These positions are in local space.
        float size = 0.05f;
        Vec3 leftPoint(-size, 0.0f, 0.0f);
        Vec3 rightPoint(size, 0.0f, 0.0f);
        Vec3 upPoint(0.0f, 0.0f, size);
        Vec3 downPoint(0.0f, 0.0f, -size);
        Vec3 center(0.0f, 0.0f, 0.0f);
        Vec3 back(0.0f, -size, 0.0f);

        AZ::VR::TrackingState* state = nullptr;
        EBUS_EVENT_RESULT(state, AZ::VR::ControllerRequestBus, GetTrackingState, id);
        Quat orientation = viewParameters->rotation * AZQuaternionToLYQuaternion(state->pose.orientation);

        Matrix34 transform;
        transform.SetRotation33(Matrix33(orientation));
        transform.SetTranslation(viewParameters->position + (viewParameters->rotation * AZVec3ToLYVec3(state->pose.position)));

        leftPoint  = transform * leftPoint;
        rightPoint = transform * rightPoint;
        upPoint    = transform * upPoint;
        downPoint  = transform * downPoint;
        center     = transform * center;
        back       = transform * back;

        ColorB white(255, 255, 255);
        auxGeom->DrawLine(leftPoint, white, rightPoint, white);
        auxGeom->DrawLine(upPoint, white, downPoint, white);
        auxGeom->DrawLine(center, white, back, white);
    }
}

void HMDDebuggerComponent::UpdateDebugInfo(float deltaTime)
{
    if (IViewSystem* viewSystem = gEnv->pGame->GetIGameFramework()->GetIViewSystem())
    {
        if (IView* mainView = viewSystem->GetActiveView())
        {
            const SViewParams* viewParams = mainView->GetCurrentParams();
            IRenderAuxGeom* auxGeom = gEnv->pRenderer->GetIRenderAuxGeom();

            DrawHMDController(AZ::VR::ControllerIndex::LeftHand, viewParams, auxGeom);
            DrawHMDController(AZ::VR::ControllerIndex::RightHand, viewParams, auxGeom);

            // Let the device do any device-specific debugging output that it wants.
            AZ::Quaternion rotation = LYQuaternionToAZQuaternion(viewParams->rotation);
            AZ::Vector3 translation = LYVec3ToAZVec3(viewParams->position);
            AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(rotation, translation);
            EBUS_EVENT(AZ::VR::HMDDeviceRequestBus, DrawDebugInfo, transform, auxGeom);
        }
    }
}

// Updates the position of the Entity currently attached to the camera. The view sets its transform relative to the camera entity.
// Translates the camera entity position forward, back, left or right relative to the camera transform.
// The camera transform can be rotated by current mouse delta, if not rendering to the HMD.
// If rendering to the HMD the camera transform is determined by the HMD orientation.
void HMDDebuggerComponent::UpdateDebugCamera(float deltaTime)
{
    if (IViewSystem* viewSystem = gEnv->pGame->GetIGameFramework()->GetIViewSystem())
    {
        if (IView* mainView = viewSystem->GetActiveView())
        {
            const float speedDelta = kDebugCameraMoveSpeed*deltaTime;
            const AZ::EntityId entityId = mainView->GetLinkedId();

            AZ::Transform cameraTM;
            EBUS_EVENT_ID_RESULT(cameraTM, entityId, AZ::TransformBus, GetWorldTM);

            const AZ::Vector3 cameraPosition = cameraTM.GetPosition();
            cameraTM.SetPosition(AZ::Vector3::CreateZero()); // zero out position before rotation calculations

            // create the move vector from our right, left, forward, back key states
            AZ::Vector3 moveVec = AZ::Vector3::CreateZero();
            moveVec.SetX((m_debugCameraInputState[EHMDDebugCameraKeys::Right] ? speedDelta : 0) + (m_debugCameraInputState[EHMDDebugCameraKeys::Left] ? -speedDelta : 0));
            moveVec.SetY((m_debugCameraInputState[EHMDDebugCameraKeys::Forward] ? speedDelta : 0) + (m_debugCameraInputState[EHMDDebugCameraKeys::Back] ? -speedDelta : 0));

            const AZ::VR::HMDDeviceInfo* deviceInfo = nullptr;
            EBUS_EVENT_RESULT(deviceInfo, AZ::VR::HMDDeviceRequestBus, GetDeviceInfo);
            const bool nullVRDevice = deviceInfo ? stricmp(deviceInfo->manufacturer, "Null") == 0 : true; // if no manufacturer, or no device info, then nullvr device

            if (gEnv->pRenderer->GetIStereoRenderer()->IsRenderingToHMD() && nullVRDevice == false)
            {
                // move relative to HMD if rendering to HMD
                AZ::Transform HMDTM = AZ::Transform::CreateIdentity();
                AZ::VR::TrackingState* trackingState = nullptr;
                EBUS_EVENT_RESULT(trackingState, AZ::VR::HMDDeviceRequestBus, GetTrackingState);

                if (trackingState)
                {
                    HMDTM.SetRotationPartFromQuaternion(trackingState->pose.orientation);
                    cameraTM.SetRotationPartFromQuaternion(AZ::Quaternion::CreateZero());
                    cameraTM.SetPosition(cameraPosition + (HMDTM * moveVec));
                }
            }
            else
            {
                // else move relative to current camera position

                // calculate rotation if not connected to HMD
                AZ::Quaternion rotationX = AZ::Quaternion::CreateRotationX(m_debugCameraRotation.GetX()*kDebugCameraRotateScale);
                AZ::Quaternion rotationY = AZ::Quaternion::CreateRotationY(m_debugCameraRotation.GetY()*kDebugCameraRotateScale);
                AZ::Quaternion rotationZ = AZ::Quaternion::CreateRotationZ(m_debugCameraRotation.GetZ()*kDebugCameraRotateScale);
                AZ::Quaternion rotationXYZ = rotationY * rotationZ * rotationX;

                cameraTM.SetRotationPartFromQuaternion(rotationXYZ);
                cameraTM.SetPosition(cameraPosition + (cameraTM * moveVec));
            }

            EBUS_EVENT_ID(entityId, AZ::TransformBus, SetWorldTM, cameraTM);
        }
    }
}

void HMDDebuggerComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
{
    if (m_debugFlags.test(EHMDDebugFlags::Info))
    {
        UpdateDebugInfo(deltaTime);
    }

    if (m_debugFlags.test(EHMDDebugFlags::Camera))
    {
        UpdateDebugCamera(deltaTime);
    }
}

bool HMDDebuggerComponent::OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel)
{
    const AzFramework::InputChannelId& inputChannelId = inputChannel.GetInputChannelId();
    switch (inputChannel.GetState())
    {
        case AzFramework::InputChannel::State::Began: // update the pressed input state
        {
            for (int i = 0; i < EHMDDebugCameraKeys::Count; ++i)
            {
                if (inputChannelId == kDebugCameraInputMap[i])
                {
                    m_debugCameraInputState[i] = true;
                    break;
                }
            }
            break;
        }
        case AzFramework::InputChannel::State::Ended: // update the released input state
        {
            for (int i = 0; i < EHMDDebugCameraKeys::Count; ++i)
            {
                if (inputChannelId == kDebugCameraInputMap[i])
                {
                    m_debugCameraInputState[i] = false;
                    break;
                }
            }
            break;
        }
        case AzFramework::InputChannel::State::Updated: // update the camera rotation
        {
            if (inputChannelId == AzFramework::InputDeviceMouse::Movement::X)
            {
                // modify yaw angle
                m_debugCameraRotation += AZ::Vector3(0.0f, 0.0f, inputChannel.GetValue());
            }
            else if (inputChannelId == AzFramework::InputDeviceMouse::Movement::Y)
            {
                // modify pitch angle
                m_debugCameraRotation += AZ::Vector3(inputChannel.GetValue(), 0.0f, 0.0f);
            }
        }
        default:
        {
            break;
        }
    }
    return false;
}
