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

#include "VideoPlayback_precompiled.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <platform_impl.h>

#include "VideoPlaybackSystemComponent.h"
#include "VideoPlaybackGameComponent.h"

#include <IGem.h>

namespace AZ
{
    namespace VideoPlayback
    {
        class VideoPlaybackModule
            : public CryHooksModule
        {
        public:
            AZ_RTTI(VideoPlaybackModule, "{602AE553-0CF4-4F0B-8BEA-6F96643D4C57}", CryHooksModule);

            VideoPlaybackModule()
                : CryHooksModule()
            {
                // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
                m_descriptors.insert(m_descriptors.end(), {
                    VideoPlaybackSystemComponent::CreateDescriptor(),
                    VideoPlaybackGameComponent::CreateDescriptor(),
                });
            }

            /**
             * Add required SystemComponents to the SystemEntity.
             */
            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return AZ::ComponentTypeList{
                    azrtti_typeid<VideoPlaybackSystemComponent>(),
                };
            }

            void OnSystemEvent(ESystemEvent event, UINT_PTR, UINT_PTR) override
            {
                switch (event)
                {
                case ESYSTEM_EVENT_FLOW_SYSTEM_REGISTER_EXTERNAL_NODES:
                    RegisterExternalFlowNodes();
                    break;
                }
            }
        };
    }//namespace VideoPlayback
}//namespace AZ

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(VideoPlayback_632473900ed84df3a2bad2887a3bff56, AZ::VideoPlayback::VideoPlaybackModule)
