/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

#include "CloudGemDefectReporter_precompiled.h"

#include <Traceroute.h>
#include <RunExternalCmd_windows.h>
#include "DefectReportManager.h"
#include <AzFramework/FileFunc/FileFunc.h>
#include <AzCore/Math/Uuid.h>
#include <ISystem.h>

#include <windows.h>

namespace CloudGemDefectReporter
{
    #define TRACEROUTE_TEMP_FILE_DIR "@user@/CloudCanvas/CloudGemDefectReporter/TracerouteTemp"

    AZStd::string Traceroute::GetResult(const AZStd::string& domainNameOrIp)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();

        // try to create temp file directory if not exists
        {
            if (!fileIO->Exists(TRACEROUTE_TEMP_FILE_DIR))
            {
                if (!fileIO->CreatePath(TRACEROUTE_TEMP_FILE_DIR))
                {
                    AZ_Warning("CloudCanvas", false, "Failed to create temp traceroute directory for defect reporter");
                    return "";
                }
            }
        }

        // get temp file path
        char resolvedTempFilePath[AZ_MAX_PATH_LEN] = { 0 };
        {
            AZStd::string uniqueFileName;
            {
                AZ::Uuid uuid = AZ::Uuid::Create();
                uuid.ToString(uniqueFileName);
            }

            AZStd::string tempFilePath = AZStd::string::format("%s/%s", TRACEROUTE_TEMP_FILE_DIR, uniqueFileName.c_str());

            fileIO->ResolvePath(tempFilePath.c_str(), resolvedTempFilePath, AZ_MAX_PATH_LEN);
        }

        // run the command and output to temp file
        {
            bool success = RunExternalCmd::Run("tracert", domainNameOrIp, resolvedTempFilePath, m_timeoutMilliSeconds);
            if (!success)
            {
                AZ_Warning("CloudCanvas", false, "Failed to run tracert command for defect reporter");
                fileIO->Remove(resolvedTempFilePath);
                return "";
            }
        }

        AZ::IO::HandleType fileHandle;
        if (!fileIO->Open(resolvedTempFilePath, AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeText, fileHandle))
        {
            AZ_Warning("CloudCanvas", false, "Failed to open temp traceroute file for defect reporter");
            fileIO->Remove(resolvedTempFilePath);
            return "";
        }

        AZ::u64 fileSize = 0;
        if (!fileIO->Size(fileHandle, fileSize))
        {
            AZ_Warning("CloudCanvas", false, "Failed to open temp traceroute file for defect reporter");
            fileIO->Remove(resolvedTempFilePath);
            return "";
        }

        AZStd::string output;
        output.resize(fileSize);

        fileIO->Read(fileHandle, &output[0], fileSize);
        fileIO->Close(fileHandle);

        // remove temp file
        AZ::IO::FileIOBase* newFileIO = AZ::IO::FileIOBase::GetDirectInstance();
        if (newFileIO)
        {
            newFileIO->Remove(resolvedTempFilePath);
        }

        return output;
    }
}