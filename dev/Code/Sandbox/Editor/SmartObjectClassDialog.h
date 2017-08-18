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


// CSmartObjectClassDialog dialog
#ifndef CRYINCLUDE_EDITOR_SMARTOBJECTCLASSDIALOG_H
#define CRYINCLUDE_EDITOR_SMARTOBJECTCLASSDIALOG_H

#include <QDialog>

namespace Ui
{
    class SmartObjectClassDialog;
}

class SmartObjectClassModel;

class CSmartObjectClassDialog
    : public QDialog
{
    Q_OBJECT
private:
    QString m_sSOClass;
    bool m_bMultiple;

    void UpdateListCurrentClasses();

public:
    CSmartObjectClassDialog(QWidget* pParent = nullptr, bool multi = false);   // standard constructor
    virtual ~CSmartObjectClassDialog();

    void SetSOClass(const QString& sSOClass) { m_sSOClass = sSOClass; }
    QString GetSOClass() { return m_sSOClass; };
    void EnableOK();

protected:
    void OnTVDblClk(const QModelIndex& index);
    void OnTVSelChanged();
    void OnNewBtn();
    void OnEditBtn();
    //void OnDeleteBtn();
    void OnRefreshBtn();
    void showEvent(QShowEvent* event) override;

public:
    void OnInitDialog();

private:
    SmartObjectClassModel* m_model;
    QScopedPointer<Ui::SmartObjectClassDialog> m_ui;
};

#endif // CRYINCLUDE_EDITOR_SMARTOBJECTCLASSDIALOG_H
