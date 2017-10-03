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

#include "MotionEventPresetCreateDialog.h"
#include <MysticQt/Source/ColorLabel.h>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>


namespace EMStudio
{
    MotionEventPresetCreateDialog::MotionEventPresetCreateDialog(QWidget* parent, const char* eventType, const char* parameter, const char* mirrorType, uint32 color)
        : QDialog(parent)
        , mEventType(nullptr)
        , mMirrorType(nullptr)
        , mParameter(nullptr)
    {
        Init(eventType, parameter, mirrorType, color);
    }


    MotionEventPresetCreateDialog::~MotionEventPresetCreateDialog()
    {
    }


    void MotionEventPresetCreateDialog::Init(const char* eventType, const char* parameter, const char* mirrorType, uint32 color)
    {
        setWindowTitle("Motion Event Preset Creation");

        QVBoxLayout* verticalLayout = new QVBoxLayout();
        verticalLayout->setSpacing(5);
        verticalLayout->setSizeConstraint(QLayout::SizeConstraint::SetMinAndMaxSize);

        QGridLayout* gridLayout = new QGridLayout();
        gridLayout->setSizeConstraint(QLayout::SizeConstraint::SetMinAndMaxSize);
        gridLayout->setSpacing(2);
        gridLayout->setMargin(0);

        gridLayout->addWidget(new QLabel("Event Type:"),        0, 0, Qt::AlignRight);
        gridLayout->addWidget(new QLabel("Event Mirror Type:"), 1, 0, Qt::AlignRight);
        gridLayout->addWidget(new QLabel("Event Parameter:"),   2, 0, Qt::AlignRight);
        gridLayout->addWidget(new QLabel("Color:"),             3, 0, Qt::AlignRight);

        mEventType = new QLineEdit(eventType);
        gridLayout->addWidget(mEventType, 0, 1);

        mMirrorType = new QLineEdit(mirrorType);
        gridLayout->addWidget(mMirrorType, 1, 1);

        mParameter = new QLineEdit(parameter);
        gridLayout->addWidget(mParameter, 2, 1);

        mColorLabel = new MysticQt::ColorLabel(MCore::RGBAColor(color));
        mColorLabel->setFixedSize(15, 15);
        gridLayout->addWidget(mColorLabel, 3, 1);

        verticalLayout->addLayout(gridLayout);

        QHBoxLayout* buttonsLayout = new QHBoxLayout();
        QPushButton* createButton = new QPushButton("Create");
        QPushButton* cancelButton = new QPushButton("Cancel");
        connect(createButton, SIGNAL(clicked()), this, SLOT(OnCreateButton()));
        connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
        buttonsLayout->addWidget(createButton);
        buttonsLayout->addWidget(cancelButton);
        verticalLayout->addLayout(buttonsLayout);

        setLayout(verticalLayout);

        setFixedSize(400, 300);
    }

    
    void MotionEventPresetCreateDialog::OnCreateButton()
    {
        if (mEventType->text().length() == 0)
        {
            QMessageBox::critical(this, "Missing Information", "Please enter at least an event type name.");
            return;
        }

        accept();
    }


    AZStd::string MotionEventPresetCreateDialog::GetEventType() const
    {
        return mEventType->text().toUtf8().data();
    }


    AZStd::string MotionEventPresetCreateDialog::GetParameter() const
    {
        return mParameter->text().toUtf8().data();
    }


    AZStd::string MotionEventPresetCreateDialog::GetMirrorType() const
    {
        return mMirrorType->text().toUtf8().data();
    }


    uint32 MotionEventPresetCreateDialog::GetColor() const
    {
        return MCore::RGBA(mColorLabel->GetRed(), mColorLabel->GetGreen(), mColorLabel->GetBlue());
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MotionEvents/MotionEventPresetCreateDialog.moc>