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

#ifndef __EMSTUDIO_COMMANDBARPLUGIN_H
#define __EMSTUDIO_COMMANDBARPLUGIN_H

// include MCore
#include "../StandardPluginsConfig.h"
#include "../../../../EMStudioSDK/Source/ToolBarPlugin.h"
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/EventHandler.h>
#include <MysticQt/Source/Slider.h>
#include <QProgressBar>

QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QIcon)
QT_FORWARD_DECLARE_CLASS(QLabel)


namespace EMStudio
{
    /**
     *
     *
     */
    class CommandBarPlugin
        : public EMStudio::ToolBarPlugin
    {
        Q_OBJECT
                           MCORE_MEMORYOBJECTCATEGORY(CommandBarPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        enum
        {
            CLASS_ID = 0x00000002
        };

        CommandBarPlugin();
        ~CommandBarPlugin();

        // overloaded
        const char* GetCompileDate() const override;
        const char* GetName() const override;
        uint32 GetClassID() const override;
        const char* GetCreatorName() const override;
        float GetVersion() const override;

        bool GetIsFloatable() const override                            { return false; }
        bool GetIsVertical() const override                             { return false; }
        bool GetIsMovable() const override                              { return true;  }
        Qt::ToolBarAreas GetAllowedAreas() const override               { return Qt::TopToolBarArea | Qt::BottomToolBarArea; }
        Qt::ToolButtonStyle GetToolButtonStyle() const override         { return Qt::ToolButtonIconOnly; }

        // overloaded main init function
        bool Init() override;
        EMStudioPlugin* Clone() override;

        void UpdateLockSelectionIcon();

        void OnProgressStart();
        void OnProgressEnd();
        void OnProgressText(const char* text);
        void OnProgressValue(float percentage);

    public slots:
        void OnEnter();
        void OnPlayForwardButton();
        void OnStopButton();
        void OnPauseButton();
        void OnSeekForwardButton();
        void OnSeekBackwardButton();
        void OnLockSelectionButton();
        void ResetGlobalSpeed();
        void OnSpeedSliderValueChanged(int value);

    private:
        class ProgressHandler
            : public EMotionFX::EventHandler
        {
        public:
            AZ_CLASS_ALLOCATOR_DECL

            ProgressHandler(CommandBarPlugin* commandbarPlugin)
                : EventHandler() { mCommandbarPlugin = commandbarPlugin; }

            const AZStd::vector<EMotionFX::EventTypes> GetHandledEventTypes() const override { return { EMotionFX::EVENT_TYPE_ON_PROGRESS_START, EMotionFX::EVENT_TYPE_ON_PROGRESS_END, EMotionFX::EVENT_TYPE_ON_PROGRESS_TEXT, EMotionFX::EVENT_TYPE_ON_PROGRESS_VALUE, EMotionFX::EVENT_TYPE_ON_SUB_PROGRESS_TEXT, EMotionFX::EVENT_TYPE_ON_SUB_PROGRESS_VALUE }; }
            void OnProgressStart() override                                 { mCommandbarPlugin->OnProgressStart(); }
            void OnProgressEnd() override                                   { mCommandbarPlugin->OnProgressEnd(); }
            void OnProgressText(const char* text) override                  { mCommandbarPlugin->OnProgressText(text); }
            void OnProgressValue(float percentage) override                 { mCommandbarPlugin->OnProgressValue(percentage); }
            void OnSubProgressText(const char* text) override               { MCORE_UNUSED(text); }
            void OnSubProgressValue(float percentage) override              { MCORE_UNUSED(percentage); }
        private:
            CommandBarPlugin* mCommandbarPlugin;
        };

        QPushButton* CreateButton(const char* iconFileName = nullptr);

        MCORE_DEFINECOMMANDCALLBACK(CommandToggleLockSelectionCallback);
        CommandToggleLockSelectionCallback* mToggleLockSelectionCallback;

        QLineEdit*          mCommandEdit;
        QLineEdit*          mResultEdit;
        QPushButton*        mPauseButton;
        QPushButton*        mPlayForwardButton;
        QPushButton*        mStopButton;
        QPushButton*        mSeekForwardButton;
        QPushButton*        mSeekBackwardButton;
        QPushButton*        mLockSelectionButton;
        QPushButton*        mSpeedResetButton;
        MysticQt::Slider*   mSpeedSlider;
        QIcon*              mLockEnabledIcon;
        QIcon*              mLockDisabledIcon;
        QAction*            mCommandEditAction;
        QAction*            mCommandResultAction;

        // progress bar
        QAction*            mProgressBarAction;
        QAction*            mProgressTextAction;
        QProgressBar*       mProgressBar;
        QLabel*             mProgressText;
        ProgressHandler*    mProgressHandler;
    };
}   // namespace EMStudio


#endif
