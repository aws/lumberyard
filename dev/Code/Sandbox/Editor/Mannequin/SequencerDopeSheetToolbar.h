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

// Description : A toolbar which contains a time display, for use in
//               mannequin editor.


#ifndef __SequencerDialogToolbar_h__
#define __SequencerDialogToolbar_h__
#pragma once

class CSequencerDopeSheetToolbar;

#include <QWidget>

class QFrame;
class QTimeEdit;
class QSpinBox;

class CSequencerDopeSheetToolbar
    : public QWidget
{
    Q_OBJECT
public:
    CSequencerDopeSheetToolbar(QWidget* parent = nullptr);
    virtual ~CSequencerDopeSheetToolbar();
    virtual void SetTime(float fTime, float fFps);

signals:
    void TimeChanged(float fTime);

protected:
    void showEvent(QShowEvent* event) override;
    void OnTimeChanged(const QTime& time);
    void OnFrameChanged(int value);

    QFrame* m_timeCodeContainer;
    QTimeEdit* m_timeWindow;
    QSpinBox* m_frameWindow;
    float m_lastTime;
    float m_lastFPS;
    bool m_updating;
};

#endif // __SequencerDialogToolbar_h__