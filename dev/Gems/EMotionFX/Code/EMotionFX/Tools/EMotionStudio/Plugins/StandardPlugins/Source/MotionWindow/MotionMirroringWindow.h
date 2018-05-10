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

#ifndef __EMSTUDIO_MOTIONMIRRORINGWINDOW_H
#define __EMSTUDIO_MOTIONMIRRORINGWINDOW_H

// include MCore
#include "../StandardPluginsConfig.h"
#include <MCore/Source/StandardHeaders.h>
#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QCheckBox)

namespace EMStudio
{
    // forward declarations
    class MotionWindowPlugin;

    class MotionMirroringWindow
        : public QWidget
    {
        Q_OBJECT
                 MCORE_MEMORYOBJECTCATEGORY(MotionMirroringWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:

        MotionMirroringWindow(QWidget* parent, MotionWindowPlugin* motionWindowPlugin);
        ~MotionMirroringWindow();

        void Init();

    public slots:
        void UpdateInterface();
        void UpdateMotions();
        void OnVerifyButtonPressed();

    private:
        MotionWindowPlugin*             mMotionWindowPlugin;
        QCheckBox*                      mMotionMirroringButton;
        QPushButton*                    mVerifyButton;
    };
} // namespace EMStudio


#endif
