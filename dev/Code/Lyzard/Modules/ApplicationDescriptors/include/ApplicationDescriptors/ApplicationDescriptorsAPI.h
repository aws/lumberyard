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

#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplication.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <LyzardSDK/Base.h>

#include <Projects/ProjectId.h>

namespace AZ
{
    class Entity;
    class ModuleEntity;

    namespace rapidxml
    {
        template<class Ch> class xml_document;
    }
}

namespace ApplicationDescriptors
{
    /**
     * All current module Configurations
     * WARNING: This list is subject to change.
     */
    enum class ConfigurationId
    {
        Game,
        Editor,

        ConfigurationIdMax,
    };

    /**
     * Represents a build "Flavor", consisting of a platform (Windows, consoles, etc.) and a configuration (Game, Editor, etc.).
     * Subclasses of IFlavorBase provide different ways of accessing the
     * data in an application descriptor file.
     */
    class IFlavorBase
    {
    public:
        AZ_TYPE_INFO(IFlavorBase, "{8C70CC11-542E-4E5B-9548-ACCE492C3B08}");
        virtual ~IFlavorBase() = default;

        /// Get the Project for this flavor
        virtual Projects::ProjectId GetProjectId() const = 0;
        /// The Platform for this flavor
        virtual AZ::PlatformID GetPlatform() const = 0;
        /// The Configuration for this flavor
        virtual ConfigurationId GetConfiguration() const = 0;

        /// Callback used for async save/load operations.
        using CompletionCB = AZStd::function<void(Lyzard::StringOutcome result)>;

        /**
        * Asynchronously save the app descriptor (Descriptor and SystemEntity) to disk.
        *
        * \param completeCb Callback invoked (on the main thread) when the operation is complete.
        */
        virtual void SaveToDisk(CompletionCB completeCb) const = 0;
        /// Synchronously save the app descriptor (Descriptor and SystemEntity) to disk.
        virtual Lyzard::StringOutcome SaveToDiskBlocking() const = 0;

        /**
        * Asynchronously load the app descriptor (Descriptor and SystemEntity) from disk.
        *
        * \param completeCb Callback invoked (on the main thread) when the operation is complete.
        */
        virtual void LoadFromDisk(CompletionCB completeCb) = 0;
        /// Synchronously loads the flavor from disk.
        virtual Lyzard::StringOutcome LoadFromDiskBlocking() = 0;

        /// Checks if the flavor is loaded and ready.
        virtual bool IsValid() const = 0;
    };

    /**
     * IFlavor provides access to the classes that are serialized within an application descriptor file.
     */
    class IFlavor
        : public IFlavorBase
    {
    public:
        AZ_TYPE_INFO(IFlavor, "{97F600E1-1208-4C9D-B7DB-9C043340D8CF}", IFlavorBase);
        ~IFlavor() override = default;

        /// The Application Descriptor used to initialize allocators on startup
        virtual AZ::ComponentApplication::Descriptor* GetDescriptor() const = 0;

        /// The System Entity used to store System Components
        virtual AZ::Entity* GetSystemEntity() const = 0;

        /// If successful, return a list of required system components that the system entity is missing.
        /// If failed, return error message.
        virtual AZ::Outcome<AZ::ComponentTypeList, AZStd::string> GetMissingSystemComponents() = 0;

        /// If successful, return a ComponentFilter for adding components to this flavor's system entity.
        /// If failed, return error message.
        virtual AZ::Outcome<AzToolsFramework::ComponentFilter, AZStd::string> GetEligibleSystemComponentsFilter() = 0;

        /// Takes ownership of the system entity from the component application to the Flavor.
        virtual void TakeOwnershipOfSystemEntity() = 0;

        /// Releases ownership of the system entity back to the component application.
        virtual void ReleaseOwnershipOfSystemEntity() = 0;
    };

    using IFlavorPtr = AZStd::shared_ptr<IFlavor>;

    /**
     * IFlavorXml provides access to the raw unprocessed data in an application descriptor file.
     * This can be useful when we need to edit the file, but the SerializeContext
     * isn't aware of all types stored within the file.
     */
    class IFlavorXml
        : public IFlavorBase
    {
    public:
        AZ_TYPE_INFO(IFlavorXml, "{FE173F5C-6908-4192-BA5D-455E05BA9B4D}", IFlavorBase);

        virtual const AZ::rapidxml::xml_document<char>* GetXmlDocument() const = 0;
        virtual AZ::rapidxml::xml_document<char>* GetXmlDocument() = 0;

        /// Update the list of modules used by this flavor.
        virtual Lyzard::StringOutcome PopulateModules() = 0;
    };

    using IFlavorXmlPtr = AZStd::shared_ptr<IFlavorXml>;

    /**
     * Provides services relating to editing System Components and the Application Descriptor.
     */
    class ApplicationDescriptorsRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual ~ApplicationDescriptorsRequests() = default;

        //////////////////////////////////////////////////////////////////////////
        // ApplicationDescriptorsRequests

        /// Get the string name of configuration.
        virtual const char* GetConfigurationName(ConfigurationId configuration) = 0;

        /// Gets the flavor instance specified.
        virtual IFlavorPtr GetFlavor(Projects::ProjectId projectId, AZ::PlatformID platform, ConfigurationId configuration) = 0;

        /// Load and reflect modules for a given flavor.
        /// \param failOnModuleError Whether this function will fail if module-loading error occurs.
        virtual Lyzard::StringOutcome LoadModulesForFlavor(const IFlavor& flavor, bool failOnModuleError) = 0;

        /// Get list of modules used by this flavor.
        /// \note The modules will be loaded into memory, if they aren't already.
        virtual AZ::Outcome<AZStd::vector<AZ::Module*>, AZStd::string> GetModulesForFlavor(const IFlavor& flavor) = 0;

        /// Get list of components available to this flavor.
        /// \note This flavor's modules will be loaded into memory, if they aren't already.
        virtual AZ::Outcome<AZStd::vector<AZ::ComponentDescriptor*>, AZStd::string> GetComponentsAvailableToFlavor(const IFlavor& flavor) = 0;

        virtual IFlavorXmlPtr GetFlavorXml(Projects::ProjectId projectId, AZ::PlatformID platform, ConfigurationId configuration) = 0;

        /// Split the system entity into the system entity and a list of ModuleEntities
        virtual AZStd::vector<AZStd::unique_ptr<AZ::ModuleEntity>> SplitSystemEntity(AZ::Entity& systemEntity) = 0;

        //////////////////////////////////////////////////////////////////////////
    };
    using ApplicationDescriptorsRequestBus = AZ::EBus<ApplicationDescriptorsRequests>;
}
