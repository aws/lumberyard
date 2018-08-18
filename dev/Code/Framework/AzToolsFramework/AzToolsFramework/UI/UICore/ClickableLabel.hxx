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

#ifndef AZTOOLSFRAMEWORK_UI_UICORE_CLICKABLELABEL_H
#define AZTOOLSFRAMEWORK_UI_UICORE_CLICKABLELABEL_H

#include <AzCore/Memory/SystemAllocator.h>

#include <QWidget>
#include <QLabel>
#include <QMouseEvent>

namespace AzToolsFramework
{
    class ClickableLabel
        : public QLabel
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ClickableLabel, AZ::SystemAllocator, 0);
        explicit ClickableLabel(QWidget* parent = nullptr);
        ~ClickableLabel();

    signals:
        void clicked();

    protected:
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;

    private:
        bool m_pressed;
    };
}

#endif