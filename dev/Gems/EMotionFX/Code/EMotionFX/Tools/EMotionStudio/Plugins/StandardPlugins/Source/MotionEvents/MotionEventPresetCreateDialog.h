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
#include <MysticQt/Source/ColorLabel.h>
#include <QDialog>

QT_FORWARD_DECLARE_CLASS(QLineEdit)


namespace EMStudio
{
    class MotionEventPresetCreateDialog
        : public QDialog
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(MotionEventPresetCreateDialog, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        MotionEventPresetCreateDialog(QWidget* parent, const char* eventType = "", const char* parameter = "", const char* mirrorType = "", uint32 color = 0xffff00ff);
        ~MotionEventPresetCreateDialog();

        AZStd::string GetEventType() const;
        AZStd::string GetParameter() const;
        AZStd::string GetMirrorType() const;
        uint32 GetColor() const;

    private slots:
        void OnCreateButton();

    private:
        QLineEdit*              mEventType;
        QLineEdit*              mParameter;
        QLineEdit*              mMirrorType;
        MysticQt::ColorLabel*   mColorLabel;

        void Init(const char* eventType, const char* parameter, const char* mirrorType, uint32 color);
    };
} // namespace EMStudio