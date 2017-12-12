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

#include <AzCore/Math/Uuid.h>

#include <AzCore/std/string/string.h>

namespace Projects
{
    /**
     * Unique identifier for projects.
     */
    using ProjectId = AZ::Uuid;

    /**
     * Unique identifier for a ProjectPlatform.
     * \example "windows", "mobile", "ios", "console"
     * ProjectPlatform is one factor in a ProjectFlavor.
     */
    using ProjectPlatformId = AZStd::string;

    /**
     * Unique identifier for a ProjectConfiguration.
     * \example "development", "final", "server"
     * ProjectConfiguration is one factor in a ProjectFlavor.
     */
    using ProjectConfigurationId = AZStd::string;

    /**
     * Represents a specific project "flavor".
     * Each flavor of the project can be configured independently.
     * A project should customize the number of unique flavors by adding
     * or removing ProjectPlatforms and ProjectConfigurations.
     * Note that ProjectPlatformId and ProjectConfigurationId
     * are distinct from BuildPlatformId and BuildConfigurationId.
     */
    struct ProjectFlavor
    {
        ProjectId m_project;
        ProjectPlatformId m_platform;
        ProjectConfigurationId m_configuration;
    };

    /**
    * BuildPlatform is the platform name as known to the build system.
    * \example "win_x64", "win_x64_vs2012", "android_armv7_gcc"
    */
    using BuildPlatformId = AZStd::string;

    /**
    * BuildConfiguration is the configuration name as known to the build system.
    * \example "release", "profile", "performance_dedicated"
    */
    using BuildConfigurationId = AZStd::string;
}