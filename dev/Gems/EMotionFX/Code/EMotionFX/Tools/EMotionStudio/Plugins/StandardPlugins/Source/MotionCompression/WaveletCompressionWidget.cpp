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

#include "WaveletCompressionWidget.h"
#include <AzFramework/StringFunc/StringFunc.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/SkeletalMotion.h>
#include <EMotionFX/Source/MotionSystem.h>
#include <MCore/Source/FileSystem.h>
#include <EMotionFX/Source/WaveletSkeletalMotion.h>
#include <EMotionFX/CommandSystem/Source/MotionCompressionCommands.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"

// include Qt specific headers
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>


namespace EMStudio
{
    // the constructor
    WaveletCompressionWidget::WaveletCompressionWidget(QWidget* parent)
        : QWidget(parent)
    {
        // generate the main layout
        QVBoxLayout* layout = new QVBoxLayout();
        layout->setSpacing(4);
        layout->setMargin(0);

        // generate the motion compression level widget
        mCompressionLevelWidget = new MysticQt::BarChartWidget("Please select raw/uncompressed motion.");
        mCompressionLevelWidget->AddBar("Compressed Size: 0kb / 0kb (0%)", 100, QColor(94, 102, 110));
        //  mCompressionLevelWidget->AddBar( "Optimized ratio: 10kb/100kb (10%)", 100, QColor(94, 102, 110) );

        // generate controls
        mStartWaveletCompression    = new QPushButton("Compress Motion");
        mPositionQualitySpinBox     = new MysticQt::DoubleSpinBox();
        mRotationQualitySpinBox     = new MysticQt::DoubleSpinBox();
        mScaleQualitySpinBox        = new MysticQt::DoubleSpinBox();
        mSamplesPerSecondSpinBox    = new MysticQt::IntSpinBox();

        // set ranges
        mPositionQualitySpinBox->setRange(1.0, 100.0);
        mPositionQualitySpinBox->setDecimals(1);
        mPositionQualitySpinBox->setSingleStep(0.1);

        mRotationQualitySpinBox->setRange(1.0, 100.0);
        mRotationQualitySpinBox->setDecimals(1);
        mRotationQualitySpinBox->setSingleStep(0.1);

        mScaleQualitySpinBox->setRange(1.0, 100.0);
        mScaleQualitySpinBox->setDecimals(1);
        mScaleQualitySpinBox->setSingleStep(0.1);

        mSamplesPerSecondSpinBox->setRange(1, 100);
        mSamplesPerSecondSpinBox->setSingleStep(1);

        // set default values
        mPositionQualitySpinBox->setValue(100.0);
        mRotationQualitySpinBox->setValue(100.0);
        mScaleQualitySpinBox->setValue(100.0);
        mSamplesPerSecondSpinBox->setValue(12);

        // set spinbox size policy
        mPositionQualitySpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        mRotationQualitySpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        mScaleQualitySpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        mSamplesPerSecondSpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

        // create the comboboxes
        mSamplesPerChunkComboBox = new MysticQt::ComboBox();
        mSamplesPerChunkComboBox->addItem("4");
        mSamplesPerChunkComboBox->addItem("8");
        mSamplesPerChunkComboBox->addItem("16");
        mSamplesPerChunkComboBox->addItem("32");

        mWaveletComboBox = new MysticQt::ComboBox();
        mWaveletComboBox->addItem("Haar");
        mWaveletComboBox->addItem("Daubechies 4");
        mWaveletComboBox->addItem("CDF 9/7");

        // add controls to the layouts
        QGridLayout* settingsLayout = new QGridLayout();
        settingsLayout->setMargin(0);
        settingsLayout->setSpacing(2);

        settingsLayout->addWidget(new QLabel("Position Quality"), 0, 0, Qt::AlignLeft);
        settingsLayout->addItem(new QSpacerItem(5, 0), 0, 1);
        settingsLayout->addWidget(mPositionQualitySpinBox, 0, 2);

        settingsLayout->addWidget(new QLabel("Rotation Quality"), 1, 0, Qt::AlignLeft);
        settingsLayout->addItem(new QSpacerItem(5, 0), 1, 1);
        settingsLayout->addWidget(mRotationQualitySpinBox, 1, 2);

        settingsLayout->addWidget(new QLabel("Scale Quality"), 2, 0, Qt::AlignLeft);
        settingsLayout->addItem(new QSpacerItem(5, 0), 2, 1);
        settingsLayout->addWidget(mScaleQualitySpinBox, 2, 2);

        settingsLayout->addWidget(new QLabel("Samples Per Sec"), 3, 0, Qt::AlignLeft);
        settingsLayout->addItem(new QSpacerItem(5, 0), 3, 1);
        settingsLayout->addWidget(mSamplesPerSecondSpinBox, 3, 2);

        settingsLayout->addWidget(new QLabel("Samples Per Chunk"), 4, 0, Qt::AlignLeft);
        settingsLayout->addItem(new QSpacerItem(5, 0), 4, 1);
        settingsLayout->addWidget(mSamplesPerChunkComboBox, 4, 2);

        settingsLayout->addWidget(new QLabel("Wavelet"), 5, 0, Qt::AlignLeft);
        settingsLayout->addItem(new QSpacerItem(5, 0), 5, 1);
        settingsLayout->addWidget(mWaveletComboBox, 5, 2);

        // combine the layouts
        layout->addLayout(settingsLayout);
        layout->addWidget(new QLabel("Compression Preview (Compressed / Original):"));
        layout->addWidget(mCompressionLevelWidget);
        layout->addWidget(mStartWaveletCompression);

        // set the layout
        setLayout(layout);

        // connect widgets to the slots
        connect(mPositionQualitySpinBox, SIGNAL(valueChanged(double)), this, SLOT(UpdateInterface()));
        connect(mRotationQualitySpinBox, SIGNAL(valueChanged(double)), this, SLOT(UpdateInterface()));
        connect(mScaleQualitySpinBox, SIGNAL(valueChanged(double)), this, SLOT(UpdateInterface()));
        connect(mSamplesPerSecondSpinBox, SIGNAL(valueChanged(int)), this, SLOT(UpdateInterface()));
        connect(mSamplesPerChunkComboBox, SIGNAL(valueChanged(int)), this, SLOT(UpdateInterface()));
        connect(mWaveletComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(UpdateInterface()));
        connect(mStartWaveletCompression, SIGNAL(clicked()), this, SLOT(StartWaveletCompression()));

        // get connection between motion list window and compression plugin
        mMotion             = nullptr;
        mPreviewMotion      = nullptr;
        UpdateInterface();
    }


    // the destructor
    WaveletCompressionWidget::~WaveletCompressionWidget()
    {
        // remove the preview motion
        if (mPreviewMotion)
        {
            mPreviewMotion->Destroy();
        }
    }


    // checks if a raw file exists for the given filename and check if it's a skeletal motion
    bool WaveletCompressionWidget::RawSkeletalMotionExists(const AZStd::string& filename)
    {
        // check if motion is a skeletal motion
        EMotionFX::Importer::SkeletalMotionSettings settings;
        settings.mForceLoading = true;
        EMotionFX::SkeletalMotion* motion = EMotionFX::GetImporter().LoadSkeletalMotion(filename, &settings);
        if (motion)
        {
            motion->Destroy();
            return true;
        }

        if (motion)
        {
            motion->Destroy();
        }

        return false;
    }


    // validate the connection between compression widget and the motion list window
    void WaveletCompressionWidget::SetMotion(EMotionFX::Motion* motion)
    {
        // set the motion
        mMotion = motion;

        // check if raw file with skeletal motion data exists
        if (mMotion && RawSkeletalMotionExists(mMotion->GetFileName()) == false && mMotion->GetType() == EMotionFX::WaveletSkeletalMotion::TYPE_ID)
        {
            mMotion = nullptr;
        }

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
    void WaveletCompressionWidget::UpdateInterface()
    {
        // delete the preview motion
        if (mPreviewMotion)
        {
            mPreviewMotion->Destroy();
            mPreviewMotion = nullptr;
        }

        // disable controls if no motion is selected
        bool dialogDisabled = (mMotion == nullptr || mMotion->GetType() != EMotionFX::SkeletalMotion::TYPE_ID);
        mStartWaveletCompression->setDisabled(dialogDisabled);
        mPositionQualitySpinBox->setDisabled(dialogDisabled);
        mRotationQualitySpinBox->setDisabled(dialogDisabled);
        mScaleQualitySpinBox->setDisabled(dialogDisabled);
        mSamplesPerSecondSpinBox->setDisabled(dialogDisabled);
        mSamplesPerChunkComboBox->setDisabled(dialogDisabled);
        mWaveletComboBox->setDisabled(dialogDisabled);
        mCompressionLevelWidget->setDisabled(dialogDisabled);

        // check if motion exists
        if (dialogDisabled)
        {
            return;
        }

        // generate the compressed preview motion
        mPreviewMotion = CommandSystem::CommandWaveletCompressMotion::WaveletCompressSkeletalMotion((EMotionFX::SkeletalMotion*)mMotion, mWaveletComboBox->currentIndex(), mPositionQualitySpinBox->value(), mRotationQualitySpinBox->value(), mScaleQualitySpinBox->value(), mSamplesPerSecondSpinBox->value(), mSamplesPerChunkComboBox->currentText().toInt(), true);
        EMotionFX::WaveletSkeletalMotion* previewMotion = (EMotionFX::WaveletSkeletalMotion*)mPreviewMotion;

        // adjust the bar plot
        uint32 compressionRatio = 100 * (previewMotion->GetCompressedSize() / (float)previewMotion->GetUncompressedSize());
        mCompressionLevelWidget->SetBarLevel(0, compressionRatio);
        mCompressionLevelWidget->SetName(0, AZStd::string::format("Compressed Size: %.2fkb/%.2fkb (%i %%)", previewMotion->GetCompressedSize() / 1024.0f, previewMotion->GetUncompressedSize() / 1024.0f, compressionRatio));
    }


    // perform the wavelet compression
    void WaveletCompressionWidget::StartWaveletCompression()
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
        uint32 waveletType      = mWaveletComboBox->currentIndex();
        float positionQuality   = mPositionQualitySpinBox->value();
        float rotationQuality   = mRotationQualitySpinBox->value();
        float scaleQuality      = mScaleQualitySpinBox->value();
        uint32 samplesPerSecond = mSamplesPerSecondSpinBox->value();
        uint32 samplesPerChunk  = mSamplesPerChunkComboBox->currentText().toInt();

        // call commands with parameters for skeletal motion
        if (mMotion->GetType() == EMotionFX::SkeletalMotion::TYPE_ID)
        {
            const AZStd::string command = AZStd::string::format("WaveletCompressMotion -motionID %i -waveletType %i -positionQuality %f -rotationQuality %f -scaleQuality %f -samplesPerSecond %i -samplesPerChunk %i", motionID, waveletType, positionQuality, rotationQuality, scaleQuality, samplesPerSecond, samplesPerChunk);

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

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MotionCompression/WaveletCompressionWidget.moc>