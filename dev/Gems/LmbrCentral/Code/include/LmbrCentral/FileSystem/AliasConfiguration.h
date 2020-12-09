/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */

#pragma once

#include <AzCore/Interface/Interface.h>

namespace LmbrCentral
{
    //! Stores the set of alias paths to be applied to the FileIOBase instance
    //! An empty path will keep the existing value, if any, that is already applied to the FileIOBase instance
    struct AliasConfiguration
    {
        std::string m_rootPath;
        std::string m_assetsPath;
        std::string m_userPath;
        std::string m_logPath;
        std::string m_cachePath;
        std::string m_engineRootPath;
        std::string m_devRootPath;
        std::string m_devAssetsPath;
        bool m_allowedRemoteIo{ false };
    };

    struct IAliasConfiguration
    {
        AZ_RTTI(IAliasConfiguration, "{B7E99515-BCBD-42D7-9808-F3AEDA2B5D05}");

        IAliasConfiguration() = default;
        virtual ~IAliasConfiguration() = default;

        //! Applies the stored AliasConfiguration (in AliasConfigurationStorage) to the active FileIOBase instance
        virtual void SetupAliases() = 0;

        //! Releases the cache lock, if we have one
        virtual void ReleaseCache() = 0;

        AZ_DISABLE_COPY_MOVE(IAliasConfiguration);
    };

    //! Stores the configured alias paths
    //! These can be set immediately at program start up, and will be applied to the FileIOBase instance once it becomes available
    struct AliasConfigurationStorage
    {
        //! Saves the configuration and attempts to apply the settings immediately
        static void SetConfig(AliasConfiguration config, bool clearExisting = false);

        static const AliasConfiguration& GetConfig();

        //! Resets the environment variable used to store the settings
        static void Reset();

    private:

        static AZ::EnvironmentVariable<AliasConfiguration>& GetVar();
    };

    inline void AliasConfigurationStorage::SetConfig(AliasConfiguration config, bool clearExisting)
    {
        if (clearExisting)
        {
            GetVar().Set(AZStd::move(config));
        }
        else
        {
            AliasConfiguration& existingConfig = GetVar().Get();

            // Helper function - checks if the provided member has a value on the new config, if so, the value is copied to the saved config
            auto updateSettingFunc = [&existingConfig, &config](std::string AliasConfiguration::* member)
            {
                if(!(config.*member).empty())
                {
                    (existingConfig.*member) = (config.*member);
                }
            };

            updateSettingFunc(&AliasConfiguration::m_rootPath);
            updateSettingFunc(&AliasConfiguration::m_assetsPath);
            updateSettingFunc(&AliasConfiguration::m_userPath);
            updateSettingFunc(&AliasConfiguration::m_logPath);
            updateSettingFunc(&AliasConfiguration::m_cachePath);
            updateSettingFunc(&AliasConfiguration::m_engineRootPath);
            updateSettingFunc(&AliasConfiguration::m_devRootPath);
            updateSettingFunc(&AliasConfiguration::m_devAssetsPath);
        }

        auto* aliasConfiguration = AZ::Interface<IAliasConfiguration>::Get();

        if(aliasConfiguration)
        {
            aliasConfiguration->SetupAliases();
        }
    }

    inline const AliasConfiguration& AliasConfigurationStorage::GetConfig()
    {
        return GetVar().Get();
    }

    inline void AliasConfigurationStorage::Reset()
    {
        GetVar().Reset();
    }

    inline AZ::EnvironmentVariable<AliasConfiguration>& AliasConfigurationStorage::GetVar()
    {
        static AZ::EnvironmentVariable<AliasConfiguration> s_config = AZ::Environment::CreateVariable<AliasConfiguration>("AliasConfiguration");

        return s_config;
    }
}
