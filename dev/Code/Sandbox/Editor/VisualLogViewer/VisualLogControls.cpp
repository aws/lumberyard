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
#include "VisualLogControls.h"

#include <AzQtComponents/Components/Widgets/ColorPicker.h>
#include <AzQtComponents/Utilities/Conversions.h>

#include <QFileDialog>
#include <QMessageBox>

#include "VisualLogCommon.h"
#include <VisualLogViewer/ui_VisualLogControls.h>
#include "../Plugins/EditorCommon/QtUtil.h"
#include "../Plugins/EditorCommon/QtUtilWin.h"

enum EVLogPlayerButtons
{
    eVLPB_None      = 0x0000,
    eVLPB_First     = 0x0001,
    eVLPB_Prev      = 0x0002,
    eVLPB_Play      = 0x0004,
    eVLPB_Stop      = 0x0008,
    eVLPB_Next      = 0x0010,
    eVLPB_Last      = 0x0020,
    eVLPB_All       = 0x003F,
};

// VisualLogDialog

// CVisualLogDialog construction & destruction
CVisualLogDialog::CVisualLogDialog(QWidget* parent)
    : QWidget(parent)
    , m_initialized(false)
    , m_ui(new Ui::CVisualLogDialog)
{
    m_ui->setupUi(this);

    connect(m_ui->VLB_BACKGROUND, &QPushButton::clicked,
        this, &CVisualLogDialog::OnBackgroundClicked);
    connect(m_ui->VLB_FIRSTFRAME, &QPushButton::clicked,
        this, &CVisualLogDialog::OnPlayerFirst);
    connect(m_ui->VLB_PREVFRAME, &QPushButton::clicked,
        this, &CVisualLogDialog::OnPlayerPrev);
    connect(m_ui->VLB_PLAY, &QPushButton::clicked,
        this, &CVisualLogDialog::OnPlayerPlay);
    connect(m_ui->VLB_STOP, &QPushButton::clicked,
        this, &CVisualLogDialog::OnPlayerStop);
    connect(m_ui->VLB_NEXTFRAME, &QPushButton::clicked,
        this, &CVisualLogDialog::OnPlayerNext);
    connect(m_ui->VLB_LASTFRAME, &QPushButton::clicked,
        this, &CVisualLogDialog::OnPlayerLast);
    connect(m_ui->VLB_FOLDER, &QPushButton::clicked,
        this, &CVisualLogDialog::OnFolderClicked);
    connect(m_ui->VLCHK_UPDATETEXT, &QCheckBox::stateChanged,
        this, &CVisualLogDialog::OnUpdateTextStateChanged);
    connect(m_ui->VLCHK_UPDATEIMAGES, &QCheckBox::stateChanged,
        this, &CVisualLogDialog::OnUpdateImagesStateChanged);
    connect(m_ui->VLCHK_KEEPASPECTR, &QCheckBox::stateChanged,
        this, &CVisualLogDialog::OnKeepAspectStateChanged);
    connect(m_ui->VLSLIDER_SPEED, &QSlider::valueChanged,
        this, &CVisualLogDialog::OnSpeedSliderMoved);
    connect(m_ui->VLSLIDER_CURFRAME, &QSlider::sliderMoved,
        this, &CVisualLogDialog::OnFrameSliderMoved);

    m_pCD = nullptr;
}

CVisualLogDialog::~CVisualLogDialog()
{
    if (m_pCD->eState == eVLPS_Playing)
    {
        killTimer(m_timerId), m_timerId = 0;
    }
}



// CVisualLogDialog operations
void CVisualLogDialog::EnablePlayerButtons()
{
    int nEnabled;

    if (m_pCD->eState == eVLPS_Uninitialized)
    {
        nEnabled = eVLPB_None;
    }
    else if (m_pCD->eState == eVLPS_Playing)
    {
        nEnabled = eVLPB_Stop;
    }
    else
    {
        nEnabled = eVLPB_Play;
        if (m_pCD->nCurFrame > 0)
        {
            nEnabled |= eVLPB_First | eVLPB_Prev;
        }
        if (m_pCD->nCurFrame < m_pCD->nLastFrame)
        {
            nEnabled |= eVLPB_Next | eVLPB_Last;
        }
    }

    m_ui->VLB_FIRSTFRAME->setEnabled(nEnabled & eVLPB_First);
    m_ui->VLB_PREVFRAME->setEnabled(nEnabled & eVLPB_Prev);
    m_ui->VLB_PLAY->setEnabled(nEnabled & eVLPB_Play);
    m_ui->VLB_STOP->setEnabled(nEnabled & eVLPB_Stop);
    m_ui->VLB_NEXTFRAME->setEnabled(nEnabled & eVLPB_Next);
    m_ui->VLB_LASTFRAME->setEnabled(nEnabled & eVLPB_Last);
}

void CVisualLogDialog::OnPlayerFirst()
{
    m_pCD->nCurFrame = 0;
    EnablePlayerButtons();
    Q_EMIT FrameChanged();

    m_ui->VLSLIDER_CURFRAME->setValue(m_pCD->nCurFrame);
    QString strTmp;
    strTmp.sprintf("%d/%d", m_pCD->nCurFrame, m_pCD->nLastFrame);
    m_ui->VLSTATIC_FRAME->setText(strTmp);
}

void CVisualLogDialog::OnPlayerPrev()
{
    m_pCD->nCurFrame--;
    EnablePlayerButtons();
    Q_EMIT FrameChanged();

    m_ui->VLSLIDER_CURFRAME->setValue(m_pCD->nCurFrame);
    QString strTmp;
    strTmp.sprintf("%d/%d", m_pCD->nCurFrame, m_pCD->nLastFrame);
    m_ui->VLSTATIC_FRAME->setText(strTmp);
}

void CVisualLogDialog::OnPlayerPlay()
{
    m_timerId = startTimer(10);

    m_pCD->eState = eVLPS_Playing;
    EnablePlayerButtons();
    m_nTimerVar = 1;
}

void CVisualLogDialog::OnPlayerStop()
{
    killTimer(m_timerId), m_timerId = 0;

    m_pCD->eState = eVLPS_Active;
    EnablePlayerButtons();
}

void CVisualLogDialog::OnPlayerNext()
{
    m_pCD->nCurFrame++;
    EnablePlayerButtons();
    Q_EMIT FrameChanged();

    m_ui->VLSLIDER_CURFRAME->setValue(m_pCD->nCurFrame);
    QString strTmp;
    strTmp.sprintf("%d/%d", m_pCD->nCurFrame, m_pCD->nLastFrame);
    m_ui->VLSTATIC_FRAME->setText(strTmp);
}

void CVisualLogDialog::OnPlayerLast()
{
    m_pCD->nCurFrame = m_pCD->nLastFrame;
    EnablePlayerButtons();
    Q_EMIT FrameChanged();

    m_ui->VLSLIDER_CURFRAME->setValue(m_pCD->nCurFrame);
    QString strTmp;
    strTmp.sprintf("%d/%d", m_pCD->nCurFrame, m_pCD->nLastFrame);
    m_ui->VLSTATIC_FRAME->setText(strTmp);
}

void CVisualLogDialog::OnInitDialog()
{
    // Initialize dialog controls
    m_ui->VLEDIT_FOLDERPATH->setText(QStringLiteral("not set"));

    m_ui->VLSLIDER_SPEED->setEnabled(false);
    m_ui->VLSLIDER_SPEED->setRange(-100, 100);
    m_ui->VLSLIDER_SPEED->setValue(100);
    m_ui->VLSLIDER_CURFRAME->setEnabled(false);

    m_ui->VLSTATIC_SPEED->setText(QStringLiteral("0.00"));

    m_ui->VLCHK_UPDATETEXT->setEnabled(false);
    m_ui->VLCHK_UPDATETEXT->setChecked(true);
    m_ui->VLCHK_UPDATEIMAGES->setEnabled(false);
    m_ui->VLCHK_UPDATEIMAGES->setChecked(true);
    m_ui->VLCHK_KEEPASPECTR->setEnabled(false);
    m_ui->VLCHK_KEEPASPECTR->setChecked(true);
}

void CVisualLogDialog::OnBackgroundClicked()
{
    QPalette p = m_ui->VLB_BACKGROUND->palette();

    AzQtComponents::ColorPicker picker(AzQtComponents::ColorPicker::Configuration::RGB);
    picker.setWindowTitle(tr("Select Color"));

    const AZ::Color currentColor = AzQtComponents::fromQColor(p.color(QPalette::Button));
    picker.setSelectedColor(currentColor);
    picker.setCurrentColor(currentColor);

    if (QDialog::Accepted != picker.exec())
    {
        return;
    }

    const QColor color = AzQtComponents::toQColor(picker.selectedColor());
    p.setColor(QPalette::Button, color);
    m_ui->VLB_BACKGROUND->setPalette(p);
    m_pCD->clrBack = QColor(color.red(), color.green(), color.blue());
    Q_EMIT BackgroundChanged();
}

void CVisualLogDialog::OnFolderClicked()
{
    const QString path = QFileDialog::getExistingDirectory(this);
    if (path.isEmpty())
    {
        return;
    }

    if (m_pCD->eState == eVLPS_Playing)
    {
        OnPlayerStop();
    }

    // Working folder
    m_pCD->strFolder = path;
    if ((m_pCD->strFolder[m_pCD->strFolder.length() - 1] != '\\') &&
        (m_pCD->strFolder[m_pCD->strFolder.length() - 1] != '/'))
    {
        m_pCD->strFolder += "\\";
    }

    Q_EMIT FolderChanged();
}

void CVisualLogDialog::FolderValid(bool validity)
{
    if (validity)
    {
        // Enable some controls
        m_ui->VLSLIDER_CURFRAME->setEnabled(true);
        m_ui->VLSLIDER_CURFRAME->setRange(0, m_pCD->nLastFrame);
        m_ui->VLSLIDER_CURFRAME->setValue(0);
        m_ui->VLSLIDER_SPEED->setEnabled(true);

        m_ui->VLCHK_UPDATETEXT->setEnabled(m_pCD->bHasText);
        m_ui->VLCHK_UPDATEIMAGES->setEnabled(m_pCD->bHasImages);
        m_ui->VLCHK_KEEPASPECTR->setEnabled(true);

        QString strTmp;
        strTmp.sprintf("0/%d", m_pCD->nLastFrame);
        m_ui->VLSTATIC_FRAME->setText(strTmp);

        int nPos = m_ui->VLSLIDER_SPEED->value();
        strTmp.sprintf("%0.2f", 1000.f / ((101 - abs(nPos)) * 10.f));
        if (nPos < 0)
        {
            strTmp = "-" + strTmp;
        }
        m_ui->VLSTATIC_SPEED->setText(strTmp);

        m_ui->VLEDIT_FOLDERPATH->setText(m_pCD->strFolder);

        EnablePlayerButtons();
    }
    else
    {
        QMessageBox::warning(this, QStringLiteral("Visual Log"), QStringLiteral("Not a valid folder!"));

        m_ui->VLCHK_UPDATETEXT->setEnabled(false);
        m_ui->VLCHK_UPDATETEXT->setEnabled(false);
        m_ui->VLCHK_UPDATETEXT->setEnabled(false);

        m_pCD->eState = eVLPS_Uninitialized;
        m_pCD->vecFiles.clear();

        m_ui->VLSLIDER_CURFRAME->setEnabled(false);
        m_ui->VLSLIDER_SPEED->setEnabled(false);

        m_ui->VLSTATIC_FRAME->clear();
        m_ui->VLSTATIC_SPEED->setText(QStringLiteral("0.00"));

        m_ui->VLEDIT_FOLDERPATH->setText(QStringLiteral("not set"));

        EnablePlayerButtons();
    }
}

void CVisualLogDialog::OnUpdateTextStateChanged(int state)
{
    m_pCD->bUpdateTxt = (state == Qt::Checked);
    Q_EMIT UpdateTextChanged(state == Qt::Checked);
}

void CVisualLogDialog::OnUpdateImagesStateChanged(int state)
{
    m_pCD->bUpdateImages = (state == Qt::Checked);
    Q_EMIT UpdateImagesChanged(state == Qt::Checked);
}

void CVisualLogDialog::OnKeepAspectStateChanged(int state)
{
    m_pCD->bKeepAspect = (state == Qt::Checked);
    Q_EMIT KeepAspectChanged(state == Qt::Checked);
}

void CVisualLogDialog::OnSpeedSliderMoved(int value)
{
    if (value == 0)
    {
        m_ui->VLSTATIC_SPEED->setText(QStringLiteral("0.00"));
    }
    else
    {
        QString strTmp;
        strTmp.sprintf("%0.2f", 1000.f / ((101 - abs(value)) * 10.f));
        if (value < 0)
        {
            strTmp = "-" + strTmp;
        }
        m_ui->VLSTATIC_SPEED->setText(strTmp);
    }
}

void CVisualLogDialog::OnFrameSliderMoved(int value)
{
    if (m_pCD->eState == eVLPS_Playing)
    {
        OnPlayerStop();
    }
    if (m_pCD->nCurFrame != value)
    {
        m_pCD->nCurFrame = value;

        QString strTmp;
        strTmp.sprintf("%d/%d", m_pCD->nCurFrame, m_pCD->nLastFrame);
        m_ui->VLSTATIC_FRAME->setText(strTmp);

        Q_EMIT FrameChanged();

        EnablePlayerButtons();
    }
}

void CVisualLogDialog::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    if (!m_initialized)
    {
        m_initialized = true;
        OnInitDialog();
    }
}

void CVisualLogDialog::timerEvent(QTimerEvent* event)
{
    if (event->timerId() != m_timerId)
    {
        return;
    }

    int nPos = m_ui->VLSLIDER_SPEED->value();

    if (nPos == 0)
    {
        return;
    }

    int nSkipVal = 101 - abs(nPos);                 // 100 ... 1
    if  (m_nTimerVar % nSkipVal == 0)
    {
        m_pCD->nCurFrame += (m_ui->VLSLIDER_SPEED->value() < 0)  ?  -1 : 1;
        if (m_pCD->nCurFrame < 0)
        {
            m_pCD->nCurFrame = m_pCD->nLastFrame;
        }
        else if (m_pCD->nCurFrame > m_pCD->nLastFrame)
        {
            m_pCD->nCurFrame = 0;
        }

        Q_EMIT InvalidateViews();

        m_ui->VLSLIDER_CURFRAME->setValue(m_pCD->nCurFrame);
        QString strTmp;
        strTmp.sprintf("%d/%d", m_pCD->nCurFrame, m_pCD->nLastFrame);
        m_ui->VLSTATIC_FRAME->setText(strTmp);
    }

    m_nTimerVar++;
    if (m_nTimerVar > 100)
    {
        m_nTimerVar = 1;
    }
}

#include <VisualLogViewer/VisualLogControls.moc>