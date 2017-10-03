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

#include "../StandardPluginsConfig.h"
#include <MysticQt/Source/ButtonGroup.h>
#include <MysticQt/Source/DoubleSpinbox.h>
#include <MysticQt/Source/IntSpinbox.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionEvent.h>
#include <EMotionFX/Source/MotionEventTrack.h>
#include <QLabel>
#include <QLineEdit>


namespace EMStudio
{
    class MotionEventWidget
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(MotionEventWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        MotionEventWidget(QWidget* parent);
        ~MotionEventWidget();

        void ReInit(EMotionFX::Motion* motion, EMotionFX::MotionEventTrack* eventTrack, EMotionFX::MotionEvent* motionEvent, uint32 motionEventNr);

    private slots:
        void ConvertToTickEvent();
        void ConvertToRangeEvent();
        void OnStartTimeChanged(double value);
        void OnEndTimeChanged(double value);
        void OnLengthChanged(double value);
        void OnEventParameterChanged();
        void OnEventTypeChanged();

    private:
        void Init();
        void SetEventStartEndTime(float startTime, float endTime, bool forceChangingBoth = false);

        MysticQt::ButtonGroup*          mTickMode;
        MysticQt::DoubleSpinBox*        mStartTime;
        MysticQt::DoubleSpinBox*        mEndTime;
        MysticQt::DoubleSpinBox*        mLength;
        QLineEdit*                      mParameter;
        QLineEdit*                      mType;
        QLineEdit*                      mMirrorType;

        EMotionFX::MotionEvent*         mMotionEvent;
        EMotionFX::Motion*              mMotion;
        EMotionFX::MotionEventTrack*    mMotionEventTrack;
        uint32                          mMotionEventNr;
    };
} // namespace EMStudio