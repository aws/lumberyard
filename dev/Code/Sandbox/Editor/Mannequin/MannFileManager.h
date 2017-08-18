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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_MANNFILEMANAGER_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_MANNFILEMANAGER_H
#pragma once

#include <QScopedPointer>
#include <QDialog>
#include <Mannequin/ui_MannFileManager.h>

class CMannequinFileChangeWriter;
class QModelIndex;
class CMannFileManagerTableModel;

namespace Ui {
    class MannFileManager;
}

class CMannFileManager
    : public QDialog
{
public:
    CMannFileManager(CMannequinFileChangeWriter& fileChangeWriter, bool bChangedFileMode, QWidget* pParent = nullptr);

    void OnRefresh();

protected:
    void OnInitDialog();

    void OnCheckOutSelection();
    void OnUndoSelection();
    void OnReloadAllFiles();
    void OnSaveFiles();

    void OnReportItemClick(const QModelIndex&);
    void OnReportItemRClick();
    void OnReportColumnRClick();
    void OnReportItemDblClick(const QModelIndex&);
    void OnReportSelChanged();
    void OnReportHyperlink(const QModelIndex&);

    void RepositionControls();

    void ReloadFileRecords();
    void UpdateFileRecords();

    void SetButtonStates();

    void OnDisplayOnlyCurrentPreviewClicked();

    void showEvent(QShowEvent* event) override;

private:
    CMannFileManagerTableModel* m_fileManagerModel;

    CMannequinFileChangeWriter& m_fileChangeWriter;
    bool m_bSourceControlAvailable;
    bool m_bInChangedFileMode;

    QScopedPointer<Ui::MannFileManager> ui;
};

#endif // CRYINCLUDE_EDITOR_MANNEQUIN_MANNFILEMANAGER_H
