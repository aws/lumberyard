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

#ifndef CRYINCLUDE_EDITOR_DIALOGEDITOR_DIALOGSCRIPTRECORD_H
#define CRYINCLUDE_EDITOR_DIALOGEDITOR_DIALOGSCRIPTRECORD_H
#pragma once


#include "DialogManager.h"

class CDialogScriptRecord
    : public CXTPReportRecord
{
public:
    CDialogScriptRecord();
    CDialogScriptRecord(CEditorDialogScript* pScript, const CEditorDialogScript::SScriptLine* pLine);
    const CEditorDialogScript::SScriptLine* GetLine() const { return &m_line; }
    void Swap(CDialogScriptRecord* pOther);

protected:
    void OnBrowseAudioTrigger(string& value, CDialogScriptRecord* pRecord);

    void OnBrowseFacial(string& value, CDialogScriptRecord* pRecord);
    void OnBrowseAG(string& value, CDialogScriptRecord* pRecord);
    void GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics);
    void FillItems();

protected:
    CEditorDialogScript* m_pScript;
    CEditorDialogScript::SScriptLine m_line;
};

#endif // CRYINCLUDE_EDITOR_DIALOGEDITOR_DIALOGSCRIPTRECORD_H
