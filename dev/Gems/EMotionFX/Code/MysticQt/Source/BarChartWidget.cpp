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

#include "BarChartWidget.h"
#include <MCore/Source/LogManager.h>
#include <QtWidgets/QLabel>


namespace MysticQt
{
    // the constructor
    BarChartWidget::BarChartWidget(const AZStd::string& errorText, QWidget* parent)
        : QWidget(parent)
    {
        Init(errorText);
    }


    // destructor
    BarChartWidget::~BarChartWidget()
    {
    }


    // init the sound view widget
    void BarChartWidget::Init(const AZStd::string& errorText)
    {
        mErrorText = errorText;
        setMinimumHeight(60);
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        mCompressionPercentage = 0;
    }


    // overload the paintevent
    void BarChartWidget::paintEvent(QPaintEvent* event)
    {
        MCORE_UNUSED(event);

        // init the painter
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        QColor fontColor = QColor(190, 190, 190);
        QPen pen(fontColor);
        painter.setPen(pen);

        if (isEnabled())
        {
            // draw background level bar
            /*
            QLinearGradient gradient(0, 0, 0, height());
            gradient.setColorAt(0.0, QColor(94, 102, 110));
            gradient.setColorAt(0.5, QColor(56, 65, 72));
            gradient.setColorAt(1.0, QColor(94, 102, 110));
            */

            painter.setPen(QColor(94, 102, 110));
            painter.setBrush(QBrush(QColor(94, 102, 110)));
            painter.drawRoundedRect(2, 2, width() - 4, height() - 4, 5.0, 5.0);

            // draw the bars
            const uint32 numBars = static_cast<uint32>(mBars.size());
            const uint32 barHeight = height() / numBars;
            for (uint32 i = 0; i < numBars; ++i)
            {
                // get the current bar
                BarChartWidgetBar* bar = mBars[i];
                if (bar == nullptr)
                {
                    continue;
                }

                // generate gradient for the bar
                QLinearGradient gradient (0, i * barHeight, 0, (i + 1) * barHeight);
                gradient.setColorAt(0.0, QColor(66, 66, 66));
                gradient.setColorAt(0.5, bar->mColor);
                gradient.setColorAt(1.0, QColor(66, 66, 66));

                // calc the start positions
                const uint32 startPosY = height() - (numBars - i) * ((height() - 4) / numBars) - 2;
                const uint32 barH = (height() - 4) / numBars;
                const uint32 barW = (width() - 4) * bar->mLevel * 0.01f;

                // set colors
                painter.setPen(QColor(66, 66, 66));
                painter.setBrush(QBrush(gradient));
                painter.drawRoundedRect(2, startPosY, barW, barHeight, 5.0, 5.0);

                // display compression level of the compressed motion
                painter.setPen(QColor(190, 190, 190));
                painter.setBrush(QBrush());
                painter.drawText(8, startPosY + (barH / 2) + 4, AZStd::string::format("%s", bar->mName.c_str(), 100, 1000).c_str());
            }
        }
        else
        {
            // create lable, to estimate width of the text
            QLabel label(mErrorText.c_str());
            uint32 textWidth = label.sizeHint().width();

            // display text if no motion is selected
            painter.drawText((width() / 2) - (textWidth / 2), (height() / 2) + 3, mErrorText.c_str());
        }

        // draw bounding rect
        painter.setPen(QColor(190, 190, 190));
        painter.setBrush(QBrush());
        painter.drawRoundedRect(2, 2, width() - 4, height() - 4, 5.0, 5.0);
    }


    // sets the sound view loading progress
    void BarChartWidget::SetBarLevel(size_t bar, uint32 level)
    {
        // check if bar exists
        if (bar >= mBars.size())
        {
            return;
        }

        // update the percentage
        mBars[bar]->mLevel = level;

        // repaint the widget
        repaint();
    }


    // set the name of a specific bar
    void BarChartWidget::SetName(size_t bar, const AZStd::string& name)
    {
        // check if bar exists
        if (bar >= mBars.size())
        {
            return;
        }

        // update the percentage
        mBars[bar]->mName = name;

        // repaint the widget
        repaint();
    }


    // set the text which is displayed if no bars are shown
    void BarChartWidget::SetErrorText(const AZStd::string& errorText)
    {
        mErrorText = errorText;
    }


    // add a bar to the widget
    void BarChartWidget::AddBar(const AZStd::string& name, uint32 level, const QColor& color)
    {
        // create and add new bar
        BarChartWidgetBar* bar = new BarChartWidgetBar(name, level, color);
        mBars.push_back(bar);

        // repaint the widget
        repaint();
    }
} // namespace MysticQt

#include <MysticQt/Source/BarChartWidget.moc>