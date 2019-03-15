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

#include "TrackHeaderWidget.h"
#include "TimeViewPlugin.h"
#include "TimeTrack.h"
#include "TrackDataWidget.h"
#include <QPaintEvent>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <MysticQt/Source/MysticQtConfig.h>
#include <MCore/Source/LogManager.h>
#include <EMotionFX/Source/Recorder.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"


namespace EMStudio
{
    // constructor
    TrackHeaderWidget::TrackHeaderWidget(TimeViewPlugin* plugin, QWidget* parent)
        : QWidget(parent)
    {
        mPlugin                     = plugin;
        mTrackLayout                = nullptr;
        mTrackWidget                = nullptr;
        mStackWidget                = nullptr;
        mNodeActivityCheckBox       = nullptr;
        mEventsCheckBox             = nullptr;
        mRelativeGraphCheckBox      = nullptr;
        mSortNodeActivityCheckBox   = nullptr;
        mLimitGraphHeightCheckBox   = nullptr;
        mNodeTypeColorsCheckBox     = nullptr;
        mDetailedNodesCheckBox      = nullptr;
        mNodeNamesCheckBox          = nullptr;
        mMotionFilesCheckBox        = nullptr;

        // create the main layout
        mMainLayout = new QVBoxLayout();
        mMainLayout->setMargin(2);
        mMainLayout->setSpacing(0);
        mMainLayout->setAlignment(Qt::AlignTop);
        //mMainLayout->setSizeConstraint( QLayout::SizeConstraint::SetNoConstraint );

        // create the add track button
        mAddTrackButton = new QPushButton("Add Event Track");
        mAddTrackButton->setIcon(MysticQt::GetMysticQt()->FindIcon("/Images/Icons/Plus.png"));
        connect(mAddTrackButton, &QPushButton::clicked, this, &TrackHeaderWidget::OnAddTrackButtonClicked);

        // add the button to the main layout
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->addWidget(mAddTrackButton);
        buttonLayout->insertSpacing(0, 4);
        buttonLayout->insertSpacing(2, 2);
        mMainLayout->addLayout(buttonLayout);
        mMainLayout->insertSpacing(0, 4);

        // recorder settings
        mStackWidget = new MysticQt::DialogStack();

        //---------------
        QWidget* displayWidget = new QWidget();
        QVBoxLayout* recorderLayout = new QVBoxLayout();
        recorderLayout->setSpacing(1);
        recorderLayout->setMargin(0);
        displayWidget->setLayout(recorderLayout);

        mNodeActivityCheckBox = new QCheckBox("Node Activity");
        mNodeActivityCheckBox->setChecked(true);
        mNodeActivityCheckBox->setCheckable(true);
        connect(mNodeActivityCheckBox, &QCheckBox::stateChanged, this, &TrackHeaderWidget::OnCheckBox);
        recorderLayout->addWidget(mNodeActivityCheckBox);

        mEventsCheckBox = new QCheckBox("Motion Events");
        mEventsCheckBox->setChecked(true);
        mEventsCheckBox->setCheckable(true);
        connect(mEventsCheckBox, &QCheckBox::stateChanged, this, &TrackHeaderWidget::OnCheckBox);
        recorderLayout->addWidget(mEventsCheckBox);

        mRelativeGraphCheckBox = new QCheckBox("Relative Graph");
        mRelativeGraphCheckBox->setChecked(true);
        mRelativeGraphCheckBox->setCheckable(true);
        connect(mRelativeGraphCheckBox, &QCheckBox::stateChanged, this, &TrackHeaderWidget::OnCheckBox);
        recorderLayout->addWidget(mRelativeGraphCheckBox);

        mStackWidget->Add(displayWidget, "Display", false, false, true);

        //-----------

        QWidget* vizWidget = new QWidget();
        QVBoxLayout* vizLayout = new QVBoxLayout();
        vizLayout->setSpacing(1);
        vizLayout->setMargin(0);
        vizWidget->setLayout(vizLayout);

        mSortNodeActivityCheckBox = new QCheckBox("Sort Node Activity");
        mSortNodeActivityCheckBox->setChecked(false);
        mSortNodeActivityCheckBox->setCheckable(true);
        connect(mSortNodeActivityCheckBox, &QCheckBox::stateChanged, this, &TrackHeaderWidget::OnCheckBox);
        vizLayout->addWidget(mSortNodeActivityCheckBox);

        mNodeTypeColorsCheckBox = new QCheckBox("Use Node Type Colors");
        mNodeTypeColorsCheckBox->setChecked(false);
        mNodeTypeColorsCheckBox->setCheckable(true);
        connect(mNodeTypeColorsCheckBox, &QCheckBox::stateChanged, this, &TrackHeaderWidget::OnCheckBox);
        vizLayout->addWidget(mNodeTypeColorsCheckBox);

        mDetailedNodesCheckBox = new QCheckBox("Detailed Nodes");
        mDetailedNodesCheckBox->setChecked(false);
        mDetailedNodesCheckBox->setCheckable(true);
        connect(mDetailedNodesCheckBox, &QCheckBox::stateChanged, this, &TrackHeaderWidget::OnDetailedNodesCheckBox);
        vizLayout->addWidget(mDetailedNodesCheckBox);

        mLimitGraphHeightCheckBox = new QCheckBox("Limit Graph Height");
        mLimitGraphHeightCheckBox->setChecked(true);
        mLimitGraphHeightCheckBox->setCheckable(true);
        connect(mLimitGraphHeightCheckBox, &QCheckBox::stateChanged, this, &TrackHeaderWidget::OnCheckBox);
        vizLayout->addWidget(mLimitGraphHeightCheckBox);

        mStackWidget->Add(vizWidget, "Visual Options", false, false, true);

        //-----------

        QWidget* contentsWidget = new QWidget();
        QVBoxLayout* contentsLayout = new QVBoxLayout();
        contentsLayout->setSpacing(1);
        contentsLayout->setMargin(0);
        contentsWidget->setLayout(contentsLayout);

        mNodeNamesCheckBox = new QCheckBox("Show Node Names");
        mNodeNamesCheckBox->setChecked(true);
        mNodeNamesCheckBox->setCheckable(true);
        connect(mNodeNamesCheckBox, &QCheckBox::stateChanged, this, &TrackHeaderWidget::OnCheckBox);
        contentsLayout->addWidget(mNodeNamesCheckBox);

        mMotionFilesCheckBox = new QCheckBox("Show Motion Files");
        mMotionFilesCheckBox->setChecked(false);
        mMotionFilesCheckBox->setCheckable(true);
        connect(mMotionFilesCheckBox, &QCheckBox::stateChanged, this, &TrackHeaderWidget::OnCheckBox);
        contentsLayout->addWidget(mMotionFilesCheckBox);

        QHBoxLayout* comboLayout = new QHBoxLayout();
        comboLayout->addWidget(new QLabel("Nodes:"));
        mNodeContentsComboBox = new MysticQt::ComboBox();
        mNodeContentsComboBox->setEditable(false);
        mNodeContentsComboBox->addItem("Global Weights");
        mNodeContentsComboBox->addItem("Local Weights");
        mNodeContentsComboBox->addItem("Local Time");
        mNodeContentsComboBox->setCurrentIndex(0);
        connect(mNodeContentsComboBox, static_cast<void (MysticQt::ComboBox::*)(int)>(&MysticQt::ComboBox::currentIndexChanged), this, &TrackHeaderWidget::OnComboBoxIndexChanged);
        comboLayout->addWidget(mNodeContentsComboBox);
        contentsLayout->addLayout(comboLayout);

        comboLayout = new QHBoxLayout();
        comboLayout->addWidget(new QLabel("Graph:"));
        mGraphContentsComboBox = new MysticQt::ComboBox();
        mGraphContentsComboBox->setEditable(false);
        mGraphContentsComboBox->addItem("Global Weights");
        mGraphContentsComboBox->addItem("Local Weights");
        mGraphContentsComboBox->addItem("Local Time");
        mGraphContentsComboBox->setCurrentIndex(0);
        connect(mGraphContentsComboBox, static_cast<void (MysticQt::ComboBox::*)(int)>(&MysticQt::ComboBox::currentIndexChanged), this, &TrackHeaderWidget::OnComboBoxIndexChanged);
        comboLayout->addWidget(mGraphContentsComboBox);
        contentsLayout->addLayout(comboLayout);

        mStackWidget->Add(contentsWidget, "Contents", false, false, true);

        //-----------

        mMainLayout->addWidget(mStackWidget);
        setFocusPolicy(Qt::StrongFocus);
        setLayout(mMainLayout);

        ReInit();
    }


    // destructor
    TrackHeaderWidget::~TrackHeaderWidget()
    {
    }


    void TrackHeaderWidget::ReInit()
    {
        mPlugin->SetRedrawFlag();

        if (mTrackWidget)
        {
            mMainLayout->removeWidget(mTrackWidget);
            mTrackWidget->deleteLater(); // TODO: this causes flickering, but normal deletion will make it crash
        }

        mTrackWidget = nullptr;

        if (EMotionFX::GetRecorder().GetIsRecording() || EMotionFX::GetRecorder().GetRecordTime() > MCore::Math::epsilon || mPlugin->mMotion == nullptr)
        {
            mAddTrackButton->setVisible(false);

            if (EMotionFX::GetRecorder().GetIsRecording() || EMotionFX::GetRecorder().GetRecordTime() > MCore::Math::epsilon)
            {
                mStackWidget->setVisible(true);
            }
            else
            {
                mStackWidget->setVisible(false);
            }

            return;
        }

        mAddTrackButton->setVisible(true);
        mStackWidget->setVisible(false);

        const uint32 numTracks = mPlugin->mTracks.GetLength();
        if (numTracks == 0)
        {
            return;
        }

        mTrackWidget = new QWidget();
        mTrackLayout = new QVBoxLayout();
        mTrackLayout->setMargin(0);
        mTrackLayout->setSpacing(1);

        for (uint32 i = 0; i < numTracks; ++i)
        {
            TimeTrack* track = mPlugin->mTracks[i];

            if (track->GetIsVisible() == false)
            {
                continue;
            }

            HeaderTrackWidget* widget = new HeaderTrackWidget(mTrackWidget, mPlugin, this, track, i);

            connect(widget, &HeaderTrackWidget::TrackNameChanged, this, &TrackHeaderWidget::OnTrackNameChanged);
            connect(widget, &HeaderTrackWidget::RemoveTrackButtonClicked, this, &TrackHeaderWidget::OnRemoveTrackButtonClicked);
            connect(widget, &HeaderTrackWidget::EnabledStateChanged, this, &TrackHeaderWidget::OnTrackEnabledStateChanged);

            mTrackLayout->addWidget(widget);
        }

        mTrackWidget->setLayout(mTrackLayout);
        mMainLayout->insertWidget(0, mTrackWidget);
    }


    HeaderTrackWidget::HeaderTrackWidget(QWidget* parent, TimeViewPlugin* parentPlugin, TrackHeaderWidget* trackHeaderWidget, TimeTrack* timeTrack, uint32 trackIndex)
        : QWidget(parent)
    {
        mPlugin = parentPlugin;

        QHBoxLayout* mainLayout = new QHBoxLayout();
        mainLayout->setMargin(0);
        mainLayout->setSpacing(0);

        mHeaderTrackWidget  = trackHeaderWidget;
        mEnabledCheckbox    = new QCheckBox();
        mNameEdit           = new QLineEdit(timeTrack->GetName());
        mRemoveButton       = new QPushButton();
        mTrack              = timeTrack;
        mTrackIndex         = trackIndex;

        if (timeTrack->GetIsEnabled())
        {
            mEnabledCheckbox->setCheckState(Qt::Checked);
        }
        else
        {
            mNameEdit->setStyleSheet("background-color: rgb(70, 70, 70);");
            mEnabledCheckbox->setCheckState(Qt::Unchecked);
        }

        if (timeTrack->GetIsDeletable() == false)
        {
            mNameEdit->setReadOnly(true);
            mRemoveButton->setEnabled(false);
            mEnabledCheckbox->setEnabled(false);
        }

        mRemoveButton->setIcon(MysticQt::GetMysticQt()->FindIcon("/Images/Icons/Remove.png"));
        mRemoveButton->setMinimumWidth(20);
        mRemoveButton->setMaximumWidth(20);

        connect(mNameEdit, &QLineEdit::editingFinished, this, &HeaderTrackWidget::NameChanged);
        connect(mNameEdit, &QLineEdit::textEdited, this, &HeaderTrackWidget::NameEdited);
        connect(mRemoveButton, &QPushButton::clicked, this, &HeaderTrackWidget::RemoveButtonClicked);
        connect(mEnabledCheckbox, &QCheckBox::stateChanged, this, &HeaderTrackWidget::EnabledCheckBoxChanged);

        mainLayout->insertSpacing(0, 4);
        mainLayout->addWidget(mEnabledCheckbox);
        mainLayout->addWidget(mNameEdit);
        mainLayout->insertSpacing(3, 4);
        mainLayout->addWidget(mRemoveButton);
        mainLayout->insertSpacing(5, 2);

        //mainLayout->setAlignment(mEnabledCheckbox, Qt::AlignLeft);
        //mainLayout->setAlignment(mRemoveButton, Qt::AlignRight);

        setLayout(mainLayout);

        setMinimumHeight(20);
        setMaximumHeight(20);
    }


    void HeaderTrackWidget::RemoveButtonClicked()
    {
        mPlugin->SetRedrawFlag();
        emit RemoveTrackButtonClicked(mTrackIndex);
    }


    void HeaderTrackWidget::NameChanged()
    {
        MCORE_ASSERT(sender()->inherits("QLineEdit"));
        mPlugin->SetRedrawFlag();
        QLineEdit* widget = qobject_cast<QLineEdit*>(sender());
        if (ValidateName())
        {
            emit TrackNameChanged(widget->text(), mTrackIndex);
        }
    }


    bool HeaderTrackWidget::ValidateName()
    {
        AZStd::string name = mNameEdit->text().toUtf8().data();

        bool nameUnique = true;
        const uint32 numTracks = mPlugin->GetNumTracks();
        for (uint32 i = 0; i < numTracks; ++i)
        {
            TimeTrack* track = mPlugin->GetTrack(i);

            if (mTrack != track)
            {
                if (name == track->GetName())
                {
                    nameUnique = false;
                    break;
                }
            }
        }

        return nameUnique;
    }


    void HeaderTrackWidget::NameEdited(const QString& text)
    {
        MCORE_UNUSED(text);
        mPlugin->SetRedrawFlag();
        if (ValidateName() == false)
        {
            GetManager()->SetWidgetAsInvalidInput(mNameEdit);
        }
        else
        {
            mNameEdit->setStyleSheet("");
        }
    }


    void HeaderTrackWidget::EnabledCheckBoxChanged(int state)
    {
        mPlugin->SetRedrawFlag();

        bool enabled = false;
        if (state == Qt::Checked)
        {
            enabled = true;
        }

        emit EnabledStateChanged(enabled, mTrackIndex);
    }


    // propagate key events to the plugin and let it handle by a shared function
    void HeaderTrackWidget::keyPressEvent(QKeyEvent* event)
    {
        if (mPlugin)
        {
            mPlugin->OnKeyPressEvent(event);
        }
    }


    // propagate key events to the plugin and let it handle by a shared function
    void HeaderTrackWidget::keyReleaseEvent(QKeyEvent* event)
    {
        if (mPlugin)
        {
            mPlugin->OnKeyReleaseEvent(event);
        }
    }


    // propagate key events to the plugin and let it handle by a shared function
    void TrackHeaderWidget::keyPressEvent(QKeyEvent* event)
    {
        if (mPlugin)
        {
            mPlugin->OnKeyPressEvent(event);
        }
    }


    // propagate key events to the plugin and let it handle by a shared function
    void TrackHeaderWidget::keyReleaseEvent(QKeyEvent* event)
    {
        if (mPlugin)
        {
            mPlugin->OnKeyReleaseEvent(event);
        }
    }


    // detailed nodes checkbox hit
    void TrackHeaderWidget::OnDetailedNodesCheckBox(int state)
    {
        MCORE_UNUSED(state);
        mPlugin->SetRedrawFlag();

        if (mDetailedNodesCheckBox->isChecked() == false)
        {
            mPlugin->mTrackDataWidget->mNodeHistoryItemHeight = 20;
        }
        else
        {
            mPlugin->mTrackDataWidget->mNodeHistoryItemHeight = 35;
        }
    }


    // a checkbox state changed
    void TrackHeaderWidget::OnCheckBox(int state)
    {
        MCORE_UNUSED(state);
        mPlugin->SetRedrawFlag();
    }


    // a combobox index changed
    void TrackHeaderWidget::OnComboBoxIndexChanged(int state)
    {
        MCORE_UNUSED(state);
        mPlugin->SetRedrawFlag();
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TrackHeaderWidget.moc>