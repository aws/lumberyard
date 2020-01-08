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

#include "StdAfx.h"
#include "DisplaySettings.h"
#include "DisplaySettingsPythonFuncs.h"
#include "Util/BoostPythonHelpers.h"
#include <AzCore/RTTI/BehaviorContext.h>

namespace
{
    int PyGetMiscDisplaySettings()
    {
        return GetIEditor()->GetDisplaySettings()->GetSettings();
    }

    void PySetMiscDisplaySettings(int flags)
    {
        GetIEditor()->GetDisplaySettings()->SetSettings(flags);
    }
}

namespace AzToolsFramework
{
    void DisplaySettingsPythonFuncsHandler::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // this will put these methods into the 'azlmbr.legacy.settings' module
            auto addLegacyGeneral = [](AZ::BehaviorContext::GlobalMethodBuilder methodBuilder)
            {
                methodBuilder->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Legacy/Settings")
                    ->Attribute(AZ::Script::Attributes::Module, "legacy.settings");
            };
            addLegacyGeneral(behaviorContext->Method("get_misc_editor_settings", PyGetMiscDisplaySettings, nullptr, "Get miscellaneous Editor settings (camera collides with terrain, AI/Physics enabled, etc)."));
            addLegacyGeneral(behaviorContext->Method("set_misc_editor_settings", PySetMiscDisplaySettings, nullptr, "Set miscellaneous Editor settings (camera collides with terrain, AI/Physics enabled, etc)."));
            
            behaviorContext->EnumProperty<EDisplaySettingsFlags::SETTINGS_NOCOLLISION>("DisplaySettings_NoCollision")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            behaviorContext->EnumProperty<EDisplaySettingsFlags::SETTINGS_NOLABELS>("DisplaySettings_NoLabels")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            behaviorContext->EnumProperty<EDisplaySettingsFlags::SETTINGS_PHYSICS>("DisplaySettings_Physics")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            behaviorContext->EnumProperty<EDisplaySettingsFlags::SETTINGS_HIDE_TRACKS>("DisplaySettings_HideTracks")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            behaviorContext->EnumProperty<EDisplaySettingsFlags::SETTINGS_HIDE_LINKS>("DisplaySettings_HideLinks")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            behaviorContext->EnumProperty<EDisplaySettingsFlags::SETTINGS_HIDE_HELPERS>("DisplaySettings_HideHelpers")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            behaviorContext->EnumProperty<EDisplaySettingsFlags::SETTINGS_SHOW_DIMENSIONFIGURES>("DisplaySettings_ShowDimensionFigures")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            behaviorContext->EnumProperty<EDisplaySettingsFlags::SETTINGS_SERIALIZABLE_FLAGS_MASK>("DisplaySettings_SerializableFlagsMask")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
        }
    }
}

REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyGetMiscDisplaySettings, settings, get_misc_editor_settings,
    "Get miscellaneous Editor settings (camera collides with terrain, AI/Physics enabled, etc).",
    "settings.get_misc_editor_settings()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PySetMiscDisplaySettings, settings, set_misc_editor_settings,
    "Set miscellaneous Editor settings (camera collides with terrain, AI/Physics enabled, etc).",
    "settings.set_misc_editor_settings()");
