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

#ifndef CRYINCLUDE_TELEMETRYPLUGIN_CONNECTIONPANEL_H
#define CRYINCLUDE_TELEMETRYPLUGIN_CONNECTIONPANEL_H
#pragma once


#include "TelemetryRepository.h"
#include "PipeClient.h"
#include "PipeMessageParser.h"

class CConnectionPanel
    : public CDialog
    , public IEditorNotifyListener
{
public:

    enum
    {
        IDD = IDD_PANEL_CONNECTION
    };

    CConnectionPanel(CTelemetryRepository& repo, CWnd* pParent = NULL);

    virtual ~CConnectionPanel();

    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);

protected:

    bool OpenFile(char* filename, unsigned int bufSize, const char* filter, const char* title);

    virtual BOOL OnInitDialog();

    virtual void DoDataExchange(CDataExchange* pDX);

    DECLARE_MESSAGE_MAP()

    afx_msg void OnOpenClicked();

    afx_msg void OnConnectClicked();

    afx_msg void OnSetMarkersMode();

    afx_msg void OnSetDensityMode();

private:

    string m_statsToolPath;

    CTelemetryRepository& m_repository;

    CPipeClient m_pipe;
    CPipeMessageParser m_pipeMsgClient;

    static const uint32 PIPE_BUFFER_SIZE = 100 * 1024;

    CButton m_btnConnect;
    CButton m_btnOpen;
    CButton m_rbMarkers;
    CButton m_rbDensity;
};

#endif // CRYINCLUDE_TELEMETRYPLUGIN_CONNECTIONPANEL_H
