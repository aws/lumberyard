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

#pragma once

///
/// Perform debugging operations for any HMD devices that are connected to the system.
/// This can include drawing debug geometry to represent different HMD objects (such as HMD controllers)
/// and logging any specific info regarding the HMDs themselves.
///

#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/vector.h>
#include <HMDBus.h>
#include <VRControllerBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/containers/bitset.h>

// for input
#include <InputNotificationBus.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>

class HMDDebuggerComponent
    : public AZ::VR::HMDDebuggerRequestBus::Handler
    , public AzFramework::InputChannelEventListener
    , public AZ::TickBus::Handler
    , public AZ::Component
{
public:

    enum EHMDDebugCameraKeys
    {
        Forward = 0,
        Back,
        Left,
        Right,

        Count
    };

    AZ_COMPONENT(HMDDebuggerComponent, "{CFBDF646-1E50-4863-9782-D3BA7EB44DC5}");
    static void Reflect(AZ::ReflectContext* context);
    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

    HMDDebuggerComponent() = default;
    ~HMDDebuggerComponent() override = default;

    /// AZ::Component //////////////////////////////////////////////////////////
    void Init() override;
    void Activate() override;
    void Deactivate() override;
    ////////////////////////////////////////////////////////////////////////////

    /// TickBus ////////////////////////////////////////////////////////////////
    void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
    ////////////////////////////////////////////////////////////////////////////

    /// AzFramework::InputChannelEventListener ////////////////////////////////
    bool OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel) override;
    ///////////////////////////////////////////////////////////////////////////

    /// HMDDebuggerBus /////////////////////////////////////////////////////////
    void EnableInfo(bool enable) override;
    void EnableCamera(bool enable) override;
    ////////////////////////////////////////////////////////////////////////////

private:
    // called whenever debug flags have changed
    void OnDebugFlagsChanged();

    // Update HMD Debug info
    void UpdateDebugInfo(float delta);

    // Update HMD Debug camera
    void UpdateDebugCamera(float delta);

    // This class is not copyable.
    HMDDebuggerComponent(const HMDDebuggerComponent& other) = delete;
    HMDDebuggerComponent& operator=(const HMDDebuggerComponent& other) = delete;

    // HMD Debug flags
    enum EHMDDebugFlags
    {
        Info = 0,
        Camera = 1
    };

    AZStd::bitset<2> m_debugFlags;
    AZ::Vector3 m_debugCameraRotation;
    bool m_debugCameraInputState[EHMDDebugCameraKeys::Count];
};
