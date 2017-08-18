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

#ifndef CRYINCLUDE_EDITOR_ABOUTDIALOG_H
#define CRYINCLUDE_EDITOR_ABOUTDIALOG_H
#pragma once

#include <QDialog>

namespace Ui {
    class CAboutDialog;
}

class CAboutDialog
    : public QDialog
{
    Q_OBJECT

public:
    CAboutDialog(QWidget* pParent = nullptr);
    ~CAboutDialog();

    void SetVersion(const SFileVersion& v);

    static CAboutDialog* s_pAboutWindow;

private:
    void OnInitDialog();
    void OnCustomerAgreement();
    void OnPrivacyNotice();

    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

    SFileVersion m_version;
    QScopedPointer<Ui::CAboutDialog> ui;
    QPixmap m_hBitmap;          // Struct to hold the background bitmap
};

#endif // CRYINCLUDE_EDITOR_ABOUTDIALOG_H
