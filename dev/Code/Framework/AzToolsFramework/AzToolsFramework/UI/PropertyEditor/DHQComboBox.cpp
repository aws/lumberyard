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
#include "StdAfx.h"
#include "DHQComboBox.hxx"

#include <QAbstractItemView>
#include <QFontMetrics>
#include <QWheelEvent>

namespace AzToolsFramework
{
    DHQComboBox::DHQComboBox(QWidget* parent) : QComboBox(parent) 
    {
        view()->setTextElideMode(Qt::ElideNone);
    }

    void DHQComboBox::showPopup()
    {
        // make sure the combobox pop-up is wide enough to accommodate its text
        QFontMetrics comboMetrics(view()->fontMetrics());
        QMargins theMargins = view()->contentsMargins();
        const int marginsWidth = theMargins.left() + theMargins.right();

        int widestStringWidth = 0;
        for (int index = 0, numIndices = count(); index < numIndices; ++index)
        {
            const int newMax = comboMetrics.boundingRect(itemText(index)).width() + marginsWidth;
            
            if (newMax > widestStringWidth)
                widestStringWidth = newMax;
        }

        view()->setMinimumWidth(widestStringWidth);
        QComboBox::showPopup();
    }

    void DHQComboBox::wheelEvent(QWheelEvent* e)
    {
        if (hasFocus())
        {
            QComboBox::wheelEvent(e);
        }
        else
        {
            e->ignore();
        }
    }
}