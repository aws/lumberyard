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

#include <AzCore/Socket/AzSocket_fwd.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/string/string.h>


namespace DeployTool
{
    class RemoteLog
    {
    public:
        RemoteLog();
        ~RemoteLog();

        void Start(const AZStd::string& deviceIpAddress, AZ::u16 port, bool loadCurrentLevel);
        void Stop();

    private:

        void Run();

        bool Send(const char* buffer, int size);
        bool Receive(char* buffer, int& size);

        void TrySleepThread();


        AZStd::thread m_worker;

        AZStd::string m_ipAddress;
        AZ::u16 m_port;

        AZSOCKET m_socket;

        bool m_loadCurrentLevel;

        bool m_isRunning;
        volatile bool m_stopRequested;
    };
} // namespace DeployTool

