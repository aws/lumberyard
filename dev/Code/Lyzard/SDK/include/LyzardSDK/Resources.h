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

namespace Lyzard
{
    // SetupAssistant Configuration
    constexpr const char* SetupConfigFilename = "SetupAssistantConfig.json";
    constexpr const char* SetupUserPrefsFilename = "SetupAssistantUserPreferences.ini";
    constexpr const char* ThirdPartySearchPathKey = "SDKSearchPath3rdParty";

    // Tools
    constexpr const char* LmbrSetupToolsDir = "Tools/LmbrSetup";
    constexpr const char* LmbrExeName = "lmbr";

    // Build Settings
    constexpr const char* BuildSettingsDir = "_WAF_";
    constexpr const char* BuildSettingsFilename = "user_settings.options";

    // Engine Config
    constexpr const char* EngineRootFilename = "engineroot.txt";
    constexpr const char* EngineConfigFilename = "engine.json";

} // namespace Lyzard

#define SETUP_CONFIG_FILENAME           (Lyzard::SetupConfigFilename)
#define SETUP_USER_PREFS_FILE           (Lyzard::SetupUserPrefsFilename)
#define THIRD_PARTY_SEARCH_PATH_KEY     (Lyzard::ThirdPartySearchPathKey)
#define LMBR_SETUP_TOOLS_DIR            (Lyzard::LmbrSetupToolsDir)
#define LMBR_EXE_NAME                   (Lyzard::LmbrExeName)
#define BUILD_SETTINGS_DIR              (Lyzard::BuildSettingsDir)
#define BUILD_SETTINGS_FILE             (Lyzard::BuildSettingsFilename)
#define ENGINE_ROOT_FILE                (Lyzard::EngineRootFilename)
#define ENGINE_CONFIG_FILE              (Lyzard::EngineConfigFilename)

