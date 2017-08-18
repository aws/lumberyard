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

#include <AzCore/Component/ComponentBus.h>

namespace LmbrCentral
{
    /**
    * Represents a request as submitted by a user of this component, can be used to start or stop a behavior tree.
    */
    class BehaviorTreeRequests
        : public AZ::ComponentBus
    {
    public:

        ///////////////////////////////////////////////////////////////////////////
        // Start an inactive behavior tree associated with this entity
        virtual void StartBehaviorTree() = 0;

        ///////////////////////////////////////////////////////////////////////////
        // Stop an active behavior tree associated with this entity
        virtual void StopBehaviorTree() = 0;

        ///////////////////////////////////////////////////////////////////////////
        // Get a list of all crc32s of the variable names
        virtual AZStd::vector<AZ::Crc32> GetVariableNameCrcs() = 0;

        ///////////////////////////////////////////////////////////////////////////
        // Get the value associated with a variable
        virtual bool GetVariableValue(AZ::Crc32 variableNameCrc) = 0;

        ///////////////////////////////////////////////////////////////////////////
        // Set the value associated with a variable
        virtual void SetVariableValue(AZ::Crc32 variableNameCrc, bool newValue) = 0;
    };

    // Bus to service the Behavior Tree component 
    using BehaviorTreeComponentRequestBus = AZ::EBus<BehaviorTreeRequests>;

} // namespace LmbrCentral
