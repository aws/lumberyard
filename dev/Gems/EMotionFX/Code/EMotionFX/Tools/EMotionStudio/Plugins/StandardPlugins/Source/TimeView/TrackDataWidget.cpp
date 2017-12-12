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

// include the required headers
#include "TrackDataWidget.h"
#include "TimeViewPlugin.h"
#include "TimeInfoWidget.h"
#include "TrackHeaderWidget.h"
#include <QPainter>
#include <QToolTip>
#include <QPaintEvent>
#include <QMenu>

#include <MCore/Source/LogManager.h>
#include <MCore/Source/Algorithms.h>

#include <MysticQt/Source/MysticQtManager.h>

#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>

#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionEvent.h>
#include <EMotionFX/Source/MotionEventTrack.h>
#include <EMotionFX/Source/Recorder.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/CommandSystem/Source/MotionEventCommands.h>
#include "../MotionWindow/MotionWindowPlugin.h"
#include "../MotionEvents/MotionEventsPlugin.h"
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "../../../../EMStudioSDK/Source/MainWindow.h"

namespace EMStudio
{
    // the constructor
    //TrackDataWidget::TrackDataWidget(TimeViewPlugin* plugin, QWidget* parent) : QGLWidget(parent)
    TrackDataWidget::TrackDataWidget(TimeViewPlugin* plugin, QWidget* parent)
        : QOpenGLWidget(parent)
        , QOpenGLFunctions()
    {
        setObjectName("TrackDataWidget");

        mPlugin             = plugin;
        mLastLeftClickedX   = 0;
        mLastMouseX         = 0;
        mLastMouseMoveX     = 0;
        mLastMouseY         = 0;
        mLastMouseMoveY     = 0;
        mNodeHistoryItemHeight = 20;
        mRectZooming        = false;
        mMouseLeftClicked   = false;
        mMouseRightClicked  = false;
        mMouseMidClicked    = false;
        mDragging           = false;
        mMotionPaused       = false;
        mIsScrolling        = false;
        mAllowContextMenu   = true;
        mDraggingElement    = nullptr;
        mDragElementTrack   = nullptr;
        mTimeLineHeight     = 34;
        mResizeElement      = nullptr;
        mGraphStartHeight   = 0;
        mEventsStartHeight  = 0;
        mNodeRectsStartHeight = 0;
        mEventHistoryTotalHeight = 0;

        // rect selection
        mSelectStart        = QPoint(0, 0);
        mSelectEnd          = QPoint(0, 0);
        mRectSelecting      = false;

        // init brushes and pens
        //  mBrushBackground            = QBrush( QColor(55, 60, 70), Qt::SolidPattern );
        mBrushBackground            = QBrush(QColor(40, 45, 50), Qt::SolidPattern);
        mBrushBackgroundClipped     = QBrush(QColor(40, 40, 40), Qt::SolidPattern);
        mBrushBackgroundOutOfRange  = QBrush(QColor(35, 35, 35), Qt::SolidPattern);
        //  mPenTimeHandles     = QPen( QColor(100, 100, 100), 1, Qt::PenStyle::DashLine );

        mHeaderGradientActive       = QLinearGradient(0, 0, 0, 35);
        mHeaderGradientActive.setColorAt(1.0f, QColor(100, 105, 110));
        mHeaderGradientActive.setColorAt(0.5f, QColor(30, 35, 40));
        mHeaderGradientActive.setColorAt(0.0f, QColor(20, 20, 20));

        mHeaderGradientActiveFocus  = QLinearGradient(0, 0, 0, 35);
        mHeaderGradientActiveFocus.setColorAt(1.0f, QColor(100, 105, 130));
        mHeaderGradientActiveFocus.setColorAt(0.5f, QColor(30, 35, 40));
        mHeaderGradientActiveFocus.setColorAt(0.0f, QColor(20, 20, 20));

        mHeaderGradientInactive     = QLinearGradient(0, 0, 0, 35);
        mHeaderGradientInactive.setColorAt(1.0f, QColor(30, 30, 30));
        mHeaderGradientInactive.setColorAt(0.0f, QColor(20, 20, 20));

        mHeaderGradientInactiveFocus = QLinearGradient(0, 0, 0, 35);
        mHeaderGradientInactiveFocus.setColorAt(1.0f, QColor(30, 30, 30));
        mHeaderGradientInactiveFocus.setColorAt(0.0f, QColor(20, 20, 20));

        mPenTimeStepLinesActive         = QPen(QColor(60, 60, 60));
        mPenMidTimeStepLinesActive      = QPen(QColor(90, 90, 90));
        mPenMainTimeStepLinesActive     = QPen(QColor(110, 110, 110));

        mPenTimeStepLinesInactive       = QPen(QColor(40, 40, 40));
        mPenMidTimeStepLinesInactive    = QPen(QColor(60, 60, 60));
        mPenMainTimeStepLinesInactive   = QPen(QColor(90, 90, 90));

        mTimeLineFont.setPixelSize(12);
        mDataFont.setPixelSize(13);
        mTitleFont.setPixelSize(13);
        mTitleFont.setBold(true);

        // load the time handle top image
        QString imageName = MysticQt::GetMysticQt()->GetDataDir().AsChar();
        mTimeHandleTop = QPixmap(imageName + "Images/Icons/TimeHandleTop.png");

        setMouseTracking(true);
        setAcceptDrops(true);
        setAutoFillBackground(false);

        setFocusPolicy(Qt::StrongFocus);
    }


    // destructor
    TrackDataWidget::~TrackDataWidget()
    {
    }


    // init gl
    void TrackDataWidget::initializeGL()
    {
        initializeOpenGLFunctions();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    }


    // resize
    void TrackDataWidget::resizeGL(int w, int h)
    {
        MCORE_UNUSED(w);
        MCORE_UNUSED(h);
        if (mPlugin)
        {
            mPlugin->SetRedrawFlag();
        }
    }


    // calculate the selection rect
    void TrackDataWidget::CalcSelectRect(QRect& outRect)
    {
        const int32 startX = MCore::Min<int32>(mSelectStart.x(), mSelectEnd.x());
        const int32 startY = MCore::Min<int32>(mSelectStart.y(), mSelectEnd.y());
        const int32 width  = abs(mSelectEnd.x() - mSelectStart.x());
        const int32 height = abs(mSelectEnd.y() - mSelectStart.y());

        outRect = QRect(startX, startY, width, height);
    }


    void TrackDataWidget::paintGL()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // start painting
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, false);

        QRect rect(0, 0, geometry().width(), geometry().height());

        // draw a background rect
        painter.setPen(Qt::NoPen);
        painter.setBrush(mBrushBackgroundOutOfRange);
        painter.drawRect(rect);
        painter.setFont(mDataFont);

        // if there is a recording show that, otherwise show motion tracks
        if (EMotionFX::GetRecorder().GetRecordTime() > MCore::Math::epsilon)
        {
            PaintRecorder(painter, rect);
        }
        else
        {
            PaintMotionTracks(painter, rect);
        }

        // draw the timeline
        painter.setRenderHint(QPainter::Antialiasing, false);
        DrawTimeLine(painter, rect);

        const uint32 height = geometry().height();
        mPlugin->RenderElementTimeHandles(painter, height, mPlugin->mPenTimeHandles);

        DrawTimeMarker(painter, rect);

        //painter.setPen( Qt::yellow );
        //painter.setBrush( Qt::NoBrush );
        //painter.drawRect( mNodeHistoryRect );

        //painter.setPen(Qt::yellow);
        //painter.drawLine(0,mPlugin->mMaxHeight+mTimeLineHeight+1-mPlugin->mScrollY, geometry().width(), mPlugin->mMaxHeight+mTimeLineHeight+1-mPlugin->mScrollY);

        // render selection rect
        if (mRectSelecting)
        {
            painter.resetTransform();
            QRect selectRect;
            CalcSelectRect(selectRect);

            if (mRectZooming)
            {
                painter.setBrush(QColor(0, 100, 200, 75));
                painter.setPen(QColor(0, 100, 255));
                painter.drawRect(selectRect);
            }
            else
            {
                if (EMotionFX::GetRecorder().GetRecordTime() < MCore::Math::epsilon && mPlugin->mMotion)
                {
                    painter.setBrush(QColor(200, 120, 0, 75));
                    painter.setPen(QColor(255, 128, 0));
                    painter.drawRect(selectRect);
                }
            }
        }
    }

    /*
    // paint event
    void TrackDataWidget::paintEvent(QPaintEvent* event)
    {

        // start painting
        QPainter painter;
        painter.begin(this);
        painter.setRenderHint(QPainter::Antialiasing, false);

        // draw a background rect
        painter.setPen(Qt::NoPen);
        painter.setBrush( mBrushBackgroundOutOfRange );
        painter.drawRect( event->rect() );
        painter.setFont( mDataFont );

        // if there is a recording show that, otherwise show motion tracks
        if (EMotionFX::GetRecorder().GetRecordTime() > MCore::Math::epsilon)
            PaintRecorder(painter, event);
        else
            PaintMotionTracks(painter, event);

        // draw the timeline
        painter.setRenderHint(QPainter::Antialiasing, false);
        DrawTimeLine(painter, event);

        const uint32 height = geometry().height();
        mPlugin->RenderElementTimeHandles(painter, height, mPlugin->mPenTimeHandles);

        DrawTimeMarker(painter, event);

        //painter.setPen( Qt::yellow );
        //painter.setBrush( Qt::NoBrush );
        //painter.drawRect( mNodeHistoryRect );

        //painter.setPen(Qt::yellow);
        //painter.drawLine(0,mPlugin->mMaxHeight+mTimeLineHeight+1-mPlugin->mScrollY, geometry().width(), mPlugin->mMaxHeight+mTimeLineHeight+1-mPlugin->mScrollY);

        // render selection rect
        if (mRectSelecting)
        {
            painter.resetTransform();
            QRect selectRect;
            CalcSelectRect( selectRect );

            if (mRectZooming)
            {
                painter.setBrush( QColor(0, 100, 200, 75) );
                painter.setPen( QColor(0,100,255) );
                painter.drawRect( selectRect );
            }
            else
            {
                if (EMotionFX::GetRecorder().GetRecordTime() < MCore::Math::epsilon && mPlugin->mMotion)
                {
                    painter.setBrush( QColor(200, 120, 0, 75) );
                    painter.setPen( QColor(255,128,0) );
                    painter.drawRect( selectRect );
                }
            }
        }

        painter.setRenderHint(QPainter::Antialiasing, true);

        // stop painting
        painter.end();

    }
    */

    // draw the time marker
    void TrackDataWidget::DrawTimeMarker(QPainter& painter, const QRect& rect)
    {
        if (mPlugin->mTrackDataWidget->mDraggingElement == nullptr && mPlugin->mTrackDataWidget->mResizeElement == nullptr && hasFocus())
        {
            painter.setPen(mPlugin->mPenCurTimeHelper);
            painter.drawLine(mPlugin->mCurMouseX, 14, mPlugin->mCurMouseX, rect.bottom());
        }

        // draw the current time marker
        float startHeight = 0.0f;
        const float curTimeX = mPlugin->TimeToPixel(mPlugin->mCurTime);
        painter.drawPixmap(curTimeX - (mTimeHandleTop.width() / 2) - 1, 0, mTimeHandleTop);

        painter.setPen(mPlugin->mPenCurTimeHandle);
        painter.drawLine(QPointF(curTimeX, startHeight), QPointF(curTimeX, rect.bottom()));
    }


    // paint the recorder data
    void TrackDataWidget::PaintRecorder(QPainter& painter, const QRect& rect)
    {
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        //painter.setRenderHint( QPainter::Antialiasing, true );
        //painter.setRenderHint( QPainter::HighQualityAntialiasing, true );

        EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();

        QRect backgroundRect    = rect;
        QRect motionRect        = rect;

        const float animationLength = recorder.GetRecordTime();
        const double animEndPixel = mPlugin->TimeToPixel(animationLength);
        backgroundRect.setLeft(animEndPixel);
        motionRect.setRight(animEndPixel);
        motionRect.setTop(mTimeLineHeight);
        backgroundRect.setTop(mTimeLineHeight);

        // render the rects
        painter.setPen(Qt::NoPen);
        painter.setBrush(mBrushBackground);
        painter.drawRect(motionRect);
        painter.setBrush(mBrushBackgroundOutOfRange);
        painter.drawRect(backgroundRect);

        // if the whole contents are outside of view, skip the full rendering of it
        //  if (geometry().intersects( mNodeHistoryRect ) == false)
        //      return;

        // find the selected actor instance
        EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        if (actorInstance == nullptr)
        {
            return;
        }

        // find the actor instance data for this actor instance
        const uint32 actorInstanceDataIndex = recorder.FindActorInstanceDataIndex(actorInstance);
        if (actorInstanceDataIndex == MCORE_INVALIDINDEX32) // it doesn't exist, so we didn't record anything for this actor instance
        {
            return;
        }

        // get the actor instance data for the first selected actor instance, and render the node history for that
        const EMotionFX::Recorder::ActorInstanceData* actorInstanceData = &recorder.GetActorInstanceData(actorInstanceDataIndex);

        const bool displayNodeActivity  = mPlugin->mTrackHeaderWidget->mNodeActivityCheckBox->isChecked();
        const bool displayEvents        = mPlugin->mTrackHeaderWidget->mEventsCheckBox->isChecked();
        const bool displayRelativeGraph = mPlugin->mTrackHeaderWidget->mRelativeGraphCheckBox->isChecked();

        int32 startOffset = mTimeLineHeight - mPlugin->mScrollY;
        bool isTop = true;

        if (displayNodeActivity)
        {
            mNodeRectsStartHeight = startOffset;
            PaintRecorderNodeHistory(painter, rect, actorInstanceData);
            isTop = false;
            startOffset = mNodeHistoryRect.bottom();
        }

        if (displayEvents)
        {
            if (isTop == false)
            {
                mEventsStartHeight = startOffset + 10;
                mEventsStartHeight += PaintSeparator(painter, mEventsStartHeight, animationLength);
                mEventsStartHeight += 10;
                startOffset = mEventsStartHeight;
            }
            else
            {
                startOffset += 3;
                mEventsStartHeight = startOffset;
            }

            startOffset += mEventHistoryTotalHeight;
            isTop = false;

            PaintRecorderEventHistory(painter, rect, actorInstanceData);
        }

        if (displayRelativeGraph)
        {
            if (isTop == false)
            {
                mGraphStartHeight = startOffset + 10;
                mGraphStartHeight += PaintSeparator(painter, mGraphStartHeight, animationLength);
                //mGraphStartHeight += 10;
                startOffset = mGraphStartHeight;
            }
            else
            {
                startOffset += 3;
                mGraphStartHeight = startOffset;
            }

            //startOffset += mRelativeGraphTotalHeight;
            isTop = false;

            PaintRelativeGraph(painter, rect, actorInstanceData);
        }
    }



    // paint the relative graph
    void TrackDataWidget::PaintRelativeGraph(QPainter& painter, const QRect& rect, const EMotionFX::Recorder::ActorInstanceData* actorInstanceData)
    {
        EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();
        const double animationLength = recorder.GetRecordTime();
        if (animationLength < MCore::Math::epsilon)
        {
            return;
        }

        painter.setRenderHint(QPainter::Antialiasing, true);

        // get the history items shortcut
        const MCore::Array<EMotionFX::Recorder::NodeHistoryItem*>&  historyItems = actorInstanceData->mNodeHistoryItems;
        int32 windowWidth = geometry().width();

        const bool useNodeColors = mPlugin->mTrackHeaderWidget->mNodeTypeColorsCheckBox->isChecked();
        const bool showNodeNames = mPlugin->mTrackHeaderWidget->mNodeNamesCheckBox->isChecked();
        const bool showMotionFiles = mPlugin->mTrackHeaderWidget->mMotionFilesCheckBox->isChecked();
        const bool limitGraphHeight = mPlugin->mTrackHeaderWidget->mLimitGraphHeightCheckBox->isChecked();

        float graphHeight = geometry().height() - mGraphStartHeight;
        float graphBottom;
        if (limitGraphHeight == false)
        {
            graphBottom = geometry().height();
        }
        else
        {
            if (graphHeight > 200)
            {
                graphHeight = 200;
            }

            graphBottom = mGraphStartHeight + graphHeight;
        }

        const uint32 graphContentsCode = mPlugin->mTrackHeaderWidget->mGraphContentsComboBox->currentIndex();

        const uint32 numItems = historyItems.GetLength();
        for (uint32 i = 0; i < numItems; ++i)
        {
            EMotionFX::Recorder::NodeHistoryItem* curItem = historyItems[i];

            double startTimePixel   = mPlugin->TimeToPixel(curItem->mStartTime);
            double endTimePixel     = mPlugin->TimeToPixel(curItem->mEndTime);

            const QRect itemRect(QPoint(startTimePixel, mGraphStartHeight), QPoint(endTimePixel, geometry().height()));
            if (rect.intersects(itemRect) == false)
            {
                continue;
            }

            const uint32 colorCode = (useNodeColors) ? curItem->mTypeColor : curItem->mColor;
            QColor color(MCore::ExtractRed(colorCode), MCore::ExtractGreen(colorCode), MCore::ExtractBlue(colorCode));

            if (mPlugin->mNodeHistoryItem != curItem || mIsScrolling || mPlugin->mIsAnimating)
            {
                painter.setPen(color);
                color.setAlpha(64);
                painter.setBrush(color);
            }
            else
            {
                color = QColor(255, 128, 0);
                painter.setPen(color);
                color.setAlpha(128);
                painter.setBrush(color);
            }

            QPainterPath path;
            int32 widthInPixels = (endTimePixel - startTimePixel);
            if (widthInPixels > 0)
            {
                EMotionFX::KeyTrackLinearDynamic<float, float>* keyTrack = &curItem->mGlobalWeights; // init on global weights
                if (graphContentsCode == 1)
                {
                    keyTrack = &curItem->mLocalWeights;
                }
                else
                if (graphContentsCode == 2)
                {
                    keyTrack = &curItem->mPlayTimes;
                }

                float lastWeight = keyTrack->GetValueAtTime(0.0f, &curItem->mCachedKey);
                const float keyTimeStep = (curItem->mEndTime - curItem->mStartTime) / (float)widthInPixels;

                const int32 pixelStepSize = 1;//(widthInPixels / 300.0f) + 1;

                path.moveTo(QPointF(startTimePixel, graphBottom + 1));
                path.lineTo(QPointF(startTimePixel, graphBottom - 1 - lastWeight * graphHeight));
                //path.moveTo( QPointF(startTimePixel, graphBottom+1) );
                //path.lineTo( QPointF(startTimePixel+1, graphBottom-1-lastWeight*graphHeight) );
                bool firstPixel = true;
                for (int32 w = 1; w < widthInPixels - 1; w += pixelStepSize)
                {
                    if (startTimePixel + w > windowWidth)
                    {
                        break;
                    }

                    if (firstPixel && startTimePixel < 0)
                    {
                        w = -startTimePixel;
                        firstPixel = false;
                    }

                    const float weight = keyTrack->GetValueAtTime(w * keyTimeStep, &curItem->mCachedKey);
                    const float height = graphBottom - weight * graphHeight;
                    path.lineTo(QPointF(startTimePixel + w + 1, height));
                }

                const float weight = keyTrack->GetValueAtTime(curItem->mEndTime, &curItem->mCachedKey);
                const float height = graphBottom - weight * graphHeight;
                path.lineTo(QPointF(startTimePixel + widthInPixels - 1, height));
                //          path.lineTo( QPointF(startTimePixel+widthInPixels+2, graphBottom+1) );
                path.lineTo(QPointF(startTimePixel + widthInPixels, graphBottom + 1));
                painter.drawPath(path);
            }
        }

        // calculate the remapped track list, based on sorted global weight, with the most influencing track on top
        recorder.ExtractNodeHistoryItems(*actorInstanceData, mPlugin->mCurTime, true, (EMotionFX::Recorder::EValueType)graphContentsCode, &mActiveItems, &mTrackRemap);

        // display the values and names
        uint32 offset = 0;
        const uint32 numActiveItems = mActiveItems.GetLength();
        for (uint32 i = 0; i < numActiveItems; ++i)
        {
            EMotionFX::Recorder::NodeHistoryItem* curItem = mActiveItems[i].mNodeHistoryItem;
            if (curItem == nullptr)
            {
                continue;
            }

            offset += 15;

            mTempString.clear();
            if (showNodeNames)
            {
                mTempString += curItem->mName.AsChar();
            }

            if (showMotionFiles && curItem->mMotionFileName.GetLength() > 0)
            {
                if (!mTempString.empty())
                {
                    mTempString += " - ";
                }

                mTempString += curItem->mMotionFileName.AsChar();
            }

            if (!mTempString.empty())
            {
                mTempString += AZStd::string::format(" = %.4f", mActiveItems[i].mValue);
            }
            else
            {
                mTempString = AZStd::string::format("%.4f", mActiveItems[i].mValue);
            }

            const uint32 colorCode = (useNodeColors) ? mActiveItems[i].mNodeHistoryItem->mTypeColor : mActiveItems[i].mNodeHistoryItem->mColor;
            QColor color(MCore::ExtractRed(colorCode), MCore::ExtractGreen(colorCode), MCore::ExtractBlue(colorCode));
            painter.setPen(color);
            painter.setBrush(Qt::NoBrush);
            painter.setFont(mDataFont);
            painter.drawText(3, offset + mGraphStartHeight, mTempString.c_str());
        }
    }


    // paint the events
    void TrackDataWidget::PaintRecorderEventHistory(QPainter& painter, const QRect& rect, const EMotionFX::Recorder::ActorInstanceData* actorInstanceData)
    {
        EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();

        const double animationLength = recorder.GetRecordTime();
        if (animationLength < MCore::Math::epsilon)
        {
            return;
        }

        // get the history items shortcut
        const MCore::Array<EMotionFX::Recorder::EventHistoryItem*>& historyItems = actorInstanceData->mEventHistoryItems;
        //  int32 windowWidth = geometry().width();

        //painter.setPen( QColor(60,70,80) );
        //painter.setBrush( Qt::NoBrush );
        //painter.drawLine( QPoint(0, mNodeHistoryRect.bottom() + 2), QPoint(mPlugin->TimeToPixel(animationLength), mNodeHistoryRect.bottom() + 2));

        QRect clipRect = rect;
        clipRect.setRight(mPlugin->TimeToPixel(animationLength));
        painter.setClipRect(clipRect);
        painter.setClipping(true);

        // for all event history items
        const float tickHalfWidth = 7;
        const float tickHeight = 16;

        QPointF tickPoints[6];
        const uint32 numItems = historyItems.GetLength();
        for (uint32 i = 0; i < numItems; ++i)
        {
            EMotionFX::Recorder::EventHistoryItem* curItem = historyItems[i];

            float height = (curItem->mTrackIndex * 20) + mEventsStartHeight;
            double startTimePixel   = mPlugin->TimeToPixel(curItem->mStartTime);
            //      double endTimePixel     = mPlugin->TimeToPixel( curItem->mEndTime );

            const QRect itemRect(QPoint(startTimePixel - tickHalfWidth, height), QSize(tickHalfWidth * 2, tickHeight));
            if (rect.intersects(itemRect) == false)
            {
                continue;
            }

            // try to locate the node based on its unique ID
            QColor borderColor(30, 30, 30);
            const uint32 colorCode = curItem->mColor;
            QColor color;

            color = QColor(MCore::ExtractRed(colorCode), MCore::ExtractGreen(colorCode), MCore::ExtractBlue(colorCode), MCore::ExtractAlpha(colorCode));

            if (mIsScrolling == false && mPlugin->mIsAnimating == false)
            {
                if (mPlugin->mNodeHistoryItem && mPlugin->mNodeHistoryItem->mNodeUniqueID == curItem->mEmitterUniqueID)
                {
                    if (curItem->mStartTime >= mPlugin->mNodeHistoryItem->mStartTime && curItem->mStartTime <= mPlugin->mNodeHistoryItem->mEndTime)
                    {
                        if (mPlugin->mTrackHeaderWidget->mNodeActivityCheckBox->isChecked())
                        {
                            borderColor = QColor(255, 128, 0);
                            color = QColor(255, 128, 0);
                        }
                    }
                }

                if (mPlugin->mEventHistoryItem == curItem)
                {
                    //color = color.lighter( 130 );
                    //borderColor = borderColor.lighter( 130 );
                    borderColor = QColor(255, 128, 0);
                    color = borderColor;
                }
            }

            /*      EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID( curItem->mAnimGraphID );
                    if (animGraph)
                    {
                        EMotionFX::AnimGraphNode* foundNode = animGraph->RecursiveFindNodeByUniqueID( curItem->mEmitterUniqueID );
                        if (foundNode)
                            borderColor = QColor(30, 30, 30);
                    }*/

            const QColor gradientColor = QColor(color.red() / 2, color.green() / 2, color.blue() / 2, color.alpha());
            QLinearGradient gradient(0, height, 0, height + tickHeight);
            gradient.setColorAt(0.0f, color);
            gradient.setColorAt(1.0f, gradientColor);

            painter.setPen(Qt::red);
            painter.setBrush(Qt::black);

            tickPoints[0] = QPoint(startTimePixel,                  height);
            tickPoints[1] = QPoint(startTimePixel + tickHalfWidth,    height + tickHeight / 2);
            tickPoints[2] = QPoint(startTimePixel + tickHalfWidth,    height + tickHeight);
            tickPoints[3] = QPoint(startTimePixel - tickHalfWidth,    height + tickHeight);
            tickPoints[4] = QPoint(startTimePixel - tickHalfWidth,    height + tickHeight / 2);
            tickPoints[5] = QPoint(startTimePixel,                  height);

            painter.setPen(Qt::NoPen);
            painter.setBrush(gradient);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.drawPolygon(tickPoints, 5, Qt::WindingFill);
            painter.setRenderHint(QPainter::Antialiasing, false);

            painter.setBrush(Qt::NoBrush);
            painter.setPen(borderColor);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.drawPolyline(tickPoints, 6);
            painter.setRenderHint(QPainter::Antialiasing, false);
        }

        painter.setClipping(false);
    }


    // paint the node history
    void TrackDataWidget::PaintRecorderNodeHistory(QPainter& painter, const QRect& rect, const EMotionFX::Recorder::ActorInstanceData* actorInstanceData)
    {
        MCORE_UNUSED(rect);

        EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();

        const double animationLength = recorder.GetRecordTime();
        if (animationLength < MCore::Math::epsilon)
        {
            return;
        }

        // skip the complete rendering of the node history data when its bounds are not inside view
        if (geometry().intersects(mNodeHistoryRect) == false)
        {
            return;
        }

        // get the history items shortcut
        const MCore::Array<EMotionFX::Recorder::NodeHistoryItem*>&  historyItems = actorInstanceData->mNodeHistoryItems;
        int32 windowWidth = geometry().width();

        // calculate the remapped track list, based on sorted global weight, with the most influencing track on top
        const bool sorted = mPlugin->mTrackHeaderWidget->mSortNodeActivityCheckBox->isChecked();
        const uint32 graphContentsCode = mPlugin->mTrackHeaderWidget->mNodeContentsComboBox->currentIndex();
        recorder.ExtractNodeHistoryItems(*actorInstanceData, mPlugin->mCurTime, sorted, (EMotionFX::Recorder::EValueType)graphContentsCode, &mActiveItems, &mTrackRemap);

        const bool useNodeColors = mPlugin->mTrackHeaderWidget->mNodeTypeColorsCheckBox->isChecked();
        const bool showNodeNames = mPlugin->mTrackHeaderWidget->mNodeNamesCheckBox->isChecked();
        const bool showMotionFiles = mPlugin->mTrackHeaderWidget->mMotionFilesCheckBox->isChecked();

        const uint32 nodeContentsCode = mPlugin->mTrackHeaderWidget->mNodeContentsComboBox->currentIndex();

        // for all history items
        QRectF itemRect;
        const uint32 numItems = historyItems.GetLength();
        for (uint32 i = 0; i < numItems; ++i)
        {
            EMotionFX::Recorder::NodeHistoryItem* curItem = historyItems[i];

            // draw the background rect
            double startTimePixel   = mPlugin->TimeToPixel(curItem->mStartTime);
            double endTimePixel     = mPlugin->TimeToPixel(curItem->mEndTime);

            const uint32 trackIndex = mTrackRemap[ curItem->mTrackIndex ];

            itemRect.setLeft(startTimePixel);
            itemRect.setRight(endTimePixel - 1);
            //      itemRect.setTop( (mTimeLineHeight + (trackIndex * (mNodeHistoryItemHeight+3)) + 3) - mPlugin->mScrollY);
            itemRect.setTop((mNodeRectsStartHeight + (trackIndex * (mNodeHistoryItemHeight + 3)) + 3) /* - mPlugin->mScrollY*/);
            itemRect.setBottom(itemRect.top() + mNodeHistoryItemHeight);

            if (geometry().intersects(itemRect.toRect()) == false)
            {
                continue;
            }

            const uint32 colorCode = (useNodeColors) ? curItem->mTypeColor : curItem->mColor;
            QColor color(MCore::ExtractRed(colorCode), MCore::ExtractGreen(colorCode), MCore::ExtractBlue(colorCode));

            bool matchesEvent = false;
            if (mIsScrolling == false && mPlugin->mIsAnimating == false)
            {
                if (mPlugin->mNodeHistoryItem == curItem)
                {
                    color = QColor(255, 128, 0);
                }
                //color = color.lighter( 130 );

                if (mPlugin->mEventEmitterNode && mPlugin->mEventEmitterNode->GetUniqueID() == curItem->mNodeUniqueID && mPlugin->mEventHistoryItem)
                {
                    if (mPlugin->mEventHistoryItem->mStartTime >= curItem->mStartTime && mPlugin->mEventHistoryItem->mStartTime <= curItem->mEndTime)
                    {
                        color = QColor(255, 128, 0);
                        matchesEvent = true;
                    }
                }
            }

            painter.setPen(color);
            color.setAlpha(128);
            painter.setBrush(color);
            painter.drawRoundedRect(itemRect, 2, 2);

            // draw weights
            //---------
            painter.setRenderHint(QPainter::Antialiasing, true);
            //painter.setPen( Qt::black );  // weight line
            QPainterPath path;
            itemRect.setRight(itemRect.right() - 1);
            painter.setClipRegion(itemRect.toRect());
            painter.setClipping(true);

            int32 widthInPixels = (endTimePixel - startTimePixel);
            if (widthInPixels > 0)
            {
                EMotionFX::KeyTrackLinearDynamic<float, float>* keyTrack = &curItem->mGlobalWeights; // init on global weights
                if (nodeContentsCode == 1)
                {
                    keyTrack = &curItem->mLocalWeights;
                }
                else
                if (nodeContentsCode == 2)
                {
                    keyTrack = &curItem->mPlayTimes;
                }

                float lastWeight = keyTrack->GetValueAtTime(0.0f, &curItem->mCachedKey);
                const float keyTimeStep = (curItem->mEndTime - curItem->mStartTime) / (float)widthInPixels;

                const int32 pixelStepSize = 1;//(widthInPixels / 300.0f) + 1;

                path.moveTo(QPointF(startTimePixel - 1, itemRect.bottom() + 1));
                path.lineTo(QPointF(startTimePixel + 1, itemRect.bottom() - 1 - lastWeight * mNodeHistoryItemHeight));
                bool firstPixel = true;
                for (int32 w = 1; w < widthInPixels - 1; w += pixelStepSize)
                {
                    if (startTimePixel + w > windowWidth)
                    {
                        break;
                    }

                    if (firstPixel && startTimePixel < 0)
                    {
                        w = -startTimePixel;
                        firstPixel = false;
                    }

                    const float weight = keyTrack->GetValueAtTime(w * keyTimeStep, &curItem->mCachedKey);
                    const float height = itemRect.bottom() - weight * mNodeHistoryItemHeight;
                    path.lineTo(QPointF(startTimePixel + w + 1, height));
                }

                const float weight = keyTrack->GetValueAtTime(curItem->mEndTime, &curItem->mCachedKey);
                const float height = itemRect.bottom() - weight * mNodeHistoryItemHeight;
                path.lineTo(QPointF(startTimePixel + widthInPixels - 1, height));
                path.lineTo(QPointF(startTimePixel + widthInPixels, itemRect.bottom() + 1));
                //          path.lineTo( QPointF(startTimePixel+widthInPixels+2, itemRect.bottom()+1) );
                painter.drawPath(path);
                painter.setRenderHint(QPainter::Antialiasing, false);
            }
            //---------

            // draw the text
            if (matchesEvent != true)
            {
                if (mIsScrolling == false && mPlugin->mIsAnimating == false)
                {
                    if (mPlugin->mNodeHistoryItem != curItem)
                    {
                        painter.setPen(QColor(255, 255, 255, 175));
                    }
                    else
                    {
                        painter.setPen(QColor(0, 0, 0));
                    }
                }
                else
                {
                    painter.setPen(QColor(255, 255, 255, 175));
                }
            }
            else
            {
                painter.setPen(Qt::black);
            }

            mTempString.clear();
            if (showNodeNames)
            {
                mTempString += curItem->mName.AsChar();
            }

            if (showMotionFiles && curItem->mMotionFileName.GetLength() > 0)
            {
                if (!mTempString.empty())
                {
                    mTempString += " - ";
                }

                mTempString += curItem->mMotionFileName.AsChar();
            }

            if (!mTempString.empty())
            {
                painter.drawText(itemRect.left() + 3, itemRect.bottom() - 2, mTempString.c_str());
            }

            painter.setClipping(false);
        }
    }


    // paint the motion tracks for a given motion
    void TrackDataWidget::PaintMotionTracks(QPainter& painter, const QRect& rect)
    {
        double animationLength  = 0.0;
        double clipStart        = 0.0;
        double clipEnd          = 0.0;

        // get the track over which the cursor is positioned
        QPoint localCursorPos = mapFromGlobal(QCursor::pos());
        TimeTrack* mouseCursorTrack = mPlugin->GetTrackAt(localCursorPos.y());
        if (localCursorPos.x() < 0 || localCursorPos.x() > width())
        {
            mouseCursorTrack = nullptr;
        }

        // handle highlighting
        const uint32 numTracks = mPlugin->GetNumTracks();
        for (uint32 i = 0; i < numTracks; ++i)
        {
            TimeTrack* track = mPlugin->GetTrack(i);

            // set the highlighting flag for the track
            if (track == mouseCursorTrack)
            {
                // highlight the track
                track->SetIsHighlighted(true);

                // get the element over which the cursor is positioned
                TimeTrackElement* mouseCursorElement = mPlugin->GetElementAt(localCursorPos.x(), localCursorPos.y());

                // get the number of elements, iterate through them and disable the highlight flag
                const uint32 numElements = track->GetNumElements();
                for (uint32 e = 0; e < numElements; ++e)
                {
                    TimeTrackElement* element = track->GetElement(e);

                    if (element == mouseCursorElement)
                    {
                        element->SetIsHighlighted(true);
                    }
                    else
                    {
                        element->SetIsHighlighted(false);
                    }
                }
            }
            else
            {
                track->SetIsHighlighted(false);

                // get the number of elements, iterate through them and disable the highlight flag
                const uint32 numElements = track->GetNumElements();
                for (uint32 e = 0; e < numElements; ++e)
                {
                    TimeTrackElement* element = track->GetElement(e);
                    element->SetIsHighlighted(false);
                }
            }
        }

        EMotionFX::Motion* motion = mPlugin->GetMotion();
        if (motion)
        {
            // get the motion length
            animationLength = motion->GetMaxTime();

            // get the playback info and read out the clip start/end times
            EMotionFX::PlayBackInfo* playbackInfo = motion->GetDefaultPlayBackInfo();
            clipStart   = playbackInfo->mClipStartTime;
            clipEnd     = playbackInfo->mClipEndTime;

            // HACK: fix this later
            clipStart = 0.0;
            clipEnd = animationLength;
        }

        // calculate the pixel index of where the animation ends and where it gets clipped
        const double animEndPixel   = mPlugin->TimeToPixel(animationLength);
        const double clipStartPixel = mPlugin->TimeToPixel(clipStart);
        const double clipEndPixel   = mPlugin->TimeToPixel(clipEnd);

        // enable anti aliassing
        //painter.setRenderHint(QPainter::Antialiasing);

        // fill with the background color
        QRectF clipStartRect    = rect;
        QRectF motionRect       = rect;
        QRectF clipEndRect      = rect;
        QRectF outOfRangeRect   = rect;

        clipEndRect.setRight(clipStartPixel);
        motionRect.setLeft(clipStartPixel);
        motionRect.setRight(clipEndPixel);
        clipEndRect.setLeft(clipEndPixel);
        clipEndRect.setRight(animEndPixel);
        outOfRangeRect.setLeft(animEndPixel);

        clipStartRect.setTop(mTimeLineHeight + 1);
        clipEndRect.setTop(mTimeLineHeight + 1);
        motionRect.setTop(mTimeLineHeight + 1);
        outOfRangeRect.setTop(mTimeLineHeight + 1);

        // render the rects
        painter.setPen(Qt::NoPen);
        painter.setBrush(mBrushBackgroundClipped);
        painter.drawRect(clipStartRect);
        painter.setBrush(mBrushBackground);
        painter.drawRect(motionRect);
        painter.setBrush(mBrushBackgroundClipped);
        painter.drawRect(clipEndRect);
        painter.setBrush(mBrushBackgroundOutOfRange);
        painter.drawRect(outOfRangeRect);

        // render the tracks
        RenderTracks(painter, rect.width(), rect.height() - mTimeLineHeight, animationLength, clipStart, clipEnd);
        /*
            // render selection rect
            if (mRectSelecting && mRectZooming == false)
            {
                painter.resetTransform();
                QRect selectRect;
                CalcSelectRect( selectRect );
                painter.setBrush( QColor(200, 120, 0, 75) );
                painter.setPen( QColor(255,128,0) );
                painter.drawRect( selectRect );
            }
        */
    }

    // render all tracks
    void TrackDataWidget::RenderTracks(QPainter& painter, uint32 width, uint32 height, double animationLength, double clipStartTime, double clipEndTime)
    {
        //MCore::Timer renderTimer;
        int32 yOffset = mTimeLineHeight + 1;

        // calculate the start and end time range of the visible area
        double visibleStartTime, visibleEndTime;
        visibleStartTime = mPlugin->PixelToTime(0); //mPlugin->CalcTime( 0, &visibleStartTime, nullptr, nullptr, nullptr, nullptr );
        visibleEndTime = mPlugin->PixelToTime(width); //mPlugin->CalcTime( width, &visibleEndTime, nullptr, nullptr, nullptr, nullptr );

        // for all tracks
        const uint32 numTracks = mPlugin->mTracks.GetLength();
        for (uint32 i = 0; i < numTracks; ++i)
        {
            TimeTrack* track = mPlugin->mTracks[i];
            track->SetStartY(yOffset);

            // path for making the cut elements a bit transparent
            if (mCutMode)
            {
                // disable cut mode for all elements on default
                const uint32 numElements = track->GetNumElements();
                for (uint32 e = 0; e < numElements; ++e)
                {
                    track->GetElement(e)->SetIsCut(false);
                }

                // get the number of copy elements and check if ours is in
                const size_t numCopyElements = mCopyElements.size();
                for (size_t c = 0; c < numCopyElements; ++c)
                {
                    // get the copy element and make sure we're in the right track
                    const CopyElement& copyElement = mCopyElements[c];
                    if (copyElement.mTrackName != track->GetName())
                    {
                        continue;
                    }

                    // set the cut mode of the elements
                    for (uint32 e = 0; e < numElements; ++e)
                    {
                        TimeTrackElement* element = track->GetElement(e);
                        if (MCore::Compare<float>::CheckIfIsClose(element->GetStartTime(), copyElement.mStartTime, MCore::Math::epsilon) &&
                            MCore::Compare<float>::CheckIfIsClose(element->GetEndTime(), copyElement.mEndTime, MCore::Math::epsilon))
                        {
                            element->SetIsCut(true);
                        }
                    }
                }
            }

            // render the track
            track->RenderData(painter, width, yOffset, visibleStartTime, visibleEndTime, animationLength, clipStartTime, clipEndTime);

            // increase the offsets
            yOffset += track->GetHeight();
            yOffset += 1;
        }

        // render the element time handles
        mPlugin->RenderElementTimeHandles(painter, height, mPlugin->mPenTimeHandles);
    }


    // show the time of the currently dragging element in the time info view
    void TrackDataWidget::ShowElementTimeInfo(TimeTrackElement* element)
    {
        if (mPlugin->GetTimeInfoWidget() == nullptr)
        {
            return;
        }

        // enable overwrite mode so that the time info widget will show the custom time rather than the current time of the plugin
        mPlugin->GetTimeInfoWidget()->SetIsOverwriteMode(true);

        // calculate the dimensions
        int32 startX, startY, width, height;
        element->CalcDimensions(&startX, &startY, &width, &height);

        // show the times of the element
        mPlugin->GetTimeInfoWidget()->SetOverwriteTime(mPlugin->PixelToTime(startX), mPlugin->PixelToTime(startX + width));
    }


    void TrackDataWidget::mouseDoubleClickEvent(QMouseEvent* event)
    {
        if (event->button() != Qt::LeftButton)
        {
            return;
        }

        // if double clicked in the timeline
        if (GetIsInsideTimeLine(event->y()))
        {
            mPlugin->MakeTimeVisible(mPlugin->PixelToTime(event->x()), 0.5);
            return;
        }

        // if we clicked inside the node history area
        if (GetIsInsideNodeHistory(event->y()) && mPlugin->mTrackHeaderWidget->mNodeActivityCheckBox->isChecked())
        {
            EMotionFX::Recorder::ActorInstanceData* actorInstanceData = FindActorInstanceData();
            EMotionFX::Recorder::NodeHistoryItem* historyItem = FindNodeHistoryItem(actorInstanceData, event->x(), event->y());
            if (historyItem)
            {
                emit mPlugin->DoubleClickedRecorderNodeHistoryItem(actorInstanceData, historyItem);
            }
        }
        else
        {
            /*      EMotionFX::Recorder::ActorInstanceData* actorInstanceData = FindActorInstanceData();
                    EMotionFX::Recorder::EventHistoryItem* historyItem = FindEventHistoryItem(actorInstanceData, event->x(), event->y());
                    if (historyItem)
                        emit mPlugin->DoubleClickedRecorderNodeHistoryItem( actorInstanceData, historyItem );*/
        }
    }


    // when the mouse is moving, while a button is pressed
    void TrackDataWidget::mouseMoveEvent(QMouseEvent* event)
    {
        mPlugin->SetRedrawFlag();

        QPoint mousePos = event->pos();

        const int32 deltaRelX = event->x() - mLastMouseX;
        mLastMouseX = event->x();
        mPlugin->mCurMouseX = event->x();
        mPlugin->mCurMouseY = event->y();

        const int32 deltaRelY = event->y() - mLastMouseY;
        mLastMouseY = event->y();

        //if (mPlugin->GetTimeInfoWidget())
        //      mPlugin->GetTimeInfoWidget()->SetOverwriteTime( mPlugin->PixelToTime(event->x()), mPlugin->PixelToTime(event->x()) );

        const bool altPressed = event->modifiers() & Qt::AltModifier;
        const bool isZooming = mMouseLeftClicked == false && mMouseRightClicked && altPressed;
        const bool isPanning = mMouseLeftClicked == false && isZooming == false && (mMouseMidClicked || mMouseRightClicked);

        if (deltaRelY != 0)
        {
            mAllowContextMenu = false;
        }

        // get the track over which the cursor is positioned
        TimeTrack* mouseCursorTrack = mPlugin->GetTrackAt(event->y());

        if (mMouseRightClicked)
        {
            mIsScrolling = true;
        }

        // if the mouse left button is pressed
        if (mMouseLeftClicked)
        {
            if (altPressed)
            {
                mRectZooming = true;
            }
            else
            {
                mRectZooming = false;
            }

            // rect selection: update mouse position
            if (mRectSelecting)
            {
                mSelectEnd = mousePos;
            }

            if (mDraggingElement == nullptr && mResizeElement == nullptr && mRectSelecting == false)
            {
                // update the current time marker
                int newX = event->x();
                newX = MCore::Clamp<int>(newX, 0, geometry().width() - 1);
                mPlugin->mCurTime = mPlugin->PixelToTime(newX);

                EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();
                if (recorder.GetRecordTime() > MCore::Math::epsilon)
                {
                    if (recorder.GetIsInPlayMode())
                    {
                        recorder.SetCurrentPlayTime(mPlugin->GetCurrentTime());
                        recorder.SetAutoPlay(false);
                        emit mPlugin->ManualTimeChange(mPlugin->GetCurrentTime());
                    }
                }
                else
                {
                    const AZStd::vector<EMotionFX::MotionInstance*>& motionInstances = MotionWindowPlugin::GetSelectedMotionInstances();
                    if (motionInstances.size() == 1)
                    {
                        EMotionFX::MotionInstance* motionInstance = motionInstances[0];
                        motionInstance->SetCurrentTime(mPlugin->GetCurrentTime(), false);
                        emit mPlugin->ManualTimeChange(mPlugin->GetCurrentTime());
                    }
                }

                mIsScrolling = true;
            }

            TimeTrack* dragElementTrack = nullptr;
            if (mDraggingElement)
            {
                dragElementTrack = mDraggingElement->GetTrack();
            }

            // calculate the delta movement
            const int32 deltaX = event->x() - mLastLeftClickedX;
            const int32 movement = abs(deltaX);
            const bool elementTrackChanged = (mouseCursorTrack && dragElementTrack && mouseCursorTrack != dragElementTrack);
            if ((movement > 1 && !mDragging) || elementTrackChanged)
            {
                mDragging = true;
            }

            // handle resizing
            if (mResizing)
            {
                if (mPlugin->FindTrackByElement(mResizeElement) == nullptr)
                {
                    mResizeElement = nullptr;
                }

                if (mResizeElement)
                {
                    TimeTrack* resizeElementTrack = mResizeElement->GetTrack();

                    // only allow resizing on enabled time tracks
                    if (resizeElementTrack->GetIsEnabled())
                    {
                        mResizeElement->SetShowTimeHandles(true);
                        mResizeElement->SetShowToolTip(false);

                        double resizeTime = (deltaRelX / mPlugin->mTimeScale) / mPlugin->mPixelsPerSecond;
                        mResizeID = mResizeElement->HandleResize(mResizeID, resizeTime, 0.02 / mPlugin->mTimeScale);

                        // show the time of the currently resizing element in the time info view
                        ShowElementTimeInfo(mResizeElement);

                        setCursor(Qt::SizeHorCursor);
                    }

                    return;
                }
            }

            // if we are not dragging or no element is being dragged, there is nothing to do
            if (mDragging == false || mDraggingElement == nullptr)
            {
                return;
            }

            // check if the mouse cursor is over another time track than the dragging element
            if (elementTrackChanged)
            {
                // if yes we need to remove the dragging element from the old time track
                dragElementTrack->RemoveElement(mDraggingElement, false);

                // and add it to the new time track where the cursor now is over
                mouseCursorTrack->AddElement(mDraggingElement);
                mDraggingElement->SetTrack(mouseCursorTrack);
            }

            // show the time of the currently dragging element in the time info view
            ShowElementTimeInfo(mDraggingElement);

            // adjust the cursor
            setCursor(Qt::ClosedHandCursor);
            mDraggingElement->SetShowToolTip(false);

            // show the time handles
            mDraggingElement->SetShowTimeHandles(true);

            const double snapThreshold = 0.02 / mPlugin->mTimeScale;

            // calculate how many pixels we moved with the mouse
            const int32 deltaMovement = event->x() - mLastMouseMoveX;
            mLastMouseMoveX = event->x();

            // snap the moved amount to a given time value
            double snappedTime = mDraggingElement->GetStartTime() + ((deltaMovement / mPlugin->mPixelsPerSecond) / mPlugin->mTimeScale);

            bool startSnapped = false;
            if (abs(deltaMovement) < 2 && abs(deltaMovement) > 0) // only snap when moving the mouse very slowly
            {
                startSnapped = mPlugin->SnapTime(&snappedTime, mDraggingElement, snapThreshold);
            }

            // in case the start time didn't snap to anything
            if (startSnapped == false)
            {
                // try to snap the end time
                double snappedEndTime = mDraggingElement->GetEndTime() + ((deltaMovement / mPlugin->mPixelsPerSecond) / mPlugin->mTimeScale);
                /*bool endSnapped = */ mPlugin->SnapTime(&snappedEndTime, mDraggingElement, snapThreshold);

                // apply the delta movement
                const double deltaTime = snappedEndTime - mDraggingElement->GetEndTime();
                mDraggingElement->MoveRelative(deltaTime);
            }
            else
            {
                // apply the snapped delta movement
                const double deltaTime = snappedTime - mDraggingElement->GetStartTime();
                mDraggingElement->MoveRelative(deltaTime);
            }
        }
        else if (isPanning)
        {
            if (EMotionFX::GetRecorder().GetIsRecording() == false)
            {
                mPlugin->DeltaScrollX(-deltaRelX, false);
                //mPlugin->DeltaScrollY( -deltaRelY );
            }
        }
        else if (isZooming)
        {
            if (deltaRelY < 0)
            {
                setCursor(*(mPlugin->GetZoomOutCursor()));
            }
            else
            {
                setCursor(*(mPlugin->GetZoomInCursor()));
            }

            DoMouseYMoveZoom(deltaRelY, mPlugin);
        }
        else // no left mouse button is pressed
        {
            UpdateMouseOverCursor(event->x(), event->y());
        }
    }


    void TrackDataWidget::DoMouseYMoveZoom(int32 deltaY, TimeViewPlugin* plugin)
    {
        // keep the scaling speed in range so that we're not scaling insanely fast
        float movement = MCore::Min<float>(deltaY, 9.0f);
        movement = MCore::Max<float>(movement, -9.0f);

        // scale relatively to the current scaling value, meaning when the range is bigger we scale faster than when viewing only a very small time range
        float timeScale = plugin->GetTimeScale();
        timeScale *= 1.0f - 0.01 * movement;

        // set the new scaling value
        plugin->SetScale(timeScale);
    }


    // update the mouse-over cursor, depending on its location
    void TrackDataWidget::UpdateMouseOverCursor(int32 x, int32 y)
    {
        // disable all tooltips
        mPlugin->DisableAllToolTips();

        // get the time track and return directly if we are not over a valid track with the cursor
        TimeTrack* timeTrack = mPlugin->GetTrackAt(y);
        if (timeTrack == nullptr)
        {
            setCursor(Qt::ArrowCursor);
            return;
        }

        // get the element over which the cursor is positioned
        TimeTrackElement* element = mPlugin->GetElementAt(x, y);

        // in case the cursor is over an element, show tool tips
        if (element)
        {
            element->SetShowToolTip(true);
        }
        else
        {
            mPlugin->DisableAllToolTips();
        }

        // do not allow any editing in case the track is not enabled
        if (timeTrack->GetIsEnabled() == false)
        {
            setCursor(Qt::ArrowCursor);
            return;
        }

        // check if we are hovering over a resize point
        if (mPlugin->FindResizePoint(x, y, &mResizeElement, &mResizeID))
        {
            setCursor(Qt::SizeHorCursor);
            mResizeElement->SetShowToolTip(true);
        }
        else // if we're not above a resize point
        {
            if (element)
            {
                setCursor(Qt::OpenHandCursor);
            }
            else
            {
                setCursor(Qt::ArrowCursor);
            }
        }
    }


    // when the mouse is pressed
    void TrackDataWidget::mousePressEvent(QMouseEvent* event)
    {
        mPlugin->SetRedrawFlag();
        //mOldCurrentTime = mPlugin->GetCurrentTime();

        QPoint mousePos = event->pos();

        const bool ctrlPressed = event->modifiers() & Qt::ControlModifier;
        const bool shiftPressed = event->modifiers() & Qt::ShiftModifier;
        const bool altPressed = event->modifiers() & Qt::AltModifier;

        // store the last clicked position
        mLastMouseMoveX     = event->x();
        mLastMouseMoveY     = event->y();
        mAllowContextMenu   = true;
        mRectSelecting      = false;

        //if (mPlugin->GetTimeInfoWidget())
        //mPlugin->GetTimeInfoWidget()->SetOverwriteTime( mPlugin->PixelToTime(event->x()), mPlugin->PixelToTime(event->x()) );

        if (event->button() == Qt::RightButton)
        {
            mMouseRightClicked = true;
        }

        if (event->button() == Qt::MidButton)
        {
            mMouseMidClicked = true;
        }

        if (event->button() == Qt::LeftButton)
        {
            mMouseLeftClicked   = true;

            EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();
            if ((GetIsInsideTimeLine(event->y()) || (mPlugin->mNodeHistoryItem == nullptr)) && altPressed == false && (recorder.GetRecordTime() >= MCore::Math::epsilon))
            {
                // update the current time marker
                int newX = event->x();
                newX = MCore::Clamp<int>(newX, 0, geometry().width() - 1);
                mPlugin->mCurTime = mPlugin->PixelToTime(newX);

                if (recorder.GetRecordTime() < MCore::Math::epsilon)
                {
                    const AZStd::vector<EMotionFX::MotionInstance*>& motionInstances = MotionWindowPlugin::GetSelectedMotionInstances();
                    if (motionInstances.size() == 1)
                    {
                        EMotionFX::MotionInstance* motionInstance = motionInstances[0];
                        mMotionPaused = motionInstance->GetIsPaused();
                        motionInstance->SetPause(true);
                        motionInstance->SetCurrentTime(mPlugin->GetCurrentTime());
                    }
                }
                else
                {
                    if (recorder.GetIsInPlayMode() == false)
                    {
                        recorder.StartPlayBack();
                    }

                    recorder.SetCurrentPlayTime(mPlugin->GetCurrentTime());
                    recorder.SetAutoPlay(false);
                }

                emit mPlugin->ManualTimeChangeStart(mPlugin->GetCurrentTime());
                emit mPlugin->ManualTimeChange(mPlugin->GetCurrentTime());
            }
            else // not inside timeline
            {
                // if we clicked inside the node history area
                if (GetIsInsideNodeHistory(event->y()) && mPlugin->mTrackHeaderWidget->mNodeActivityCheckBox->isChecked())
                {
                    EMotionFX::Recorder::ActorInstanceData* actorInstanceData = FindActorInstanceData();
                    EMotionFX::Recorder::NodeHistoryItem* historyItem = FindNodeHistoryItem(actorInstanceData, event->x(), event->y());
                    if (historyItem && altPressed == false)
                    {
                        emit mPlugin->ClickedRecorderNodeHistoryItem(actorInstanceData, historyItem);
                    }
                }
                {
                    // unselect all elements
                    if (ctrlPressed == false && shiftPressed == false)
                    {
                        mPlugin->UnselectAllElements();
                    }

                    // find the element we're clicking in
                    TimeTrackElement* element = mPlugin->GetElementAt(event->x(), event->y());
                    if (element)
                    {
                        // show the time of the currently dragging element in the time info view
                        ShowElementTimeInfo(element);

                        TimeTrack* timeTrack = element->GetTrack();

                        if (timeTrack->GetIsEnabled())
                        {
                            mDraggingElement    = element;
                            mDragElementTrack   = timeTrack;
                            mDraggingElement->SetShowTimeHandles(true);
                            setCursor(Qt::ClosedHandCursor);
                        }
                        else
                        {
                            mDraggingElement    = nullptr;
                            mDragElementTrack   = nullptr;
                        }

                        // shift select
                        if (shiftPressed)
                        {
                            // get the element number of the clicked element
                            const uint32 clickedElementNr = element->GetElementNumber();

                            // get the element number of the first previously selected element
                            TimeTrackElement* firstSelectedElement = timeTrack->GetFirstSelectedElement();
                            const uint32 firstSelectedNr = firstSelectedElement ? firstSelectedElement->GetElementNumber() : 0;

                            // range select
                            timeTrack->RangeSelectElements(firstSelectedNr, clickedElementNr);
                        }
                        else
                        {
                            // normal select
                            element->SetIsSelected(!element->GetIsSelected());
                        }

                        element->SetShowToolTip(false);

                        emit SelectionChanged();
                    }
                    else // no element clicked
                    {
                        mDraggingElement    = nullptr;
                        mDragElementTrack   = nullptr;

                        // rect selection
                        mRectSelecting  = true;
                        mSelectStart    = mousePos;
                        mSelectEnd      = mSelectStart;
                        setCursor(Qt::ArrowCursor);

                        if (GetIsInsideTimeLine(event->y()))
                        {
                            const AZStd::vector<EMotionFX::MotionInstance*>& motionInstances = MotionWindowPlugin::GetSelectedMotionInstances();
                            if (motionInstances.size() == 1)
                            {
                                mRectSelecting = false;
                                EMotionFX::MotionInstance* motionInstance = motionInstances[0];
                                mMotionPaused = motionInstance->GetIsPaused();
                                motionInstance->Pause();
                                const float newTime =  mPlugin->PixelToTime(event->x());
                                recorder.SetCurrentPlayTime(newTime);
                                mPlugin->mCurTime = newTime;
                                motionInstance->SetCurrentTime(newTime);
                            }
                        }
                    }

                    // if we're going to resize
                    if (mResizeElement && mResizeID != MCORE_INVALIDINDEX32)
                    {
                        mResizing = true;
                    }
                    else
                    {
                        mResizing = false;
                    }

                    // store the last clicked position
                    mDragging           = false;
                    mMouseLeftClicked   = true;
                    mLastLeftClickedX   = event->x();
                    /*
                            if (mDraggingElement == nullptr && mResizeElement == nullptr)
                            {
                                int32 newX = event->x();
                                newX = MCore::Clamp<int32>(newX, 0, geometry().width() - 1);
                                mPlugin->mCurTimeX = newX;
                            }*/

                    // render the viewports again
                    //mPlugin->UpdateVisualData();
                    /*
                                    MCore::Array<EMotionFX::MotionInstance*>& motionInstances = MotionWindowPlugin::GetSelectedMotionInstances();
                                    if (motionInstances.GetLength() == 1)
                                    {
                                        EMotionFX::MotionInstance* motionInstance = motionInstances[0];
                                        mMotionPaused = motionInstance->IsPaused();
                                        motionInstance->SetPause(true);
                                        motionInstance->SetCurrentTime(mPlugin->GetCurrentTime());
                                    }*/
                }
            }
        }
        else
        {
            mDragging = false;
        }

        //const bool altPressed = event->modifiers() & Qt::AltModifier;
        const bool isZooming = mMouseLeftClicked == false && mMouseRightClicked && altPressed;
        const bool isPanning = mMouseLeftClicked == false && isZooming == false && (mMouseMidClicked || mMouseRightClicked);

        if (isPanning)
        {
            setCursor(Qt::SizeHorCursor);
        }

        if (isZooming)
        {
            setCursor(*(mPlugin->GetZoomInCursor()));
        }
    }


    // when releasing the mouse button
    void TrackDataWidget::mouseReleaseEvent(QMouseEvent* event)
    {
        mPlugin->SetRedrawFlag();

        setCursor(Qt::ArrowCursor);

        // disable overwrite mode in any case when the mouse gets released so that we display the current time from the plugin again
        if (mPlugin->GetTimeInfoWidget())
        {
            mPlugin->GetTimeInfoWidget()->SetIsOverwriteMode(false);
            //mPlugin->GetTimeInfoWidget()->SetOverwriteTime( mPlugin->PixelToTime(event->x()), mPlugin->PixelToTime(event->x()) );
        }

        mLastMouseMoveX = event->x();
        mLastMouseMoveY = event->y();

        const bool ctrlPressed = event->modifiers() & Qt::ControlModifier;
        //const bool shiftPressed = event->modifiers() & Qt::ShiftModifier;

        if (event->button() == Qt::RightButton)
        {
            mMouseRightClicked = false;
            mIsScrolling = false;
        }

        if (event->button() == Qt::MidButton)
        {
            mMouseMidClicked = false;
        }

        if (event->button() == Qt::LeftButton)
        {
            TimeTrack* mouseCursorTrack = mPlugin->GetTrackAt(event->y());
            const bool elementTrackChanged = (mouseCursorTrack && mDragElementTrack && mouseCursorTrack != mDragElementTrack);

            if ((mResizing || mDragging) && elementTrackChanged == false && mDraggingElement)
            {
                emit MotionEventChanged(mDraggingElement, mDraggingElement->GetStartTime(), mDraggingElement->GetEndTime());
            }

            mMouseLeftClicked   = false;
            mDragging           = false;
            mResizing           = false;
            mIsScrolling        = false;

            // rect selection
            if (mRectSelecting)
            {
                if (mRectZooming)
                {
                    mRectZooming = false;

                    // calc the selection rect
                    QRect selectRect;
                    CalcSelectRect(selectRect);

                    // zoom in on the rect
                    if (selectRect.isEmpty() == false)
                    {
                        mPlugin->ZoomRect(selectRect);
                    }
                }
                else
                {
                    // calc the selection rect
                    QRect selectRect;
                    CalcSelectRect(selectRect);

                    // select things inside it
                    if (selectRect.isEmpty() == false)
                    {
                        //selectRect = mActiveGraph->GetTransform().inverted().mapRect( selectRect );

                        // rect select the elements
                        const bool overwriteSelection = (ctrlPressed == false);
                        SelectElementsInRect(selectRect, overwriteSelection, true, ctrlPressed);
                    }
                }
            }

            // check if we moved an element to another track
            if (elementTrackChanged && mDraggingElement)
            {
                // lastly fire a signal so that the data can change along with
                emit ElementTrackChanged(mDraggingElement->GetElementNumber(), mDraggingElement->GetStartTime(), mDraggingElement->GetEndTime(), mDragElementTrack->GetName(), mouseCursorTrack->GetName());
            }
            mDragElementTrack = nullptr;

            if (mDraggingElement)
            {
                mDraggingElement->SetShowTimeHandles(false);
                mDraggingElement = nullptr;
            }
            /*
                    MCore::Array<EMotionFX::MotionInstance*>& motionInstances = MotionWindowPlugin::GetSelectedMotionInstances();
                    if (motionInstances.GetLength() == 1)
                    {
                        EMotionFX::MotionInstance* motionInstance = motionInstances[0];
                        motionInstance->SetPause(mMotionPaused);
                    }*/

            // disable rect selection mode again
            mRectSelecting  = false;
            //      update();
            return;
        }
        else
        {
            mResizing = false;
            mDragging = false;
        }

        //mPlugin->SetCurrentTime( mOldCurrentTime );

        // disable rect selection mode again
        mRectSelecting  = false;

        UpdateMouseOverCursor(event->x(), event->y());
        //  update();
    }


    // the mouse wheel is adjusted
    void TrackDataWidget::DoWheelEvent(QWheelEvent* event, TimeViewPlugin* plugin)
    {
        plugin->SetRedrawFlag();

        const int numDegrees    = event->delta() / 8;
        const int numSteps      = numDegrees / 15;
        float delta             = numSteps / 10.0f;
        //double multiplier     = 1.0 - delta;

        double zoomDelta = delta * 4 * MCore::Clamp(plugin->GetTimeScale() / 2.0, 1.0, 22.0);
        if (event->orientation() == Qt::Vertical)
        {
            plugin->SetScale(plugin->GetTimeScale() + zoomDelta);
        }

        if (event->orientation() == Qt::Horizontal)
        {
            if (EMotionFX::GetRecorder().GetIsRecording() == false)
            {
                if (delta > 0)
                {
                    delta = 1;
                }
                else
                {
                    delta = -1;
                }

                plugin->DeltaScrollX(-delta * 600);
            }
        }
    }


    // handle mouse wheel event
    void TrackDataWidget::wheelEvent(QWheelEvent* event)
    {
        DoWheelEvent(event, mPlugin);
    }


    // drag & drop support
    void TrackDataWidget::dragEnterEvent(QDragEnterEvent* event)
    {
        mPlugin->SetRedrawFlag();
        mOldCurrentTime = mPlugin->GetCurrentTime();

        // this is needed to actually reach the drop event function
        event->acceptProposedAction();
    }


    void TrackDataWidget::dragMoveEvent(QDragMoveEvent* event)
    {
        mPlugin->SetRedrawFlag();
        QPoint mousePos = event->pos();

        double dropTime = mPlugin->PixelToTime(mousePos.x());
        mPlugin->SetCurrentTime(dropTime);

        const AZStd::vector<EMotionFX::MotionInstance*>& motionInstances = MotionWindowPlugin::GetSelectedMotionInstances();
        if (motionInstances.size() == 1)
        {
            EMotionFX::MotionInstance* motionInstance = motionInstances[0];
            motionInstance->SetCurrentTime(dropTime, false);
            motionInstance->Pause();
        }

        //event->accept();
    }


    void TrackDataWidget::dropEvent(QDropEvent* event)
    {
        mPlugin->SetRedrawFlag();
        // accept the drop
        event->acceptProposedAction();

        // emit drop event
        emit MotionEventPresetsDropped(event->pos());

        mPlugin->SetCurrentTime(mOldCurrentTime);
    }


    // the context menu event
    void TrackDataWidget::contextMenuEvent(QContextMenuEvent* event)
    {
        mPlugin->SetRedrawFlag();
        if (mAllowContextMenu == false)
        {
            return;
        }

        if (EMotionFX::GetRecorder().GetRecordTime() > MCore::Math::epsilon)
        {
            DoRecorderContextMenuEvent(event);
            return;
        }

        if (mPlugin->mMotion == nullptr)
        {
            return;
        }

        QPoint point = event->pos();
        mContextMenuX = point.x();
        mContextMenuY = point.y();

        TimeTrack* timeTrack = mPlugin->GetTrackAt(mContextMenuY);

        uint32 numElements          = 0;
        uint32 numSelectedElements  = 0;

        // calculate the number of selected and total events
        const uint32 numTracks = mPlugin->GetNumTracks();
        for (uint32 i = 0; i < numTracks; ++i)
        {
            // get the current time view track
            TimeTrack* track = mPlugin->GetTrack(i);
            if (track->GetIsVisible() == false)
            {
                continue;
            }

            // get the number of elements in the track and iterate through them
            const uint32 numTrackElements = track->GetNumElements();
            for (uint32 j = 0; j < numTrackElements; ++j)
            {
                TimeTrackElement* element = track->GetElement(j);
                numElements++;

                if (element->GetIsSelected())
                {
                    numSelectedElements++;
                }
            }
        }

        if (timeTrack)
        {
            numElements = timeTrack->GetNumElements();
            for (uint32 i = 0; i < numElements; ++i)
            {
                TimeTrackElement* element = timeTrack->GetElement(i);

                // increase the counter in case the element is selected
                if (element->GetIsSelected())
                {
                    numSelectedElements++;
                }
            }
        }

        // create the context menu
        QMenu menu(this);

        if (timeTrack)
        {
            TimeTrackElement* element = mPlugin->GetElementAt(mContextMenuX, mContextMenuY);
            if (element == nullptr)
            {
                QAction* action = menu.addAction("Add Motion Event");
                action->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Plus.png"));
                connect(action, SIGNAL(triggered()), this, SLOT(OnAddElement()));

                // add action to add a motion event which gets its param and type from the selected preset
                EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionEventsPlugin::CLASS_ID);
                if (plugin)
                {
                    MotionEventsPlugin* eventsPlugin = static_cast<MotionEventsPlugin*>(plugin);
                    if (eventsPlugin->CheckIfIsPresetReadyToDrop())
                    {
                        QAction* presetAction = menu.addAction("Add Preset Event");
                        presetAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Plus.png"));
                        connect(presetAction, SIGNAL(triggered()), this, SLOT(OnCreatePresetEvent()));
                    }
                }

                if (timeTrack->GetNumElements() > 0)
                {
                    action = menu.addAction("Cut All Events In Track");
                    action->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Cut.png"));
                    connect(action, SIGNAL(triggered()), this, SLOT(OnCutTrack()));

                    action = menu.addAction("Copy All Events In Track");
                    action->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Copy.png"));
                    connect(action, SIGNAL(triggered()), this, SLOT(OnCopyTrack()));
                }

                if (GetIsReadyForPaste())
                {
                    action = menu.addAction("Paste");
                    action->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Paste.png"));
                    connect(action, SIGNAL(triggered()), this, SLOT(OnPaste()));

                    action = menu.addAction("Paste At Location");
                    action->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Paste.png"));
                    connect(action, SIGNAL(triggered()), this, SLOT(OnPasteAtLocation()));
                }
            }
            else if (element->GetIsSelected())
            {
                QAction* action = menu.addAction("Cut");
                action->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Cut.png"));
                connect(action, SIGNAL(triggered()), this, SLOT(OnCutElement()));

                action = menu.addAction("Copy");
                action->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Copy.png"));
                connect(action, SIGNAL(triggered()), this, SLOT(OnCopyElement()));
            }

            // menu entry for removing selected elements in track
            /*if (timeTrack->CalcNumSelectedElements() > 0)
            {
                // construct the action name
                MCore::AnsiString actionName = "Remove Selected In Track";

                // add the action
                QAction* action = menu.addAction( actionName.AsChar() );
                action->setIcon( MysticQt::GetMysticQt()->FindIcon("Images/Icons/Minus.png") );
                connect(action, SIGNAL(triggered()), this, SLOT(RemoveSelectedMotionEventsInTrack()));
            }*/
        }
        else
        {
            QAction* action = menu.addAction("Add Event Track");
            action->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Plus.png"));
            connect(action, SIGNAL(triggered()), this, SLOT(OnAddTrack()));
        }

        // menu entry for removing elements
        if (numSelectedElements > 0)
        {
            // construct the action name
            MCore::String actionName = "Remove Selected Event";
            if (numSelectedElements > 1)
            {
                actionName += "s";
            }

            // add the action
            QAction* action = menu.addAction(actionName.AsChar());
            action->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Minus.png"));
            connect(action, SIGNAL(triggered()), mPlugin, SLOT(RemoveSelectedMotionEvents()));
        }

        // menu entry for removing all elements
        if (timeTrack && timeTrack->GetNumElements() > 0)
        {
            // add the action
            QAction* action = menu.addAction("Clear Track");
            action->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Clear.png"));
            connect(action, SIGNAL(triggered()), this, SLOT(RemoveAllMotionEventsInTrack()));
        }

        // menu entry for removing all elements
        /*if (numElements > 0)
        {
            // add the action
            QAction* action = menu.addAction( "Remove All" );
            action->setIcon( MysticQt::GetMysticQt()->FindIcon("Images/Icons/Minus.png") );
            connect(action, SIGNAL(triggered()), mPlugin, SLOT(RemoveAllMotionEvents()));
        }*/

        // show the menu at the given position
        menu.exec(event->globalPos());
    }


    // propagate key events to the plugin and let it handle by a shared function
    void TrackDataWidget::keyPressEvent(QKeyEvent* event)
    {
        if (mPlugin)
        {
            mPlugin->OnKeyPressEvent(event);
        }
    }


    // propagate key events to the plugin and let it handle by a shared function
    void TrackDataWidget::keyReleaseEvent(QKeyEvent* event)
    {
        if (mPlugin)
        {
            mPlugin->OnKeyReleaseEvent(event);
        }
    }


    void TrackDataWidget::AddMotionEvent(int32 x, int32 y)
    {
        mPlugin->SetRedrawFlag();
        // calculate the start time for the motion event
        double dropTimeInSeconds = mPlugin->PixelToTime(x);
        //mPlugin->CalcTime( x, &dropTimeInSeconds, nullptr, nullptr, nullptr, nullptr );

        // get the time track on which we dropped the preset
        TimeTrack* timeTrack = mPlugin->GetTrackAt(y);
        if (timeTrack == nullptr)
        {
            return;
        }

        CommandSystem::CommandHelperAddMotionEvent(timeTrack->GetName(), dropTimeInSeconds, dropTimeInSeconds);
    }


    void TrackDataWidget::RemoveMotionEvent(int32 x, int32 y)
    {
        mPlugin->SetRedrawFlag();
        // get the time track on which we dropped the preset
        TimeTrack* timeTrack = mPlugin->GetTrackAt(y);
        if (timeTrack == nullptr)
        {
            return;
        }

        // get the time track on which we dropped the preset
        TimeTrackElement* element = mPlugin->GetElementAt(x, y);
        if (element == nullptr)
        {
            return;
        }

        CommandSystem::CommandHelperRemoveMotionEvent(timeTrack->GetName(), element->GetElementNumber());
    }


    // remove selected motion events in track
    void TrackDataWidget::RemoveSelectedMotionEventsInTrack()
    {
        mPlugin->SetRedrawFlag();
        // get the track where we are at the moment
        TimeTrack* timeTrack = mPlugin->GetTrackAt(mLastMouseY);
        if (timeTrack == nullptr)
        {
            return;
        }

        MCore::Array<uint32> eventNumbers;

        // calculate the number of selected events
        const uint32 numEvents = timeTrack->GetNumElements();
        for (uint32 i = 0; i < numEvents; ++i)
        {
            TimeTrackElement* element = timeTrack->GetElement(i);

            // increase the counter in case the element is selected
            if (element->GetIsSelected())
            {
                eventNumbers.Add(i);
            }
        }

        // remove the motion events
        CommandSystem::CommandHelperRemoveMotionEvents(timeTrack->GetName(), eventNumbers);

        mPlugin->UnselectAllElements();
    }


    // remove all motion events in track
    void TrackDataWidget::RemoveAllMotionEventsInTrack()
    {
        mPlugin->SetRedrawFlag();

        // get the track where we are at the moment
        TimeTrack* timeTrack = mPlugin->GetTrackAt(mLastMouseY);
        if (timeTrack == nullptr)
        {
            return;
        }

        MCore::Array<uint32> eventNumbers;

        // construct an array with the event numbers
        const uint32 numEvents = timeTrack->GetNumElements();
        for (uint32 i = 0; i < numEvents; ++i)
        {
            eventNumbers.Add(i);
        }

        // remove the motion events
        CommandSystem::CommandHelperRemoveMotionEvents(timeTrack->GetName(), eventNumbers);

        mPlugin->UnselectAllElements();
    }


    void TrackDataWidget::FillCopyElements(bool selectedItemsOnly)
    {
        // clear the array before feeding it
        mCopyElements.clear();

        // get the time track name
        TimeTrack* timeTrack = mPlugin->GetTrackAt(mContextMenuY);
        if (timeTrack == nullptr)
        {
            return;
        }
        MCore::String trackName = timeTrack->GetName();

        // check if the motion is valid and return failure in case it is not
        EMotionFX::Motion* motion = mPlugin->GetMotion();
        if (motion == nullptr)
        {
            return;
        }

        // get the motion event table and find the track on which we will work on
        EMotionFX::MotionEventTable* eventTable = motion->GetEventTable();
        EMotionFX::MotionEventTrack* eventTrack = eventTable->FindTrackByName(trackName.AsChar());
        if (eventTrack == nullptr)
        {
            return;
        }

        // iterate through the elements
        const uint32 numElements = timeTrack->GetNumElements();
        MCORE_ASSERT(numElements == eventTrack->GetNumEvents());
        for (uint32 i = 0; i < numElements; ++i)
        {
            // get the element and skip all unselected ones
            TimeTrackElement* element = timeTrack->GetElement(i);
            if (selectedItemsOnly && element->GetIsSelected() == false)
            {
                continue;
            }

            // get the motion event
            EMotionFX::MotionEvent& motionEvent = eventTrack->GetEvent(i);

            // create the copy paste element and add it to the array
            CopyElement copyElem;
            copyElem.mMotionID          = motion->GetID();
            copyElem.mTrackName         = eventTrack->GetName();
            copyElem.mEventType         = motionEvent.GetEventTypeString();
            copyElem.mEventParameters   = eventTrack->GetParameter(motionEvent.GetParameterIndex());
            copyElem.mStartTime         = motionEvent.GetStartTime();
            copyElem.mEndTime           = motionEvent.GetEndTime();
            mCopyElements.push_back(copyElem);
        }
    }


    // cut all events from a track
    void TrackDataWidget::OnCutTrack()
    {
        mPlugin->SetRedrawFlag();

        FillCopyElements(false);

        mCutMode            = true;
        mPasteAtMousePos    = false;
    }


    // copy all events from a track
    void TrackDataWidget::OnCopyTrack()
    {
        mPlugin->SetRedrawFlag();

        FillCopyElements(false);

        mCutMode            = false;
        mPasteAtMousePos    = false;
    }


    // cut motion event
    void TrackDataWidget::OnCutElement()
    {
        mPlugin->SetRedrawFlag();

        FillCopyElements(true);

        mCutMode            = true;
        mPasteAtMousePos    = false;
    }


    // copy motion event
    void TrackDataWidget::OnCopyElement()
    {
        mPlugin->SetRedrawFlag();

        FillCopyElements(true);

        mCutMode            = false;
        mPasteAtMousePos    = false;
    }


    // paste motion event at the context menu position
    void TrackDataWidget::OnPasteAtLocation()
    {
        DoPaste(true);
    }


    // paste motion events at their original positions
    void TrackDataWidget::OnPaste()
    {
        DoPaste(false);
    }


    // paste motion events
    void TrackDataWidget::DoPaste(bool useLocation)
    {
        mPlugin->SetRedrawFlag();

        // get the time track name where we are pasting
        TimeTrack* timeTrack = mPlugin->GetTrackAt(mContextMenuY);
        if (timeTrack == nullptr)
        {
            return;
        }
        MCore::String trackName = timeTrack->GetName();

        // get the number of elements to copy
        const size_t numElements = mCopyElements.size();

        // create the command group
        MCore::CommandGroup commandGroup("Paste motion events");

        // find the min and maximum time values of the events to paste
        float minTime =  FLT_MAX;
        float maxTime = -FLT_MAX;
        if (useLocation)
        {
            for (size_t i = 0; i < numElements; ++i)
            {
                const CopyElement& copyElement = mCopyElements[i];

                if (copyElement.mStartTime  < minTime)
                {
                    minTime = copyElement.mStartTime;
                }
                if (copyElement.mEndTime    < minTime)
                {
                    minTime = copyElement.mEndTime;
                }
                if (copyElement.mStartTime  > maxTime)
                {
                    maxTime = copyElement.mStartTime;
                }
                if (copyElement.mEndTime    > maxTime)
                {
                    maxTime = copyElement.mEndTime;
                }
            }
        }
        //const float timeRange = (maxTime-minTime);
        //const float midTime       = minTime + timeRange * 0.5f;

        if (mCutMode)
        {
            // iterate through the copy elements from back to front and delete the selected ones
            for (int32 i = static_cast<int32>(numElements) - 1; i >= 0; i--)
            {
                const CopyElement& copyElement = mCopyElements[i];

                // get the motion to which the original element belongs to
                EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(copyElement.mMotionID);
                if (motion == nullptr)
                {
                    continue;
                }

                // get the motion event table and track
                EMotionFX::MotionEventTable*    eventTable  = motion->GetEventTable();
                EMotionFX::MotionEventTrack*    eventTrack  = eventTable->FindTrackByName(copyElement.mTrackName.c_str());
                if (eventTrack == nullptr)
                {
                    continue;
                }

                // get the number of events and iterate through them
                uint32 eventNr = MCORE_INVALIDINDEX32;
                const uint32 numEvents = eventTrack->GetNumEvents();
                for (eventNr = 0; eventNr < numEvents; ++eventNr)
                {
                    EMotionFX::MotionEvent& motionEvent = eventTrack->GetEvent(eventNr);
                    if (MCore::Compare<float>::CheckIfIsClose(motionEvent.GetStartTime(), copyElement.mStartTime, MCore::Math::epsilon) &&
                        MCore::Compare<float>::CheckIfIsClose(motionEvent.GetEndTime(), copyElement.mEndTime, MCore::Math::epsilon) &&
                        copyElement.mEventParameters == motionEvent.GetParameterString(eventTrack).AsChar() &&
                        copyElement.mEventType == motionEvent.GetEventTypeString())
                    {
                        break;
                    }
                }

                // remove event
                if (eventNr != MCORE_INVALIDINDEX32)
                {
                    CommandSystem::CommandHelperRemoveMotionEvent(copyElement.mMotionID, copyElement.mTrackName.c_str(), eventNr, &commandGroup);
                }
            }
        }

        // iterate through the elements to copy and add the new motion events
        for (uint32 i = 0; i < numElements; ++i)
        {
            const CopyElement& copyElement = mCopyElements[i];

            float startTime = copyElement.mStartTime;
            float endTime   = copyElement.mEndTime;

            // calculate the duration of the motion event
            float duration  = 0.0f;
            if (MCore::Compare<float>::CheckIfIsClose(startTime, endTime, MCore::Math::epsilon) == false)
            {
                duration = endTime - startTime;
            }

            if (useLocation)
            {
                // calculate the time of where we pasted
                double pasteTimeInSecs = 0.0;
                pasteTimeInSecs = mPlugin->PixelToTime(mContextMenuX, true);

                startTime   = pasteTimeInSecs;
                endTime     = startTime + duration;
                //startTime = (startTime-midTime)   + pasteTimeInSecs;
                //endTime       = (endTime-midTime)     + pasteTimeInSecs;
            }

            CommandSystem::CommandHelperAddMotionEvent(trackName.AsChar(), startTime, endTime, copyElement.mEventType.c_str(), copyElement.mEventParameters.c_str(), &commandGroup);
        }

        // execute the group command
        MCore::String outResult;
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult) == false)
        {
            MCore::LogError(outResult.AsChar());
        }

        if (mCutMode)
        {
            mCopyElements.clear();
        }
    }


    void TrackDataWidget::OnCreatePresetEvent()
    {
        mPlugin->SetRedrawFlag();
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionEventsPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return;
        }

        MotionEventsPlugin* eventsPlugin = static_cast<MotionEventsPlugin*>(plugin);

        QPoint mousePos(mContextMenuX, mContextMenuY);
        eventsPlugin->OnEventPresetDropped(mousePos);
    }


    // select all elements within a given rect
    void TrackDataWidget::SelectElementsInRect(const QRect& rect, bool overwriteCurSelection, bool select, bool toggleMode)
    {
        // get the number of tracks and iterate through them
        const uint32 numTracks = mPlugin->GetNumTracks();
        for (uint32 i = 0; i < numTracks; ++i)
        {
            // get the current time track
            TimeTrack* track = mPlugin->GetTrack(i);
            if (track->GetIsVisible() == false)
            {
                continue;
            }

            // select all elements in rect for this track
            track->SelectElementsInRect(rect, overwriteCurSelection, select, toggleMode);
        }
    }


    bool TrackDataWidget::event(QEvent* event)
    {
        if (event->type() == QEvent::ToolTip)
        {
            QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);

            QPoint localPos     = helpEvent->pos();
            QPoint tooltipPos   = helpEvent->globalPos();

            // get the position
            if (localPos.y() < 0)
            {
                return QOpenGLWidget::event(event);
            }

            // if we have a recording
            if (EMotionFX::GetRecorder().GetRecordTime() > MCore::Math::epsilon)
            {
                EMotionFX::Recorder::NodeHistoryItem* motionItem = FindNodeHistoryItem(FindActorInstanceData(), localPos.x(), localPos.y());
                if (motionItem)
                {
                    AZStd::string toolTipString;
                    BuildToolTipString(motionItem, toolTipString);

                    QRect toolTipRect(tooltipPos.x() - 4, tooltipPos.y() - 4, 8, 8);
                    QToolTip::showText(tooltipPos, toolTipString.c_str(), this, toolTipRect);
                }
                else
                {
                    EMotionFX::Recorder::EventHistoryItem* eventItem = FindEventHistoryItem(FindActorInstanceData(), localPos.x(), localPos.y());
                    if (eventItem)
                    {
                        AZStd::string toolTipString;
                        BuildToolTipString(eventItem, toolTipString);

                        QRect toolTipRect(tooltipPos.x() - 4, tooltipPos.y() - 4, 8, 8);
                        QToolTip::showText(tooltipPos, toolTipString.c_str(), this, toolTipRect);
                    }
                }
            }
            else
            {
                // get the hovered element and track
                TimeTrackElement*   element = mPlugin->GetElementAt(localPos.x(), localPos.y());
                //TimeTrack*            track   = mPlugin->GetTrackAt( localPos.y() );
                if (element == nullptr /* && track == nullptr*/)
                {
                    return QOpenGLWidget::event(event);
                }

                QString toolTipString;
                if (element)
                {
                    toolTipString = element->GetToolTip();
                }
                /*else if (track)
                {
                    toolTipString.Format( "Events: %i", track->GetNumElements() );
                }*/

                QRect toolTipRect(tooltipPos.x() - 4, tooltipPos.y() - 4, 8, 8);
                QToolTip::showText(tooltipPos, toolTipString, this, toolTipRect);
            }
        }

        return QOpenGLWidget::event(event);
    }


    // draw the time line
    void TrackDataWidget::DrawTimeLine(QPainter& painter, const QRect& rect)
    {
        // get the time values in seconds
        double animationLength  = 0.0;
        double clipStart        = 0.0;
        double clipEnd          = 0.0;
        mPlugin->GetDataTimes(&animationLength, &clipStart, &clipEnd);

        // calculate the pixel offsets
        double animEndPixel     = mPlugin->TimeToPixel(animationLength);
        double clipStartPixel   = mPlugin->TimeToPixel(clipStart);
        double clipEndPixel     = mPlugin->TimeToPixel(clipEnd);

        // fill with the background color
        QRect clipStartRect     = rect;
        QRect motionRect        = rect;
        QRect clipEndRect       = rect;
        QRect outOfRangeRect    = rect;

        clipEndRect.setRight(clipStartPixel);
        motionRect.setLeft(clipStartPixel);
        motionRect.setRight(animEndPixel);
        clipEndRect.setLeft(animEndPixel);
        clipEndRect.setRight(clipEndPixel);
        outOfRangeRect.setLeft(animEndPixel);
        clipStartRect.setBottom(mTimeLineHeight);
        clipEndRect.setBottom(mTimeLineHeight);
        motionRect.setBottom(mTimeLineHeight);
        outOfRangeRect.setBottom(mTimeLineHeight);

        painter.setPen(Qt::NoPen);
        if (hasFocus() == false)
        {
            painter.setBrush(mHeaderGradientActive);
            painter.drawRect(motionRect);
            painter.setBrush(mHeaderGradientInactive);
            painter.drawRect(outOfRangeRect);
        }
        else
        {
            painter.setBrush(mHeaderGradientActiveFocus);
            painter.drawRect(motionRect);
            painter.setBrush(mHeaderGradientInactiveFocus);
            painter.drawRect(outOfRangeRect);
        }

        // show the recorder range
        EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();
        if (recorder.GetRecordTime() > MCore::Math::epsilon)
        {
            QRectF recorderRect = rect;
            recorderRect.setRight(mPlugin->TimeToPixel(recorder.GetRecordTime()));
            recorderRect.setTop(mTimeLineHeight - 2);
            recorderRect.setBottom(mTimeLineHeight + 1);

            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(175, 0, 0));
            painter.drawRect(recorderRect);
        }
        else
        {
            if (animationLength > MCore::Math::epsilon)
            {
                QRectF rangeRect = rect;
                rangeRect.setRight(mPlugin->TimeToPixel(animationLength));
                rangeRect.setTop(mTimeLineHeight - 2);
                rangeRect.setBottom(mTimeLineHeight + 1);

                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(0, 175, 0));
                painter.drawRect(rangeRect);
            }
        }

        QTextOption options;
        options.setAlignment(Qt::AlignCenter);
        painter.setFont(mTimeLineFont);

        const uint32 width = rect.width();
        //const uint32 height = rect.height();

        float yOffset = 19.0f;

        //const double pixelsPerSecond = mPlugin->mPixelsPerSecond;

        double timeOffset = mPlugin->PixelToTime(0.0) * 1000.0;
        timeOffset = (timeOffset - ((int32)timeOffset % 5000)) / 1000.0;

        //if (rand() % 10 == 0)
        //MCore::LogInfo("%f", mPlugin->mTimeScale);

        uint32 minutes, seconds, milSecs, frameNumber;
        double pixelTime;

        double curX = 0.0;
        double curTime = timeOffset;

        //uint32 index = 0;
        while (curX <= width)
        {
            curX = mPlugin->TimeToPixel(curTime, false);
            mPlugin->CalcTime(curX, &pixelTime, &minutes, &seconds, &milSecs, &frameNumber, false);
            seconds += minutes * 60;
            curX *= mPlugin->mTimeScale;

            curTime += 5.0;

            painter.setPen(mPenMainTimeStepLinesActive);
            painter.drawLine(QPointF(curX, yOffset - 3.0f), QPointF(curX, yOffset + 10.0f));

            mTimeString = AZStd::string::format("%.2d:%.2d", seconds, milSecs); // will only do an allocation once, reuses the memory

            //painter.setPen( mPenText );
            painter.setPen(QColor(175, 175, 175));
            painter.drawText(QRect(curX - 25, yOffset - 23, 52, 20), mTimeString.c_str(), options);
        }

        // draw the seconds
        curTime = timeOffset;
        if (mPlugin->mTimeScale >= 0.25)
        {
            uint32 index = 0;
            curX = 0.0;
            while (curX <= width)
            {
                curX = mPlugin->TimeToPixel(curTime, false);
                mPlugin->CalcTime(curX, &pixelTime, &minutes, &seconds, &milSecs, &frameNumber, false);
                seconds += minutes * 60;
                curX *= mPlugin->mTimeScale;

                curTime += 1.0;
                if (index % 5 == 0)
                {
                    index++;
                    continue;
                }
                index++;

                if (curX > -100 && curX < width + 100)
                {
                    painter.setPen(mPenMainTimeStepLinesActive);
                    if (mPlugin->mTimeScale < 0.9)
                    {
                        painter.drawLine(QPointF(curX, yOffset - 1.0f), QPointF(curX, yOffset + 5.0f));
                    }
                    else
                    {
                        painter.drawLine(QPointF(curX, yOffset - 3.0f), QPointF(curX, yOffset + 10.0f));
                    }

                    if (mPlugin->mTimeScale >= 0.48)
                    {
                        mTimeString = AZStd::string::format("%.2d:%.2d", seconds, milSecs); // will only do an allocation once, reuses the memory

                        float alpha = (mPlugin->mTimeScale - 0.48f) / 1.0f;
                        alpha *= 2;
                        if (alpha > 1.0f)
                        {
                            alpha = 1.0;
                        }

                        painter.setPen(QColor(200, 200, 200, alpha * 255));
                        painter.drawText(QRect(curX - 25, yOffset - 23, 52, 20), mTimeString.c_str(), options);
                    }
                }
            }
        }

        // 500 ms
        curTime = timeOffset;
        if (mPlugin->mTimeScale >= 0.1)
        {
            uint32 index = 0;
            curX = 0;
            while (curX <= width)
            {
                curX = mPlugin->TimeToPixel(curTime, false);
                mPlugin->CalcTime(curX, &pixelTime, &minutes, &seconds, &milSecs, &frameNumber, false);
                seconds += minutes * 60;
                curX *= mPlugin->mTimeScale;

                curTime += 0.5;
                if (index % 2 == 0)
                {
                    index++;
                    continue;
                }
                index++;

                if (curX > -100 && curX < width + 100)
                {
                    painter.setPen(mPenMainTimeStepLinesActive);
                    if (mPlugin->mTimeScale < 1.5)
                    {
                        if (mPlugin->mTimeScale < 1.0)
                        {
                            painter.drawLine(QPointF(curX, yOffset - 1.0f), QPointF(curX, yOffset + 1.0f));
                        }
                        else
                        {
                            painter.drawLine(QPointF(curX, yOffset - 1.0f), QPointF(curX, yOffset + 8.0f));
                        }
                    }
                    else
                    {
                        painter.drawLine(QPointF(curX, yOffset - 3.0f), QPointF(curX, yOffset + 10.0f));
                    }

                    if (mPlugin->mTimeScale >= 2.0f)
                    {
                        mTimeString = AZStd::string::format("%.2d:%.2d", seconds, milSecs); // will only do an allocation once, reuses the memory

                        float alpha = (mPlugin->mTimeScale - 2.0f) / 2.0f;
                        if (alpha > 1.0f)
                        {
                            alpha = 1.0;
                        }

                        //painter.setPen( mPenText );
                        painter.setPen(QColor(175, 175, 175, alpha * 255));
                        painter.drawText(QRect(curX - 25, yOffset - 23, 52, 20), mTimeString.c_str(), options);
                    }
                }
            }
        }

        // 100 ms
        curTime = timeOffset;
        if (mPlugin->mTimeScale >= 0.95f)
        {
            uint32 index = 0;
            curX = 0;
            while (curX <= width)
            {
                if (index == 11)
                {
                    index = 1;
                }

                curX = mPlugin->TimeToPixel(curTime, false);
                mPlugin->CalcTime(curX, &pixelTime, &minutes, &seconds, &milSecs, &frameNumber, false);
                seconds += minutes * 60;
                curX *= mPlugin->mTimeScale;

                curTime += 0.1;
                if (index == 0 || index == 5 || index == 10)
                {
                    index++;
                    continue;
                }

                index++;

                if (curX > -100 && curX < width + 100)
                {
                    painter.setPen(mPenMainTimeStepLinesActive);
                    painter.drawLine(QPointF(curX, yOffset), QPointF(curX, yOffset + 3.0f));

                    if (mPlugin->mTimeScale >= 11.0f)
                    {
                        float alpha = (mPlugin->mTimeScale - 11.0f) / 4.0f;
                        if (alpha > 1.0f)
                        {
                            alpha = 1.0;
                        }

                        mTimeString = AZStd::string::format("%.2d:%.2d", seconds, milSecs); // will only do an allocation once, reuses the memory
                        painter.setPen(QColor(110, 110, 110, alpha * 255));
                        painter.drawText(QRect(curX - 25, yOffset - 23, 52, 20), mTimeString.c_str(), options);
                    }
                }
            }
        }

        timeOffset = mPlugin->PixelToTime(0.0) * 1000.0;
        timeOffset = (timeOffset - ((int32)timeOffset % 1000)) / 1000.0;

        // 50 ms
        curTime = timeOffset;
        if (mPlugin->mTimeScale >= 1.9)
        {
            uint32 index = 0;
            curX = 0;
            while (curX <= width)
            {
                curX = mPlugin->TimeToPixel(curTime, false);
                mPlugin->CalcTime(curX, &pixelTime, &minutes, &seconds, &milSecs, &frameNumber, false);
                seconds += minutes * 60;
                curX *= mPlugin->mTimeScale;

                curTime += 0.05;
                if (index % 2 == 0)
                {
                    index++;
                    continue;
                }

                index++;

                if (curX > -100 && curX < width + 100)
                {
                    painter.setPen(mPenMainTimeStepLinesActive);
                    painter.drawLine(QPointF(curX, yOffset), QPointF(curX, yOffset + 1.0f));

                    if (mPlugin->mTimeScale >= 25.0f)
                    {
                        float alpha = (mPlugin->mTimeScale - 25.0f) / 6.0f;
                        if (alpha > 1.0f)
                        {
                            alpha = 1.0;
                        }

                        mTimeString = AZStd::string::format("%.2d:%.2d", seconds, milSecs); // will only do an allocation once, reuses the memory
                        //painter.setPen( mPenText );
                        painter.setPen(QColor(80, 80, 80, alpha * 255));
                        painter.drawText(QRect(curX - 25, yOffset - 23, 52, 20), mTimeString.c_str(), options);
                    }
                }
            }
        }


        // 10 ms
        curTime = timeOffset;
        if (mPlugin->mTimeScale >= 7.9)
        {
            uint32 index = 0;
            curX = 0;
            while (curX <= width)
            {
                curX = mPlugin->TimeToPixel(curTime, false);
                mPlugin->CalcTime(curX, &pixelTime, &minutes, &seconds, &milSecs, &frameNumber, false);
                curX *= mPlugin->mTimeScale;

                curTime += 0.01;
                if (index % 5 == 0)
                {
                    index++;
                    continue;
                }

                index++;

                if (curX > -100 && curX < width + 100)
                {
                    //MCore::LogInfo("%f", curX);
                    painter.setPen(mPenMainTimeStepLinesActive);
                    painter.drawLine(QPointF(curX, yOffset), QPointF(curX, yOffset + 1.0f));

                    if (mPlugin->mTimeScale >= 65.0)
                    {
                        float alpha = (mPlugin->mTimeScale - 65.0f) / 5.0f;
                        if (alpha > 1.0f)
                        {
                            alpha = 1.0;
                        }

                        mTimeString = AZStd::string::format("%.2d:%.2d", seconds, milSecs); // will only do an allocation once, reuses the memory
                        //painter.setPen( mPenText );
                        painter.setPen(QColor(60, 60, 60, alpha * 255));
                        painter.drawText(QRect(curX - 25, yOffset - 23, 52, 20), mTimeString.c_str(), options);
                    }
                }
            }
        }
    }


    // update the rects
    void TrackDataWidget::UpdateRects()
    {
        EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();

        // get the actor instance data for the first selected actor instance, and render the node history for that
        const EMotionFX::Recorder::ActorInstanceData* actorInstanceData = FindActorInstanceData();

        // if we recorded node history
        mNodeHistoryRect = QRect();
        if (actorInstanceData && actorInstanceData->mNodeHistoryItems.GetLength() > 0)
        {
            const uint32 height = (recorder.CalcMaxNodeHistoryTrackIndex(*actorInstanceData) + 1) * (mNodeHistoryItemHeight + 3) + mNodeRectsStartHeight;
            mNodeHistoryRect.setTop(mNodeRectsStartHeight);
            mNodeHistoryRect.setBottom(height);
            mNodeHistoryRect.setLeft(0);
            mNodeHistoryRect.setRight(geometry().width());
        }

        mEventHistoryTotalHeight = 0;
        if (actorInstanceData && actorInstanceData->mEventHistoryItems.GetLength() > 0)
        {
            mEventHistoryTotalHeight = (recorder.CalcMaxEventHistoryTrackIndex(*actorInstanceData) + 1) * 20;
        }
    }


    // find the node history item at a given mouse location
    EMotionFX::Recorder::NodeHistoryItem* TrackDataWidget::FindNodeHistoryItem(EMotionFX::Recorder::ActorInstanceData* actorInstanceData, int32 x, int32 y)
    {
        if (actorInstanceData == nullptr)
        {
            return nullptr;
        }

        if (hasFocus() == false)
        {
            return nullptr;
        }

        // make sure the mTrackRemap array is up to date
        const bool sorted = mPlugin->mTrackHeaderWidget->mSortNodeActivityCheckBox->isChecked();
        const uint32 graphContentsCode = mPlugin->mTrackHeaderWidget->mNodeContentsComboBox->currentIndex();
        EMotionFX::GetRecorder().ExtractNodeHistoryItems(*actorInstanceData, mPlugin->mCurTime, sorted, (EMotionFX::Recorder::EValueType)graphContentsCode, &mActiveItems, &mTrackRemap);

        // get the history items shortcut
        const MCore::Array<EMotionFX::Recorder::NodeHistoryItem*>&  historyItems = actorInstanceData->mNodeHistoryItems;

        QRect rect;
        const uint32 numItems = historyItems.GetLength();
        for (uint32 i = 0; i < numItems; ++i)
        {
            EMotionFX::Recorder::NodeHistoryItem* curItem = historyItems[i];

            // draw the background rect
            double startTimePixel   = mPlugin->TimeToPixel(curItem->mStartTime);
            double endTimePixel     = mPlugin->TimeToPixel(curItem->mEndTime);

            if (startTimePixel > x || endTimePixel < x)
            {
                continue;
            }

            rect.setLeft(startTimePixel);
            rect.setRight(endTimePixel);
            rect.setTop((mNodeRectsStartHeight + (mTrackRemap[curItem->mTrackIndex] * (mNodeHistoryItemHeight + 3)) + 3));
            rect.setBottom(rect.top() + mNodeHistoryItemHeight);

            if (rect.contains(x, y))
            {
                return curItem;
            }
        }

        return nullptr;
    }


    // find the actor instance data for the current selection
    EMotionFX::Recorder::ActorInstanceData* TrackDataWidget::FindActorInstanceData() const
    {
        EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();

        // find the selected actor instance
        EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        if (actorInstance == nullptr)
        {
            return nullptr;
        }

        // find the actor instance data for this actor instance
        const uint32 actorInstanceDataIndex = recorder.FindActorInstanceDataIndex(actorInstance);
        if (actorInstanceDataIndex == MCORE_INVALIDINDEX32) // it doesn't exist, so we didn't record anything for this actor instance
        {
            return nullptr;
        }

        // get the actor instance data for the first selected actor instance, and render the node history for that
        return &recorder.GetActorInstanceData(actorInstanceDataIndex);
    }


    // context event when recorder has a recording
    void TrackDataWidget::DoRecorderContextMenuEvent(QContextMenuEvent* event)
    {
        QPoint point = event->pos();
        mContextMenuX = point.x();
        mContextMenuY = point.y();

        // create the context menu
        QMenu menu(this);

        //---------------------
        // Timeline actions
        //---------------------
        QAction* action = menu.addAction("Zoom To Fit All");
        //action->setIcon( MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/FitAll.png") );
        connect(action, SIGNAL(triggered()), mPlugin, SLOT(OnZoomAll()));

        action = menu.addAction("Reset Timeline");
        //action->setIcon( MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/FitAll.png") );
        connect(action, SIGNAL(triggered()), mPlugin, SLOT(OnResetTimeline()));

        //action = menu.addAction( "Center on current time" );
        //action->setIcon( MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/FitAll.png") );
        //connect(action, SIGNAL(triggered()), mPlugin, SLOT(OnCenterOnCurTime()));


        //---------------------
        // Right-clicked on a motion item
        //---------------------
        EMotionFX::Recorder::NodeHistoryItem* historyItem = FindNodeHistoryItem(FindActorInstanceData(), point.x(), point.y());
        if (historyItem)
        {
            menu.addSeparator();

            action = menu.addAction("Show Node In Graph");
            //action->setIcon( MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/FitAll.png") );
            connect(action, SIGNAL(triggered()), mPlugin, SLOT(OnShowNodeHistoryNodeInGraph()));
        }
        /*
            menu.addSeparator();

            //--------------------------
            // Motion Contents Submenu
            //--------------------------
            QMenu* motionMenu = menu.addMenu("Node Contents");

            action = motionMenu->addAction( "Global Weights" );
            action->setCheckable( true );
            action->setChecked( mRecordContextFlags & CONTEXTFLAG_GLOBALWEIGHTS );
            connect(action, SIGNAL(triggered()), this, SLOT(OnToggleGlobalWeights()));

            action = motionMenu->addAction( "Local Weights" );
            action->setCheckable( true );
            action->setChecked( mRecordContextFlags & CONTEXTFLAG_LOCALWEIGHTS );
            connect(action, SIGNAL(triggered()), this, SLOT(OnToggleLocalWeights()));

            action = motionMenu->addAction( "Normalized Play Time" );
            action->setCheckable( true );
            action->setChecked( mRecordContextFlags & CONTEXTFLAG_PLAYTIMES );
            connect(action, SIGNAL(triggered()), this, SLOT(OnTogglePlayTimes()));

            motionMenu->addSeparator();

            action = motionMenu->addAction( "Node Name" );
            action->setCheckable( true );
            action->setChecked( mRecordContextFlags & CONTEXTFLAG_NODENAMES );
            connect(action, SIGNAL(triggered()), this, SLOT(OnToggleNodeNames()));

            action = motionMenu->addAction( "Motion FileName" );
            action->setCheckable( true );
            action->setChecked( mRecordContextFlags & CONTEXTFLAG_MOTIONFILENAMES );
            connect(action, SIGNAL(triggered()), this, SLOT(OnToggleMotionFileNames()));
        */
        //---------------------

        // show the menu at the given position
        menu.exec(event->globalPos());
    }


    /*
    void TrackDataWidget::OnToggleGlobalWeights()
    {
        mRecordContextFlags |= CONTEXTFLAG_GLOBALWEIGHTS;
        mRecordContextFlags &= ~CONTEXTFLAG_LOCALWEIGHTS;
        mRecordContextFlags &= ~CONTEXTFLAG_PLAYTIMES;
        mGraphValueType = EMotionFX::Recorder::VALUETYPE_GLOBALWEIGHT;
    }


    void TrackDataWidget::OnToggleLocalWeights()
    {
        mRecordContextFlags |= CONTEXTFLAG_LOCALWEIGHTS;
        mRecordContextFlags &= ~CONTEXTFLAG_GLOBALWEIGHTS;
        mRecordContextFlags &= ~CONTEXTFLAG_PLAYTIMES;
        mGraphValueType = EMotionFX::Recorder::VALUETYPE_LOCALWEIGHT;
    }


    void TrackDataWidget::OnTogglePlayTimes()
    {
        mRecordContextFlags |= CONTEXTFLAG_PLAYTIMES;
        mRecordContextFlags &= ~CONTEXTFLAG_GLOBALWEIGHTS;
        mRecordContextFlags &= ~CONTEXTFLAG_LOCALWEIGHTS;
        mGraphValueType = EMotionFX::Recorder::VALUETYPE_PLAYTIME;
    }


    void TrackDataWidget::OnToggleNodeNames()
    {
        if (mRecordContextFlags & CONTEXTFLAG_NODENAMES)
            mRecordContextFlags &= ~CONTEXTFLAG_NODENAMES;
        else
            mRecordContextFlags |= CONTEXTFLAG_NODENAMES;
    }


    void TrackDataWidget::OnToggleMotionFileNames()
    {
        if (mRecordContextFlags & CONTEXTFLAG_MOTIONFILENAMES)
            mRecordContextFlags &= ~CONTEXTFLAG_MOTIONFILENAMES;
        else
            mRecordContextFlags |= CONTEXTFLAG_MOTIONFILENAMES;
    }

    */
    // build a tooltip for a node history item
    void TrackDataWidget::BuildToolTipString(EMotionFX::Recorder::NodeHistoryItem* item, AZStd::string& outString)
    {
        outString = "<table border=\"0\">";

        // node name
        outString += AZStd::string::format("<tr><td width=\"150\"><p style=\"color:rgb(200,200,200)\"><b>Node Name:&nbsp;</b></p></td>");
        outString += AZStd::string::format("<td width=\"400\"><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", item->mName.AsChar());

        // build the node path string
        EMotionFX::ActorInstance* actorInstance = FindActorInstanceData()->mActorInstance;
        EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
        if (animGraphInstance)
        {
            EMotionFX::AnimGraph* animGraph = animGraphInstance->GetAnimGraph();
            EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNodeByUniqueID(item->mNodeUniqueID);
            if (node)
            {
                MCore::Array<EMotionFX::AnimGraphNode*> nodePath;
                EMotionFX::AnimGraphNode* curNode = node->GetParentNode();
                while (curNode)
                {
                    nodePath.Insert(0, curNode);
                    curNode = curNode->GetParentNode();
                }

                MCore::String nodePathString;
                nodePathString.Reserve(256);
                for (uint32 i = 0; i < nodePath.GetLength(); ++i)
                {
                    nodePathString += nodePath[i]->GetName();
                    if (i != nodePath.GetLength() - 1)
                    {
                        nodePathString += " > ";
                    }
                }

                outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Node Path:&nbsp;</b></p></td>");
                outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", nodePathString.AsChar());

                outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Node Type:&nbsp;</b></p></td>");
                outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", node->GetTypeString());

                outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Parent Type:&nbsp;</b></p></td>");
                outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", node->GetParentNode()->GetTypeString());

                if (node->GetNumChildNodes() > 0)
                {
                    outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Child Nodes:&nbsp;</b></p></td>");
                    outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%d</p></td></tr>", node->GetNumChildNodes());

                    outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Recursive Children:&nbsp;</b></p></td>");
                    outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%d</p></td></tr>", node->RecursiveCalcNumNodes());
                }
            }
        }

        // motion name
        if (item->mMotionID != MCORE_INVALIDINDEX32 && item->mMotionFileName.GetLength() > 0)
        {
            outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Motion FileName:&nbsp;</b></p></td>");
            outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", item->mMotionFileName.AsChar());

            // show motion info
            EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(item->mMotionID);
            if (motion)
            {
                MCore::String path = motion->GetFileNameString().ExtractPath();
                path.Replace(EMotionFX::GetEMotionFX().GetMediaRootFolder(), "");
                outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Motion Path:&nbsp;</b></p></td>");
                outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", path.AsChar());

                outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Motion Type:&nbsp;</b></p></td>");
                outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", motion->GetTypeString());

                outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Motion Duration:&nbsp;</b></p></td>");
                outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%.3f seconds</p></td></tr>", motion->GetMaxTime());

                outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Event Tracks:&nbsp;</b></p></td>");
                outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%d</p></td></tr>", motion->GetEventTable()->GetNumTracks());
            }
            else
            {
                outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Motion FileName:&nbsp;</b></p></td>");
                outString += AZStd::string::format("<td><p style=\"color:rgb(255, 0, 0)\">%s</p></td></tr>", "<not loaded anymore>");
            }
        }

        outString += "</table>";
    }


    // find the item at a given location
    EMotionFX::Recorder::EventHistoryItem* TrackDataWidget::FindEventHistoryItem(EMotionFX::Recorder::ActorInstanceData* actorInstanceData, int32 x, int32 y) const
    {
        if (actorInstanceData == nullptr)
        {
            return nullptr;
        }

        if (hasFocus() == false)
        {
            return nullptr;
        }

        const MCore::Array<EMotionFX::Recorder::EventHistoryItem*>& historyItems = actorInstanceData->mEventHistoryItems;
        const float tickHalfWidth = 7;
        const float tickHeight = 16;

        const uint32 numItems = historyItems.GetLength();
        for (uint32 i = 0; i < numItems; ++i)
        {
            EMotionFX::Recorder::EventHistoryItem* curItem = historyItems[i];

            float height = (curItem->mTrackIndex * 20) + mEventsStartHeight;
            double startTimePixel   = mPlugin->TimeToPixel(curItem->mStartTime);
            //double endTimePixel       = mPlugin->TimeToPixel( curItem->mEndTime );

            const QRect rect(QPoint(startTimePixel - tickHalfWidth, height), QSize(tickHalfWidth * 2, tickHeight));
            if (rect.contains(QPoint(x, y)))
            {
                return curItem;
            }
        }

        return nullptr;
    }


    // build a tooltip for an event history item
    void TrackDataWidget::BuildToolTipString(EMotionFX::Recorder::EventHistoryItem* item, AZStd::string& outString)
    {
        outString = "<table border=\"0\">";

        // node name
        outString += AZStd::string::format("<tr><td width=\"150\"><p style=\"color:rgb(200,200,200)\"><b>Event Type:&nbsp;</b></p></td>");
        outString += AZStd::string::format("<td width=\"400\"><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", item->mEventInfo.mTypeString->AsChar());

        outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Event Parameters:&nbsp;</b></p></td>");
        outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", item->mEventInfo.mParameters->AsChar());

        outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Event ID:&nbsp;</b></p></td>");
        outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%d</p></td></tr>", item->mEventInfo.mTypeID);

        outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Local Event Time:&nbsp;</b></p></td>");
        outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%.3f seconds</p></td></tr>", item->mEventInfo.mTimeValue);

        outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Event Trigger Time:&nbsp;</b></p></td>");
        outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%.3f seconds</p></td></tr>", item->mStartTime);

        outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Is Ranged Event:&nbsp;</b></p></td>");
        outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", (item->mIsTickEvent == false) ? "Yes" : "No");

        if (item->mIsTickEvent == false)
        {
            outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Ranged Info:&nbsp;</b></p></td>");
            outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", (item->mEventInfo.mIsEventStart) ? "Event Start" : "Event End");
        }

        outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Global Weight:&nbsp;</b></p></td>");
        outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%.3f</p></td></tr>", item->mEventInfo.mGlobalWeight);

        outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Local Weight:&nbsp;</b></p></td>");
        outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%.3f</p></td></tr>", item->mEventInfo.mLocalWeight);

        // build the node path string
        EMotionFX::ActorInstance* actorInstance = FindActorInstanceData()->mActorInstance;
        EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
        if (animGraphInstance)
        {
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(item->mAnimGraphID);//animGraphInstance->GetAnimGraph();
            EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNodeByUniqueID(item->mEmitterUniqueID);
            if (node)
            {
                outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Emitted By:&nbsp;</b></p></td>");
                outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", node->GetName());

                MCore::Array<EMotionFX::AnimGraphNode*> nodePath;
                EMotionFX::AnimGraphNode* curNode = node->GetParentNode();
                while (curNode)
                {
                    nodePath.Insert(0, curNode);
                    curNode = curNode->GetParentNode();
                }

                MCore::String nodePathString;
                nodePathString.Reserve(256);
                for (uint32 i = 0; i < nodePath.GetLength(); ++i)
                {
                    nodePathString += nodePath[i]->GetName();
                    if (i != nodePath.GetLength() - 1)
                    {
                        nodePathString += " > ";
                    }
                }

                outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Node Path:&nbsp;</b></p></td>");
                outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", nodePathString.AsChar());

                outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Node Type:&nbsp;</b></p></td>");
                outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", node->GetTypeString());

                outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Parent Type:&nbsp;</b></p></td>");
                outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", node->GetParentNode()->GetTypeString());

                if (node->GetNumChildNodes() > 0)
                {
                    outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Child Nodes:&nbsp;</b></p></td>");
                    outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%d</p></td></tr>", node->GetNumChildNodes());

                    outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Recursive Children:&nbsp;</b></p></td>");
                    outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%d</p></td></tr>", node->RecursiveCalcNumNodes());
                }

                // show the motion info
                if (node->GetType() == EMotionFX::AnimGraphMotionNode::TYPE_ID)
                {
                    EMotionFX::AnimGraphMotionNode* motionNode = static_cast<EMotionFX::AnimGraphMotionNode*>(node);
                    EMotionFX::MotionInstance* motionInstance = motionNode->FindMotionInstance(animGraphInstance);
                    if (motionInstance)
                    {
                        EMotionFX::Motion* motion = motionInstance->GetMotion();
                        if (motion)
                        {
                            outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Motion FileName:&nbsp;</b></p></td>");
                            outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", motion->GetFileNameString().ExtractFileName().AsChar());

                            outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Motion Type:&nbsp;</b></p></td>");
                            outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", motion->GetTypeString());

                            outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Motion Duration:&nbsp;</b></p></td>");
                            outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%.3f seconds</p></td></tr>", motion->GetMaxTime());

                            outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Event Tracks:&nbsp;</b></p></td>");
                            outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%d</p></td></tr>", motion->GetEventTable()->GetNumTracks());
                        }
                        else
                        {
                            outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Motion FileName:&nbsp;</b></p></td>");
                            outString += AZStd::string::format("<td><p style=\"color:rgb(255, 0, 0)\">%s</p></td></tr>", "<not loaded anymore>");
                        }
                    }
                }
            }
        }

        outString += "</table>";
    }


    // paint a title bar
    uint32 TrackDataWidget::PaintTitle(const QString& text, QPainter& painter, int32 heightOffset, float animationLength)
    {
        QRect rect(QPoint(0, heightOffset), QSize(mPlugin->TimeToPixel(animationLength), 20));

        //  QLinearGradient bgGradient(0, rect.top(), 0, rect.bottom());
        QLinearGradient bgGradient(0, 0, rect.width(), 0);
        bgGradient.setColorAt(0.0f, QColor(0, 0, 0, 128));
        bgGradient.setColorAt(1.0f, QColor(0, 0, 0, 0));

        //painter.setBrush( QColor(0,0,0,64) );
        painter.setBrush(bgGradient);
        painter.setPen(Qt::NoPen);
        painter.drawRect(rect);

        painter.setFont(mTitleFont);
        painter.setPen(QColor(255, 255, 255, 200));
        painter.drawText(QPoint(3, rect.bottom() - 4), text);

        return 20;
    }


    // paint a title bar
    uint32 TrackDataWidget::PaintSeparator(QPainter& painter, int32 heightOffset, float animationLength)
    {
        painter.setPen(QColor(60, 70, 80));
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(QPoint(0, heightOffset), QPoint(mPlugin->TimeToPixel(animationLength), heightOffset));
        return 1;
    }
}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TrackDataWidget.moc>