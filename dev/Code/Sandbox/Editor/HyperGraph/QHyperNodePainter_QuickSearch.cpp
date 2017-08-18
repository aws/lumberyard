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
#include "QHyperNodePainter_QuickSearch.h"
#include "QuickSearchNode.h"
#include "HyperGraph.h"

#include <QtUtil.h>
#include <QtUtilWin.h>

namespace
{
    struct SAssets
    {
        QColor TransparentColor(const QColor& c)
        {
            QColor r = c;
            r.setAlpha(GET_TRANSPARENCY);
            return r;
        }

        SAssets()
            : font("Tahoma", 10.0f, QFont::Bold)
            , brushBackground(TransparentColor(gSettings.hyperGraphColors.colorQuickSearchBackground))
            , brushTextCount(TransparentColor(gSettings.hyperGraphColors.colorQuickSearchCountText))
            , brushTextResult(TransparentColor(gSettings.hyperGraphColors.colorQuickSearchResultText))
            , penBorder(TransparentColor(gSettings.hyperGraphColors.colorQuickSearchBorder))
        {
            sf = Qt::AlignHCenter;
        }
        ~SAssets()
        {
        }

        void Update()
        {
            brushBackground.setColor(TransparentColor(gSettings.hyperGraphColors.colorQuickSearchBackground));
            brushTextCount.setColor(TransparentColor(gSettings.hyperGraphColors.colorQuickSearchCountText));
            brushTextResult.setColor(TransparentColor(gSettings.hyperGraphColors.colorQuickSearchResultText));
            penBorder.setColor(TransparentColor(gSettings.hyperGraphColors.colorQuickSearchBorder));
        }

        QFont font;
        QBrush brushBackground;
        QBrush brushTextCount;
        QBrush brushTextResult;
        int sf;
        QPen penBorder;
    };
}

void QHyperNodePainter_QuickSearch::Paint(CHyperNode* pNode, QDisplayList* pList)
{
    static SAssets* pAssets = 0;
    if (!pAssets)
    {
        pAssets = new SAssets();
    }

    pAssets->Update();

    QDisplayRectangle* pBackground = pList->Add<QDisplayRectangle>()
            ->SetHitEvent(eSOID_ShiftFirstOutputPort)
            ->SetStroked(pAssets->penBorder);

    QString text = pNode->GetName();
    text.replace('_', ' ');
    text.replace("\\n", "\r\n");

    QDisplayString* pString = pList->Add<QDisplayString>()
            ->SetHitEvent(eSOID_ShiftFirstOutputPort)
            ->SetText(text)
            ->SetBrush(pAssets->brushTextResult)
            ->SetFont(pAssets->font)
            ->SetStringFormat(pAssets->sf);

    pBackground->SetFilled(pAssets->brushBackground);

    CQuickSearchNode* pQuickSearchNode = static_cast<CQuickSearchNode*>(pNode);
    QString resultCount = QString("%1 / %2").arg(pQuickSearchNode->GetIndex()).arg(pQuickSearchNode->GetSearchResultCount());
    QDisplayString* pSearchCountString = pList->Add<QDisplayString>()
            ->SetHitEvent(eSOID_ShiftFirstOutputPort)
            ->SetText(resultCount)
            ->SetBrush(pAssets->brushTextCount)
            ->SetFont(pAssets->font)
            ->SetStringFormat(pAssets->sf);

    QRectF rect;
    rect.setWidth(200.0f);
    rect.setHeight(50.0f);
    rect.setLeft(0.f);
    rect.setTop(0.0f);
    pBackground->SetRect(rect);
    rect.setTop(13.0f);

    QRectF rectString = pString->GetBounds();

    if (rectString.width() > rect.width())
    {
        text = text.mid(text.lastIndexOf(':'));
        pString->SetText(text + QStringLiteral("..."));
    }

    pString->SetRect(rect);
    pString->SetLocation(QPointF(100.0f, 32.0f));
    pSearchCountString->SetLocation(QPointF(100.0f, 32.0f));
    pBackground->SetHitEvent(eSOID_Title);
    pNode->SetRect(rect);

    QPointF start(0.0f, 32.0f);
    QPointF end(200.0f, 32.0f);

    pList->AddLine(start, end,  pAssets->penBorder);

    start.setY(14.0f);
    end.setY(14.0f);
    pList->AddLine(start, end,  pAssets->penBorder);
}
