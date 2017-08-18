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
#ifndef CRYINCLUDE_EDITOR_HYPERGRAPH_QHYPERNODEPAINTER_BASE
#define CRYINCLUDE_EDITOR_HYPERGRAPH_QHYPERNODEPAINTER_BASE
#pragma once

#include "IQHyperNodePainter.h"
#include <vector>
#include <map>

//! Base for all Hyper Node Painters
class QHyperNodePainterBase
    : public IQHyperNodePainter
{
public:

    virtual void Paint(CHyperNode* pNode, QDisplayList* pList) = 0;

    /**
    * Returns the requisite font in the range of fonts based on
    * indicated relative font size
    */
    const QFont& GetFont(int relativeFontSize);

    QHyperNodePainterBase();

    QHyperNodePainterBase(float smallestFontSize, float largestFontSize, float steppingSize)
        : m_smallestFontSize(smallestFontSize)
        , m_largestFontSize(largestFontSize)
        , m_steppingSize(steppingSize)
    {
        m_numberOfFonts = ((largestFontSize - smallestFontSize) / steppingSize) + 1;
    }

    virtual ~QHyperNodePainterBase();

private:

    //! Cache used to store all the fonts that have been requested till now
    std::map<int, QFont> m_fontCache;

    //! These members combined define the size of the fonts being offered
    const float m_smallestFontSize;
    const float m_largestFontSize;
    const float m_steppingSize;

    //! Indicates the number of fonts that can be offered
    float m_numberOfFonts;
};

#endif // CRYINCLUDE_EDITOR_HYPERGRAPH_QHYPERNODEPAINTER_BASE
