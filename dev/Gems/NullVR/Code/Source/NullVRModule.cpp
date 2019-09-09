/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

#include "NullVR_precompiled.h"
#include <platform_impl.h>

#include "NullVRDevice.h"

#include <IGem.h>

#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>

namespace NullVR
{
    class NullVRModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(NullVRModule, "{93DFA1CE-E049-4D71-91CD-09B11673A54D}", CryHooksModule);

        NullVRModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                NullVRDevice::CreateDescriptor(),
            });

            // This is an internal Amazon gem, so register it's components for metrics tracking, otherwise the name of the component won't get sent back.
            // IF YOU ARE A THIRDPARTY WRITING A GEM, DO NOT REGISTER YOUR COMPONENTS WITH EditorMetricsComponentRegistrationBus
            AZStd::vector<AZ::Uuid> typeIds;
            typeIds.reserve(m_descriptors.size());
            for (AZ::ComponentDescriptor* descriptor : m_descriptors)
            {
                typeIds.emplace_back(descriptor->GetUuid());
            }
            EBUS_EVENT(AzFramework::MetricsPlainTextNameRegistrationBus, RegisterForNameSending, typeIds);
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<NullVRDevice>(),
            };
        }

        void OnCrySystemCVarRegistry() override
        {
            // Note: Defaults are based on HTC Vive CV1 as of April 6, 2017
            REGISTER_CVAR(hmd_null_renderWidth, 1512, 0, "The width of the render texture in pixels. Set once on start.");
            REGISTER_CVAR(hmd_null_renderHeight, 1680, 0, "The height of the render texture in pixels. Set once on start.");
            REGISTER_CVAR(hmd_null_fovH, 1.84633982f, 0, "The field of view height in radians. Used on start.");
            REGISTER_CVAR(hmd_null_fovV, 1.94590104f, 0, "The field of view width in radians. Used on start.");
            REGISTER_CVAR(hmd_null_fov, 1.91986f, 0, "The field of view in radians. Can be updated at runtime.");
            REGISTER_CVAR(hmd_null_aspectRatio, 0.9f, 0, "Aspect ratio of the null VR implementation.");
            REGISTER_CVAR(hmd_null_frustumPlane_horizontalDistance, -0.0701720715f, 0, "Frustum plane horizontal distance.");
            REGISTER_CVAR(hmd_null_frustumPlane_verticalDistance, -0.00206005573f, 0, "Frustum plane vertical distance.");
            REGISTER_CVAR(hmd_null_eyeOffsetX, -0.0346999988f, 0, "Eye X offset (Horizontal distance).");
        }

    private:
        // null render cvars
        int hmd_null_renderWidth;
        int hmd_null_renderHeight;
        float hmd_null_fovH;
        float hmd_null_fovV;
        float hmd_null_fov;
        float hmd_null_aspectRatio;
        float hmd_null_frustumPlane_horizontalDistance;
        float hmd_null_frustumPlane_verticalDistance;
        float hmd_null_eyeOffsetX;
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(NullVR_f7e6617abd7b4a83ae113ade4c641032, NullVR::NullVRModule)
