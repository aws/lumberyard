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

#ifndef PROPERTY_FLOATSPINBOX_CTRL
#define PROPERTY_FLOATSPINBOX_CTRL

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <QtGUI/QWidget>

#pragma once

class QDoubleSpinBox;
class QLineEdit;
class QPushButton;

namespace AzToolsFramework
{
    namespace PropertySystem
    {
        class PropertyFloatSpinCtrl
            : public QWidget
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(PropertyFloatSpinCtrl, AZ::SystemAllocator, 0);

            PropertyFloatSpinCtrl(QWidget* pParent = NULL);
            virtual ~PropertyFloatSpinCtrl();

            float value() const;
            float minimum() const;
            float maximum() const;
            float step() const;

        signals:
            void revertToDefault();
            void valueChanged(float newValue);

        public slots:
            void setValue(float val);
            void setMinimum(float val);
            void setMaximum(float val);
            void setStep(float val);

        protected slots:
            void revertToDefaultClicked();
            void onChildSpinboxValueChange(double value);

        private:
            QDoubleSpinBox* m_pSpinBox;
            QToolButton* m_pDefaultButton;

        protected:
            virtual void focusInEvent(QFocusEvent* e);
        };
    };
} // namespace AzToolsFramework

#endif