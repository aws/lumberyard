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

#include <LyzardSDK/Base.h>

#include <Projects/ProjectId.h>

#include <GemRegistry/IGemRegistry.h>

#define DEFAULT_GEM_PATH "Gems"

namespace Gems
{
    class GemsProjectRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = Projects::ProjectId;
        //////////////////////////////////////////////////////////////////////////

        virtual ~GemsProjectRequests() = default;

        /**
         * Enables the specified instance of a Gem.
         *
         * \param[in] spec      The specific Gem to enable.
         *
         * \returns             True on success, False on failure.
         */
        virtual bool EnableGem(const Gems::ProjectGemSpecifier& spec) = 0;

        /**
         * Disables the specified instance of a Gem.
         *
         * \param[in] spec      The specific Gem to disable.
         *
         * \returns             True on success, False on failure.
         */
        virtual bool DisableGem(const Gems::GemSpecifier& spec) = 0;

        /**
         * Checks if a Gem of the specified description is enabled.
         *
         * \param[in] spec      The specific Gem to check.
         *
         * \returns             True if the Gem is enabled, False if it is disabled.
         */
        virtual bool IsGemEnabled(const Gems::GemSpecifier& spec) const = 0;

        /**
         * Checks if a Gem of the specified ID and version constraints is enabled.
         *
         * \param[in] id                     The ID of the Gem to check.
         * \param[in] versionConstraints     An array of strings, each of which is a condition using the gem dependency syntax
         *
         * \returns             True if the Gem is enabled and passes every version constraint condition, False if it is disabled or does not match the version constraints.
         */
        virtual bool IsGemVersionsEnabled(const AZ::Uuid& id, const AZStd::vector<AZStd::string>& versionConstraints) const = 0;

        /**
        * Checks if all the gems dependencies of the specific Gem is met.
        *
        * \param[in] dep    The dependency of the gem to check in the project
        *
        * \returns          True if that dependency is enabled on the project, false otherwise.
        */
        virtual bool IsGemDependencyMet(const AZStd::shared_ptr<GemDependency> dep) const = 0;

        /**
        * Checks if the engine dependency is met.
        *
        * \param[in] dep    The dependency to validate
        * \param[in] againstVersion
        *                   The version of the engine to validate against
        *
        * \returns          True if that dependency is enabled on the project, false otherwise.
        */
        virtual bool IsEngineDependencyMet(const AZStd::shared_ptr<EngineDependency> dep, const EngineVersion& againstVersion) const = 0;

        /**
         * Gets the Gems known to this project.
         * Only enabled Gems are actually used at runtime.
         * A project can only reference one version of a Gem.
         *
         * \returns             The vector of enabled Gems.
         */
        virtual const Gems::ProjectGemSpecifierMap& GetGems() const = 0;

        /**
         * Checks that all installed Gems have their dependencies met.
         * Any unmet dependencies can be found via IGemRegistry::GetErrorMessage();
         *
         * \param[in] engineVersion
         *                      The version of the engine to validate against
         *
         * \returns             Void if all dependencies are met, error message on failure.
         */
        virtual Lyzard::StringOutcome ValidateDependencies(const EngineVersion& engineVersion) const = 0;

        /**
         * Saves the current state of the project settings to it's project configuration file.
         *
         * \returns             Void on success, error message on failure.
         */
        virtual void Save(AZStd::function<void(Lyzard::StringOutcome result)> callback) const = 0;

        /**
        * Examine a project's Gems list add any missing required Gem automatically
        * 
        * \returns             List of missing required gems that were added for the project
        */
        virtual AZStd::vector<IGemDescriptionConstPtr> AddMissingRequiredGemsForProject() = 0;

    };
    using GemsProjectRequestBus = AZ::EBus<GemsProjectRequests>;

    /**
     * Messages serviced by the GemsController.
     * The GemsController lets the user interact with the Gems system without
     * resorting to manual file parsing and manipulation.
     */
    class GemsRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual ~GemsRequests() = default;

        /**
         * Add to the list of paths to search for Gems when calling LoadAllGemsFromDisk
         *
         * \param[in] searchPath    The path to add
         * \param[in] loadGemsNow   Load Gems from searchPath now
         *
         * \returns                 Void on success, error message on failure.
         */
        virtual Lyzard::StringOutcome AddSearchPath(const AZStd::string& path, bool loadGemsNow) = 0;

        /**
         * Scans the registered search paths for all installed Gems.
         *
         * In the event of an error loading a Gem, the error will be recorded,
         * the Gem will not be loaded, and false will be returned.
         *
         * Loaded Gems can be accessed via GetGemDescription() or GetAllGemDescriptions().
         *
         * \returns             AZ::Success if the search succeeded, 
         *                      AZ::Failure with error message if any errors occurred.
         */
        virtual Lyzard::StringOutcome LoadAllGemsFromDisk() = 0;

        /**
        * Looks for a gems.json file in the given folder and returns a IGemDescriptionConstPtr if found
        *
        * \param[in] gemFolderRelPath   A valid path on disk to a directory that contains a gem.json file relative to the engine root
        *
        * \returns                 IGemDescriptionConstPtr if a gem.json file could be parsed out of the gemFolderPath,
        *                          AZ::Failure with error message if any errors occurred.
        */
        virtual AZ::Outcome<IGemDescriptionConstPtr, AZStd::string> ParseToGemDescriptionPtr(const AZStd::string& gemFolderRelPath, const char* absoluteFilePath) = 0;

        /**
        * Destroys the existing ProjectSettings object on the GemsProject and recreates a new one
        *
        * \returns                 True if the search succeeded, False if any errors occurred.
        */
        virtual bool RefreshProjectSettings(Projects::ProjectId project) = 0;

        /**
         * Loads Gems for the specified project.
         * May be called for multiple projects.
         *
         * \param[in] project   The ID of the project project to load Gems for.
         *
         * \returns             Void on success, error message on failure.
         */
        virtual Lyzard::StringOutcome LoadProjectGems(Projects::ProjectId project) = 0;

        /**
         * Gets the description for a Gem.
         *
         * \param[in] spec      The specific Gem to search for.
         *
         * \returns             A pointer to the Gem's description if it exists, otherwise nullptr.
         */
        virtual IGemDescriptionConstPtr GetGemDescription(const GemSpecifier& spec) const = 0;

        /**
         * Gets the description for the latest version of a Gem.
         *
         * \param[in] uuid      The specific uuid of the Gem to search for.
         *
         * \returns             A pointer to the Gem's description highest version if it exists, otherwise nullptr.
         */
        virtual IGemDescriptionConstPtr GetLatestGem(const AZ::Uuid& uuid) const = 0;

        /**
         * Options provided to CreateGem.
         */
        struct NewGemDescriptor
        {
            /// The gem template used to create the new gem. Defaults to default template.
            AZStd::string m_gemTemplate = "default_gem_template";

            /// The name of the new Gem. Required.
            AZStd::string m_name = "";
            /// The summary of the new gem. Optional.
            AZStd::string m_summary = "A short description of my Gem.";
            /// The Id of the new Gem. Defaults to Random Id.
            AZ::Uuid m_id = AZ::Uuid::CreateRandom();
            /// The Version of the new Gem. Defaults to 0.1.0
            Gems::GemVersion m_version = { 0, 1, 0 };
            /// The path to create the Gem at. Required, use DEFAULT_GEM_PATH as a default for non-project Gems.
            AZStd::string m_rootPath;
            /// The name of the folder to create the Gem in. Defaults to name, falls back to name-version.
            AZStd::string m_outFolder;
        };

        /**
         * Creates a new Gem.
         *
         * \param[in] desc      The descriptor of the new Gem to create.
         *
         * \returns             Void on success, error message on failure.
         */
        virtual Lyzard::StringOutcome CreateGem(const NewGemDescriptor& desc) = 0;

        /**
         * Gets a list of all loaded Gem descriptions.
         */
        virtual AZStd::vector<IGemDescriptionConstPtr> GetAllGemDescriptions() const = 0;

        /**
        * Gets a list of all required Gem descriptions.
        * 
        * \returns             List of all required Gems
        */
        virtual AZStd::vector<IGemDescriptionConstPtr> GetAllRequiredGemDescriptions() const = 0;


        /**
        * Gets the project specific gem if available
        *
        * \returns             The project specific gem description if it exists, null if not
        */
        virtual IGemDescriptionConstPtr GetProjectGemDescription(const AZStd::string& projectName) const = 0;

    };
    using GemsRequestBus = AZ::EBus<GemsRequests>;

    /**
     * Messages sent out by the GemsController.
     * Messages are sent when significant Gem actions are performed.
     */
    class GemsNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        /**
         * Sent when a Gem is enabled in a project.
         *
         * Params:
         *     spec:        Information about the Gem enabled
         *     projectId:   The id of the project the Gem was enabled in
         */
        virtual void OnGemEnabled(const Gems::ProjectGemSpecifier& /*spec*/, Projects::ProjectId /*projectId*/) { }

        /**
         * Sent when a Gem is disabled in a project.
         *
         * Params:
         *     spec:        Information about the Gem enabled
         *     projectId:   The id of the project the Gem was enabled in
         */
        virtual void OnGemDisabled(const Gems::GemSpecifier& /*spec*/, Projects::ProjectId /*projectId*/) { }

        /**
         * Sent when the current state of the project settings is being saved to a file.
         */
        virtual Lyzard::StringOutcome OnGemsProjectSave(Projects::ProjectId /*projectId*/) { return AZ::Success(); }
    };
    using GemsNotificationBus = AZ::EBus<GemsNotifications>;
}
