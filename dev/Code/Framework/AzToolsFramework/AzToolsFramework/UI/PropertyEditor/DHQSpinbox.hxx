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

#ifndef AZ_SPINBOX_HXX
#define AZ_SPINBOX_HXX

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <AzQtComponents/Components/Widgets/SpinBox.h>

namespace AzToolsFramework
{
    // LUMBERYARD_DEPRECATED(LY-108269)
    class DHQSpinbox
        : public AzQtComponents::SpinBox
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(DHQSpinbox, AZ::SystemAllocator, 0);

        explicit DHQSpinbox(QWidget* parent = 0);
    };

    // LUMBERYARD_DEPRECATED(LY-108269)
    class DHQDoubleSpinbox
        : public AzQtComponents::DoubleSpinBox
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(DHQDoubleSpinbox, AZ::SystemAllocator, 0);

        explicit DHQDoubleSpinbox(QWidget* parent = 0);
    };
}

#endif
