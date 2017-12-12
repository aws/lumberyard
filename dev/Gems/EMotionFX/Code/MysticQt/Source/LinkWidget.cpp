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

// include the required headers
#include "LinkWidget.h"


namespace MysticQt
{
    // the constructor
    LinkWidget::LinkWidget(const char* text, QWidget* parent)
        : QPushButton(parent)
    {
        setObjectName("LinkWidget");
        setText(text);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setCursor(Qt::PointingHandCursor);
    }


    // destructor
    LinkWidget::~LinkWidget()
    {
    }
} // namespace MysticQt

#include <MysticQt/Source/LinkWidget.moc>