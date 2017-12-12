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
#include "../MotionWindow/MotionListWindow.h"
#include "../MotionWindow/MotionWindowPlugin.h"
#include <MysticQt/Source/DialogStack.h>
#include "../../../../EMStudioSDK/Source/DockWidgetPlugin.h"
#include <MysticQt/Source/DoubleSpinbox.h>
#include <MysticQt/Source/IntSpinbox.h>
#include <EMotionFX/Source/Motion.h>

// include Qt specific headers
#include <QWidget>
#include <MysticQt/Source/BarChartWidget.h>

// forward declarations
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QTableWidget)


namespace EMStudio
{
    class KeyframeOptimizationWidget
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(KeyframeOptimizationWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        KeyframeOptimizationWidget(QWidget* parent);
        virtual ~KeyframeOptimizationWidget();

        void SetMotion(EMotionFX::Motion* motion);
    public slots:
        void UpdateInterface();
        void StartKeyframeOptimization();

    private:
        QCheckBox*                  mDisableRootNodeOptimizationCheckBox;
        QCheckBox*                  mOptimizePositionCheckBox;
        QCheckBox*                  mOptimizeRotationCheckBox;
        QCheckBox*                  mOptimizeScaleCheckBox;
        QLabel*                     mOptimizePositionLabel;
        QLabel*                     mOptimizeRotationLabel;
        QLabel*                     mOptimizeScaleLabel;
        MysticQt::DoubleSpinBox*    mPositionMaxErrorSpinBox;
        MysticQt::DoubleSpinBox*    mRotationMaxErrorSpinBox;
        MysticQt::DoubleSpinBox*    mScaleMaxErrorSpinBox;
        QPushButton*                mStartKeyframeOptimization;

        MysticQt::BarChartWidget*   mCompressionLevelWidget;

        EMotionFX::Motion*          mMotion;
        EMotionFX::Motion*          mPreviewMotion;
    };
} // namespace EMStudio