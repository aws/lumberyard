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

#include <LyzardSDK/Base.h>

#include <AzCore/EBus/EBus.h>

#include <AzCore/std/containers/set.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <Projects/ProjectId.h>

namespace Projects
{
    /**
     * Settings for a project platform.
     * ProjectPlatform is one factor in a ProjectFlavor.
     */
    struct ProjectPlatform
    {
        virtual ~ProjectPlatform() = default;

        virtual const ProjectPlatformId& GetId() const = 0;

        /// A text description of the ProjectPlatform.
        /// \example "For mobile phone platforms (ios, android, etc)"
        AZStd::string m_description;

        /// The contents of a #if which is used at runtime to choose this ProjectPlatform.
        /// \example "AZ_PLATFORM_APPLE_IOS || AZ_PLATFORM_ANDROID"
        AZStd::string m_cppConditional;

        /// The BuildPlatformIds which correspond to this ProjectPlatform.
        /// \example {"android_armv7_gcc", "ios"}
        AZStd::set<BuildPlatformId> m_correspondingBuildPlatforms;
    };

    /**
     * Settings for a project configuration.
     * ProjectConfiguration is one factor in a ProjectFlavor.
     */
    struct ProjectConfiguration
    {
        virtual ~ProjectConfiguration() = default;

        virtual const ProjectConfigurationId& GetId() const = 0;

        /// A text description of the ProjectConfiguration.
        /// \example "The 'final' configuration is what we ship to customers"
        AZStd::string m_description;

        /// The contents of a #if which is used at runtime to choose this ProjectPlatform.
        /// \example "_RELEASE && !DEDICATED_SERVER"
        AZStd::string m_cppConditional;

        /// All ProjectPlatformIds which are valid flavors when combined with this ProjectConfiguration.
        AZStd::set<ProjectPlatformId> m_compatibleProjectPlatforms;

        /// The BuildConfigurationIds which map to this ProjectConfiguration.
        /// \example {"performance", "release"}
        AZStd::set<BuildConfigurationId> m_correspondingBuildConfigurations;
    };

    using ProjectPlatformPtr = AZStd::shared_ptr<ProjectPlatform>;
    using ProjectConfigurationPtr = AZStd::shared_ptr<ProjectConfiguration>;

    /**
     * All settings concerning a project's flavors (see ProjectFlavor).
     */
    class ProjectFlavorSettings
    {
    public:
        virtual ~ProjectFlavorSettings() = default;

        /// \return the ProjectId which owns these settings.
        virtual const ProjectId& GetProjectId() const = 0;

        /// \return all ProjectPlatformIds known to this project.
        virtual AZStd::vector<ProjectPlatformId> GetPlatformIds() const = 0;

        /// \return pointer to ProjectPlatform data, or empty pointer if not found.
        virtual ProjectPlatformPtr GetPlatform(const ProjectPlatformId& platform) = 0;

        /// Add a new ProjectPlatform.
        /// \param platform The new platform's name.
        /// \param copyFrom (optional) If specified, the new platform copies settings from this platform.
        ///                 If unspecified, the new platform uses default settings.
        /// \return         ProjectPlatform pointer on success, error message on failure.
        virtual AZ::Outcome<ProjectPlatformPtr, AZStd::string> AddPlatform(const ProjectPlatformId& platform, const ProjectPlatformId* copyFrom = nullptr) = 0;

        /// Remove a ProjectPlatform.
        /// \return Void on success, error message on failure.
        virtual Lyzard::StringOutcome RemovePlatform(const ProjectPlatformId& platform) = 0;

        /// Rename a ProjectPlatform.
        /// \return Void on success, error message on failure.
        virtual Lyzard::StringOutcome RenamePlatform(const ProjectPlatformId& oldName, const ProjectPlatformId& newName) = 0;

        /// \return all ProjectConfigurationIds known to this project.
        virtual AZStd::vector<ProjectConfigurationId> GetConfigurationIds() = 0;

        /// \return pointer to ProjectConfiguration data, or empty pointer if not found.
        virtual ProjectConfigurationPtr GetConfiguration(const ProjectConfigurationId& configuration) = 0;

        /// Add a new ProjectConfiguration.
        /// \param configuration The new configuration's name.
        /// \param copyFrom (optional) If specified, the new configuration copies
        ///                 settings from this configuration.
        ///                 If unspecified, the new configuration uses default settings.
        /// \return         ProjectConfiguration pointer on success, error message on failure.
        virtual AZ::Outcome<ProjectConfigurationPtr, AZStd::string> AddConfiguration(const ProjectConfigurationId& configuration, const ProjectConfigurationId* copyFrom = nullptr) = 0;

        /// Remove a ProjectConfiguration.
        /// \return Void on success, error message on failure.
        virtual Lyzard::StringOutcome RemoveConfiguration(const ProjectConfigurationId& configuration) = 0;

        /// Rename a ProjectConfiguration.
        /// \return Void on success, error message on failure.
        virtual Lyzard::StringOutcome RenameConfiguration(const ProjectConfigurationId& oldName, const ProjectConfigurationId& newName) = 0;

        /// \return the ProjectConfiguration used by the Editor.
        virtual ProjectConfigurationId GetEditorConfiguration() const = 0;

        /// Set the ProjectConfiguration used by the Editor.
        virtual void SetEditorConfiguration(const ProjectConfigurationId& configuration) = 0;

        /// \return all valid ProjectFlavors.
        virtual AZStd::vector<ProjectFlavor> GetFlavors() const = 0;

        /// \return whether this is a valid ProjectFlavor.
        virtual bool IsValidFlavor(const ProjectFlavor& flavor) const = 0;


        /// Check validity of current settings.
        /// \param[out] errorsOut    (optional) If specified, fatal error
        ///                          messages are pushed into this parameter.
        /// \param[out] warningsOut  (optional) If specified, non-fatal warning
        ///                          messages will be pushed into this parameter.
        /// \return  False if any errors exist.
        ///          True if there are no errors (there may still be warnings).
        virtual bool Validate(AZStd::vector<AZStd::string>* errorsOut = nullptr, AZStd::vector<AZStd::string>* warningsOut = nullptr) const = 0;
    };

    /**
     * Requests for accessing and modifying a project's flavors.
     */
    class ProjectFlavorRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual ~ProjectFlavorRequests() = default;

        /// Get a project's flavor settings.
        /// \return ProjectFlavorSettings on success, error message on failure.
        virtual AZ::Outcome<AZStd::unique_ptr<ProjectFlavorSettings>, AZStd::string> GetFlavorSettings(const ProjectId& projectId) = 0;

        /// Save a project's flavor settings.
        /// \return Void on success, error message on failure.
        virtual Lyzard::StringOutcome SaveFlavorSettings(const ProjectFlavorSettings& flavorSettings) = 0;
    };
    using ProjectFlavorRequestBus = AZ::EBus<ProjectFlavorRequests>;

    /**
     * Notifications related to a project's flavors.
     */
    class ProjectFlavorNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusId = ProjectId;
        //////////////////////////////////////////////////////////////////////////

        virtual ~ProjectFlavorNotifications() = default;
    };
}