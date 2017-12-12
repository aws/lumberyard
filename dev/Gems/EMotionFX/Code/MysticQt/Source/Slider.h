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

#ifndef __MYSTICQT_SLIDER_H
#define __MYSTICQT_SLIDER_H

#include "MysticQtConfig.h"
#include <QSlider>


namespace MysticQt
{
    /**
     * Slider widget without mouse wheel event and focus to avoid mouse scroll conflict
     */
    class MYSTICQT_API Slider
        : public QSlider
    {
        Q_OBJECT

    public:
        Slider(QWidget* parent = nullptr);
        Slider(Qt::Orientation orientation, QWidget* parent = nullptr);
        ~Slider();

    protected:
        void wheelEvent(QWheelEvent* event) override;
    };
} // namespace MysticQt

#endif
