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
#include "EditorPreferencesPageFiles.h"


void CEditorPreferencesPage_Files::Reflect(AZ::SerializeContext& serialize)
{
    serialize.Class<Files>()
        ->Version(1)
        ->Field("BackupOnSave", &Files::m_backupOnSave)
        ->Field("BackupOnSaveMaxCount", &Files::m_backupOnSaveMaxCount)
        ->Field("TempDirectory", &Files::m_standardTempDirectory)
        ->Field("AutoSaveTagPoints", &Files::m_autoSaveTagPoints);

    serialize.Class<ExternalEditors>()
        ->Version(1)
        ->Field("Scripts", &ExternalEditors::m_scripts)
        ->Field("Shaders", &ExternalEditors::m_shaders)
        ->Field("BSpaces", &ExternalEditors::m_BSpaces)
        ->Field("Textures", &ExternalEditors::m_textures)
        ->Field("Animations", &ExternalEditors::m_animations);

    serialize.Class<AutoBackup>()
        ->Version(1)
        ->Field("Enabled", &AutoBackup::m_enabled)
        ->Field("Interval", &AutoBackup::m_timeInterval)
        ->Field("MaxCount", &AutoBackup::m_maxCount)
        ->Field("RemindTime", &AutoBackup::m_remindTime);

    serialize.Class<CEditorPreferencesPage_Files>()
        ->Version(1)
        ->Field("Files", &CEditorPreferencesPage_Files::m_files)
        ->Field("Editors", &CEditorPreferencesPage_Files::m_editors)
        ->Field("AutoBackup", &CEditorPreferencesPage_Files::m_autoBackup);


    AZ::EditContext* editContext = serialize.GetEditContext();
    if (editContext)
    {
        editContext->Class<Files>("Files", "File Preferences")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Files::m_backupOnSave, "Backup on Save", "Backup on Save")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &Files::m_backupOnSaveMaxCount, "Maximum Save Backups", "Maximum Save Backups")
            ->Attribute(AZ::Edit::Attributes::Min, 1)
            ->Attribute(AZ::Edit::Attributes::Max, 100)
            ->DataElement(AZ::Edit::UIHandlers::LineEdit, &Files::m_standardTempDirectory, "Standard Temporary Directory", "Standard Temporary Directory")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Files::m_autoSaveTagPoints, "Auto Save Camera Tag Points", "Instantly Save Changed Camera Tag Points");

        editContext->Class<ExternalEditors>("External Editors", "External Editors")
            ->DataElement(AZ::Edit::UIHandlers::LineEdit, &ExternalEditors::m_scripts, "Scripts Editor", "Scripts Text Editor")
            ->DataElement(AZ::Edit::UIHandlers::LineEdit, &ExternalEditors::m_shaders, "Shaders Editor", "Shaders Text Editor")
            ->DataElement(AZ::Edit::UIHandlers::LineEdit, &ExternalEditors::m_BSpaces, "BSpace Editor", "Bspace Text Editor")
            ->DataElement(AZ::Edit::UIHandlers::LineEdit, &ExternalEditors::m_textures, "Texture Editor", "Texture Editor")
            ->DataElement(AZ::Edit::UIHandlers::LineEdit, &ExternalEditors::m_animations, "Animation Editor", "Animation Editor");

        editContext->Class<AutoBackup>("Auto Backup", "Auto Backup")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &AutoBackup::m_enabled, "Enable", "Enable Auto Backup")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &AutoBackup::m_timeInterval, "Time Interval", "Auto Backup Interval (Minutes)")
            ->Attribute(AZ::Edit::Attributes::Min, 2)
            ->Attribute(AZ::Edit::Attributes::Max, 10000)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &AutoBackup::m_maxCount, "Maximum Backups", "Maximum Auto Backups")
            ->Attribute(AZ::Edit::Attributes::Min, 1)
            ->Attribute(AZ::Edit::Attributes::Max, 100)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &AutoBackup::m_remindTime, "Remind Time", "Auto Remind Every (Minutes)");

        editContext->Class<CEditorPreferencesPage_Files>("File Preferences", "Class for handling File Preferences")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_Files::m_files, "Files", "File Preferences")
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_Files::m_editors, "External Editors", "External Editors")
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_Files::m_autoBackup, "Auto Backup", "Auto Backup");
    }
}


CEditorPreferencesPage_Files::CEditorPreferencesPage_Files()
{
    InitializeSettings();
}

void CEditorPreferencesPage_Files::OnApply()
{
    gSettings.bBackupOnSave = m_files.m_backupOnSave;
    gSettings.backupOnSaveMaxCount = m_files.m_backupOnSaveMaxCount;
    gSettings.bAutoSaveTagPoints = m_files.m_autoSaveTagPoints;
    gSettings.strStandardTempDirectory = m_files.m_standardTempDirectory.c_str();

    gSettings.textEditorForScript = m_editors.m_scripts.c_str();
    gSettings.textEditorForShaders = m_editors.m_shaders.c_str();
    gSettings.textEditorForBspaces = m_editors.m_BSpaces.c_str();
    gSettings.textureEditor = m_editors.m_textures.c_str();
    gSettings.animEditor = m_editors.m_animations.c_str();

    gSettings.autoBackupEnabled = m_autoBackup.m_enabled;
    gSettings.autoBackupTime = m_autoBackup.m_timeInterval;
    gSettings.autoBackupMaxCount = m_autoBackup.m_maxCount;
    gSettings.autoRemindTime = m_autoBackup.m_remindTime;
}

void CEditorPreferencesPage_Files::InitializeSettings()
{
    m_files.m_backupOnSave = gSettings.bBackupOnSave;
    m_files.m_backupOnSaveMaxCount = gSettings.backupOnSaveMaxCount;
    m_files.m_autoSaveTagPoints = gSettings.bAutoSaveTagPoints;
    m_files.m_standardTempDirectory = gSettings.strStandardTempDirectory.toUtf8().data();

    m_editors.m_scripts = gSettings.textEditorForScript.toUtf8().data();
    m_editors.m_shaders = gSettings.textEditorForShaders.toUtf8().data();
    m_editors.m_BSpaces = gSettings.textEditorForBspaces.toUtf8().data();
    m_editors.m_textures = gSettings.textureEditor.toUtf8().data();
    m_editors.m_animations = gSettings.animEditor.toUtf8().data();

    m_autoBackup.m_enabled = gSettings.autoBackupEnabled;
    m_autoBackup.m_timeInterval = gSettings.autoBackupTime;
    m_autoBackup.m_maxCount = gSettings.autoBackupMaxCount;
    m_autoBackup.m_remindTime = gSettings.autoRemindTime;
}