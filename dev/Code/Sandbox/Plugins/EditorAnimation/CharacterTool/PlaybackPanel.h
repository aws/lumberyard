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

#include <QWidget>
#include <QtGui/QIcon>
#include <vector>
#include <memory>

class QPushButton;
class QComboBox;
class QLineEdit;
class QDoubleSpinBox;
class QLabel;
class CTimeline;
class QKeySequence;

struct STimelineContent;

namespace Serialization
{
    class IArchive;
}

namespace CharacterTool
{
    using std::vector;
    using std::unique_ptr;

    struct AnimEventPreset;
    struct ExplorerEntry;
    struct ExplorerEntryModifyEvent;
    struct System;
    class AnimEventPresetPanel;

    enum ETimeUnits
    {
        TIME_IN_SECONDS,
        TIME_IN_FRAMES,
        TIME_NORMALIZED
    };

    class PlaybackPanel
        : public QWidget
    {
        Q_OBJECT
    public:
        PlaybackPanel(QWidget* parent, System* system, AnimEventPresetPanel* presetPalette);
        virtual ~PlaybackPanel();

        void Serialize(Serialization::IArchive& ar);

        bool HandleKeyEvent(int key);
        bool ProcessesKey(const QKeySequence& keySequence);

    public slots:
        void PutAnimEvent(const AnimEventPreset& preset);
        void OnPresetsChanged();

    protected slots:
        void OnTimelineScrub(bool scrubThrough);
        void OnTimelineChanged(bool continuousChange);
        void OnTimelineSelectionChanged(bool continuousChange);
        void OnTimelineHotkey(int number);
        void OnTimelineUndo();
        void OnTimelineRedo();
        void OnPlaybackTimeChanged();
        void OnPlaybackStateChanged();
        void OnPlaybackOptionsChanged();
        void OnExplorerEntryModified(ExplorerEntryModifyEvent& ev);
        void OnDocumentActiveAnimationSwitched();

        void OnPlayPausePushed();
        void OnTimeEditEditingFinished();
        void OnTimeEditValueChanged(double newTime);
        void OnTimeUnitsChanged(int index);
        void OnTimelinePlay();
        void OnSpeedChanged(int index);
        void OnLoopToggled(bool);
        void OnOptionPlayOnSelection();
        void OnOptionWrapSlider();
        void OnOptionSmoothTimelineSlider();
        void OnOptionFirstFrameAtEnd();
        void OnOptionPlayFromTheStart();
        void OnSubselectionChanged(int layer);

        void OnEventsImport();
        void OnEventsExport();
    private:
        void WriteTimeline();
        void ReadTimeline();

        void UpdateTimeUnitsUI(bool timeChanged, bool durationChanged, bool frameRateChanged);
        float FrameRate() const;
        CTimeline* m_timeline;
        AnimEventPresetPanel* m_presetPanel;
        QDoubleSpinBox* m_timeEdit;
        bool m_timeEditChanging;
        QLabel* m_timeTotalLabel;
        QPushButton* m_playPauseButton;
        QComboBox* m_timeUnitsCombo;
        QComboBox* m_speedCombo;
        ETimeUnits m_timeUnits;
        QPushButton* m_loopButton;
        QPushButton* m_optionsButton;
        QAction* m_actionPlayOnSelection;
        QAction* m_actionWrapSlider;
        QAction* m_actionSmoothTimelineSlider;
        QAction* m_actionFirstFrameAtEnd;
        QAction* m_actionPlayFromTheStart;
        vector<QWidget*> m_activeControls;
        System* m_system;
        unique_ptr<STimelineContent> m_timelineContent;
        vector<int> m_selectedEvents;

        QString m_sAnimationProgressStatusReason;
        QString m_sAnimationName;
        QString m_sAnimationIDName;
        bool m_playing;
        QIcon m_playIcon;
        QIcon m_pauseIcon;

        float m_normalizedTime;
        float m_duration;
        float m_frameRate;
    };
}
