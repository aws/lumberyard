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

#ifndef CRYINCLUDE_EDITOR_LEVELFILEDIALOG_H
#define CRYINCLUDE_EDITOR_LEVELFILEDIALOG_H
#pragma once

#include <QDialog>
#include <QScopedPointer>

namespace Ui {
    class Dialog;
}

class LevelTreeModel;
class LevelTreeModelFilter;

class CLevelFileDialog
    : public QDialog
{
    Q_OBJECT
public:
    explicit CLevelFileDialog(bool openDialog, QWidget* parent = nullptr);
    ~CLevelFileDialog();

    QString GetFileName() const;

    static bool CheckLevelFolder(const QString folder, QStringList* levelFiles = nullptr);
    static bool CheckSubFoldersForLevelsRec(const QString folder, bool bRoot = true);

protected Q_SLOTS:
    void OnOK();
    void OnCancel();
    void OnTreeSelectionChanged();
    void OnNewFolder();
    void OnFilterChanged();
protected:
    void ReloadTree();
    bool ValidateLevelPath(const QString& folder);

    void SaveLastUsedLevelPath();
    void LoadLastUsedLevelPath();

private:
    QString NameForIndex(const QModelIndex& index) const;

    bool IsValidLevelSelected();
    QString GetLevelPath();
    QString GetEnteredPath();
    QString GetFileName(QString levelPath);

    QScopedPointer<Ui::Dialog> ui;
    QString m_fileName;
    QString m_filter;
    const bool m_bOpenDialog;
    bool m_initialized = false;
    LevelTreeModel* const m_model;
    LevelTreeModelFilter* const m_filterModel;
};

#endif // CRYINCLUDE_EDITOR_LEVELFILEDIALOG_H
