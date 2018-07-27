
#include "CloudGemDefectReporter_precompiled.h"

#include <DxDiag.h>
#include <RunExternalCmd_windows.h>
#include "DefectReportManager.h"
#include <AzFramework/FileFunc/FileFunc.h>
#include <AzCore/Math/Uuid.h>
#include <ISystem.h>

#include <windows.h>

namespace CloudGemDefectReporter
{
    #define DXDIAG_TEMP_FILE_DIR "@user@/CloudCanvas/CloudGemDefectReporter/DxDiag"

    AZStd::string DxDiag::GetResult()
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetDirectInstance();

        // try to create temp file directory if not exists
        {            
            if (!fileIO->Exists(DXDIAG_TEMP_FILE_DIR))
            {
                if (!fileIO->CreatePath(DXDIAG_TEMP_FILE_DIR))
                {
                    AZ_Warning("CloudCanvas", false, "Failed to create temp dxdiag directory for defect reporter");
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

            AZStd::string tempFilePath = AZStd::string::format("%s/%s.txt", DXDIAG_TEMP_FILE_DIR, uniqueFileName.c_str());

            fileIO->ResolvePath(tempFilePath.c_str(), resolvedTempFilePath, AZ_MAX_PATH_LEN);
        }

        // run the command and output to temp file
        {
            AZStd::string arg = AZStd::string::format("/dontskip /whql:off /t %s", resolvedTempFilePath);
            bool success = RunExternalCmd::Run("dxdiag", arg, nullptr, m_timeoutMilliSeconds);
            if (!success)
            {
                AZ_Warning("CloudCanvas", false, "Failed to run dxdiag command for defect reporter");
                fileIO->Remove(resolvedTempFilePath);
                return "";
            }
        }
        
        // read temp file and cleanup
        AZ::IO::HandleType fileHandle;
        if (!fileIO->Open(resolvedTempFilePath, AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeText, fileHandle))
        {
            AZ_Warning("CloudCanvas", false, "Failed to open temp dxdiag file for defect reporter");
            fileIO->Remove(resolvedTempFilePath);
            return "";
        }

        AZ::u64 fileSize = 0;
        if (!fileIO->Size(fileHandle, fileSize))
        {
            AZ_Warning("CloudCanvas", false, "Failed to open temp dxdiag file for defect reporter");
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