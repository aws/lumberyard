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

#ifndef CRYINCLUDE_EDITOR_VISUALLOGVIEWER_VISUALLOGCONTROLS_H
#define CRYINCLUDE_EDITOR_VISUALLOGVIEWER_VISUALLOGCONTROLS_H
#pragma once

#include <QWidget>
#include <QScopedPointer>

namespace Ui {
    class CVisualLogDialog;
}

struct SVLogCommonData;

// VisualLogDialog
class CVisualLogDialog
    : public QWidget
{
    Q_OBJECT

public:             // Construction & destruction
    CVisualLogDialog(QWidget* parent = nullptr);
    virtual ~CVisualLogDialog();

public:         // Operations
    void SetCommonData(SVLogCommonData* pCD) { m_pCD = pCD; }
    void EnablePlayerButtons();

protected:
    void showEvent(QShowEvent* event) override;
    void timerEvent(QTimerEvent* event) override;
    void OnInitDialog();

signals:
    void BackgroundChanged();
    void FolderChanged();
    void FrameChanged();
    void InvalidateViews();
    void UpdateTextChanged(bool state);
    void UpdateImagesChanged(bool state);
    void KeepAspectChanged(bool state);

public slots:
    void FolderValid(bool validity);

protected slots:
    void OnBackgroundClicked();
    void OnFolderClicked();
    void OnUpdateTextStateChanged(int state);
    void OnUpdateImagesStateChanged(int state);
    void OnKeepAspectStateChanged(int state);
    void OnSpeedSliderMoved(int value);
    void OnFrameSliderMoved(int value);
    void OnPlayerFirst();
    void OnPlayerPrev();
    void OnPlayerPlay();
    void OnPlayerStop();
    void OnPlayerNext();
    void OnPlayerLast();

private:        // Member data
    int                                 m_nTimerVar;

    SVLogCommonData* m_pCD;

    bool m_initialized;
    int m_timerId;
    QScopedPointer<Ui::CVisualLogDialog> m_ui;
};

#endif // CRYINCLUDE_EDITOR_VISUALLOGVIEWER_VISUALLOGCONTROLS_H