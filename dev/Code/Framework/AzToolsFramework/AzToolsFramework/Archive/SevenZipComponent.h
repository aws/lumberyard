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

#include <AzCore/base.h>
#include <AzCore/Component/Component.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/conditional_variable.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_set.h>

#include <AzToolsFramework/Archive/ArchiveAPI.h>

namespace AzToolsFramework
{
    enum class SevenZipExitCode : AZ::u32
    {
        NoError = 0,
        Warning = 1,
        FatalError = 2,
        CommandLineError = 7,
        NotEnoughMemory = 8,
        UserStoppedProcess = 255
    };

    // the SevenZip's job is to execute 7z commands
    // it parses the status of 7z commands and returns results.
    class SevenZipComponent
        : public AZ::Component
        , private ArchiveCommands::Bus::Handler
    {
    public:
        AZ_COMPONENT(SevenZipComponent, "{A19EEA33-3736-447F-ACF7-DAA4B6A179AA}")

        SevenZipComponent() = default;
        ~SevenZipComponent() override = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component overrides
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////
    private:
#if defined(AZ_COMPILER_MSVC) && AZ_COMPILER_MSVC <= 1800
        // Workaround for VS2013 - Delete the copy constructor and make it private
        // https://connect.microsoft.com/VisualStudio/feedback/details/800328/std-is-copy-constructible-is-broken
        SevenZipComponent(const SevenZipComponent &) = delete;
#endif
        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // ArchiveCommands::Bus::Handler overrides
        void CreateArchive(const AZStd::string& archivePath, const AZStd::string& dirToArchive, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback) override;
        bool CreateArchiveBlocking(const AZStd::string& archivePath, const AZStd::string& dirToArchive) override;
        bool ExtractArchiveBlocking(const AZStd::string& archivePath, const AZStd::string& destinationPath, bool extractWithRootDirectory) override;
        void ExtractArchive(const AZStd::string& archivePath, const AZStd::string& destinationPath, AZ::Uuid taskHandle, const ArchiveResponseCallback& respCallback) override;
        void ExtractArchiveOutput(const AZStd::string& archivePath, const AZStd::string& destinationPath, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback) override;
        void ExtractArchiveWithoutRoot(const AZStd::string& archivePath, const AZStd::string& destinationPath, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback) override;
        void CancelTasks(AZ::Uuid taskHandle) override;
        //////////////////////////////////////////////////////////////////////////

        // Launches the 7za.exe as a background child process in a detached background thread, if the task handle is not null 
        // otherwise launches 7za.exe in the calling thread.
        void Launch7z(const AZStd::string& commandLineArgs, const ArchiveResponseOutputCallback& respCallback, AZ::Uuid taskHandle = AZ::Uuid::CreateNull(), const AZStd::string& workingDir = "", bool captureOutput = false);
        
        AZStd::string m_7zPath; // Path to Dev/Tools/7za.exe

        // Struct for tracking background threads/tasks
        struct ThreadInfo
        {
            bool shouldStop = false;
            AZStd::set<AZStd::thread::id> threads;
        };

        AZStd::mutex m_threadControlMutex; // Guards m_threadInfoMap
        AZStd::condition_variable m_cv;
        AZStd::unordered_map<AZ::Uuid, ThreadInfo> m_threadInfoMap;
    };
} // namespace AzToolsFramework
