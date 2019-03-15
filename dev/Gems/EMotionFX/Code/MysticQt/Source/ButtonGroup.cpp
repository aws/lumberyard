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
#include "ButtonGroup.h"
#include <QtWidgets/QPushButton>
#include <MCore/Source/LogManager.h>


namespace MysticQt
{
    // calculate the interpolated gradient colors based on the global start and end color for the button group
    void ButtonGroup::GetButtonGradientColors(uint32 rowIndex, uint32 numRows, QColor startColor, QColor endColor, QColor* outStartColor, QColor* outEndColor)
    {
        if (numRows == 0)
        {
            return;
        }

        const float startR  = startColor.redF();
        const float startG  = startColor.greenF();
        const float startB  = startColor.blueF();
        const float endR    = endColor.redF();
        const float endG    = endColor.greenF();
        const float endB    = endColor.blueF();

        const float weightStart = rowIndex / (float)numRows;
        const float weightEnd = (rowIndex + 1) / (float)numRows;

        const float interpolatedStartR = MCore::LinearInterpolate<float>(startR, endR, weightStart);
        const float interpolatedStartG = MCore::LinearInterpolate<float>(startG, endG, weightStart);
        const float interpolatedStartB = MCore::LinearInterpolate<float>(startB, endB, weightStart);

        const float interpolatedEndR = MCore::LinearInterpolate<float>(startR, endR, weightEnd);
        const float interpolatedEndG = MCore::LinearInterpolate<float>(startG, endG, weightEnd);
        const float interpolatedEndB = MCore::LinearInterpolate<float>(startB, endB, weightEnd);

        outStartColor->setRgbF(interpolatedStartR, interpolatedStartG, interpolatedStartB);
        outEndColor->setRgbF(interpolatedEndR, interpolatedEndG, interpolatedEndB);
    }


    // prepare the different border styles depending on where the cell is located
    void ButtonGroup::GetBorderStyleSheet(AZStd::string* outStyleSheet, uint32 numRows, uint32 numColumns, uint32 i, uint32 j)
    {
        if (numRows == 1 && numColumns == 1)
        {
        }
        // we are dealing with a horizontal button group
        else if (numRows == 1)
        {
            // left
            if (i == 0)
            {
                *outStyleSheet = AZStd::string::format("border-bottom: 1px solid rgb(90,90,90); border-top-right-radius: 0px; border-bottom-right-radius: 0px; border-right: none; border-left-width: 1px; border-top-width: 1px;");
            }
            // right
            else if (i == numColumns - 1)
            {
                *outStyleSheet = AZStd::string::format("border-bottom: 1px solid rgb(90,90,90); border-right: 1px solid rgb(90,90,90); border-top-left-radius: 0px; border-bottom-left-radius: 0px; border-top-right-radius: 5px; border-bottom-right-radius: 5px; border-top-width: 1px;");
            }
            // all middle buttons in the horizontal button group
            else
            {
                *outStyleSheet = AZStd::string::format("border-bottom: 1px solid rgb(90,90,90); border-radius: 0px; border-right: none; border-left-width: 1px; border-top-width: 1px;");
            }
        }
        // if we are dealing with a vertical button group
        else if (numColumns == 1)
        {
            // top
            if (j == 0)
            {
                *outStyleSheet = AZStd::string::format("border-right: 1px solid rgb(90,90,90); border-radius: 0px; border-top-left-radius: 5px; border-top-right-radius: 5px; border-bottom: none; border-left-width: 1px; border-top-width: 1px;");
            }
            // bottom
            else if (j == numRows - 1)
            {
                *outStyleSheet = AZStd::string::format("border-bottom: 1px solid rgb(90,90,90); border-radius: 0px; border-bottom-left-radius: 5px; border-bottom-right-radius: 5px; border-top: none; border-left-width: 1px; border-right-width: 1px;");
            }
            // all middle buttons in the horizontal button group
            else
            {
                *outStyleSheet = AZStd::string::format("border-right: 1px solid rgb(90,90,90); border-radius: 0px; border-top: none; border-left-width: 1px; border-bottom-width: 1px;");
            }
        }
        // most left column
        else if (i == 0)
        {
            // left top
            if (j == 0)
            {
                *outStyleSheet = AZStd::string::format("border-radius: 0px; border-right: none; border-top-left-radius: 5px; border-bottom: none; border-right: none; border-top-width: 1px; border-left-width: 1px;");
            }
            // left bottom
            else if (j == numRows - 1)
            {
                *outStyleSheet = AZStd::string::format("border-bottom : 1px solid rgb(90,90,90); border-radius: 0px; border-bottom-left-radius: 5px; border-right: none; border-left-width: 1px;");
            }
            // all middle buttons in the most left column
            else
            {
                *outStyleSheet = AZStd::string::format("border-radius: 0px; border-bottom: none; border-right: none; border-left-width: 1px;");
            }
        }
        // most right column
        else if (i == numColumns - 1)
        {
            // right top
            if (j == 0)
            {
                *outStyleSheet = AZStd::string::format("border-right: 1px solid rgb(90,90,90); border-radius: 0px; border-top-right-radius: 5px; border-bottom: none;");
            }
            // right bottom
            else if (j == numRows - 1)
            {
                *outStyleSheet = AZStd::string::format("border-right: 1px solid rgb(90,90,90); border-bottom: 1px solid rgb(90,90,90); border-radius: 0px; border-bottom-right-radius: 5px;");
            }
            // all middle buttons in the most right columns
            else
            {
                *outStyleSheet = AZStd::string::format("border-right: 1px solid rgb(90,90,90); border-radius: 0px; border-bottom: none;");
            }
        }
        // all middle columns
        else
        {
            // top
            if (j == 0)
            {
                *outStyleSheet = AZStd::string::format("border-right: none; border-radius: 0px; border-bottom: none;");
            }
            // bottom
            else if (j == numRows - 1)
            {
                *outStyleSheet = AZStd::string::format("border-right: none; border-bottom: 1px solid rgb(90,90,90); border-radius: 0px;");
            }
            // all middle buttons
            else
            {
                *outStyleSheet = AZStd::string::format("border-right: none; border-radius: 0px; border-bottom: none;");
            }
        }
    }


    // prepare a whole style sheet for a given button
    void ButtonGroup::PrepareStyleSheet(AZStd::string* outStyleSheet, uint32 numRows, uint32 numColumns, uint32 i, uint32 j)
    {
        AZStd::string helperStyleString;

        //////////////////////////////////////////////////////////////////////////////////////////////
        // normal state
        //////////////////////////////////////////////////////////////////////////////////////////////
        *outStyleSheet += AZStd::string::format("QPushButton#ButtonGroup                                           \n");
        *outStyleSheet += AZStd::string::format("{                                                                 \n");
        *outStyleSheet += AZStd::string::format("  background-color: rgb(45, 45, 45);                              \n");
        *outStyleSheet += AZStd::string::format("  color: rgb(170, 170, 170);                                      \n");

        GetBorderStyleSheet(&helperStyleString, numRows, numColumns, i, j);
        *outStyleSheet += AZStd::string::format(" %s \n", helperStyleString.c_str());

        *outStyleSheet += AZStd::string::format("  padding: 3px;                                                   \n");
        *outStyleSheet += AZStd::string::format("}                                                                 \n\n");

        //////////////////////////////////////////////////////////////////////////////////////////////
        // checked state
        //////////////////////////////////////////////////////////////////////////////////////////////
        *outStyleSheet += AZStd::string::format("QPushButton#ButtonGroup:checked                                   \n");
        *outStyleSheet += AZStd::string::format("{                                                                 \n");

        // these are the global start and end colors for the button group (gradient goes vertically like in the tab headers)
        QColor startColor(244, 156, 28);
        QColor endColor(200, 110, 0);

        QColor localStartColor, localEndColor;
        GetButtonGradientColors(j, numRows, startColor, endColor, &localStartColor, &localEndColor);
        *outStyleSheet += AZStd::string::format("background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 rgb(%i, %i, %i), stop:1 rgb(%i, %i, %i));\n", localStartColor.red(), localStartColor.green(), localStartColor.blue(), localEndColor.red(), localEndColor.green(), localEndColor.blue());

        *outStyleSheet += AZStd::string::format("  color: black;                                                   \n");

        GetBorderStyleSheet(&helperStyleString, numRows, numColumns, i, j);
        *outStyleSheet += AZStd::string::format(" %s \n", helperStyleString.c_str());

        *outStyleSheet += AZStd::string::format("}                                                                 \n\n");


        //////////////////////////////////////////////////////////////////////////////////////////////
        // hover state
        //////////////////////////////////////////////////////////////////////////////////////////////
        *outStyleSheet += AZStd::string::format("QPushButton#ButtonGroup:hover                                     \n");
        *outStyleSheet += AZStd::string::format("{                                                                 \n");
        *outStyleSheet += AZStd::string::format("  background-color: rgb(144, 152, 160);                           \n");
        *outStyleSheet += AZStd::string::format("  color: black;                                                   \n");

        GetBorderStyleSheet(&helperStyleString, numRows, numColumns, i, j);
        *outStyleSheet += AZStd::string::format(" %s \n", helperStyleString.c_str());

        *outStyleSheet += AZStd::string::format("}                                                                 \n\n");

        //////////////////////////////////////////////////////////////////////////////////////////////
        // pressed state
        //////////////////////////////////////////////////////////////////////////////////////////////
        *outStyleSheet += AZStd::string::format("QPushButton#ButtonGroup:pressed                                   \n");
        *outStyleSheet += AZStd::string::format("{                                                                 \n");
        *outStyleSheet += AZStd::string::format("  background-color: black;                                        \n");
        *outStyleSheet += AZStd::string::format("  color: rgb(244, 156, 28);                                       \n");

        GetBorderStyleSheet(&helperStyleString, numRows, numColumns, i, j);
        *outStyleSheet += AZStd::string::format(" %s \n", helperStyleString.c_str());

        *outStyleSheet += AZStd::string::format("}                                                                 \n\n");


        //////////////////////////////////////////////////////////////////////////////////////////////
        // disabled state
        //////////////////////////////////////////////////////////////////////////////////////////////
        *outStyleSheet += AZStd::string::format("QPushButton#ButtonGroup:disabled                                  \n");
        *outStyleSheet += AZStd::string::format("{                                                                 \n");
        *outStyleSheet += AZStd::string::format("  background-color: rgb(55, 55, 55);                              \n");
        *outStyleSheet += AZStd::string::format("  color: rgb(105, 105, 105);                                      \n");

        GetBorderStyleSheet(&helperStyleString, numRows, numColumns, i, j);
        *outStyleSheet += AZStd::string::format(" %s \n", helperStyleString.c_str());

        *outStyleSheet += AZStd::string::format("}                                                                 \n\n");
    }


    // the constructor
    ButtonGroup::ButtonGroup(QWidget* parent, uint32 numRows, uint32 numColumns, EMode mode)
        : QWidget(parent)
    {
        uint32 i;
        mMode = mode;

        // create the grid layout
        mGridLayout = new QGridLayout(this);

        mGridLayout->setMargin(0);

        // set the vertical and the horizontal spacing
        mGridLayout->setSpacing(0);

        // reserve memory upfront to prevent allocs
        AZStd::string styleModString;
        styleModString.reserve(16192);

        // iterate over the columns
        QString temp;
        for (i = 0; i < numColumns; ++i)
        {
            // iterate over the rows
            for (uint32 j = 0; j < numRows; ++j)
            {
                // create a button
                temp.sprintf("%d%d", i, j);
                QPushButton* button = new QPushButton(temp, this);

                button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
                button->setCheckable(true);
                button->setMinimumHeight(22);
                button->setMaximumHeight(22);
                connect(button, &QPushButton::clicked, this, &MysticQt::ButtonGroup::OnButtonPressed);

                // prepare and set the style sheet
                PrepareStyleSheet(&styleModString, numRows, numColumns, i, j);
                button->setStyleSheet(styleModString.c_str());

                // add our button to the layout
                mGridLayout->addWidget(button, j, i);
            }
        }
    }


    // destructor
    ButtonGroup::~ButtonGroup()
    {
        delete mGridLayout;
    }


    QPushButton* ButtonGroup::GetButton(uint32 rowNr, uint32 columnNr)
    {
        QLayoutItem* layoutItem = mGridLayout->itemAtPosition(rowNr, columnNr);
        assert(layoutItem);

        QWidget* widget = layoutItem->widget();
        return (QPushButton*)widget;
    }


    // find the column and row indices for the given button widget
    void ButtonGroup::FindButtonIndices(QWidget* buttonToFind, uint32* outRowNr, uint32* outColumnNr)
    {
        // reset the output indices
        *outColumnNr = MCORE_INVALIDINDEX32;
        *outRowNr    = MCORE_INVALIDINDEX32;

        // get the number of columns and iterate through them
        const uint32 numColumns = GetNumColumns();
        for (uint32 i = 0; i < numColumns; ++i)
        {
            // get the number of rows and iterate through them
            const uint32 numRows = GetNumRows();
            for (uint32 j = 0; j < numRows; ++j)
            {
                // check the current widget against the one we are trying to find
                QLayoutItem* layoutItem = mGridLayout->itemAtPosition(j, i);
                if (layoutItem->widget() == buttonToFind)
                {
                    *outColumnNr = i;
                    *outRowNr    = j;
                    return;
                }
            }
        }
    }


    // enable or disable all buttons in the button group
    void ButtonGroup::SetEnabled(bool isEnabled)
    {
        // get the number of columns and iterate through them
        const uint32 numColumns = GetNumColumns();
        for (uint32 columnNr = 0; columnNr < numColumns; ++columnNr)
        {
            // get the number of rows and iterate through them
            const uint32 numRows = GetNumRows();
            for (uint32 rowNr = 0; rowNr < numRows; ++rowNr)
            {
                QPushButton* button = GetButton(rowNr, columnNr);
                if (button)
                {
                    button->setEnabled(isEnabled);
                }
            }
        }
    }


    bool ButtonGroup::GetIsButtonChecked(uint32 rowNr, uint32 columnNr)
    {
        QPushButton* button = GetButton(rowNr, columnNr);
        assert(button);
        return button->isChecked();
    }


    void ButtonGroup::OnButtonPressed()
    {
        if (mMode == MODE_RADIOBUTTONS)
        {
            QPushButton* button = qobject_cast<QPushButton*>(sender());

            // get the number of columns and iterate through them
            const uint32 numColumns = GetNumColumns();
            for (uint32 i = 0; i < numColumns; ++i)
            {
                // get the number of rows and iterate through them
                const uint32 numRows = GetNumRows();
                for (uint32 j = 0; j < numRows; ++j)
                {
                    // check the current widget against the one we are trying to find
                    QLayoutItem* layoutItem = mGridLayout->itemAtPosition(j, i);
                    QPushButton* currentButton = (QPushButton*)layoutItem->widget();

                    if (currentButton == button)
                    {
                        currentButton->setChecked(true);
                    }
                    else
                    {
                        currentButton->setChecked(false);
                    }
                }
            }
        }

        emit ButtonPressed();
    }
} // namespace MysticQt

#include <MysticQt/Source/ButtonGroup.moc>