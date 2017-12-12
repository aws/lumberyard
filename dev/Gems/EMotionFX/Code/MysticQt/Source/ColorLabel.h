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

#ifndef __MYSTICQT_COLORLABEL_H
#define __MYSTICQT_COLORLABEL_H

//
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/Color.h>
#include "MysticQtConfig.h"
#include <QtGui/QColor>
#include <QtWidgets/QLabel>
#include <QtWidgets/QColorDialog>


namespace MysticQt
{
    /**
     * label for the example image which supports mouse clicks.
     */
    class MYSTICQT_API ColorLabel
        : public QLabel
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(ColorLabel, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MYSTICQT_CUSTOMWIDGETS);

    public:
        ColorLabel(const MCore::RGBAColor& startColor, void* dataObject = nullptr, QWidget* parent = nullptr, bool enableColorAdjustment = true);
        ~ColorLabel();

        QColor GetColor() const;
        MCore::RGBAColor GetRGBAColor() const;
        int GetRed();
        int GetGreen();
        int GetBlue();
        void* GetDataObject() const;

    signals:
        void clicked();
        void ColorChangeEvent(const QColor& qColor);
        void ColorChangeEvent();

    public slots:
        void ColorChanged(const QColor& qColor);

    protected:
        void mouseDoubleClickEvent(QMouseEvent* event) override;

        QColorDialog*       mColorDialog;
        QColor              mColor;
        void*               mDataObject;
        bool                mEnableColorAdjustment;
    };
} // namespace MysticQt


#endif
