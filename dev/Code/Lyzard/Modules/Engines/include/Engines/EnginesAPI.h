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
#include <AzCore/Math/Uuid.h>
#include <AzCore/Outcome/Outcome.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>

namespace AzFramework
{
    // Version struct forward declaration
    template <size_t N>
    struct Version;
}

namespace Gems
{
    // Forward declaring EngineVersion
    using EngineVersion = AzFramework::Version<4>;
}

namespace Engines
{
    /// Unique identifier for engine installations
    using EngineId = AZ::Uuid;

    /**
     * Processes requests relating to individual engine instances.
     */
    class EngineRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = EngineId;
        //////////////////////////////////////////////////////////////////////////

        virtual ~EngineRequests() = default;

        /// Get the Id of the engine instance.
        virtual EngineId GetId() = 0;
        /// Get the path to the root of the instance (/dev).
        virtual const AZStd::string& GetPath() = 0;
        /// Get the version of the engine instance.
        virtual const Gems::EngineVersion& GetEngineVersion() = 0;
    };
    using EngineRequestBus = AZ::EBus<EngineRequests>;

    /**
     * Processes requests relating to all engine instances.
     */
    class EngineManagerRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual ~EngineManagerRequests() = default;

        /// Discover all engine installation from app root
        virtual void LoadEngines() = 0;

        /// Get the list of all known engine instances (by Id).
        virtual AZStd::vector<EngineId> GetAllEngineIds() = 0;

        /// Get the active engine, if available. 
        /// Returns Uuid::CreateNull() if engine is not available.
        /// Currently it is assumed that GetAllEngineIds()[0] is the active engine.
        virtual EngineId GetActiveEngineId() const = 0;

        // Check if any Lumberyard Editor is currently running
        virtual bool IsEditorProcessRunning() const = 0;
    };
    using EngineManagerRequestBus = AZ::EBus<EngineManagerRequests>;

    /**
     * Receive notifications whenever an Engine operation is performed.
     */
    class EngineManagerNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual ~EngineManagerNotifications() = default;

        /// Called when a new engine instance is created or loaded.
        virtual void OnEngineLoaded(EngineId engine) = 0;
    };
    using EngineManagerNotificationBus = AZ::EBus<EngineManagerNotifications>;
}
