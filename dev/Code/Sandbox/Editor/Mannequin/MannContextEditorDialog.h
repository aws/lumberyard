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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_MANNCONTEXTEDITORDIALOG_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_MANNCONTEXTEDITORDIALOG_H
#pragma once

#include "MannequinBase.h"
#include <QDialog>

#include <QScopedPointer>

class CXTPMannContextRecord;
class CMannContextTableModel;

namespace Ui {
    class MannContextEditorDialog;
}

class CMannContextEditorDialog
    : public QDialog
{
public:
    CMannContextEditorDialog(QWidget* pParent = nullptr);
    virtual ~CMannContextEditorDialog();

private:
    void OnReportItemDblClick(const QModelIndex& index);
    void OnReportSelChanged();

    void OnNewContext();
    void OnEditContext();
    void OnCloneAndEditContext();
    void OnDeleteContext();
    void OnMoveUp();
    void OnMoveDown();

    void OnImportBackground();

    void OnInitDialog();

    void PopulateReport();
    void EnableControls();
    CXTPMannContextRecord* GetFocusedRecord();
    QModelIndex GetFocusedRecordIndex() const;
    void SetFocusedRecord(const uint32 dataID);

    void OnColumnHeaderClicked(int column);

    CMannContextTableModel* m_contextModel;

    QScopedPointer<Ui::MannContextEditorDialog> ui;
};

#endif // CRYINCLUDE_EDITOR_MANNEQUIN_MANNCONTEXTEDITORDIALOG_H
