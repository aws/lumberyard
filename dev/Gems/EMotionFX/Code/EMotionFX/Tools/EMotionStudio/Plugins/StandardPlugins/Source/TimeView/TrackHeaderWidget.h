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
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/LogManager.h>
#include <EMotionFX/CommandSystem/Source/MotionEventCommands.h>
#include <MysticQt/Source/MysticQtConfig.h>
#include <MysticQt/Source/DialogStack.h>
#include <MysticQt/Source/ComboBox.h>
#include "../StandardPluginsConfig.h"
#include <QWidget>
#include <QPen>
#include <QFont>

QT_FORWARD_DECLARE_CLASS(QBrush)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)


namespace EMStudio
{
    // forward declarations
    class TrackDataHeaderWidget;
    class TargetDataWidget;
    class TimeViewPlugin;
    class TimeTrack;
    class TrackHeaderWidget;

    class HeaderTrackWidget
        : public QWidget
    {
        Q_OBJECT
                 MCORE_MEMORYOBJECTCATEGORY(HeaderTrackWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        HeaderTrackWidget(QWidget* parent, TimeViewPlugin* parentPlugin, TrackHeaderWidget* trackHeaderWidget, TimeTrack* timeTrack, uint32 trackIndex);

        QCheckBox*              mEnabledCheckbox;
        QLineEdit*              mNameEdit;
        QPushButton*            mRemoveButton;
        TimeTrack*              mTrack;
        uint32                  mTrackIndex;
        TrackHeaderWidget*      mHeaderTrackWidget;
        TimeViewPlugin*         mPlugin;

        bool ValidateName();

    signals:
        void RemoveTrackButtonClicked(int trackNr);
        void TrackNameChanged(const QString& text, int trackNr);
        void EnabledStateChanged(bool checked, int trackNr);

    public slots:
        void RemoveButtonClicked();
        void NameChanged();
        void NameEdited(const QString& text);
        void EnabledCheckBoxChanged(int state);

        void keyPressEvent(QKeyEvent* event);
        void keyReleaseEvent(QKeyEvent* event);
    };

    /**
     * The part left of the timeline and data.
     */
    class TrackHeaderWidget
        : public QWidget
    {
        Q_OBJECT
                 MCORE_MEMORYOBJECTCATEGORY(TrackHeaderWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

        friend class TrackDataHeaderWidget;
        friend class TrackDataWidget;
        friend class TimeViewPlugin;
    public:
        TrackHeaderWidget(TimeViewPlugin* plugin, QWidget* parent = nullptr);
        ~TrackHeaderWidget();

        void ReInit();
        void UpdateDataContents();

        QPushButton* GetAddTrackButton()                                                                    { return mAddTrackButton; }

    public slots:
        void OnAddTrackButtonClicked()                                                                      { CommandSystem::CommandAddEventTrack(); }
        void OnRemoveTrackButtonClicked(int eventTrackNr)                                                   { CommandSystem::CommandRemoveEventTrack(eventTrackNr); }
        void OnTrackNameChanged(const QString& text, int trackNr)                                           { CommandSystem::CommandRenameEventTrack(trackNr, FromQtString(text).c_str()); }
        void OnTrackEnabledStateChanged(bool enabled, int trackNr)                                          { CommandSystem::CommandEnableEventTrack(trackNr, enabled); }
        void OnDetailedNodesCheckBox(int state);
        void OnCheckBox(int state);
        void OnComboBoxIndexChanged(int state);

    private:
        TimeViewPlugin*                     mPlugin;
        QVBoxLayout*                        mMainLayout;
        QWidget*                            mTrackWidget;
        QVBoxLayout*                        mTrackLayout;
        QPushButton*                        mAddTrackButton;
        MysticQt::DialogStack*              mStackWidget;
        QCheckBox*                          mNodeActivityCheckBox;
        QCheckBox*                          mEventsCheckBox;
        QCheckBox*                          mRelativeGraphCheckBox;
        QCheckBox*                          mSortNodeActivityCheckBox;
        QCheckBox*                          mNodeTypeColorsCheckBox;
        QCheckBox*                          mDetailedNodesCheckBox;
        QCheckBox*                          mLimitGraphHeightCheckBox;
        MysticQt::ComboBox*                 mGraphContentsComboBox;
        MysticQt::ComboBox*                 mNodeContentsComboBox;
        QCheckBox*                          mNodeNamesCheckBox;
        QCheckBox*                          mMotionFilesCheckBox;

        void keyPressEvent(QKeyEvent* event);
        void keyReleaseEvent(QKeyEvent* event);
    };
}   // namespace EMStudio