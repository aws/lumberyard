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

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>

#include <AzCore/JSON/document.h>

#include <LyzardSDK/Base.h>

#include <Engines/EnginesAPI.h>

#include <Projects/ProjectId.h>
#include <Gems/GemsAPI.h>

namespace Projects
{
    /**
    * Describes the source type of the project
    */
    enum SourceType
    {
        FilePath,   //< The project source is contained in the engine root
        ExternalFilePath,    //< The project source is outside of the engine root
        RawText,    //< The project source is entirely in memory (useful for mock/test environments)
    };

    /**
     * Processes requests relating to game projects.
     */
    class ProjectRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ProjectId;
        //////////////////////////////////////////////////////////////////////////

        virtual ~ProjectRequests() = default;

        /// Get the Id of the project.
        virtual ProjectId GetId() = 0;
        /// Get the Id of the engine instance this project is associated with.
        virtual Engines::EngineId GetEngineId() = 0;
        /// Get the name of the project.
        virtual const AZStd::string& GetName() const = 0;
        /// Get the relative path to the project from the current app root.
        virtual const AZStd::string& GetRelativePath() const = 0;
        /// Get the absolute path to the project's app root folder.
        virtual const AZStd::string& GetRootPath() const = 0;
        /// Get the full absolute path to the project
        virtual const AZStd::string& GetFullPath() const = 0;
        /// Returns the SourceType of the project
        virtual SourceType GetProjectSourceType() const = 0;

        /// Get the settings object at key in the Project Settings as a Document (contains an allocator). If the key doesn't exist, returns a valid document containing the "null" value.
        virtual AZ::Outcome<rapidjson::Document, AZStd::string> GetProjectSettingsValue(const char* key) = 0;
        /// Save a document to the project settings. Pass in nullptr to erase the object.
        virtual Lyzard::StringOutcome SaveProjectSettingsValue(const char* key, const rapidjson::Document* value) = 0;
    };

    using ProjectRequestBus = AZ::EBus<ProjectRequests>;

    class ProjectNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ProjectId;
        //////////////////////////////////////////////////////////////////////////

        virtual ~ProjectNotifications() = default;

        /// A root-level value in project settings has changed.
        /// \param key  The name of the value that has changed.
        /// \param previousValue    The old value. nullptr if
        ///                         this value has just been added.
        /// \param currentValue     The current value. If a value has just been
        ///                         removed then currentValue will be nullptr.
        void OnProjectSettingsValueChanged(const char* key, const rapidjson::Value* previousValue, const rapidjson::Value* currentValue) {}
    };

    using ProjectNotificationBus = AZ::EBus<ProjectNotifications>;

    /**
    * Options provided to CreateProject.  New projects will also involve creation of new game
    * GEMS, so the properties for the NewGemDescriptor will also need to be provided
    */
    struct NewProjectDescriptor :
        public Gems::GemsRequests::NewGemDescriptor
    {
        /// The engine to create the project on. Required.
        Engines::EngineId m_engine;

        AZStd::string m_templateName;
        AZStd::string m_logFileNameOverride;
    };

    /**
     * Processes requests relating to the existence of game projects.
     */
    class ProjectManagerRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual ~ProjectManagerRequests() = default;

        /// Find the first project known with name in the application root (returns it's Id).
        virtual ProjectId GetProjectByName(const AZStd::string& name) const = 0;

        /// Find the first project known with name and source project type (returns it's Id).
        virtual ProjectId GetProjectByNameAndSourceType(const AZStd::string& name, SourceType projectSourceType) const = 0;

        /// Get the list of all known projects (by Id).
        virtual AZStd::vector<ProjectId> GetAllProjectIds() const = 0;

        /**
         * Creates a new Project.
         *
         * \param[in] desc      The descriptor of the new Project to create.
         *
         * \returns             Void on success, error message on failure.
         */
        virtual AZ::Outcome<void, AZStd::string> CreateProject(const NewProjectDescriptor& desc) = 0;

        /**
         * Sets the currently active projects.
         *
         * \param[in] project   The Id of the project to make active.
         *
         * \returns             Void on success, error message on failure.
         */
        virtual AZ::Outcome<void, AZStd::string> SetActiveProject(ProjectId project) = 0;

        /// Gets the name of the active project.
        virtual AZ::Outcome<AZStd::string, AZStd::string> GetActiveProjectName() const = 0;

        /**
        * Perform validation on a project name against a particular engine for its validity
        *
        * \param[in] projectName    The name of the new project
        * \param[in] engineId       Engine Id to validate against
        * \param[in] allowExisting  Allow existing project names for this validation.  Generally used to update existing projects
        *
        * \returns                  Void on success, error message on failure.
        */
        virtual Lyzard::StringOutcome ValidateProjectName(const AZStd::string& projectName, const Engines::EngineId& engineId, bool allowExisting) const = 0;

        /**
        * Determine the location of the detailed log file that will capture the status/error logs from the CreateProject bus call. 
        *
        * \param[in] desc           The descriptor of the new Project to determine/create the log file
        * \param[in] createOnDemand Create the log file if it does not exist
        *
        * \returns                  The full path to the log file if successful, error message on failure.
        */
        virtual AZ::Outcome<AZStd::string, AZStd::string> GetProjectCreationLogFile(const NewProjectDescriptor& desc, bool createOnDemand) const = 0;

        /**
        *  Configure an external project folder to an engine path
        *
        * \param[in] pathToProjectFolder    The path to the external project folder to set the engine path for
        * \param[in] pathToEngineFolder     The path to the engine to set the external project to
        * \param[in] gameName               The name of the game in the external folder.  If blank, attempt to get it from a required bootstrap.cfg
        * \returns                          The version number of the engine path that the external project folder is set to on success, error message on failure
        */
        virtual AZ::Outcome<AZStd::string, AZStd::string> SetExternalProjectToEngine(const AZStd::string& pathToProjectFolder, const AZStd::string& pathToEngineFolder, const AZStd::string& gameName) const = 0;
    };

    using ProjectManagerRequestBus = AZ::EBus<ProjectManagerRequests>;

    /**
    * Processes WAF-related requests for game projects
    */
    class ProjectManagerWAFRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        // lockless dispatch is necssary to allow manual cancellation of waf requests since
        // they are all blocking calls
        static const bool LocklessDispatch = true;
        //////////////////////////////////////////////////////////////////////////

        virtual ~ProjectManagerWAFRequests() = default;

        /**
        * Run WAF configure 
        *
        * \param[in] desc           The descriptor of the new project to base the game project solution on
        *
        * \returns                  Void on success, error message on failure.
        */
        virtual Lyzard::StringOutcome WAFConfigure(const NewProjectDescriptor& desc) const = 0;

#if defined (AZ_PLATFORM_WINDOWS)
        /**
        * Run WAF to create a visual studio solution (on windows only)
        *
        * \param[in] desc           The descriptor of the new project to base the game project solution on
        * \param[in] msvsVersion    The product version number of visual studio to generate the solution.  If zero, use the current settings in _WAF_/user_settings.options
        *
        * \returns                  Void on success, error message on failure.
        */
        virtual Lyzard::StringOutcome GenerateVisualStudioSolution(const NewProjectDescriptor& desc, unsigned int  msvsVersion) const = 0;
#endif // AZ_PLATFORM_WINDOWS


        /**
        * Build the game project 
        *
        * \param[in] projectDesc            The new project descriptor
        * \param[in] targetPlatform         The target platform to build the project against
        * \param[in] targetConfiguration    The build configuration to build the target against.
        *
        * \returns                  Void on success, error message on failure.
        */
        virtual Lyzard::StringOutcome BuildProject(const NewProjectDescriptor& projectDesc, const AZStd::string& targetPlatform, const AZStd::string& targetConfiguration) const = 0;

        /**
        * Get the default msvs version specified in _WAF_/user_settings.options
        *
        * \param[in] engineId       Engine Id to determine the settings file
        *
        * \returns                  The msvs version specified in _WAF_/user_settings.options if successful, 0 if not
        */
        virtual int GetMsvsVersionFromSettings(const Engines::EngineId& engineId) const = 0;

        /**
        * Perform a clean of a project folder (and project build artifacts) if possible
        *
        * \param[in] projectName    The name of the new project
        * \param[in] engineId       Engine Id to validate against
        *
        * \returns                  true if anything was found and deleted, false if there was a problem
        */
        virtual Lyzard::StringOutcome DeleteProjectArtifacts(const AZStd::string& projectName, const Engines::EngineId& engineId) const = 0;

        /**
         * Cancels the last running WAF request
         * 
         * \returns     true if there is a request to cancel, false otherwise
         */
        virtual Lyzard::StringOutcome CancelWAFRequest() = 0;

    };

    using ProjectManagerWAFRequestsBus = AZ::EBus<ProjectManagerWAFRequests>;


    /**
     * Receive notifications whenever a Project operation is performed.
     */
    class ProjectManagerNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual ~ProjectManagerNotifications() = default;

        /// Called when a new project instance is created or loaded.
        virtual void OnProjectLoaded(ProjectId /*project*/) {};

        /// Called when a new project creation process fails
        virtual void OnProjectUnloaded(ProjectId /*project*/) {};

        /// Called whenever there is new output from a build
        virtual void OnNewBuildOutput(const AZStd::string& /*string*/) {};

        /// Called whenever output from an active build job is acquired
        virtual void OnBuildProgressUpdated(unsigned int jobNumber, unsigned int /*jobCount*/) {};
    };

    using ProjectManagerNotificationBus = AZ::EBus<ProjectManagerNotifications>;
}
