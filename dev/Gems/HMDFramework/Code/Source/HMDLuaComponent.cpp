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

#include <HMDBus.h>
#include <VRControllerBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include "HMDLuaComponent.h"

namespace AZ 
{
    namespace VR
    {
        AZ_CLASS_ALLOCATOR_IMPL(Playspace, SystemAllocator, 0)

        void HMDLuaComponent::Reflect(AZ::ReflectContext* context)
        {
            TrackingState::Reflect(context);
            HMDDeviceInfo::Reflect(context);
            Playspace::Reflect(context);

            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<HMDLuaComponent, AZ::Component>()
                    ->Version(1);
            }

            AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->EBus<HMDDeviceRequestBus>("HMDDeviceRequestBus")->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)->
                    Event("GetTrackingState", &HMDDeviceRequestBus::Events::GetTrackingState)->
                    Event("RecenterPose", &HMDDeviceRequestBus::Events::RecenterPose)->
                    Event("SetTrackingLevel", &HMDDeviceRequestBus::Events::SetTrackingLevel)->
                    Event("OutputHMDInfo", &HMDDeviceRequestBus::Events::OutputHMDInfo)->
                    Event("GetDeviceInfo", &HMDDeviceRequestBus::Events::GetDeviceInfo)->
                    Event("IsInitialized", &HMDDeviceRequestBus::Events::IsInitialized)->
                    Event("GetPlayspace", &HMDDeviceRequestBus::Events::GetPlayspace);

                behaviorContext->EBus<ControllerRequestBus>("ControllerRequestBus")->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)->
                    Event("GetTrackingState", &ControllerRequestBus::Events::GetTrackingState)->
                    Event("IsConnected", &ControllerRequestBus::Events::IsConnected);
            }
        }

        void TrackingState::Reflect(AZ::ReflectContext* context)
        {
            PoseState::Reflect(context);
            DynamicsState::Reflect(context);

            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<TrackingState>()
                    ->Version(1)
                    ->Field("pose", &TrackingState::pose)
                    ->Field("dynamics", &TrackingState::dynamics)
                    ->Field("statusFlags", &TrackingState::statusFlags)
                    ;
            }
            AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Class<TrackingState>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Property("pose", BehaviorValueProperty(&TrackingState::pose))
                    ->Property("dynamics", BehaviorValueProperty(&TrackingState::dynamics))
                    ->Property("statusFlags", BehaviorValueProperty(&TrackingState::statusFlags))
                    ;
            }
        }

        void PoseState::Reflect(AZ::ReflectContext* context)
        {
            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<PoseState>()
                    ->Version(1)
                    ->Field("orientation", &PoseState::orientation)
                    ->Field("position", &PoseState::position)
                    ;
            }
            AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Class<PoseState>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Property("orientation", BehaviorValueProperty(&PoseState::orientation))
                    ->Property("position", BehaviorValueProperty(&PoseState::position))
                    ;
            }
        }

        void DynamicsState::Reflect(AZ::ReflectContext* context)
        {
            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<DynamicsState>()
                    ->Version(1)
                    ->Field("angularVelocity", &DynamicsState::angularVelocity)
                    ->Field("angularAcceleration", &DynamicsState::angularAcceleration)
                    ->Field("linearVelocity", &DynamicsState::linearVelocity)
                    ->Field("linearAcceleration", &DynamicsState::linearAcceleration)
                    ;
            }

            AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Class<DynamicsState>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Property("angularVelocity", BehaviorValueProperty(&DynamicsState::angularVelocity))
                    ->Property("angularAcceleration", BehaviorValueProperty(&DynamicsState::angularAcceleration))
                    ->Property("linearVelocity", BehaviorValueProperty(&DynamicsState::linearVelocity))
                    ->Property("linearAcceleration", BehaviorValueProperty(&DynamicsState::linearAcceleration))
                    ;
            }
        }

        void HMDDeviceInfo::Reflect(AZ::ReflectContext* context)
        {
            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<HMDDeviceInfo>()
                    ->Version(1)
                    ->Field("productName", &HMDDeviceInfo::productName)
                    ->Field("manufacturer", &HMDDeviceInfo::manufacturer)
                    ->Field("renderWidth", &HMDDeviceInfo::renderWidth)
                    ->Field("renderHeight", &HMDDeviceInfo::renderHeight)
                    ->Field("fovH", &HMDDeviceInfo::fovH)
                    ->Field("fovV", &HMDDeviceInfo::fovV)
                    ;
            }

            AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Class<HMDDeviceInfo>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Property("productName", BehaviorValueProperty(&HMDDeviceInfo::productName))
                    ->Property("manufacturer", BehaviorValueProperty(&HMDDeviceInfo::manufacturer))
                    ->Property("renderWidth", BehaviorValueProperty(&HMDDeviceInfo::renderWidth))
                    ->Property("renderHeight", BehaviorValueProperty(&HMDDeviceInfo::renderHeight))
                    ->Property("fovH", BehaviorValueProperty(&HMDDeviceInfo::fovH))
                    ->Property("fovV", BehaviorValueProperty(&HMDDeviceInfo::fovV))
                    ;
            }
        }

        void Playspace::Reflect(AZ::ReflectContext* context)
        {
            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<Playspace>()
                    ->Version(1)
                    ->Field("isValid", &Playspace::isValid)
                    ->Field("corners", &Playspace::corners)
                    ;
            }

            AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Class<Playspace>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Property("isValid", BehaviorValueProperty(&Playspace::isValid))
                    ->Property("corners", BehaviorValueProperty(&Playspace::corners))
                    ;
            }
        }
    }
}
