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

#include <qfont.h>
#include <qfontinfo.h>

#include <AzCore/Math/Color.h>

namespace GraphCanvas
{
    enum RootGraphicsItemDisplayState
    {
        // Order of this enum, also determines the priority, and which states
        // are stacked over each other.
        Neutral     = 0,
        Preview,
        GroupHighlight,
        Inspection,
        InspectionTransparent,
        Deletion
    };

    class FontConfiguration
    {
    public:
        AZ_TYPE_INFO(FontConfiguration, "{6D1FBE30-5BD8-4E8D-9D57-7BE79DAA9CF4}");

        FontConfiguration()
            : m_fontColor(0.0f, 0.0f, 0.0f, 1.0f)
            , m_fontFamily("default")
            , m_pixelSize(-1)
            , m_weight(QFont::Normal)
            , m_style(QFont::Style::StyleNormal)
            , m_verticalAlignment(Qt::AlignmentFlag::AlignTop)
            , m_horizontalAlignment(Qt::AlignmentFlag::AlignLeft)
        {}

        void InitializePixelSize()
        {
            if (m_pixelSize < 0)
            {
                QFont defaultFont(m_fontFamily.c_str());
                QFontInfo defaultFontInfo(defaultFont);

                m_pixelSize = defaultFontInfo.pixelSize();
            }
        }


        AZ::Color               m_fontColor;
        AZStd::string           m_fontFamily;
        AZ::s32                 m_pixelSize; 
        AZ::s32                 m_weight;
        AZ::s32                 m_style;
        AZ::s32                 m_verticalAlignment;
        AZ::s32                 m_horizontalAlignment;
    };
}