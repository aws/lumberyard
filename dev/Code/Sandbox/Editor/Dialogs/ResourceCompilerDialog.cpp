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

#include "StdAfx.h"
#include "ResourceCompilerDialog.h"

BEGIN_MESSAGE_MAP(CResourceCompilerDialog, CXTResizeDialog)
ON_COMMAND(IDC_CLOSE, OnClose)
ON_COMMAND(IDC_SHOWLOG, OnShowLog)
END_MESSAGE_MAP()

CResourceCompilerDialog::CResourceCompilerDialog(const CString& fileName, const CString& additionalSettings,
    std::function<void(const IResourceCompilerHelper::ERcCallResult)> finishedCallback)
    : CXTResizeDialog(IDD_RCPROGRESS)
    , m_fileName(fileName)
    , m_additionalSettings(additionalSettings)
    , m_bRcCanceled(false)
    , m_bRcFinished(false)
    , m_finishedCallback(finishedCallback)
{
}

BOOL CResourceCompilerDialog::OnInitDialog()
{
    CXTResizeDialog::OnInitDialog();

    if (m_outputFont.CreateFont(
            14,                    // nHeight
            0,                     // nWidth
            0,                     // nEscapement
            0,                     // nOrientation
            FW_NORMAL,             // nWeight
            FALSE,                 // bItalic
            FALSE,                 // bUnderline
            0,                     // cStrikeOut
            ANSI_CHARSET,          // nCharSet
            OUT_DEFAULT_PRECIS,    // nOutPrecision
            CLIP_DEFAULT_PRECIS,   // nClipPrecision
            DEFAULT_QUALITY,       // nQuality
            DEFAULT_PITCH | FF_SWISS, // nPitchAndFamily
            "Consolas") != 0)
    {
        m_outputCtrl.SetFont(&m_outputFont, true);
    }
    else if (m_outputFont.CreateFont(
                 14,               // nHeight
                 0,                // nWidth
                 0,                // nEscapement
                 0,                // nOrientation
                 FW_NORMAL,        // nWeight
                 FALSE,            // bItalic
                 FALSE,            // bUnderline
                 0,                // cStrikeOut
                 ANSI_CHARSET,     // nCharSet
                 OUT_DEFAULT_PRECIS, // nOutPrecision
                 CLIP_DEFAULT_PRECIS, // nClipPrecision
                 DEFAULT_QUALITY,  // nQuality
                 DEFAULT_PITCH | FF_SWISS, // nPitchAndFamily
                 "Courier New") != 0)
    {
        m_outputCtrl.SetFont(&m_outputFont, true);
    }

    SetResize(IDC_PROGRESS, CXTResizeRect(0, 0, 1, 0));
    SetResize(IDC_RICHEDIT, CXTResizeRect(0, 0, 1, 1));
    SetResize(IDC_SHOWLOG, CXTResizeRect(0, 1, 0, 1));
    SetResize(IDC_CLOSE, CXTResizeRect(1, 1, 1, 1));

    Start();
    return TRUE;
}

void CResourceCompilerDialog::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_PROGRESS, m_progressCtrl);
    DDX_Control(pDX, IDC_RICHEDIT, m_outputCtrl);
    DDX_Control(pDX, IDC_CLOSE, m_closeButton);
}

void CResourceCompilerDialog::Run()
{
    if ((!gEnv) || (!gEnv->pResourceCompilerHelper))
    {
        m_finishedCallback(IResourceCompilerHelper::eRcCallResult_notFound);
    }

    m_rcCallResult = gEnv->pResourceCompilerHelper->CallResourceCompiler(m_fileName, m_additionalSettings, this, true, IResourceCompilerHelper::eRcExePath_currentFolder);

    if (!m_bRcCanceled)
    {
        m_progressCtrl.SetPos(100);
        m_bRcFinished = true;

        m_finishedCallback(m_rcCallResult);
    }
}

void CResourceCompilerDialog::OnRCMessage(MessageSeverity severity, const char* pText)
{
    if (severity == IResourceCompilerListener::MessageSeverity_Info)
    {
        AppendToOutput(pText, true, RGB(0, 0, 0));

        if (strstr(pText, "%") != nullptr)
        {
            m_progressCtrl.SetPos(atoi(pText));
        }
    }
    else if (severity == IResourceCompilerListener::MessageSeverity_Warning)
    {
        AppendToOutput(pText, false, RGB(221, 72, 0));
    }
    else if (severity == IResourceCompilerListener::MessageSeverity_Error)
    {
        AppendToOutput(pText, false, RGB(150, 0, 0));
    }
}

void CResourceCompilerDialog::AppendToOutput(CString text, bool bUseDefaultColor, COLORREF color)
{
    text += "\r\n";

    CHARFORMAT charFormat;
    // Initialize character format structure
    charFormat.cbSize = sizeof(CHARFORMAT);
    charFormat.dwMask = CFM_COLOR;
    charFormat.dwEffects = bUseDefaultColor ?  CFE_AUTOCOLOR : 0;
    charFormat.crTextColor = color;

    // Get the initial text length.
    const int length = m_outputCtrl.GetWindowTextLength();
    // Put the selection at the end of text.
    m_outputCtrl.SetSel(length, -1);

    //  Set the character format
    m_outputCtrl.SetSelectionCharFormat(charFormat);

    // Replace the selection.
    m_outputCtrl.ReplaceSel(text);

    const int visibleLines = GetNumVisibleLines();
    if (&m_outputCtrl != m_outputCtrl.GetFocus())
    {
        m_outputCtrl.LineScroll(INT_MAX);
        m_outputCtrl.LineScroll(1 - visibleLines);
    }
}

int CResourceCompilerDialog::GetNumVisibleLines()
{
    CRect rect;
    long firstChar, lastChar;
    long firstLine, lastLine;

    // Get client rect of rich edit control
    m_outputCtrl.GetClientRect(rect);

    // Get character index close to upper left corner
    firstChar = m_outputCtrl.CharFromPos(CPoint(0, 0));

    // Get character index close to lower right corner
    lastChar = m_outputCtrl.CharFromPos(CPoint(rect.right, rect.bottom));
    if (lastChar < 0)
    {
        lastChar = m_outputCtrl.GetTextLength();
    }

    // Convert to lines
    firstLine = m_outputCtrl.LineFromChar(firstChar);
    lastLine  = m_outputCtrl.LineFromChar(lastChar);

    return (lastLine - firstLine);
}

void CResourceCompilerDialog::OnClose()
{
    if (!m_bRcFinished && QMessageBox:::question(QApplication::activeWindow(), QString(), "Do you want to cancel the computation?", QMessageBox::Ok|QMessageBox::Cancel) == QMessageBox::Ok)
    {
        m_bRcCanceled = true;
        m_currentRCProcess->Terminate();
        m_currentRCProcess = nullptr;
        m_finishedCallback(IResourceCompilerHelper::eRcCallResult_crash);
        EndDialog(IDCANCEL);
    }
    else
    {
        EndDialog(IDOK);
    }
}

void CResourceCompilerDialog::OnShowLog()
{
    char workingDirectory[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, workingDirectory);

    CString path(workingDirectory);
#if (_MSC_VER == 1900)
    path.Append("/Bin64vc140/rc/rc_log.log");
#else // _MSC_VER == 1900
    path.Append("/Bin64vc120/rc/rc_log.log");
#endif // _MSC_VER == 1900

    // Explorer command lines needs windows style paths
    path.Replace('/', '\\');

    const CString parameters = "/select," + path;
    ShellExecute(NULL, "Open", "explorer.exe", parameters, "", SW_NORMAL);
}
