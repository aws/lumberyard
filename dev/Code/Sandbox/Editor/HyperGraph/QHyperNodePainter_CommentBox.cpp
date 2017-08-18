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
//////////////////////////////////////////////////////////////////////////
// draw object for CommentBox nodes
//////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "QHyperNodePainter_CommentBox.h"

#include <QtUtil.h>

#define TRANSPARENT_ALPHA 50   // 0..255
#define BORDER_SIZE 2.f
#define FONT_SIZE_SMALL 15
#define FONT_SIZE_MEDIUM 20
#define FONT_SIZE_BIG 40

// those are used to create a table with a range of font sizes
#define SMALLEST_FONT 10
#define BIGGEST_FONT 200
#define FONT_STEP 5
#define NUM_FONTS (((BIGGEST_FONT - SMALLEST_FONT) / FONT_STEP) + 1)

namespace
{
    struct SAssets
    {
        SAssets()
            : blackBrush(Qt::black)
        {
            assert((SMALLEST_FONT + (NUM_FONTS - 1) * FONT_STEP) == BIGGEST_FONT);        // checks that FONT_STEP does match the size limits
            sf = Qt::AlignHCenter;

            for (int i = 0; i < NUM_FONTS; ++i)
            {
                float size = SMALLEST_FONT + i * FONT_STEP;
                m_fonts[i] = QFont("Tahoma", size);
            }
        }
        ~SAssets()
        {
        }

        QFont m_fonts[NUM_FONTS]; // only fonts that are actually used are created, although there is room in the array for pointers to all of them
        int sf;
        QBrush blackBrush;
    };
}

void QHyperNodePainter_CommentBox::Paint(CHyperNode* pNode, QDisplayList* pList)
{
    static SAssets assets;

    if (pNode->GetBlackBox()) //hide node
    {
        return;
    }

    const CHyperNode::Ports& inputs = *pNode->GetInputs();
    bool bFilled = false;
    bool bDisplayBox = true;
    int fontSize = 0;
    Vec3 rawColor(0, 0, 0);
    for (int i = 0; i < inputs.size(); ++i)
    {
        const CHyperNodePort& port = inputs[i];
        if (QString::compare(port.pVar->GetName(), "TextSize") == 0)
        {
            port.pVar->Get(fontSize);
        }
        if (QString::compare(port.pVar->GetName(), "DisplayFilled") == 0)
        {
            port.pVar->Get(bFilled);
        }
        if (QString::compare(port.pVar->GetName(), "DisplayBox") == 0)
        {
            port.pVar->Get(bDisplayBox);
        }
        if (QString::compare(port.pVar->GetName(), "Color") == 0)
        {
            port.pVar->Get(rawColor);
        }
    }

    rawColor = rawColor * 255;
    if (pNode->IsSelected())
    {
        rawColor = rawColor * 1.2f;
    }
    rawColor.x = rawColor.x > 255 ? 255 : rawColor.x;
    rawColor.y = rawColor.y > 255 ? 255 : rawColor.y;
    rawColor.z = rawColor.z > 255 ? 255 : rawColor.z;

    QColor colorTransparent((int)rawColor.x, (int)rawColor.y, (int)rawColor.z, TRANSPARENT_ALPHA);
    QColor colorSolid(rawColor.x, rawColor.y, rawColor.z);

    m_brushSolid.setColor(colorSolid);
    m_brushTransparent.setColor(colorTransparent);

    QDisplayRectangle* pBackground = pList->Add<QDisplayRectangle>();
    QDisplayRectangle* pBorderUp = pList->Add<QDisplayRectangle>();
    QDisplayRectangle* pBorderDown = pList->Add<QDisplayRectangle>();
    QDisplayRectangle* pBorderRight = pList->Add<QDisplayRectangle>();
    QDisplayRectangle* pBorderLeft = pList->Add<QDisplayRectangle>();
    QDisplayRectangle* pBorderUpLeft = pList->Add<QDisplayRectangle>();
    QDisplayRectangle* pBorderDownLeft = pList->Add<QDisplayRectangle>();
    QDisplayRectangle* pBorderUpRight = pList->Add<QDisplayRectangle>();
    QDisplayRectangle* pBorderDownRight = pList->Add<QDisplayRectangle>();

    pBackground->SetHitEvent(eSOID_InputTransparent);


    if (bDisplayBox)
    {
        pBorderUp->SetHitEvent(eSOID_Border_Up)->SetFilled(m_brushSolid);
        pBorderDown->SetHitEvent(eSOID_Border_Down)->SetFilled(m_brushSolid);
        pBorderRight->SetHitEvent(eSOID_Border_Right)->SetFilled(m_brushSolid);
        pBorderLeft->SetHitEvent(eSOID_Border_Left)->SetFilled(m_brushSolid);
        pBorderUpLeft->SetHitEvent(eSOID_Border_UpLeft)->SetFilled(m_brushSolid);
        pBorderDownLeft->SetHitEvent(eSOID_Border_DownLeft)->SetFilled(m_brushSolid);
        pBorderUpRight->SetHitEvent(eSOID_Border_UpRight)->SetFilled(m_brushSolid);
        pBorderDownRight->SetHitEvent(eSOID_Border_DownRight)->SetFilled(m_brushSolid);
        if (bFilled)
        {
            pBackground->SetFilled(m_brushTransparent);
        }
    }
    else
    {
        pBorderUp->SetHitEvent(eSOID_InputTransparent);
        pBorderDown->SetHitEvent(eSOID_InputTransparent);
        pBorderRight->SetHitEvent(eSOID_InputTransparent);
        pBorderLeft->SetHitEvent(eSOID_InputTransparent);
        pBorderUpLeft->SetHitEvent(eSOID_InputTransparent);
        pBorderDownLeft->SetHitEvent(eSOID_InputTransparent);
        pBorderUpRight->SetHitEvent(eSOID_InputTransparent);
        pBorderDownRight->SetHitEvent(eSOID_InputTransparent);
    }

    QString text = pNode->GetName();
    text.replace('_', ' ');
    text.replace("\\n", "\r\n");

    // choose the font size depending on the size specified for this commentBox, and the current zoom
    int normalSize;
    switch (fontSize)
    {
    case 1:
        normalSize = FONT_SIZE_SMALL;
        break;
    case 2:
        normalSize = FONT_SIZE_MEDIUM;
        break;
    case 3:
        normalSize = FONT_SIZE_BIG;
        break;
    default:
        normalSize = FONT_SIZE_MEDIUM;
        break;
    }
    float desiredFontSize = normalSize * AccessStaticVar_ZoomFactor();
    int fontIndex = (desiredFontSize - SMALLEST_FONT) / FONT_STEP;
    fontIndex = CLAMP(fontIndex, 0, NUM_FONTS - 1);

    QFont pFont = assets.m_fonts[fontIndex];

    QDisplayString* pStringShadow = pList->Add<QDisplayTitle>()
            ->SetHitEvent(eSOID_Title)
            ->SetText(text)
            ->SetBrush(assets.blackBrush)
            ->SetFont(pFont)
            ->SetStringFormat(assets.sf);

    QDisplayString* pString = pList->Add<QDisplayTitle>()
            ->SetHitEvent(eSOID_Title)
            ->SetText(text)
            ->SetBrush(m_brushSolid)
            ->SetFont(pFont)
            ->SetStringFormat(assets.sf);

    QRectF stringBounds = pString->GetBounds();
    pString->SetRect(0, 0, stringBounds.width(), stringBounds.height());

    float shadowDisp = AccessStaticVar_ZoomFactor();
    pStringShadow->SetRect(shadowDisp, shadowDisp, stringBounds.width(), stringBounds.height());

    if (!pNode->GetResizeBorderRect())
    {
        return;
    }

    // recalcs stringsborder size and pos if needed
    QRectF rectFillArea = *pNode->GetResizeBorderRect();

    rectFillArea.setY(stringBounds.height());
    pString->SetRect(0, 0, stringBounds.width(), stringBounds.height());
    pStringShadow->SetRect(shadowDisp, shadowDisp, stringBounds.width(), stringBounds.height());

    float borderThickness = AccessStaticVar_ZoomFactor() * BORDER_SIZE;  // adjust border thickness depending of zoom
    borderThickness = std::min<float>(borderThickness, std::min<float>(rectFillArea.height() / 2., rectFillArea.width() / 2.)); // to avoid overlapping with big zoom out levels

    //...background
    QRectF rectTemp = rectFillArea;
    rectTemp.translate(borderThickness, borderThickness);
    rectTemp.setWidth(rectTemp.width() - borderThickness * 2);
    rectTemp.setHeight(rectTemp.height() - borderThickness * 2);
    pBackground->SetRect(rectTemp);

    //...borders
    rectTemp = rectFillArea;
    rectTemp.setHeight(borderThickness);
    pBorderUp->SetRect(rectTemp);

    rectTemp = rectFillArea;
    rectTemp.moveTop(rectTemp.bottom() - borderThickness);
    rectTemp.setHeight(borderThickness);
    pBorderDown->SetRect(rectTemp);

    rectTemp = rectFillArea;
    rectTemp.moveLeft(rectTemp.right() - borderThickness);
    rectTemp.setWidth(borderThickness);
    pBorderRight->SetRect(rectTemp);

    rectTemp = rectFillArea;
    rectTemp.setWidth(borderThickness);
    pBorderLeft->SetRect(rectTemp);

    //...corners
    rectTemp = rectFillArea;
    rectTemp.setWidth(borderThickness * 2);
    rectTemp.setHeight(borderThickness * 2);
    pBorderUpLeft->SetRect(rectTemp);

    rectTemp.moveLeft(rectFillArea.right() - borderThickness * 2);
    rectTemp.moveTop(rectFillArea.y());
    pBorderUpRight->SetRect(rectTemp);

    rectTemp.moveLeft(rectFillArea.x());
    rectTemp.moveTop(rectFillArea.bottom() - borderThickness * 2);
    pBorderDownLeft->SetRect(rectTemp);

    rectTemp.moveLeft(rectFillArea.right() - borderThickness * 2);
    rectTemp.moveTop(rectFillArea.bottom() - borderThickness * 2);
    pBorderDownRight->SetRect(rectTemp);
}
