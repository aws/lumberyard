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

// Description : Logs images of game and related logging text, to be viewed
//               in the Editor Visual Log Viewer


#include "CryLegacy_precompiled.h"
#include "VisualLog.h"
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/functional.h>
#include <AzFramework/IO/FileOperations.h>

CVisualLog* CVisualLog::  m_pInstance = NULL;

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CVisualLog::CVisualLog()
    : m_bInit(false)
    , m_logFileHandle(AZ::IO::InvalidHandle)
    , m_logParamsFileHandle(AZ::IO::InvalidHandle)
{
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CVisualLog::~CVisualLog()
{
}

// Description:
//   Create
// Arguments:
//
// Return:
//
CVisualLog& CVisualLog::ref()
{
    if (NULL == m_pInstance)
    {
        Create();
    }
    CRY_ASSERT(m_pInstance);
    PREFAST_ASSUME(m_pInstance);

    return(*m_pInstance);
}

// Description:
//   Create
// Arguments:
//
// Return:
//
bool CVisualLog::Create()
{
    if (NULL == m_pInstance)
    {
        m_pInstance = new CVisualLog();
    }
    else
    {
        CRY_ASSERT("Trying to Create() the singleton more than once");
    }

    return(m_pInstance != NULL);
}

// Description:
//
// Arguments:
//
// Return:
//
void CVisualLog::Init()
{
    if (m_bInit == false)
    {
        InitCVars();
        Reset();

        m_bInit = true;
    }
}

// Description:
//
// Arguments:
//
// Return:
//
void CVisualLog::Shutdown()
{
    if (m_bInit == true)
    {
        Reset();
        SAFE_DELETE(m_pInstance);
    }
}

// Description:
//
// Arguments:
//
// Return:
//
void CVisualLog::Reset()
{
    CloseLogs();
    m_bLoggingEnabled = false;
    m_defaultParams = SVisualLogParams();
    m_iFrameId = 0;
    m_eFormat = ICaptureKey::Jpg;
    m_sFormat.clear();
    m_sLogBuffer.clear();
    m_sLogParamsBuffer.clear();
    m_sLogFolder.clear();
    m_sLogPath.clear();
    m_iLogFolderNum = 0;
    m_iLogFrameNum = 0;

    m_pCVVisualLog->Set(0);   // Turn off logging
}

// Description:
//
// Arguments:
//
// Return:
//
void CVisualLog::Update()
{
    if (m_bInit == false)
    {
        GameWarning("[VisualLog] Update called before Visual Log has been initialized");
        return;
    }

    // Check if we already called update this frame
    if (m_iFrameId == gEnv->pRenderer->GetFrameID())
    {
        return;
    }
    m_iFrameId = gEnv->pRenderer->GetFrameID();

    bool bDoLogging = m_pCVVisualLog->GetIVal() != 0;
    if (!bDoLogging && !m_bLoggingEnabled)
    {
        // Logging is off, so make this the fastest path
        return;
    }

    if (bDoLogging && !m_bLoggingEnabled)
    {
        // Start of logging run, open log files before doing an update
        m_bLoggingEnabled = OpenLogs();
        m_iLogFrameNum = -1; // Set this so number is incremented to correct value on first update
    }

    if (m_bLoggingEnabled)
    {
        if (bDoLogging)
        {
            // Regular logging frame update
            m_bLoggingEnabled = UpdateLogs();
        }
        else
        {
            // End of logging run
            m_bLoggingEnabled = false;
        }
    }

    if (m_bLoggingEnabled)
    {
        // tell renderer to capture a frame
        m_pCV_capture_frames->Set(1);
        m_pCV_capture_frame_once->Set(1);
        m_pCV_capture_buffer->Set(static_cast<int>(ICaptureKey::Color));

        // display onscreen message
        ShowDisplayMessage();
    }
    else
    {
        // Logging just ended, or an error occurred
        Reset();
    }
}

// Description:
//
// Arguments:
//
// Return:
//
bool CVisualLog::OpenLogs()
{
    m_sFormat = m_pCVVisualLogImageFormat->GetString();
    m_eFormat = GetFormatType(m_sFormat);
    m_sLogFolder = m_pCVVisualLogFolder->GetString();
    int iLogFolderLen = m_sLogFolder.length();

    // Check we have good params to use
    if (iLogFolderLen == 0)
    {
        GameWarning("[VisualLog] Log folder value invalid");
        return false;
    }

    // Create base directory if necessary
    gEnv->pFileIO->CreatePath(m_sLogFolder);

    // Figure out next number in sequence m_sLogFolderName/m_sLogFolderNameXXXX, where XXXX is 0000, 0001, etc.
    int iSeqNum = 0;
    gEnv->pFileIO->FindFiles(m_sLogFolder, "*.*", [&](const char* filePath) -> bool
        {
            if ((strcmp(filePath, ".") != 0) && m_sLogFolder.compareNoCase(filePath) && gEnv->pFileIO->IsDirectory(filePath))
            {
                iSeqNum = max(iSeqNum, atoi(filePath + iLogFolderLen) + 1);
            }
            return true;
        });

    // Now create directory
    char sLogPath[256];
    azsnprintf(sLogPath, sizeof(sLogPath), "%s\\%s%04d", m_sLogFolder.c_str(), m_sLogFolder.c_str(), iSeqNum);
    if (!gEnv->pFileIO->CreatePath(sLogPath))
    {
        GameWarning("[VisualLog] Unable to create directory for log files: %s", sLogPath);
        return false;
    }

    m_sLogPath = sLogPath;
    m_iLogFolderNum = iSeqNum;

    char sLogFileName[256];
    azsnprintf(sLogFileName, sizeof(sLogFileName), "%s\\%s%04d.log", m_sLogPath.c_str(), m_sLogFolder.c_str(), m_iLogFolderNum);
    char sLogParamsFileName[256];
    azsnprintf(sLogParamsFileName, sizeof(sLogParamsFileName), "%s\\%s%04d_params.log", m_sLogPath.c_str(), m_sLogFolder.c_str(), m_iLogFolderNum);

    // Open Log Files
    m_logFileHandle = fxopen(sLogFileName, "w");
    m_logParamsFileHandle = fxopen(sLogParamsFileName, "w");
    if (m_logFileHandle == AZ::IO::InvalidHandle || m_logParamsFileHandle == AZ::IO::InvalidHandle)
    {
        GameWarning("[VisualLog] Unable to open log files [%s] [%s]", sLogFileName, sLogParamsFileName);
        CloseLogs();
        return false;
    }

    WriteFileHeaders(sLogFileName, sLogParamsFileName);

    return true;
}

// Description:
//
// Arguments:
//
// Return:
//
void CVisualLog::CloseLogs()
{
    if (m_logFileHandle != AZ::IO::InvalidHandle)
    {
        gEnv->pFileIO->Close(m_logFileHandle);
        m_logFileHandle = AZ::IO::InvalidHandle;
    }

    if (m_logParamsFileHandle != AZ::IO::InvalidHandle)
    {
        gEnv->pFileIO->Close(m_logParamsFileHandle);
        m_logParamsFileHandle = AZ::IO::InvalidHandle;
    }
}

// Description:
//
// Arguments:
//
// Return:
//
bool CVisualLog::UpdateLogs()
{
    if (m_bInit == false)
    {
        GameWarning("[VisualLog] Update called before Visual Log has been initialized");
        return false;
    }

    // Don't do any updating if the last frame wasn't captured for some reason
    if (m_pCV_capture_frames->GetIVal() != 0)
    {
        return true;
    }

    // Do some checks to make sure logging parameters haven't been tampered with
    if (azstricmp(m_sFormat,  m_pCVVisualLogImageFormat->GetString()))
    {
        // Attempted image file format change during capture session, put it back
        m_pCVVisualLogImageFormat->Set(m_sFormat);
    }
    if (azstricmp(m_sLogFolder,  m_pCVVisualLogFolder->GetString()))
    {
        // Attempted log folder change during capture session, put it back
        m_pCVVisualLogFolder->Set(m_sLogFolder);
    }

    m_iLogFrameNum++;

    char sImageFileName[256];
    azsnprintf(sImageFileName, sizeof(sImageFileName), "Frame%06d.%s", m_iLogFrameNum, m_sFormat.c_str());

    char sImageFullFileName[512];
    azsnprintf(sImageFullFileName, sizeof(sImageFullFileName), "%s\\%s", m_sLogPath.c_str(), sImageFileName);
    m_pCV_capture_file_name->Set(sImageFullFileName);

    // Write log for this frame
    WriteFrameHeaders(m_iLogFrameNum, sImageFileName);
    AZ::IO::Print(m_logFileHandle, m_sLogBuffer.c_str());
    AZ::IO::Print(m_logParamsFileHandle, m_sLogParamsBuffer.c_str());
    gEnv->pFileIO->Flush(m_logFileHandle);
    gEnv->pFileIO->Flush(m_logParamsFileHandle);

    // Clear buffers now they have been written
    m_sLogBuffer.clear();
    m_sLogParamsBuffer.clear();

    return true;
}

// Description:
//
// Arguments:
//
// Return:
//
void CVisualLog::Log(const char* format, ...)
{
    if (m_bInit == false)
    {
        GameWarning("[VisualLog] Update called before Visual Log has been initialized");
        return;
    }
    if (!m_bLoggingEnabled)
    {
        return;
    }

    va_list ArgList;
    char    szBuffer[MAX_WARNING_LENGTH];

    va_start(ArgList, format);
    azvsprintf(szBuffer, format, ArgList);
    va_end(ArgList);

    // Add log to buffer
    m_sLogBuffer += szBuffer;
    m_sLogBuffer += "\n";

    // Add empty params string to params buffer
    m_sLogParamsBuffer += "[]\n";
}

// Description:
//
// Arguments:
//
// Return:
//
void CVisualLog::Log(const SVisualLogParams& params, const char* format, ...)
{
    if (!m_bLoggingEnabled)
    {
        return;
    }

    va_list ArgList;
    char    szBuffer[MAX_WARNING_LENGTH];
    char    szParamsBuffer[256];

    va_start(ArgList, format);
    azvsprintf(szBuffer, format, ArgList);
    va_end(ArgList);

    // Add log to buffer
    m_sLogBuffer += szBuffer;
    m_sLogBuffer += "\n";


    // Add params string to params buffer, only including non-default params
    bool bComma = false;
    m_sLogParamsBuffer += "[";
    if (params.color != m_defaultParams.color)
    {
        azsnprintf(szParamsBuffer, sizeof(szParamsBuffer), "color(%0.3f,%0.3f,%0.3f,%0.3f)",
            params.color.r, params.color.g, params.color.b, params.color.a);
        m_sLogParamsBuffer += szParamsBuffer;
        bComma = true;
    }
    if (params.size != m_defaultParams.size)
    {
        azsnprintf(szParamsBuffer, sizeof(szParamsBuffer), "%ssize(%0.3f)",
            bComma ? "," : "", max(0.1f, params.size));
        m_sLogParamsBuffer += szParamsBuffer;
        bComma = true;
    }
    if (params.column != m_defaultParams.column)
    {
        azsnprintf(szParamsBuffer, sizeof(szParamsBuffer), "%scolumn(%d)",
            bComma ? "," : "", max(1, params.column));
        m_sLogParamsBuffer += szParamsBuffer;
        bComma = true;
    }
    if (params.alignColumnsToThis != m_defaultParams.alignColumnsToThis)
    {
        azsnprintf(szParamsBuffer, sizeof(szParamsBuffer), "%salign_columns_to_this(%d)",
            bComma ? "," : "", params.alignColumnsToThis ? 1 : 0);
        m_sLogParamsBuffer += szParamsBuffer;
        bComma = true;
    }
    m_sLogParamsBuffer += "]\n";
}

// Description:
//
// Arguments:
//
// Return:
//

void CVisualLog::WriteFileHeaders(const char* sLogFileName, const char* sLogParamsFileName)
{
    // Headers start the same for both the log file and log params file
    // Just a listing of both of these file names
    AZ::IO::Print(m_logFileHandle, "%s\n%s\n", sLogFileName, sLogParamsFileName);
    AZ::IO::Print(m_logParamsFileHandle, "%s\n%s\n", sLogFileName, sLogParamsFileName);

    // Params file now lists all of the default parameters
    AZ::IO::Print(m_logParamsFileHandle, "color(%0.3f,%0.3f,%0.3f,%0.3f)\nsize(%0.3f)\ncolumn(%d)\nalign_columns_to_this(%d)\n",
        m_defaultParams.color.r, m_defaultParams.color.g, m_defaultParams.color.b, m_defaultParams.color.a,
        m_defaultParams.size,
        m_defaultParams.column,
        m_defaultParams.alignColumnsToThis ? 1 : 0);
}

// Description:
//
// Arguments:
//
// Return:
//
void CVisualLog::WriteFrameHeaders(int iFrameNum, const char* sImageName)
{
    // Frame header example:  Frame 0 [Frame000000.jpg] [34.527]
    CTimeValue time = gEnv->pTimer->GetFrameStartTime();

    AZ::IO::Print(m_logFileHandle, "Frame %d [%s] [%0.3f]\n", iFrameNum, sImageName, time.GetSeconds());
    AZ::IO::Print(m_logParamsFileHandle, "Frame %d [%s] [%0.3f]\n", iFrameNum, sImageName, time.GetSeconds());
}

// Description:
//
// Arguments:
//
// Return:
//
void CVisualLog::InitCVars()
{
    m_pCVVisualLog = REGISTER_INT("cl_visualLog", 0, 0, "Enables Visual Logging.");
    m_pCVVisualLogFolder = REGISTER_STRING("cl_visualLogFolder", "VisualLog", 0, "Specifies sub folder to write logs to.");
    m_pCVVisualLogImageFormat = REGISTER_STRING("cl_visualLogImageFormat", "tif", 0, "Specifies file format of captured files (jpg, tga, tif).");
    m_pCVVisualLogImageScale = REGISTER_FLOAT("cl_visualLogImageScale", 128, 0, "Image size. [0-1] = scale value. >1 = actual pixels for image width");

    assert(gEnv->pConsole);
    PREFAST_ASSUME(gEnv->pConsole);
    m_pCV_capture_frames = gEnv->pConsole->GetCVar("capture_frames");
    m_pCV_capture_file_format = gEnv->pConsole->GetCVar("capture_file_format");
    m_pCV_capture_frame_once = gEnv->pConsole->GetCVar("capture_frame_once");
    m_pCV_capture_file_name = gEnv->pConsole->GetCVar("capture_file_name");
    m_pCV_capture_buffer = gEnv->pConsole->GetCVar("capture_buffer");

    CRY_ASSERT(m_pCV_capture_frames);
    CRY_ASSERT(m_pCV_capture_file_format);
    CRY_ASSERT(m_pCV_capture_frame_once);
    CRY_ASSERT(m_pCV_capture_file_name);
    CRY_ASSERT(m_pCV_capture_buffer);
}

// Description:
//
// Arguments:
//
// Return:
//
ICaptureKey::CaptureFileFormat CVisualLog::GetFormatType(const string& sFormat)
{
    ICaptureKey::CaptureFileFormat format = ICaptureKey::Tif;

    if (!sFormat.compareNoCase("jpg"))
    {
        format = ICaptureKey::Jpg;
    }
    else if (!sFormat.compareNoCase("tga"))
    {
        format = ICaptureKey::Tga;
    }
    else if (!sFormat.compareNoCase("tif"))
    {
        format = ICaptureKey::Tif;
    }

    return format;
}

void CVisualLog::ShowDisplayMessage()
{
    float red[4] = {1, 0, 0, 1};
    gEnv->pRenderer->Draw2dLabel(gEnv->pRenderer->GetWidth() * 0.5f, gEnv->pRenderer->GetHeight() * 0.9f, 2, red, true, "VISUAL LOGGING ENABLED");
}


