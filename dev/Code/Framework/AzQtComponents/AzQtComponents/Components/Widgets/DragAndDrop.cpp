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



#include <AzQtComponents/Components/Widgets/DragAndDrop.h>

#include <AzQtComponents/Components/Style.h>

#include <AzQtComponents/Components/ConfigHelpers.h>



#include <QColor>

#include <QPainter>

#include <QSettings>

#include <QStyleOption>



namespace AzQtComponents

{



    DragAndDrop::Config DragAndDrop::loadConfig(QSettings& settings)

    {

        DragAndDrop::Config config = defaultConfig();



        settings.beginGroup(QStringLiteral("DropIndicator"));

        config.dropIndicator.rectOutlineWidth = settings.value(QStringLiteral("rectOutlineWidth"), config.dropIndicator.rectOutlineWidth).toInt();

        config.dropIndicator.lineSeparatorOutlineWidth = settings.value(QStringLiteral("lineSeparatorOutlineWidth"), config.dropIndicator.lineSeparatorOutlineWidth).toInt();

        config.dropIndicator.ballDiameter = settings.value(QStringLiteral("ballDiameter"), config.dropIndicator.ballDiameter).toInt();

        config.dropIndicator.ballOutlineWidth = settings.value(QStringLiteral("ballOutlineWidth"), config.dropIndicator.ballOutlineWidth).toInt();



        config.dropIndicator.rectOutlineColor = settings.value(QStringLiteral("rectOutlineColor"), config.dropIndicator.rectOutlineColor).value<QColor>();

        config.dropIndicator.lineSeparatorColor = settings.value(QStringLiteral("lineSeparatorColor"), config.dropIndicator.lineSeparatorColor).value<QColor>();

        config.dropIndicator.ballFillColor = settings.value(QStringLiteral("ballFillColor"), config.dropIndicator.ballFillColor).value<QColor>();

        config.dropIndicator.ballOutlineColor = settings.value(QStringLiteral("ballOutlineColor"), config.dropIndicator.ballOutlineColor).value<QColor>();



        settings.endGroup();



        return config;

    }



    DragAndDrop::Config DragAndDrop::defaultConfig()

    {

        return {

            DropIndicator{ 1, 1, 8, 2, QColor{Qt::white}, QColor{Qt::white} , QColor{Qt::white} , QColor{Qt::white}  }

        };

    }



    bool DragAndDrop::drawDropIndicator(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const DragAndDrop::Config& config)

    {

        Q_UNUSED(style)

        Q_UNUSED(widget)



        painter->save();

        painter->setRenderHint(QPainter::Antialiasing, true);



        // If true means we're moving the row, not dropping into another one

        const bool inBetweenRows = option->rect.height() == 0;



        if (inBetweenRows)

        {

            //draw circle followed by line for drops between items



            const int ballDiameter = config.dropIndicator.ballDiameter;

            const int ballFillDiameter = config.dropIndicator.ballDiameter - 2*config.dropIndicator.ballOutlineWidth;



            QPen pen(config.dropIndicator.ballOutlineColor);

            pen.setWidth(config.dropIndicator.ballOutlineWidth);

            painter->setPen(pen);



            painter->drawEllipse(option->rect.topLeft(), ballFillDiameter, ballFillDiameter);

            painter->drawEllipse(option->rect.topRight() - QPoint(ballFillDiameter, 0), ballFillDiameter, ballFillDiameter);



            pen = QPen(config.dropIndicator.lineSeparatorColor);

            pen.setWidth(config.dropIndicator.lineSeparatorOutlineWidth);

            painter->setPen(pen);

            painter->drawLine(QPoint(option->rect.topLeft().x() + ballFillDiameter, option->rect.topLeft().y()), option->rect.topRight() - QPoint(ballDiameter, 0));

        }

        else

        {

            //draw a rounded box for drops on items

            QPen pen(config.dropIndicator.rectOutlineColor);

            pen.setWidth(config.dropIndicator.rectOutlineWidth);

            painter->setPen(pen);

            painter->drawRect(option->rect);

        }



        painter->restore();

        return true;

    }

}

