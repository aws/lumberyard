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

#include "pch.h"
#include "../EditorCommon/Timeline.h"
#include "../EditorCommon/TimelineContent.h"
#include "PlaybackPanel.h"
#include "CharacterDocument.h"

#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QToolButton>
#include <QMenu>

#include "EntryList.h"
#include "AnimationList.h"
#include "Serialization.h"
#include "Expected.h"
#include "AnimEventPresetPanel.h"
#include "CharacterToolSystem.h"
#include "CharacterGizmoManager.h"

namespace CharacterTool
{
    SERIALIZATION_ENUM_BEGIN(ETimeUnits, "Time Units")
    SERIALIZATION_ENUM(TIME_IN_SECONDS, "seconds", "Seconds");
    SERIALIZATION_ENUM(TIME_IN_FRAMES, "frames", "Frames");
    SERIALIZATION_ENUM(TIME_NORMALIZED, "normalized", "Normalized");
    SERIALIZATION_ENUM_END()

    static SEntry<AnimationContent>* GetActiveAnimationEntry(System * system)
    {
        ExplorerEntry* activeAnimationEntry = system->document->GetActiveAnimationEntry();
        if (!activeAnimationEntry)
        {
            return 0;
        }
        return system->animationList->GetEntry(activeAnimationEntry->id);
    }

    static float gPlaybackSpeeds[] = { 0.01f, 0.1f, 0.25f, 0.50f, 1.0f, 1.5f, 2.0f, 10.0f };

    PlaybackPanel::PlaybackPanel(QWidget* parent, System* system, AnimEventPresetPanel* presetPanel)
        : QWidget(parent)
        , m_playing(false)
        , m_system(system)
        , m_normalizedTime(0.0f)
        , m_duration(1.0f)
        , m_frameRate(DEFAULT_FRAME_RATE)
        , m_timeEditChanging(false)
        , m_timeUnits(TIME_IN_SECONDS)
        , m_timelineContent(new STimelineContent())
        , m_presetPanel(presetPanel)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

        EXPECTED(connect(m_system->document.get(), SIGNAL(SignalPlaybackTimeChanged()), SLOT(OnPlaybackTimeChanged())));
        EXPECTED(connect(m_system->document.get(), SIGNAL(SignalPlaybackStateChanged()), SLOT(OnPlaybackStateChanged())));
        EXPECTED(connect(m_system->document.get(), SIGNAL(SignalPlaybackOptionsChanged()), SLOT(OnPlaybackOptionsChanged())));
        EXPECTED(connect(m_system->document.get(), SIGNAL(SignalActiveAnimationSwitched()), SLOT(OnDocumentActiveAnimationSwitched())));
        EXPECTED(connect(m_system->explorer.get(), SIGNAL(SignalEntryModified(ExplorerEntryModifyEvent &)), SLOT(OnExplorerEntryModified(ExplorerEntryModifyEvent &))));
        EXPECTED(connect(m_system->characterGizmoManager.get(), SIGNAL(SignalSubselectionChanged(int)), SLOT(OnSubselectionChanged(int))));

        EXPECTED(connect(presetPanel, SIGNAL(SignalPutEvent(const AnimEventPreset&)), this, SLOT(PutAnimEvent(const AnimEventPreset&))));
        EXPECTED(connect(presetPanel, SIGNAL(SignalPresetsChanged()), this, SLOT(OnPresetsChanged())));

        QBoxLayout* topHBox = new QBoxLayout(QBoxLayout::LeftToRight);
        {
            {
                QBoxLayout* vbox = new QBoxLayout(QBoxLayout::TopToBottom);
                vbox->setSpacing(2);
                topHBox->addLayout(vbox);
                {
                    QBoxLayout* hbox = new QBoxLayout(QBoxLayout::LeftToRight);

                    hbox->addWidget(m_timeEdit = new QDoubleSpinBox(), 1);
                    EXPECTED(connect(m_timeEdit, SIGNAL(valueChanged(double)), this, SLOT(OnTimeEditValueChanged(double))));
                    EXPECTED(connect(m_timeEdit, SIGNAL(editingFinished()), this, SLOT(OnTimeEditEditingFinished())));
                    m_activeControls.push_back(m_timeEdit);

                    hbox->addWidget(m_timeTotalLabel = new QLabel(), 0);
                    m_timeTotalLabel->setText(" / 1.0 ");
                    m_activeControls.push_back(m_timeTotalLabel);

                    hbox->addWidget(m_timeUnitsCombo = new QComboBox(), 0);
                    m_timeUnitsCombo->addItem("Seconds");
                    m_timeUnitsCombo->addItem("Frames");
                    m_timeUnitsCombo->addItem("Normalized");
                    m_timeUnitsCombo->setCurrentIndex(0);
                    EXPECTED(connect(m_timeUnitsCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(OnTimeUnitsChanged(int))));
                    vbox->addLayout(hbox);
                    m_activeControls.push_back(m_timeUnitsCombo);

                    hbox->addWidget(m_speedCombo = new QComboBox(), 0);
                    for (size_t i = 0; i < sizeof(gPlaybackSpeeds) / sizeof(gPlaybackSpeeds[0]); ++i)
                    {
                        float speed = gPlaybackSpeeds[i];
                        QString str;
                        str.sprintf("%.2fx", speed);
                        m_speedCombo->addItem(str);
                    }
                    EXPECTED(connect(m_speedCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(OnSpeedChanged(int))));
                    m_activeControls.push_back(m_speedCombo);
                }

                {
                    QBoxLayout* hbox = new QBoxLayout(QBoxLayout::LeftToRight);
                    hbox->setSpacing(4);
                    vbox->addLayout(hbox);

                    hbox->addWidget(m_playPauseButton = new QPushButton("Play"), 1);
                    connect(m_playPauseButton, SIGNAL(clicked()), this, SLOT(OnPlayPausePushed()));
                    m_playPauseButton->setIcon(m_playIcon);
                    m_playPauseButton->setEnabled(false);
                    m_activeControls.push_back(m_playPauseButton);

                    m_loopButton = new QPushButton("Loop", this);
                    m_loopButton->setCheckable(true);
                    m_loopButton->setChecked(m_system->document->GetPlaybackOptions().loopAnimation);
                    connect(m_loopButton, SIGNAL(toggled(bool)), this, SLOT(OnLoopToggled(bool)));
                    hbox->addWidget(m_loopButton, 0);
                    m_activeControls.push_back(m_loopButton);

                    m_optionsButton = new QPushButton(QIcon("Editor/Icons/animation/playback_options.png"), QString(), this);
                    {
                        QMenu* optionsMenu = new QMenu(this);
                        m_optionsButton->setMenu(optionsMenu);

                        m_actionPlayOnSelection = optionsMenu->addAction("Play when animation selected", this, SLOT(OnOptionPlayOnSelection()));
                        m_actionPlayOnSelection->setCheckable(true);

                        m_actionPlayFromTheStart = optionsMenu->addAction("Always play from the begining", this, SLOT(OnOptionPlayFromTheStart()));
                        m_actionPlayFromTheStart->setCheckable(true);

                        m_actionFirstFrameAtEnd = optionsMenu->addAction("First frame at the end of timeline", this, SLOT(OnOptionFirstFrameAtEnd()));
                        m_actionFirstFrameAtEnd->setCheckable(true);

                        m_actionWrapSlider = optionsMenu->addAction("Wrap timeline slider", this, SLOT(OnOptionWrapSlider()));
                        m_actionWrapSlider->setCheckable(true);

                        m_actionSmoothTimelineSlider = optionsMenu->addAction("Smooth timeline slider", this, SLOT(OnOptionSmoothTimelineSlider()));
                        m_actionSmoothTimelineSlider->setCheckable(true);
                    }
                    m_activeControls.push_back(m_optionsButton);
                    hbox->addWidget(m_optionsButton, 0);
                }
            }

            m_timeline = new CTimeline(this);
            m_timeline->SetContent(m_timelineContent.get());
            connect(m_timeline, SIGNAL(SignalScrub(bool)), this, SLOT(OnTimelineScrub(bool)));
            connect(m_timeline, SIGNAL(SignalSelectionChanged(bool)), this, SLOT(OnTimelineSelectionChanged(bool)));
            connect(m_timeline, SIGNAL(SignalContentChanged(bool)), this, SLOT(OnTimelineChanged(bool)));
            connect(m_timeline, SIGNAL(SignalPlay()), this, SLOT(OnTimelinePlay()));
            connect(m_timeline, SIGNAL(SignalNumberHotkey(int)), this, SLOT(OnTimelineHotkey(int)));
            connect(m_timeline, SIGNAL(SignalUndo()), this, SLOT(OnTimelineUndo()));
            connect(m_timeline, SIGNAL(SignalRedo()), this, SLOT(OnTimelineRedo()));
            m_playIcon = QIcon("Editor/Icons/animation/play.png");
            m_pauseIcon = QIcon("Editor/Icons/animation/pause.png");
            m_timeline->SetSizeToContent(true);
            topHBox->addWidget(m_timeline, 1);
            m_activeControls.push_back(m_timeline);

            setLayout(topHBox);
        }

        OnPlaybackOptionsChanged();
        OnPlaybackStateChanged();
        UpdateTimeUnitsUI(true, true, true);

        WriteTimeline();
    }

    PlaybackPanel::~PlaybackPanel()
    {
    }

    void PlaybackPanel::OnEventsImport()
    {
    }

    void PlaybackPanel::OnEventsExport()
    {
    }

    void PlaybackPanel::OnTimelineScrub(bool scrubThrough)
    {
        float newTime = m_timeline->Time().ToFloat() * m_duration;
        m_system->document->ScrubTime(newTime, scrubThrough);
    }

    void PlaybackPanel::OnTimelineUndo()
    {
        ExplorerEntries entries(1, m_system->document->GetActiveAnimationEntry());
        if (!entries[0])
        {
            return;
        }
        m_system->explorer->Undo(entries, 1);
    }

    void PlaybackPanel::OnTimelineRedo()
    {
        ExplorerEntries entries(1, m_system->document->GetActiveAnimationEntry());
        if (!entries[0])
        {
            return;
        }
        m_system->explorer->Redo(entries);
    }

    void PlaybackPanel::OnTimelineSelectionChanged(bool continuous)
    {
        m_selectedEvents.clear();
        STimelineContent& content = *m_timeline->Content();
        const STimelineTrack& track = *content.track.tracks[0];

        for (size_t i = 0; i < track.elements.size(); ++i)
        {
            const STimelineElement& element = track.elements[i];
            if (element.selected)
            {
                m_selectedEvents.push_back(aznumeric_cast<int>(element.userId - 1));
            }
        }

        SEntry<AnimationContent>* animation = GetActiveAnimationEntry(m_system);
        if (!animation)
        {
            return;
        }

        vector<const void*> handles;
        handles.reserve(m_selectedEvents.size());
        for (size_t i = 0; i < m_selectedEvents.size(); ++i)
        {
            size_t index = m_selectedEvents[i];
            if (index >= animation->content.events.size())
            {
                continue;
            }
            handles.push_back(&animation->content.events[index]);
        }

        m_system->characterGizmoManager->SetSubselection(GIZMO_LAYER_ANIMATION, handles);

        ExplorerEntries selection(1, m_system->document->GetActiveAnimationEntry());
        if (selection[0])
        {
            ExplorerEntries currentlySelected;
            m_system->document->GetSelectedExplorerEntries(&currentlySelected);
            if (currentlySelected.size() != 1 || currentlySelected[0] != selection[0])
            {
                m_system->document->SetSelectedExplorerEntries(selection, CharacterDocument::SELECT_DO_NOT_REWIND);
            }
        }
    }

    void PlaybackPanel::OnTimelineChanged(bool continuous)
    {
        ReadTimeline();
        m_system->explorer->CheckIfModified(m_system->document->GetActiveAnimationEntry(), "Anim Event Change", continuous);
    }

    void PlaybackPanel::OnTimelinePlay()
    {
        OnPlayPausePushed();
    }

    void PlaybackPanel::OnTimelineHotkey(int number)
    {
        const AnimEventPreset* preset = m_presetPanel->GetPresetByHotkey(number);
        if (!preset)
        {
            return;
        }
        PutAnimEvent(*preset);
    }

    static void SelectElementsById(STimelineTrack* track, const vector<int>& userIds)
    {
        for (size_t i = 0; i < track->elements.size(); ++i)
        {
            STimelineElement& e = track->elements[i];
            bool selected = std::find(userIds.begin(), userIds.end(), e.userId) != userIds.end();
            e.selected = selected;
        }
    }

    void PlaybackPanel::OnTimeEditEditingFinished()
    {
        if (m_timeEditChanging)
        {
            return;
        }
        float newTime = float(m_timeEdit->value());
        m_system->document->ScrubTime(newTime, false);
    }

    void PlaybackPanel::OnTimeEditValueChanged(double newValue)
    {
        if (m_timeEditChanging)
        {
            return;
        }

        double time = 0.0;

        switch (m_timeUnits)
        {
        case TIME_IN_FRAMES:
            time = newValue / FrameRate();
            break;
        case TIME_NORMALIZED:
            time = newValue * m_duration;
            break;
        case TIME_IN_SECONDS:
            time = newValue;
            break;
        }
        m_system->document->ScrubTime(float(time), false);
    }

    void PlaybackPanel::OnTimeUnitsChanged(int index)
    {
        m_timeUnits = (ETimeUnits)index;

        UpdateTimeUnitsUI(true, true, true);
    }

    void PlaybackPanel::OnSpeedChanged(int index)
    {
        float speed = 1.0f;
        if (index >= 0 && index < sizeof(gPlaybackSpeeds) / sizeof(gPlaybackSpeeds[0]))
        {
            speed = gPlaybackSpeeds[index];
        }
        m_system->document->GetPlaybackOptions().playbackSpeed = speed;
        m_system->document->PlaybackOptionsChanged();
    }

    void PlaybackPanel::OnPlayPausePushed()
    {
        if (m_playing)
        {
            m_system->document->Pause();
        }
        else
        {
            m_system->document->Play();
        }
    }

    void PlaybackPanel::OnLoopToggled(bool loop)
    {
        m_system->document->GetPlaybackOptions().loopAnimation = loop;
        m_system->document->PlaybackOptionsChanged();
    }

    void PlaybackPanel::OnOptionPlayOnSelection()
    {
        m_system->document->GetPlaybackOptions().playOnAnimationSelection = !m_system->document->GetPlaybackOptions().playOnAnimationSelection;
        m_system->document->PlaybackOptionsChanged();
    }

    void PlaybackPanel::OnOptionPlayFromTheStart()
    {
        m_system->document->GetPlaybackOptions().playFromTheStart = !m_system->document->GetPlaybackOptions().playFromTheStart;
        m_system->document->PlaybackOptionsChanged();
    }

    void PlaybackPanel::OnOptionWrapSlider()
    {
        m_system->document->GetPlaybackOptions().wrapTimelineSlider = !m_system->document->GetPlaybackOptions().wrapTimelineSlider;
        m_system->document->PlaybackOptionsChanged();
    }

    void PlaybackPanel::OnOptionSmoothTimelineSlider()
    {
        m_system->document->GetPlaybackOptions().smoothTimelineSlider = !m_system->document->GetPlaybackOptions().smoothTimelineSlider;
        m_system->document->PlaybackOptionsChanged();
    }

    void PlaybackPanel::OnOptionFirstFrameAtEnd()
    {
        m_system->document->GetPlaybackOptions().firstFrameAtEndOfTimeline = !m_system->document->GetPlaybackOptions().firstFrameAtEndOfTimeline;
        m_system->document->PlaybackOptionsChanged();
    }


    void PlaybackPanel::OnPlaybackOptionsChanged()
    {
        const PlaybackOptions& options = m_system->document->GetPlaybackOptions();
        m_loopButton->setChecked(options.loopAnimation);
        m_timeEdit->setWrapping(options.loopAnimation);
        m_timeline->SetCycled(options.loopAnimation && options.wrapTimelineSlider);

        m_actionPlayOnSelection->setChecked(options.playOnAnimationSelection);
        m_actionPlayFromTheStart->setChecked(options.playFromTheStart);
        m_actionWrapSlider->setEnabled(options.loopAnimation);
        m_actionWrapSlider->setChecked(options.wrapTimelineSlider);
        m_actionSmoothTimelineSlider->setChecked(options.smoothTimelineSlider);
        m_actionFirstFrameAtEnd->setChecked(options.firstFrameAtEndOfTimeline);

        {
            size_t numSpeeds = sizeof(gPlaybackSpeeds) / sizeof(gPlaybackSpeeds[0]);
            int selectedSpeedIndex = 0;
            for (size_t i = 0; i < numSpeeds; ++i)
            {
                float speed = gPlaybackSpeeds[i];
                if (fabsf(speed - options.playbackSpeed) < 1e-5f)
                {
                    selectedSpeedIndex = i;
                }
            }
            m_speedCombo->setCurrentIndex(selectedSpeedIndex);
        }
    }

    void PlaybackPanel::OnDocumentActiveAnimationSwitched()
    {
        WriteTimeline();
    }

    void PlaybackPanel::OnPlaybackTimeChanged()
    {
        float time = m_system->document->PlaybackTime();
        float duration = m_system->document->PlaybackDuration();
        float frameRate = m_system->document->FrameRate();
        float normalizedTime = duration != 0.0f ? time / duration : time;

        bool timeChanged = false;
        if (m_normalizedTime != normalizedTime)
        {
            m_normalizedTime = normalizedTime;
            timeChanged = true;
        }

        bool durationChanged = false;
        if (m_duration != duration)
        {
            m_duration = duration;
            durationChanged = true;
        }

        bool frameRateChanged = false;
        if (m_frameRate != frameRate)
        {
            m_frameRate = frameRate;
            frameRateChanged = true;
        }

        UpdateTimeUnitsUI(timeChanged, durationChanged, frameRateChanged);
    }

    void PlaybackPanel::OnPlaybackStateChanged()
    {
        PlaybackState state = m_system->document->GetPlaybackState();

        bool controlsEnabled = true;
        if (state == PLAYBACK_PLAY)
        {
            m_playPauseButton->setText("Pause");
            controlsEnabled = true;
            m_playing = true;
        }
        else if (state == PLAYBACK_PAUSE)
        {
            m_playPauseButton->setText("Play");
            controlsEnabled = true;
            m_playing = false;
        }
        else
        {
            m_playPauseButton->setText("Play");
            controlsEnabled = false;
            m_playing = false;
        }

        const char* tooltip = state == PLAYBACK_UNAVAILABLE ? m_system->document->PlaybackBlockReason() : "";
        for (size_t i = 0; i < m_activeControls.size(); ++i)
        {
            QWidget* w = m_activeControls[i];
            w->setEnabled(controlsEnabled);
            w->setToolTip(tooltip);
        }
    }

    void PlaybackPanel::OnExplorerEntryModified(ExplorerEntryModifyEvent& ev)
    {
        if ((ev.entryParts & ENTRY_PART_CONTENT) == 0)
        {
            return;
        }
        if (ev.entry == m_system->document->GetActiveAnimationEntry())
        {
            WriteTimeline();
        }
    }

    float PlaybackPanel::FrameRate() const
    {
        return m_frameRate;
    }

    void PlaybackPanel::UpdateTimeUnitsUI(bool timeChanged, bool durationChanged, bool frameRateChanged)
    {
        float time = m_normalizedTime;
        float duration = m_duration;
        int numFrames = int(duration * FrameRate() + 0.5f);

        if (durationChanged || frameRateChanged)
        {
            double minimum = 0.0;
            double maximum = 1.0;
            double step = 1.0;

            double totalDisplay = 0.0;

            switch (m_timeUnits)
            {
            case TIME_IN_SECONDS:
                minimum = 0.0;
                maximum = duration;
                step = 1.0 / double(FrameRate());
                totalDisplay = duration;
                break;
            case TIME_NORMALIZED:
                minimum = 0.0;
                maximum = 1.0;
                totalDisplay = 1.0;
                step = 0.01;
                break;
            case TIME_IN_FRAMES:
                minimum = 0.0;
                maximum = (double)numFrames;
                totalDisplay = numFrames;
                step = 1.0;
                break;
            }

            if (duration >= 0.0f)
            {
                QString str;
                if (m_timeUnits == TIME_IN_FRAMES)
                {
                    str.sprintf("/ %i ", int(totalDisplay + 0.5));
                }
                else
                {
                    str.sprintf("/ %.2f ", totalDisplay);
                }
                m_timeTotalLabel->setText(str);
            }
            else
            {
                m_timeTotalLabel->setText("/ 0.0 ");
            }


            m_timeEditChanging = true;
            m_timeEdit->setMinimum(minimum);
            m_timeEdit->setMaximum(maximum);
            m_timeEdit->setSingleStep(step);
            m_timeEdit->setDecimals(m_timeUnits == TIME_IN_FRAMES ? 0 : 2);
            m_timeEditChanging = false;

            m_timeline->SetTimeUnitScale(aznumeric_cast<float>(totalDisplay), aznumeric_cast<float>(step));
        }

        if (timeChanged)
        {
            float normalizedTime = m_normalizedTime;
            ;
            if (!m_timeline->IsDragged())
            {
                m_timeline->SetTime(SAnimTime(normalizedTime));
            }
            m_timeline->SetTimeSnapping(m_timeUnits == TIME_IN_FRAMES);

            if (!m_timeEdit->hasFocus())
            {
                double displayTime = 0.0;
                switch (m_timeUnits)
                {
                case TIME_IN_SECONDS:
                    displayTime = (double)normalizedTime * m_duration;
                    break;
                case TIME_NORMALIZED:
                    displayTime = normalizedTime;
                    break;
                case TIME_IN_FRAMES:
                    displayTime = normalizedTime * numFrames;
                    break;
                }

                m_timeEditChanging = true;
                m_timeEdit->setValue(displayTime);
                m_timeEditChanging = false;
            }
        }
    }

    void PlaybackPanel::Serialize(Serialization::IArchive& ar)
    {
        ar(m_timeUnits, "timeUnits");
        if (ar.IsInput())
        {
            UpdateTimeUnitsUI(true, true, true);
            m_timeUnitsCombo->setCurrentIndex(int(m_timeUnits));
        }
    }


    void PlaybackPanel::WriteTimeline()
    {
        m_timelineContent->track.tracks.resize(1, new STimelineTrack());
        STimelineTrack& track = *m_timelineContent->track.tracks[0];
        track.startTime = SAnimTime(0.0f);
        track.endTime = SAnimTime(1.0f);
        track.height = 24;

        SEntry<AnimationContent>* animation = GetActiveAnimationEntry(m_system);
        int numEvents = animation ? animation->content.events.size() : 0;

        STimelineElements& elements = track.elements;
        vector<int> selectedIds;
        {
            int newSelectedElementIndex = 0;
            for (size_t i = 0; i < elements.size(); ++i)
            {
                if (elements[i].selected)
                {
                    if (elements[i].userId)
                    {
                        selectedIds.push_back(aznumeric_cast<int>(elements[i].userId));
                    }
                    else
                    {
                        selectedIds.push_back(numEvents - newSelectedElementIndex);
                        ++newSelectedElementIndex;
                    }
                }
            }
        }
        elements.clear();

        const EventContentToColorMap& eventContentToColor = m_presetPanel->GetEventContentToColorMap();

        if (animation)
        {
            for (size_t i = 0; i < animation->content.events.size(); ++i)
            {
                const AnimEvent& ev = animation->content.events[i];

                STimelineElement e;
                e.start = SAnimTime(ev.startTime);
                e.userId = i + 1;
                e.selected = std::find(selectedIds.begin(), selectedIds.end(), e.userId) != selectedIds.end();

                AnimEvent eventCopy = ev;
                eventCopy.startTime = -1.0f;
                eventCopy.endTime = -1.0f;
                SerializeToMemory(&e.userSideLoad, Serialization::SStruct(eventCopy));
                unsigned int hash = CCrc32::Compute(e.userSideLoad.data(), e.userSideLoad.size());
                EventContentToColorMap::const_iterator it = std::lower_bound(eventContentToColor.begin(), eventContentToColor.end(), std::make_pair(hash, ColorB()),
                        [&](const std::pair<unsigned int, ColorB>& a, const std::pair<unsigned int, ColorB>& b) { return a.first < b.first; }
                        );
                if (it != eventContentToColor.end() && it->first == hash)
                {
                    e.color = it->second;
                }
                else
                {
                    e.color = ColorB(255, 255, 255, 255);
                }

                elements.push_back(e);
            }
        }
        m_timeline->ContentUpdated();
    }

    void PlaybackPanel::ReadTimeline()
    {
        SEntry<AnimationContent>* animation = GetActiveAnimationEntry(m_system);
        if (!animation)
        {
            return;
        }

        vector<int> removedElements;

        if (m_timelineContent->track.tracks.empty() || !m_timelineContent->track.tracks[0])
        {
            return;
        }
        const STimelineTrack& track = *m_timelineContent->track.tracks[0];
        for (size_t i = 0; i < track.elements.size(); ++i)
        {
            const STimelineElement& e = track.elements[i];
            size_t index = size_t(e.userId - 1);
            if (e.added)
            {
                AnimEvent newEvent;
                newEvent.startTime = e.start.ToFloat();
                SerializeFromMemory(Serialization::SStruct(newEvent), e.userSideLoad);
                animation->content.events.push_back(newEvent);
                index = animation->content.events.size() - 1;
            }
            if (e.deleted)
            {
                removedElements.push_back(i);
            }
            if (index < animation->content.events.size())
            {
                AnimEvent& ev = animation->content.events[index];
                if (e.sideLoadChanged)
                {
                    SerializeFromMemory(Serialization::SStruct(ev), e.userSideLoad);
                }
                ev.startTime = e.start.ToFloat();
            }
        }

        std::sort(removedElements.begin(), removedElements.end());
        for (int i = int(removedElements.size() - 1); i >= 0; --i)
        {
            animation->content.events.erase(animation->content.events.begin() + removedElements[i]);
        }
    }

    void PlaybackPanel::PutAnimEvent(const AnimEventPreset& preset)
    {
        SEntry<AnimationContent>* animation = GetActiveAnimationEntry(m_system);
        if (!animation)
        {
            return;
        }

        AnimEvent ev = preset.event;
        ev.startTime = ev.endTime = m_timeline->Time().ToFloat();
        int index = animation->content.events.size();
        animation->content.events.push_back(ev);
        m_system->explorer->CheckIfModified(m_system->document->GetActiveAnimationEntry(), "Add AnimEvent", false);

        vector<int> selection(1, index + 1);
        SelectElementsById(m_timelineContent->track.tracks[0], selection);
        m_timeline->ContentUpdated();
        m_timeline->setFocus();
    }

    bool PlaybackPanel::HandleKeyEvent(int key)
    {
        return m_timeline->HandleKeyEvent(key);
    }

    bool PlaybackPanel::ProcessesKey(const QKeySequence& keySequence)
    {
        // We let the timeline handle key events; let it tell us if it's interested in this key sequence as well
        // If we don't do this, shortcuts may take the key event before it gets here
        return m_timeline->ProcessesKey(keySequence);
    }

    void PlaybackPanel::OnPresetsChanged()
    {
        WriteTimeline();
    }

    void PlaybackPanel::OnSubselectionChanged(int layer)
    {
        if (layer != GIZMO_LAYER_ANIMATION)
        {
            return;
        }
        const vector<const void*>& handles = m_system->characterGizmoManager->Subselection(GIZMO_LAYER_ANIMATION);

        SEntry<AnimationContent>* animation = GetActiveAnimationEntry(m_system);
        if (!animation)
        {
            return;
        }
        if (animation->content.events.empty())
        {
            return;
        }

        const AnimEvent* events = &animation->content.events[0];
        const AnimEvent* eventsEnd = events + animation->content.events.size();

        std::vector<int> newSelection;
        for (size_t i = 0; i < handles.size(); ++i)
        {
            const AnimEvent* handle = (const AnimEvent*)handles[i];
            if (handle >= events && handle < eventsEnd)
            {
                int index = aznumeric_cast<int>(handle - events);
                int userId = index + 1;
                newSelection.push_back(userId);
            }
        }
        SelectElementsById(m_timeline->Content()->track.tracks[0].get(), newSelection);
        m_timeline->ContentUpdated();
    }
}

#include <CharacterTool/PlaybackPanel.moc>
