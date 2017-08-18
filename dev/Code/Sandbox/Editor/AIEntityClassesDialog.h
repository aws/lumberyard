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

#ifndef CRYINCLUDE_EDITOR_AIENTITYCLASSESDIALOG_H
#define CRYINCLUDE_EDITOR_AIENTITYCLASSESDIALOG_H

#pragma once

#include <QDialog>
#include <QScopedPointer>

class AIEntityClassesModel;

namespace Ui {
    class CAIEntityClassesDialog;
}

class CAIEntityClassesDialog
    : public QDialog
{
    Q_OBJECT

public:
    CAIEntityClassesDialog(QWidget* pParent = nullptr);
    ~CAIEntityClassesDialog();

    QString GetAIEntityClasses() const;
    void    SetAIEntityClasses(const QString& sAIEntityClasses);

private:
    QString m_sAIEntityClasses;

    void OnTVSelChanged();

private:
    void UpdateList();
    void UpdateDescription();
    void UpdateAIEntityClassesString();

    AIEntityClassesModel* m_model;

    QScopedPointer<Ui::CAIEntityClassesDialog> ui;
};

#endif // CRYINCLUDE_EDITOR_AIENTITYCLASSESDIALOG_H
