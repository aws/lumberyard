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
#include "ColorLabel.h"


namespace MysticQt
{
    // constructor
    ColorLabel::ColorLabel(const MCore::RGBAColor& startColor, void* dataObject, QWidget* parent, bool enableColorAdjustment)
        : QLabel(parent)
    {
        setObjectName("ColorLabel");

        setCursor(Qt::PointingHandCursor);
        setMinimumWidth(15);
        setMinimumHeight(15);
        setMaximumHeight(15);

        mEnableColorAdjustment = enableColorAdjustment;
        mDataObject = dataObject;
        mColor = QColor(startColor.r * 255, startColor.g * 255, startColor.b * 255, startColor.a * 255);

        mColorDialog = new QColorDialog(mColor, this);
        connect(mColorDialog, &QColorDialog::colorSelected, this, &MysticQt::ColorLabel::ColorChanged);

        // update color
        ColorChanged(mColor);
    }


    // destructor
    ColorLabel::~ColorLabel()
    {
        delete mColorDialog;
    }


    QColor ColorLabel::GetColor() const                     { return mColor; }
    int ColorLabel::GetRed()                                { return mColor.red(); }
    int ColorLabel::GetGreen()                              { return mColor.green(); }
    int ColorLabel::GetBlue()                               { return mColor.blue(); }
    void* ColorLabel::GetDataObject() const                 { return mDataObject; }


    MCore::RGBAColor ColorLabel::GetRGBAColor() const
    {
        int r, g, b;
        mColor.getRgb(&r, &g, &b);
        return MCore::RGBAColor(r / 255.0f, g / 255.0f, b / 255.0f);
    }


    void ColorLabel::ColorChanged(const QColor& qColor)
    {
        int r, g, b;
        qColor.getRgb(&r, &g, &b);
        setStyleSheet(AZStd::string::format("#ColorLabel{ color: rgb(%i, %i, %i); background-color: rgb(%i, %i, %i); border: 1px solid rgb(0,0,0); }", r, g, b, r, g, b).c_str());
        mColor = qColor;

        emit ColorChangeEvent();
        emit ColorChangeEvent(mColor);
    }


    void ColorLabel::mouseDoubleClickEvent(QMouseEvent* event)
    {
        MCORE_UNUSED(event);
        if (mEnableColorAdjustment)
        {
            emit clicked();
            mColorDialog->exec();
        }
    }
} // namespace MysticQt

#include <MysticQt/Source/ColorLabel.moc>