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
#include "stdafx.h"
#include "ExportSettings.h"

#include <QFile>
#include <QTextStream>

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Component/TickBus.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

namespace
{
    QString getExportSettingsFileName(const char* filename)
    {
        QString exportSettingsFilename = filename;
        exportSettingsFilename.append(".exportsettings");
        return exportSettingsFilename;
    }
}

bool ImageExportSettings::LoadSettings(const char* filename, string& settings)
{
    QFile exportSettingsFile(getExportSettingsFileName(filename));
    if (!exportSettingsFile.open(QFile::ReadOnly | QFile::Text))
    {
        return false;
    }

    QTextStream inStream(&exportSettingsFile);
    QString inSettings = inStream.readAll();
    settings = inSettings.toStdString().c_str();

    exportSettingsFile.close();
    return true;
}

bool ImageExportSettings::SaveSettings(const char* filename, const string& settings)
{
    // If no old settings and new settings are empty, don't write them out
    string oldSettings;
    if (!LoadSettings(filename, oldSettings) && settings.empty())
    {
        return true;
    }

    //check if the settings changed
    bool bSettingsChanged = oldSettings.compare(settings) != 0;

    QString exportSettingFileName = getExportSettingsFileName(filename);

    //if the settings changed if not return
    if (!bSettingsChanged)
    {
        return true;
    }

    auto settingsFile = exportSettingFileName.toUtf8();
    CheckSourceControl(settingsFile.data());

    //write the settings to the export settings file
    QFile exportSettingsFile(exportSettingFileName);
    if (!exportSettingsFile.open(QFile::WriteOnly | QFile::Text))
    {
        // Unable to open for write
        return false;
    }

    QTextStream outStream(&exportSettingsFile);
    outStream << settings.c_str();

    exportSettingsFile.close();
    return true;
}

void ImageExportSettings::CheckSourceControl(const char* filename)
{
    // if the file exists and is writable, don't do anything.
    AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
    if (!AZ::IO::SystemFile::Exists(filename) || fileIO->IsReadOnly(filename))
    {
        CheckoutFile(filename);
    }
}

bool ImageExportSettings::CheckoutFile(const char* filename)
{
    using SCCommandBus = AzToolsFramework::SourceControlCommandBus;

    bool opSuccess = false;
    bool opComplete = false;
    SCCommandBus::Broadcast(&SCCommandBus::Events::RequestEdit, filename, true,
        [&opSuccess, &opComplete](bool success, const AzToolsFramework::SourceControlFileInfo&)
        {
            opSuccess = success;
            opComplete = true;
        }
    );

    while (!opComplete)
    {
        AZStd::this_thread::yield();
        AZ::TickBus::ExecuteQueuedEvents();
    }

    return opSuccess;
}

