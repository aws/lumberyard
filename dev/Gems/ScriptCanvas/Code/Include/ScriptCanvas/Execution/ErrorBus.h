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

#include <ScriptCanvas/Core/Endpoint.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string_view.h>

namespace ScriptCanvas
{
    class Node;
    //! Execution RequestBus for interfacing with a running graph
    class ErrorReporter : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        //! BusIdType represents a unique id for the execution component
        //! Because multiple Script Canvas graphs can execute on the same entity
        //! this is not an "EntityId" in the sense that it uniquely identifies an entity.
        using BusIdType = AZ::EntityId;

        virtual AZStd::string_view GetLastErrorDescription() const = 0;
        virtual void HandleError(const Node& callStackTop) = 0;
        virtual bool IsInErrorState() const = 0;
        virtual bool IsInIrrecoverableErrorState() const = 0;
        virtual void ReportError(const Node& reporter, const char* format, ...) = 0;
    };

    using ErrorReporterBus = AZ::EBus<ErrorReporter>;
}
