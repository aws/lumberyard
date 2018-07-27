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

#include "StdAfx.h"
#include "AboutDialog.h"
#include <ui_AboutDialog.h>

#include <QLabel>
#include <QDesktopServices>
#include <QPainter>
#include <QPixmap>
#include <QMouseEvent>
#include <QDesktopServices>
#include <QFont>

CAboutDialog::CAboutDialog(QString versionText, QString richTextCopyrightNotice, QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , m_ui(new Ui::CAboutDialog)
{
    m_ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    connect(m_ui->m_transparentAgreement, &QLabel::linkActivated, this, &CAboutDialog::OnCustomerAgreement);
    connect(m_ui->m_transparentNotice, &QLabel::linkActivated, this, &CAboutDialog::OnPrivacyNotice);

    m_ui->m_transparentTrademarks->setText(versionText);

    m_ui->m_transparentAllRightReserved->setTextFormat(Qt::RichText);
    m_ui->m_transparentAllRightReserved->setText(richTextCopyrightNotice);

    m_backgroundImage = QPixmap(QStringLiteral(":/StartupLogoDialog/sandbox_dark.png"));

    QFont smallFont(QStringLiteral("MS Shell Dlg 2"));
    smallFont.setPointSizeF(7.5);
    m_ui->m_transparentAllRightReserved->setFont(smallFont);

    // Prevent re-sizing
    setFixedSize(width(), height());
}

CAboutDialog::~CAboutDialog()
{
}

void CAboutDialog::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.drawPixmap(rect(), m_backgroundImage);
}

void CAboutDialog::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        accept();
    }

    QDialog::mouseReleaseEvent(event);
}

void CAboutDialog::OnCustomerAgreement()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("http://aws.amazon.com/agreement/")));
}

void CAboutDialog::OnPrivacyNotice()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("http://aws.amazon.com/privacy/")));
}

#include <AboutDialog.moc>
