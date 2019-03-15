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

#include "RecorderWidget.h"
#include "AnimGraphPlugin.h"
#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMessageBox>
#include <EMotionFX/Source/Recorder.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <MCore/Source/LogManager.h>
#include "../TimeView/TimeViewPlugin.h"
#include "../TimeView/TimeInfoWidget.h"
#include "../TimeView/TrackDataHeaderWidget.h"
#include "../TimeView/TrackDataWidget.h"
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MetricsEventSender.h>


namespace EMStudio
{
    // constructor
    RecorderWidget::RecorderWidget(AnimGraphPlugin* plugin, QWidget* parent)
        : QWidget(parent)
    {
        EMotionFX::RecorderNotificationBus::Handler::BusConnect();
        mPlugin = plugin;

        // add the buttons to add, remove and clear the motions
        QHBoxLayout* buttonsLayout = new QHBoxLayout();
        buttonsLayout->setSpacing(0);
        buttonsLayout->setMargin(0);
        buttonsLayout->setAlignment(Qt::AlignLeft);
        setLayout(buttonsLayout);

        // create the buttons
        mRecordButton       = new QPushButton();
        mClearButton        = new QPushButton();
        mFirstFrameButton   = new QPushButton();
        mPrevFrameButton    = new QPushButton();
        mPlayButton         = new QPushButton();
        mLastFrameButton    = new QPushButton();
        mNextFrameButton    = new QPushButton();
        mOpenButton         = nullptr;
        mSaveButton         = nullptr;
        mConfigButton       = nullptr;
        //mOpenButton       = new QPushButton();
        //mSaveButton       = new QPushButton();
        //mConfigButton     = new QPushButton();

        EMStudioManager::MakeTransparentButton(mRecordButton,          "/Images/Icons/RecordButton.png",   "Start recording");
        EMStudioManager::MakeTransparentButton(mClearButton,           "/Images/Icons/Clear.png",          "Clear recording");

        EMStudioManager::MakeTransparentButton(mFirstFrameButton,      "/Images/Icons/SkipBackward.png",   "First frame");
        EMStudioManager::MakeTransparentButton(mPrevFrameButton,       "/Images/Icons/SeekBackward.png",   "Previous frame");
        EMStudioManager::MakeTransparentButton(mPlayButton,            "/Images/Icons/PlayForward.png",    "Playback recording");
        EMStudioManager::MakeTransparentButton(mNextFrameButton,       "/Images/Icons/SeekForward.png",    "Next frame");
        EMStudioManager::MakeTransparentButton(mLastFrameButton,       "/Images/Icons/SkipForward.png",    "Last frame");

        //EMStudioManager::MakeTransparentButton( mOpenButton,          "/Images/Menu/FileOpen.png",        "Open a recording" );
        //EMStudioManager::MakeTransparentButton( mSaveButton,          "/Images/Menu/FileSave.png",        "Save the current recording" );
        //EMStudioManager::MakeTransparentButton( mConfigButton,        "/Images/Icons/Edit.png",           "Config the recorder" );

        buttonsLayout->addWidget(mRecordButton);

        QWidget* spacerWidget = new QWidget();
        spacerWidget->setObjectName("TransparentWidget");
        spacerWidget->setFixedSize(10, 1);
        spacerWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
        buttonsLayout->addWidget(spacerWidget);

        buttonsLayout->addWidget(mFirstFrameButton);
        buttonsLayout->addWidget(mPrevFrameButton);
        buttonsLayout->addWidget(mPlayButton);
        buttonsLayout->addWidget(mNextFrameButton);
        buttonsLayout->addWidget(mLastFrameButton);

        spacerWidget = new QWidget();
        spacerWidget->setObjectName("TransparentWidget");
        spacerWidget->setFixedSize(10, 1);
        spacerWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
        buttonsLayout->addWidget(spacerWidget);

        buttonsLayout->addWidget(mClearButton);

        //buttonsLayout->addWidget(mOpenButton);
        //buttonsLayout->addWidget(mSaveButton);
        //buttonsLayout->addWidget(mConfigButton);

        connect(mRecordButton, &QPushButton::released, this, &RecorderWidget::OnRecordButton);
        connect(mPlayButton, &QPushButton::released, this, &RecorderWidget::OnPlayButton);
        connect(mFirstFrameButton, &QPushButton::released, this, &RecorderWidget::OnFirstFrameButton);
        connect(mPrevFrameButton, &QPushButton::released, this, &RecorderWidget::OnPrevFrameButton);
        connect(mNextFrameButton, &QPushButton::released, this, &RecorderWidget::OnNextFrameButton);
        connect(mLastFrameButton, &QPushButton::released, this, &RecorderWidget::OnLastFrameButton);
        connect(mClearButton, &QPushButton::released, this, &RecorderWidget::OnClearButton);

        //connect(mOpenButton, SIGNAL(released()), this, SLOT(OnOpenButton()));
        //connect(mSaveButton, SIGNAL(released()), this, SLOT(OnSaveButton()));
        //connect(mConfigButton, SIGNAL(released()), this, SLOT(OnPlayButton()));

        spacerWidget = new QWidget();
        spacerWidget->setObjectName("TransparentWidget");
        spacerWidget->setFixedSize(10, 1);
        spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
        buttonsLayout->addWidget(spacerWidget);

        QHBoxLayout* filterLayout = new QHBoxLayout();
        filterLayout->setSpacing(6);
        filterLayout->addWidget(new QLabel("Filter:"));
        mFilterButtonGroup = new MysticQt::ButtonGroup(this, 1, 3);
        mFilterButtonGroup->GetButton(0, 0)->setText("Motions Only");
        mFilterButtonGroup->GetButton(0, 1)->setText("States Only");
        mFilterButtonGroup->GetButton(0, 2)->setText("Events");
        mFilterButtonGroup->GetButton(0, 0)->setChecked(true);
        mFilterButtonGroup->GetButton(0, 1)->setChecked(false);
        mFilterButtonGroup->GetButton(0, 2)->setChecked(true);
        //connect(mFilterButtonGroup->GetButton(0,0), SIGNAL(clicked()), this, SLOT(OnFilterButtonGroup()));
        //connect(mFilterButtonGroup->GetButton(0,1), SIGNAL(clicked()), this, SLOT(OnFilterButtonGroup()));
        mFilterButtonGroup->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
        filterLayout->addWidget(mFilterButtonGroup);
        buttonsLayout->addLayout(filterLayout);

        UpdateButtons();
    }


    // destructor
    RecorderWidget::~RecorderWidget()
    {
    }


    // try to find the time view plugin
    TimeViewPlugin* RecorderWidget::GetTimeViewPlugin() const
    {
        EMStudioPlugin* timeViewBasePlugin = EMStudio::GetPluginManager()->FindActivePlugin(TimeViewPlugin::CLASS_ID);
        if (timeViewBasePlugin)
        {
            return static_cast<TimeViewPlugin*>(timeViewBasePlugin);
        }

        return nullptr;
    }


    // update the signal connections
    void RecorderWidget::UpdateSignalConnections()
    {
        TimeViewPlugin* timeViewPlugin = GetTimeViewPlugin();
        if (timeViewPlugin)
        {
            connect(timeViewPlugin, &TimeViewPlugin::ManualTimeChangeStart, this, &RecorderWidget::OnTimeChangeStart);
        }
    }


    void RecorderWidget::OnRecordingFailed(const AZStd::string& why)
    {
        QMessageBox::critical(this, "EMotionFX recording failed", why.c_str());
    }


    // pressing the record button
    void RecorderWidget::OnRecordButton()
    {
        if (EMotionFX::GetRecorder().GetIsRecording() == false)
        {
            EMotionFX::Recorder::RecordSettings settings;
            settings.mFPS                       = 1000000;
            settings.mRecordTransforms          = true;
            settings.mRecordAnimGraphStates    = true;//false;
            settings.mRecordNodeHistory         = true;
            settings.mRecordScale               = true;
            //settings.mRecordNonUniformScale       = true;
            //settings.mForceMatrixDecompose        = true;
            settings.mInitialAnimGraphAnimBytes = 4 * 1024 * 1024; // 4 mb
            settings.mHistoryStatesOnly         = mFilterButtonGroup->GetButton(0, 1)->isChecked();
            settings.mRecordEvents              = mFilterButtonGroup->GetButton(0, 2)->isChecked();

            if (mFilterButtonGroup->GetButton(0, 0)->isChecked())
            {
                settings.mNodeHistoryTypes.insert(azrtti_typeid<EMotionFX::AnimGraphMotionNode>());
            }

            EMotionFX::GetRecorder().StartRecording(settings);

            UpdateSignalConnections();

            // reinit the time view plugin
            TimeViewPlugin* timeViewPlugin = GetTimeViewPlugin();
            if (timeViewPlugin)
            {
                timeViewPlugin->ReInit();
                timeViewPlugin->SetScale(1.0);
                timeViewPlugin->SetScrollX(0);
            }

            //mFilterButtonGroup->setEnabled(false);

            // Send LyMetrics event.
            MetricsEventSender::SendStartRecorderEvent(EMotionFX::GetRecorder());
        }
        else
        {
            EMotionFX::GetRecorder().StopRecording();
            EMotionFX::GetRecorder().StartPlayBack();
            EMotionFX::GetRecorder().SetAutoPlay(false);
            EMotionFX::GetRecorder().SetCurrentPlayTime(0.0f);

            // reinit the time view plugin
            TimeViewPlugin* timeViewPlugin = GetTimeViewPlugin();
            if (timeViewPlugin)
            {
                timeViewPlugin->ReInit();
                timeViewPlugin->OnZoomAll();
                timeViewPlugin->SetCurrentTime(0.0f);
                timeViewPlugin->GetTrackDataWidget()->setFocus();
                timeViewPlugin->GetTrackDataHeaderWidget()->setFocus();
            }

            //mFilterButtonGroup->setEnabled(true);

            // Send LyMetrics event.
            MetricsEventSender::SendStopRecorderEvent(EMotionFX::GetRecorder());
        }

        // update the recorder buttons
        UpdateButtons();
        emit RecorderStateChanged();
    }


    // when pressing the play button
    void RecorderWidget::OnPlayButton()
    {
        if (EMotionFX::GetRecorder().GetIsInPlayMode() == false)
        {
            EMotionFX::GetRecorder().StartPlayBack();
            EMotionFX::GetRecorder().SetAutoPlay(true);

            UpdateSignalConnections();
        }
        else
        {
            UpdateSignalConnections();

            if (EMotionFX::GetRecorder().GetIsInAutoPlayMode())
            {
                EMotionFX::GetRecorder().SetAutoPlay(false);
            }
            else
            {
                EMotionFX::GetRecorder().SetAutoPlay(true);
            }
        }

        UpdateButtons();
        emit RecorderStateChanged();
    }


    // on the first frame button press
    void RecorderWidget::OnFirstFrameButton()
    {
        if (EMotionFX::GetRecorder().GetIsInPlayMode())
        {
            EMotionFX::GetRecorder().SetCurrentPlayTime(0.0f);
            TimeViewPlugin* timeViewPlugin = GetTimeViewPlugin();
            if (timeViewPlugin)
            {
                timeViewPlugin->SetCurrentTime(EMotionFX::GetRecorder().GetCurrentPlayTime());
                timeViewPlugin->SetRedrawFlag();
            }
        }

        emit RecorderStateChanged();
    }


    // previous frame button pressed
    void RecorderWidget::OnPrevFrameButton()
    {
        const float newTime = MCore::Max<float>(EMotionFX::GetRecorder().GetCurrentPlayTime() - (1.0f / 60.0f), 0.0f);
        EMotionFX::GetRecorder().SetCurrentPlayTime(newTime);

        TimeViewPlugin* timeViewPlugin = GetTimeViewPlugin();
        if (timeViewPlugin)
        {
            timeViewPlugin->SetCurrentTime(newTime);
            timeViewPlugin->GetTimeInfoWidget()->update();
            timeViewPlugin->SetRedrawFlag();
        }

        emit RecorderStateChanged();
    }


    // next frame button pressed
    void RecorderWidget::OnNextFrameButton()
    {
        if (EMotionFX::GetRecorder().GetIsInPlayMode())
        {
            const float newTime = MCore::Min<float>(EMotionFX::GetRecorder().GetCurrentPlayTime() + (1.0f / 60.0f), EMotionFX::GetRecorder().GetRecordTime());
            EMotionFX::GetRecorder().SetCurrentPlayTime(newTime);

            TimeViewPlugin* timeViewPlugin = GetTimeViewPlugin();
            if (timeViewPlugin)
            {
                timeViewPlugin->SetCurrentTime(newTime);
                timeViewPlugin->GetTimeInfoWidget()->update();
                timeViewPlugin->SetRedrawFlag();
            }
        }

        emit RecorderStateChanged();
    }


    // last frame button pressed
    void RecorderWidget::OnLastFrameButton()
    {
        if (EMotionFX::GetRecorder().GetIsInPlayMode())
        {
            EMotionFX::GetRecorder().SetCurrentPlayTime(EMotionFX::GetRecorder().GetRecordTime());
            TimeViewPlugin* timeViewPlugin = GetTimeViewPlugin();
            if (timeViewPlugin)
            {
                timeViewPlugin->SetCurrentTime(EMotionFX::GetRecorder().GetCurrentPlayTime());
                timeViewPlugin->SetRedrawFlag();
            }
        }

        emit RecorderStateChanged();
    }


    // when the clear button is pressed
    void RecorderWidget::OnClearButton()
    {
        EMotionFX::GetRecorder().Clear();
        UpdateButtons();

        // reinit the time view plugin
        TimeViewPlugin* timeViewPlugin = GetTimeViewPlugin();
        if (timeViewPlugin)
        {
            timeViewPlugin->ReInit();
            timeViewPlugin->SetScale(1.0);
            timeViewPlugin->SetScrollX(0);
            timeViewPlugin->SetCurrentTime(0.0f);
        }

        emit RecorderStateChanged();
    }


    // open a recording button pressed
    void RecorderWidget::OnOpenButton()
    {
        UpdateButtons();
    }


    // save recording button pressed
    void RecorderWidget::OnSaveButton()
    {
        UpdateButtons();
    }


    // config button pressed
    void RecorderWidget::OnConfigButton()
    {
    }


    // update the button enable/disable state etc
    void RecorderWidget::UpdateButtons()
    {
        EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();

        if (recorder.GetIsRecording() == false)
        {
            mRecordButton->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/RecordButton.png"));
            mRecordButton->setToolTip("Start recording");
            mRecordButton->setEnabled(true);
        }

        // if we're recording
        if (recorder.GetIsRecording()) // we're recording
        {
            mFirstFrameButton->setEnabled(false);
            mPrevFrameButton->setEnabled(false);
            mNextFrameButton->setEnabled(false);
            mLastFrameButton->setEnabled(false);
            //mOpenButton->setEnabled(true);
            //mSaveButton->setEnabled(true);
            //mConfigButton->setEnabled(false);
            mClearButton->setEnabled(false);
            mPlayButton->setEnabled(false);
            mRecordButton->setEnabled(true);

            mRecordButton->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/StopRecorder.png"));
            mRecordButton->setToolTip("Stop recording");
        }
        else
        if (recorder.GetIsInPlayMode()) // we are playing back a recording
        {
            mFirstFrameButton->setEnabled(true);
            mPrevFrameButton->setEnabled(true);
            mNextFrameButton->setEnabled(true);
            mLastFrameButton->setEnabled(true);
            //mOpenButton->setEnabled(true);
            //mSaveButton->setEnabled(true);
            //mConfigButton->setEnabled(true);
            mClearButton->setEnabled(true);
            mPlayButton->setEnabled(true);
            //      mRecordButton->setEnabled(false);

            if (recorder.GetIsInAutoPlayMode())
            {
                mPlayButton->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Pause.png"));
                mPlayButton->setToolTip("Pause playback");
            }
            else
            {
                mPlayButton->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/PlayForward.png"));
                mPlayButton->setToolTip("Play recording");
            }
        }
        else
        if (recorder.GetRecordTime() <= MCore::Math::epsilon) // we have no recording
        {
            mFirstFrameButton->setEnabled(false);
            mPrevFrameButton->setEnabled(false);
            mNextFrameButton->setEnabled(false);
            mLastFrameButton->setEnabled(false);
            //mOpenButton->setEnabled(true);
            //mSaveButton->setEnabled(true);
            //mConfigButton->setEnabled(true);
            mClearButton->setEnabled(false);
            mPlayButton->setEnabled(false);
            mRecordButton->setEnabled(true);
            mPlayButton->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/PlayForward.png"));
            mPlayButton->setToolTip("Play recording");
        }
        else // we have a recording but are not playing yet
        {
            mFirstFrameButton->setEnabled(true);
            mPrevFrameButton->setEnabled(true);
            mNextFrameButton->setEnabled(true);
            mLastFrameButton->setEnabled(true);
            //mOpenButton->setEnabled(true);
            //mSaveButton->setEnabled(true);
            //mConfigButton->setEnabled(true);
            mClearButton->setEnabled(true);
            mPlayButton->setEnabled(true);
            mRecordButton->setEnabled(true);
        }
    }


    // update the buttons
    void RecorderWidget::OnTimeChangeStart(float newTime)
    {
        MCORE_UNUSED(newTime);

        UpdateButtons();
        emit RecorderStateChanged();
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/RecorderWidget.moc>
