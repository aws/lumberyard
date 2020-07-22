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
#include <QString>
#include <QPixmap>

namespace Ui {
    class CAboutDialog;
}

class CAboutDialog
    : public QDialog
{
    Q_OBJECT

public:
    CAboutDialog(QString versionText, QString richTextCopyrightNotice, QWidget* pParent = nullptr);
    ~CAboutDialog();

private:

    void OnCustomerAgreement();
    void OnPrivacyNotice();

    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

    QScopedPointer<Ui::CAboutDialog>    m_ui;
    QPixmap                             m_backgroundImage;

    int m_enforcedWidth = 602;
    double m_enforcedRatio = 300.0 / 602.0;
};

