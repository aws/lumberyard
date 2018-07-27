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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "pch.h"
#include "stdafx.h"
#include "platform.h"
#include "CafCompressionHelper.h"
#include "Shared/AnimSettings.h"

#include <ISystem.h>
#include <ICryPak.h>
#include "ResourceCompilerHelper.h"
#include <QtCore/QDir>
#include <Util/PathUtil.h>

static const char* ANIMATION_COMPRESSION_TEMP_ROOT = "Editor/Tmp/AnimationCompression/";

struct RCLogger
    : IResourceCompilerListener
{
    virtual void OnRCMessage(MessageSeverity severity, const char* text)
    {
        if (severity == MessageSeverity_Error)
        {
            gEnv->pLog->LogError("RC: %s", text);
        }
        else if (severity == MessageSeverity_Warning)
        {
            gEnv->pLog->LogWarning("RC: %s", text);
        }
        else
        {
            qDebug("RC: ");
            qDebug(text);
            qDebug("\n");
        }
    }
};

//////////////////////////////////////////////////////////////////////////
bool CafCompressionHelper::CompressAnimation(const string& animationPath, string& outErrorMessage, bool* failReported)
{
    const string inputFilePath = PathUtil::ReplaceExtension(animationPath, "i_caf");

    if (gEnv->pCryPak->IsFileExist(inputFilePath, ICryPak::eFileLocation_OnDisk) == false)
    {
        outErrorMessage.Format("Uncompressed animation '%s' doesn't exist on disk.", inputFilePath.c_str());
        if (failReported)
        {
            *failReported = true;
        }
        return false;
    }

    const string gameFolderPath = PathUtil::AddSlash(Path::GetEditingGameDataFolder().c_str());
    string additionalSettings;
    additionalSettings += " /animConfigFolder=Animations";
    additionalSettings += " /SkipDba=1";
    additionalSettings += " /sourceroot=\"" + gameFolderPath + "\"";
    ICVar* resourceCacheFolderCVar = gEnv->pSystem->GetIConsole()->GetCVar("sys_resource_cache_folder");
    if (resourceCacheFolderCVar)
    {
        additionalSettings += " /targetroot=\"" + string(resourceCacheFolderCVar->GetString()) + "\"";
    }
    additionalSettings += " /debugcompression";
    additionalSettings += " /refresh=1";

    RCLogger rcLogger;
    const bool mayShowWindow = true;

    const bool silent = false;
    const bool noUserDialog = false;

    qDebug("RC Call: ");
    qDebug(inputFilePath.c_str());
    qDebug(additionalSettings.c_str());
    qDebug("\n");

    CResourceCompilerHelper rcHelper;
    const CResourceCompilerHelper::ERcCallResult rcCallResult = rcHelper.CallResourceCompiler(
            inputFilePath,
            additionalSettings,
            &rcLogger,
            mayShowWindow,
            CResourceCompilerHelper::eRcExePath_currentFolder,
            silent,
            noUserDialog,
            L".");

    if (rcCallResult != CResourceCompilerHelper::eRcCallResult_success)
    {
        outErrorMessage.Format("Compression of '%s' by RC failed. (%s)", inputFilePath.c_str(), CResourceCompilerHelper::GetCallResultDescription(rcCallResult));
        // If rcCallResult == CResourceCompilerHelper::eRcCallResult_error,
        // the RC likely already logged the error, so tell the caller not to report
        // it again. In the other cases the RC wasn't able to run yet, so we do need
        // to report it.
        if (failReported && (rcCallResult == CResourceCompilerHelper::eRcCallResult_error))
        {
            *failReported = true;
        }
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CafCompressionHelper::CompressAnimationForPreview(string* outputCafPath, string* outErrorMessage, const string& animationPath, const SAnimSettings& animSettings, bool ignorePresets, int sessionIndex)
{
    if (!outErrorMessage)
    {
        return false;
    }

    const string inputFilePath = PathUtil::ReplaceExtension(animationPath, "i_caf"); //AnimSettingsFileHelper::GetIntermediateFilename(animationPath);
    const string inputFilePathFull = Path::GamePathToFullPath(animationPath.c_str()).toUtf8().data();

    if (animSettings.build.skeletonAlias.empty())
    {
        outErrorMessage->Format("Skeleton alias is not specified.");
        return false;
    }

    if (gEnv->pCryPak->IsFileExist(inputFilePathFull, ICryPak::eFileLocation_OnDisk) == false)
    {
        outErrorMessage->Format("Uncompressed animation '%s' doesn't exist on disk.", inputFilePath.c_str());
        return false;
    }

    string outputFilename;
    outputFilename.Format("%s%d/", ANIMATION_COMPRESSION_TEMP_ROOT, sessionIndex);
    if (!QDir().mkpath(QString::fromLocal8Bit(outputFilename.c_str())))
    {
        outErrorMessage->Format("Failed to create directory: \"%s\"", outputFilename.c_str());
        return false;
    }
    const string settingsFilename = outputFilename + "override.animsettings";

    if (!animSettings.SaveUsingFullPath(settingsFilename.c_str()))
    {
        outErrorMessage->Format("Failed to save %s", settingsFilename.c_str());
        QFile::remove(settingsFilename.c_str());
        return false;
    }

    const string outputDirectory = PathUtil::GetPath(outputFilename.c_str());
    outputFilename += animationPath;
    *outputCafPath = outputFilename;

    const string gameFolderPath = PathUtil::AddSlash(Path::GetEditingGameDataFolder().c_str());
    string additionalSettings;
    additionalSettings += " /animConfigFolder=Animations";
    additionalSettings += " /SkipDba=1";
    additionalSettings += " /sourceroot=\"" + gameFolderPath + "\"";
    additionalSettings += " /targetroot=\"" + outputDirectory + "\"";
    if (ignorePresets)
    {
        additionalSettings += " /ignorepresets=1";
    }
    additionalSettings += " /debugcompression";
    additionalSettings += " /refresh";
    additionalSettings += " /animSettingsFile=\"" + settingsFilename + "\"";

    RCLogger rcLogger;
    const bool mayShowWindow = true;

    const bool silent = false;
    const bool noUserDialog = false;

    qDebug("RC Call: ");
    qDebug(inputFilePath.c_str());
    qDebug(additionalSettings.c_str());
    qDebug("\n");

    CResourceCompilerHelper rcHelper;
    const CResourceCompilerHelper::ERcCallResult rcCallResult = rcHelper.CallResourceCompiler(
            inputFilePath,
            additionalSettings,
            &rcLogger,
            mayShowWindow,
            CResourceCompilerHelper::eRcExePath_currentFolder,
            silent,
            noUserDialog,
            L".");


    bool succeed = rcCallResult == CResourceCompilerHelper::eRcCallResult_success;
    QFile::remove(settingsFilename.c_str());
    string tempAnimSettingsFilename = PathUtil::ReplaceExtension(outputFilename, "$animsettings");
    QFile::remove(QString::fromLocal8Bit(tempAnimSettingsFilename.c_str()));


    if (succeed)
    {
        return true;
    }
    else
    {
        outErrorMessage->Format("Compression of '%s' by RC failed. (%s)", PathUtil::GetFile(inputFilePath.c_str()), CResourceCompilerHelper::GetCallResultDescription(rcCallResult));
        return false;
    }
}

static bool GetFileTime(__time64_t* filetime, const char* filename)
{
    _finddata_t FindFileData;
    const intptr_t hFind = gEnv->pCryPak->FindFirst(filename, &FindFileData, 0);

    if (hFind < 0)
    {
        return false;
    }

    gEnv->pCryPak->FindClose(hFind);

    if (filetime)
    {
        *filetime = FindFileData.time_write;
    }
    return true;
}

static bool ModificationTimeEqual(const char* filename1, const char* filename2)
{
    __time64_t time1;
    if (!GetFileTime(&time1, filename1))
    {
        return false;
    }

    __time64_t time2;
    if (!GetFileTime(&time2, filename2))
    {
        return false;
    }

    return time1 == time2;
}

static string AnimationNameToPath(const char* animationName)
{
    return string(Path::GetEditingGameDataFolder().c_str()) + "/" + animationName;
}

bool CafCompressionHelper::CheckIfNeedUpdate(const char* animationName)
{
    string intermediatePath = PathUtil::ReplaceExtension(animationName, "i_caf");
    if (!gEnv->pCryPak->IsFileExist(intermediatePath, ICryPak::eFileLocation_OnDisk))
    {
        return false;
    }

    string settingsPath = PathUtil::ReplaceExtension(animationName, "animsettings");
    string settingsDollarPath = PathUtil::ReplaceExtension(animationName, "$animsettings");
    if (!gEnv->pCryPak->IsFileExist(settingsPath, ICryPak::eFileLocation_OnDisk))
    {
        return false;
    }
    if (!gEnv->pCryPak->IsFileExist(settingsDollarPath, ICryPak::eFileLocation_OnDisk))
    {
        return true;
    }

    string animationPath = PathUtil::ReplaceExtension(intermediatePath.c_str(), "caf");
    if (!gEnv->pCryPak->IsFileExist(animationPath, ICryPak::eFileLocation_OnDisk))
    {
        return true;
    }

    if (!ModificationTimeEqual(intermediatePath.c_str(), animationPath.c_str()))
    {
        return true;
    }

    if (!ModificationTimeEqual(settingsPath.c_str(), settingsDollarPath.c_str()))
    {
        return true;
    }

    return false;
}

bool CafCompressionHelper::RemoveCompressionTarget(string* outErrorMessage, const char* destination)
{
    const string gameFolderPath = PathUtil::AddSlash(Path::GetEditingGameDataFolder().c_str());
    const string compressionPreviewFile = gameFolderPath + destination;
    if (QFile::exists(compressionPreviewFile.c_str()) && !QFile::remove(compressionPreviewFile.c_str()))
    {
        outErrorMessage->Format("Failed to remove existing file: %s", compressionPreviewFile.c_str());
        return false;
    }
    return true;
}

bool CafCompressionHelper::MoveCompressionResult(string* outErrorMessage, const char* createdFile, const char* destination)
{
    if (createdFile[0] == '\0')
    {
        return false;
    }
    if (!QFile::exists(createdFile))
    {
        outErrorMessage->Format("Compiled animation was not created by RC: \"%s\"", createdFile);
        return false;
    }

    const string gameFolderPath = PathUtil::AddSlash(Path::GetEditingGameDataFolder().c_str());
    const string compressionPreviewFile = gameFolderPath + destination;
    string path;
    string filename;
    PathUtil::Split(compressionPreviewFile, path, filename);
    path = PathUtil::RemoveSlash(path);
    const bool createPathSuccess = QDir().mkpath(path.c_str());
    if (!createPathSuccess)
    {
        outErrorMessage->Format("Cannot create the path of '%s' for the compression preview file.", path.c_str());
        return false;
    }

    string dest = path + "/" + filename;
    if (!QFile::rename(createdFile, dest.c_str()))
    {
        outErrorMessage->Format("Unable to move compiled animation %s to %s", createdFile, path.c_str());
        return false;
    }

    return true;
}

void CafCompressionHelper::CleanUpCompressionResult(const char* createdFile)
{
    string createdFolder;
    string filename;
    PathUtil::Split(createdFile, createdFolder, filename);
    if (createdFile[0] != '\0')
    {
        QFile::remove(createdFile);
    }

    // remove all empty folders inside ANIMATION_COMPRESSION_TEMP_ROOT
    string path = createdFolder;
    while (!path.empty() && azstricmp(path.c_str(), ANIMATION_COMPRESSION_TEMP_ROOT) != 0)
    {
        path.TrimRight("\\/");
        if (!QDir().rmdir(path.c_str()))
        {
            break;
        }
        PathUtil::Split(path, createdFolder, filename);
        path = createdFolder;
    }
}
