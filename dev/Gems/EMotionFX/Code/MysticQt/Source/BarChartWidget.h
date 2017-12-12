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

#include "MysticQtConfig.h"
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <QtWidgets/QWidget>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QColor>


namespace MysticQt
{
    /**
     * class used to display the compression ratio of a motion.
     */
    class MYSTICQT_API BarChartWidget
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(BarChartWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MYSTICQT_CUSTOMWIDGETS)

    public:
        BarChartWidget(const AZStd::string& errorText = "", QWidget* parent = nullptr);
        virtual ~BarChartWidget();

        void Init(const AZStd::string& errorText = "");
        void SetBarLevel(size_t bar, uint32 level);
        void SetName(size_t bar, const AZStd::string& name);
        void SetErrorText(const AZStd::string& errorText);

        void AddBar(const AZStd::string& name, uint32 level, const QColor& color);

        void paintEvent(QPaintEvent* event) override;
    protected:
        QPixmap*    mPixmap;
        uint32      mCompressionPercentage;

        // nested class for a single bar within the widget
        class BarChartWidgetBar
        {
        public:
            BarChartWidgetBar(const AZStd::string& name, uint32 level, QColor color)
            {
                mName = name;
                mLevel = level;
                mColor = color;
            }

            AZStd::string mName;
            uint32 mLevel;
            QColor mColor;
        };

        AZStd::vector<BarChartWidgetBar*>   mBars;
        AZStd::string                       mErrorText;
    };
} // namespace MysticQt