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

#include <AzCore/Component/Component.h>
#include <LyShine/Bus/World/UiCanvasRefBus.h>
#include <LyShine/Bus/World/UiCanvasOnMeshBus.h>
#include <AzCore/Math/Vector3.h>

struct IPhysicalEntity;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiCanvasOnMeshComponent
    : public AZ::Component
    , public UiCanvasOnMeshBus::Handler
    , public UiCanvasAssetRefNotificationBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiCanvasOnMeshComponent, "{0C1B2542-6813-451A-BD11-42F92DD48E36}");

    UiCanvasOnMeshComponent();

    // UiCanvasOnMeshInterface
    bool ProcessCollisionInputEvent(const AzFramework::InputChannel::Snapshot& inputSnapshot, int triangleIndex, Vec3 hitPoint) override;
    bool ProcessRayHitInputEvent(const AzFramework::InputChannel::Snapshot& inputSnapshot, const ray_hit& rayHit) override;
    // ~UiCanvasOnMeshInterface

    // UiCanvasAssetRefListener
    void OnCanvasLoadedIntoEntity(AZ::EntityId uiCanvasEntity) override;
    // ~UiCanvasAssetRefListener

public: // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("UiCanvasOnMeshService", 0xd2539f92));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("UiCanvasOnMeshService", 0xd2539f92));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("LegacyMeshService", 0xb462a299));
        required.push_back(AZ_CRC("UiCanvasRefService", 0xb4cb5ef4));
    }

    static void Reflect(AZ::ReflectContext* context);

protected: // member functions

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    bool ProcessCollisionInputEventInternal(const AzFramework::InputChannel::Snapshot& inputSnapshot, int triangleIndex, Vec3 hitPoint, IPhysicalEntity* collider, int partIndex);
    AZ::EntityId GetCanvas();

    AZ_DISABLE_COPY_MOVE(UiCanvasOnMeshComponent);

protected: // data

    //! Render target name to use (overrides the render target name in the UI canvas)
    AZStd::string m_renderTargetOverride;

    //! The UI Canvas that this component provides mesh collision input services for
    AZ::EntityId m_canvasEntityId;
};
