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

#include "KeyframeOptimizationWidget.h"
#include <AzCore/std/string/string.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <EMotionFX/CommandSystem/Source/MotionCompressionCommands.h>
#include <EMotionFX/Source/MotionManager.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <EMotionFX/Exporters/ExporterLib/Exporter/ExporterFileProcessor.h>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QSpacerItem>
#include <QCheckBox>
#include <QTableWidget>
#include <QPushButton>


namespace EMStudio
{
    // the constructor
    KeyframeOptimizationWidget::KeyframeOptimizationWidget(QWidget* parent)
        : QWidget(parent)
    {
        // create the main layout
        QVBoxLayout* layout = new QVBoxLayout();
        layout->setSpacing(4);
        layout->setMargin (0);

        // generate the motion compression level widget
        mCompressionLevelWidget = new MysticQt::BarChartWidget("Please select raw/uncompressed motion.");
        mCompressionLevelWidget->AddBar("Frames 0 / 0", 100, QColor(94, 102, 110));
        mCompressionLevelWidget->AddBar("Compressed Size: 0kb / 0kb (0%)", 100, QColor(94, 102, 110));

        // generate the checkboxes
        mDisableRootNodeOptimizationCheckBox    = new QCheckBox("Disable Optimizations On Root Nodes Of Hierarchies");
        mOptimizePositionCheckBox               = new QCheckBox("Optimize Position");
        mOptimizeRotationCheckBox               = new QCheckBox("Optimize Rotation");
        mOptimizeScaleCheckBox                  = new QCheckBox("Optimize Scale");

        // create labels
        mOptimizePositionLabel  = new QLabel("Max Pos Error:");
        mOptimizeRotationLabel  = new QLabel("Max Rot Error:");
        mOptimizeScaleLabel     = new QLabel("Max Scale Error:");

        // create the spinboxes
        mPositionMaxErrorSpinBox    = new MysticQt::DoubleSpinBox();
        mRotationMaxErrorSpinBox    = new MysticQt::DoubleSpinBox();
        mScaleMaxErrorSpinBox       = new MysticQt::DoubleSpinBox();

        // set spinbox precision
        mPositionMaxErrorSpinBox->setDecimals(6);
        mRotationMaxErrorSpinBox->setDecimals(6);
        mScaleMaxErrorSpinBox->setDecimals(6);

        // set single step
        mPositionMaxErrorSpinBox->setSingleStep(0.000001);
        mRotationMaxErrorSpinBox->setSingleStep(0.000001);
        mScaleMaxErrorSpinBox->setSingleStep(0.000001);

        // set initial values
        mPositionMaxErrorSpinBox->setValue(0.00025);
        mRotationMaxErrorSpinBox->setValue(0.00025);
        mScaleMaxErrorSpinBox->setValue(0.001);

        // create sublayout for the transformation controls
        QGridLayout* transformationControlLayout = new QGridLayout();
        transformationControlLayout->setMargin(0);
        transformationControlLayout->setSpacing(2);

        transformationControlLayout->addWidget(mOptimizePositionCheckBox, 0, 0, Qt::AlignLeft);
        transformationControlLayout->addItem(new QSpacerItem(15, 0), 0, 1);
        transformationControlLayout->addWidget(mOptimizePositionLabel, 0, 2, Qt::AlignLeft);
        transformationControlLayout->addWidget(mPositionMaxErrorSpinBox, 0, 3);

        transformationControlLayout->addWidget(mOptimizeRotationCheckBox, 1, 0, Qt::AlignLeft);
        transformationControlLayout->addItem(new QSpacerItem(15, 0), 1, 1);
        transformationControlLayout->addWidget(mOptimizeRotationLabel, 1, 2, Qt::AlignLeft);
        transformationControlLayout->addWidget(mRotationMaxErrorSpinBox, 1, 3);

        transformationControlLayout->addWidget(mOptimizeScaleCheckBox, 2, 0, Qt::AlignLeft);
        transformationControlLayout->addItem(new QSpacerItem(15, 0), 1, 1);
        transformationControlLayout->addWidget(mOptimizeScaleLabel, 2, 2, Qt::AlignLeft);
        transformationControlLayout->addWidget(mScaleMaxErrorSpinBox, 2, 3);

        // create button to start the compression
        mStartKeyframeOptimization = new QPushButton("Optimize Keyframes");

        // add controls to the main layout
        layout->addWidget(mDisableRootNodeOptimizationCheckBox);
        layout->addLayout(transformationControlLayout);

        layout->addWidget(new QLabel("Compression Preview (Compressed / Original):"));
        layout->addWidget(mCompressionLevelWidget);
        layout->addWidget(mStartKeyframeOptimization);

        // set the main layout
        setLayout(layout);

        // adjust size policy
        mPositionMaxErrorSpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        mRotationMaxErrorSpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        mScaleMaxErrorSpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

        // connect slots to the controls
        connect(mOptimizePositionCheckBox, SIGNAL(clicked()), this, SLOT(UpdateInterface()));
        connect(mOptimizeRotationCheckBox, SIGNAL(clicked()), this, SLOT(UpdateInterface()));
        connect(mOptimizeScaleCheckBox, SIGNAL(clicked()), this, SLOT(UpdateInterface()));
        connect(mPositionMaxErrorSpinBox, SIGNAL(editingFinished()), this, SLOT(UpdateInterface()));
        connect(mRotationMaxErrorSpinBox, SIGNAL(editingFinished()), this, SLOT(UpdateInterface()));
        connect(mScaleMaxErrorSpinBox, SIGNAL(editingFinished()), this, SLOT(UpdateInterface()));
        connect(mStartKeyframeOptimization, SIGNAL(clicked()), this, SLOT(StartKeyframeOptimization()));

        // get connection between motion list window and compression plugin
        mMotion             = nullptr;
        mPreviewMotion      = nullptr;
        UpdateInterface();
    }


    // the destructor
    KeyframeOptimizationWidget::~KeyframeOptimizationWidget()
    {
        // delete the preview motion
        if (mPreviewMotion)
        {
            mPreviewMotion->Destroy();
        }
    }


    // set the motion
    void KeyframeOptimizationWidget::SetMotion(EMotionFX::Motion* motion)
    {
        mMotion = motion;

        // delete the preview motion

        if (mPreviewMotion)
        {
            mPreviewMotion->Destroy();
            mPreviewMotion = nullptr;
        }

        // update the interface
        UpdateInterface();
    }


    // updates the interface of the morph target group
    void KeyframeOptimizationWidget::UpdateInterface()
    {
        //  uint32 numMotion = EMotionFX::GetMotionManager().GetNumMotions();

        // delete the preview motion
        if (mPreviewMotion)
        {
            mPreviewMotion->Destroy();
            mPreviewMotion = nullptr;
        }

        // check states of the checkboxes
        bool optimizePosition   = mOptimizePositionCheckBox->isChecked();
        bool optimizeRotation   = mOptimizeRotationCheckBox->isChecked();
        bool optimizeScale      = mOptimizeScaleCheckBox->isChecked();
        float maxPositionError  = mPositionMaxErrorSpinBox->value();
        float maxRotationError  = mRotationMaxErrorSpinBox->value();
        float maxScaleError     = mScaleMaxErrorSpinBox->value();

        // enable / disable controls depending on check states
        mPositionMaxErrorSpinBox->setDisabled(!optimizePosition);
        mRotationMaxErrorSpinBox->setDisabled(!optimizeRotation);
        mScaleMaxErrorSpinBox->setDisabled(!optimizeScale);

        // enable controls only when motion is selected
        bool dialogDisabled = (mMotion == nullptr);
        //  bool morphMotion    = (mMotion && mMotion->GetType() == MorphMotion::TYPE_ID);
        mDisableRootNodeOptimizationCheckBox->setDisabled(dialogDisabled);
        mOptimizePositionCheckBox->setDisabled(dialogDisabled);
        mOptimizeRotationCheckBox->setDisabled(dialogDisabled /* || morphMotion*/);
        mOptimizeScaleCheckBox->setDisabled(dialogDisabled /* || morphMotion*/);
        mCompressionLevelWidget->setDisabled(dialogDisabled);
        mPositionMaxErrorSpinBox->setDisabled(!optimizePosition || dialogDisabled);
        mRotationMaxErrorSpinBox->setDisabled(!optimizeRotation || dialogDisabled /* || morphMotion*/);
        mScaleMaxErrorSpinBox->setDisabled(!optimizeScale || dialogDisabled /* || morphMotion*/);
        mStartKeyframeOptimization->setDisabled(dialogDisabled);

        mOptimizeRotationCheckBox->setVisible(true);
        mOptimizeScaleCheckBox->setVisible(true);
        mRotationMaxErrorSpinBox->setVisible(true);
        mScaleMaxErrorSpinBox->setVisible(true);
        mOptimizeRotationLabel->setVisible(true);
        mOptimizeScaleLabel->setVisible(true);

        // break if the moiton does not exist
        if (mMotion == nullptr)
        {
            return;
        }

        // create statistics variable
        CommandSystem::CommandKeyframeCompressMotion::Statistics statistics;

        if (mMotion->GetType() == EMotionFX::SkeletalMotion::TYPE_ID)
        {
            // adjust the interface for skeletal motions
            mOptimizePositionCheckBox->setText("Optimize Position");
            mOptimizePositionLabel->setText("Max Pos Error:");

            // typecast to skeletal motion
            EMotionFX::SkeletalMotion* motion = (EMotionFX::SkeletalMotion*)mMotion;

            // optimize the motion
            mPreviewMotion = CommandSystem::CommandKeyframeCompressMotion::OptimizeSkeletalMotion(motion, optimizePosition, optimizeRotation, optimizeScale, maxPositionError, maxRotationError, maxScaleError, &statistics, true);
        }
        /*  else if (mMotion->GetType() == MorphMotion::TYPE_ID)
            {
                // adjust the interface for morph motions
                mOptimizePositionCheckBox->setText( "Optimize Morph Motion" );
                mOptimizePositionLabel->setText( "Max Error:" );

                // typecast to morph motion
                MorphMotion* motion = (MorphMotion*)mMotion;

                // optimize the motion
                mPreviewMotion = CommandKeyframeCompressMotion::OptimizeMorphMotion( motion, optimizePosition, maxPositionError, &statistics, true );
            }*/

        // adjust the optimization overview widget
        mCompressionLevelWidget->SetBarLevel(0, 100.0 * (statistics.mNumCompressedKeyframes / (float)statistics.mNumOriginalKeyframes));
        mCompressionLevelWidget->SetName(0, AZStd::string::format("Keyframes: %i / %i", statistics.mNumCompressedKeyframes, statistics.mNumOriginalKeyframes));

        uint32 compressionLevel = 100.0 * (statistics.mNumCompressedKeyframes / (float)statistics.mNumOriginalKeyframes);
        mCompressionLevelWidget->SetBarLevel(1, compressionLevel);
        mCompressionLevelWidget->SetName(1, AZStd::string::format("Compressed Size: 10kb / 100kb (%i%%)", compressionLevel));
    }


    // starts the keyframe optimization
    void KeyframeOptimizationWidget::StartKeyframeOptimization()
    {
        // delete the preview motion
        if (mPreviewMotion)
        {
            mPreviewMotion->Destroy();
            mPreviewMotion = nullptr;
        }

        // check if motion exists
        if (mMotion == nullptr)
        {
            return;
        }

        // check states of the checkboxes
        uint32 motionID         = mMotion->GetID();
        bool optimizePosition   = mOptimizePositionCheckBox->isChecked();
        bool optimizeRotation   = mOptimizeRotationCheckBox->isChecked();
        bool optimizeScale      = mOptimizeScaleCheckBox->isChecked();
        float maxPositionError  = mPositionMaxErrorSpinBox->value();
        float maxRotationError  = mRotationMaxErrorSpinBox->value();
        float maxScaleError     = mScaleMaxErrorSpinBox->value();

        // call commands with parameters for either skeletal or morph motion
        if (mMotion->GetType() == EMotionFX::SkeletalMotion::TYPE_ID)
        {
            const AZStd::string command = AZStd::string::format("KeyframeCompressMotion -motionID %i -optimizePosition %i -optimizeRotation %i -optimizeScale %i -maxPositionError %f -maxRotationError %f -maxScaleError %f", motionID, optimizePosition, optimizeRotation, optimizeScale, maxPositionError, maxRotationError, maxScaleError);

            AZStd::string result;
            if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
            else
            {
                const int motionId = AzFramework::StringFunc::ToInt(result.c_str());
                SetMotion(EMotionFX::GetMotionManager().FindMotionByID(motionId));
            }
        }
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MotionCompression/KeyframeOptimizationWidget.moc>