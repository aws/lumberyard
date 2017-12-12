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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_MANNERRORREPORTDIALOG_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_MANNERRORREPORTDIALOG_H
#pragma once

#include "MannequinBase.h"

#include <QMetaType>

#include <QWidget>

namespace Ui
{
    class MannErrorReportDialog;
}

class CMannErrorReportTableModel;

// CMannErrorReportDialog dialog

class CMannErrorRecord
{
public:
    enum ESeverity
    {
        ESEVERITY_ERROR,
        ESEVERITY_WARNING,
        ESEVERITY_COMMENT
    };
    enum EFlags
    {
        FLAG_NOFILE         = 0x0001,   // Indicate that required file was not found.
        FLAG_SCRIPT         = 0x0002,   // Error with scripts.
        FLAG_TEXTURE        = 0x0004,   // Error with scripts.
        FLAG_OBJECTID       = 0x0008,   // Error with object Ids, Unresolved/Duplicate etc...
        FLAG_AI                 = 0x0010,   // Error with AI.
    };
    //! Severity of this error.
    ESeverity severity;
    //! Error Text.
    QString error;
    //! Error Type.
    SMannequinErrorReport::ErrorType type;
    //! File which is missing or causing problem.
    QString file;
    int count;
    FragmentID fragmentID;
    SFragTagState tagState;
    uint32 fragOptionID;
    FragmentID fragmentIDTo;
    SFragTagState tagStateTo;
    const SScopeContextData* contextDef;

    int flags;

    CMannErrorRecord(ESeverity _severity, const char* _error, int _flags = 0, int _count = 0)
        : severity(_severity)
        , flags(_flags)
        , count(_count)
        , error(_error)
        , fragmentID(FRAGMENT_ID_INVALID)
        , fragmentIDTo(FRAGMENT_ID_INVALID)
        , fragOptionID(0)
        , contextDef(NULL)
    {
    }

    CMannErrorRecord()
    {
        severity = ESEVERITY_WARNING;
        flags = 0;
        count = 0;
    }
    QString GetErrorText() const;
};

Q_DECLARE_METATYPE(CMannErrorRecord*);

class CMannErrorReportDialog
    : public QWidget
{
    Q_OBJECT
public:

    enum EReportColumn
    {
        COLUMN_CHECKBOX,
        COLUMN_SEVERITY,
        COLUMN_FRAGMENT,
        COLUMN_TAGS,
        COLUMN_FRAGMENTTO,
        COLUMN_TAGSTO,
        COLUMN_TYPE,
        COLUMN_TEXT,
        COLUMN_FILE
    };

    CMannErrorReportDialog(QWidget* pParent = nullptr);
    virtual ~CMannErrorReportDialog();

    void Clear();
    void Refresh();

    void AddError(const CMannErrorRecord& error);
    bool HasErrors() const
    {
        return (m_errorRecords.size() > 0);
    }

public slots:
    void CopyToClipboard();

    void SendInMail();
    void OpenInExcel();

protected:
    void OnInitDialog();

    void OnSelectObjects();
    void OnSelectAll();
    void OnClearSelection();
    void OnCheckErrors();
    void OnDeleteFragments();

    void OnReportItemClick(const QModelIndex& index);
    void OnReportItemRClick();
    void OnReportColumnRClick();
    void OnReportItemDblClick(const QModelIndex& index);
    void keyPressEvent(QKeyEvent* event) override;

    void UpdateErrors();
    void ReloadErrors();

    std::vector<CMannErrorRecord> m_errorRecords;

    SMannequinContexts* m_contexts;

    QScopedPointer<Ui::MannErrorReportDialog> m_ui;
    CMannErrorReportTableModel* m_errorReportModel;
};

#endif // CRYINCLUDE_EDITOR_MANNEQUIN_MANNERRORREPORTDIALOG_H
