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

#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/UnicodeString.h>
#include <MCore/Source/Array.h>
#include "../StandardPluginsConfig.h"
#include <EMotionFX/CommandSystem/Source/MotionEventCommands.h>
#include <EMotionFX/Source/Recorder.h>
#include "TimeTrack.h"
#include <QOpenGLWidget>
#include <QPen>
#include <QFont>
#include <QOpenGLFunctions>

QT_FORWARD_DECLARE_CLASS(QBrush)


namespace EMStudio
{
    class TimeViewPlugin;

    class TrackDataWidget
        : public QOpenGLWidget
        , public QOpenGLFunctions
    {
        MCORE_MEMORYOBJECTCATEGORY(TrackDataWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);
        Q_OBJECT

        friend class TimeViewPlugin;
        friend class TrackHeaderWidget;

    public:
        TrackDataWidget(TimeViewPlugin* plugin, QWidget* parent = nullptr);
        ~TrackDataWidget();

        static void DoWheelEvent(QWheelEvent* event, TimeViewPlugin* plugin);
        static void DoMouseYMoveZoom(int32 deltaY, TimeViewPlugin* plugin);

        void initializeGL() override;
        void resizeGL(int w, int h) override;
        void paintGL() override;

    protected:
        //void paintEvent(QPaintEvent* event);
        void mouseDoubleClickEvent(QMouseEvent* event);
        void mouseMoveEvent(QMouseEvent* event);
        void mousePressEvent(QMouseEvent* event);
        void mouseReleaseEvent(QMouseEvent* event);
        void dragEnterEvent(QDragEnterEvent* event);
        void dragMoveEvent(QDragMoveEvent* event);
        void dropEvent(QDropEvent* event);
        void keyPressEvent(QKeyEvent* event);
        void keyReleaseEvent(QKeyEvent* event);
        void contextMenuEvent(QContextMenuEvent* event);

    signals:
        void MotionEventPresetsDropped(QPoint position);
        void MotionEventChanged(TimeTrackElement* element, double startTime, double endTime);
        void TrackAdded(TimeTrack* track);
        void SelectionChanged();
        void ElementTrackChanged(uint32 eventNr, float startTime, float endTime, const char* oldTrackName, const char* newTrackName);

    private slots:
        void OnRemoveElement()                          { RemoveMotionEvent(mContextMenuX, mContextMenuY); }
        void OnAddElement()                             { AddMotionEvent(mContextMenuX, mContextMenuY); }
        void OnAddTrack()                               { CommandSystem::CommandAddEventTrack(); }
        void OnCreatePresetEvent();
        void RemoveSelectedMotionEventsInTrack();
        void RemoveAllMotionEventsInTrack();

        void OnCutTrack();
        void OnCopyTrack();
        void OnCutElement();
        void OnCopyElement();
        void OnPaste();
        void OnPasteAtLocation();

        void RemoveMotionEvent(int32 x, int32 y);
        void AddMotionEvent(int32 x, int32 y);

    private:
        void wheelEvent(QWheelEvent* event);
        void DoPaste(bool useLocation);
        void PaintRecorder(QPainter& painter, const QRect& rect);
        void PaintRecorderNodeHistory(QPainter& painter, const QRect& rect, const EMotionFX::Recorder::ActorInstanceData* actorInstanceData);
        void PaintRecorderEventHistory(QPainter& painter, const QRect& rect, const EMotionFX::Recorder::ActorInstanceData* actorInstanceData);
        void PaintMotionTracks(QPainter& painter, const QRect& rect);
        void ShowElementTimeInfo(TimeTrackElement* element);
        void PaintRelativeGraph(QPainter& painter, const QRect& rect, const EMotionFX::Recorder::ActorInstanceData* actorInstanceData);
        uint32 PaintTitle(const QString& text, QPainter& painter, int32 heightOffset, float animationLength);
        uint32 PaintSeparator(QPainter& painter, int32 heightOffset, float animationLength);

        QBrush              mBrushBackground;
        QBrush              mBrushBackgroundClipped;
        QBrush              mBrushBackgroundOutOfRange;
        QPen                mPenTimeHandles;
        TimeViewPlugin*     mPlugin;
        bool                mMouseLeftClicked;
        bool                mMouseMidClicked;
        bool                mMouseRightClicked;
        bool                mDragging;
        bool                mResizing;
        bool                mMotionPaused;
        bool                mRectZooming;
        bool                mIsScrolling;
        int32               mLastLeftClickedX;
        int32               mLastMouseMoveX;
        int32               mLastMouseX;
        int32               mLastMouseMoveY;
        int32               mLastMouseY;
        uint32              mNodeHistoryItemHeight;
        uint32              mEventHistoryTotalHeight;
        bool                mAllowContextMenu;

        TimeTrackElement*   mDraggingElement;
        TimeTrack*          mDragElementTrack;
        TimeTrackElement*   mResizeElement;
        uint32              mResizeID;
        int32               mContextMenuX;
        int32               mContextMenuY;
        uint32              mGraphStartHeight;
        uint32              mEventsStartHeight;
        uint32              mNodeRectsStartHeight;
        double              mOldCurrentTime;

        MCore::Array<EMotionFX::Recorder::ExtractedNodeHistoryItem> mActiveItems;
        MCore::Array<uint32>                                        mTrackRemap;

        QPixmap             mTimeHandleTop;

        // copy and paste
        struct CopyElement
        {
            uint32          mMotionID;
            AZStd::string   mTrackName;
            AZStd::string   mEventType;
            AZStd::string   mEventParameters;
            float           mStartTime;
            float           mEndTime;
        };

        bool GetIsReadyForPaste() const                                                             { return mCopyElements.empty() == false; }
        void FillCopyElements(bool selectedItemsOnly);

        AZStd::vector<CopyElement>      mCopyElements;
        bool                            mCutMode;
        bool                            mPasteAtMousePos;

        QFont           mTimeLineFont;
        QFont           mDataFont;
        QFont           mTitleFont;
        AZStd::string   mTimeString;
        AZStd::string   mTempString;
        QLinearGradient mHeaderGradientActive;
        QLinearGradient mHeaderGradientInactive;
        QLinearGradient mHeaderGradientActiveFocus;
        QLinearGradient mHeaderGradientInactiveFocus;
        QPen            mPenTimeStepLinesActive;
        QPen            mPenMidTimeStepLinesActive;
        QPen            mPenMainTimeStepLinesActive;
        QPen            mPenTimeStepLinesInactive;
        QPen            mPenMidTimeStepLinesInactive;
        QPen            mPenMainTimeStepLinesInactive;
        int32           mTimeLineHeight;

        uint32          mRecordContextFlags;

        // rect selection
        QPoint          mSelectStart;
        QPoint          mSelectEnd;
        bool            mRectSelecting;

        QRect           mNodeHistoryRect;

        void CalcSelectRect(QRect& outRect);
        void SelectElementsInRect(const QRect& rect, bool overwriteCurSelection, bool select, bool toggleMode);

        void RenderTracks(QPainter& painter, uint32 width, uint32 height, double animationLength, double clipStartTime, double clipEndTime);
        void UpdateMouseOverCursor(int32 x, int32 y);
        void DrawTimeLine(QPainter& painter, const QRect& rect);
        void DrawTimeMarker(QPainter& painter, const QRect& rect);

        bool GetIsInsideNodeHistory(int32 y) const                                              { return mNodeHistoryRect.contains(1, y); }
        void DoRecorderContextMenuEvent(QContextMenuEvent* event);

        MCORE_INLINE bool GetIsInsideTimeLine(int32 y)                                          { return (y <= mTimeLineHeight); }
        void UpdateRects();
        EMotionFX::Recorder::NodeHistoryItem* FindNodeHistoryItem(EMotionFX::Recorder::ActorInstanceData* actorInstanceData, int32 x, int32 y);
        EMotionFX::Recorder::EventHistoryItem* FindEventHistoryItem(EMotionFX::Recorder::ActorInstanceData* actorInstanceData, int32 x, int32 y) const;
        EMotionFX::Recorder::ActorInstanceData* FindActorInstanceData() const;
        void BuildToolTipString(EMotionFX::Recorder::NodeHistoryItem* item, AZStd::string& outString);
        void BuildToolTipString(EMotionFX::Recorder::EventHistoryItem* item, AZStd::string& outString);

        bool event(QEvent* event);
    };
}   // namespace EMStudio