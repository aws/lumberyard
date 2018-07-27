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
#include "QHyperNodePainterBase.h"

static const float g_defaultFontMin = 10;
static const float g_defaultFontMax = 200;
static const float g_defaultFontSteppingSize = 5;

QHyperNodePainterBase::QHyperNodePainterBase()
    : m_smallestFontSize(g_defaultFontMin)
    , m_largestFontSize(g_defaultFontMax)
    , m_steppingSize(g_defaultFontSteppingSize)
{
    m_numberOfFonts = ((m_largestFontSize - m_smallestFontSize) / m_steppingSize) + 1;
}

const QFont& QHyperNodePainterBase::GetFont(const int relativeFontSize)
{
    int fontIndex = (relativeFontSize - m_smallestFontSize) / m_steppingSize;
    fontIndex = CLAMP(fontIndex, 0, m_numberOfFonts - 1);

    // Creates fonts on demand and caches them, prevents fonts that are never used
    // but are in the range of possible fonts from ever being created
    auto existingFont = m_fontCache.find(fontIndex);
    if (existingFont == m_fontCache.end())
    {
        float size = m_smallestFontSize + fontIndex * m_steppingSize;
        m_fontCache[fontIndex] = QFont("Tahoma", size);
        return m_fontCache[fontIndex];
    }
    else
    {
        return existingFont->second;
    }
}

QHyperNodePainterBase::~QHyperNodePainterBase()
{
    m_fontCache.clear();
}
