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

#include <QDesktopServices>
#include <QPainter>
#include <QMouseEvent>
#include <QDesktopServices>

CAboutDialog* CAboutDialog::s_pAboutWindow = 0;

CAboutDialog::CAboutDialog(QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , ui(new Ui::CAboutDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_version = GetIEditor()->GetFileVersion();

    connect(ui->m_transparentAgreement, &QLabel::linkActivated, this, &CAboutDialog::OnCustomerAgreement);
    connect(ui->m_transparentNotice, &QLabel::linkActivated, this, &CAboutDialog::OnPrivacyNotice);

    OnInitDialog();
}

CAboutDialog::~CAboutDialog()
{
    s_pAboutWindow = 0;
}

void CAboutDialog::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.drawPixmap(rect(), m_hBitmap);
}

void CAboutDialog::OnInitDialog()
{
    s_pAboutWindow = this;

    SetVersion(m_version);

    m_hBitmap = QPixmap(QStringLiteral(":/StartupLogoDialog/sandbox_dark.png"));

    QFont smallFont(QStringLiteral("MS Shell Dlg 2"));
    smallFont.setPointSizeF(7.5);
    ui->m_transparentAllRightReserved->setFont(smallFont);

    // Prevent re-sizing
    this->setFixedSize(this->width(), this->height());
}

void CAboutDialog::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        s_pAboutWindow->accept();
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

void CAboutDialog::SetVersion(const SFileVersion& v)
{
#if defined(LY_BUILD)
    ui->m_transparentTrademarks->setText(tr("Version %1.%2.%3.%4 - Build %5").arg(v[3]).arg(v[2]).arg(v[1]).arg(v[0]).arg(LY_BUILD));
#else
    ui->m_transparentTrademarks->setText(tr("Version %1.%2.%3.%4").arg(v[3]).arg(v[2]).arg(v[1]).arg(v[0]));
#endif
}

#include <AboutDialog.moc>
