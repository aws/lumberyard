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

#ifndef CRYINCLUDE_EDITOR_DIALOGS_RESOURCECOMPILERDIALOG_H
#define CRYINCLUDE_EDITOR_DIALOGS_RESOURCECOMPILERDIALOG_H
#pragma once


#include "IResourceCompilerHelper.h"

class CResourceCompilerDialog
    : public CXTResizeDialog
    , public CrySimpleThread<>
    , public IResourceCompilerListener
{
public:
    CResourceCompilerDialog(const CString& fileName, const CString& additionalSettings,
        std::function<void(const IResourceCompilerHelper::ERcCallResult)> finishedCallback);

    virtual BOOL OnInitDialog() override;

private:
    virtual void DoDataExchange(CDataExchange* pDX);

    virtual void Run() override;
    virtual void OnRCMessage(MessageSeverity severity, const char* pText);

    void AppendError(const char* pText);
    void AppendToOutput(CString string, bool bUseDefaultColor, COLORREF color);
    int GetNumVisibleLines();

    afx_msg void OnClose();
    afx_msg void OnShowLog();

    CString m_fileName;
    CString m_additionalSettings;

    CButton m_closeButton;
    CFont m_outputFont;
    CRichEditCtrl m_outputCtrl;
    CProgressCtrl m_progressCtrl;

    volatile bool m_bRcCanceled;
    volatile bool m_bRcFinished;
    IResourceCompilerHelper::ERcCallResult m_rcCallResult;

    boost::shared_ptr<IResourceCompilerProcess> m_currentRCProcess;

    std::function<void(const IResourceCompilerHelper::ERcCallResult)> m_finishedCallback;

    DECLARE_MESSAGE_MAP()
};
#endif // CRYINCLUDE_EDITOR_DIALOGS_RESOURCECOMPILERDIALOG_H
