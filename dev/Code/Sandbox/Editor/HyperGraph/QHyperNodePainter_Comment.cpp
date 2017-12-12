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
#include "QHyperNodePainter_Comment.h"

#include <QtUtil.h>

namespace
{
    struct SAssets
    {
        SAssets()
            : font("Tahoma", 10.0f)
            ,//brushBackground( Gdiplus::Color(255,214,0) ),
             //brushBackgroundSelected( Gdiplus::Color(255,247,204) ),
            brushBackground(QColor(255, 255, 225))
            , brushBackgroundSelected(QColor(255, 255, 255))
            , brushText(QColor(0, 0, 0))
            , brushTextSelected(QColor(200, 0, 0))
            , penBorder(QColor(0, 0, 0))
        {
            sf = Qt::AlignHCenter;
        }
        ~SAssets()
        {
        }

        QFont font;
        QBrush brushBackground;
        QBrush brushBackgroundSelected;
        QBrush brushText;
        QBrush brushTextSelected;
        int sf;
        QPen penBorder;
    };
}

void QHyperNodePainter_Comment::Paint(CHyperNode* pNode, QDisplayList* pList)
{
    static SAssets* pAssets = 0;
    if (!pAssets)
    {
        pAssets = new SAssets();
    }

    if (pNode->GetBlackBox()) //hide node
    {
        return;
    }

    QDisplayRectangle* pBackground = pList->Add<QDisplayRectangle>()
            ->SetHitEvent(eSOID_ShiftFirstOutputPort)
            ->SetStroked(pAssets->penBorder);

    QString text = pNode->GetName();
    text.replace('_', ' ');
    text.replace("\\n", "\r\n");

    QDisplayString* pString = pList->Add<QDisplayString>()
            ->SetHitEvent(eSOID_ShiftFirstOutputPort)
            ->SetText(text)
            ->SetBrush(pAssets->brushText)
            ->SetFont(pAssets->font)
            ->SetStringFormat(pAssets->sf);

    if (pNode->IsSelected())
    {
        pString->SetBrush(pAssets->brushTextSelected);
        pBackground->SetFilled(pAssets->brushBackgroundSelected);
    }
    else
    {
        pBackground->SetFilled(pAssets->brushBackground);
    }

    QRectF rect = pString->GetBounds();
    rect.setWidth(rect.width() + 32.0f);
    rect.setHeight(rect.height() + 6.0f);
    rect.moveTo(0, 0);
    pBackground->SetRect(rect);
    pString->SetLocation(QPointF(rect.width() * 0.5f, 3.0f));

    pBackground->SetHitEvent(eSOID_Title);
}
