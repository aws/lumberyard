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


#ifndef CRYINCLUDE_CRYACTION_VISUALLOG_VISUALLOG_H
#define CRYINCLUDE_CRYACTION_VISUALLOG_VISUALLOG_H
#pragma once

#include "AnimKey.h"
#include "IVisualLog.h"

#define VISUAL_LOG_FRAME_PREFIX "Frame"

class CVisualLog
    : public IVisualLog
{
public:

    /*$1- Singleton Stuff ----------------------------------------------------*/
    static CVisualLog&  ref();
    static bool         Create();
    void                Shutdown();

    /*$1- Basics -------------------------------------------------------------*/
    void  Init();
    void    Update();
    void    Reset();

    /*$1- Utils --------------------------------------------------------------*/
    void Log(const char* format, ...)  PRINTF_PARAMS(2, 3);
    void Log(const SVisualLogParams& params, const char* format, ...)  PRINTF_PARAMS(3, 4);

protected:

    /*$1- Creation and destruction via singleton -----------------------------*/
    CVisualLog();
    virtual ~CVisualLog();

private:
    void InitCVars();
    bool OpenLogs();
    void CloseLogs();
    bool UpdateLogs();

    // Log writing functions
    void WriteFileHeaders(const char* sLogFileName, const char* sLogParamsFileName);
    void WriteFrameHeaders(int iFrameNum, const char* sImageName);

    ICaptureKey::CaptureFileFormat GetFormatType(const string& sFormat);

    void ShowDisplayMessage();

    /*$1- Members ------------------------------------------------------------*/
    bool                m_bInit;
    static CVisualLog*  m_pInstance;
    SVisualLogParams        m_defaultParams;

    bool                            m_bLoggingEnabled;
    int                         m_iFrameId;
    ICaptureKey::CaptureFileFormat                      m_eFormat;
    string                          m_sFormat;
    string                          m_sLogFolder;
    int                         m_iLogFolderNum;
    int                         m_iLogFrameNum;
    string                          m_sLogPath;

    string                          m_sLogBuffer;
    string                          m_sLogParamsBuffer;

    AZ::IO::HandleType m_logFileHandle;
    AZ::IO::HandleType m_logParamsFileHandle;

    // Visual Logger CVars
    ICVar*                          m_pCVVisualLog;
    ICVar*                          m_pCVVisualLogFolder;
    ICVar*                          m_pCVVisualLogImageFormat;
    ICVar*                          m_pCVVisualLogImageScale;
    ICVar*                          m_pCVVisualLogShowHUD;

    // CVars to control renderer 'capture frames' facility
    ICVar* m_pCV_capture_frames;
    ICVar* m_pCV_capture_file_format;
    ICVar* m_pCV_capture_frame_once;
    ICVar* m_pCV_capture_file_name;
    ICVar* m_pCV_capture_buffer;
};
#endif // CRYINCLUDE_CRYACTION_VISUALLOG_VISUALLOG_H
