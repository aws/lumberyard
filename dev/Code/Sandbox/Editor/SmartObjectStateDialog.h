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

#pragma once

#include <QDialog>

namespace Ui
{
    class SmartObjectStateDialog;
}

class QItemSelection;

namespace
{
    class SmartObjectListModel;
    class SmartObjectTreeModel;
}

// CSmartObjectStateDialog dialog
#ifndef CRYINCLUDE_EDITOR_SMARTOBJECTSTATEDIALOG_H
#define CRYINCLUDE_EDITOR_SMARTOBJECTSTATEDIALOG_H

class CSmartObjectStateDialog
    : public QDialog
{
    Q_OBJECT

private:
    QScopedPointer<Ui::SmartObjectStateDialog> m_ui;
    SmartObjectListModel* m_model;
    SmartObjectTreeModel* m_treeModel;
    bool m_bMultiple;

    void UpdateListCurrentStates();

public:
    CSmartObjectStateDialog(const QString& soState, QWidget* parent = nullptr, bool multi = false, bool hasAdvanced = false);     // standard constructor
    virtual ~CSmartObjectStateDialog();

    QString GetSOState();

    static const int ADVANCED_MODE_REQUESTED = -1;

private:
    void OnTVSelChanged(const QItemSelection& selected, const QItemSelection& deselected);
    void OnNewBtn();
    void OnEditBtn();
    void OnDeleteBtn();
    void OnRefreshBtn(const QStringList& included, const QStringList& excluded);
    void UpdateIncludeExcludeEdits();

    void UpdateCSOLibrary(const QStringList& tokens);
    void Expand(QModelIndex index);

    virtual void OnInitDialog(const QString& soString, bool hasAdvanced);
};

#endif // CRYINCLUDE_EDITOR_SMARTOBJECTSTATEDIALOG_H
