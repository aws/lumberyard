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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : implementation file


#include "StdAfx.h"
#include "SequencerDopeSheet.h"

#include "AnimationContext.h"

#include "SequencerUndo.h"

#include "FragmentTrack.h"

#define KEY_TEXT_COLOR QColor(0, 0, 50)
#define INACTIVE_TEXT_COLOR QColor(128, 128, 128)
ColorB READONLY_TRACK_COLOUR(100, 100, 100);
SKeyColour READONLY_KEY_COLOUR = {
    { 150, 150, 150 }, { 150, 150, 150 }, { 150, 150, 150 }
};
#include <QPainter>

namespace
{

std::function<float(float)> GetBlendFunction(ECurveType curveType)
{
    switch (curveType)
    {
    case ECurveType::Linear:
    default:
        return [](float t) { return t; };

    case ECurveType::EaseIn:
        return [](float t) { return t*t; };

    case ECurveType::EaseOut:
        return [](float t) { return -t*(t - 2); };

    case ECurveType::EaseInOut:
        return [](float t) { t *= 2; return t < 1 ? .5f*t*t : -.5f*((t - 1)*(t - 3) - 1); };
    }
}

std::function<float(float)> GetFlippedBlendFunction(ECurveType curveType)
{
    auto curveFunction = GetBlendFunction(curveType);

    return [=](float t) { return 1.f - curveFunction(t); };
}

void AppendBlendCurvePoints(QPolygonF& points, int xStart, int xEnd, int yBottom, int yTop, ECurveType curveType, bool flipped)
{
    auto curveFunction = flipped ? GetFlippedBlendFunction(curveType) : GetBlendFunction(curveType);

    static const int kCurveSteps = 8;

    for (int i = 1; i < kCurveSteps; i++)
    {
        float t = static_cast<float>(i)/kCurveSteps;

        float x = xStart + t*(xEnd - xStart);
        float y = yBottom + curveFunction(t)*(yTop - yBottom);

        points << QPointF(x, y);
    }
}

}

// CSequencerKeyList

CSequencerDopeSheet::CSequencerDopeSheet(QWidget* parent)
    : CSequencerDopeSheetBase(parent)
{
    m_leftOffset = 30;

    m_itemWidth = 1000;
}

CSequencerDopeSheet::~CSequencerDopeSheet()
{
}

// CSequencerKeyList message handlers

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheet::DrawTrack(int item, QPainter* painter, const QRect& rcItem)
{
    const QPen pen(QColor(120, 120, 120));
    const QPen prevPen = painter->pen();
    painter->setPen(pen);
    painter->drawLine(rcItem.left(), rcItem.bottom(), rcItem.right(), rcItem.bottom());
    painter->setPen(prevPen);

    CSequencerTrack* track = GetTrack(item);
    if (!track)
    {
        CSequencerNode* pAnimNode = GetNode(item);
        if (pAnimNode && GetItem(item).paramId == 0)
        {
            // Draw Anim node track.
            DrawNodeItem(pAnimNode, painter, rcItem);
        }
        return;
    }

    QRect rcInner = rcItem;
    rcInner.setLeft(max(rcItem.left(), m_leftOffset - m_scrollOffset.x()));
    rcInner.setRight(min(rcItem.right(), (m_scrollMax + m_scrollMin) - m_scrollOffset.x() + m_leftOffset * 2));

    const QRect rcInnerDraw(QPoint(rcInner.left() - 6, rcInner.top()), QPoint(rcInner.right() + 6, rcInner.bottom()));
    ColorB trackColor = track->GetColor();
    if ((track->GetFlags() & CSequencerTrack::SEQUENCER_TRACK_READONLY) != 0)
    {
        trackColor = READONLY_TRACK_COLOUR;
    }

    const QRect rc = rcInnerDraw.adjusted(0, 1, 0, 0);


    if (IsSelectedItem(item))
    {
        QLinearGradient gradient(rc.topLeft(), rc.bottomLeft());
        gradient.setColorAt(0, QColor(trackColor.r, trackColor.g, trackColor.b));
        gradient.setColorAt(1, QColor(trackColor.r / 2, trackColor.g / 2, trackColor.b / 2));
        painter->fillRect(rc, gradient);
    }
    else
    {
        painter->fillRect(rc, QColor(trackColor.r, trackColor.g, trackColor.b));
    }

    // Left outside
    QRect rcOutside = rcItem;
    rcOutside.setRight(rcInnerDraw.left() - 1);
    rcOutside.adjust(1, 1, -1, 0);
    painter->setBrush(m_bkgrBrushEmpty);

    QLinearGradient gradient(rcOutside.topLeft(), rcOutside.bottomLeft());
    gradient.setColorAt(0, QColor(210, 210, 210));
    gradient.setColorAt(1, QColor(180, 180, 180));
    painter->fillRect(rcOutside, gradient);

    // Right outside.
    rcOutside = rcItem;
    rcOutside.setLeft(rcInnerDraw.right() + 1);
    rcOutside.adjust(1, 1, -1, 0);

    painter->fillRect(rcOutside, gradient);

    // Get time range of update rectangle.
    Range timeRange = GetTimeRange(rcItem);

    // Draw keys in time range.
    DrawKeys(track, painter, rcInner, timeRange, DRAW_BACKGROUND);
    DrawKeys(track, painter, rcInner, timeRange, (EDSRenderFlags)(DRAW_HANDLES | DRAW_NAMES));

    // Draw tick marks in time range.
    DrawTicks(painter, rcInner, timeRange);
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheet::DrawKeys(CSequencerTrack* track, QPainter* painter, const QRect& rc, const Range& timeRange, EDSRenderFlags renderFlags)
{
    enum EBlendType
    {
        B_Entry,
        B_Exit
    };
    QString keydesc;
    int prevKeyPixel = -10000;

    const ESequencerParamType paramType = track->GetParameterType();

    const QFont prevFont = painter->font();
    painter->setFont(m_descriptionFont);

    // Draw keys.
    const int numKeys = track->GetNumKeys();
    for (int i = 0; i < numKeys; i++)
    {
        const float keyTime = track->GetKeyTime(i);
        const int xKeyTime = TimeToClient(keyTime);
        if (xKeyTime - 10 > rc.right())
        {
            continue;
        }
        const float nextKeyTime = i + 1 < numKeys ? track->GetKeyTime(i + 1) : FLT_MAX;

        int xClipStart = xKeyTime;
        int xClipCurrent = xKeyTime;

        SKeyColour keyCol = track->GetKeyColour(i);
        if ((track->GetFlags() & CSequencerTrack::SEQUENCER_TRACK_READONLY) != 0)
        {
            keyCol = READONLY_KEY_COLOUR;
        }
        const QColor colourBase(keyCol.base[0], keyCol.base[1], keyCol.base[2]);

        // Get info about this key.
        QString description;
        float tempDuration = 0;
        track->GetKeyInfo(i, description, tempDuration);
        const float duration = tempDuration;    // so that I can avoid using a non-const version, helpful for learning the code
        const bool hasDuration = (duration > 0.0f);
        const float keyTimeEnd = MIN(keyTime + duration, nextKeyTime);
        const int xKeyTimeEnd = TimeToClient(keyTimeEnd);

        bool showBlend  = false;
        bool isLooping  = false;
        float startTime = 0.0;
        EBlendType blendType = B_Entry;
        bool limitTextToDuration = false;
        bool hasValidAnim = true;
        bool hasValidProc = true;
        if (paramType == SEQUENCER_PARAM_ANIMLAYER)
        {
            CClipKey clipKey;
            track->GetKey(i, &clipKey);

            startTime = clipKey.startTime;
            isLooping = (clipKey.animFlags &    CA_LOOP_ANIMATION);
            if (isLooping)
            {
                // re-use tempDuration
                tempDuration = clipKey.GetDuration();
            }

            if (clipKey.blendDuration > 0.0f)
            {
                const float endt = min(keyTime + clipKey.blendDuration, m_timeRange.end);
                xClipCurrent = TimeToClient(endt);
                showBlend = true;
                if (clipKey.animRef.IsEmpty())
                {
                    blendType = B_Exit;
                }
            }
            hasValidAnim = clipKey.HasValidFile();
        }
        else if (paramType == SEQUENCER_PARAM_PROCLAYER)
        {
            CProcClipKey procKey;
            track->GetKey(i, &procKey);
            hasValidProc = !procKey.typeNameHash.IsEmpty();

            if (procKey.blendDuration > 0.0f)
            {
                const float endt = min(keyTime + procKey.blendDuration, m_timeRange.end);
                xClipCurrent = TimeToClient(endt);
                showBlend = true;
                if (procKey.typeNameHash.IsEmpty())
                {
                    blendType = B_Exit;
                }
            }
        }
        else if (paramType == SEQUENCER_PARAM_FRAGMENTID)
        {
            CFragmentKey fragKey;
            track->GetKey(i, &fragKey);

            const float keyStartTime = (fragKey.transition) ? (max(keyTime, fragKey.tranStartTime) + fragKey.transitionTime) : keyTime;
            xClipCurrent = xClipStart = TimeToClient(keyStartTime);
            limitTextToDuration = fragKey.transition;
        }
        else if (paramType == SEQUENCER_PARAM_TRANSITIONPROPS)
        {
            CTransitionPropertyKey propsKey;
            track->GetKey(i, &propsKey);

            limitTextToDuration = (propsKey.prop == CTransitionPropertyKey::eTP_Transition);
            showBlend = false;

            //--- Draw additional indicator markers for select times
            if (propsKey.prop == CTransitionPropertyKey::eTP_Select)
            {
                const CTransitionPropertyTrack* pPropTrack = (CTransitionPropertyTrack*)track;

                float cycleTime = 0.0f;
                if (propsKey.tranFlags & SFragmentBlend::Cyclic)
                {
                    float cycle = (propsKey.time - propsKey.prevClipStart) / max(propsKey.prevClipDuration, 0.1f);
                    cycle = (float)(int)cycle; // 0-based index of the cycle the propsKey is set in
                    cycleTime = (cycle * propsKey.prevClipDuration) + propsKey.prevClipStart; // time of the start of the cycle this propsKey is set in
                }

                const  float selectStart = TimeToClient(propsKey.time);
                float selectEnd = -1;

                const uint32 numBlends = pPropTrack->GetNumBlendsForKey(i);
                for (uint32 b = 0; b < numBlends; b++)
                {
                    const SFragmentBlend* pFragmentBlend = pPropTrack->GetAlternateBlendForKey(i, b);
                    if (pFragmentBlend)
                    {
                        const float blendSelectTime = (pFragmentBlend->flags & SFragmentBlend::Cyclic) ?
                            cycleTime + (pFragmentBlend->selectTime * propsKey.prevClipDuration) :
                            propsKey.prevClipStart + pFragmentBlend->selectTime;

                        const QColor colourSelect(255, 0, 128);
                        const int xBlend = TimeToClient(blendSelectTime);
                        const QPoint p1(xBlend - 5, rc.bottom() - 5);
                        const QPoint p2(xBlend + 5, rc.bottom() - 5);
                        const QPoint p3(xBlend, rc.bottom() + 2);
                        QPainterPath path;
                        path.moveTo(p1);
                        path.lineTo(p2);
                        path.lineTo(p3);
                        path.lineTo(p1);
                        painter->fillPath(path, colourSelect);

                        if (b == propsKey.blend.blendIdx + 1)
                        {
                            // the end of the current select region is when the next blend starts (assuming sorted blends)
                            selectEnd = TimeToClient(blendSelectTime);
                        }
                    }
                }

                // Draw a bar representing the range in which this transition will be used
                if (selectEnd < 0.f)
                {
                    // we didn't find a next blend in the list
                    selectEnd = TimeToClient(propsKey.prevClipStart + propsKey.prevClipDuration);
                }

                const QColor colourSelect(255, 0, 128);
                painter->setPen(colourSelect);
                painter->drawLine(selectStart, rc.bottom() - 10, selectEnd, rc.bottom() - 10);
                painter->drawLine(selectStart, rc.bottom() - 5, selectStart, rc.bottom() - 10);
                painter->drawLine(selectEnd, rc.bottom() - 5, selectEnd, rc.bottom() - 10);
            }
        }

        // this value might differ from "duration" if "isLooping" is true
        const float nonLoopedDuration = tempDuration;

        if (renderFlags & DRAW_BACKGROUND)
        {
            // draw the key body based on it's type and it's place in the sequence along the track
            if (hasDuration)
            {
                const float endt = min(keyTimeEnd, m_timeRange.end);
                int x1 = TimeToClient(endt);
                if (x1 < 0)
                {
                    if (xClipCurrent > 0)
                    {
                        x1 = rc.right();
                    }
                }

                // this selects from the palette of images available
                // limited to the number of them which is sadly hardcoded in SequencerDopeSheetBase.cpp, line ~202
                int imgType = limitTextToDuration ? SEQBMP_TRANSITION : SEQBMP_ERROR;
                if (!limitTextToDuration)
                {
                    switch (paramType)
                    {
                    case SEQUENCER_PARAM_TAGS:
                        imgType = SEQBMP_TAGS;
                        break;
                    case SEQUENCER_PARAM_PARAMS:
                        imgType = SEQBMP_PARAMS;
                        break;
                    case SEQUENCER_PARAM_ANIMLAYER:
                        if (hasValidAnim)
                        {
                            imgType = SEQBMP_ANIMLAYER;
                        }
                        else
                        {
                            imgType = SEQBMP_ERROR;
                        }
                        break;
                    case SEQUENCER_PARAM_PROCLAYER:
                        if (hasValidProc)
                        {
                            imgType = SEQBMP_PROCLAYER;
                        }
                        else
                        {
                            imgType = SEQBMP_ERROR;
                        }
                        break;
                    case SEQUENCER_PARAM_FRAGMENTID:
                        imgType = SEQBMP_FRAGMENTID;
                        break;
                    case SEQUENCER_PARAM_TRANSITIONPROPS:
                        imgType = SEQBMP_PARAMS;
                        break;
                    }
                    ;
                }

                // images are ordered (sel,unsel),(sel,unsel),(sel,unsel),(sel,unsel)
                // so need to shift the sequence then determine offset or not.
                const int imageSeq = (i % 4) << 1;
                const int imageOffset = track->IsKeySelected(i) ? 0 : 1;

                const int start = clamp_tpl(xKeyTime, int(rc.left()), int(rc.right()));
                const int width = min(x1 - start, rc.width());
                if ((track->GetFlags() & CSequencerTrack::SEQUENCER_TRACK_READONLY) == 0)
                {
                    painter->drawPixmap(QRect(start, rc.top() + 2, width, rc.height() - 2), m_imageSeqKeyBody[imgType], QRect(16 * (imageSeq + imageOffset), 0, 16, 16));
                }
                else
                {
                    painter->fillRect(QRect(start, rc.top() + 2, width, rc.height() - 2), colourBase);
                }
                painter->drawLine(x1, rc.top(), x1, rc.bottom());
            }
            painter->drawLine(xKeyTime, rc.top(), xKeyTime, rc.bottom());
        }


        if (renderFlags & DRAW_HANDLES)
        {
            // -- draw "frame" == 4 triangles at each corner + lines connecting them.
            const int numSecondarySelPts = track->GetNumSecondarySelPts(i);
            if (hasDuration && B_Exit != blendType) // drawFrame
            {
                // find where the next key begins
                const int nexti = (i + 1);
                const bool hasNextKey = (nexti < track->GetNumKeys());
                const float nextKeyTime = hasNextKey ? track->GetKeyTime(nexti) : keyTimeEnd;

                float completionDuration = 0.0f;
                if (track->GetParameterType() == SEQUENCER_PARAM_ANIMLAYER)
                {
                    if (track->GetNumSecondarySelPts(i) >= CClipTrack::eCKSS_BlendOut)
                    {
                        const CClipKey* pClipKey = static_cast<const CClipKey*>(track->GetKey(i));
                        if (pClipKey)
                        {
                            completionDuration = pClipKey->blendOutDuration;
                        }
                    }
                }

                const float blendOutStartTime = hasNextKey ? nextKeyTime : keyTimeEnd - completionDuration;
                ECurveType blendOutCurveType = hasNextKey ? track->GetBlendCurveType(nexti) : ECurveType::Linear;

                float timeNext2ndKey = keyTimeEnd;
                if (hasNextKey && track->GetNumSecondarySelPts(nexti) > 0)
                {
                    timeNext2ndKey = max(track->GetSecondaryTime(nexti, CClipTrack::eCKSS_BlendIn), nextKeyTime);
                }
                const float blendOutEndTime = timeNext2ndKey;
                const int xBlendOutStart = TimeToClient(blendOutStartTime);
                const int xBlendOutEnd = TimeToClient(blendOutEndTime);

                // when & where is our blend marker
                float time2ndKey = keyTime;
                ECurveType blendInCurveType = ECurveType::Linear;
                if (numSecondarySelPts > 0)
                {
                    const int id2ndPt = track->GetSecondarySelectionPt(i, keyTime, keyTimeEnd);
                    time2ndKey = track->GetSecondaryTime(i, id2ndPt);
                    blendInCurveType = track->GetBlendCurveType(i);
                }
                const float blendInEndTime = max(time2ndKey, keyTime);
                const int xBlendInEnd = TimeToClient(blendInEndTime);

                const QRect lrc = rc.adjusted(1, 1, -1, -1);

                if (isLooping)
                {
                    const int x1stLoopEnd = TimeToClient(keyTime + nonLoopedDuration);
                    const int xNonLoopEnd = TimeToClient(keyTime + nonLoopedDuration + startTime);
                    int endXPos = x1stLoopEnd;
                    const int xLoopStep = max(xNonLoopEnd - xKeyTime, 2);
                    const int yPos = max(0L, rc.top() + rc.bottom() - 10L) / 2; // -10 to avoid overlap with the text in the middle of the clip
                    const QPen pen(QColor(100, 100, 100), 1, Qt::DotLine);
                    painter->setPen(pen);
                    while (endXPos < xKeyTimeEnd)
                    {
                        painter->drawLine(endXPos, rc.top(), endXPos, yPos);
                        // next position
                        endXPos += xLoopStep;
                    }
                }

                const bool canEditKey = track->CanEditKey(i);
                const QColor triColour = (track->IsKeySelected(i)) ? QColor(0, 255, 0) : (canEditKey ? QColor(50, 50, 100) : QColor(100, 100, 100));
                m_bAnySelected |= track->IsKeySelected(i);
                const QPen pen(triColour);
                painter->setPen(pen);

                static const int kCurveSteps = 8;

                QPolygonF points;

                // draw | vertical start line

                points << QPointF(xKeyTime, lrc.bottom());

                if (blendInCurveType != ECurveType::Linear)
                {
                    AppendBlendCurvePoints(points, xKeyTime, xBlendInEnd, lrc.bottom(), lrc.top(), blendInCurveType, false);
                }

                points << QPointF(xBlendInEnd, lrc.top());

                // draw -- horizontal top line

                points << QPointF(xBlendOutStart, lrc.top());

                // draw | vertical end line

                if (blendOutCurveType != ECurveType::Linear)
                {
                    AppendBlendCurvePoints(points, xBlendOutStart, xBlendOutEnd, lrc.bottom(), lrc.top(), blendOutCurveType, true);
                }

                points << QPointF(xBlendOutEnd, lrc.bottom());

                painter->drawPolyline(points);

                // draw \ blend out line for the anim clips
                if (track->GetParameterType() == SEQUENCER_PARAM_ANIMLAYER)
                {
                    if (track->GetNumSecondarySelPts(i) >= CClipTrack::eCKSS_BlendOut)
                    {
                        painter->drawLine(xBlendOutStart, lrc.top(), xKeyTimeEnd, lrc.bottom());
                    }
                }
            }

            // -- draw the vertical blend time seperator lines
            for (int s = 0; s < numSecondarySelPts; s++)
            {
                const float secondaryTime = track->GetSecondaryTime(i, s + 1);
                if (secondaryTime > 0.0f)
                {
                    const int x2 = TimeToClient(secondaryTime);

                    const QPen penThin(QColor(80, 100, 80));
                    painter->setPen(penThin);
                    painter->drawLine(x2 + 1, rc.top(), x2 + 1, rc.bottom());
                    painter->drawLine(x2 - 1, rc.top(), x2 - 1, rc.bottom());
                }
            }
        }

        if ((renderFlags & DRAW_NAMES) && !description.isNull())
        {
            if (hasDuration)
            {
                keydesc = QString();
            }
            else
            {
                keydesc = QStringLiteral("{");
            }
            keydesc += description;
            if (!hasDuration)
            {
                keydesc += QStringLiteral("}");
            }
            // Draw key description text.
            // Find next key.
            int x1 = rc.right(); // End of dope sheet
            int nextKey = track->NextKeyByTime(i);
            if (nextKey > 0)
            {
                float nextKeyTime = track->GetKeyTime(nextKey);
                if (paramType == SEQUENCER_PARAM_FRAGMENTID)
                {
                    CFragmentKey nextFragKey;
                    track->GetKey(nextKey, &nextFragKey);
                    if (nextFragKey.transition)
                    {
                        nextKeyTime = max(nextKeyTime, nextFragKey.tranStartTime);
                    }

                    CFragmentTrack* pFragTrack = (CFragmentTrack*)track;
                    int nextFragID = pFragTrack->GetNextFragmentKey(i);
                    if (nextFragID >= 0)
                    {
                        nextKeyTime = track->GetKeyTime(nextFragID);
                    }
                    else
                    {
                        nextKeyTime = rc.right();
                    }
                }
                x1 = TimeToClient(nextKeyTime) - 10;
            }

            const int hasBlendTime = track->GetNumSecondarySelPts(i) > 0;
            const int blendEnd = hasBlendTime ? TimeToClient(track->GetSecondaryTime(i, 1)) : xClipStart;
            const int textStart = max(int(rc.left()), blendEnd) + 4;
            int textEnd = x1;
            if (limitTextToDuration)
            {
                textEnd = min(x1, xKeyTimeEnd);
            }
            const QRect textRect(QPoint(textStart, rc.top()), QPoint(textEnd, rc.bottom()));
            painter->setPen(KEY_TEXT_COLOR);
            painter->drawText(textRect, Qt::TextSingleLine | Qt::AlignLeft | Qt::AlignVCenter, painter->fontMetrics().elidedText(keydesc, Qt::ElideRight, textRect.width(), Qt::TextSingleLine));
        }

        // don't display key handles that aren't on screen, only applies to zero length keys now anyway
        if (xKeyTime < 0)
        {
            continue;
        }

        if (renderFlags & DRAW_HANDLES)
        {
            if (track->CanEditKey(i) && !hasDuration)
            {
                int imageID = 0;
                if (abs(xKeyTime - prevKeyPixel) < 2)
                {
                    // If multiple keys on the same time.
                    imageID = 2;
                }
                else if (track->IsKeySelected(i))
                {
                    imageID = 1;
                }

                const int iconOffset = (gSettings.mannequinSettings.trackSize / 2) - 8;

                painter->drawPixmap(QPoint(xKeyTime - 6, rc.top() + 2 + iconOffset), m_imageList[imageID]);
            }

            prevKeyPixel = xKeyTime;
        }
    }
    painter->setFont(prevFont);
}

//////////////////////////////////////////////////////////////////////////
int CSequencerDopeSheet::FirstKeyFromPoint(const QPoint& point, bool exact) const
{
    const int item = ItemFromPoint(point);
    if (item < 0)
    {
        return -1;
    }

    const float t1 = TimeFromPointUnsnapped(QPoint(point.x() - 4, point.y()));
    const float t2 = TimeFromPointUnsnapped(QPoint(point.x() + 4, point.y()));

    CSequencerTrack* track = GetTrack(item);
    if (!track)
    {
        return -1;
    }

    const int numKeys = track->GetNumKeys();

    for (int i = numKeys - 1; i >= 0; i--)
    {
        const float time = track->GetKeyTime(i);
        if (time >= t1 && time <= t2)
        {
            return i;
        }
    }

    if (!exact)
    {
        typedef std::vector<std::pair<int, float> > TGoodKeys;
        TGoodKeys goodKeys;
        for (int i = numKeys - 1; i >= 0; i--)
        {
            const float time = track->GetKeyTime(i);
            const float duration = track->GetKeyDuration(i);
            if (time + duration >= t1 && time <= t2)
            {
                //return i;
                goodKeys.push_back(std::pair<int, float>(i, time));
            }
        }

        // pick the "best" key
        if (!goodKeys.empty())
        {
            int bestKey = goodKeys.front().first;
            float prevStart = -10000.0f;
            float prevEnd = 10000.0f;
            for (TGoodKeys::const_iterator itey = goodKeys.begin(); itey != goodKeys.end(); ++itey)
            {
                const int currKey = (*itey).first;
                const float duration = track->GetKeyDuration(currKey);
                const float start = (*itey).second;
                const float end = start + duration;
                // is this key covered by the previous best key?
                if ((prevStart <= start) && (prevEnd >= end))
                {
                    // yup, so to allow it to be picked at all we make this the new best key
                    prevStart = start;
                    prevEnd     = end;
                    bestKey     = currKey;
                }
            }

            return bestKey;
        }
    }
    return -1;
}

//////////////////////////////////////////////////////////////////////////
int CSequencerDopeSheet::LastKeyFromPoint(const QPoint& point, bool exact) const
{
    const int item = ItemFromPoint(point);
    if (item < 0)
    {
        return -1;
    }

    const float t1 = TimeFromPointUnsnapped(QPoint(point.x() - 4, point.y()));
    const float t2 = TimeFromPointUnsnapped(QPoint(point.x() + 4, point.y()));

    CSequencerTrack* track = GetTrack(item);
    if (!track)
    {
        return -1;
    }

    if ((track->GetFlags() & CSequencerTrack::SEQUENCER_TRACK_READONLY) != 0)
    {
        return -1;
    }
    const int numKeys = track->GetNumKeys();

    for (int i = numKeys - 1; i >= 0; i--)
    {
        const float time = track->GetKeyTime(i);
        if (time >= t1 && time <= t2)
        {
            return i;
        }
    }

    if (!exact)
    {
        typedef std::vector<std::pair<int, float> > TGoodKeys;
        TGoodKeys goodKeys;
        for (int i = 0; i < numKeys; ++i)
        {
            const float time = track->GetKeyTime(i);
            const float duration = track->GetKeyDuration(i);
            if (time + duration >= t1 && time <= t2)
            {
                //return i;
                goodKeys.push_back(std::pair<int, float>(i, time));
            }
        }

        // pick the "best" key
        if (!goodKeys.empty())
        {
            int bestKey = goodKeys.rbegin()->first;
            float prevStart = -10000.0f;
            float prevEnd = 10000.0f;
            for (TGoodKeys::const_reverse_iterator itey = goodKeys.rbegin(); itey != goodKeys.rend(); ++itey)
            {
                const int currKey = (*itey).first;
                const float duration = track->GetKeyDuration(currKey);
                const float start = (*itey).second;
                const float end = start + duration;
                // is this key covered by the previous best key?
                if ((prevStart <= start) && (prevEnd >= end))
                {
                    // yup, so to allow it to be picked at all we make this the new best key
                    prevStart = start;
                    prevEnd     = end;
                    bestKey     = currKey;
                }
            }

            return bestKey;
        }
    }
    return -1;
}

//////////////////////////////////////////////////////////////////////////
int CSequencerDopeSheet::NumKeysFromPoint(const QPoint& point) const
{
    int item = ItemFromPoint(point);
    if (item < 0)
    {
        return -1;
    }

    float t1 = TimeFromPointUnsnapped(QPoint(point.x() - 4, point.y()));
    float t2 = TimeFromPointUnsnapped(QPoint(point.x() + 4, point.y()));

    CSequencerTrack* track = GetTrack(item);
    if (!track)
    {
        return -1;
    }

    int count = 0;
    int numKeys = track->GetNumKeys();
    for (int i = 0; i < numKeys; i++)
    {
        float time = track->GetKeyTime(i);
        if (time >= t1 && time <= t2)
        {
            ++count;
        }
    }
    return count;
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheet::SelectKeys(const QRect& rc)
{
    // put selection rectangle from client to item space.
    const QRect rci = rc.translated(m_scrollOffset);

    Range selTime = GetTimeRange(rci);

    QRect rcItem;
    for (int i = 0; i < GetCount(); i++)
    {
        GetItemRect(i, rcItem);
        // Decrease item rectangle a bit.
        rcItem.adjust(4, 4, -4, -4);
        // Check if item rectanle intersects with selection rectangle in y axis.
        if ((rcItem.top() >= rc.top() && rcItem.top() <= rc.bottom()) ||
            (rcItem.bottom() >= rc.top() && rcItem.bottom() <= rc.bottom()) ||
            (rc.top() >= rcItem.top() && rc.top() <= rcItem.bottom()) ||
            (rc.bottom() >= rcItem.top() && rc.bottom() <= rcItem.bottom()))
        {
            CSequencerTrack* track = GetTrack(i);
            if (!track || (track->GetFlags() & CSequencerTrack::SEQUENCER_TRACK_READONLY) != 0)
            {
                continue;
            }

            // Check which keys we intersect.
            for (int j = 0; j < track->GetNumKeys(); j++)
            {
                float time = track->GetKeyTime(j);
                if (selTime.IsInside(time))
                {
                    track->SelectKey(j, true);
                    m_bAnySelected = true;
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheet::DrawNodeItem(CSequencerNode* pAnimNode, QPainter* painter, const QRect& rcItem)
{
    const QFont prevFont = painter->font();
    painter->setFont(m_descriptionFont);

    painter->setPen(KEY_TEXT_COLOR);

    const QRect textRect = rcItem.adjusted(4, 0, -4, 0);

    string sAnimNodeName = pAnimNode->GetName();
    painter->drawText(textRect, Qt::TextSingleLine | Qt::AlignLeft | Qt::AlignVCenter, painter->fontMetrics().elidedText(QString::fromLatin1(sAnimNodeName.c_str()), Qt::ElideRight, textRect.width(), Qt::TextSingleLine));

    painter->setFont(prevFont);
}

#include <Mannequin/SequencerDopeSheet.moc>
