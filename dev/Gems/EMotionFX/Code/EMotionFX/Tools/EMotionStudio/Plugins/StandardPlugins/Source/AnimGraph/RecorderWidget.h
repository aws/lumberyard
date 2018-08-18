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

#pragma once

#include <MCore/Source/StandardHeaders.h>
#include "../StandardPluginsConfig.h"
#include <QWidget>
#include <MysticQt/Source/ButtonGroup.h>
#include <EMotionFX/Source/RecorderBus.h>

QT_FORWARD_DECLARE_CLASS(QPushButton)


namespace EMStudio
{
    // forward declarations
    class AnimGraphPlugin;
    class TimeViewPlugin;


    class RecorderWidget
        : public QWidget
        , private EMotionFX::RecorderNotificationBus::Handler
    {
        MCORE_MEMORYOBJECTCATEGORY(RecorderWidget, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
        Q_OBJECT

    public:
        RecorderWidget(AnimGraphPlugin* plugin, QWidget* parent = nullptr);
        ~RecorderWidget();

        void UpdateButtons();

    public slots:
        void OnRecordButton();
        void OnClearButton();
        void OnPlayButton();
        void OnFirstFrameButton();
        void OnPrevFrameButton();
        void OnNextFrameButton();
        void OnLastFrameButton();
        void OnOpenButton();
        void OnSaveButton();
        void OnConfigButton();
        void OnTimeChangeStart(float newTime);

    signals:
        void RecorderStateChanged();

    private:
        AnimGraphPlugin*        mPlugin;
        QPushButton*            mRecordButton;
        QPushButton*            mClearButton;
        QPushButton*            mPlayButton;
        QPushButton*            mFirstFrameButton;
        QPushButton*            mPrevFrameButton;
        QPushButton*            mNextFrameButton;
        QPushButton*            mLastFrameButton;
        QPushButton*            mOpenButton;
        QPushButton*            mSaveButton;
        QPushButton*            mConfigButton;
        MysticQt::ButtonGroup*  mFilterButtonGroup;

        TimeViewPlugin* GetTimeViewPlugin() const;
        void UpdateSignalConnections();
        void OnRecordingFailed(const AZStd::string& why) override;
    };
} // namespace EMStudio