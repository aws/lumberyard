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

// include required headers
#include "TimeViewPlugin.h"
#include "TrackDataHeaderWidget.h"
#include "TrackDataWidget.h"
#include "TrackHeaderWidget.h"
#include "TimeInfoWidget.h"

#include "../MotionWindow/MotionWindowPlugin.h"
#include "../MotionWindow/MotionListWindow.h"
#include "../MotionSetsWindow/MotionSetsWindowPlugin.h"
#include "../MotionEvents/MotionEventsPlugin.h"
#include "../MotionEvents/MotionEventPresetsWidget.h"

#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "../../../../EMStudioSDK/Source/MainWindow.h"

#include <QCheckBox>
#include <QDockWidget>
#include <QGridLayout>
#include <QPushButton>
#include <QScrollBar>

#include <MCore/Source/LogManager.h>
#include <MCore/Source/Compare.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/MotionEvent.h>
#include <EMotionFX/Source/MotionEventTrack.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/Recorder.h>
#include <EMotionFX/Source/MotionEventTable.h>


namespace EMStudio
{
    // constructor
    TimeViewPlugin::TimeViewPlugin()
        : EMStudio::DockWidgetPlugin()
    {
        mPixelsPerSecond    = 60;
        mCurTime            = 0;
        mFPS                = 32;
        mTimeScale          = 1.0;
        mTargetTimeScale    = 1.0;
        mScrollX            = 0.0;
        mTargetScrollX      = 0.0;
        mMaxTime            = 0.0;
        mMaxHeight          = 0.0;
        mMinScale           = 0.25;
        mMaxScale           = 100.0;
        mCurMouseX          = 0;
        mCurMouseY          = 0;
        mTotalTime          = FLT_MAX;
        mZoomInCursor       = nullptr;
        mZoomOutCursor      = nullptr;
        mIsAnimating        = false;
        mDirty              = true;

        mTrackDataHeaderWidget = nullptr;
        mTrackDataWidget    = nullptr;
        mTrackHeaderWidget  = nullptr;
        mTimeInfoWidget     = nullptr;

        mNodeHistoryItem    = nullptr;
        mEventHistoryItem   = nullptr;
        mActorInstanceData  = nullptr;
        mEventEmitterNode   = nullptr;

        mMainWidget                 = nullptr;
        mMotionWindowPlugin         = nullptr;
        mMotionEventsPlugin         = nullptr;
        mMotionListWindow           = nullptr;
        m_motionSetPlugin           = nullptr;
        mMotion                     = nullptr;
        mAdjustMotionCallback       = nullptr;
        mSelectCallback             = nullptr;
        mUnselectCallback           = nullptr;
        mClearSelectionCallback     = nullptr;
        mRecorderClearCallback      = nullptr;

        mBrushCurTimeHandle = QBrush(QColor(255, 180, 0));
        mPenCurTimeHandle   = QPen(QColor(255, 180, 0));
        mPenTimeHandles     = QPen(QColor(150, 150, 150), 1, Qt::DotLine);
        mPenCurTimeHelper   = QPen(QColor(100, 100, 100), 1, Qt::DotLine);
    }


    // destructor
    TimeViewPlugin::~TimeViewPlugin()
    {
        GetCommandManager()->RemoveCommandCallback(mAdjustMotionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mSelectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mUnselectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mClearSelectionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mRecorderClearCallback, false);
        delete mAdjustMotionCallback;
        delete mSelectCallback;
        delete mUnselectCallback;
        delete mClearSelectionCallback;
        delete mRecorderClearCallback;

        RemoveAllTracks();

        // get rid of the cursors
        delete mZoomInCursor;
        delete mZoomOutCursor;

        // get rid of the motion infos
        const uint32 numMotionInfos = mMotionInfos.GetLength();
        for (uint32 i = 0; i < numMotionInfos; ++i)
        {
            delete mMotionInfos[i];
        }
    }


    // get the compile date
    const char* TimeViewPlugin::GetCompileDate() const
    {
        return MCORE_DATE;
    }


    // get the name
    const char* TimeViewPlugin::GetName() const
    {
        return "Time View";
    }


    // get the plugin type id
    uint32 TimeViewPlugin::GetClassID() const
    {
        return TimeViewPlugin::CLASS_ID;
    }


    // get the creator name
    const char* TimeViewPlugin::GetCreatorName() const
    {
        return "MysticGD";
    }


    // get the version
    float TimeViewPlugin::GetVersion() const
    {
        return 1.0f;
    }


    // clone the log window
    EMStudioPlugin* TimeViewPlugin::Clone()
    {
        TimeViewPlugin* newPlugin = new TimeViewPlugin();
        return newPlugin;
    }


    // on before remove plugin
    void TimeViewPlugin::OnBeforeRemovePlugin(uint32 classID)
    {
        if (classID == MotionWindowPlugin::CLASS_ID)
        {
            mMotionWindowPlugin = nullptr;
        }

        if (classID == MotionEventsPlugin::CLASS_ID)
        {
            mMotionEventsPlugin = nullptr;
        }
    }


    // init after the parent dock window has been created
    bool TimeViewPlugin::Init()
    {
        // create callbacks
        mAdjustMotionCallback = new CommandAdjustMotionCallback(false);
        mSelectCallback = new CommandSelectCallback(false);
        mUnselectCallback = new CommandUnselectCallback(false);
        mClearSelectionCallback = new CommandClearSelectionCallback(false);
        mRecorderClearCallback = new CommandRecorderClearCallback(false);
        GetCommandManager()->RegisterCommandCallback("AdjustMotion", mAdjustMotionCallback);
        GetCommandManager()->RegisterCommandCallback("Select", mSelectCallback);
        GetCommandManager()->RegisterCommandCallback("Unselect", mUnselectCallback);
        GetCommandManager()->RegisterCommandCallback("ClearSelection", mClearSelectionCallback);
        GetCommandManager()->RegisterCommandCallback("RecorderClear", mRecorderClearCallback);

        // load the cursors
        mZoomInCursor       = new QCursor(QPixmap(AZStd::string(MysticQt::GetDataDir() + "Images/Rendering/ZoomInCursor.png").c_str()).scaled(32, 32));
        mZoomOutCursor      = new QCursor(QPixmap(AZStd::string(MysticQt::GetDataDir() + "Images/Rendering/ZoomOutCursor.png").c_str()).scaled(32, 32));

        // create main widget
        mMainWidget = new QWidget(mDock);
        mDock->SetContents(mMainWidget);
        QGridLayout* mainLayout = new QGridLayout();
        mainLayout->setMargin(0);
        mainLayout->setSpacing(0);
        mMainWidget->setLayout(mainLayout);

        // create widgets in the header
        // Top-left
        mTimeInfoWidget = new TimeInfoWidget(this);
        mTimeInfoWidget->setFixedSize(175, 40);
        mainLayout->addWidget(mTimeInfoWidget, 0, 0);

        // Top-right
        mTrackDataHeaderWidget = new TrackDataHeaderWidget(this, mDock);
        mTrackDataHeaderWidget->setFixedHeight(40);
        mainLayout->addWidget(mTrackDataHeaderWidget, 0, 1);

        // create widgets in the body. For the body we are going to put a scroll area
        // so we can get a vertical scroll bar when we have more tracks than what the
        // view can show
        QScrollArea* bodyWidget = new QScrollArea(mMainWidget);
        bodyWidget->setFrameShape(QFrame::NoFrame);
        bodyWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        bodyWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        bodyWidget->setWidgetResizable(true);

        // scroll areas require an inner widget to hold the layout (instead of a layout
        // directly).
        QWidget* innerWidget = new QWidget(bodyWidget);
        QHBoxLayout* bodyLayout = new QHBoxLayout();
        bodyLayout->setMargin(0);
        bodyLayout->setSpacing(0);
        innerWidget->setLayout(bodyLayout);
        bodyWidget->setWidget(innerWidget);
        mainLayout->addWidget(bodyWidget, 1, 0, 1, 2);

        // Bottom-left
        mTrackHeaderWidget = new TrackHeaderWidget(this, mDock);
        mTrackHeaderWidget->setFixedWidth(175);
        bodyLayout->addWidget(mTrackHeaderWidget);

        // bottom-right
        mTrackDataWidget = new TrackDataWidget(this, mDock);
        bodyLayout->addWidget(mTrackDataWidget);

        connect(mTrackDataWidget, &TrackDataWidget::SelectionChanged, this, &TimeViewPlugin::OnSelectionChanged);

        connect(mTrackDataWidget, &TrackDataWidget::ElementTrackChanged, this, &TimeViewPlugin::MotionEventTrackChanged);
        connect(mTrackDataWidget, &TrackDataWidget::MotionEventChanged, this, &TimeViewPlugin::MotionEventChanged);
        connect(this, &TimeViewPlugin::DeleteKeyPressed, this, &TimeViewPlugin::RemoveSelectedMotionEvents);
        connect(mDock, &MysticQt::DockWidget::visibilityChanged, this, &TimeViewPlugin::VisibilityChanged);

        connect(this, &TimeViewPlugin::ManualTimeChange, this, &TimeViewPlugin::OnManualTimeChange);

        SetCurrentTime(0.0f);
        SetScale(1.0f);

        SetRedrawFlag();

        return true;
    }


    // add a new track
    void TimeViewPlugin::AddTrack(TimeTrack* track)
    {
        mTracks.Add(track);
        SetRedrawFlag();
    }


    // delete all tracks
    void TimeViewPlugin::RemoveAllTracks()
    {
        // get the number of time tracks and iterate through them
        const uint32 numTracks = mTracks.GetLength();
        for (uint32 i = 0; i < numTracks; ++i)
        {
            delete mTracks[i];
        }

        mTracks.Clear();
        SetRedrawFlag();
    }

    TimeTrack* TimeViewPlugin::FindTrackByElement(TimeTrackElement* element) const
    {
        // get the number of time tracks and iterate through them
        const uint32 numTracks = mTracks.GetLength();
        for (uint32 i = 0; i < numTracks; ++i)
        {
            TimeTrack* timeTrack = mTracks[i];

            // get the number of time track elements and iterate through them
            const uint32 numElements = timeTrack->GetNumElements();
            for (uint32 j = 0; j < numElements; ++j)
            {
                if (timeTrack->GetElement(j) == element)
                {
                    return timeTrack;
                }
            }
        }

        return nullptr;
    }


    // round a double based on 0.5 (everything above is rounded up, everything else rounded down)
    double TimeViewPlugin::RoundDouble(double x) const
    {
        if ((std::numeric_limits<double>::max() - 0.5) <= x)
        {
            return std::numeric_limits<double>::max();
        }

        if ((-1 * std::numeric_limits<double>::max() + 0.5) > x)
        {
            return (-1 * std::numeric_limits<double>::max());
        }

        double intpart;
        double fractpart = modf(x, &intpart);

        if (fractpart >= 0.5)
        {
            return (intpart + 1);
        }
        else
        if (fractpart >= -0.5)
        {
            return intpart;
        }
        else
        {
            return (intpart - 1);
        }
    }


    void TimeViewPlugin::DecomposeTime(double timeValue, uint32* outMinutes, uint32* outSeconds, uint32* outMilSecs, uint32* outFrameNr) const
    {
        if (outMinutes)
        {
            *outMinutes     = timeValue / 60.0;
        }
        if (outSeconds)
        {
            *outSeconds     = fmod(timeValue, 60.0);
        }
        if (outMilSecs)
        {
            *outMilSecs     = fmod(RoundDouble(timeValue * 1000.0), 1000.0) / 10.0;              //fmod(pixelTime, 1.0) * 100.0;
        }
        if (outFrameNr)
        {
            *outFrameNr     = timeValue / (double)mFPS;
        }
    }


    // calculate time values
    void TimeViewPlugin::CalcTime(double xPixel, double* outPixelTime, uint32* outMinutes, uint32* outSeconds, uint32* outMilSecs, uint32* outFrameNr, bool scaleXPixel) const
    {
        if (scaleXPixel)
        {
            xPixel *= mTimeScale;
        }

        const double pixelTime = ((xPixel + mScrollX) / mPixelsPerSecond);

        if (outPixelTime)
        {
            *outPixelTime   = pixelTime;
        }
        if (outMinutes)
        {
            *outMinutes     = pixelTime / 60.0;
        }
        if (outSeconds)
        {
            *outSeconds     = fmod(pixelTime, 60.0);
        }
        if (outMilSecs)
        {
            *outMilSecs     = fmod(RoundDouble(pixelTime * 1000.0), 1000.0) / 10.0;             //fmod(pixelTime, 1.0) * 100.0;
        }
        if (outFrameNr)
        {
            *outFrameNr     = pixelTime / (double)mFPS;
        }
    }


    void TimeViewPlugin::UpdateCurrentMotionInfo()
    {
        if (EMotionFX::GetRecorder().GetRecordTime() > MCore::Math::epsilon)
        {
            return;
        }

        if (mMotion)
        {
            MotionInfo* motionInfo  = FindMotionInfo(mMotion->GetID());
            motionInfo->mScale      = mTargetTimeScale;
            motionInfo->mScrollX    = mTargetScrollX;
        }
    }


    // update the sub-widgets
    void TimeViewPlugin::UpdateVisualData()
    {
        ValidatePluginLinks();
        mTrackDataHeaderWidget->update();
        mTrackDataWidget->update();
        mTimeInfoWidget->update();
        mDirty = false;
    }


    /*
    // scroll horizontally
    void TimeViewPlugin::OnHSliderValueChanged(int value)
    {
        mHorizontalScroll->blockSignals(true);
        SetScrollX(value);
        mHorizontalScroll->blockSignals(false);

    //  UpdateVisualData();
    }
    */

    // change the scale / zoom
    /*void TimeViewPlugin::OnScaleSliderValueChanged(int value)
    {
        mTimeScale = value / TIMEVIEW_PIXELSPERSECOND;
        UpdateVisualData();

        // inform the motion info about the changes
        UpdateCurrentMotionInfo();
    }*/


    // calc the time value to a pixel value (excluding scroll)
    double TimeViewPlugin::TimeToPixel(double timeInSeconds, bool scale) const
    {
        //  return ((timeInSeconds * mPixelsPerSecond)/* / mTimeScale*/) - mScrollX;
        double result = ((timeInSeconds * mPixelsPerSecond)) - mScrollX;
        if (scale)
        {
            return (result * mTimeScale);
        }
        else
        {
            return result;
        }
    }


    // return the element at a given pixel, or nullptr
    TimeTrackElement* TimeViewPlugin::GetElementAt(int32 x, int32 y)
    {
        // for all tracks
        const uint32 numTracks = mTracks.GetLength();
        for (uint32 i = 0; i < numTracks; ++i)
        {
            // check if the absolute pixel is inside
            TimeTrackElement* result = mTracks[i]->GetElementAt(x + mScrollX, y);
            if (result)
            {
                return result;
            }
        }

        return nullptr;
    }


    // return the track at a given pixel y value, or nullptr
    TimeTrack* TimeViewPlugin::GetTrackAt(int32 y)
    {
        // for all tracks
        const uint32 numTracks = mTracks.GetLength();
        for (uint32 i = 0; i < numTracks; ++i)
        {
            // check if the absolute pixel is inside
            if (mTracks[i]->GetIsInside(y))
            {
                return mTracks[i];
            }
        }

        return nullptr;
    }


    // unselect all elements
    void TimeViewPlugin::UnselectAllElements()
    {
        // for all tracks
        const uint32 numTracks = mTracks.GetLength();
        for (uint32 t = 0; t < numTracks; ++t)
        {
            TimeTrack* track = mTracks[t];

            // for all elements, deselect it
            const uint32 numElems = track->GetNumElements();
            for (uint32 i = 0; i < numElems; ++i)
            {
                track->GetElement(i)->SetIsSelected(false);
            }
        }
        SetRedrawFlag();

        emit SelectionChanged();
    }


    // return the time of the current time marker, in seconds
    double TimeViewPlugin::GetCurrentTime() const
    {
        return mCurTime;
    }


    double TimeViewPlugin::PixelToTime(double xPixel, bool isScaledPixel) const
    {
        if (isScaledPixel)
        {
            xPixel /= mTimeScale;
        }

        return ((xPixel + mScrollX) / mPixelsPerSecond);
    }


    void TimeViewPlugin::DeltaScrollX(double deltaX, bool animate)
    {
        double newTime = (mTargetScrollX + (deltaX / mTimeScale)) / mPixelsPerSecond;
        if (newTime < mMaxTime - (1 / mTimeScale))
        {
            SetScrollX(mTargetScrollX + (deltaX / mTimeScale), animate);
        }
        else
        {
            SetScrollX((mMaxTime - ((1 / mTimeScale))) * mPixelsPerSecond, animate);
        }
        SetRedrawFlag();
    }

    void TimeViewPlugin::SetScrollX(double scrollX, bool animate)
    {
        mTargetScrollX = scrollX;

        if (mTargetScrollX < 0)
        {
            mTargetScrollX = 0;
        }

        if (animate == false)
        {
            mScrollX = mTargetScrollX;
        }

        // inform the motion info about the changes
        UpdateCurrentMotionInfo();
        SetRedrawFlag();
    }


    // set the current time in seconds
    void TimeViewPlugin::SetCurrentTime(double timeInSeconds)
    {
        mCurTime = timeInSeconds;
    }


    // snap a time value
    bool TimeViewPlugin::SnapTime(double* inOutTime, TimeTrackElement* elementToIgnore, double snapThreshold)
    {
        double inTime = *inOutTime;

        // make sure we don't go out of bounds
        if (inTime < 0.0)
        {
            inTime = 0.0;
        }

        // snap to the current time marker
        double curTimeMarker = GetCurrentTime();
        if (MCore::Math::Abs(inTime - curTimeMarker) < snapThreshold)
        {
            inTime = curTimeMarker;
        }

        // for all tracks
        const uint32 numTracks = mTracks.GetLength();
        for (uint32 t = 0; t < numTracks; ++t)
        {
            TimeTrack* track = mTracks[t];
            if (track->GetIsVisible() == false || track->GetIsEnabled() == false)
            {
                continue;
            }

            // for all elements
            const uint32 numElems = track->GetNumElements();
            for (uint32 i = 0; i < numElems; ++i)
            {
                // don't snap to itself
                TimeTrackElement* element = track->GetElement(i);
                if (element == elementToIgnore)
                {
                    continue;
                }

                // snap the input time to snap locations inside the element
                element->SnapTime(&inTime, snapThreshold);
            }
        }

        // if snapping occurred
        if (MCore::Math::Abs(inTime - *inOutTime) > 0.0001)
        {
            *inOutTime = inTime;
            return true;
        }
        else // no snapping occurred
        {
            *inOutTime = inTime;
            return false;
        }
    }


    // render the element time handles on top of everything
    void TimeViewPlugin::RenderElementTimeHandles(QPainter& painter, uint32 dataWindowHeight, const QPen& pen)
    {
        // for all tracks
        const uint32 numTracks = mTracks.GetLength();
        for (uint32 t = 0; t < numTracks; ++t)
        {
            TimeTrack* track = mTracks[t];
            if (track->GetIsVisible() == false)
            {
                continue;
            }

            // for all elements
            const uint32 numElems = track->GetNumElements();
            for (uint32 i = 0; i < numElems; ++i)
            {
                TimeTrackElement* elem = track->GetElement(i);

                // if the element has to show its time handles, do it
                if (elem->GetShowTimeHandles())
                {
                    // calculate the dimensions
                    int32 startX, startY, width, height;
                    elem->CalcDimensions(&startX, &startY, &width, &height);

                    // draw the lines
                    painter.setPen(pen);
                    //painter.setRenderHint(QPainter::Antialiasing);
                    painter.drawLine(startX, 0, startX, dataWindowHeight);
                    painter.drawLine(startX + width, 0, startX + width, dataWindowHeight);
                }
            }
        }
    }

    // disables all tool tips
    void TimeViewPlugin::DisableAllToolTips()
    {
        // for all tracks
        const uint32 numTracks = mTracks.GetLength();
        for (uint32 t = 0; t < numTracks; ++t)
        {
            TimeTrack* track = mTracks[t];

            // for all elements
            const uint32 numElems = track->GetNumElements();
            for (uint32 i = 0; i < numElems; ++i)
            {
                TimeTrackElement* elem = track->GetElement(i);
                elem->SetShowToolTip(false);
            }
        }
        SetRedrawFlag();
    }


    // check if we're at some resize point with the mouse
    bool TimeViewPlugin::FindResizePoint(int32 x, int32 y, TimeTrackElement** outElement, uint32* outID)
    {
        // for all tracks
        const uint32 numTracks = mTracks.GetLength();
        for (uint32 t = 0; t < numTracks; ++t)
        {
            TimeTrack* track = mTracks[t];
            if (track->GetIsVisible() == false)
            {
                continue;
            }

            // for all elements
            const uint32 numElems = track->GetNumElements();
            for (uint32 i = 0; i < numElems; ++i)
            {
                TimeTrackElement* elem = track->GetElement(i);

                // check for a resize point in the element
                uint32 id;
                if (elem->FindResizePoint(x, y, &id))
                {
                    *outElement = elem;
                    *outID = id;
                    return true;
                }
            }
        }

        // no resize point at this location
        *outElement = nullptr;
        *outID      = MCORE_INVALIDINDEX32;

        // no resize point found
        return false;
    }


    void TimeViewPlugin::VisibilityChanged(bool visible)
    {
        MCORE_UNUSED(visible);
        ValidatePluginLinks();
        SetRedrawFlag();
    }


    // render the frame
    void TimeViewPlugin::ProcessFrame(float timePassedInSeconds)
    {
        if (GetManager()->GetAvoidRendering() || mMainWidget->visibleRegion().isEmpty())
        {
            return;
        }

        mTotalTime += timePassedInSeconds;

        ValidatePluginLinks();

        // animate the zoom
        mScrollX += (mTargetScrollX - mScrollX) * 0.2;

        mIsAnimating = false;
        if (mTargetTimeScale > mTimeScale)
        {
            if (MCore::Math::Abs(mTargetScrollX - mScrollX) <= 1)
            {
                mTimeScale += (mTargetTimeScale - mTimeScale) * 0.1;
            }
        }
        else
        {
            mTimeScale += (mTargetTimeScale - mTimeScale) * 0.1;
        }

        if (MCore::Math::Abs(mTargetScrollX - mScrollX) <= 1)
        {
            mScrollX = mTargetScrollX;
        }
        else
        {
            mIsAnimating = true;
        }

        if (MCore::Math::Abs(mTargetTimeScale - mTimeScale) <= 0.001)
        {
            mTimeScale = mTargetTimeScale;
        }
        else
        {
            mIsAnimating = true;
        }

        // get the maximum time
        GetDataTimes(&mMaxTime, nullptr, nullptr);
        UpdateMaxHeight();
        mTrackDataWidget->UpdateRects();

        if (MCore::Math::Abs(mMaxHeight - mLastMaxHeight) > 0.0001)
        {
            mLastMaxHeight = mMaxHeight;
        }

        if (mTrackDataWidget->mDragging == false && mTrackDataWidget->mResizing == false)
        {
            mTimeInfoWidget->SetOverwriteTime(PixelToTime(mCurMouseX), mMaxTime);
        }

        // update the hovering items
        mEventEmitterNode = nullptr;
        mActorInstanceData  = mTrackDataWidget->FindActorInstanceData();
        if (EMotionFX::GetRecorder().GetRecordTime() > MCore::Math::epsilon)
        {
            mEventHistoryItem   = mTrackDataWidget->FindEventHistoryItem(mActorInstanceData, mCurMouseX, mCurMouseY);
            mNodeHistoryItem    = mTrackDataWidget->FindNodeHistoryItem(mActorInstanceData, mCurMouseX, mCurMouseY);

            if (mEventHistoryItem)
            {
                EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(mEventHistoryItem->mAnimGraphID);
                if (animGraph)
                {
                    mEventEmitterNode = animGraph->RecursiveFindNodeById(mEventHistoryItem->mEmitterNodeId);
                }
            }
        }
        else
        {
            mActorInstanceData  = nullptr;
            mNodeHistoryItem    = nullptr;
            mEventHistoryItem   = nullptr;
        }

        // if we're using the recorder
        EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();
        //if (recorder.IsRecording() || recorder.IsInPlayMode())
        if (recorder.GetRecordTime() > MCore::Math::epsilon)
        {
            if (recorder.GetIsInPlayMode() && recorder.GetIsInAutoPlayMode())
            {
                SetCurrentTime(recorder.GetCurrentPlayTime());
                MakeTimeVisible(mCurTime, 0.5, false);
            }

            if (recorder.GetIsRecording())
            {
                SetCurrentTime(mMaxTime);
                MakeTimeVisible(recorder.GetRecordTime(), 0.95, false);
            }
        }
        else // we're not using the recorder
        {
            const AZStd::vector<EMotionFX::MotionInstance*>& motionInstances = MotionWindowPlugin::GetSelectedMotionInstances();
            if (motionInstances.size() == 1)
            {
                EMotionFX::MotionInstance* motionInstance = motionInstances[0];
                if (MCore::Compare<float>::CheckIfIsClose(mCurTime, motionInstance->GetCurrentTime(), MCore::Math::epsilon) == false)
                {
                    SetCurrentTime(motionInstance->GetCurrentTime());
                    mDirty = true;
                }
            }
        }

        if (mIsAnimating)
        {
            mDirty = true;
        }

        bool redraw = false;
        float fps = 15.0f;
    #ifndef MCORE_DEBUG
        if (mIsAnimating)
        {
            fps = 60.0f;
        }
        else
        {
            fps = 40.0f;
        }
    #endif

        if (mTotalTime >= 1.0f / fps)
        {
            redraw = true;
            mTotalTime = 0.0f;
        }

        if (redraw && mDirty)
        {
            UpdateVisualData();
        }
    }

    /*
    // handle timer event
    void TimeViewPlugin::timerEvent(QTimerEvent* event)
    {
        ValidatePluginLinks();

        if (EMotionFX::GetRecorder().IsRecording() || EMotionFX::GetRecorder().IsInPlayMode())
        {
            if (EMotionFX::GetRecorder().IsInPlayMode() && EMotionFX::GetRecorder().IsInAutoPlayMode())
                SetCurrentTime( EMotionFX::GetRecorder().GetCurrentPlayTime() );

            //mTimeViewWidget->update();
            mTrackDataWidget->update();
        }
        else
        {
            MCore::Array<EMotionFX::MotionInstance*>& motionInstances = MotionWindowPlugin::GetSelectedMotionInstances();
            if (motionInstances.GetLength() == 1)
            {
                MotionInstance* motionInstance = motionInstances[0];
                SetCurrentTime( motionInstance->GetCurrentTime() );
            }
        }
    }
    */


    void TimeViewPlugin::UpdateViewSettings()
    {
        SetScale(mTimeScale);
    }


    void TimeViewPlugin::SetScale(double scale, bool animate)
    {
        //  if (mMaxTime < centerTime)
        /*  double rangeStart   = PixelToTime( 0.0 );
            double rangeEnd     = PixelToTime( mTrackDataWidget->geometry().width() );
            if (rangeEnd > mMaxTime)
                rangeEnd = mMaxTime;

            double centerTime   = (rangeStart + rangeEnd) / 2.0;
        */
        double curTime = GetCurrentTime();

        mTargetTimeScale    = scale;
        mTargetTimeScale    = MCore::Clamp(scale, mMinScale, mMaxScale);

        if (animate == false)
        {
            mTimeScale  = mTargetTimeScale;
        }

        UpdateCurrentMotionInfo();

        SetCurrentTime(curTime);

        //  MakeTimeVisible( centerTime, 0.5 );
    }

    // set the maximum time value
    /*void TimeViewPlugin::SetMaxTime(double maxTime)
    {
        mMaxTime            = maxTime;
        mMaxTimeInPixels    = (maxTime * TIMEVIEW_PIXELSPERSECOND) / mTimeScale;

        double oldSliderMax         = mHorizontalScroll->maximum();
        double oldNormalizedValue   = (double)mHorizontalScroll->value() / oldSliderMax;

        mHorizontalScroll->setRange( 0, mMaxTimeInPixels );
        mHorizontalScroll->setValue( oldNormalizedValue * mMaxTimeInPixels );
    }


    // set the time view scale and keep it in a reasonable range
    void TimeViewPlugin::SetTimeScale(float scale)
    {
        mTimeScale = scale;

        const float minScale = 0.01f;
        //const float maxScale = 100.0f;

        if (mTimeScale < minScale)
            mTimeScale = minScale;

        if (mTimeScale > mMaxScale)
            mTimeScale = mMaxScale;

        // adjust the maximum time
        SetMaxTime(mMaxTime);

        // adjust the current play time
        SetCurrentTime( GetCurrentTime() );

        // inform the motion info about the changes
        UpdateCurrentMotionInfo();
    }*/


    void TimeViewPlugin::OnKeyPressEvent(QKeyEvent* event)
    {
        // delete pressed
        if (event->key() == Qt::Key_Delete)
        {
            emit DeleteKeyPressed();
            event->accept();
            return;
        }

        if (event->key() == Qt::Key_Down)
        {
            mTrackDataWidget->scroll(0, 20);
            event->accept();
            return;
        }

        if (event->key() == Qt::Key_Up)
        {
            mTrackDataWidget->scroll(0, -20);
            event->accept();
            return;
        }

        if (event->key() == Qt::Key_Plus)
        {
            double zoomDelta = 0.1 * 3 * MCore::Clamp(mTargetTimeScale / 2.0, 1.0, 22.0);
            SetScale(mTargetTimeScale + zoomDelta);
            event->accept();
            return;
        }

        if (event->key() == Qt::Key_Minus)
        {
            double zoomDelta = 0.1 * 3 * MCore::Clamp(mTargetTimeScale / 2.0, 1.0, 22.0);
            SetScale(mTargetTimeScale - zoomDelta);
            event->accept();
            return;
        }

        if (EMotionFX::GetRecorder().GetIsRecording() == false && EMotionFX::GetRecorder().GetIsInAutoPlayMode() == false)
        {
            if (event->key() == Qt::Key_Left)
            {
                mTargetScrollX -= (mPixelsPerSecond * 3) / mTimeScale;
                if (mTargetScrollX < 0)
                {
                    mTargetScrollX = 0;
                }
                event->accept();
                return;
            }

            if (event->key() == Qt::Key_Right)
            {
                const double newTime = (mScrollX + ((mPixelsPerSecond * 3) / mTimeScale)) / mPixelsPerSecond;
                if (newTime < mMaxTime)
                {
                    mTargetScrollX += ((mPixelsPerSecond * 3) / mTimeScale);
                }

                event->accept();
                return;
            }

            // fit the contents
            if (event->key() == Qt::Key_A)
            {
                OnZoomAll();
                event->accept();
                return;
            }

            if (event->key() == Qt::Key_Z)
            {
                OnCenterOnCurTime();
                event->accept();
                return;
            }

            if (event->key() == Qt::Key_Home)
            {
                OnGotoTimeZero();
                event->accept();
                return;
            }

            if (event->key() == Qt::Key_PageUp)
            {
                mTargetScrollX -= mTrackDataWidget->geometry().width() / mTimeScale;
                if (mTargetScrollX < 0)
                {
                    mTargetScrollX = 0;
                }
                event->accept();
                return;
            }

            if (event->key() == Qt::Key_PageDown)
            {
                const double newTime = (mScrollX + (mTrackDataWidget->geometry().width() / mTimeScale)) / mPixelsPerSecond;
                if (newTime < mMaxTime)
                {
                    mTargetScrollX += mTrackDataWidget->geometry().width() / mTimeScale;
                }

                event->accept();
                return;
            }
        }


        event->ignore();
    }


    void TimeViewPlugin::OnKeyReleaseEvent(QKeyEvent* event)
    {
        if (event->key() == Qt::Key_Delete)
        {
            event->accept();
            return;
        }

        event->ignore();
    }


    void TimeViewPlugin::ValidatePluginLinks()
    {
        if (mMotionWindowPlugin == nullptr)
        {
            EMStudioPlugin* motionBasePlugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionWindowPlugin::CLASS_ID);
            if (motionBasePlugin)
            {
                mMotionWindowPlugin = (MotionWindowPlugin*)motionBasePlugin;
                mMotionListWindow   = mMotionWindowPlugin->GetMotionListWindow();

                // Attention: do this only once!
                connect(mMotionListWindow, &MotionListWindow::MotionSelectionChanged, this, &TimeViewPlugin::MotionSelectionChanged);
            }
        }

        if (!m_motionSetPlugin)
        {
            EMStudioPlugin* motionSetBasePlugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionSetsWindowPlugin::CLASS_ID);
            if (motionSetBasePlugin)
            {
                m_motionSetPlugin = (MotionSetsWindowPlugin*)motionSetBasePlugin;
                connect(m_motionSetPlugin->GetMotionSetWindow(), &MotionSetWindow::MotionSelectionChanged, this, &TimeViewPlugin::MotionSelectionChanged);
            }
        }

        if (mMotionEventsPlugin == nullptr)
        {
            EMStudioPlugin* motionEventsBasePlugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionEventsPlugin::CLASS_ID);
            if (motionEventsBasePlugin)
            {
                mMotionEventsPlugin = (MotionEventsPlugin*)motionEventsBasePlugin;
                mMotionEventsPlugin->ValidatePluginLinks();
            }
        }
    }


    void TimeViewPlugin::MotionSelectionChanged()
    {
        EMotionFX::Motion* motion = GetCommandManager()->GetCurrentSelection().GetSingleMotion();
        if (mMotion != motion)
        {
            mMotion = motion;
            ReInit();
        }

        if (mTrackHeaderWidget)
        {
            if (motion == nullptr)
            {
                mTrackHeaderWidget->GetAddTrackButton()->setEnabled(false);
            }
            else
            {
                mTrackHeaderWidget->GetAddTrackButton()->setEnabled(true);
            }
        }
    }


    void TimeViewPlugin::UpdateSelection()
    {
        mSelectedEvents.Clear(false);
        if (!mMotion)
        {
            return;
        }

        // get the motion event table
        const EMotionFX::MotionEventTable* eventTable = mMotion->GetEventTable();

        // get the number of tracks in the time view and iterate through them
        const uint32 numTracks = GetNumTracks();
        for (uint32 trackIndex = 0; trackIndex < numTracks; ++trackIndex)
        {
            // get the current time view track
            const TimeTrack* track = GetTrack(trackIndex);
            if (track->GetIsVisible() == false)
            {
                continue;
            }

            const AZ::Outcome<size_t> trackNr = eventTable->FindTrackIndexByName(track->GetName());
            if (!trackNr.IsSuccess())
            {
                continue;
            }

            // get the number of elements in the track and iterate through them
            const uint32 numTrackElements = track->GetNumElements();
            for (uint32 elementIndex = 0; elementIndex < numTrackElements; ++elementIndex)
            {
                TimeTrackElement* element = track->GetElement(elementIndex);
                if (element->GetIsVisible() == false)
                {
                    continue;
                }

                if (element->GetIsSelected())
                {
                    EventSelectionItem selectionItem;
                    selectionItem.mMotion = mMotion;
                    selectionItem.mTrackNr = trackNr.GetValue();
                    selectionItem.mEventNr = element->GetElementNumber();
                    mSelectedEvents.Add(selectionItem);
                }
            }
        }
    }


    void TimeViewPlugin::ReInit()
    {
        if (EMotionFX::GetMotionManager().FindMotionIndex(mMotion) == MCORE_INVALIDINDEX32)
        {
            // set the motion first back to nullptr
            mMotion = nullptr;
        }

        // update the selection and save it
        UpdateSelection();

        ValidatePluginLinks();

        // if we have a recording, don't init things for the motions
        if (EMotionFX::GetRecorder().GetIsRecording() || EMotionFX::GetRecorder().GetRecordTime() > MCore::Math::epsilon || EMotionFX::GetRecorder().GetIsInPlayMode())
        {
            SetScrollX(0);
            mTrackHeaderWidget->ReInit();
            return;
        }

        if (mMotion)
        {
            size_t trackIndex;
            AZStd::string text;

            const EMotionFX::MotionEventTable* eventTable = mMotion->GetEventTable();

            // get the number of motion event tracks and iterate through them
            const size_t numEventTracks = eventTable->GetNumTracks();
            for (trackIndex = 0; trackIndex < numEventTracks; ++trackIndex)
            {
                const EMotionFX::MotionEventTrack* eventTrack = eventTable->GetTrack(trackIndex);

                TimeTrack* timeTrack = nullptr;
                if (trackIndex < GetNumTracks())
                {
                    timeTrack = GetTrack(static_cast<uint32>(trackIndex));
                }
                else
                {
                    timeTrack = new TimeTrack(this);
                    AddTrack(timeTrack);
                }

                timeTrack->SetName(eventTrack->GetName());
                timeTrack->SetIsEnabled(eventTrack->GetIsEnabled());
                timeTrack->SetIsVisible(true);
                timeTrack->SetIsDeletable(eventTrack->GetIsDeletable());

                // get the number of motion events and iterate through them
                size_t eventIndex;
                const size_t numMotionEvents = eventTrack->GetNumEvents();
                if (numMotionEvents == 0)
                {
                    timeTrack->RemoveAllElements();
                }
                else
                {
                    for (eventIndex = 0; eventIndex < numMotionEvents; ++eventIndex)
                    {
                        const EMotionFX::MotionEvent& motionEvent = eventTrack->GetEvent(eventIndex);

                        TimeTrackElement* element = nullptr;
                        if (eventIndex < timeTrack->GetNumElements())
                        {
                            element = timeTrack->GetElement(static_cast<uint32>(eventIndex));
                        }
                        else
                        {
                            element = new TimeTrackElement("", timeTrack);
                            timeTrack->AddElement(element);
                        }

                        text = '{';
                        AZStd::string delimiter;
                        const EMotionFX::EventDataSet& eventDatas = motionEvent.GetEventDatas();
                        for (const EMotionFX::EventDataPtr& data : eventDatas)
                        {
                            if (data)
                            {
                                text += delimiter + data->ToString() + "}";
                            }
                            else
                            {
                                text += "<null>}";
                            }
                            delimiter = ", {";
                        }
                        uint32 color = GetEventPresetManager()->GetEventColor(motionEvent.GetEventDatas());
                        QColor qColor = QColor(MCore::ExtractRed(color), MCore::ExtractGreen(color), MCore::ExtractBlue(color));

                        element->SetIsVisible(true);
                        element->SetName(text.c_str());
                        element->SetColor(qColor);
                        element->SetElementNumber(static_cast<uint32>(eventIndex));
                        element->SetStartTime(motionEvent.GetStartTime());
                        element->SetEndTime(motionEvent.GetEndTime());

                        // tooltip
                        AZStd::string tooltip;
                        AZStd::string rowName;
                        tooltip.reserve(16384);

                        tooltip = "<table border=\"0\">";

                        if (motionEvent.GetIsTickEvent())
                        {
                            // time
                            rowName = "Time";
                            tooltip += AZStd::string::format("<tr><td><p style=\"color:rgb(%i,%i,%i)\"><b>%s:&nbsp;</b></p></td>", qColor.red(), qColor.green(), qColor.blue(), rowName.c_str());
                            tooltip += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%.3f s</p></td></tr>", motionEvent.GetStartTime());
                        }
                        else // range event
                        {
                            // start time
                            rowName = "Start&nbsp;Time";
                            tooltip += AZStd::string::format("<tr><td><p style=\"color:rgb(%i,%i,%i)\"><b>%s:&nbsp;</b></p></td>", qColor.red(), qColor.green(), qColor.blue(), rowName.c_str());
                            tooltip += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%.3f s</p></td></tr>", motionEvent.GetStartTime());

                            // end time
                            rowName = "End&nbsp;Time";
                            tooltip += AZStd::string::format("<tr><td><p style=\"color:rgb(%i,%i,%i)\"><b>%s:&nbsp;</b></p></td>", qColor.red(), qColor.green(), qColor.blue(), rowName.c_str());
                            tooltip += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%.3f s</p></td></tr>", motionEvent.GetEndTime());
                        }

                        for (const EMotionFX::EventDataPtr& eventData : eventDatas)
                        {
                            if (!eventData.get())
                            {
                                continue;
                            }
                            const auto& motionDataProperties = MCore::ReflectionSerializer::SerializeIntoMap(eventData.get());
                            if (motionDataProperties.IsSuccess())
                            {
                                for (const AZStd::pair<AZStd::string, AZStd::string>& keyValuePair : motionDataProperties.GetValue())
                                {
                                    tooltip += AZStd::string::format(
                                            "<tr><td><p style=\"color:rgb(%i,%i,%i)\"><b>%s:&nbsp;</b></p></td>" // no comma, concat these 2 string literals
                                            "<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>",
                                            qColor.red(),
                                            qColor.green(),
                                            qColor.blue(),
                                            keyValuePair.first.c_str(),
                                            keyValuePair.second.c_str()
                                            );
                                }
                            }
                        }

                        tooltip += "</table>";

                        // set tooltip
                        element->SetToolTip(tooltip.c_str());
                    }
                }
                timeTrack->SetElementCount(numMotionEvents);
            }

            for (trackIndex = numEventTracks; trackIndex < GetNumTracks(); ++trackIndex)
            {
                TimeTrack* timeTrack = GetTrack(static_cast<uint32>(trackIndex));
                timeTrack->SetIsVisible(false);
            }
        }
        else // mMotion == nullptr
        {
            const uint32 numEventTracks = GetNumTracks();
            for (uint32 trackIndex = 0; trackIndex < numEventTracks; ++trackIndex)
            {
                TimeTrack* timeTrack = GetTrack(trackIndex);
                timeTrack->SetIsVisible(false);

                const uint32 numMotionEvents = timeTrack->GetNumElements();
                for (uint32 j = 0; j < numMotionEvents; ++j)
                {
                    TimeTrackElement* element = timeTrack->GetElement(j);
                    element->SetIsVisible(false);
                }
            }
        }

        // update the time view plugin
        mTrackHeaderWidget->ReInit();

        if (mMotion)
        {
            //animationLength = mMotion->GetMaxTime();
            MotionInfo* motionInfo = FindMotionInfo(mMotion->GetID());

            // if we already selected before, set the remembered settings
            if (motionInfo->mInitialized)
            {
                const int32 tempScroll = motionInfo->mScrollX;
                SetScale(motionInfo->mScale);
                SetScrollX(tempScroll);
            }
            else
            {
                // selected the animation the first time
                motionInfo->mInitialized = true;
                mTargetTimeScale        = CalcFitScale(mMinScale, mMaxScale) * 0.8;
                motionInfo->mScale      = mTargetTimeScale;
                motionInfo->mScrollX    = 0.0;
            }
        }
    }


    // find the motion info for the given motion id
    TimeViewPlugin::MotionInfo* TimeViewPlugin::FindMotionInfo(uint32 motionID)
    {
        const uint32 numMotionInfos = mMotionInfos.GetLength();
        for (uint32 i = 0; i < numMotionInfos; ++i)
        {
            MotionInfo* motionInfo = mMotionInfos[i];

            if (motionInfo->mMotionID == motionID)
            {
                return motionInfo;
            }
        }

        // we haven't found a motion info for the given id yet, so create a new one
        MotionInfo* motionInfo  = new MotionInfo();
        motionInfo->mMotionID   = motionID;
        motionInfo->mInitialized = false;
        mMotionInfos.Add(motionInfo);
        return motionInfo;
    }


    void TimeViewPlugin::Select(const MCore::Array<EventSelectionItem>& selection)
    {
        uint32 i;

        mSelectedEvents = selection;

        // get the number of tracks in the time view and iterate through them
        const uint32 numTracks = GetNumTracks();
        for (i = 0; i < numTracks; ++i)
        {
            TimeTrack* track = GetTrack(i);

            // get the number of elements in the track and iterate through them
            const uint32 numTrackElements = track->GetNumElements();
            for (uint32 j = 0; j < numTrackElements; ++j)
            {
                TimeTrackElement* element = track->GetElement(j);
                element->SetIsSelected(false);
            }
        }

        const uint32 numSelectedEvents = selection.GetLength();
        for (i = 0; i < numSelectedEvents; ++i)
        {
            const EventSelectionItem*   selectionItem   = &selection[i];
            TimeTrack*                  track           = GetTrack(static_cast<uint32>(selectionItem->mTrackNr));
            TimeTrackElement*           element         = track->GetElement(selectionItem->mEventNr);

            element->SetIsSelected(true);
        }
    }



    EMotionFX::MotionEvent* EventSelectionItem::GetMotionEvent()
    {
        EMotionFX::MotionEventTrack* eventTrack = GetEventTrack();
        if (eventTrack == nullptr)
        {
            return nullptr;
        }

        if (mEventNr >= eventTrack->GetNumEvents())
        {
            return nullptr;
        }

        return &(eventTrack->GetEvent(mEventNr));
    }


    EMotionFX::MotionEventTrack* EventSelectionItem::GetEventTrack()
    {
        if (mTrackNr >= mMotion->GetEventTable()->GetNumTracks())
        {
            return nullptr;
        }

        EMotionFX::MotionEventTrack* eventTrack = mMotion->GetEventTable()->GetTrack(mTrackNr);
        return eventTrack;
    }



    void TimeViewPlugin::AddMotionEvent(int32 x, int32 y)
    {
        if (mMotion == nullptr)
        {
            return;
        }

        SetRedrawFlag();

        // calculate the start time for the motion event
        double dropTimeInSeconds = PixelToTime(x);
        //CalcTime( x, &dropTimeInSeconds, nullptr, nullptr, nullptr, nullptr );

        // get the time track on which we dropped the preset
        TimeTrack* timeTrack = GetTrackAt(y);
        if (timeTrack == nullptr)
        {
            return;
        }

        CommandSystem::CommandHelperAddMotionEvent(timeTrack->GetName(), dropTimeInSeconds, dropTimeInSeconds);
    }


    void TimeViewPlugin::RemoveMotionEvent(int32 x, int32 y)
    {
        // get the time track on which we dropped the preset
        TimeTrack* timeTrack = GetTrackAt(y);
        if (timeTrack == nullptr)
        {
            return;
        }

        // get the time track on which we dropped the preset
        TimeTrackElement* element = GetElementAt(x, y);
        if (element == nullptr)
        {
            return;
        }

        CommandSystem::CommandHelperRemoveMotionEvent(timeTrack->GetName(), element->GetElementNumber());
    }


    void TimeViewPlugin::MotionEventChanged(TimeTrackElement* element, double startTime, double endTime)
    {
        if (element == nullptr)
        {
            return;
        }

        // get the motion event number by getting the time track element number
        uint32 motionEventNr = element->GetElementNumber();
        if (motionEventNr == MCORE_INVALIDINDEX32)
        {
            return;
        }

        // find the corresponding time track for the element
        TimeTrack* timeTrack = FindTrackByElement(element);
        if (timeTrack == nullptr)
        {
            return;
        }

        // get the corresponding motion event track
        EMotionFX::MotionEventTable* eventTable = mMotion->GetEventTable();
        EMotionFX::MotionEventTrack* eventTrack = eventTable->FindTrackByName(timeTrack->GetName());
        if (eventTrack == nullptr)
        {
            return;
        }

        // check if the motion event number is valid and return in case it is not
        if (motionEventNr >= eventTrack->GetNumEvents())
        {
            return;
        }

        // adjust the motion event
        AZStd::string outResult, command;
        command = AZStd::string::format("AdjustMotionEvent -motionID %i -eventTrackName \"%s\" -eventNr %i -startTime %f -endTime %f", mMotion->GetID(), eventTrack->GetName(), motionEventNr, startTime, endTime);
        if (EMStudio::GetCommandManager()->ExecuteCommand(command.c_str(), outResult) == false)
        {
            MCore::LogError(outResult.c_str());
        }
    }


    void TimeViewPlugin::RemoveSelectedMotionEvents()
    {
        // create the command group
        AZStd::string result;
        MCore::CommandGroup commandGroup("Remove motion events");

        if (mMotion == nullptr)
        {
            return;
        }

        if (EMotionFX::GetMotionManager().FindMotionIndex(mMotion) == MCORE_INVALIDINDEX32)
        {
            return;
        }

        // get the motion event table
        //  MotionEventTable& eventTable = mMotion->GetEventTable();
        MCore::Array<uint32> eventNumbers;

        // get the number of tracks in the time view and iterate through them
        const uint32 numTracks = GetNumTracks();
        for (uint32 i = 0; i < numTracks; ++i)
        {
            // get the current time view track
            TimeTrack* track = GetTrack(i);
            if (track->GetIsVisible() == false)
            {
                continue;
            }

            eventNumbers.Clear(false);

            // get the number of elements in the track and iterate through them
            const uint32 numTrackElements = track->GetNumElements();
            for (uint32 j = 0; j < numTrackElements; ++j)
            {
                TimeTrackElement* element = track->GetElement(j);

                if (element->GetIsSelected() && element->GetIsVisible())
                {
                    eventNumbers.Add(j);
                }
            }

            // remove selected motion events
            CommandSystem::CommandHelperRemoveMotionEvents(track->GetName(), eventNumbers, &commandGroup);
        }

        // execute the command group
        if (EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result) == false)
        {
            MCore::LogError(result.c_str());
        }

        UnselectAllElements();
    }


    void TimeViewPlugin::RemoveAllMotionEvents()
    {
        // create the command group
        AZStd::string result;
        MCore::CommandGroup commandGroup("Remove motion events");

        if (mMotion == nullptr)
        {
            return;
        }

        if (EMotionFX::GetMotionManager().FindMotionIndex(mMotion) == MCORE_INVALIDINDEX32)
        {
            return;
        }

        // get the motion event table
        //  MotionEventTable& eventTable = mMotion->GetEventTable();
        MCore::Array<uint32> eventNumbers;

        // get the number of tracks in the time view and iterate through them
        const uint32 numTracks = GetNumTracks();
        for (uint32 i = 0; i < numTracks; ++i)
        {
            // get the current time view track
            TimeTrack* track = GetTrack(i);
            if (track->GetIsVisible() == false)
            {
                continue;
            }

            eventNumbers.Clear(false);

            // get the number of elements in the track and iterate through them
            const uint32 numTrackElements = track->GetNumElements();
            for (uint32 j = 0; j < numTrackElements; ++j)
            {
                TimeTrackElement* element = track->GetElement(j);
                if (element->GetIsVisible())
                {
                    eventNumbers.Add(j);
                }
            }

            // remove selected motion events
            CommandSystem::CommandHelperRemoveMotionEvents(track->GetName(), eventNumbers, &commandGroup);
        }

        // execute the command group
        if (EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result) == false)
        {
            MCore::LogError(result.c_str());
        }

        UnselectAllElements();
    }


    // get the data times
    void TimeViewPlugin::GetDataTimes(double* outMaxTime, double* outClipStart, double* outClipEnd) const
    {
        if (outMaxTime)
        {
            *outMaxTime     = 0.0;
        }
        if (outClipStart)
        {
            *outClipStart   = 0.0;
        }
        if (outClipEnd)
        {
            *outClipEnd     = 0.0;
        }

        EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();
        if (recorder.GetRecordTime() > MCore::Math::epsilon)
        {
            if (outClipEnd)
            {
                *outClipEnd = recorder.GetRecordTime();
            }
            if (outMaxTime)
            {
                *outMaxTime = recorder.GetRecordTime();
            }
        }
        else
        {
            EMotionFX::Motion* motion = GetMotion();
            if (motion)
            {
                EMotionFX::PlayBackInfo* playbackInfo = motion->GetDefaultPlayBackInfo();
                if (outClipStart)
                {
                    *outClipStart   = playbackInfo->mClipStartTime;
                }
                if (outClipEnd)
                {
                    *outClipEnd     = playbackInfo->mClipEndTime;
                }
                if (outMaxTime)
                {
                    *outMaxTime     = motion->GetMaxTime();
                }
            }
        }
    }


    // zoom to fit
    void TimeViewPlugin::ZoomToFit()
    {
        mTargetScrollX      = 0.0;
        mTargetTimeScale    = CalcFitScale(mMinScale, mMaxScale);
    }


    // calculate the scale needed to fit exactly
    double TimeViewPlugin::CalcFitScale(double minScale, double maxScale) const
    {
        // get the duration
        double maxTime;
        GetDataTimes(&maxTime, nullptr, nullptr);

        // calculate the right scale to fit it
        double scale = 1.0;
        if (maxTime > 0.0)
        {
            double width    = mTrackDataWidget->geometry().width();
            scale   = (width / mPixelsPerSecond) / maxTime;
        }

        if (scale < minScale)
        {
            scale = minScale;
        }

        if (scale > maxScale)
        {
            scale = maxScale;
        }

        return scale;
    }


    // is the given time value visible?
    bool TimeViewPlugin::GetIsTimeVisible(double timeValue) const
    {
        const double pixel = TimeToPixel(timeValue);
        return (pixel >= 0.0 && pixel < mTrackDataWidget->geometry().width());
    }


    // make a given time value visible
    void TimeViewPlugin::MakeTimeVisible(double timeValue, double offsetFactor, bool animate)
    {
        SetRedrawFlag();

        const double pixel = TimeToPixel(timeValue, false);

        // if we need to scroll to the right
        double width = mTrackDataWidget->geometry().width() / mTimeScale;
        mTargetScrollX += (pixel - width) + width * (1.0 - offsetFactor);

        if (mTargetScrollX < 0)
        {
            mTargetScrollX = 0;
        }

        if (animate == false)
        {
            mScrollX = mTargetScrollX;
        }
    }


    // update the maximum height
    void TimeViewPlugin::UpdateMaxHeight()
    {
        mMaxHeight = 0.0;

        // find the selected actor instance
        EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();
        if (recorder.GetRecordTime() > MCore::Math::epsilon)
        {
            EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
            if (actorInstance)
            {
                // find the actor instance data for this actor instance
                const uint32 actorInstanceDataIndex = recorder.FindActorInstanceDataIndex(actorInstance);
                if (actorInstanceDataIndex != MCORE_INVALIDINDEX32)
                {
                    const bool displayNodeActivity  = mTrackHeaderWidget->mNodeActivityCheckBox->isChecked();
                    const bool displayEvents        = mTrackHeaderWidget->mEventsCheckBox->isChecked();
                    const bool displayRelativeGraph = mTrackHeaderWidget->mRelativeGraphCheckBox->isChecked();
                    bool isTop = true;

                    const EMotionFX::Recorder::ActorInstanceData& actorInstanceData = recorder.GetActorInstanceData(actorInstanceDataIndex);
                    if (displayNodeActivity)
                    {
                        mMaxHeight += ((recorder.CalcMaxNodeHistoryTrackIndex(actorInstanceData) + 1) * (mTrackDataWidget->mNodeHistoryItemHeight + 3));
                        isTop = false;
                    }

                    if (displayEvents)
                    {
                        if (isTop == false)
                        {
                            mMaxHeight += 10 + 10;
                        }

                        isTop = false;
                        mMaxHeight += mTrackDataWidget->mEventHistoryTotalHeight;
                    }

                    if (displayRelativeGraph)
                    {
                        if (isTop == false)
                        {
                            mMaxHeight += 10;
                        }

                        isTop = false;
                    }
                }
            }
        }
        else
        {
            if (mMotion)
            {
                const uint32 numTracks = mTracks.GetLength();
                for (uint32 i = 0; i < numTracks; ++i)
                {
                    TimeTrack* track = mTracks[i];
                    if (track->GetIsVisible() == false)
                    {
                        continue;
                    }

                    mMaxHeight += track->GetHeight();
                    mMaxHeight += 1;
                }
            }
        }
    }


    // zoom all
    void TimeViewPlugin::OnZoomAll()
    {
        ZoomToFit();
        //if (mTargetTimeScale < 1.0)
        //mTargetTimeScale = 1.0;
    }


    // goto time zero
    void TimeViewPlugin::OnGotoTimeZero()
    {
        mTargetScrollX = 0;
    }


    // reset timeline
    void TimeViewPlugin::OnResetTimeline()
    {
        mTargetScrollX      = 0;
        mTargetTimeScale    = 1.0;
    }


    // center on current time
    void TimeViewPlugin::OnCenterOnCurTime()
    {
        MakeTimeVisible(mCurTime, 0.5);
    }


    // center on current time
    void TimeViewPlugin::OnShowNodeHistoryNodeInGraph()
    {
        if (mNodeHistoryItem && mActorInstanceData)
        {
            emit DoubleClickedRecorderNodeHistoryItem(mActorInstanceData, mNodeHistoryItem);
        }
    }


    // center on current time
    void TimeViewPlugin::OnClickNodeHistoryNode()
    {
        if (mNodeHistoryItem && mActorInstanceData)
        {
            emit ClickedRecorderNodeHistoryItem(mActorInstanceData, mNodeHistoryItem);
        }
    }


    // zooming on rect
    void TimeViewPlugin::ZoomRect(const QRect& rect)
    {
        mTargetScrollX      = mScrollX + (rect.left() / mTimeScale);
        mTargetTimeScale    = mTrackDataWidget->geometry().width() / (double)(rect.width() / mTimeScale);

        if (mTargetTimeScale < 1.0)
        {
            mTargetTimeScale = 1.0;
        }

        if (mTargetTimeScale > mMaxScale)
        {
            mTargetTimeScale = mMaxScale;
        }
    }


    // callbacks
    bool ReInitTimeViewPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(TimeViewPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        TimeViewPlugin* timeViewPlugin = (TimeViewPlugin*)plugin;
        timeViewPlugin->ReInit();

        return true;
    }


    bool MotionSelectionChangedTimeViewPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(TimeViewPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        TimeViewPlugin* timeViewPlugin = (TimeViewPlugin*)plugin;
        timeViewPlugin->MotionSelectionChanged();

        return true;
    }


    bool TimeViewPlugin::CommandAdjustMotionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)       { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitTimeViewPlugin(); }
    bool TimeViewPlugin::CommandAdjustMotionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)          { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitTimeViewPlugin(); }
    bool TimeViewPlugin::CommandSelectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasMotionSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return MotionSelectionChangedTimeViewPlugin();
    }
    bool TimeViewPlugin::CommandSelectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasMotionSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return MotionSelectionChangedTimeViewPlugin();
    }
    bool TimeViewPlugin::CommandUnselectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasMotionSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return MotionSelectionChangedTimeViewPlugin();
    }
    bool TimeViewPlugin::CommandUnselectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasMotionSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return MotionSelectionChangedTimeViewPlugin();
    }
    bool TimeViewPlugin::CommandClearSelectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)     { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return MotionSelectionChangedTimeViewPlugin(); }
    bool TimeViewPlugin::CommandClearSelectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)        { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return MotionSelectionChangedTimeViewPlugin(); }

    bool TimeViewPlugin::CommandRecorderClearCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)     { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitTimeViewPlugin(); }
    bool TimeViewPlugin::CommandRecorderClearCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)        { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitTimeViewPlugin(); }

    // calculate the content heights
    uint32 TimeViewPlugin::CalcContentHeight() const
    {
        const bool displayNodeActivity  = mTrackHeaderWidget->mNodeActivityCheckBox->isChecked();
        const bool displayEvents        = mTrackHeaderWidget->mEventsCheckBox->isChecked();
        const bool displayRelativeGraph = mTrackHeaderWidget->mRelativeGraphCheckBox->isChecked();

        uint32 result = 0;
        if (displayNodeActivity)
        {
            result += mTrackDataWidget->mNodeHistoryRect.bottom();
        }

        if (displayEvents)
        {
            result += mTrackDataWidget->mEventHistoryTotalHeight;
        }

        if (displayRelativeGraph)
        {
        }

        //  MCore::LogInfo("maxHeight = %d", result);
        return result;
    }


    void TimeViewPlugin::OnManualTimeChange(float timeValue)
    {
        MCORE_UNUSED(timeValue);
        GetMainWindow()->OnUpdateRenderPlugins();
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeViewPlugin.moc>
