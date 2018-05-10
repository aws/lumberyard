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

#include "MotionEventWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QPushButton>
#include <QIcon>
#include <MCore/Source/Compare.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"


namespace EMStudio
{
#define MOTIONEVENT_MINIMAL_RANGE 0.01f

    MotionEventWidget::MotionEventWidget(QWidget* parent)
        : QWidget(parent)
        , mMotionEvent(nullptr)
        , mMotion(nullptr)
        , mMotionEventTrack(nullptr)
        , mTickMode(nullptr)
    {
        Init();
        ReInit(nullptr, nullptr, nullptr, MCORE_INVALIDINDEX32);
    }


    MotionEventWidget::~MotionEventWidget()
    {
    }


    // main init function
    void MotionEventWidget::Init()
    {
        QVBoxLayout* mainLayout = new QVBoxLayout();
        mainLayout->setMargin(0);

        // tick or range event button group
        mTickMode                       = new MysticQt::ButtonGroup(this, 1, 2, MysticQt::ButtonGroup::MODE_RADIOBUTTONS);
        QPushButton* tickEventButton    = mTickMode->GetButton(0, 0);
        QPushButton* rangeEventButton   = mTickMode->GetButton(0, 1);
        tickEventButton->setText("Tick Event");
        rangeEventButton->setText("Range Event");
        connect(tickEventButton, SIGNAL(clicked()), this, SLOT(ConvertToTickEvent()));
        connect(rangeEventButton, SIGNAL(clicked()), this, SLOT(ConvertToRangeEvent()));

        // add the properties
        QGridLayout* gridLayout = new QGridLayout();

        gridLayout->addWidget(new QLabel("Mode:"),         0, 0, Qt::AlignLeft);
        gridLayout->addWidget(new QLabel("Start Time:"),   1, 0, Qt::AlignLeft);
        gridLayout->addWidget(new QLabel("End Time:"),     2, 0, Qt::AlignLeft);
        gridLayout->addWidget(new QLabel("Length:"),       3, 0, Qt::AlignLeft);
        gridLayout->addWidget(new QLabel("Type:"),         4, 0, Qt::AlignLeft);
        gridLayout->addWidget(new QLabel("Parameter:"),    5, 0, Qt::AlignLeft);
        gridLayout->addWidget(new QLabel("Mirror Type:"),  6, 0, Qt::AlignLeft);

        mStartTime  = new MysticQt::DoubleSpinBox();
        mEndTime    = new MysticQt::DoubleSpinBox();
        mLength     = new MysticQt::DoubleSpinBox();
        mType       = new QLineEdit();
        mParameter  = new QLineEdit();
        mMirrorType = new QLineEdit();

        connect(mStartTime, SIGNAL(valueChanged(double)), this, SLOT(OnStartTimeChanged(double)));
        connect(mEndTime, SIGNAL(valueChanged(double)), this, SLOT(OnEndTimeChanged(double)));
        connect(mLength, SIGNAL(valueChanged(double)), this, SLOT(OnLengthChanged(double)));
        connect(mParameter, SIGNAL(editingFinished()), this, SLOT(OnEventParameterChanged()));
        connect(mType, SIGNAL(editingFinished()), this, SLOT(OnEventTypeChanged()));
        connect(mMirrorType, SIGNAL(editingFinished()), this, SLOT(OnEventTypeChanged()));

        mStartTime->setRange(-FLT_MAX, FLT_MAX);
        mEndTime->setRange(-FLT_MAX, FLT_MAX);
        mLength->setRange(-FLT_MAX, FLT_MAX);

        mStartTime->setSuffix(" sec");
        mEndTime->setSuffix(" sec");
        mLength->setSuffix(" sec");

        gridLayout->addWidget(mTickMode,       0, 1);
        gridLayout->addWidget(mStartTime,      1, 1);
        gridLayout->addWidget(mEndTime,        2, 1);
        gridLayout->addWidget(mLength,         3, 1);
        gridLayout->addWidget(mType,           4, 1);
        gridLayout->addWidget(mParameter,      5, 1);
        gridLayout->addWidget(mMirrorType,     6, 1);

        // put it all together
        //mainLayout->addWidget(mTickMode);
        mainLayout->addLayout(gridLayout);
        setLayout(mainLayout);

        mStartTime->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        mEndTime->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        mLength->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        mParameter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        mType->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        mMirrorType->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    }


    // reinit the widget
    void MotionEventWidget::ReInit(EMotionFX::Motion* motion, EMotionFX::MotionEventTrack* eventTrack, EMotionFX::MotionEvent* motionEvent, uint32 motionEventNr)
    {
        if (!motionEvent)
        {
            mTickMode->setEnabled(false);
            mStartTime->setEnabled(false);
            mEndTime->setEnabled(false);
            mLength->setEnabled(false);
            mParameter->setEnabled(false);
            mType->setEnabled(false);
            mMirrorType->setEnabled(false);

            mStartTime->blockSignals(true);
            mStartTime->setValue(0.0);
            mStartTime->blockSignals(false);

            mEndTime->blockSignals(true);
            mEndTime->setValue(0.0);
            mEndTime->blockSignals(false);

            mLength->blockSignals(true);
            mLength->setValue(0.0);
            mLength->blockSignals(false);

            mParameter->blockSignals(true);
            mParameter->setText("");
            mParameter->blockSignals(false);

            mType->blockSignals(true);
            mType->setText("");
            mType->blockSignals(false);

            mMirrorType->blockSignals(true);
            mMirrorType->setText("");
            mMirrorType->blockSignals(false);

            mMotionEvent        = nullptr;
            mMotion             = nullptr;
            mMotionEventTrack   = nullptr;
            mMotionEventNr      = MCORE_INVALIDINDEX32;
            return;
        }

        mMotionEvent        = motionEvent;
        mMotion             = motion;
        mMotionEventTrack   = eventTrack;
        mMotionEventNr      = motionEventNr;

        mTickMode->setEnabled(true);
        mStartTime->setEnabled(true);
        mEndTime->setEnabled(!motionEvent->GetIsTickEvent());
        mLength->setEnabled(!motionEvent->GetIsTickEvent());
        mParameter->setEnabled(true);
        mType->setEnabled(true);
        mMirrorType->setEnabled(true);

        if (motionEvent->GetIsTickEvent())
        {
            mTickMode->GetButton(0, 0)->setChecked(true);
            mTickMode->GetButton(0, 1)->setChecked(false);
        }
        else
        {
            mTickMode->GetButton(0, 0)->setChecked(false);
            mTickMode->GetButton(0, 1)->setChecked(true);
        }


        mStartTime->blockSignals(true);
        mStartTime->setValue(motionEvent->GetStartTime());
        mStartTime->blockSignals(false);

        mEndTime->blockSignals(true);
        mEndTime->setValue(motionEvent->GetEndTime());
        mEndTime->blockSignals(false);

        mLength->blockSignals(true);
        mLength->setValue(motionEvent->GetEndTime() - motionEvent->GetStartTime());
        mLength->blockSignals(false);

        mParameter->blockSignals(true);
        mParameter->setText(mMotionEvent->GetParameterString(mMotionEventTrack).c_str());
        mParameter->blockSignals(false);

        mType->blockSignals(true);
        mType->setText(motionEvent->GetEventTypeString());
        mType->blockSignals(false);

        mMirrorType->blockSignals(true);
        mMirrorType->setText(motionEvent->GetMirrorEventTypeString());
        mMirrorType->blockSignals(false);
    }


    void MotionEventWidget::OnEventParameterChanged()
    {
        assert(sender()->inherits("QLineEdit"));
        QLineEdit* widget = qobject_cast<QLineEdit*>(sender());
        QString newValue = widget->text();

        // don't call the command if nothing changed
        if (mMotionEvent->GetParameterString(mMotionEventTrack) == FromQtString(newValue).c_str())
        {
            return;
        }

        const AZStd::string command = AZStd::string::format("AdjustMotionEvent -motionID %i -eventTrackName \"%s\" -eventNr %i -parameters \"%s\"", mMotion->GetID(), mMotionEventTrack->GetName(), mMotionEventNr, newValue.toUtf8().data());

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    void MotionEventWidget::OnEventTypeChanged()
    {
        // don't call the command if nothing changed
        if (mType->text() == mMotionEvent->GetEventTypeString() && mMirrorType->text() == mMotionEvent->GetMirrorEventTypeString())
        {
            return;
        }


        const AZStd::string command = AZStd::string::format("AdjustMotionEvent -motionID %i -eventTrackName \"%s\" -eventNr %i -eventType \"%s\" -mirrorType \"%s\"", mMotion->GetID(), mMotionEventTrack->GetName(), mMotionEventNr, mType->text().toUtf8().data(), mMirrorType->text().toUtf8().data());

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    void MotionEventWidget::OnStartTimeChanged(double value)
    {
        if (!mMotionEvent->GetIsTickEvent())
        {
            SetEventStartEndTime(value, mMotionEvent->GetEndTime());
        }
        else
        {
            SetEventStartEndTime(value, value, true);
        }
    }


    void MotionEventWidget::OnEndTimeChanged(double value)
    {
        SetEventStartEndTime(mMotionEvent->GetStartTime(), value);
    }


    void MotionEventWidget::OnLengthChanged(double value)
    {
        SetEventStartEndTime(mMotionEvent->GetStartTime(), mMotionEvent->GetStartTime() + value);
    }


    void MotionEventWidget::SetEventStartEndTime(float startTime, float endTime, bool forceChangingBoth)
    {
        bool startTimeModified = false;
        if (!MCore::Compare<float>::CheckIfIsClose(mMotionEvent->GetStartTime(), startTime, MCore::Math::epsilon))
        {
            startTimeModified = true;
        }

        bool endTimeModified = false;
        if (!MCore::Compare<float>::CheckIfIsClose(mMotionEvent->GetEndTime(), endTime, MCore::Math::epsilon))
        {
            endTimeModified = true;
        }

        if (!startTimeModified && !endTimeModified && !forceChangingBoth)
        {
            return;
        }

        AZStd::string command = AZStd::string::format("AdjustMotionEvent -motionID %i -eventTrackName \"%s\" -eventNr %i", mMotion->GetID(), mMotionEventTrack->GetName(), mMotionEventNr);

        if (startTimeModified || forceChangingBoth)
        {
            command += AZStd::string::format(" -startTime %f", startTime);
        }

        if (endTimeModified || forceChangingBoth)
        {
            command += AZStd::string::format(" -endTime %f", endTime);
        }

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    void MotionEventWidget::ConvertToTickEvent()
    {
        float startTime = mMotionEvent->GetStartTime();
        float endTime   = startTime;

        SetEventStartEndTime(startTime, endTime, true);
    }


    void MotionEventWidget::ConvertToRangeEvent()
    {
        float startTime = mMotionEvent->GetStartTime();
        float endTime   = startTime + MOTIONEVENT_MINIMAL_RANGE;

        SetEventStartEndTime(startTime, endTime, true);
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MotionEvents/MotionEventWidget.moc>