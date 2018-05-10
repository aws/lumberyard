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

#include <QLabel>

#include <AzCore/Memory/SystemAllocator.h>

namespace GraphCanvas
{
    // This class will provide a label that auto truncates itself to the size that it is given.
    class AutoElidingLabel
        : public QLabel
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(AutoElidingLabel, AZ::SystemAllocator, 0);

        AutoElidingLabel(QWidget* parent = nullptr, Qt::WindowFlags flags = 0);
        ~AutoElidingLabel();
            
        void SetElideMode(Qt::TextElideMode elideMode);
        Qt::TextElideMode GetElideMode() const;

        QString fullText() const;
        void setFullText(const QString& text);

        // QLabel
        void resizeEvent(QResizeEvent* resizeEvent) override;

        QSize minimumSizeHint() const override;
        QSize sizeHint() const override;
        ////

        void RefreshLabel();

    private:

        QString m_fullText;
        Qt::TextElideMode m_elideMode;
    };
}