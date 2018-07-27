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

#include "stdafx.h"

// CommandLauncher runs synchronous, asynchronous, and detached processes using QProcess
class CommandLauncher
{
public:
    CommandLauncher() { }
    CommandLauncher(const CommandLauncher& rhs) = delete;
    CommandLauncher& operator=(const CommandLauncher& rhs) = delete;
    CommandLauncher& operator=(CommandLauncher&& rhs) = delete;
    ~CommandLauncher() { }

    // run command in shared console and wait for exit
    bool SyncProcess(const char* cmd, QProcess* process)
    {
        bool normalExit = true;
        QObject::connect(process,
            static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), // use static_cast to specify which overload of QProcess::finished to use
            process,
            [&](int exitCode, QProcess::ExitStatus exitStatus)
        {
            if (exitStatus != QProcess::NormalExit || exitCode != 0)
                normalExit = false;
        });

        process->start(QString(cmd));

        if (!process->waitForStarted())
        {
            return false;
        }
        if (!process->waitForFinished())
        {
            return false;
        }

        return normalExit;
    }

    // run command in shared console asynchronously
    bool AsyncProcess(const char* cmd, QProcess* process)
    {
        process->start(QString(cmd));
        return process->waitForStarted();
    }

    // run and detach command
    bool DetachProcess(const char* cmd)
    {
        return QProcess::startDetached(cmd);
    }


    bool DetachProcess(const char* cmd, const char* workingDir)
    {
        QStringList emptyArgList;
        return QProcess::startDetached(cmd, emptyArgList, workingDir);
    }
};
