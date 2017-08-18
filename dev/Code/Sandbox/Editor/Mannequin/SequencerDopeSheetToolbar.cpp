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

// Description : A toolbar which contains basic playback functionality for use
//               in the mannequin editor


#include "StdAfx.h"
#include "SequencerDopeSheetToolbar.h"

#include <QBoxLayout>
#include <QFrame>
#include <QSpinBox>
#include <QTimeEdit>

CSequencerDopeSheetToolbar::CSequencerDopeSheetToolbar(QWidget* parent)
    : QWidget(parent)
    , m_timeCodeContainer(new QFrame(this))
    , m_timeWindow(new QTimeEdit)
    , m_frameWindow(new QSpinBox)
    , m_updating(false)
{
    m_lastTime = -1;
    m_lastFPS = -1;

    QFont f;
    f.setBold(true);
    m_timeCodeContainer->setFont(f);
    m_timeCodeContainer->setFrameShape(QFrame::Panel);
    m_timeWindow->setFrame(false);
    m_timeWindow->setDisplayFormat(QStringLiteral("mm:ss:zzz"));
    m_timeWindow->setMaximumTime(QTime(0,59,59,999));
    m_timeWindow->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_timeWindow->setFixedWidth(fontMetrics().width('0') * 9);
    m_frameWindow->setFrame(false);
    m_frameWindow->setPrefix(QStringLiteral("("));
    m_frameWindow->setSuffix(QStringLiteral(")"));
    m_frameWindow->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_frameWindow->setFixedWidth(fontMetrics().width('0') * 4);
    m_frameWindow->setRange(0, 29);

    auto cLayout = new QHBoxLayout;
    m_timeCodeContainer->setLayout(cLayout);
    cLayout->setContentsMargins(0, 0, 0, 0);
    cLayout->addWidget(m_timeWindow);
    cLayout->addWidget(m_frameWindow);

    connect(m_timeWindow, &QTimeEdit::timeChanged,
        this, &CSequencerDopeSheetToolbar::OnTimeChanged);
    connect(m_frameWindow, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
        this, &CSequencerDopeSheetToolbar::OnFrameChanged);
}

CSequencerDopeSheetToolbar::~CSequencerDopeSheetToolbar()
{
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetToolbar::SetTime(float fTime, float fFps)
{
    if (fTime == m_lastTime)
    {
        return;
    }

    m_lastTime = fTime;
    if (fFps != m_lastFPS)
    {
        m_frameWindow->setRange(0, fFps-1);
        m_lastFPS = fFps;
    }

    int nMins = (int)(fTime / 60.0f);
    fTime -= (float)(nMins * 60);
    int nSecs = (int)fTime;
    fTime -= (float)nSecs;
    int nMillis = fTime * 1000.0f;
    int nFrames = (int)(fTime / (1.0f / CLAMP(fFps, FLT_EPSILON, FLT_MAX)));

    m_updating = true;
    m_timeWindow->setTime(QTime(0, nMins, nSecs,nMillis));
    m_frameWindow->setValue(nFrames);
    m_updating = false;
}

void CSequencerDopeSheetToolbar::OnTimeChanged(const QTime &time)
{
    if (m_updating)
    {
        return;
    }

    float fTime = time.second();
    fTime += time.minute() * 60.f;
    fTime += time.msec() / 1000.f;
    emit TimeChanged(fTime);
}

void CSequencerDopeSheetToolbar::OnFrameChanged(int value)
{
    if (m_updating || m_lastFPS <= 0)
    {
        return;
    }

    float fTime = int(m_lastTime);
    fTime += value / m_lastFPS;

    emit TimeChanged(fTime);
}

void CSequencerDopeSheetToolbar::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    QBoxLayout* l = qobject_cast<QBoxLayout*>(layout());
    if (l && l->indexOf(m_timeCodeContainer) == -1)
    {
        l->insertWidget(0, m_timeCodeContainer);
    }
}

#include <Mannequin/SequencerDopeSheetToolbar.moc>