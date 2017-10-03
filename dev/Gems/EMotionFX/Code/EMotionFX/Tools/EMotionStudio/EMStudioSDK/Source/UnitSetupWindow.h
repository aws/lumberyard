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

#include "EMStudioConfig.h"
#include <QWidget>
#include <MysticQt/Source/DoubleSpinbox.h>

QT_FORWARD_DECLARE_CLASS(QComboBox);


namespace EMStudio
{
    class UnitSetupWindow
        : public QWidget
    {
        Q_OBJECT
                 MCORE_MEMORYOBJECTCATEGORY(UnitSetupWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK);

    public:
        UnitSetupWindow(QWidget* parent);
        ~UnitSetupWindow();

        void ScaleData();

    public slots:
        void OnUnitTypeComboIndexChanged(int index);
        void ApplyWidgetValues();

    private:
        QComboBox*                  mUnitTypeComboBox;
        MysticQt::DoubleSpinBox*    mNearClipSpinner;
        MysticQt::DoubleSpinBox*    mFarClipSpinner;
        MysticQt::DoubleSpinBox*    mGridSpinner;
    };
} // namespace EMStudio
