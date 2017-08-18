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

#include "stdafx.h"
#include "FFMPEGPlugin.h"
#include "CryString.h"
typedef CryStringT<char> string;
#include "platform_impl.h"
#include "Include/ICommandManager.h"
#include "Util/PathUtil.h"

namespace PluginInfo
{
    const char* kName = "FFMPEG Writer";
    const char* kGUID = "{D2A3A44A-00FF-4341-90BA-89A473F44A65}";
    const int kVersion = 1;
}

void CFFMPEGPlugin::Release()
{
    GetIEditor()->GetICommandManager()->UnregisterCommand(COMMAND_MODULE, COMMAND_NAME);
    delete this;
}

void CFFMPEGPlugin::ShowAbout()
{
}

const char* CFFMPEGPlugin::GetPluginGUID()
{
    return PluginInfo::kGUID;
}

DWORD CFFMPEGPlugin::GetPluginVersion()
{
    return PluginInfo::kVersion;
}

const char* CFFMPEGPlugin::GetPluginName()
{
    return PluginInfo::kName;
}

bool CFFMPEGPlugin::CanExitNow()
{
    return true;
}

static void Command_FFMPEGEncode(const char* input, const char* output, const char* codec, int bitRateinKb, int fps, const char* etc)
{
    // the FFMPEG exe could be in a number of possible locations
    // our preferred location is from the path in the registry @
    // "Software\\Amazon\\Lumberyard\\Settings"
    // in a value called "FFMPEG_PLUGIN"

    // if its not there, try the current folder and a few others
    // see what can be found!

    QString ffmpegEXEPath;
    QString outTxt;

    HKEY hKey = nullptr;
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Amazon\\Lumberyard\\Settings", 0, KEY_READ, &hKey);
    if (result == ERROR_SUCCESS)
    {
        unsigned long type = REG_SZ, size = 1024;
        char buffer[1024];
        memset(buffer, '\0', sizeof(buffer));
        size = 1024;
        if (RegQueryValueEx(hKey, "FFMPEG_PLUGIN", NULL, &type, (LPBYTE)&buffer[0], &size) == ERROR_SUCCESS)
        {
            if (::GetFileAttributes(buffer) != INVALID_FILE_ATTRIBUTES)
            {
                ffmpegEXEPath = buffer;
            }
        }

        RegCloseKey(hKey);
    }

    if (ffmpegEXEPath.isEmpty())
    {
        char dllPath[_MAX_PATH];
        GetModuleFileName(GetModuleHandle(NULL), dllPath, sizeof(dllPath));
        char buffer[_MAX_PATH];
        char drive[_MAX_DRIVE];
        char dir[_MAX_DIR];
        _splitpath(dllPath, drive, dir, 0, 0);
        _makepath(buffer, drive, dir, 0, 0);
        // buffer now contains the path to the editor exe.
        QString checkPath;

        const char* possibleLocations[] = {
            "rc\\ffmpeg.exe",
            "editorplugins\\ffmpeg.exe",
            "..\\editor\\plugins\\ffmpeg.exe",
        };

        for (int idx = 0; idx < 3; ++idx)
        {
            checkPath = QStringLiteral("%s%s").arg(buffer, possibleLocations[idx]);
            if (GetFileAttributes(checkPath.toLatin1().data()) != INVALID_FILE_ATTRIBUTES)
            {
                ffmpegEXEPath = checkPath;
                break;
            }
        }

        if (ffmpegEXEPath.isEmpty())
        {
            ffmpegEXEPath = "ffmpeg.exe";
            // try the current folder if all else fails, this will also work if its in the PATH somehow.
        }
    }

    QString ffmpegCmdLine = QStringLiteral("\"%1\" -r %2 -i \"%3\" -vcodec %4 -b %5k -r %6 %7 -strict experimental -y \"%8\"")
        .arg(ffmpegEXEPath, QString::number(fps), input, codec, QString::number(bitRateinKb), QString::number(fps), etc, output);
    GetIEditor()->GetSystem()->GetILog()->Log("Executing \"%s\" from FFMPEGPlugin...", ffmpegCmdLine.toLatin1().data());
    GetIEditor()->ExecuteConsoleApp(ffmpegCmdLine, outTxt, true, true);
    GetIEditor()->GetSystem()->GetILog()->Log("FFMPEG execution done.");
}

void CFFMPEGPlugin::RegisterTheCommand()
{
    CommandManagerHelper::RegisterCommand(GetIEditor()->GetICommandManager(),
        COMMAND_MODULE, COMMAND_NAME, "Encodes a video using ffmpeg.",
        "plugin.ffmpeg_encode 'input.avi' 'result.mov' 'libx264' 200 30",
        functor(Command_FFMPEGEncode));
}

void CFFMPEGPlugin::OnEditorNotify(EEditorNotifyEvent aEventId)
{
}