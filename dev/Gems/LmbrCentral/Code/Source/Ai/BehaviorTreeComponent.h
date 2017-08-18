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
#include "include/LmbrCentral/Ai/BehaviorTreeAsset.h"
#include "include/LmbrCentral/Ai/BehaviorTreeComponentBus.h"
#include <AzCore/Asset/AssetCommon.h>

namespace LmbrCentral
{
    //////////////////////////////////////////////////////////////////////////
    /// The BehaviorTreeComponent allows you to run a behavior tree for the 
    /// attached entity.  Behavior Trees are often used for AI and animation
    class BehaviorTreeComponent
        : public AZ::Component
        , public BehaviorTreeComponentRequestBus::Handler
    {
    public:
        AZ_COMPONENT(BehaviorTreeComponent, "{8D36BC54-507B-41EF-989B-AE1A64B5E60C}");
        //////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* context);

    private:
        //////////////////////////////////////////////////////////////////
        // BehaviorTreeComponentRequestBus::Handler
        void StartBehaviorTree() override;
        void StopBehaviorTree() override;
        AZStd::vector<AZ::Crc32> GetVariableNameCrcs() override;
        bool GetVariableValue(AZ::Crc32 variableNameCrc) override;
        void SetVariableValue(AZ::Crc32 variableNameCrc, bool newValue) override;

        //////////////////////////////////////////////////////////////////
        // Reflected Data
        AZ::Data::Asset<BehaviorTreeAsset> m_behaviorTreeAsset;
        bool m_enabledInitially = true;
    };
} // namespace LmbrCentral