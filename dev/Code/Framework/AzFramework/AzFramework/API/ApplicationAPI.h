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

#ifndef AZFRAMEWORK_APPLICATIONAPI_H
#define AZFRAMEWORK_APPLICATIONAPI_H

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/std/string/string.h>

#include <AzFramework/CommandLine/CommandLine.h>

namespace AZ
{
    class Entity;
    class ComponentApplication;

    namespace Data
    {
        class AssetDatabase;
    }
}

namespace AzFramework
{
    class ApplicationRequests
        : public AZ::EBusTraits
    {
    public:

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides - Application is a singleton
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        ApplicationRequests() = default;
        ~ApplicationRequests() = default;

        using Bus = AZ::EBus<ApplicationRequests>;

        /// Fixup slashes and lowercase path.
        virtual void NormalizePath(AZStd::string& /*path*/) = 0;

        /// Fixup slashes.
        virtual void NormalizePathKeepCase(AZStd::string& /*path*/) = 0;

        /// Make path relative, based on the application root.
        virtual void MakePathRootRelative(AZStd::string& /*fullPath*/) {}

        /// Make path relative, based on the asset root.
        virtual void MakePathAssetRootRelative(AZStd::string& /*fullPath*/) {}

        /// Make path relative to the provided root.
        virtual void MakePathRelative(AZStd::string& /*fullPath*/, const char* /*rootPath*/) {}

        /// Retrieves the asset root path for the application.
        virtual const char* GetAssetRoot() { return nullptr; }

        /// Sets the asset root path for the application.
        virtual void SetAssetRoot(const char* /*assetRoot*/) {}

        /// Get the Command Line arguments passed in.
        virtual const CommandLine* GetCommandLine() { return nullptr; }

        /*!
        * Returns a Type Uuid of the component for the given componentId and entityId.
        * if no comopnent matches the entity and component Id pair, a Null Uuid is returned
        * \param entityId - the Id of the entity containing the component
        * \param componentId - the Id of the component whose TypeId you wish to get
        */
        virtual AZ::Uuid GetComponentTypeId(const AZ::EntityId& entityId, const AZ::ComponentId& componentId) { (void)entityId; (void)componentId; return AZ::Uuid::CreateNull(); };

        template<typename ComponentFactoryType>
        void RegisterComponentType()
        {
            RegisterComponent(new ComponentFactoryType());
        }
    };

    class ApplicationLifecycleEvents
        : public AZ::EBusTraits
    {
    public:
        enum class Event
        {
            None = 0,
            Unconstrain,
            Constrain,
            Suspend,
            Resume
        };

        // Bus Configuration
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual ~ApplicationLifecycleEvents() {}

        using Bus = AZ::EBus<ApplicationLifecycleEvents>;

        // AzFramework::Application listens for all the system specific
        // events, and translates them into the system independent ones
        // defined here. Applications are free to listen to one or both
        // sets of events but responding just to the system independent
        // ones should be sufficient for most applications.
        virtual void OnApplicationConstrained(Event /*lastEvent*/) {}
        virtual void OnApplicationUnconstrained(Event /*lastEvent*/) {}

        virtual void OnApplicationSuspended(Event /*lastEvent*/) {}
        virtual void OnApplicationResumed(Event /*lastEvent*/) {}

        virtual void OnMobileApplicationWillTerminate() {}
        virtual void OnMobileApplicationLowMemoryWarning() {}
    };

    class ApplicationLifecycleEventsHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(ApplicationLifecycleEventsHandler, AZ::SystemAllocator, 0);
        ApplicationLifecycleEventsHandler();
        ~ApplicationLifecycleEventsHandler();
    private:
        // Private implementation to be implemented for each platform
        class Pimpl;
        Pimpl* m_pimpl;
    };
} // namespace AzFramework

#endif // AZFRAMEWORK_APPLICATIONAPI_H
