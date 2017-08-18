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

// Description : View that draws main debugging output


#ifndef CRYINCLUDE_EDITOR_AI_AIDEBUGGERVIEW_H
#define CRYINCLUDE_EDITOR_AI_AIDEBUGGERVIEW_H
#pragma once

#include <list>
#include <IAISystem.h>

#include <QWidget>
#include <QDialog>

class QMenu;
class QAction;

struct IAIDebugRecord;
struct IAIDebugStream;

namespace Ui
{
    class AIDebuggerViewFindDialog;
}

class AIDebuggerViewFindDialog
    : public QDialog
{
    Q_OBJECT
public:
    AIDebuggerViewFindDialog(QWidget* parent = nullptr);
    virtual ~AIDebuggerViewFindDialog();

signals:
    void findNext(const QString& target);

private:
    void done();

    QScopedPointer<Ui::AIDebuggerViewFindDialog> m_ui;
};

//////////////////////////////////////////////////////////////////////////
//
// The view control for AI Debugger
//
//////////////////////////////////////////////////////////////////////////
class CAIDebuggerView
    : public QWidget
{
    Q_OBJECT
public:
    CAIDebuggerView(QWidget* parent = nullptr);
    virtual ~CAIDebuggerView();

    void UpdateView();

    // Debug option controls
    void    DebugEnableAll();
    void    DebugDisableAll();

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

signals:
    void scrollRequest(const QPoint& position);

protected:

    void Create();

    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event);

    void OnLButtonDown(QMouseEvent* event);
    void OnLButtonUp(QMouseEvent* event);
    void OnMButtonDown(QMouseEvent* event);
    void OnMButtonUp(QMouseEvent* event);
    void OnRButtonUp(QMouseEvent* event);
    void OnSetCursor(const QPoint& position);
    void OnRButtonUp_Timeline(QMouseEvent* event);
    void OnRButtonUp_List(QMouseEvent* event);
    void OnFindReplace(const QString& target);

    ///////////////////////////////
    // Internal members and functions
    ///////////////////////////////

    static const int RULER_HEIGHT = 30;
    static const int LIST_WIDTH = 200;
    static const int DETAILS_WIDTH = 150;
    static const int ITEM_HEIGHT = 20;
    static const int TIME_TICK_SPACING = 40;

    void    UpdateAIDebugger() const;
    void    UpdateViewRects();
    void    DrawRuler(QPainter& dc);
    void    DrawList(QPainter& dc);
    void    DrawTimeline(QPainter& dc);
    void    DrawDetails(QPainter& dc);
    void    DrawStream(QPainter& dc, IAIDebugStream* pStream, int iy, int ih, const QColor& color, int i);
    void    DrawStreamDetail(QPainter& dc, IAIDebugStream* pStream, const QRect& bounds, const QColor& color);
    void    DrawStreamFloat(QPainter& dc, IAIDebugStream* pStream, int iy, int ih, const QColor& color, int i, float vmin, float vmax);
    void    DrawStreamFloatDetail(QPainter& dc, IAIDebugStream* pStream, const QRect& bounds, const QColor& color);

    enum ERulerHit
    {
        RULERHIT_NONE,
        RULERHIT_EMPTY,
        RULERHIT_SEGMENT,
        RULERHIT_START,
        RULERHIT_END,
    };

    enum ECursorHit
    {
        CURSORHIT_NONE,
        CURSORHIT_EMPTY,
        CURSORHIT_CURSOR,
    };

    enum ESeparatorHit
    {
        SEPARATORHIT_NONE,
        SEPARATORHIT_LIST,
        SEPARATORHIT_DETAILS,
    };

    enum EDragType
    {
        DRAG_NONE,
        DRAG_PAN,
        DRAG_RULER_START,
        DRAG_RULER_END,
        DRAG_RULER_SEGMENT,
        DRAG_TIME_CURSOR,
        DRAG_LIST_WIDTH,
        DRAG_DETAILS_WIDTH,
        DRAG_ITEMS,
    };

    struct SItem
    {
        inline SItem(const char* szName, int type)
            : name(szName)
            , type(type)
            , y(0)
            , h(0)
            , bDebugEnabled(false)
            , bSetView(false)
            , debugColor(Qt::white) {}
        inline bool operator<(const SItem& rhs) const
        {
            if (bDebugEnabled && !rhs.bDebugEnabled)
            {
                return true;
            }
            else if (!bDebugEnabled && rhs.bDebugEnabled)
            {
                return false;
            }
            else if (type != rhs.type)
            {
                return (type > rhs.type);
            }
            return name.compare(rhs.name) < 0;
        }
        inline bool operator>(const SItem& rhs) const { return !(*this < rhs); }

        QString name;
        int y;
        int h;
        int type;

        // Options
        bool bDebugEnabled;
        bool bSetView;
        QColor debugColor;
    };

    int     HitTestRuler(const QPoint& pt);
    int     HitTestTimeCursor(const QPoint& pt);
    int     HitTestSeparators(const QPoint& pt);

    int     TimeToClient(float t);
    float   ClientToTime(int x);
    CAIDebuggerView::SItem* ClientToItem(int clientY);

    bool    FindNext(const QString& what, float start, float& t);
    bool    FindInStream(const QString& what, float start, IAIDebugStream* pStream, float& t);

    float   GetRecordStartTime();
    float   GetRecordEndTime();

    void    SetCursorPrevEvent(bool bShiftDown);
    void    SetCursorNextEvent(bool bShiftDown);

    void UpdateVertScrollInfo();

    QString GetLabelAtPoint(const QPoint& point);
    Vec3                GetPosAtPoint(const QPoint& point);

    void    AdjustViewToTimeCursor();

    void UpdateVertScroll();

    void ClearSetViewProperty();
    bool GetSetViewData(const SItem& sItem, Vec3& vPos, Vec3& vDir) const;
    const SItem* FindItemByName(const QString& sName) const;

    bool IsStreamEmpty(const SItem& sItem, int streamType) const;

    typedef std::list<SItem>    TItemList;

    TItemList   m_items;

    int     m_listFullHeight;

    QRect   m_rulerRect;
    QRect   m_listRect;
    QRect   m_timelineRect;
    QRect   m_detailsRect;

    QFont       m_smallFont;

    float       m_timeStart;
    float       m_timeScale;

    QPixmap m_offscreenBitmap;

    EDragType   m_dragType;
    QPoint      m_dragStartPt;
    float           m_dragStartVal;
    float           m_dragStartVal2;

    float           m_timeRulerStart;
    float           m_timeRulerEnd;

    float           m_timeCursor;

    int             m_listWidth;
    int             m_detailsWidth;
    int             m_itemsOffset;

    QString     m_sFocusedItemName;

    QScopedPointer<AIDebuggerViewFindDialog> m_findDialog;

    QMenu* m_timelineContext;
    QAction* m_copyLabel;
    QAction* m_gotoPos;
    QAction* m_copyPos;
    QAction* m_streamSignalReceived;
    QAction* m_streamSignalReceivedAux;
    QAction* m_streamBehaviorSelected;
    QAction* m_streamAttenionTarget;
    QAction* m_streamRegisterStimulus;
    QAction* m_streamGoalPipeSelected;
    QAction* m_streamGoalPipeInserted;
    QAction* m_streamLuaComment;
    QAction* m_streamPersonalLog;
    QAction* m_streamHealth;
    QAction* m_streamHitDamage;
    QAction* m_streamDeath;
    QAction* m_streamPressureGraph;
    QAction* m_streamBookmarks;

    QMenu* m_listContext;
    QAction* m_listOptionDebug;
    QAction* m_listOptionSetView;

    enum ETrackType
    {
        TRACK_TEXT,
        TRACK_FLOAT,
    };

    struct SStreamDesc
    {
        SStreamDesc(const char* na, int t, ETrackType tr, const QColor& c, int h, float vmin = 0.0f, float vmax = 0.0f)
            : name(na)
            , type(t)
            , track(tr)
            , color(c)
            , height(h)
            , vmin(vmin)
            , vmax(vmax) {}
        QString         name;
        int                 type;
        int                 height;
        int                 nameWidth;
        float               vmin, vmax;
        ETrackType  track;
        QColor      color;
    };

    void    UpdateStreamDesc();

    typedef std::vector<SStreamDesc> TStreamVec;

    TStreamVec  m_streams;
};


#endif // CRYINCLUDE_EDITOR_AI_AIDEBUGGERVIEW_H
