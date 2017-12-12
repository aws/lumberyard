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
#include <AzCore/std/string/string.h>
#include "../MotionWindow/MotionListWindow.h"
#include "../MotionWindow/MotionWindowPlugin.h"
#include <EMotionFX/Source/Motion.h>
#include <MysticQt/Source/DialogStack.h>
#include <MysticQt/Source/ComboBox.h>
#include "../../../../EMStudioSDK/Source/DockWidgetPlugin.h"
#include <MysticQt/Source/DoubleSpinbox.h>
#include <MysticQt/Source/IntSpinbox.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <EMotionFX/CommandSystem/Source/ActorCommands.h>

// include Qt specific headers
#include <QWidget>
#include <MysticQt/Source/BarChartWidget.h>

// forward declarations
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QTableWidget)

namespace EMStudio
{
    class WaveletCompressionWidget
        : public QWidget
    {
        Q_OBJECT
                 MCORE_MEMORYOBJECTCATEGORY(WaveletCompressionWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        WaveletCompressionWidget(QWidget* parent);
        virtual ~WaveletCompressionWidget();

        void SetMotion(EMotionFX::Motion* motion);

    public slots:
        void UpdateInterface();
        void StartWaveletCompression();

    private:
        // function to check if raw file exists and is a skeletal motion
        bool RawSkeletalMotionExists(const AZStd::string& filename);

        // the controls for the dialog
        QPushButton*                mStartWaveletCompression;
        MysticQt::DoubleSpinBox*    mPositionQualitySpinBox;
        MysticQt::DoubleSpinBox*    mRotationQualitySpinBox;
        MysticQt::DoubleSpinBox*    mScaleQualitySpinBox;
        MysticQt::IntSpinBox*       mSamplesPerSecondSpinBox;
        MysticQt::ComboBox*         mSamplesPerChunkComboBox;
        MysticQt::ComboBox*         mWaveletComboBox;

        // the compression percentage widget
        MysticQt::BarChartWidget*   mCompressionLevelWidget;

        // pointers to the motions
        EMotionFX::Motion*          mMotion;
        EMotionFX::Motion*          mPreviewMotion;
    };
} // namespace EMStudio