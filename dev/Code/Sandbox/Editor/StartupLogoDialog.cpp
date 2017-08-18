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

// Description : implementation file


#include "StdAfx.h"
#include "StartupLogoDialog.h"
#include <ui_StartupLogoDialog.h>

#include <QPainter>
#include <QThread>


/////////////////////////////////////////////////////////////////////////////
// CStartupLogoDialog dialog

CStartupLogoDialog* CStartupLogoDialog::s_pLogoWindow = 0;

CStartupLogoDialog::CStartupLogoDialog(QWidget* pParent /*=NULL*/)
    : QWidget(pParent, Qt::Dialog | Qt::FramelessWindowHint)
    , m_ui(new Ui::StartupLogoDialog)
{
    m_ui->setupUi(this);
    OnInitDialog();

    // TODO: move this into the global qss file once mainwindow and dialogs_2 branches are merged
    setStyleSheet("CStartupLogoDialog > QLabel { background: transparent; color: 'white' }");
}

CStartupLogoDialog::~CStartupLogoDialog()
{
    s_pLogoWindow = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CStartupLogoDialog message handlers+


void CStartupLogoDialog::SetVersion(const SFileVersion& v)
{
#if defined(LY_BUILD)
    m_ui->m_TransparentVersion->setText(tr("Version %1.%2.%3.%4 - Build %5").arg(v[3]).arg(v[2]).arg((v[1] << 16)).arg(v[0]).arg(LY_BUILD));
#else
    m_ui->m_TransparentVersion->setText(tr("Version %1.%2.%3.%4").arg(v[3]).arg(v[2]).arg((v[1] << 16)).arg(v[0]));
#endif
}


void CStartupLogoDialog::SetInfo(const char* text)
{
    m_ui->m_TransparentText->setText(text);
    if (QThread::currentThread() == thread())
    {
        m_ui->m_TransparentText->repaint();
    }
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);  // if you don't process events, repaint does not function correctly.
}

void CStartupLogoDialog::SetText(const char* text)
{
    if (s_pLogoWindow)
    {
        s_pLogoWindow->SetInfo(text);
    }
}

void CStartupLogoDialog::SetInfoText(const char* text)
{
    SetInfo(text);
}

void CStartupLogoDialog::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.drawPixmap(rect(), m_hBitmap);
}

void CStartupLogoDialog::OnInitDialog()
{
    s_pLogoWindow = this;

    m_hBitmap = QPixmap(QStringLiteral(":/StartupLogoDialog/sandbox_dark.png"));

    setFixedSize(m_hBitmap.size());

    QFont smallFont(QStringLiteral("MS Shell Dlg 2"));
    smallFont.setPointSizeF(7.5);
    m_ui->m_TransparentConfidential->setFont(smallFont);

    QFont bigFont(QStringLiteral("MS Shell Dlg 2"));
    bigFont.setPointSizeF(12);
    m_ui->m_TransparentBeta->setFont(bigFont);

    setWindowTitle(tr("Starting Lumberyard Editor"));
}

#include <StartupLogoDialog.moc>
