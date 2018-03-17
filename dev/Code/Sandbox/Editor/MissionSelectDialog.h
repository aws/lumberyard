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

#ifndef CRYINCLUDE_EDITOR_MISSIONSELECTDIALOG_H
#define CRYINCLUDE_EDITOR_MISSIONSELECTDIALOG_H

#pragma once
// MissionSelectDialog.h : header file
//

#include <QDialog>
#include <QStringList>
#include <QScopedPointer>

/////////////////////////////////////////////////////////////////////////////
// CMissionSelectDialog dialog

namespace Ui {
    class CMissionSelectDialog;
}

class QListWidget;


class CMissionSelectDialog
    : public QDialog
{
    // Construction
public:
    CMissionSelectDialog(QWidget* pParent = nullptr);   // standard constructor
    ~CMissionSelectDialog();

    // Dialog Data
    QListWidget*    m_missions;
    QString m_description;
    QString m_selected;

    QString GetSelected() { return m_selected; }

    // Implementation
protected:
    void accept() override;
    void reject() override;
    void OnSelectMission();
    void OnDblclkMissions();
    void OnUpdateDescription();

    QStringList m_descriptions;
    QScopedPointer<Ui::CMissionSelectDialog> ui;
};

#endif // CRYINCLUDE_EDITOR_MISSIONSELECTDIALOG_H
