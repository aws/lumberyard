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

#include "stdafx.h"
#include "ConnectionPanel.h"

//////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CConnectionPanel, CDialog)
ON_BN_CLICKED(IDC_OPEN, OnOpenClicked)
ON_BN_CLICKED(IDC_CONNECT, OnConnectClicked)
ON_BN_CLICKED(IDC_MARKERS_RB, OnSetMarkersMode)
ON_BN_CLICKED(IDC_DENSITY_RB, OnSetDensityMode)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////

CConnectionPanel::CConnectionPanel(CTelemetryRepository& repo, CWnd* pParent)
    : CDialog(CConnectionPanel::IDD, pParent)
    , m_repository(repo)
    , m_pipeMsgClient(repo)
{
    char path[_MAX_PATH];
    GetCurrentDirectory(1024, path);

    m_statsToolPath = path;
    m_statsToolPath += "/Tools/StatTools/StatsTool/StatsTool2.exe";

    GetIEditor()->RegisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////

CConnectionPanel::~CConnectionPanel()
{
    if (m_pipe.isConnected())
    {
        m_pipe.CloseConnection();
    }

    GetIEditor()->UnregisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////

void CConnectionPanel::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    if (event == eNotify_OnIdleUpdate)
    {
        m_pipeMsgClient.DeliverMessages();
    }
    else if (event == eNotify_OnBeginLoad)
    {
        m_btnConnect.EnableWindow(FALSE);
        m_btnOpen.EnableWindow(FALSE);
        m_repository.ClearData(false);
    }
    else if (event == eNotify_OnEndSceneOpen)
    {
        m_btnConnect.EnableWindow(TRUE);
        m_btnOpen.EnableWindow(TRUE);
        m_repository.ClearData(false);
    }
}

//////////////////////////////////////////////////////////////////////////

void CConnectionPanel::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_CONNECT, m_btnConnect);
    DDX_Control(pDX, IDC_OPEN, m_btnOpen);
    DDX_Control(pDX, IDC_MARKERS_RB, m_rbMarkers);
    DDX_Control(pDX, IDC_DENSITY_RB, m_rbDensity);
}

//////////////////////////////////////////////////////////////////////////

BOOL CConnectionPanel::OnInitDialog()
{
    BOOL res = CDialog::OnInitDialog();

    m_rbMarkers.SetCheck(TRUE);

    return res;
}

//////////////////////////////////////////////////////////////////////////

bool CConnectionPanel::OpenFile(char* filename, unsigned int bufSize, const char* filter, const char* title)
{
    if (!filename || !bufSize)
    {
        return false;
    }

    filename[0] = '\0';

    OPENFILENAME ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(OPENFILENAME);

    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFilter = filter;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = bufSize;
    ofn.lpstrTitle = title;

    return GetOpenFileName(&ofn) != 0;
}

//////////////////////////////////////////////////////////////////////////

void CConnectionPanel::OnOpenClicked()
{
    char filename[_MAX_PATH];

    struct stat desc;
    if (stat(m_statsToolPath.c_str(), &desc))
    {
        if (OpenFile(filename, sizeof(filename), "StatsTool2\0StatsTool2.exe\0", "StatsTool location"))
        {
            m_statsToolPath = filename;
        }
        else
        {
            return;
        }
    }

    if (OpenFile(filename, sizeof(filename), "XML files (*.xml)\0*.xml\0", "Choose telemetry file to open"))
    {
        if (!m_pipe.isConnected())
        {
            OnConnectClicked();
        }

        stack_string args = m_statsToolPath.c_str();
        args += " --open \"";
        args += filename;
        args += "\" --sandbox_connect";

        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        ZeroMemory(&pi, sizeof(pi));

        CreateProcessA(m_statsToolPath.c_str(),
            (char*)args.c_str(),
            0, 0, FALSE, CREATE_DEFAULT_ERROR_MODE, 0, 0, &si, &pi);
    }
}

//////////////////////////////////////////////////////////////////////////

void CConnectionPanel::OnConnectClicked()
{
    if (!m_pipe.isConnected())
    {
        char version[256];
        gEnv->pSystem->GetProductVersion().ToString(version);
        stack_string pipe_name("\\\\.\\pipe\\CryStatsToolPipe_");
        pipe_name += version;

        m_pipe.OpenConnection(pipe_name.c_str(), &m_pipeMsgClient, PIPE_BUFFER_SIZE);

        m_btnConnect.SetWindowTextA("Disconnect");
    }
    else
    {
        m_pipe.CloseConnection();
        m_btnConnect.SetWindowTextA("Connect");
    }
}

//////////////////////////////////////////////////////////////////////////

void CConnectionPanel::OnSetMarkersMode()
{
    m_repository.setRenderMode(eTLRM_Markers);
}

//////////////////////////////////////////////////////////////////////////

void CConnectionPanel::OnSetDensityMode()
{
    m_repository.setRenderMode(eTLRM_Density);
}

//////////////////////////////////////////////////////////////////////////
