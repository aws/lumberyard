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

#ifndef CRYINCLUDE_EDITOR_CUSTOMACTIONS_CUSTOMACTIONDIALOG_H
#define CRYINCLUDE_EDITOR_CUSTOMACTIONS_CUSTOMACTIONDIALOG_H
#pragma once

#include <QDialog>

#include <CustomActions/ui_CustomActionDialog.h>

class QStringListModel;

//////////////////////////////////////////////////////////////////////////
// Dialog shown for custom action property
//////////////////////////////////////////////////////////////////////////
class CCustomActionDialog
    : public QDialog
{
    Q_OBJECT
public:
    CCustomActionDialog(QWidget* pParent = nullptr);   // standard constructor
    ~CCustomActionDialog();

    void SetCustomAction(const QString& customAction);
    QString GetCustomAction() const;

// Dialog Data
protected:
    bool OpenViewForCustomAction();

    void OnLbnDblClk();
    void OnLbnSelchangeAction(const QModelIndex& current, const QModelIndex& previous);
    void Refresh();

protected slots:
    void OnNewBtn();
    void OnEditBtn();
    void OnRefreshBtn();

private:
    QScopedPointer<Ui_CCustomActionDialog> m_ui;
    QStringListModel* m_model;
    QString m_customAction;
};

#endif // CRYINCLUDE_EDITOR_CUSTOMACTIONS_CUSTOMACTIONDIALOG_H
