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

#ifndef __MYSTICQT_LINKWIDGET_H
#define __MYSTICQT_LINKWIDGET_H

//
#include <MCore/Source/StandardHeaders.h>
#include "MysticQtConfig.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QPushButton>



namespace MysticQt
{
    class MYSTICQT_API LinkWidget
        : public QPushButton
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(LinkWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MYSTICQT_CUSTOMWIDGETS)

    public:
        /**
         * Constructor.
         */
        LinkWidget(const char* text = "", QWidget* parent = nullptr);

        /**
         * Destructor.
         */
        ~LinkWidget();
    };
} // namespace MysticQt


#endif
