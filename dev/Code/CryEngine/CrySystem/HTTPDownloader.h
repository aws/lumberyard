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

#ifndef CRYINCLUDE_CRYSYSTEM_HTTPDOWNLOADER_H
#define CRYINCLUDE_CRYSYSTEM_HTTPDOWNLOADER_H
#pragma once

#pragma once


#if defined(WIN32)

#include <wininet.h>
#include <IScriptSystem.h>


#define HTTP_BUFFER_SIZE        (16384)


enum
{
    HTTP_STATE_WORKING = 0,
    HTTP_STATE_COMPLETE,
    HTTP_STATE_CANCELED,
    HTTP_STATE_ERROR,
    HTTP_STATE_NONE,
};


class CDownloadManager;


class CHTTPDownloader // : public _ScriptableEx<CHTTPDownloader>
{
public:
    CHTTPDownloader();
    virtual ~CHTTPDownloader();

    int Create(ISystem* pISystem, CDownloadManager* pParent);
    int Download(const char* szURL, const char* szDestination);
    void Cancel();
    int GetState() { return m_iState; };
    int GetFileSize() const { return m_iFileSize; };
    const string& GetURL() const { return m_szURL; };
    const string& GetDstFileName() const { return m_szDstFile; };
    void    Release();

    int Download(IFunctionHandler* pH);
    int Cancel(IFunctionHandler* pH);
    int Release(IFunctionHandler* pH);

    int GetURL(IFunctionHandler* pH);
    int GetFileSize(IFunctionHandler* pH);
    int GetFileName(IFunctionHandler* pH);

    void OnError();
    void OnComplete();
    void OnCancel();

private:

    static  DWORD DownloadProc(CHTTPDownloader* _this);
    void    CreateThread();
    DWORD DoDownload();
    void    PrepareBuffer();
    IScriptTable* GetScriptObject() { return 0; };

    string                      m_szURL;
    string                      m_szDstFile;
    HANDLE                      m_hThread;
    HINTERNET                   m_hINET;
    HINTERNET                   m_hUrl;

    unsigned char* m_pBuffer;
    int                             m_iFileSize;

    volatile int            m_iState;
    volatile bool           m_bContinue;

    ISystem* m_pSystem;
    CDownloadManager* m_pParent;

    IScriptSystem* m_pScriptSystem;
};

#endif //LINUX

#endif // CRYINCLUDE_CRYSYSTEM_HTTPDOWNLOADER_H
