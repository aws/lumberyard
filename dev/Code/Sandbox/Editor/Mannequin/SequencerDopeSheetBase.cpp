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

#include "StdAfx.h"
#include "SequencerDopeSheetBase.h"

#include "AnimationContext.h"
#include "SequencerUndo.h"

#include "Clipboard.h"

#include "SequencerTrack.h"
#include "SequencerNode.h"
#include "SequencerSequence.h"
#include "SequencerUtils.h"
#include "SequencerKeyPropertiesDlg.h"
#include "MannequinDialog.h"
#include "FragmentTrack.h"

#include <Editor/Util/FileUtil.h>

#include <ISourceControl.h>

#include <QKeyEvent>
#include <QLinearGradient>
#include <QMimeData>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QScrollBar>
#include <QStaticText>
#include <QToolTip>
#if defined(Q_OS_WIN)
#include <QtWinExtras/QtWin>
#endif

#include "QtUtil.h"

#include <AzQtComponents/Utilities/QtWindowUtilities.h>

#ifdef LoadCursor
#undef LoadCursor
#endif

enum ETVMouseMode
{
    MOUSE_MODE_NONE = 0,
    MOUSE_MODE_SELECT = 1,
    MOUSE_MODE_MOVE,
    MOUSE_MODE_CLONE,
    MOUSE_MODE_DRAGTIME,
    MOUSE_MODE_DRAGSTARTMARKER,
    MOUSE_MODE_DRAGENDMARKER,
    MOUSE_MODE_PASTE_DRAGTIME,
    MOUSE_MODE_SELECT_WITHIN_TIME,
    MOUSE_MODE_PRE_DRAG
};

#define KEY_TEXT_COLOR QColor(255, 255, 255)
#define EDIT_DISABLE_GRAY_COLOR QColor(128, 128, 128)
#define DRAG_MIN_THRESHOLD 3

static const int MARGIN_FOR_MAGNET_SNAPPING = 10;

CSequencerDopeSheetBase::CSequencerDopeSheetBase(QWidget* parent)
    : QWidget(parent)
    , m_scrollBar(new QScrollBar(Qt::Horizontal, this))
    , m_wndPropsOnSpot(nullptr)
{
    setFocusPolicy(Qt::StrongFocus);

    connect(m_scrollBar, &QScrollBar::valueChanged, this, &CSequencerDopeSheetBase::OnHScroll);

    m_prePasteSheetData = NULL;
    m_pSequence = 0;
    //m_wndTrack = NULL;
    m_bkgrBrush = palette().brush(QPalette::Window);
    //m_bkgrBrushEmpty.CreateHatchBrush( HS_BDIAGONAL,GetSysColor(COLOR_3DFACE) );
    m_bkgrBrushEmpty = QBrush(QColor(190, 190, 190));
    m_timeBkgBrush = QBrush(QColor(0xE0, 0xE0, 0xE0));
    m_timeHighlightBrush = QBrush(QColor(0xFF, 0x0, 0x0));
    m_selectedBrush = QBrush(QColor(200, 200, 230));
    //m_visibilityBrush = QBrush(QColor(0,150,255) );
    m_visibilityBrush = QBrush(QColor(120, 120, 255));
    m_selectTrackBrush = QBrush(QColor(100, 190, 255));

    m_mouseMoveStartTimeOffset = 0.0f;
    m_mouseMoveStartTrackOffset = 0;
    m_timeScale = 1;
    m_ticksStep = 10;

    m_bZoomDrag = false;
    m_bMoveDrag = false;

    m_bMouseOverKey = false;

    m_leftOffset = 0;
    m_scrollOffset = QPoint(0, 0);
    m_bAnySelected = 0;
    m_mouseMode = MOUSE_MODE_NONE;
    m_currentTime = 40;
    m_storedTime = m_currentTime;
    m_rcSelect = new QRubberBand(QRubberBand::Rectangle, this);
    m_rcSelect->setVisible(false);
    m_keyTimeOffset = 0;
    m_mouseActionMode = SEQMODE_MOVEKEY;

    m_itemWidth = 1000;
    m_scrollMin = 0;
    m_scrollMax = 1000;

    m_descriptionFont = QFont(QStringLiteral("Verdana"), 8);

    m_bCursorWasInKey = false;
    m_bValidLastPaste = false;
    m_fJustSelected = 0.f;
    m_snappingMode = SEQKEY_SNAP_NONE;
    m_snapFrameTime = 1.0 / 30.0;
    m_bMouseMovedAfterRButtonDown = false;

    m_bEditLock = false;

    m_pLastTrackSelectedOnSpot = NULL;

    m_changeCount = 0;

    m_tickDisplayMode = SEQTICK_INSECONDS;
    ComputeFrameSteps(GetVisibleRange());

    setAcceptDrops(true);
    OnCreate();
}

CSequencerDopeSheetBase::~CSequencerDopeSheetBase()
{
    GetIEditor()->UnregisterNotifyListener(this);

    OnDestroy();
}


// CSequencerKeys message handlers

namespace DopeSheetActions
{
    // probably don't need to namespace this, but wanted to make sure it always works in uber files.

    static QAction* CreateAction(CSequencerDopeSheetBase* parent, const QString& text, const QKeySequence& shortcut, void (CSequencerDopeSheetBase::* slot)(void))
    {
        QAction* action = new QAction(text, parent);
        action->setShortcut(shortcut);
        QObject::connect(action, &QAction::triggered, parent, slot);
        parent->addAction(action);

        return action;
    }
} // namespace DopeSheetActions

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::OnCreate()
{
    m_imageList.push_back(QPixmap(":/FragmentBrowser/Controls/sequencer_keys_00.png"));
    m_imageList.push_back(QPixmap(":/FragmentBrowser/Controls/sequencer_keys_01.png"));
    m_imageList.push_back(QPixmap(":/FragmentBrowser/Controls/sequencer_keys_02.png"));

    for (int i = 0; i < SNumOfBmps; ++i)
    {
        m_imageSeqKeyBody[i] = QPixmap(QString::fromLatin1(":/FragmentBrowser/seq_%1_colour_keys.png").arg(i + 1));
    }

    m_crsLeftRight = Qt::SizeHorCursor;
    m_crsAddKey = CMFCUtils::LoadCursor(IDC_ARROW_ADDKEY);
    m_crsCross = CMFCUtils::LoadCursor(IDC_POINTER_OBJHIT);
    m_crsMoveBlend = CMFCUtils::LoadCursor(IDC_LEFTRIGHT);

    GetIEditor()->RegisterNotifyListener(this);

    setMouseTracking(true);

    //InitializeFlatSB(GetSafeHwnd());

    // make the actions for our keyboard shortcuts
    DopeSheetActions::CreateAction(this, tr("Shift Timeline Right"), Qt::Key_Right, &CSequencerDopeSheetBase::OnArrowRight);
    DopeSheetActions::CreateAction(this, tr("Shift Timeline Left"), Qt::Key_Left, &CSequencerDopeSheetBase::OnArrowLeft);
    DopeSheetActions::CreateAction(this, tr("Delete"), QKeySequence::Delete, &CSequencerDopeSheetBase::OnDelete);
    DopeSheetActions::CreateAction(this, tr("Cut"), QKeySequence::Cut, &CSequencerDopeSheetBase::OnCut);
    DopeSheetActions::CreateAction(this, tr("Copy"), QKeySequence::Copy, &CSequencerDopeSheetBase::OnCopy);
    DopeSheetActions::CreateAction(this, tr("Paste"), QKeySequence::Paste, &CSequencerDopeSheetBase::OnPaste);
    DopeSheetActions::CreateAction(this, tr("Undo"), QKeySequence::Undo, &CSequencerDopeSheetBase::OnUndo);
    DopeSheetActions::CreateAction(this, tr("Redo"), QKeySequence::Redo, &CSequencerDopeSheetBase::OnRedo);
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::OnDestroy()
{
    HideKeyPropertyCtrlOnSpot();
    GetIEditor()->UnregisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::DrawSelectedKeyIndicators(QPainter* painter)
{
    if (m_pSequence == NULL)
    {
        return;
    }

    const QPen prevPen = painter->pen();
    painter->setPen(QColor(255, 255, 0));
    CSequencerUtils::SelectedKeys selectedKeys;
    CSequencerUtils::GetSelectedKeys(m_pSequence, selectedKeys);
    for (int i = 0; i < selectedKeys.keys.size(); ++i)
    {
        CSequencerTrack* pTrack = selectedKeys.keys[i].pTrack;
        int keyIndex = selectedKeys.keys[i].nKey;
        int x = TimeToClient(pTrack->GetKeyTime(keyIndex));
        painter->drawLine(x, m_rcClient.top(), x, m_rcClient.bottom());
    }
    painter->setPen(prevPen);
}



//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::ComputeFrameSteps(const Range& VisRange)
{
    float fNbFrames = fabsf ((VisRange.end - VisRange.start) / m_snapFrameTime);
    float afStepTable [4] = { 1.0f, 0.5f, 0.2f, 0.1f };
    bool bNotDone = true;
    float fFact = 1.0f;
    int nAttempts = 10;
    int i;
    while (bNotDone && --nAttempts > 0)
    {
        bool bLess = true;
        for (i = 0; i < 4; ++i)
        {
            float fFactNbFrames = fNbFrames / (afStepTable[i] * fFact);
            if (fFactNbFrames >= 3 && fFactNbFrames <= 9)
            {
                bNotDone = false;
                break;
            }
            else if (fFactNbFrames < 3)
            {
                bLess = true;
            }
            else
            {
                bLess = false;
            }
        }
        if (bNotDone)
        {
            fFact *= (bLess) ? 0.1 : 10.0f;
        }
    }

    int nBIntermediateTicks = 5;
    m_fFrameLabelStep = fFact * afStepTable[i];
    if (m_fFrameLabelStep <= 1.0f)
    {
        m_fFrameLabelStep = 1.0f;
    }

    if (TimeToClient(m_fFrameLabelStep) - TimeToClient(0) > 1300)
    {
        nBIntermediateTicks = 10;
    }

    while (m_fFrameLabelStep / double(nBIntermediateTicks) < 1.0f && nBIntermediateTicks != 1)
    {
        nBIntermediateTicks = int ( nBIntermediateTicks / 2.0f );
    }

    m_fFrameTickStep = m_fFrameLabelStep * double (m_snapFrameTime) / double(nBIntermediateTicks);
}

void CSequencerDopeSheetBase::DrawTimeLineInFrames(QPainter* painter, const QRect& rc, const QColor& lineCol, const QColor& textCol, double step)
{
    float fFramesPerSec = 1.0f / m_snapFrameTime;
    float fInvFrameLabelStep = 1.0f / m_fFrameLabelStep;
    Range VisRange = GetVisibleRange();

    const Range& timeRange = m_timeRange;

    const QPen ltgray(QColor(90, 90, 90));
    const QPen black(textCol);

    for (double t = TickSnap(timeRange.start); t <= timeRange.end + m_fFrameTickStep; t += m_fFrameTickStep)
    {
        double st = t;
        if (st > timeRange.end)
        {
            st = timeRange.end;
        }
        if (st < VisRange.start)
        {
            continue;
        }
        if (st > VisRange.end)
        {
            break;
        }
        if (st < m_timeRange.start || st > m_timeRange.end)
        {
            continue;
        }
        int x = TimeToClient(st);

        float fFrame = st * fFramesPerSec;
        /*float fPowerOfN = float(pow (double(fN), double(nFramePowerOfN)));
        float fFramePow = fFrame * fPowerOfN;
        float nFramePow = float(RoundFloatToInt(fFramePow));*/

        float fFrameScaled = fFrame * fInvFrameLabelStep;
        if (fabsf(fFrameScaled - RoundFloatToInt(fFrameScaled)) < 0.001f)
        {
            painter->setPen(black);
            painter->drawLine(x, rc.bottom() - 2, x, rc.bottom() - 14);
            painter->drawText(x + 2, rc.top(), QString::number(fFrame, 'g'));
            painter->setPen(ltgray);
        }
        else
        {
            painter->drawLine(x, rc.bottom() - 2, x, rc.bottom() - 6);
        }
    }
}


void CSequencerDopeSheetBase::DrawTimeLineInSeconds(QPainter* painter, const QRect& rc, const QColor& lineCol, const QColor& textCol, double step)
{
    Range VisRange = GetVisibleRange();
    const Range& timeRange = m_timeRange;
    int nNumberTicks = 10;

    const QPen ltgray(QColor(90, 90, 90));
    const QPen black(textCol);

    for (double t = TickSnap(timeRange.start); t <= timeRange.end + step; t += step)
    {
        double st = TickSnap(t);
        if (st > timeRange.end)
        {
            st = timeRange.end;
        }
        if (st < VisRange.start)
        {
            continue;
        }
        if (st > VisRange.end)
        {
            break;
        }
        if (st < m_timeRange.start || st > m_timeRange.end)
        {
            continue;
        }
        int x = TimeToClient(st);

        int k = RoundFloatToInt(st * m_ticksStep);
        if (k % nNumberTicks == 0)
        {
            painter->setPen(black);
            painter->drawLine(x, rc.bottom() - 2, x, rc.bottom() - 14);
            painter->drawText(x + 2, rc.top(), QString::number(st, 'g'));
            painter->setPen(ltgray);
        }
        else
        {
            painter->drawLine(x, rc.bottom() - 2, x, rc.bottom() - 6);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::DrawTimeline(QPainter* painter, const QRect& rcUpdate)
{
    const QColor lineCol(255, 0, 255);
    const QColor textCol(0, 0, 0);

    // Draw vertical line showing current time.
    {
        int x = TimeToClient(m_currentTime);
        if (x > m_rcClient.left() && x < m_rcClient.right())
        {
            const QPen prevPen = painter->pen();
            painter->setPen(lineCol);
            painter->drawLine(x, 0, x, m_rcClient.bottom());
            painter->setPen(prevPen);
        }
    }

    const QRect rc = m_rcTimeline;
    if (!m_rcTimeline.intersects(rcUpdate))
    {
        return;
    }

    QLinearGradient gradient(m_rcTimeline.topLeft(), m_rcTimeline.bottomLeft());
    gradient.setColorAt(0, QColor(250, 250, 250));
    gradient.setColorAt(1, QColor(180, 180, 180));
    painter->fillRect(rc, gradient);

    const QPen prevPen = painter->pen();

    // Draw time ticks every tick step seconds.
    const Range& timeRange = m_timeRange;

    const QPen ltgray(QColor(90, 90, 90));
    painter->setPen(ltgray);

    double step = 1.0 / double(m_ticksStep);
    if (GetTickDisplayMode() == SEQTICK_INFRAMES)
    {
        DrawTimeLineInFrames(painter, rc, lineCol, textCol, step);
    }
    else if (GetTickDisplayMode() == SEQTICK_INSECONDS)
    {
        DrawTimeLineInSeconds(painter, rc, lineCol, textCol, step);
    }
    else
    {
        assert (0);
    }

    // Draw time markers.
    int x;

    x = TimeToClient(m_timeMarked.start);
    painter->drawPixmap(QPoint(x, m_rcTimeline.bottom() - 9), QPixmap(":/Trackview/marker/bmp00016_01.png"));
    x = TimeToClient(m_timeMarked.end);
    painter->drawPixmap(QPoint(x - 7, m_rcTimeline.bottom() - 9), QPixmap(":/Trackview/marker/bmp00016_00.png"));

    const QPen redpen(lineCol);
    painter->setPen(redpen);
    x = TimeToClient(m_currentTime);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(QRect(QPoint(x - 3, rc.top()), QPoint(x + 2, rc.bottom())));

    painter->setPen(redpen);
    painter->drawLine(x, rc.top(), x, rc.bottom());
    //  painter->drawRect( QRect(QPoint(x-3,rc.top()),QPoint(x+3,rc.bottom())) );

    painter->setPen(prevPen);
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::DrawSummary(QPainter* painter, const QRect& rcUpdate)
{
    if (m_pSequence == NULL)
    {
        return;
    }

    const QColor lineCol(0, 0, 0);
    const QColor fillCol(150, 100, 220);

    const QRect rc = m_rcSummary.intersected(rcUpdate);
    if (!m_rcSummary.intersects(rcUpdate))
    {
        return;
    }

    painter->fillRect(rc, fillCol);

    const QPen prevPen = painter->pen();
    const QPen blackPen(lineCol, 3);
    Range timeRange = m_timeRange;

    painter->setPen(blackPen);

    // Draw a short thick line at each place where there is a key in any tracks.
    CSequencerUtils::SelectedKeys selectedKeys;
    CSequencerUtils::GetAllKeys(m_pSequence, selectedKeys);
    for (int i = 0; i < selectedKeys.keys.size(); ++i)
    {
        CSequencerTrack* pTrack = selectedKeys.keys[i].pTrack;
        int keyIndex = selectedKeys.keys[i].nKey;
        int x = TimeToClient(pTrack->GetKeyTime(keyIndex));
        painter->drawLine(x, rc.bottom() - 2, x, rc.top() + 2);
    }

    painter->setPen(prevPen);
}

//////////////////////////////////////////////////////////////////////////
int CSequencerDopeSheetBase::TimeToClient(float time) const
{
    int x = m_leftOffset - m_scrollOffset.x() + time * m_timeScale;
    return x;
}

//////////////////////////////////////////////////////////////////////////
Range CSequencerDopeSheetBase::GetVisibleRange()
{
    Range r;
    r.start = (m_scrollOffset.x() - m_leftOffset) / m_timeScale;
    r.end = r.start + (m_rcClient.width()) / m_timeScale;
    // Intersect range with global time range.
    r = m_timeRange & r;
    return r;
}

//////////////////////////////////////////////////////////////////////////
Range CSequencerDopeSheetBase::GetTimeRange(const QRect& rc)
{
    Range r;
    r.start = (rc.left() - m_leftOffset + m_scrollOffset.x()) / m_timeScale;
    r.end = r.start + (rc.width()) / m_timeScale;

    r.start = TickSnap(r.start);
    r.end = TickSnap(r.end);
    // Intersect range with global time range.
    r = m_timeRange & r;
    return r;
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::DrawTicks(QPainter* painter, const QRect& rc, const Range& timeRange)
{
    // Draw time ticks every tick step seconds.
    const QPen ltgray(QColor(90, 90, 90));
    const QPen prevPen = painter->pen();
    painter->setPen(ltgray);
    Range VisRange = GetVisibleRange();
    int nNumberTicks = 10;
    if (GetTickDisplayMode() == SEQTICK_INFRAMES)
    {
        nNumberTicks = RoundFloatToInt(1.0f / m_snapFrameTime);
    }
    double step = 1.0 / m_ticksStep;
    for (double t = TickSnap(timeRange.start); t <= timeRange.end + step; t += step)
    {
        double st = TickSnap(t);
        if (st > timeRange.end)
        {
            st = timeRange.end;
        }
        if (st < VisRange.start)
        {
            continue;
        }
        if (st > VisRange.end)
        {
            break;
        }
        int x = TimeToClient(st);
        if (x < 0)
        {
            continue;
        }

        int k = RoundFloatToInt(st * m_ticksStep);
        if (k % nNumberTicks == 0)
        {
            painter->setPen(Qt::black);
            painter->drawLine(x, rc.bottom() - 1, x, rc.bottom() - 5);
            painter->setPen(ltgray);
        }
        else
        {
            painter->drawLine(x, rc.bottom() - 1, x, rc.bottom() - 3);
        }
    }
    painter->setPen(prevPen);
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::DrawKeys(CSequencerTrack* track, QPainter* painter, const QRect& rc, const Range& timeRange, EDSRenderFlags renderFlags)
{
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::RedrawItem(int item)
{
    QRect rc;
    if (GetItemRect(item, rc) != -1)
    {
        update(rc);
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::SetTimeRange(float start, float end)
{
    /*
    if (m_timeMarked.start==m_timeRange.start)
        m_timeMarked.start=start;
    if (m_timeMarked.end==m_timeRange.end)
        m_timeMarked.end=end;
    if (m_timeMarked.end>end)
        m_timeMarked.end=end;
        */
    if (m_timeMarked.start < start)
    {
        m_timeMarked.start = start;
    }
    if (m_timeMarked.end > end)
    {
        m_timeMarked.end = end;
    }

    m_realTimeRange.Set(start, end);
    m_timeRange.Set(start, end);
    //SetHorizontalExtent( m_timeRange.Length() *m_timeScale + 2*m_leftOffset );

    SetHorizontalExtent(m_timeRange.start * m_timeScale - m_leftOffset, m_timeRange.end * m_timeScale - m_leftOffset);
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::SetTimeScale(float timeScale, float fAnchorTime)
{
    //m_leftOffset - m_scrollOffset.x + time*m_timeScale
    double fOldOffset = -fAnchorTime * m_timeScale;

    double fOldScale = m_timeScale;
    if (timeScale < 0.001f)
    {
        timeScale = 0.001f;
    }
    if (timeScale > 100000.0f)
    {
        timeScale = 100000.0f;
    }
    m_timeScale = timeScale;
    double fPixelsPerTick;

    int steps = 0;
    if (GetTickDisplayMode() == SEQTICK_INSECONDS)
    {
        m_ticksStep = 10.0;
    }
    else if (GetTickDisplayMode() == SEQTICK_INFRAMES)
    {
        m_ticksStep = 1.0 / m_snapFrameTime;
    }
    else
    {
        assert(0);
    }
    do
    {
        fPixelsPerTick = (1.0 / m_ticksStep) * (double)m_timeScale;
        if (fPixelsPerTick < 6.0)
        {
            //if (m_ticksStep>=10)
            m_ticksStep *= 0.5;
        }
        if (m_ticksStep <= 0.0)
        {
            m_ticksStep = 1.0;
            break;
        }
        steps++;
    }   while (fPixelsPerTick < 6.0 && steps < 100);

    steps = 0;
    do
    {
        fPixelsPerTick = (1.0 / m_ticksStep) * (double)m_timeScale;
        if (fPixelsPerTick >= 12.0)
        {
            m_ticksStep *= 2.0;
        }
        if (m_ticksStep <= 0.0)
        {
            m_ticksStep = 1.0;
            break;
        }
        steps++;
    } while (fPixelsPerTick >= 12.0 && steps < 100);

    //float
    //m_scrollOffset.x*=timeScale/fOldScale;

    float fCurrentOffset = -fAnchorTime * m_timeScale;
    m_scrollOffset.rx() += fOldOffset - fCurrentOffset;

    update();

    //SetHorizontalExtent( m_timeRange.Length()*m_timeScale + 2*m_leftOffset );
    SetHorizontalExtent(m_timeRange.start * m_timeScale - m_leftOffset, m_timeRange.end * m_timeScale);

    ComputeFrameSteps(GetVisibleRange());
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::resizeEvent(QResizeEvent* event)
{
    m_rcClient = rect();

    m_rcTimeline = rect();
    m_rcTimeline.setBottom(m_rcTimeline.top() + gSettings.mannequinSettings.trackSize);
    m_rcSummary = m_rcTimeline;
    m_rcSummary.setTop(m_rcTimeline.bottom());
    m_rcSummary.setBottom(m_rcSummary.top() + 8);

    SetHorizontalExtent(m_scrollMin, m_scrollMax);

    m_scrollBar->setGeometry(0, height() - m_scrollBar->sizeHint().height(), width(), m_scrollBar->sizeHint().height());
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::OnArrow(bool bLeft)
{
    const bool bUseFrames = GetTickDisplayMode() == SEQTICK_INFRAMES;
    const float fOffset = (bLeft ? -1.0f : 1.0f) * (bUseFrames ? m_snapFrameTime * (0.5f + 0.01f) : GetTickTime());
    const float fTime = max(0.0f, TickSnap(GetCurrTime() + fOffset));
    if (CMannequinDialog::GetCurrentInstance()->IsPaneSelected<CFragmentEditorPage*>())
    {
        CMannequinDialog::GetCurrentInstance()->FragmentEditor()->SetTime(fTime);
    }
    else if (CMannequinDialog::GetCurrentInstance()->IsPaneSelected<CPreviewerPage*>())
    {
        CMannequinDialog::GetCurrentInstance()->Previewer()->SetTime(fTime);
    }
    else if (CMannequinDialog::GetCurrentInstance()->IsPaneSelected<CTransitionEditorPage*>())
    {
        CMannequinDialog::GetCurrentInstance()->TransitionEditor()->SetTime(fTime);
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::OnDelete()
{
    if (m_mouseMode == MOUSE_MODE_PASTE_DRAGTIME)
    {
        FinalizePasteKeys();
    }

    DelSelectedKeys(true);
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::OnCopy()
{
    if (m_mouseMode != MOUSE_MODE_PASTE_DRAGTIME)
    {
        CopyKeys();
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::OnCut()
{
    if (m_mouseMode != MOUSE_MODE_PASTE_DRAGTIME)
    {
        // Copying via keyboard shouldn't use mouse offset
        m_mouseMoveStartTimeOffset = 0.0f;
        if (CopyKeys())
        {
            DelSelectedKeys(false);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::OnPaste()
{
    if (m_mouseMode != MOUSE_MODE_PASTE_DRAGTIME)
    {
        // Don't want to pick up a previous drag as a paste!
        m_bUseClipboard = true;

        StartPasteKeys();
        FinalizePasteKeys();
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::OnUndo()
{
    // trigger an undo in the editor
    GetIEditor()->Undo();

    if (m_mouseMode != MOUSE_MODE_PASTE_DRAGTIME)
    {
        // -- undo already handled in IEditorImpl, don't double undo
        // -- just clear out key selection so we don't have stale data in view
        UnselectAllKeys(true);
        update();
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::OnRedo()
{
    // trigger a redo in the editor
    GetIEditor()->Redo();

    if (m_mouseMode != MOUSE_MODE_PASTE_DRAGTIME)
    {
        // -- undo already handled in IEditorImpl, don't double undo
        // -- just clear out key selection so we don't have stale data in view
        UnselectAllKeys(true);
        update();
    }
}

//////////////////////////////////////////////////////////////////////////
static float GetTimelineWheelScaleFactor()
{
    const float defaultScaleFactor = 1.25f;
    return pow(defaultScaleFactor, clamp_tpl(gSettings.mannequinSettings.timelineWheelZoomSpeed, 0.1f, 5.0f));
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::wheelEvent(QWheelEvent* event)
{
    const QPoint pt = event->pos();

    const float fAnchorTime = TimeFromPointUnsnapped(pt);
    const float newTimeScale = m_timeScale * pow(GetTimelineWheelScaleFactor(), float(event->delta()) / WHEEL_DELTA);

    SetTimeScale(newTimeScale, fAnchorTime);
    event->accept();
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::OnHScroll()
{
    int curpos = m_scrollBar->value();
    m_scrollOffset.setX(curpos);
    update();
}


//////////////////////////////////////////////////////////////////////////
double CSequencerDopeSheetBase::GetTickTime() const
{
    if (GetTickDisplayMode() == SEQTICK_INFRAMES)
    {
        return m_fFrameTickStep;
    }
    else
    {
        return 1.0f / m_ticksStep;
    }
}


//////////////////////////////////////////////////////////////////////////
float CSequencerDopeSheetBase::TickSnap(float time) const
{
    // const Range & timeRange = m_timeRange;
    const bool bUseFrames = GetTickDisplayMode() == SEQTICK_INFRAMES;
    const double tickTime = bUseFrames ? m_snapFrameTime : GetTickTime();
    double t = floor((double)time / tickTime + 0.5);
    t *= tickTime;
    return t;
}

//////////////////////////////////////////////////////////////////////////
float CSequencerDopeSheetBase::TimeFromPoint(const QPoint& point) const
{
    int x = point.x() - m_leftOffset + m_scrollOffset.x();
    double t = (double)x / m_timeScale;
    return (float)TickSnap(t);
}

//////////////////////////////////////////////////////////////////////////
float CSequencerDopeSheetBase::TimeFromPointUnsnapped(const QPoint& point) const
{
    int x = point.x() - m_leftOffset + m_scrollOffset.x();
    double t = (double)x / m_timeScale;
    return t;
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::AddItem(const Item& item)
{
    m_tracks.push_back(item);
    update();
}

//////////////////////////////////////////////////////////////////////////
const CSequencerDopeSheetBase::Item& CSequencerDopeSheetBase::GetItem(int item) const
{
    return m_tracks[item];
}

//////////////////////////////////////////////////////////////////////////
CSequencerTrack* CSequencerDopeSheetBase::GetTrack(int item) const
{
    if (item < 0 || item >= GetCount())
    {
        return 0;
    }
    CSequencerTrack* track = m_tracks[item].track;
    return track;
}

//////////////////////////////////////////////////////////////////////////
CSequencerNode* CSequencerDopeSheetBase::GetNode(int item) const
{
    if (item < 0 || item >= GetCount())
    {
        return 0;
    }
    return m_tracks[item].node;
}

//////////////////////////////////////////////////////////////////////////
int CSequencerDopeSheetBase::FirstKeyFromPoint(const QPoint& point, bool exact) const
{
    return -1;
}

//////////////////////////////////////////////////////////////////////////
int CSequencerDopeSheetBase::LastKeyFromPoint(const QPoint& point, bool exact) const
{
    return -1;
}

bool CSequencerDopeSheetBase::IsDragging() const
{
    return (m_mouseMode != MOUSE_MODE_NONE);
}

void CSequencerDopeSheetBase::mousePressEvent(QMouseEvent* event)
{
    m_mouseOverPos = event->pos();

    switch (event->button())
    {
    case Qt::LeftButton:
        OnLButtonDown(event->pos(), event->modifiers());
        break;
    case Qt::RightButton:
        OnRButtonDown(event->pos(), event->modifiers());
        break;
    case Qt::MiddleButton:
        OnMButtonDown(event->pos(), event->modifiers());
        break;
    }
}

void CSequencerDopeSheetBase::mouseReleaseEvent(QMouseEvent* event)
{
    switch (event->button())
    {
    case Qt::LeftButton:
        OnLButtonUp(event->pos(), event->modifiers());
        break;
    case Qt::RightButton:
        OnRButtonUp(event->pos(), event->modifiers());
        break;
    case Qt::MiddleButton:
        OnMButtonUp(event->pos(), event->modifiers());
        break;
    }

    // instantly trigger a mouse move event to make sure the cursor is still set correct
    mouseMoveEvent(event);
}

void CSequencerDopeSheetBase::mouseDoubleClickEvent(QMouseEvent* event)
{
    switch (event->button())
    {
    case Qt::LeftButton:
        OnLButtonDblClk(event->pos(), event->modifiers());
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::OnLButtonDown(const QPoint& point, Qt::KeyboardModifiers modifiers)
{
    HideKeyPropertyCtrlOnSpot();

    m_mouseDownPos = point;

    // -- need to reset our selection box on mouse down, since if we don't move it won't get reset otherwise
    QRect rc(m_mouseDownPos, m_mouseDownPos);
    rc = rc.normalized();
    m_rcSelect->setGeometry(rc);

    // -- update over point here so that they match for dragging (and they should!)

    if (m_rcTimeline.contains(point))
    {
        // Clicked inside timeline.
        m_mouseMode = MOUSE_MODE_DRAGTIME;
        // If mouse over selected key and it can be moved, change cursor to left-right arrows.
        setCursor(m_crsLeftRight);

        SetCurrTime(TimeFromPoint(point));
        return;
    }

    if (m_bEditLock)
    {
        return;
    }

    if (m_mouseMode == MOUSE_MODE_PASTE_DRAGTIME)
    {
        FinalizePasteKeys();
        return;
    }

    // The summary region is used for moving already selected keys.
    if (m_rcSummary.contains(point) && m_bAnySelected)
    {
        /// Move/Clone Key Undo Begin
        GetIEditor()->BeginUndo();

        CSequencerUtils::SelectedKeys selectedKeys;
        CSequencerUtils::GetSelectedKeys(m_pSequence, selectedKeys);
        CSequencerTrack* pTrack = NULL;
        for (int k = 0; k < (int)selectedKeys.keys.size(); ++k)
        {
            const CSequencerUtils::SelectedKey& skey = selectedKeys.keys[k];
            if (pTrack != skey.pTrack)
            {
                CUndo::Record(new CUndoSequencerSequenceModifyObject(skey.pTrack, m_pSequence));
                pTrack = skey.pTrack;
            }
        }

        m_keyTimeOffset = 0;
        m_mouseMode = MOUSE_MODE_MOVE;
        bool canAnyKeyBeMoved = CSequencerUtils::CanAnyKeyBeMoved(selectedKeys);
        if (canAnyKeyBeMoved)
        {
            setCursor(m_crsLeftRight);
        }
        else
        {
            setCursor(m_crsCross);
        }
        return;
    }

    int key = LastKeyFromPoint(point);
    if (key >= 0)
    {
        const int item = ItemFromPoint(point);
        CSequencerTrack* track = GetTrack(item);

        const float t1 = TimeFromPointUnsnapped(QPoint(point.x() - 4, point.y()));
        const float t2 = TimeFromPointUnsnapped(QPoint(point.x() + 4, point.y()));

        int secondarySelKey;
        int secondarySelection = track->FindSecondarySelectionPt(secondarySelKey, t1, t2);
        if (secondarySelection)
        {
            if (track->CanMoveSecondarySelection(secondarySelKey, secondarySelection))
            {
                key = secondarySelKey;
            }
            else
            {
                secondarySelection = 0;
            }
        }

        /// Move/Clone Key Undo Begin
        GetIEditor()->BeginUndo();

        if (!track->IsKeySelected(key) && !(modifiers & Qt::ControlModifier))
        {
            UnselectAllKeys(false);
        }
        m_bAnySelected = true;
        m_fJustSelected = gEnv->pTimer->GetCurrTime();
        m_keyTimeOffset = 0;
        if (secondarySelection)
        {
            // snap the mouse cursor to the key
            const int xPointNew = TimeToClient(track->GetSecondaryTime(key, secondarySelection));
            const QPoint newPoint(xPointNew, point.y());
            m_mouseDownPos = newPoint;
            m_grabOffset = 0;
            if (!gSettings.stylusMode) // there's no point in changing the cursor when we're dealing with absolute pointing device.
            {
                // set the mouse position on-screen
                AzQtComponents::SetCursorPos(mapToGlobal(newPoint));
            }
        }
        else
        {
            m_grabOffset = point.x() - TimeToClient(track->GetKeyTime(key));
        }
        track->SelectKey(key, true);

        int nNodeCount = m_pSequence->GetNodeCount();
        for (int nodeID = 0; nodeID < nNodeCount; ++nodeID)
        {
            CSequencerNode* pAnimNode = m_pSequence->GetNode(nodeID);

            int nTrackCount = pAnimNode->GetTrackCount();
            for (int trackID = 0; trackID < nTrackCount; ++trackID)
            {
                CSequencerTrack* pAnimTrack = pAnimNode->GetTrackByIndex(trackID);
                CUndo::Record(new CUndoSequencerSequenceModifyObject(pAnimTrack, m_pSequence));
            }
        }

        m_secondarySelection = secondarySelection;
        if (secondarySelection)
        {
            m_mouseMode = MOUSE_MODE_MOVE;
            setCursor(m_crsMoveBlend);
        }
        else if (track->CanMoveKey(key))
        {
            if (track->GetParameterType() == SEQUENCER_PARAM_TRANSITIONPROPS)
            {
                m_mouseMode = MOUSE_MODE_MOVE;
            }
            else
            {
                m_mouseMode = MOUSE_MODE_PRE_DRAG;
            }
            setCursor(m_crsLeftRight);
        }
        else
        {
            m_mouseMode = MOUSE_MODE_MOVE;
            setCursor(m_crsCross);
        }

        update();
        SetKeyInfo(track, key);
        return;
    }

    if (m_mouseActionMode == SEQMODE_ADDKEY)
    {
        // Add key here.
        int item = ItemFromPoint(point);
        CSequencerTrack* track = GetTrack(item);
        if (track)
        {
            float keyTime = TimeFromPoint(point);
            if (IsOkToAddKeyHere(track, keyTime))
            {
                RecordTrackUndo(GetItem(item));
                track->CreateKey(keyTime);
                update();
                UpdateAnimation(item);
            }
        }
        return;
    }

    if (modifiers & Qt::ShiftModifier)
    {
        m_mouseMode = MOUSE_MODE_SELECT_WITHIN_TIME;
    }
    else
    {
        m_mouseMode = MOUSE_MODE_SELECT;
    }

    if (m_bAnySelected && !(modifiers & Qt::ControlModifier))
    {
        // First unselect all buttons.
        UnselectAllKeys(true);
        update();
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::OnLButtonUp(const QPoint& point, Qt::KeyboardModifiers modifiers)
{
    if (m_mouseMode == MOUSE_MODE_SELECT)
    {
        bool prevSelected = m_bAnySelected;
        // Check if any key are selected.
        const QRect rcSelect = m_rcSelect->geometry().translated(-m_scrollOffset);
        SelectKeys(rcSelect);
        NotifyKeySelectionUpdate();
        /*
        if (prevSelected == m_bAnySelected)
        Invalidate();
        else
        {
        CDC *dc = GetDC();
        dc->DrawDragRect( CRect(0,0,0,0),CSize(0,0),m_rcSelect,CSize(1,1) );
        ReleaseDC(dc);
        }
        */
        update();
        m_rcSelect->setVisible(false);
    }
    else if (m_mouseMode == MOUSE_MODE_SELECT_WITHIN_TIME)
    {
        const QRect rcSelect = m_rcSelect->geometry().translated(-m_scrollOffset);
        SelectAllKeysWithinTimeFrame(rcSelect);
        NotifyKeySelectionUpdate();
        update();
        m_rcSelect->setVisible(false);
    }
    else if (m_mouseMode == MOUSE_MODE_DRAGTIME)
    {
        unsetCursor();
    }
    else if (m_mouseMode == MOUSE_MODE_PASTE_DRAGTIME)
    {
        FinalizePasteKeys();
    }
    else if (m_mouseMode == MOUSE_MODE_MOVE)
    {
        CSequencerUtils::SelectedKeys selectedKeys;
        CSequencerUtils::GetSelectedKeys(m_pSequence, selectedKeys);
        for (int i = 0; i < (int)selectedKeys.keys.size(); ++i)
        {
            UpdateAnimation(selectedKeys.keys[i].pTrack);
        }
    }

    if (m_pSequence == NULL)
    {
        m_bAnySelected = false;
    }

    if (m_bAnySelected)
    {
        CSequencerTrack* track = 0;
        int key = 0;
        if (FindSingleSelectedKey(track, key))
        {
            SetKeyInfo(track, key);
        }
    }
    m_keyTimeOffset = 0;
    m_secondarySelection = 0;

    //if (GetIEditor()->IsUndoRecording())
    //GetIEditor()->AcceptUndo( "Track Modify" );

    /// Move/Clone Key Undo End
    if (m_mouseMode == MOUSE_MODE_MOVE || m_mouseMode == MOUSE_MODE_CLONE)
    {
        GetIEditor()->AcceptUndo("Move/Clone Keys");
    }

    m_mouseMode = MOUSE_MODE_NONE;
}


//////////////////////////////////////////////////////////////////////////
bool CSequencerDopeSheetBase::IsPointValidForFragmentInPreviewDrop(const QPoint& point, STreeFragmentDataPtr pFragData) const
{
    if (!CMannequinDialog::GetCurrentInstance()->IsPaneSelected<CPreviewerPage*>())
    {
        return false;
    }

    if (nullptr == pFragData)
    {
        return false;
    }

    // inside timeline.
    if (m_rcTimeline.contains(point))
    {
        return false;
    }

    if (m_bEditLock)
    {
        return false;
    }

    int key = LastKeyFromPoint(point, true);
    if (key >= 0)
    {
        return false;
    }

    // Add key here.
    int item = ItemFromPoint(point);
    CSequencerTrack* pTrack = GetTrack(item);
    if (!pTrack)
    {
        return false;
    }

    if (SEQUENCER_PARAM_FRAGMENTID != pTrack->GetParameterType())
    {
        return false;
    }

    float keyTime = TimeFromPoint(point);
    if (!IsOkToAddKeyHere(pTrack, keyTime))
    {
        return false;
    }

    CFragmentTrack* pFragTrack = static_cast<CFragmentTrack*>(pTrack);
    uint32 nScopeID = pFragTrack->GetScopeData().scopeID;

    ActionScopes scopes = CMannequinDialog::GetCurrentInstance()->Contexts()->m_controllerDef->GetScopeMask
        (
            pFragData->fragID,
            pFragData->tagState
        );

    if ((scopes & (1 << nScopeID)) == 0)
    {
        return false;
    }

    return true;
}


//////////////////////////////////////////////////////////////////////////
// Assume IsPointValidForFragmentInPreviewDrop returned true
//////////////////////////////////////////////////////////////////////////
bool CSequencerDopeSheetBase::CreatePointForFragmentInPreviewDrop(const QPoint& point, STreeFragmentDataPtr pFragData)
{
    // Add key here.
    int item = ItemFromPoint(point);
    CSequencerTrack* pTrack = GetTrack(item);
    if (nullptr != pTrack)
    {
        bool keyCreated = false;
        float keyTime = TimeFromPoint(point);
        keyTime = max(keyTime, 0.0f);

        if (IsOkToAddKeyHere(pTrack, keyTime))
        {
            RecordTrackUndo(GetItem(item));
            int keyID = pTrack->CreateKey(keyTime);

            CFragmentKey newKey;
            newKey.time = keyTime;
            newKey.fragmentID = pFragData->fragID;
            newKey.fragOptionIdx = pFragData->option;
            newKey.tagStateFull = pFragData->tagState;
            pTrack->SetKey(keyID, &newKey);

            UnselectAllKeys(false);

            m_bAnySelected = true;
            m_fJustSelected = gEnv->pTimer->GetCurrTime();
            pTrack->SelectKey(keyID, true);

            keyCreated = true;
        }

        if (keyCreated)
        {
            // Find the first track (the preview window global Tag State track)
            CSequencerTrack* pTagTrack = NULL;
            for (int trackItem = 0; trackItem < GetCount(); ++trackItem)
            {
                pTagTrack = GetTrack(trackItem);
                if (nullptr != pTagTrack && pTagTrack->GetParameterType() == SEQUENCER_PARAM_TAGS)
                {
                    break;
                }
                else
                {
                    pTagTrack = nullptr;
                }
            }

            // Add global tag state to the track
            if (nullptr != pTagTrack)
            {
                RecordTrackUndo(GetItem(item));
                int keyID = pTagTrack->CreateKey(keyTime);

                CTagKey newKey;
                newKey.time = keyTime;
                newKey.tagState = pFragData->tagState.globalTags;
                pTagTrack->SetKey(keyID, &newKey);
            }
            update();
            UpdateAnimation(item);
            return true;
        }
    }
    return false;
}


//////////////////////////////////////////////////////////////////////////
bool CSequencerDopeSheetBase::IsPointValidForAnimationInLayerDrop(const QPoint& point, QString animationName) const
{
    if (!CMannequinDialog::GetCurrentInstance()->IsPaneSelected<CFragmentEditorPage*>())
    {
        return false;
    }

    if (animationName.isEmpty())
    {
        return false;
    }

    // inside timeline.
    if (m_rcTimeline.contains(point))
    {
        return false;
    }

    if (m_bEditLock)
    {
        return false;
    }

    int key = FirstKeyFromPoint (point, true);
    if (key >= 0)
    {
        return false;
    }

    // Add key here.
    int item = ItemFromPoint (point);
    CSequencerTrack* pTrack = GetTrack (item);
    if (nullptr == pTrack)
    {
        return false;
    }

    if (SEQUENCER_PARAM_ANIMLAYER != pTrack->GetParameterType())
    {
        return false;
    }

    float keyTime = TimeFromPoint(point);
    if (!IsOkToAddKeyHere (pTrack, keyTime))
    {
        return false;
    }

    CClipTrack* pClipTrack = static_cast<CClipTrack*> (pTrack);

    int nAnimID = pClipTrack->GetContext()->animSet->GetAnimIDByName(animationName.toUtf8().data());
    if (-1 == nAnimID)
    {
        return false;
    }

    return true;
}


//////////////////////////////////////////////////////////////////////////
// Assume IsPointValidForAnimationInLayerDrop returned true
//////////////////////////////////////////////////////////////////////////
bool CSequencerDopeSheetBase::CreatePointForAnimationInLayerDrop(const QPoint& point, QString animationName)
{
    // Add key here.
    int item = ItemFromPoint (point);
    CSequencerTrack* pTrack = GetTrack (item);
    if (nullptr != pTrack)
    {
        float keyTime = TimeFromPoint(point);

        if (IsOkToAddKeyHere(pTrack, keyTime))
        {
            CClipTrack* pClipTrack = static_cast<CClipTrack*> (pTrack);
            IAnimationSet* pAnimationSet = pClipTrack->GetContext()->animSet;

            RecordTrackUndo(GetItem(item));
            int keyID = pTrack->CreateKey(keyTime);

            CClipKey newKey;
            newKey.time = keyTime;

            newKey.SetAnimation(animationName, pAnimationSet);
            pTrack->SetKey(keyID, &newKey);

            UnselectAllKeys(false);

            m_bAnySelected = true;
            m_fJustSelected = gEnv->pTimer->GetCurrTime();
            pTrack->SelectKey(keyID, true);
            UpdateAnimation(pTrack);
            update();
        }
    }
    return false;
}


//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::OnLButtonDblClk(const QPoint& point, Qt::KeyboardModifiers modifiers)
{
    if (m_rcTimeline.contains(point))
    {
        // Clicked inside timeline.
        return;
    }

    if (m_bEditLock)
    {
        return;
    }

    int key = LastKeyFromPoint(point, true);
    if (key >= 0)
    {
        int item = ItemFromPoint(point);
        CSequencerTrack* track = GetTrack(item);
        UnselectAllKeys(false);
        track->SelectKey(key, true);
        m_bAnySelected = true;
        m_keyTimeOffset = 0;
        update();

        SetKeyInfo(track, key, true);

        QPoint p = QCursor::pos();

        bool bKeyChangeInSameTrack
            = m_pLastTrackSelectedOnSpot
                && track == m_pLastTrackSelectedOnSpot;
        m_pLastTrackSelectedOnSpot = track;

        ShowKeyPropertyCtrlOnSpot(p.x(), p.y(), false, bKeyChangeInSameTrack);
        return;
    }

    m_mouseMode = MOUSE_MODE_NONE;
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::OnMButtonDown(const QPoint& point, Qt::KeyboardModifiers modifiers)
{
    m_mouseDownPos = point;
    m_bMoveDrag = true;
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::OnMButtonUp(const QPoint& point, Qt::KeyboardModifiers modifiers)
{
    m_bMoveDrag = false;
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::OnRButtonDown(const QPoint& point, Qt::KeyboardModifiers modifiers)
{
    HideKeyPropertyCtrlOnSpot();

    m_bCursorWasInKey = false;
    m_bMouseMovedAfterRButtonDown = false;

    if (m_rcTimeline.contains(point))
    {
        // Clicked inside timeline.
        // adjust markers.
        int nMarkerStart = TimeToClient(m_timeMarked.start);
        int nMarkerEnd = TimeToClient(m_timeMarked.end);
        if ((abs(point.x() - nMarkerStart)) < (abs(point.x() - nMarkerEnd)))
        {
            SetStartMarker(TimeFromPoint(point));
            m_mouseMode = MOUSE_MODE_DRAGSTARTMARKER;
        }
        else
        {
            SetEndMarker(TimeFromPoint(point));
            m_mouseMode = MOUSE_MODE_DRAGENDMARKER;
        }
        return;
    }

    //int item = ItemFromPoint(point);
    //SetCurSel(item);

    m_mouseDownPos = point;

    if (modifiers & Qt::ShiftModifier)  // alternative zoom
    {
        m_bZoomDrag = true;
        return;
    }

    int key = LastKeyFromPoint(point);
    if (key >= 0)
    {
        m_bCursorWasInKey = true;
        UnselectAllKeys(false);
        int item = ItemFromPoint(point);
        CSequencerTrack* track = GetTrack(item);
        track->SelectKey(key, true);
        CSequencerKey* sequencerKey = track->GetKey(key);
        m_bAnySelected = true;
        m_keyTimeOffset = 0;
        update();
        SetKeyInfo(track, key);

        // Show a little pop-up menu for copy & delete.
        enum
        {
            COPY_KEY = 100, DELETE_KEY, EDIT_KEY_ON_SPOT, EXPLORE_KEY, OPEN_FOR_EDIT, CHK_OR_ADD_SOURCE_CONTROL, CLONE_ON_SPOT_KEY, PASTE_KEY, NEW_KEY
        };
        QMenu menu;
        bool bEnableEditOnSpot = false;
        CSequencerUtils::SelectedKeys selectedKeys;
        CSequencerUtils::GetSelectedKeys(m_pSequence, selectedKeys);
        if (selectedKeys.keys.size() > 0)
        {
            bEnableEditOnSpot = true;
        }
        QAction* action = menu.addAction(tr("Edit On Spot"));
        action->setData(EDIT_KEY_ON_SPOT);
        action->setEnabled(bEnableEditOnSpot);
        menu.addSeparator();
        if (track->GetParameterType() != SEQUENCER_PARAM_TRANSITIONPROPS)
        {// Copy and paste on transition props result in unexpected behavior.
            menu.addAction(tr("Copy"))->setData(COPY_KEY);
            menu.addAction(tr("Paste"))->setData(PASTE_KEY);
        }
        menu.addAction(tr("Clone On Spot"))->setData(CLONE_ON_SPOT_KEY);
        menu.addAction(tr("New"))->setData(NEW_KEY);
        menu.addAction(tr("Delete"))->setData(DELETE_KEY);

        QStringList paths;
        QString relativePath;
        QString fileName;
        QString editableExtension;
        const bool isOnDisk = sequencerKey->IsFileOnDisk();
        const bool isInPak = sequencerKey->IsFileInsidePak();

        // If the key represents a resource with a file, then allow explorer and perforce options
        if (sequencerKey->HasFileRepresentation())
        {
            sequencerKey->GetFilePaths(paths, relativePath, fileName, editableExtension);

            menu.addSeparator();
            menu.addAction(tr("Open for editing"))->setData(OPEN_FOR_EDIT);
            menu.addAction(tr("Show in explorer"))->setData(EXPLORE_KEY);

            if (GetIEditor()->IsSourceControlAvailable())
            {
                menu.addSeparator();
                menu.addAction(tr("Checkout or Add to Perforce"))->setData(CHK_OR_ADD_SOURCE_CONTROL);
            }
        }

        track->InsertKeyMenuOptions(&menu, key);

        const QPoint p = QCursor::pos();

        action = menu.exec(p);
        const int cmd = action ? action->data().toInt() : -1;
        if (cmd == EDIT_KEY_ON_SPOT)
        {
            bool bKeyChangeInSameTrack
                = m_pLastTrackSelectedOnSpot
                    && selectedKeys.keys.size() == 1
                    && selectedKeys.keys[0].pTrack == m_pLastTrackSelectedOnSpot;

            if (selectedKeys.keys.size() == 1)
            {
                m_pLastTrackSelectedOnSpot = selectedKeys.keys[0].pTrack;
            }
            else
            {
                m_pLastTrackSelectedOnSpot = NULL;
            }

            ShowKeyPropertyCtrlOnSpot(p.x(), p.y(), selectedKeys.keys.size() > 1, bKeyChangeInSameTrack);
        }
        else if (cmd == COPY_KEY)
        {
            CopyKeys();
        }
        else if (cmd == PASTE_KEY)
        {
            OnPaste();
        }
        else if (cmd == CLONE_ON_SPOT_KEY)
        {
            CloneSelectedKeys();
        }
        else if (cmd == DELETE_KEY)
        {
            DelSelectedKeys(true);
        }
        else if (cmd == NEW_KEY)
        {
            int item = ItemFromPoint(point);
            CSequencerTrack* track = GetTrack(item);
            CSequencerNode* node = GetNode(item);
            if (track)
            {
                bool keyCreated = false;
                float keyTime = TimeFromPoint(point);

                if (IsOkToAddKeyHere(track, keyTime))
                {
                    RecordTrackUndo(GetItem(item));
                    int keyID = track->CreateKey(keyTime);

                    UnselectAllKeys(false);

                    m_bAnySelected = true;
                    m_fJustSelected = gEnv->pTimer->GetCurrTime();
                    track->SelectKey(keyID, true);

                    keyCreated = true;
                }

                if (keyCreated)
                {
                    UpdateAnimation(item);
                    update();
                }
            }
        }
        else if (cmd == OPEN_FOR_EDIT || cmd == EXPLORE_KEY)
        {
            if (isInPak &&  GetIEditor()->IsSourceControlAvailable())
            {
                EDSSourceControlResponse getLatestRes = TryGetLatestOnFiles(paths, TRUE);
                if (getLatestRes == USER_ABORTED_OPERATION || getLatestRes == FAILED_OPERATION)
                {
                    return;
                }

                EDSSourceControlResponse checkoutRes = TryCheckoutFiles(paths, TRUE);
                if (checkoutRes == USER_ABORTED_OPERATION || checkoutRes == FAILED_OPERATION)
                {
                    return;
                }

                // Attempt to open file/explorer
                if (cmd == OPEN_FOR_EDIT)
                {
                    TryOpenFile(relativePath, fileName, editableExtension);
                }
                else if (cmd == EXPLORE_KEY)
                {
                    CFileUtil::ShowInExplorer((relativePath + fileName + editableExtension).toUtf8().data());
                }

                // Update flags, file will now be on HDD
                sequencerKey->UpdateFlags();
            }
            else if (isOnDisk)
            {
                // Attempt to open file/explorer
                if (cmd == OPEN_FOR_EDIT)
                {
                    TryOpenFile(relativePath, fileName, editableExtension);
                }
                else if (cmd == EXPLORE_KEY)
                {
                    CFileUtil::ShowInExplorer((relativePath + fileName + editableExtension).toUtf8().data());
                }
            }
            else
            {
                QMessageBox::critical(this, tr("Error!"), tr("File does not exist on local HDD, and source control is unavailable to obtain file"));
            }
        }
        else if (cmd == CHK_OR_ADD_SOURCE_CONTROL)
        {
            bool bFilesExist = true;
            std::vector<bool> vExists;
            vExists.reserve(paths.size());
            for (QStringList::const_iterator iter = paths.begin(), itEnd = paths.end(); iter != itEnd; ++iter)
            {
                const bool thisFileExists = CFileUtil::FileExists(*iter);
                vExists.push_back(thisFileExists);
                bFilesExist = (bFilesExist && thisFileExists);
            }

            bool tryToGetLatestFiles = false;
            if (!bFilesExist)
            {
                const QMessageBox::StandardButton mbResult = QMessageBox::question(this, QString(), tr("Some files could not be found, do you want to attempt to get the latest version from Perforce?"));
                tryToGetLatestFiles = (QMessageBox::Yes == mbResult);
                if (QMessageBox::No == mbResult)
                {
                    return;
                }
            }

            if (GetIEditor()->IsSourceControlAvailable())
            {
                QString outMsg;
                bool overall_vcs_success = true;
                for (int idx = 0, idxEnd = vExists.size(); idx < idxEnd; ++idx)
                {
                    bool vcs_success = false;
                    const QString& currentPath = paths[idx];
                    if (vExists[idx])
                    {
                        vcs_success = AddOrCheckoutFile(QtUtil::ToString(currentPath));
                    }
                    else if (tryToGetLatestFiles)
                    {
                        const AZStd::string currentAbsPath = Path::GetEditingGameDataFolder() + QtUtil::ToString(QDir::separator() + currentPath).c_str();
                        const string gamePath = PathUtil::ToDosPath(currentAbsPath.c_str());
                        const uint32 attr = CFileUtil::GetAttributes(gamePath);
                        if (attr & SCC_FILE_ATTRIBUTE_MANAGED)
                        {
                            if (CFileUtil::GetLatestFromSourceControl(gamePath, this))
                            {
                                vcs_success = CFileUtil::CheckoutFile(gamePath, this);
                            }
                        }
                    }

                    overall_vcs_success = (overall_vcs_success && vcs_success);

                    if (vcs_success)
                    {
                        outMsg += tr("The %1 file was successfully added/checked out.\n").arg(PathUtil::GetExt(QtUtil::ToString(currentPath)));
                    }
                    else
                    {
                        outMsg += tr("The %1 file could NOT be added/checked out.\n").arg(PathUtil::GetExt(QtUtil::ToString(currentPath)));
                    }
                }

                if (!overall_vcs_success)
                {
                    outMsg += tr("\nAt least one file failed to be added or checked out.\nCheck that the file exists on the disk, or if already in Perforce then is not exclusively checked out by another user.");
                }

                QMessageBox::information(this, QString(), outMsg);
            }
        }
        else
        {
            track->OnKeyMenuOption(cmd, key);
        }
    }
}

bool CSequencerDopeSheetBase::AddOrCheckoutFile(const string& filename)
{
    return CFileUtil::CheckoutFile(filename, this);
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::TryOpenFile(const QString& relativePath, const QString& fileName, const QString& extension) const
{
    const QString filePath = relativePath + fileName + extension;
    const bool bFileExists = CFileUtil::FileExists(filePath);

    if (bFileExists)
    {
        CFileUtil::EditFile(QtUtil::ToString(filePath), FALSE, TRUE);
    }
    else
    {
        CFileUtil::ShowInExplorer(filePath.toUtf8().data());
    }
}

//////////////////////////////////////////////////////////////////////////
CSequencerDopeSheetBase::EDSSourceControlResponse CSequencerDopeSheetBase::TryGetLatestOnFiles(const QStringList& paths, bool bPromptUser)
{
    ISourceControl* pSourceControl = GetIEditor()->GetSourceControl();
    if (pSourceControl == NULL)
    {
        return SOURCE_CONTROL_UNAVAILABLE;
    }

    // File states
    bool bAllFilesExist = true;
    bool bAllFilesAreUpToDate = true;
    std::vector<bool> vExists;
    std::vector<bool> vNeedsUpdate;
    vExists.reserve(paths.size());
    vNeedsUpdate.reserve(paths.size());

    // Check state of all files
    for (QStringList::const_iterator iter = paths.begin(), itEnd = paths.end(); iter != itEnd; ++iter)
    {
        const QString workspacePath = Path::GamePathToFullPath((*iter).toUtf8().constData());
        uint32 fileAttributes = CFileUtil::GetAttributes(workspacePath.toUtf8().constData());

        const bool fileExistsOnDisk = fileAttributes & SCC_FILE_ATTRIBUTE_NORMAL;
        const bool fileNeedsUpdate = (fileAttributes & SCC_FILE_ATTRIBUTE_MANAGED) && (fileAttributes & SCC_FILE_ATTRIBUTE_NOTATHEAD);

        vExists.push_back(fileExistsOnDisk);
        vNeedsUpdate.push_back(fileNeedsUpdate);

        bAllFilesExist = bAllFilesExist && fileExistsOnDisk;
        bAllFilesAreUpToDate = bAllFilesAreUpToDate && !fileNeedsUpdate;
    }

    // User input
    bool bGetLatest = false;

    // If some files need updating, prompt user
    if (bPromptUser && (!bAllFilesAreUpToDate || !bAllFilesExist))
    {
        const QMessageBox::StandardButton nResult = QMessageBox::warning(this, tr("Out of date files!"), tr("Some files are not up to date, do you want to update them?"), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        bGetLatest = nResult == QMessageBox::Yes;

        if (nResult == QMessageBox::Cancel)
        {
            // Abort, user wants to cancel operation
            return USER_ABORTED_OPERATION;
        }
    }

    // If the user wants to get latest
    if (bGetLatest)
    {
        QStringList vFailures;

        for (QStringList::const_iterator iter = paths.begin(), itEnd = paths.end(); iter != itEnd; ++iter)
        {
            const QString workspacePath = Path::GamePathToFullPath((*iter).toUtf8().constData());
            if (!CFileUtil::GetLatestFromSourceControl(workspacePath.toUtf8().constData(), this))
            {
                vFailures.push_back(*iter);
            }
        }

        if (vFailures.size() > 0)
        {
            QString errorMsg = tr("Failed to get latest on the following files:");
            for (QStringList::const_iterator iter = vFailures.begin(), itEnd = vFailures.end(); iter != itEnd; ++iter)
            {
                errorMsg += QStringLiteral("\n");
                errorMsg += *iter;
            }
            errorMsg += tr("\nDo you wish to continue?");

            const QMessageBox::StandardButton nResult = QMessageBox::warning(this, tr("Warning!"), errorMsg, QMessageBox::Yes | QMessageBox::No);

            if (nResult == QMessageBox::No)
            {
                return FAILED_OPERATION;
            }
        }
    }

    return SUCCEEDED_OPERATION;
}

//////////////////////////////////////////////////////////////////////////
// N.B. Does NOT check if latest version - use TryGetLatestOnFiles()
CSequencerDopeSheetBase::EDSSourceControlResponse CSequencerDopeSheetBase::TryCheckoutFiles(const QStringList& paths, bool bPromptUser)
{
    ISourceControl* pSourceControl = GetIEditor()->GetSourceControl();
    if (pSourceControl == NULL)
    {
        return SOURCE_CONTROL_UNAVAILABLE;
    }

    // File states
    bool bAllFilesAreCheckedOut = true;
    std::vector<bool> vCheckedOut;
    vCheckedOut.reserve(paths.size());

    // Check state of all files
    for (QStringList::const_iterator iter = paths.begin(), itEnd = paths.end(); iter != itEnd; ++iter)
    {
        const QString workspacePath = Path::GamePathToFullPath((*iter).toUtf8().constData());
        uint32 fileAttributes = CFileUtil::GetAttributes(workspacePath.toUtf8().constData());

        const bool bThisFileCheckedOut = fileAttributes & SCC_FILE_ATTRIBUTE_CHECKEDOUT;
        vCheckedOut.push_back(bThisFileCheckedOut);
        bAllFilesAreCheckedOut = bAllFilesAreCheckedOut && bThisFileCheckedOut;
    }

    // User input
    bool bCheckOut = false;

    // If files are up to date and not checked out, no checking out out of date files
    if (bPromptUser && !bAllFilesAreCheckedOut)
    {
        const QMessageBox::StandardButton nResult = QMessageBox::question(this, tr("Checkout?"), tr("Do you want to check out these files?"), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (nResult == QMessageBox::Cancel)
        {
            // Abort, user wants to cancel operation
            return USER_ABORTED_OPERATION;
        }
        bCheckOut = nResult == QMessageBox::Yes;
    }

    // If user wants to check out these files
    if (bCheckOut)
    {
        QStringList vFailures;

        for (QStringList::const_iterator iter = paths.begin(), itEnd = paths.end(); iter != itEnd; ++iter)
        {
            const AZStd::string filename((*iter).toUtf8().constData());
            if (!CFileUtil::CheckoutFile(filename.c_str()))
            {
                vFailures.push_back(*iter);
            }
        }

        if (vFailures.size() > 0)
        {
            QString errorMsg = tr("Failed to check out the following files:");
            for (QStringList::const_iterator iter = vFailures.begin(), itEnd = vFailures.end(); iter != itEnd; ++iter)
            {
                errorMsg += QStringLiteral("\n");
                errorMsg += *iter;
            }
            errorMsg += tr("\nDo you wish to continue?");

            const int nResult = QMessageBox::warning(this, tr("Warning!"), errorMsg, QMessageBox::Yes | QMessageBox::No);

            if (nResult == QMessageBox::No)
            {
                return FAILED_OPERATION;
            }
        }
    }

    return SUCCEEDED_OPERATION;
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::OnRButtonUp(const QPoint& point, Qt::KeyboardModifiers modifiers)
{
    m_mouseMode = MOUSE_MODE_NONE;
    m_bZoomDrag = false;

    CClipboard clip(this);
    if (m_bCursorWasInKey == false)
    {
        bool bHasCopiedKey = true;

        if (clip.GetTitle() != "Track view keys")
        {
            bHasCopiedKey = false;
        }
        if (clip.IsEmpty())
        {
            bHasCopiedKey = false;
        }

        {
            XmlNodeRef copyNode = clip.Get();
            if (copyNode == NULL || strcmp(copyNode->getTag(), "CopyKeysNode"))
            {
                bHasCopiedKey = false;
            }
            else
            {
                int nNumTracksToPaste = copyNode->getChildCount();
                if (nNumTracksToPaste == 0)
                {
                    bHasCopiedKey = false;
                }
            }
        }
        if (m_bMouseMovedAfterRButtonDown == false)  // Once moved, it means the user wanted to scroll, so no paste pop-up.
        {
            // Show a little pop-up menu for paste.
            QMenu menu;

            enum
            {
                PASTE_KEY = 100, NEW_KEY
            };
            menu.addAction(tr("New"))->setData(NEW_KEY);
            if (bHasCopiedKey)
            {
                menu.addAction(tr("Paste"))->setData(PASTE_KEY);
            }
            QAction* action = menu.exec(QCursor::pos());
            const int cmd = action ? action->data().toInt() : -1;
            if (cmd == PASTE_KEY)
            {
                OnPaste();
            }
            else if (cmd == NEW_KEY)
            {
                int item = ItemFromPoint(point);
                CSequencerTrack* track = GetTrack(item);
                CSequencerNode* node = GetNode(item);
                if (track)
                {
                    bool keyCreated = false;
                    float keyTime = TimeFromPoint(point);

                    if (IsOkToAddKeyHere(track, keyTime))
                    {
                        RecordTrackUndo(GetItem(item));
                        int keyID = track->CreateKey(keyTime);

                        UnselectAllKeys(false);

                        m_bAnySelected = true;
                        m_fJustSelected = gEnv->pTimer->GetCurrTime();
                        track->SelectKey(keyID, true);

                        keyCreated = true;
                    }

                    if (keyCreated)
                    {
                        UpdateAnimation(item);
                        update();
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::mouseMoveEvent(QMouseEvent* event)
{
    // To prevent the key moving while selecting

    float fCurrentTime = gEnv->pTimer->GetCurrTime();
    float fSelectionThreshold = 0.1f;

    if (fCurrentTime - m_fJustSelected < fSelectionThreshold)
    {
        return;
    }

    m_bMouseMovedAfterRButtonDown = true;

    m_mouseOverPos = event->pos();
    m_bMouseOverKey = false;
    if (m_bZoomDrag && (event->modifiers() & Qt::ShiftModifier))
    {
        float fAnchorTime = TimeFromPointUnsnapped(m_mouseDownPos);
        SetTimeScale(m_timeScale * (1.0f + (event->x() - m_mouseDownPos.x()) * 0.0025f), fAnchorTime);
        m_mouseDownPos = event->pos();
        return;
    }
    else
    {
        m_bZoomDrag = false;
    }
    if (m_bMoveDrag)
    {
        m_scrollOffset.rx() += m_mouseDownPos.x() - event->x();
        if (m_scrollOffset.x() < m_scrollMin)
        {
            m_scrollOffset.rx() = m_scrollMin;
        }
        if (m_scrollOffset.x() > m_scrollMax)
        {
            m_scrollOffset.rx() = m_scrollMax;
        }
        m_mouseDownPos = event->pos();
        // Set the new position of the thumb (scroll box).
        m_scrollBar->setValue(m_scrollOffset.x());
        update();
        setCursor(m_crsLeftRight);
        return;
    }

    if (m_mouseMode == MOUSE_MODE_SELECT
        || m_mouseMode == MOUSE_MODE_SELECT_WITHIN_TIME)
    {
        MouseMoveSelect(event->pos());
    }
    else if (m_mouseMode == MOUSE_MODE_MOVE)
    {
        MouseMoveMove(event->pos(), event->modifiers());
    }
    else if (m_mouseMode == MOUSE_MODE_CLONE)
    {
        CloneSelectedKeys();
        m_mouseMode = MOUSE_MODE_MOVE;
    }
    else if (m_mouseMode == MOUSE_MODE_DRAGTIME)
    {
        MouseMoveDragTime(event->pos(), event->modifiers());
    }
    else if (m_mouseMode == MOUSE_MODE_DRAGSTARTMARKER)
    {
        MouseMoveDragStartMarker(event->pos(), event->modifiers());
    }
    else if (m_mouseMode == MOUSE_MODE_DRAGENDMARKER)
    {
        MouseMoveDragEndMarker(event->pos(), event->modifiers());
    }
    else if (m_mouseMode == MOUSE_MODE_PASTE_DRAGTIME)
    {
        MouseMovePaste(event->pos(), event->modifiers());
    }
    else if (m_mouseMode == MOUSE_MODE_PRE_DRAG)
    {
        if (abs(m_mouseOverPos.x() - m_mouseDownPos.x()) > DRAG_MIN_THRESHOLD ||
            abs(m_mouseOverPos.y() - m_mouseDownPos.y()) > DRAG_MIN_THRESHOLD)
        {
            StartDraggingKeys(m_mouseDownPos);
        }
    }
    else
    {
        //////////////////////////////////////////////////////////////////////////
        if (m_mouseActionMode == SEQMODE_ADDKEY)
        {
            setCursor(m_crsAddKey);
        }
        else
        {
            MouseMoveOver(event->pos());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    QLinearGradient gradient(rect().topLeft(), rect().bottomLeft());
    gradient.setColorAt(0, QColor(250, 250, 250));
    gradient.setColorAt(1, QColor(220, 220, 220));
    painter.fillRect(event->rect(), gradient);

    if (m_bEditLock)
    {
        const QBrush brushGray(QColor(180, 180, 180));
        painter.fillRect(event->rect(), brushGray);
    }

    DrawControl(&painter, event->rect());
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::DrawTrack(int item, QPainter* dc, const QRect& rcItem)
{
}


//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::DrawControl(QPainter* painter, const QRect& rcUpdate)
{
    QRect rc;

    // Draw all items.
    int count = GetCount();
    for (int i = 0; i < count; i++)
    {
        GetItemRect(i, rc);
        //if (rc.intersects(rcUpdate))
        {
            DrawTrack(i, painter, rc);
        }
    }

    DrawTimeline(painter, rcUpdate);

    DrawSummary(painter, rcUpdate);

    DrawSelectedKeyIndicators(painter);
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::UnselectAllKeys(bool bNotify)
{
    CSequencerUtils::SelectedKeys selectedKeys;
    CSequencerUtils::GetSelectedKeys(m_pSequence, selectedKeys);
    for (int i = 0; i < selectedKeys.keys.size(); ++i)
    {
        CSequencerTrack* pTrack = selectedKeys.keys[i].pTrack;
        int keyIndex = selectedKeys.keys[i].nKey;
        pTrack->SelectKey(keyIndex, false);
    }
    m_bAnySelected = false;

    if (bNotify)
    {
        NotifyKeySelectionUpdate();
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::SelectKeys(const QRect& rc)
{}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::SelectAllKeysWithinTimeFrame(const QRect& rc)
{
    if (m_pSequence == NULL)
    {
        return;
    }

    // put selection rectangle from client to item space.
    const QRect rci = rc.translated(m_scrollOffset);

    Range selTime = GetTimeRange(rci);

    for (int k = 0; k < m_pSequence->GetNodeCount(); ++k)
    {
        CSequencerNode* node = m_pSequence->GetNode(k);

        for (int i = 0; i < node->GetTrackCount(); i++)
        {
            CSequencerTrack* track = node->GetTrackByIndex(i);

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
bool CSequencerDopeSheetBase::SelectFirstKey()
{
    return SelectFirstKey(SEQUENCER_PARAM_TOTAL);
}

//////////////////////////////////////////////////////////////////////////
bool CSequencerDopeSheetBase::SelectFirstKey(const ESequencerParamType type)
{
    const bool selectAnyTrackType = (type == SEQUENCER_PARAM_TOTAL);
    UnselectAllKeys(false);

    for (int i = 0; i < GetCount(); ++i)
    {
        CSequencerTrack* pTrack = GetTrack(i);
        if (pTrack)
        {
            const ESequencerParamType trackType = pTrack->GetParameterType();
            const bool isOfRequiredType = (selectAnyTrackType || (trackType == type));
            if (isOfRequiredType)
            {
                const int numKeys = pTrack->GetNumKeys();
                if (0 < numKeys)
                {
                    pTrack->SelectKey(0, true);
                    NotifyKeySelectionUpdate();
                    return true;
                }
            }
        }
    }

    NotifyKeySelectionUpdate();
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::DelSelectedKeys(bool bPrompt, bool bAllowUndo, bool bIgnorePermission)
{
    CSequencerUtils::SelectedKeys selectedKeys;
    CSequencerUtils::GetSelectedKeys(m_pSequence, selectedKeys);

    if (!bIgnorePermission)
    {
        bool keysCanBeDeleted = false;
        for (int k = 0; k < (int)selectedKeys.keys.size(); ++k)
        {
            CSequencerUtils::SelectedKey& skey = selectedKeys.keys[k];
            keysCanBeDeleted |= skey.pTrack->CanRemoveKey(skey.nKey);
        }

        if (!keysCanBeDeleted)
        {
            qApp->beep();
            return;
        }
    }

    // Confirm.
    if (bPrompt)
    {
        if (QMessageBox::question(this, QString(), tr("Delete selected keys?"), QMessageBox::Ok | QMessageBox::Cancel) != QMessageBox::Ok)
        {
            return;
        }
    }

    if (m_pSequence && bAllowUndo)
    {
        CAnimSequenceUndo(m_pSequence, "Delete Keys");
    }

    update();

    for (int k = (int)selectedKeys.keys.size() - 1; k >= 0; --k)
    {
        CSequencerUtils::SelectedKey& skey = selectedKeys.keys[k];
        if (bIgnorePermission || skey.pTrack->CanRemoveKey(skey.nKey))
        {
            skey.pTrack->RemoveKey(skey.nKey);

            UnselectAllKeys(true);
            UpdateAnimation(skey.pTrack);
            update();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::OffsetSelectedKeys(const float timeOffset, const bool bSnap)
{
    CSequencerUtils::SelectedKeys selectedKeys;
    CSequencerUtils::GetSelectedKeys(m_pSequence, selectedKeys);
    for (int k = 0; k < (int)selectedKeys.keys.size(); ++k)
    {
        CSequencerUtils::SelectedKey skey = selectedKeys.keys[k];

        OffsetKey(skey.pTrack, skey.nKey, timeOffset);

        UpdateAnimation(skey.pTrack);
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::ScaleSelectedKeys(float timeOffset, bool bSnapKeys)
{
    if (timeOffset <= 0)
    {
        return;
    }
    CSequencerUtils::SelectedKeys selectedKeys;
    CSequencerUtils::GetSelectedKeys(m_pSequence, selectedKeys);
    for (int k = 0; k < (int)selectedKeys.keys.size(); ++k)
    {
        CSequencerUtils::SelectedKey skey = selectedKeys.keys[k];
        float keyt = skey.pTrack->GetKeyTime(skey.nKey) * timeOffset;
        if (bSnapKeys)
        {
            keyt = TickSnap(keyt);
        }
        skey.pTrack->SetKeyTime(skey.nKey, keyt);
        UpdateAnimation(skey.pTrack);
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::CloneSelectedKeys()
{
    CSequencerUtils::SelectedKeys selectedKeys;
    CSequencerUtils::GetSelectedKeys(m_pSequence, selectedKeys);
    CSequencerTrack* pTrack = 0;
    // In case of multiple cloning, indices cannot be used as a solid pointer to the original.
    // So use the time of keys as an identifier, instead.
    std::vector<float> selectedKeyTimes;
    for (size_t k = 0; k < selectedKeys.keys.size(); ++k)
    {
        CSequencerUtils::SelectedKey skey = selectedKeys.keys[k];
        if (pTrack != skey.pTrack)
        {
            CUndo::Record(new CUndoSequencerSequenceModifyObject(skey.pTrack, m_pSequence));
            pTrack = skey.pTrack;
        }

        selectedKeyTimes.push_back(skey.pTrack->GetKeyTime(skey.nKey));
    }

    // Now, do the actual cloning.
    for (size_t k = 0; k < selectedKeyTimes.size(); ++k)
    {
        CSequencerUtils::SelectedKey skey = selectedKeys.keys[k];

        int keyIndex = skey.pTrack->FindKey(selectedKeyTimes[k]);
        assert(keyIndex > -1);
        if (keyIndex <= -1)
        {
            continue;
        }

        if (skey.pTrack->IsKeySelected(keyIndex))
        {
            // Unselect cloned key.
            skey.pTrack->SelectKey(keyIndex, false);

            int newKey = skey.pTrack->CloneKey(keyIndex);

            // Select new key.
            skey.pTrack->SelectKey(newKey, true);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
float CSequencerDopeSheetBase::GetCurrTime() const
{
    return m_currentTime;
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::SetCurrTime(float time, bool bForce)
{
    if (time < m_timeRange.start)
    {
        time = m_timeRange.start;
    }
    if (time > m_timeRange.end)
    {
        time = m_timeRange.end;
    }

    //bool bChange = fabs(time-m_currentTime) >= (1.0f/m_ticksStep);
    bool bChange = fabs(time - m_currentTime) >= 0.001f;

    if (bChange || bForce)
    {
        int x1 = TimeToClient(m_currentTime);
        int x2 = TimeToClient(time);
        m_currentTime = time;

        const QRect rc(QPoint(x1 - 3, m_rcClient.top()), QPoint(x1 + 4, m_rcClient.bottom()));
        update(rc);
        const QRect rc1(QPoint(x2 - 3, m_rcClient.top()), QPoint(x2 + 4, m_rcClient.bottom()));
        update(rc1);
    }
}

void CSequencerDopeSheetBase::SetStartMarker(float fTime)
{
    m_timeMarked.start = fTime;
    if (m_timeMarked.start < m_timeRange.start)
    {
        m_timeMarked.start = m_timeRange.start;
    }
    if (m_timeMarked.start > m_timeRange.end)
    {
        m_timeMarked.start = m_timeRange.end;
    }
    if (m_timeMarked.start > m_timeMarked.end)
    {
        m_timeMarked.end = m_timeMarked.start;
    }
    update();
}

void CSequencerDopeSheetBase::SetEndMarker(float fTime)
{
    m_timeMarked.end = fTime;
    if (m_timeMarked.end < m_timeRange.start)
    {
        m_timeMarked.end = m_timeRange.start;
    }
    if (m_timeMarked.end > m_timeRange.end)
    {
        m_timeMarked.end = m_timeRange.end;
    }
    if (m_timeMarked.start > m_timeMarked.end)
    {
        m_timeMarked.start = m_timeMarked.end;
    }
    update();
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::SetMouseActionMode(ESequencerActionMode mode)
{
    m_mouseActionMode = mode;
    if (mode == SEQMODE_ADDKEY)
    {
        setCursor(m_crsAddKey);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CSequencerDopeSheetBase::CanCopyPasteKeys()
{
    CSequencerTrack* pCopyFromTrack = NULL;
    // are all selected keys from the same source track ?
    if (!m_bAnySelected)
    {
        return false;
    }
    CSequencerUtils::SelectedKeys selectedKeys;
    CSequencerUtils::GetSelectedKeys(m_pSequence, selectedKeys);
    for (int k = 0; k < (int)selectedKeys.keys.size(); ++k)
    {
        CSequencerUtils::SelectedKey skey = selectedKeys.keys[k];
        if (!pCopyFromTrack)
        {
            pCopyFromTrack = skey.pTrack;
        }
        else
        {
            if (pCopyFromTrack != skey.pTrack)
            {
                return false;
            }
        }
    }
    if (!pCopyFromTrack)
    {
        return false;
    }

    // is a destination-track selected ?
    CSequencerUtils::SelectedTracks selectedTracks;
    CSequencerUtils::GetSelectedTracks(m_pSequence, selectedTracks);
    if (selectedTracks.tracks.size() != 1)
    {
        return false;
    }
    CSequencerTrack* pCurrTrack = selectedTracks.tracks[0].pTrack;
    if (!pCurrTrack)
    {
        return false;
    }
    return (pCopyFromTrack == pCurrTrack);
}

//////////////////////////////////////////////////////////////////////////
bool CSequencerDopeSheetBase::CopyPasteKeys()
{
    CSequencerTrack* pCopyFromTrack = NULL;
    std::vector<int> vecKeysToCopy;
    if (!CanCopyPasteKeys())
    {
        return false;
    }
    if (!m_bAnySelected)
    {
        return false;
    }
    CSequencerUtils::SelectedKeys selectedKeys;
    CSequencerUtils::GetSelectedKeys(m_pSequence, selectedKeys);
    for (int k = 0; k < (int)selectedKeys.keys.size(); ++k)
    {
        CSequencerUtils::SelectedKey skey = selectedKeys.keys[k];
        pCopyFromTrack = skey.pTrack;
        vecKeysToCopy.push_back(skey.nKey);
    }
    if (!pCopyFromTrack)
    {
        return false;
    }

    // Get Selected Track.
    CSequencerTrack* pCurrTrack = 0;
    CSequencerUtils::SelectedTracks selectedTracks;
    CSequencerUtils::GetSelectedTracks(m_pSequence, selectedTracks);
    if (selectedTracks.tracks.size() == 1)
    {
        pCurrTrack = selectedTracks.tracks[0].pTrack;
    }

    if (!pCurrTrack)
    {
        return false;
    }
    for (int i = 0; i < (int)vecKeysToCopy.size(); i++)
    {
        pCurrTrack->CopyKey(pCopyFromTrack, vecKeysToCopy[i]);
    }
    update();
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::SetKeyInfo(CSequencerTrack* track, int key, bool openWindow)
{
    NotifyKeySelectionUpdate();
}

//////////////////////////////////////////////////////////////////////////
bool CSequencerDopeSheetBase::FindSingleSelectedKey(CSequencerTrack*& selTrack, int& selKey)
{
    selTrack = 0;
    selKey = 0;
    CSequencerUtils::SelectedKeys selectedKeys;
    CSequencerUtils::GetSelectedKeys(m_pSequence, selectedKeys);
    if (selectedKeys.keys.size() != 1)
    {
        return false;
    }
    CSequencerUtils::SelectedKey skey = selectedKeys.keys[0];
    selTrack = skey.pTrack;
    selKey = skey.nKey;
    return true;
}

//////////////////////////////////////////////////////////////////////////
int CSequencerDopeSheetBase::GetItemRect(int item, QRect& rect) const
{
    if (item < 0 || item >= GetCount())
    {
        return -1;
    }

    int y = -m_scrollOffset.y();

    int x = 0;
    for (int i = 0, num = (int)m_tracks.size(); i < num && i < item; i++)
    {
        y += m_tracks[i].nHeight;
    }
    rect = QRect(x, y, m_rcClient.width(), m_tracks[item].nHeight);
    return 0;
}

//////////////////////////////////////////////////////////////////////////
int CSequencerDopeSheetBase::ItemFromPoint(const QPoint& pnt) const
{
    QRect rc;
    int num = GetCount();
    for (int i = 0; i < num; i++)
    {
        GetItemRect(i, rc);
        if (rc.contains(pnt))
        {
            return i;
        }
    }
    return -1;
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::SetHorizontalExtent(int min, int max)
{
    m_scrollMin = min;
    m_scrollMax = max;
    m_itemWidth = max - min;

    m_scrollBar->setPageStep(m_rcClient.width() / 2);
    m_scrollBar->setRange(min, max - m_scrollBar->pageStep() * 2 + m_leftOffset);
};

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::UpdateAnimation(int item)
{
    m_changeCount++;

    CSequencerTrack* animTrack = GetTrack(item);
    if (animTrack)
    {
        animTrack->OnChange();
    }
}


//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::UpdateAnimation(CSequencerTrack* animTrack)
{
    m_changeCount++;

    if (animTrack)
    {
        animTrack->OnChange();
    }
}

//////////////////////////////////////////////////////////////////////////
bool CSequencerDopeSheetBase::CopyKeys(bool bPromptAllowed, bool bUseClipboard, bool bCopyTrack)
{
    CSequencerNode* pCurNode = NULL;

    CSequencerUtils::SelectedKeys selectedKeys;
    CSequencerUtils::GetSelectedKeys(m_pSequence, selectedKeys);
    const bool bCopySelected = !bCopyTrack && !selectedKeys.keys.empty();
    bool bIsSeveralEntity = false;
    for (int k = 0; k < (int)selectedKeys.keys.size(); ++k)
    {
        CSequencerUtils::SelectedKey skey = selectedKeys.keys[k];
        if (pCurNode && pCurNode != skey.pNode)
        {
            bIsSeveralEntity = true;
        }
        pCurNode = skey.pNode;
    }

    if (bIsSeveralEntity)
    {
        if (bPromptAllowed)
        {
            QMessageBox::warning(this, QString(), tr("Copying aborted. You should select keys in a single node only."));
        }
        return false;
    }

    // Get Selected Track.
    CSequencerUtils::SelectedTracks selectedTracks;
    CSequencerUtils::GetSelectedTracks(m_pSequence, selectedTracks);
    if (!bCopySelected && !CSequencerUtils::IsOneTrackSelected(selectedTracks))
    {
        return false;
    }

    int iSelTrackCount = (int)selectedTracks.tracks.size();

    XmlNodeRef copyNode = XmlHelpers::CreateXmlNode("CopyKeysNode");

    if (bCopySelected)
    {
        CSequencerUtils::SelectedKeys selectedKeys;
        CSequencerUtils::GetSelectedKeys(m_pSequence, selectedKeys);
        int iSelKeyCount = (int)selectedKeys.keys.size();

        // Find the earliest point of all tracks (to allow us to copy relative time)
        bool bFoundStart = false;
        float fAbsoluteStartTime = 0;
        for (int i = 0; i < iSelKeyCount; ++i)
        {
            CSequencerUtils::SelectedKey sKey = selectedKeys.keys[i];

            if (!bFoundStart || sKey.fTime < fAbsoluteStartTime)
            {
                bFoundStart = true;
                fAbsoluteStartTime = sKey.fTime;
            }
        }

        CSequencerTrack* pCopyFromTrack = NULL;
        XmlNodeRef childNode;

        int firstTrack = -1;
        for (int k = 0; k < iSelKeyCount; ++k)
        {
            CSequencerUtils::SelectedKey skey = selectedKeys.keys[k];

            if (pCopyFromTrack != skey.pTrack)
            {
                // Find which track this is on the node
                int trackOffset = 0;
                for (int trackID = 0; trackID < skey.pNode->GetTrackCount(); ++trackID)
                {
                    CSequencerTrack* thisTrack = skey.pNode->GetTrackByIndex(trackID);
                    if (skey.pTrack == thisTrack)
                    {
                        if (firstTrack == -1)
                        {
                            firstTrack = trackOffset;
                        }
                        trackOffset = trackOffset - firstTrack;
                        break;
                    }

                    if (skey.pTrack->GetParameterType() == thisTrack->GetParameterType())
                    {
                        trackOffset++;
                    }
                }

                childNode = copyNode->newChild("Track");
                childNode->setAttr("paramId", skey.pTrack->GetParameterType());
                childNode->setAttr("paramIndex", trackOffset);
                skey.pTrack->SerializeSelection(childNode, false, true, -fAbsoluteStartTime);
                pCopyFromTrack = skey.pTrack;
            }
        }
    }
    else
    {
        // Find the earliest point of all tracks (to allow us to copy relative time)
        bool bFoundStart = false;
        float fAbsoluteStartTime = 0;
        for (int i = 0; i < iSelTrackCount; ++i)
        {
            CSequencerUtils::SelectedTrack sTrack = selectedTracks.tracks[i];
            Range trackTimeRange = sTrack.pTrack->GetTimeRange();

            if (!bFoundStart || trackTimeRange.start < fAbsoluteStartTime)
            {
                fAbsoluteStartTime = trackTimeRange.start;
            }
        }

        for (int i = 0; i < iSelTrackCount; ++i)
        {
            CSequencerTrack* pCurrTrack = selectedTracks.tracks[i].pTrack;
            if (!pCurrTrack)
            {
                continue;
            }

            XmlNodeRef childNode = copyNode->newChild("Track");
            childNode->setAttr("paramId", pCurrTrack->GetParameterType());
            if (pCurrTrack->GetNumKeys() > 0)
            {
                pCurrTrack->SerializeSelection(childNode, false, false, -fAbsoluteStartTime);
            }
        }
    }

    // Drag and drop operations don't want to use the clipboard
    m_bUseClipboard = bUseClipboard;
    if (bUseClipboard)
    {
        CClipboard clip(this);
        clip.Put(copyNode, "Track view keys");
    }
    else
    {
        m_dragClipboard = copyNode;
    }

    return true;
}


void CSequencerDopeSheetBase::CopyTrack()
{
    CSequencerUtils::SelectedTracks selectedTracks;
    CSequencerUtils::GetSelectedTracks(m_pSequence, selectedTracks);
    if (!CSequencerUtils::IsOneTrackSelected(selectedTracks))
    {
        return;
    }

    CSequencerTrack* pCurrTrack = selectedTracks.tracks[0].pTrack;
    if (!pCurrTrack)
    {
        return;
    }

    XmlNodeRef copyNode = XmlHelpers::CreateXmlNode("TrackCopy");

    XmlNodeRef childNode = copyNode->newChild("Track");
    childNode->setAttr("paramId", pCurrTrack->GetParameterType());
    if (pCurrTrack->GetNumKeys() > 0)
    {
        pCurrTrack->SerializeSelection(childNode, false, false, 0.0f);
    }

    CClipboard clip(this);
    clip.Put(copyNode, "Track Copy");
}


void CSequencerDopeSheetBase::StartDraggingKeys(const QPoint& point)
{
    CSequencerUtils::SelectedKeys selectedKeys;
    CSequencerUtils::GetSelectedKeys(m_pSequence, selectedKeys);

    // Dragging mode entered but no keys selected
    if (selectedKeys.keys.size() == 0)
    {
        return;
    }

    CSequencerTrack* pFirstTrack = selectedKeys.keys[0].pTrack;
    CSequencerNode* pFirstNode = selectedKeys.keys[0].pNode;
    for (std::vector<CSequencerUtils::SelectedKey>::iterator iter = selectedKeys.keys.begin(), iterEnd = selectedKeys.keys.end(); iter != iterEnd; ++iter)
    {
        // If we are not allowed to move this key
        if (!(*iter).pTrack->CanMoveKey((*iter).nKey))
        {
            return;
        }

        // If the keys are from different nodes then we abort
        if ((*iter).pNode != pFirstNode)
        {
            return;
        }
    }

    const int mouseOverItemID = ItemFromPoint(point);
    CSequencerTrack* mouseOverTrack = GetTrack(mouseOverItemID);
    CSequencerNode* mouseOverNode = GetNode(mouseOverItemID);
    CSequencerTrack* firstKeyTrack = selectedKeys.keys[0].pTrack;

    // We want to know which track it is, but there could be multiple keys on a single track
    int mouseOverTrackID = 0;
    int firstKeyTrackID = 0;
    bool bMouseOverFound = false;
    bool bFirstKeyFound = false;

    for (int trackID = 0; trackID < mouseOverNode->GetTrackCount(); ++trackID)
    {
        CSequencerTrack* currentTrack = mouseOverNode->GetTrackByIndex(trackID);

        if (currentTrack->GetParameterType() == mouseOverTrack->GetParameterType())
        {
            // If this is the correct track, then we've found it
            if (currentTrack == mouseOverTrack)
            {
                bMouseOverFound = true;
            }

            if (!bMouseOverFound)
            {
                mouseOverTrackID++;
            }
        }

        if (currentTrack->GetParameterType() == firstKeyTrack->GetParameterType())
        {
            // If this is the correct track, then we've found it
            if (currentTrack == firstKeyTrack)
            {
                bFirstKeyFound = true;
            }

            if (!bFirstKeyFound)
            {
                firstKeyTrackID++;
            }
        }
    }

    // We want to know where the cursor is compared to the earliest key
    float earliestKeyTime = 9999.9f;
    for (std::vector<CSequencerUtils::SelectedKey>::iterator iter = selectedKeys.keys.begin(), iterEnd = selectedKeys.keys.end(); iter != iterEnd; ++iter)
    {
        if (earliestKeyTime > (*iter).fTime)
        {
            earliestKeyTime = (*iter).fTime;
        }
    }

    // Get the time and track offset so we can keep the clip(s) in the same place under the cursor
    m_mouseMoveStartTimeOffset = TimeFromPointUnsnapped(point) - earliestKeyTime;
    m_mouseMoveStartTrackOffset = mouseOverTrackID - firstKeyTrackID;

    // Use the copy paste interface as it is required when moving onto different tracks anyway
    if (CopyKeys(false, false))
    {
        DelSelectedKeys(false, false, true);
        StartPasteKeys();
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::StartPasteKeys()
{
    if (m_pSequence)
    {
        CAnimSequenceUndo(m_pSequence, "Paste Keys");
    }

    // Store the current state of everything (we need this to be able to "undo" previous pastes during the operation)
    XmlNodeRef serializedData = XmlHelpers::CreateXmlNode("PrePaste");
    SerializeTracks(serializedData);
    m_prePasteSheetData = serializedData;

    m_mouseMode = MOUSE_MODE_PASTE_DRAGTIME;
    // If mouse over selected key, change cursor to left-right arrows.
    setCursor(m_crsLeftRight);
    m_mouseDownPos = m_mouseOverPos;

    // Paste at mouse cursor
    float time = TimeFromPointUnsnapped(m_mouseDownPos);
    m_bValidLastPaste = PasteKeys(NULL, NULL, time - m_mouseMoveStartTimeOffset);
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::FinalizePasteKeys()
{
    GetIEditor()->AcceptUndo("Paste Keys");
    unsetCursor();
    m_mouseMode = MOUSE_MODE_NONE;

    // Clean out the serialized data
    m_prePasteSheetData = NULL;

    CSequencerUtils::SelectedKeys selectedKeys;
    CSequencerUtils::GetSelectedKeys(m_pSequence, selectedKeys);
    for (int i = 0; i < (int)selectedKeys.keys.size(); ++i)
    {
        UpdateAnimation(selectedKeys.keys[i].pTrack);
    }

    // we didn't finish with a valid paste, revert!
    if (!m_bValidLastPaste)
    {
        GetIEditor()->Undo();
        UnselectAllKeys(true);
        update();
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::SerializeTracks(XmlNodeRef& destination)
{
    int nNodeCount = m_pSequence->GetNodeCount();
    for (int nodeID = 0; nodeID < nNodeCount; ++nodeID)
    {
        CSequencerNode* pAnimNode = m_pSequence->GetNode(nodeID);

        // Create a new node and add it as a child
        XmlNodeRef animXmlNode = XmlHelpers::CreateXmlNode(pAnimNode->GetName());
        destination->addChild(animXmlNode);

        int nTrackCount = pAnimNode->GetTrackCount();
        for (int trackID = 0; trackID < nTrackCount; ++trackID)
        {
            CSequencerTrack* pAnimTrack = pAnimNode->GetTrackByIndex(trackID);

            // Add the track to the anim node
            XmlNodeRef trackXmlNode = XmlHelpers::CreateXmlNode("track");
            animXmlNode->addChild(trackXmlNode);

            //If transition property track, abort! These cannot be serialized
            if (pAnimTrack->GetParameterType() == SEQUENCER_PARAM_TRANSITIONPROPS)
            {
                continue;
            }

            pAnimTrack->Serialize(trackXmlNode, false);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::DeserializeTracks(const XmlNodeRef& source)
{
    int nNodeCount = m_pSequence->GetNodeCount();
    for (int nodeID = 0; nodeID < nNodeCount; ++nodeID)
    {
        CSequencerNode* pAnimNode = m_pSequence->GetNode(nodeID);
        XmlNodeRef animXmlNode = source->getChild(nodeID);

        // Tracks can be added during pasting, the xml serialization won't have a child for the new track
        int xmlNodeCount = animXmlNode->getChildCount();
        int nTrackCount = pAnimNode->GetTrackCount();
        for (int trackID = 0; trackID < nTrackCount; ++trackID)
        {
            CSequencerTrack* pAnimTrack = pAnimNode->GetTrackByIndex(trackID);

            //If transition property track, abort! These cannot be serialized
            if (pAnimTrack->GetParameterType() == SEQUENCER_PARAM_TRANSITIONPROPS)
            {
                continue;
            }

            // Remove existing keys
            int nKeyCount = pAnimTrack->GetNumKeys();
            for (int keyID = nKeyCount - 1; keyID >= 0; --keyID)
            {
                pAnimTrack->RemoveKey(keyID);
            }

            // If the track was added (test by it not having been serialized at start) then there wont be data for it
            if (trackID < xmlNodeCount)
            {
                // Serialize in the source data
                XmlNodeRef trackXmlNode = animXmlNode->getChild(trackID);
                pAnimTrack->Serialize(trackXmlNode, true);
            }
        }
    }

    m_pSequence->UpdateKeys();
}

//////////////////////////////////////////////////////////////////////////
bool CSequencerDopeSheetBase::PasteKeys(CSequencerNode* pAnimNode, CSequencerTrack* pAnimTrack, float fTimeOffset)
{
    int nPasteToItem = -1;
    if (!pAnimNode)
    {
        // Find mouse-over item.
        int nPasteToItem = ItemFromPoint(m_mouseOverPos);
        if (nPasteToItem >= 0)
        {
            const Item& item = GetItem(nPasteToItem);

            // If cursor is dragged over something which isn't a valid track
            if (item.node == NULL || item.track == NULL)
            {
                update();
                return false;
            }

            pAnimNode = item.node;
            pAnimTrack = item.track;

            // If drag/drop then it's possible we are dragging from a key different from the first, so we need to figure out an offset from the cursor
            if (m_mouseMoveStartTrackOffset != 0)
            {
                int hoverTrackID = 0;
                int trackCount = pAnimNode->GetTrackCount();
                for (int trackID = 0; trackID < trackCount; ++trackID)
                {
                    CSequencerTrack* currentTrack = pAnimNode->GetTrackByIndex(trackID);
                    if (currentTrack->GetParameterType() == item.track->GetParameterType())
                    {
                        if (item.track == currentTrack)
                        {
                            break;
                        }
                        hoverTrackID++;
                    }
                }

                int offsetTrackID = CLAMP(hoverTrackID - m_mouseMoveStartTrackOffset, 0, trackCount - 1);
                pAnimTrack = pAnimNode->GetTrackForParameter(item.track->GetParameterType(), offsetTrackID);
            }
        }
    }

    if (!pAnimNode || !pAnimTrack || pAnimNode->IsReadOnly())
    {
        return false;
    }

    CClipboard clip(this);
    if (m_bUseClipboard && clip.IsEmpty())
    {
        return false;
    }

    // When dragging we don't save to clipboard - functionality is identical though
    XmlNodeRef copyNode = m_bUseClipboard ? clip.Get() : m_dragClipboard;
    if (copyNode == NULL || strcmp(copyNode->getTag(), "CopyKeysNode"))
    {
        return false;
    }

    int nNumTracksToPaste = copyNode->getChildCount();

    if (nNumTracksToPaste == 0)
    {
        return false;
    }

    bool bUseGivenTrackIfPossible = false;
    if (nNumTracksToPaste == 1 && pAnimTrack)
    {
        bUseGivenTrackIfPossible = true;
    }

    bool bIsPasted = false;

    UnselectAllKeys(false);
    m_bAnySelected = true;

    {
        // Find how many of the type we're dragging exist
        int numOfMouseOverType = 0;
        for (int i = 0; i < copyNode->getChildCount(); i++)
        {
            XmlNodeRef trackNode = copyNode->getChild(i);
            int intParamId;
            uint32 paramIndex = 0;
            trackNode->getAttr("paramId", intParamId);
            trackNode->getAttr("paramIndex", paramIndex);

            if ((ESequencerParamType)intParamId == pAnimTrack->GetParameterType())
            {
                numOfMouseOverType++;
            }
        }


        bool bNewTrackCreated = false;
        for (int i = 0; i < copyNode->getChildCount(); i++)
        {
            XmlNodeRef trackNode = copyNode->getChild(i);
            int intParamId;
            uint32 paramIndex = 0;
            trackNode->getAttr("paramId", intParamId);
            trackNode->getAttr("paramIndex", paramIndex);

            const ESequencerParamType paramId = static_cast<ESequencerParamType>(intParamId);

            // Find which track the mouse is over
            int mouseOverTrackId = 0;
            int numOfTrackType = 0;
            bool trackFound = false;
            for (int j = 0; j < pAnimNode->GetTrackCount(); ++j)
            {
                CSequencerTrack* pTrack = pAnimNode->GetTrackByIndex(j);

                if (pTrack->GetParameterType() == paramId)
                {
                    if (pTrack == pAnimTrack)
                    {
                        trackFound = true;
                    }

                    if (!trackFound)
                    {
                        mouseOverTrackId++;
                    }

                    numOfTrackType++;
                }
            }

            if (!trackFound)
            {
                mouseOverTrackId = 0;
            }

            // To stop cursor from allowing you to drag further than will fit on the sheet (otherwise the clips would compress onto the last track)
            mouseOverTrackId = CLAMP(mouseOverTrackId, 0, numOfTrackType - numOfMouseOverType);

            // Sanitize the track number (don't want to create lots of spare tracks)
            int trackId = paramIndex + mouseOverTrackId;

            // If the track would be otherwise pasted on a track which doesn't exist, then abort
            trackId = CLAMP(trackId, 0, numOfTrackType - 1);

            CSequencerTrack* pTrack = pAnimNode->GetTrackForParameter(paramId, trackId);
            if (bUseGivenTrackIfPossible && pAnimTrack->GetParameterType() == paramId)
            {
                pTrack = pAnimTrack;
            }
            if (!pTrack && pAnimNode->CanAddTrackForParameter(paramId))
            {
                // Make this track.
                pTrack = pAnimNode->CreateTrack(paramId);
                if (pTrack)
                {
                    bNewTrackCreated = true;
                }
            }
            if (pTrack)
            {
                // Serialize the pasted data onto the track
                pTrack->SerializeSelection(trackNode, true, true, fTimeOffset);
            }
            else
            {
                return false;
            }
        }
        if (bNewTrackCreated)
        {
            GetIEditor()->Notify(eNotify_OnUpdateSequencer);
            NotifyKeySelectionUpdate();
        }
    }

    update();

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::RecordTrackUndo(const Item& item)
{
    if (item.track != 0)
    {
        CUndo undo("Track Modify");
        CUndo::Record(new CUndoSequencerSequenceModifyObject(item.track, m_pSequence));
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::ShowKeyTooltip(CSequencerTrack* pTrack, int nKey, int secondarySelection, const QPoint& point)
{
    if (m_lastTooltipPos == point)
    {
        return;
    }

    m_lastTooltipPos = point;

    assert(pTrack);
    float time = pTrack->GetKeyTime(nKey);
    QString desc;
    QString additionalDesc;
    float duration = 0;
    pTrack->GetTooltip(nKey, desc, duration);

    if (secondarySelection)
    {
        time = pTrack->GetSecondaryTime(nKey, secondarySelection);
        additionalDesc = pTrack->GetSecondaryDescription(nKey, secondarySelection);
    }

    QString tipText;
    if (GetTickDisplayMode() == SEQTICK_INSECONDS)
    {
        tipText = tr("%1, %2").arg(time, 0, 'f', 3).arg(desc);
    }
    else if (GetTickDisplayMode() == SEQTICK_INFRAMES)
    {
        tipText = tr("%1, %2").arg(FloatToIntRet(time / m_snapFrameTime)).arg(desc);
    }
    else
    {
        assert (0);
    }
    if (!additionalDesc.isEmpty())
    {
        tipText.append(QStringLiteral(" - "));
        tipText.append(additionalDesc);
    }
    QToolTip::hideText();
    QToolTip::showText(mapToGlobal(point), tipText);
}

//////////////////////////////////////////////////////////////////////////
void  CSequencerDopeSheetBase::InvalidateItem(int item)
{
    QRect rc;
    if (GetItemRect(item, rc) != -1)
    {
        update(rc);
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnUpdateSequencerKeys:
        update();
        m_changeCount++;
        break;
    case eNotify_OnBeginGameMode:
        m_storedTime = m_currentTime;
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::ClearSelection()
{
    for (int i = 0; i < GetCount(); i++)
    {
        m_tracks[i].bSelected = false;
    }
    update();
}

//////////////////////////////////////////////////////////////////////////
bool CSequencerDopeSheetBase::GetSelectedTracks(std::vector< CSequencerTrack* >& tracks) const
{
    CSequencerUtils::SelectedKeys selectedKeys;
    CSequencerUtils::GetSelectedKeys(m_pSequence, selectedKeys);

    for (std::vector<CSequencerUtils::SelectedKey>::iterator it = selectedKeys.keys.begin(),
         end = selectedKeys.keys.end(); it != end; ++it)
    {
        stl::push_back_unique(tracks, (*it).pTrack);
    }

    CSequencerUtils::SelectedTracks selectedTracks;
    CSequencerUtils::GetSelectedTracks(m_pSequence, selectedTracks);

    for (std::vector<CSequencerUtils::SelectedTrack>::iterator it = selectedTracks.tracks.begin(),
         end = selectedTracks.tracks.end(); it != end; ++it)
    {
        stl::push_back_unique(tracks, (*it).pTrack);
    }

    return !tracks.empty();
}

//////////////////////////////////////////////////////////////////////////
bool CSequencerDopeSheetBase::GetSelectedNodes(std::vector< CSequencerNode* >& nodes) const
{
    CSequencerUtils::SelectedKeys selectedKeys;
    CSequencerUtils::GetSelectedKeys(m_pSequence, selectedKeys);

    for (std::vector<CSequencerUtils::SelectedKey>::iterator it = selectedKeys.keys.begin(),
         end = selectedKeys.keys.end(); it != end; ++it)
    {
        stl::push_back_unique(nodes, (*it).pNode);
    }

    CSequencerUtils::SelectedTracks selectedTracks;
    CSequencerUtils::GetSelectedTracks(m_pSequence, selectedTracks);

    for (std::vector<CSequencerUtils::SelectedTrack>::iterator it = selectedTracks.tracks.begin(),
         end = selectedTracks.tracks.end(); it != end; ++it)
    {
        stl::push_back_unique(nodes, (*it).pNode);
    }

    return !nodes.empty();
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::SelectItem(int item)
{
    if (item >= 0 && item < GetCount())
    {
        m_tracks[item].bSelected = true;
    }
    update();
}

//////////////////////////////////////////////////////////////////////////
bool CSequencerDopeSheetBase::IsSelectedItem(int item)
{
    if (item >= 0 && item < GetCount())
    {
        return m_tracks[item].bSelected;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::OffsetKey(CSequencerTrack* track, const int keyIndex, const float timeOffset) const
{
    if (m_secondarySelection)
    {
        const float time = track->GetSecondaryTime(keyIndex, m_secondarySelection) + timeOffset;
        track->SetSecondaryTime(keyIndex, m_secondarySelection, time);
    }
    else
    {
        // do the actual setting of the key time
        const float keyt = track->GetKeyTime(keyIndex) + timeOffset;
        track->SetKeyTime(keyIndex, keyt);
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::NotifyKeySelectionUpdate()
{
    GetIEditor()->Notify(eNotify_OnUpdateSequencerKeySelection);

    // In the end, focus this again in order to properly catch 'KeyDown' messages.
    setFocus();
}

//////////////////////////////////////////////////////////////////////////
bool CSequencerDopeSheetBase::IsOkToAddKeyHere(const CSequencerTrack* pTrack, float time) const
{
    const float timeEpsilon = 0.05f;
    if ((pTrack->GetFlags() & CSequencerTrack::SEQUENCER_TRACK_READONLY) != 0)
    {
        return false;
    }

    if (!pTrack->CanAddKey(time))
    {
        return false;
    }

    for (int i = 0; i < pTrack->GetNumKeys(); ++i)
    {
        if (fabs(pTrack->GetKeyTime(i) - time) < timeEpsilon)
        {
            return false;
        }
    }

    return true;
}

void CSequencerDopeSheetBase::MouseMoveSelect(const QPoint& point)
{
    unsetCursor();
    QRect rc(m_mouseDownPos, point);
    rc = rc.normalized();

    if (m_mouseMode == MOUSE_MODE_SELECT_WITHIN_TIME)
    {
        rc.setTop(m_rcClient.top());
        rc.setBottom(m_rcClient.bottom());
    }
    m_rcSelect->setGeometry(rc);
    m_rcSelect->setVisible(true);
}

void CSequencerDopeSheetBase::MouseMoveMove(const QPoint& p, Qt::KeyboardModifiers modifiers)
{
    {
        CSequencerUtils::SelectedKeys selectedKeys;
        CSequencerUtils::GetSelectedKeys(m_pSequence, selectedKeys);
        bool canAnyKeyBeMoved = CSequencerUtils::CanAnyKeyBeMoved(selectedKeys);
        if (canAnyKeyBeMoved)
        {
            setCursor(m_crsLeftRight);
        }
        else
        {
            setCursor(m_crsCross);
        }
    }

    QPoint point = p;
    if (point.x() < m_rcClient.left())
    {
        point.rx() = m_rcClient.left();
    }
    if (point.x() > m_rcClient.right())
    {
        point.rx() = m_rcClient.right();
    }
    const QPoint ofs = point - m_mouseDownPos;

    if (modifiers & Qt::ControlModifier)
    {
        SetSnappingMode(SEQKEY_SNAP_NONE);
    }
    else if (modifiers & Qt::ShiftModifier)
    {
        SetSnappingMode(SEQKEY_SNAP_TICK);
    }
    else
    {
        SetSnappingMode(SEQKEY_SNAP_MAGNET);
    }

    const ESequencerSnappingMode snappingMode = GetSnappingMode();
    const bool bTickSnap = snappingMode == SEQKEY_SNAP_TICK;

    if (m_mouseActionMode == SEQMODE_SCALEKEY)
    {
        // Offset all selected keys back by previous offset.
        if (m_keyTimeOffset != 0)
        {
            ScaleSelectedKeys(1.0f / (1.0f + m_keyTimeOffset), bTickSnap);
        }
    }
    else
    {
        // Offset all selected keys back by previous offset.
        if (m_keyTimeOffset != 0)
        {
            OffsetSelectedKeys(-m_keyTimeOffset, bTickSnap);
        }
    }

    QPoint keyStartPt = point;
    if (!m_secondarySelection)
    {
        keyStartPt.rx() -= m_grabOffset;
    }
    float newTime;
    float oldTime;
    newTime = TimeFromPointUnsnapped(keyStartPt);

    CSequencerNode* node = 0;
    const int key = LastKeyFromPoint(m_mouseDownPos);
    if ((key >= 0) && (!m_secondarySelection))
    {
        int item = ItemFromPoint(m_mouseDownPos);
        CSequencerTrack* track = GetTrack(item);
        node = GetNode(item);

        oldTime = track->GetKeyTime(key);
    }
    else
    {
        oldTime = TimeFromPointUnsnapped(m_mouseDownPos);
    }

    // Snap it, if necessary.
    if (GetSnappingMode() == SEQKEY_SNAP_MAGNET)
    {
        newTime = MagnetSnap(newTime, node);
    }
    else if (GetSnappingMode() == SEQKEY_SNAP_TICK)
    {
        newTime = TickSnap(newTime);
    }

    m_realTimeRange.ClipValue(newTime);

    const float timeOffset = newTime - oldTime;

    if (m_mouseActionMode == SEQMODE_SCALEKEY)
    {
        const float tscale = 0.005f;
        const float tofs = ofs.x() * tscale;
        // Offset all selected keys by this offset.
        ScaleSelectedKeys(1.0f + tofs, bTickSnap);
        m_keyTimeOffset = tofs;
    }
    else
    {
        // Offset all selected keys by this offset.
        OffsetSelectedKeys(timeOffset, bTickSnap);
        m_keyTimeOffset = timeOffset;

        CSequencerUtils::SelectedKeys selectedKeys;
        CSequencerUtils::GetSelectedKeys(m_pSequence, selectedKeys);
        if (selectedKeys.keys.size() > 0)
        {
            ShowKeyTooltip(selectedKeys.keys[0].pTrack, selectedKeys.keys[0].nKey, m_secondarySelection, point);
        }

        if ((modifiers& Qt::AltModifier) && CMannequinDialog::GetCurrentInstance()->IsPaneSelected<CFragmentEditorPage*>())
        {
            CMannequinDialog::GetCurrentInstance()->FragmentEditor()->SetTimeToSelectedKey();
        }
    }
    update();
}

void CSequencerDopeSheetBase::MouseMoveDragTime(const QPoint& point, Qt::KeyboardModifiers modifiers)
{
    QPoint p = point;
    if (p.x() < m_rcClient.left())
    {
        p.rx() = m_rcClient.left();
    }
    if (p.x() > m_rcClient.right())
    {
        p.rx() = m_rcClient.right();
    }
    if (p.y() < m_rcClient.top())
    {
        p.ry() = m_rcClient.top();
    }
    if (p.y() > m_rcClient.bottom())
    {
        p.ry() = m_rcClient.bottom();
    }

    bool bCtrlHeld = modifiers & Qt::ControlModifier;
    bool bCtrlForSnap = gSettings.mannequinSettings.bCtrlForScrubSnapping;
    bool bSnap = !(bCtrlHeld ^ bCtrlForSnap);

    float time = TimeFromPointUnsnapped(p);
    m_realTimeRange.ClipValue(time);
    if (bSnap)
    {
        time = TickSnap(time);
    }
    SetCurrTime(time);
}

void CSequencerDopeSheetBase::MouseMoveDragStartMarker(const QPoint& point, Qt::KeyboardModifiers modifiers)
{
    QPoint p = point;
    if (p.x() < m_rcClient.left())
    {
        p.rx() = m_rcClient.left();
    }
    if (p.x() > m_rcClient.right())
    {
        p.rx() = m_rcClient.right();
    }
    if (p.y() < m_rcClient.top())
    {
        p.ry() = m_rcClient.top();
    }
    if (p.y() > m_rcClient.bottom())
    {
        p.ry() = m_rcClient.bottom();
    }

    bool bCtrlHeld = modifiers & Qt::ControlModifier;
    bool bCtrlForSnap = gSettings.mannequinSettings.bCtrlForScrubSnapping;
    bool bSnap = !(bCtrlHeld ^ bCtrlForSnap);

    float time = TimeFromPointUnsnapped(p);
    m_realTimeRange.ClipValue(time);
    if (bSnap)
    {
        time = TickSnap(time);
    }
    SetStartMarker(time);
}

void CSequencerDopeSheetBase::MouseMoveDragEndMarker(const QPoint& point, Qt::KeyboardModifiers modifiers)
{
    QPoint p = point;
    if (p.x() < m_rcClient.left())
    {
        p.rx() = m_rcClient.left();
    }
    if (p.x() > m_rcClient.right())
    {
        p.rx() = m_rcClient.right();
    }
    if (p.y() < m_rcClient.top())
    {
        p.ry() = m_rcClient.top();
    }
    if (p.y() > m_rcClient.bottom())
    {
        p.ry() = m_rcClient.bottom();
    }

    bool bCtrlHeld = modifiers & Qt::ControlModifier;
    bool bCtrlForSnap = gSettings.mannequinSettings.bCtrlForScrubSnapping;
    bool bSnap = !(bCtrlHeld ^ bCtrlForSnap);

    float time = TimeFromPointUnsnapped(p);
    m_realTimeRange.ClipValue(time);
    if (bSnap)
    {
        time = TickSnap(time);
    }
    SetEndMarker(time);
}

void CSequencerDopeSheetBase::MouseMovePaste(const QPoint& point, Qt::KeyboardModifiers modifiers)
{
    // Remove the excess keys inserted during last paste operation
    if (m_prePasteSheetData != NULL)
    {
        DeserializeTracks(m_prePasteSheetData);
    }

    QPoint p = point;
    if (p.x() < m_rcClient.left())
    {
        p.rx() = m_rcClient.left();
    }
    if (p.x() > m_rcClient.right())
    {
        p.rx() = m_rcClient.right();
    }

    bool bCtrlHeld = modifiers & Qt::ControlModifier;
    bool bCtrlForSnap = gSettings.mannequinSettings.bCtrlForScrubSnapping;
    bool bSnap = !(bCtrlHeld ^ bCtrlForSnap);

    // Tick snapping
    float time = TimeFromPointUnsnapped(p) - m_mouseMoveStartTimeOffset;
    m_realTimeRange.ClipValue(time);
    if (bSnap)
    {
        time = TickSnap(time);
    }

    // If shift is not held, then magnet snap
    if ((modifiers& Qt::ShiftModifier) == 0)
    {
        time = MagnetSnap(time, NULL); // second parameter isn't actually used...
    }

    m_bValidLastPaste = PasteKeys(NULL, NULL, time);
}

void CSequencerDopeSheetBase::MouseMoveOver(const QPoint& point)
{
    // No mouse mode.
    unsetCursor();
    int key = LastKeyFromPoint(point);
    if (key >= 0)
    {
        m_bMouseOverKey = true;
        int item = ItemFromPoint(point);
        CSequencerTrack* track = GetTrack(item);

        float t1 = TimeFromPointUnsnapped(QPoint(point.x() - 4, point.y()));
        float t2 = TimeFromPointUnsnapped(QPoint(point.x() + 4, point.y()));

        int secondarySelKey;
        int secondarySelection = track->FindSecondarySelectionPt(secondarySelKey, t1, t2);
        if (secondarySelection)
        {
            if (!track->CanMoveSecondarySelection(secondarySelKey, secondarySelection))
            {
                secondarySelection = 0;
            }
        }

        if (secondarySelection)
        {
            setCursor(m_crsMoveBlend);
        }
        else if (track->IsKeySelected(key) && track->CanMoveKey(key))
        {
            // If mouse over selected key and it can be moved, change cursor to left-right arrows.
            setCursor(m_crsLeftRight);
        }
        else
        {
            setCursor(m_crsCross);
        }

        if (track)
        {
            ShowKeyTooltip(track, key, secondarySelection, point);
        }
    }
    else
    {
        QToolTip::hideText();
    }
}

int CSequencerDopeSheetBase::GetAboveKey(CSequencerTrack*& aboveTrack)
{
    CSequencerTrack* track;
    int keyIndex;
    if (FindSingleSelectedKey(track, keyIndex) == false)
    {
        return -1;
    }

    // First, finds a track above.
    CSequencerTrack* pLastTrack = NULL;
    for (int i = 0; i < GetCount(); ++i)
    {
        if (m_tracks[i].track == track)
        {
            break;
        }

        bool bValidSingleTrack = m_tracks[i].track && m_tracks[i].track->GetNumKeys() > 0;
        if (bValidSingleTrack)
        {
            pLastTrack = m_tracks[i].track;
        }
    }

    if (pLastTrack == NULL)
    {
        return -1;
    }
    else
    {
        aboveTrack = pLastTrack;
    }

    // A track found. Now gets a proper key index there.
    int k;
    float selectedKeyTime = track->GetKeyTime(keyIndex);
    for (k = 0; k < aboveTrack->GetNumKeys(); ++k)
    {
        if (aboveTrack->GetKeyTime(k) >= selectedKeyTime)
        {
            break;
        }
    }

    return k % aboveTrack->GetNumKeys();
}

int CSequencerDopeSheetBase::GetBelowKey(CSequencerTrack*& belowTrack)
{
    CSequencerTrack* track;
    int keyIndex;
    if (FindSingleSelectedKey(track, keyIndex) == false)
    {
        return -1;
    }

    // First, finds a track below.
    CSequencerTrack* pLastTrack = NULL;
    for (int i = GetCount() - 1; i >= 0; --i)
    {
        if (m_tracks[i].track == track)
        {
            break;
        }

        bool bValidSingleTrack = m_tracks[i].track && m_tracks[i].track->GetNumKeys() > 0;
        if (bValidSingleTrack)
        {
            pLastTrack = m_tracks[i].track;
        }
    }

    if (pLastTrack == NULL)
    {
        return -1;
    }
    else
    {
        belowTrack = pLastTrack;
    }

    // A track found. Now gets a proper key index there.
    int k;
    float selectedKeyTime = track->GetKeyTime(keyIndex);
    for (k = 0; k < belowTrack->GetNumKeys(); ++k)
    {
        if (belowTrack->GetKeyTime(k) >= selectedKeyTime)
        {
            break;
        }
    }

    return k % belowTrack->GetNumKeys();
}

int CSequencerDopeSheetBase::GetRightKey(CSequencerTrack*& track)
{
    int keyIndex;
    if (FindSingleSelectedKey(track, keyIndex) == false)
    {
        return -1;
    }


    return (keyIndex + 1) % track->GetNumKeys();
}

int CSequencerDopeSheetBase::GetLeftKey(CSequencerTrack*& track)
{
    int keyIndex;
    if (FindSingleSelectedKey(track, keyIndex) == false)
    {
        return -1;
    }

    return (keyIndex + track->GetNumKeys() - 1) % track->GetNumKeys();
}

float CSequencerDopeSheetBase::MagnetSnap(const float origTime, const CSequencerNode* node) const
{
    CSequencerUtils::SelectedKeys selectedKeys;
    CSequencerUtils::GetKeysInTimeRange(m_pSequence, selectedKeys,
        origTime - MARGIN_FOR_MAGNET_SNAPPING / m_timeScale,
        origTime + MARGIN_FOR_MAGNET_SNAPPING / m_timeScale);

    float newTime = origTime;
    if (selectedKeys.keys.size() > 0)
    {
        // By default, just use the first key that belongs to the time range as a magnet.
        newTime = selectedKeys.keys[0].fTime;
        // But if there is an in-range key in a sibling track, use it instead.
        // Here a 'sibling' means a track that belongs to a same node.
        float bestDiff = fabsf(origTime - newTime);
        for (int i = 0; i < selectedKeys.keys.size(); ++i)
        {
            const float fTime = selectedKeys.keys[i].fTime;
            const float newDiff = fabsf(origTime - fTime);
            if (newDiff <= bestDiff)
            {
                bestDiff = newDiff;
                newTime = fTime;
            }
        }
    }
    return newTime;
}

//////////////////////////////////////////////////////////////////////////
float CSequencerDopeSheetBase::FrameSnap(float time) const
{
    double t = double(time / m_snapFrameTime + 0.5);
    t = t * m_snapFrameTime;
    return t;
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::ShowKeyPropertyCtrlOnSpot(int x, int y, bool bMultipleKeysSelected, bool bKeyChangeInSameTrack)
{
    if (m_keyPropertiesDlg == NULL)
    {
        return;
    }

    if (m_wndPropsOnSpot == nullptr)
    {
        const QRect rc = rect();
        m_wndPropsOnSpot = new ReflectedPropertyControl;
        m_wndPropsOnSpot->Setup(false, 175);
        m_wndPropsOnSpot->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
        bKeyChangeInSameTrack = false;
    }

    if (bKeyChangeInSameTrack)
    {
        m_wndPropsOnSpot->ClearSelection();
        m_wndPropsOnSpot->ReloadValues();
    }
    else
    {
        m_keyPropertiesDlg->PopulateVariables(*m_wndPropsOnSpot);
    }
    m_wndPropsOnSpot->move(x, y);
    m_wndPropsOnSpot->show();
    m_wndPropsOnSpot->raise();
    m_wndPropsOnSpot->ExpandAll();
    m_wndPropsOnSpot->resize(300, qMin(30, m_wndPropsOnSpot->GetContentHeight()));
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::HideKeyPropertyCtrlOnSpot()
{
    if (m_wndPropsOnSpot)
    {
        m_wndPropsOnSpot->setVisible(false);
        m_wndPropsOnSpot->ClearSelection();
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetBase::SetScrollOffset(int hpos)
{
    m_scrollBar->setValue(hpos);
    m_scrollOffset.setX(hpos);
    update();
}

void CSequencerDopeSheetBase::dragEnterEvent(QDragEnterEvent * event)
{
    // Drag a fragment
    STreeFragmentDataPtr pFragData = CFragmentBrowser::draggedFragment(event->mimeData());
    if (pFragData && pFragData->item)
    {
        event->accept();
        return;
    }

    // Drag an animation (from Geppetto)
    auto animations = CFragmentBrowser::draggedAnimations(event->mimeData());
    if(!animations.empty())
    {
        event->accept();
        return;
    }
}

void CSequencerDopeSheetBase::dropEvent(QDropEvent *event)
{
    // Drop a fragment in preview timeline
    STreeFragmentDataPtr pFragData = CFragmentBrowser::draggedFragment(event->mimeData());
    if (pFragData && pFragData->item)
    {
        // Check for dropping a fragment into the preview panel
        if (IsPointValidForFragmentInPreviewDrop(event->pos(), pFragData))
        {
            CreatePointForFragmentInPreviewDrop(event->pos(), pFragData);
        }
    }

    // Drop an animation in timeline (from Geppetto)
    std::vector<QString> animations = CFragmentBrowser::draggedAnimations(event->mimeData());
    if (!animations.empty())
    {
        // Only support for drop single animation
        if (IsPointValidForAnimationInLayerDrop(event->pos(), animations[0]))
        {
            CreatePointForAnimationInLayerDrop(event->pos(), animations[0]);
        }
    }
}

#include <Mannequin/SequencerDopeSheetBase.moc>