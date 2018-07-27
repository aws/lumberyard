
#include "CloudGemDefectReporter_precompiled.h"

#include <NSLookup.h>
#include <RunExternalCmd_windows.h>
#include "DefectReportManager.h"
#include <AzFramework/FileFunc/FileFunc.h>
#include <AzCore/Math/Uuid.h>
#include <ISystem.h>

#include <windows.h>

namespace CloudGemDefectReporter
{
    #define NSLOOKUP_TEMP_FILE_DIR "@user@/CloudCanvas/CloudGemDefectReporter/NSLookupTemp"

    AZStd::string NSLookup::GetResult(const AZStd::string& domainName)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();

        // try to create temp file directory if not exists
        {            
            if (!fileIO->Exists(NSLOOKUP_TEMP_FILE_DIR))
            {
                if (!fileIO->CreatePath(NSLOOKUP_TEMP_FILE_DIR))
                {
                    AZ_Warning("CloudCanvas", false, "Failed to create temp nslookup directory for defect reporter");
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

            AZStd::string tempFilePath = AZStd::string::format("%s/%s", NSLOOKUP_TEMP_FILE_DIR, uniqueFileName.c_str());

            fileIO->ResolvePath(tempFilePath.c_str(), resolvedTempFilePath, AZ_MAX_PATH_LEN);
        }                

        // run the command and output to temp file
        {
            bool success = RunExternalCmd::Run("nslookup", domainName, resolvedTempFilePath, m_timeoutMilliSeconds);
            if (!success)
            {
                AZ_Warning("CloudCanvas", false, "Failed to run nslookup command for defect reporter");
                fileIO->Remove(resolvedTempFilePath);
                return "";
            }
        }

        AZ::IO::HandleType fileHandle;
        if (!fileIO->Open(resolvedTempFilePath, AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeText, fileHandle))
        {
            AZ_Warning("CloudCanvas", false, "Failed to open temp nslookup file for defect reporter");
            fileIO->Remove(resolvedTempFilePath);
            return "";
        }

        AZ::u64 fileSize = 0;
        if (!fileIO->Size(fileHandle, fileSize))
        {
            AZ_Warning("CloudCanvas", false, "Failed to open temp nslookup file for defect reporter");
            fileIO->Remove(resolvedTempFilePath);
            return "";
        }

        AZStd::string output;
        output.resize(fileSize);

        fileIO->Read(fileHandle, &output[0], fileSize);
        fileIO->Close(fileHandle);

        // remove temp file
        fileIO->Remove(resolvedTempFilePath);

        return output;
    }
}