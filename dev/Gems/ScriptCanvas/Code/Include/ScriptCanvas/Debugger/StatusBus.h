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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Outcome/Outcome.h>

namespace ScriptCanvas
{
    struct StatusErrors
    {
        AZStd::vector<AZStd::string> m_graphErrors;
        AZStd::vector<AZ::EntityId> m_invalidConnectionIds;
    };
    class Graph;
    class StatusRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Validates the graph for invalid connections between node's endpoints
        //! Any errors are logged to the "Script Canvas" window
        virtual AZ::Outcome<void, StatusErrors> ValidateGraph(const Graph&) = 0;
    };

    using StatusRequestBus = AZ::EBus<StatusRequests>;
}
