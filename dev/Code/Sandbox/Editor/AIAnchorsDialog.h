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


// CAIAnchorsDialog dialog
#ifndef CRYINCLUDE_EDITOR_AIANCHORSDIALOG_H
#define CRYINCLUDE_EDITOR_AIANCHORSDIALOG_H

#include <QDialog>

class QStringListModel;

namespace Ui {
    class CAIAnchorsDialog;
}

class CAIAnchorsDialog
    : public QDialog
{
    Q_OBJECT

private:
    QString m_sAnchor;

public:
    CAIAnchorsDialog(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CAIAnchorsDialog();

    void SetAIAnchor(const QString& sAnchor);
    QString GetAIAnchor() { return m_sAnchor; };

protected:
    void OnLbnDblClk();
    void OnLbnSelchangeAnchors();
    void OnNewBtn();
    void OnEditBtn();
    void OnDeleteBtn();
    void OnRefreshBtn();

    QStringListModel* m_anchorsModel;
    QScopedPointer<Ui::CAIAnchorsDialog> ui;

public:
    virtual void OnInitDialog();
};

#endif // CRYINCLUDE_EDITOR_AIANCHORSDIALOG_H
