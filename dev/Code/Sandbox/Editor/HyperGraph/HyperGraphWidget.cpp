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

#include "HyperGraphWidget.h"
#include "HyperGraphManager.h"

#include "FlowGraphNode.h"
#include "CommentNode.h"
#include "CommentBoxNode.h"
#include "BlackBoxNode.h"
#include "TrackEventNode.h"
#include "QuickSearchNode.h"
#include "Tarray.h"
#include "Viewport.h"

#include "Clipboard.h"
#include "FlowGraphManager.h"
#include "FlowGraph.h"
#include "FlowGraphNode.h"
#include "FlowGraphVariables.h"
#include "GameEngine.h"
#include "HyperGraphConstants.h"
#include "HyperGraphDialog.h"

#include "Objects/EntityObject.h"
#include "Objects/ObjectLayerManager.h"
#include "HyperGraph/FlowGraphDebuggerEditor.h"
#include "HyperGraph/FlowGraphModuleManager.h"

#include <QtUtil.h>
#include <QtUtilWin.h>

#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QKeyEvent>
#include <QDebug>
#include <QTextOption>
#include <QFontMetrics>

#define _USE_MATH_DEFINES                           // M_PI
#include <math.h>
#include <QRubberBand>
#include <QScrollBar>
#include <QToolTip>
#include <QMimeData>

//////////////////////////////////////////////////////////////////////////

#define BACKGROUND_COLOR (gSettings.hyperGraphColors.colorBackground)
#define GRID_COLOR           (gSettings.hyperGraphColors.colorGrid)
#define ARROW_COLOR (gSettings.hyperGraphColors.colorArrow)
#define ARROW_SEL_COLOR_IN (gSettings.hyperGraphColors.colorInArrowHighlighted)
#define ARROW_SEL_COLOR_OUT (gSettings.hyperGraphColors.colorOutArrowHighlighted)
#define PORT_EDGE_HIGHLIGHT (gSettings.hyperGraphColors.colorPortEdgeHighlighted)
#define ARROW_DIS_COLOR (gSettings.hyperGraphColors.colorArrowDisabled)
#define ARROW_WIDTH 14

#define MIN_ZOOM 0.01f
#define MAX_ZOOM 100.0f
#define MIN_ZOOM_CAN_EDIT 0.8f
#define MIN_ZOOM_CHANGE_HEIGHT 1.2f

#define DEFAULT_EDIT_HEIGHT 14

#define SMALLNODE_SPLITTER 40
#define SMALLNODE_WIDTH 160

#define PORT_HEIGHT 16


//////////////////////////////////////////////////////////////////////////
// CEditPropertyCtrl implementation.
//////////////////////////////////////////////////////////////////////////


#include "HyperGraphWidget.h"

void CEditPropertyCtrl::keyPressEvent(QKeyEvent* event)
{
    ReflectedPropertyControl::keyPressEvent(event);

    switch (event->key())
    {
    case Qt::Key_Return:
        if (m_pQView)
        {
            m_pQView->HideEditPort();
        }
        break;
    }
}

void CEditPropertyCtrl::SelectItem(ReflectedPropertyItem* item)
{
    ReflectedPropertyControl::SelectItem(item);
    if (!item && m_pQView)
    {
        m_pQView->HideEditPort();
    }
}

//////////////////////////////////////////////////////////////////////////
void CEditPropertyCtrl::Expand(ReflectedPropertyItem* item, bool bExpand)
{
    // Avoid parent method calling
}


//////////////////////////////////////////////////////////////////////////
QHyperGraphWidget::QHyperGraphWidget(QWidget* parent)
    : QWidget(parent)
    , m_rcSelect(new QRubberBand(QRubberBand::Rectangle, this))
    , m_renameEdit(0)
    , m_quickSearchEdit(0)
    , m_horizontalScrollBar(new QScrollBar(this))
    , m_verticalScrollBar(new QScrollBar(this))
    , m_bIsClosing(false)
    , m_horizontalScrollBarPos(0)
    , m_verticalScrollBarPos(0)
{
    m_pGraph = 0;
    m_pSrcDragNode = 0;
    m_mouseDownPos = QPoint(0, 0);
    m_scrollOffset = QPoint(0, 0);
    m_rcSelect->setVisible(false);
    m_rcSelect->lower();
    m_mode = NothingMode;

    m_pPropertiesCtrl = 0;
    m_isEditPort = false;
    m_zoom = 1.0f;
    m_bSplineArrows = true;
    m_bCopiedBlackBox = false;
    m_componentViewMask = 0; // will be set by dialog
    m_bRefitGraphInOnSize = false;
    m_pEditPort = 0;
    m_bIsFrozen = false;
    m_pQuickSearchNode = NULL;
    m_bIgnoreHyperGraphEvents = false;

    m_horizontalScrollBar->setOrientation(Qt::Horizontal);
    m_verticalScrollBar->setOrientation(Qt::Vertical);
    connect(m_horizontalScrollBar, &QScrollBar::valueChanged, this, &QHyperGraphWidget::OnHScroll);
    connect(m_verticalScrollBar, &QScrollBar::valueChanged, this, &QHyperGraphWidget::OnVScroll);

    setMouseTracking(true);

    m_editParamCtrl.Setup();
    m_editParamCtrl.SetView(this);
    m_editParamCtrl.ExpandAll();

    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
}

//////////////////////////////////////////////////////////////////////////
QHyperGraphWidget::~QHyperGraphWidget()
{
    m_bIsClosing = true;
    SetGraph(NULL);
}

//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::SetPropertyCtrl(ReflectedPropertyControl* propsCtrl)
{
    m_pPropertiesCtrl = propsCtrl;

    if (m_pPropertiesCtrl)
    {
        m_pPropertiesCtrl->SetUpdateCallback(functor(*this, &QHyperGraphWidget::OnUpdateProperties));
        m_pPropertiesCtrl->EnableUpdateCallback(true);
    }
}

//////////////////////////////////////////////////////////////////////////
static bool LinesegRectSingle(QPointF* pWhere, const Lineseg& line1, float x1, float y1, float x2, float y2)
{
    Vec3 a(x1, y1, 0);
    Vec3 b(x2, y2, 0);
    Lineseg line2(a, b);
    float t1, t2;
    if (Intersect::Lineseg_Lineseg2D(line1, line2, t1, t2))
    {
        Vec3 pos = line1.start + t1 * (line1.end - line1.start);
        pWhere->setX(pos.x);
        pWhere->setY(pos.y);
        return true;
    }
    return false;
}

static void LinesegRectIntersection(QPointF* pWhere, EHyperEdgeDirection* pDir, const Lineseg& line, const QRectF& rect)
{
    if (LinesegRectSingle(pWhere, line, rect.x(), rect.y(), rect.x() + rect.width(), rect.y()))
    {
        *pDir = eHED_Up;
    }
    else if (LinesegRectSingle(pWhere, line, rect.x(), rect.y(), rect.x(), rect.y() + rect.height()))
    {
        *pDir = eHED_Left;
    }
    else if (LinesegRectSingle(pWhere, line, rect.x() + rect.width(), rect.y(), rect.x() + rect.width(), rect.y() + rect.height()))
    {
        *pDir = eHED_Right;
    }
    else if (LinesegRectSingle(pWhere, line, rect.x(), rect.y() + rect.height(), rect.x() + rect.width(), rect.y() + rect.height()))
    {
        *pDir = eHED_Down;
    }
    else
    {
        *pDir = eHED_Down;
        pWhere->setY(rect.x() + rect.width() * 0.5f);
        pWhere->setY(rect.y() + rect.height() * 0.5f);
    }
}

void QHyperGraphWidget::ReroutePendingEdges()
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_EDITOR);

    std::vector<PendingEdges::iterator> eraseIters;

    for (PendingEdges::iterator iter = m_edgesToReroute.begin(); iter != m_edgesToReroute.end(); ++iter)
    {
        CHyperEdge* pEdge = *iter;
        //      if (pEdge->nodeOut == -1)
        //          DEBUG_BREAK;
        CHyperNode* pNodeOut = (CHyperNode*)m_pGraph->FindNode(pEdge->nodeOut);
        CHyperNode* pNodeIn = (CHyperNode*)m_pGraph->FindNode(pEdge->nodeIn);
        bool bDraggingEdge = std::find(m_draggingEdges.begin(), m_draggingEdges.end(), pEdge) != m_draggingEdges.end();
        if ((!pNodeIn && m_bDraggingFixedOutput && !bDraggingEdge) || (!pNodeOut && !m_bDraggingFixedOutput && !bDraggingEdge))
        {
            continue;
        }

        QRectF rectOut, rectIn;
        if (pNodeIn)
        {
            if (!pNodeIn->GetAttachQRect(pEdge->nPortIn, &rectIn) && !pNodeIn->GetBlackBoxWhenMinimized())
            {
                continue;
            }
            QPointF p = pNodeIn->GetPos();
            rectIn.translate(p.x(), p.y());
        }
        else if (m_bDraggingFixedOutput && bDraggingEdge)
        {
            rectIn = QRectF(pEdge->pointIn.x(), pEdge->pointIn.y(), 0, 0);
        }
        else
        {
            continue;
        }

        if (pNodeOut)
        {
            if (!pNodeOut->GetAttachQRect(pEdge->nPortOut + 1000, &rectOut) && !pNodeOut->GetBlackBoxWhenMinimized())
            {
                continue;
            }
            QPointF p = pNodeOut->GetPos();
            rectOut.translate(p.x(), p.y());
        }
        else if (!m_bDraggingFixedOutput && bDraggingEdge)
        {
            rectOut = QRectF(pEdge->pointOut.x(), pEdge->pointOut.y(), 0, 0);
        }
        else
        {
            continue;
        }

        if (rectOut.isEmpty() && rectIn.isEmpty())
        {
            pEdge->pointOut.setX(rectOut.left());
            pEdge->pointOut.setY(rectOut.top());
            pEdge->pointIn.setX(rectIn.left());
            pEdge->pointIn.setY(rectIn.top());
            // hack... but it works
            pEdge->dirOut = eHED_Right;
            pEdge->dirIn = eHED_Left;
        }
        else if (!rectOut.isEmpty() && !rectIn.isEmpty())
        {
            Vec3 centerOut(rectOut.x() + rectOut.width() * 0.5f, rectOut.y() + rectOut.height() * 0.5f, 0.0f);
            Vec3 centerIn(rectIn.x() + rectIn.width() * 0.5f, rectIn.y() + rectIn.height() * 0.5f, 0.0f);
            Vec3 dir = centerOut - centerIn;
            dir.Normalize();
            dir *= 10.0f;
            std::swap(dir.x, dir.y);
            dir.x = -dir.x;
            centerIn += dir;
            centerOut += dir;
            Lineseg centerLine(centerOut, centerIn);
            LinesegRectIntersection(&pEdge->pointOut, &pEdge->dirOut, centerLine, rectOut);
            LinesegRectIntersection(&pEdge->pointIn, &pEdge->dirIn, centerLine, rectIn);
        }
        else if (!rectOut.isEmpty() && rectIn.isEmpty())
        {
            Vec3 centerOut(rectOut.x() + rectOut.width() * 0.5f, rectOut.y() + rectOut.height() * 0.5f, 0.0f);
            Vec3 centerIn(rectIn.x() + rectIn.width() * 0.5f, rectIn.y() + rectIn.height() * 0.5f, 0.0f);
            Lineseg centerLine(centerOut, centerIn);
            LinesegRectIntersection(&pEdge->pointOut, &pEdge->dirOut, centerLine, rectOut);
            pEdge->pointIn.setX(rectIn.left());
            pEdge->pointIn.setY(rectIn.top());
            pEdge->dirIn = eHED_Left;
        }
        else if (rectOut.isEmpty() && !rectIn.isEmpty())
        {
            Vec3 centerOut(rectOut.x() + rectOut.width() * 0.5f, rectOut.y() + rectOut.height() * 0.5f, 0.0f);
            Vec3 centerIn(rectIn.x() + rectIn.width() * 0.5f, rectIn.y() + rectIn.height() * 0.5f, 0.0f);
            Lineseg centerLine(centerOut, centerIn);
            LinesegRectIntersection(&pEdge->pointIn, &pEdge->dirIn, centerLine, rectIn);
            pEdge->pointOut.setX(rectOut.left());
            pEdge->pointOut.setY(rectOut.top());
            pEdge->dirOut = eHED_Right;
        }
        else
        {
            assert(false);
        }

        eraseIters.push_back(iter);
    }
    while (!eraseIters.empty())
    {
        m_edgesToReroute.erase(eraseIters.back());
        eraseIters.pop_back();
    }
}

//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::RerouteAllEdges()
{
    std::vector<CHyperEdge*> edges;
    m_pGraph->GetAllEdges(edges);
    std::copy(edges.begin(), edges.end(), std::inserter(m_edgesToReroute, m_edgesToReroute.end()));

    std::set<CHyperNode*> nodes;
    for (std::vector<CHyperEdge*>::const_iterator iter = edges.begin(); iter != edges.end(); ++iter)
    {
        CHyperNode* pNodeIn = (CHyperNode*)m_pGraph->FindNode((*iter)->nodeIn);
        CHyperNode* pNodeOut = (CHyperNode*)m_pGraph->FindNode((*iter)->nodeOut);
        if (pNodeIn && pNodeOut)
        {
            nodes.insert(pNodeIn);
            nodes.insert(pNodeOut);
        }
    }

    QPainter p(this);
    for (std::set<CHyperNode*>::const_iterator iter = nodes.begin(); iter != nodes.end(); ++iter)
    {
        CHyperNode* fn = (CHyperNode*)(*iter);
        fn->Draw(this, &p, false);
    }
}

//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::paintEvent(QPaintEvent* e)
{
    QWidget::paintEvent(e);

    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_EDITOR);

    QPainter gr(this); // device context for painting

    QRect rect = e->rect();
    QRectF clip = ViewToLocalRect(rect);
    gr.fillRect(rect, QBrush(BACKGROUND_COLOR));

    gr.translate(m_scrollOffset.x(), m_scrollOffset.y());
    gr.scale(m_zoom, m_zoom);

    if (m_pGraph)
    {
        DrawGrid(&gr, rect);
        ReroutePendingEdges();

        if (gSettings.bFlowGraphEdgesOnTopOfNodes)
        {
            DrawNodes(&gr, rect);
            gr.setRenderHints(QPainter::Antialiasing, true);
            DrawEdges(&gr, rect);
        }
        else
        {
            gr.setRenderHints(QPainter::Antialiasing, true);
            DrawEdges(&gr, rect);
            gr.setRenderHints(QPainter::Antialiasing, false);
            DrawNodes(&gr, rect);
        }

        for (size_t i = 0; i < m_draggingEdges.size(); ++i)
        {
            DrawArrow(&gr, m_draggingEdges[i], 0);
        }
    }
    else
    {
        QString str("There is no Flow Graph Loaded\nCreate a new Flow Graph using \"File > New\"");
        QPainter painter(this);
        painter.setPen(QColor(Qt::white));
        painter.setFont(QFont("Tohama", 12));
        QFontMetrics fm(painter.font());
        QRect r = fm.boundingRect(str);
        r.setHeight(r.height() * 2.5);
        QTextOption to;
        to.setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        QPoint p(size().width() / 2 - r.width() / 2, size().height() / 2 - r.height() / 2);
        r.moveTo(p);
        painter.drawText(r, str, to);
    }

    if (m_bIsFrozen)
    {
        QColor c(BACKGROUND_COLOR);
        c.setAlpha(0x80);
        gr.fillRect(clip, c);
    }
}

void QHyperGraphWidget::mousePressEvent(QMouseEvent* event)
{
    event->accept();
    switch (event->button())
    {
    case Qt::LeftButton:
        OnLButtonDown(event->modifiers(), event->pos());
        break;
    case Qt::RightButton:
        OnRButtonDown(event->modifiers(), event->pos());
        break;
    case Qt::MiddleButton:
        OnMButtonDown(event->modifiers(), event->pos());
        break;
    }
}

void QHyperGraphWidget::mouseReleaseEvent(QMouseEvent* event)
{
    event->accept();
    switch (event->button())
    {
    case Qt::LeftButton:
        OnLButtonUp(event->modifiers(), event->pos());
        break;
    case Qt::RightButton:
        OnRButtonUp(event->modifiers(), event->pos());
        break;
    case Qt::MiddleButton:
        OnMButtonUp(event->modifiers(), event->pos());
        break;
    }
}

void QHyperGraphWidget::mouseMoveEvent(QMouseEvent* event)
{
    event->accept();
    OnMouseMove(event->modifiers(), event->pos());
    if (event->buttons() == Qt::NoButton)
    {
        OnSetCursor();
    }
}

void QHyperGraphWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    event->accept();
    if (event->button() == Qt::LeftButton)
    {
        OnLButtonDblClk(event->modifiers(), event->pos());
    }
    else
    {
        event->ignore();
    }
}

void QHyperGraphWidget::wheelEvent(QWheelEvent* event)
{
    event->accept();
    OnMouseWheel(event->modifiers(), event->delta(), event->pos());
}

void QHyperGraphWidget::OnMouseWheel(Qt::KeyboardModifiers modifiers, short zDelta, const QPoint& pt)
{
    HideEditPort();
    if (m_pQuickSearchNode)
    {
        OnCancelSearch();
    }

    float multiplier = 0.001f;

    bool bShift = modifiers & Qt::ShiftModifier;
    if (bShift)
    {
        multiplier *= 2.0f;
    }

    // zoom around the mouse cursor position, first get old mouse pos
    const QPointF oldZoomFocus = ViewToLocal(pt);

    // zoom
    SetZoom(m_zoom + m_zoom * multiplier * zDelta);

    // determine where the focus has moved as part of the zoom
    const QPoint newZoomFocus = LocalToView(oldZoomFocus);

    // adjust offset based on displacement and update windows stuff
    m_scrollOffset -= (newZoomFocus - pt);
    SetScrollExtents(false);
    update();
}

void QHyperGraphWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    //marcok: the following code handles the case when we've tried to fit the graph to view whilst not knowing the correct size of the client rect
    if (m_bRefitGraphInOnSize)
    {
        m_bRefitGraphInOnSize = false;
        OnCommandFitAll();
    }

    SetScrollExtents(false);
}

//////////////////////////////////////////////////////////////////////////
bool QHyperGraphWidget::HitTestEdge(const QPoint& pnt, CHyperEdge*& pOutEdge, int& nIndex)
{
    if (!m_pGraph)
    {
        return false;
    }

    std::vector<CHyperEdge*> edges;
    if (m_pGraph->GetAllEdges(edges))
    {
        QPointF mousePt = ViewToLocal(pnt);

        for (int i = 0; i < edges.size(); i++)
        {
            CHyperEdge* pEdge = edges[i];

            for (int j = 0; j < 4; j++)
            {
                float dx = 0.0f;
                float dy = 0.0f;
                if (m_bSplineArrows && pEdge->cornerModified && (j == 0))
                {
                    dx = pEdge->originalCenter.x() - mousePt.x();
                    dy = pEdge->originalCenter.y() - mousePt.y();
                }
                else
                {
                    dx = pEdge->cornerPoints[j].x() - mousePt.x();
                    dy = pEdge->cornerPoints[j].y() - mousePt.y();
                }

                if ((dx * dx + dy * dy) < 7)
                {
                    pOutEdge = pEdge;
                    nIndex = j;
                    return true;
                }
            }
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnLButtonDown(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    if (!m_pGraph)
    {
        return;
    }

    HideEditPort();

    m_pHitEdge = 0;

    setFocus();
    // Save the mouse down position
    m_mouseDownPos = point;


    if (m_bIsFrozen)
    {
        return;
    }

    bool bAltClick = modifiers & Qt::AltModifier;
    bool bCtrlClick = modifiers & Qt::ControlModifier;
    bool bShiftClick = modifiers & Qt::ShiftModifier;

    QPointF mousePt = ViewToLocal(point);

    CHyperEdge* pHitEdge = 0;
    int nHitEdgePoint = 0;
    if (HitTestEdge(point, pHitEdge, nHitEdgePoint))
    {
        m_pHitEdge = pHitEdge;
        m_nHitEdgePoint = nHitEdgePoint;
        m_prevCornerW = m_pHitEdge->cornerW;
        m_prevCornerH = m_pHitEdge->cornerH;
        m_mode = EdgePointDragMode;
        return;
    }

    CHyperNode* pNode = GetMouseSensibleNodeAtPoint(point);
    if (pNode)
    {
        int objectClicked = pNode->GetObjectAt(ViewToLocal(point));

        CHyperNodePort* pPort = NULL;
        static const int WAS_A_PORT = -10000;
        static const int WAS_A_SELECT = -10001;

        if (objectClicked >= eSOID_ShiftFirstOutputPort && objectClicked <= eSOID_ShiftLastOutputPort)
        {
            if (bShiftClick)
            {
                objectClicked = eSOID_FirstOutputPort + (objectClicked - eSOID_ShiftFirstOutputPort);
            }
            else if (bAltClick)
            {
                objectClicked = eSOID_FirstOutputPort + (objectClicked - eSOID_ShiftFirstOutputPort);
            }
            else
            {
                objectClicked = eSOID_Background;
            }
        }

        if ((bAltClick && (objectClicked < eSOID_FirstOutputPort && objectClicked > eSOID_LastOutputPort)) || bCtrlClick)
        {
            objectClicked = WAS_A_SELECT;
        }
        else if (objectClicked >= eSOID_FirstInputPort && objectClicked <= eSOID_LastInputPort && !pNode->GetInputs()->empty())
        {
            pPort = &pNode->GetInputs()->at(objectClicked - eSOID_FirstInputPort);
            objectClicked = WAS_A_PORT;
        }
        else if (objectClicked >= eSOID_FirstOutputPort && objectClicked <= eSOID_LastOutputPort && !pNode->GetOutputs()->empty())
        {
            pPort = &pNode->GetOutputs()->at(objectClicked - eSOID_FirstOutputPort);
            objectClicked = WAS_A_PORT;
        }
        else if (pNode->GetClassName() == CBlackBoxNode::GetClassType())
        {
            pPort = ((CBlackBoxNode*)(pNode))->GetPortAtPoint2(ViewToLocal(point));
            if (pPort)
            {
                objectClicked = WAS_A_PORT;
                pNode = ((CBlackBoxNode*)(pNode))->GetNode(pPort);
            }
            else if (objectClicked == eSOID_Minimize)
            {
                ((CBlackBoxNode*)(pNode))->Minimize();
                objectClicked = eSOID_Background;
            }
            else if (objectClicked == -1)
            {
                CBlackBoxNode* pBB = (CBlackBoxNode*)pNode;
                if (pBB->IsMinimized() == false)
                {
                    std::vector<CHyperNode*>* nodes = pBB->GetNodes();
                    for (int i = 0; i < nodes->size(); ++i)
                    {
                        int clicked = ((*nodes)[i])->GetObjectAt(ViewToLocal(point));
                        if (clicked > 0)
                        {
                            pNode = (*nodes)[i];
                            objectClicked = clicked;
                            break;
                        }
                    }
                }
            }
        }
        switch (objectClicked)
        {
        case eSOID_InputTransparent:
            break;
        case WAS_A_SELECT:
        case eSOID_AttachedEntity:
        case eSOID_Background:
        case eSOID_Title:
        {
            bool bUnselect = bCtrlClick && pNode->IsSelected();
            if (!pNode->IsSelected() || bUnselect)
            {
                CUndo undo("Select Graph Node(s)");
                m_pGraph->RecordUndo();
                // Select this node if not selected yet.
                if (!bCtrlClick && !bUnselect)
                {
                    m_pGraph->UnselectAll();
                }
                //pNode->SetSelected( !bUnselect );
                IHyperGraphEnumerator* pEnum = pNode->GetRelatedNodesEnumerator();
                for (IHyperNode* pRelative = pEnum->GetFirst(); pRelative; pRelative = pEnum->GetNext())
                {
                    static_cast<CHyperNode*>(pRelative)->SetSelected(!bUnselect);
                }
                pEnum->Release();
                OnSelectionChange();
            }
            m_mode = MoveMode;
            m_moveHelper.clear();
            GetIEditor()->BeginUndo();
        }
        break;
        case eSOID_InputGripper:
            ShowPortsConfigMenu(point, true, pNode);
            break;
        case eSOID_OutputGripper:
            ShowPortsConfigMenu(point, false, pNode);
            break;
        case eSOID_Minimize:
            OnToggleMinimizeNode(pNode);
            break;
        case eSOID_Border_UpRight:
        case eSOID_Border_Right:
        case eSOID_Border_DownRight:
        case eSOID_Border_Down:
        case eSOID_Border_DownLeft:
        case eSOID_Border_Left:
        case eSOID_Border_UpLeft:
        case eSOID_Border_Up:
        {
            if (pNode->GetResizeBorderRect())
            {
                m_mode = NodeBorderDragMode;
                m_DraggingBorderNodeInfo.m_pNode = pNode;
                m_DraggingBorderNodeInfo.m_border = objectClicked;
                m_DraggingBorderNodeInfo.m_origNodePos = pNode->GetPos();
                m_DraggingBorderNodeInfo.m_origBorderRect = *pNode->GetResizeBorderRect();
                m_DraggingBorderNodeInfo.m_clickPoint = mousePt;
            }
            break;
        }

        default:
            //assert(false);
            break;

        case WAS_A_PORT:

            if (!bAltClick && (!pPort->bInput ||(pPort->nConnected <= 1)))
            {
                if (!pNode->IsSelected())
                {
                    CUndo undo("Select Graph Node(s)");
                    m_pGraph->RecordUndo();
                    // Select this node if not selected yet.
                    m_pGraph->UnselectAll();
                    pNode->SetSelected(true);
                    OnSelectionChange();
                }

                if (pPort->bInput && !pPort->nConnected && pPort->pVar->GetType() != IVariable::UNKNOWN && m_zoom > MIN_ZOOM_CAN_EDIT)
                {
                    QRectF rect = pNode->GetRect();
                    if (mousePt.x() - rect.x() > ARROW_WIDTH)
                    {
                        m_pEditPort = pPort;
                    }
                }

                // Undo must start before edge delete.
                GetIEditor()->BeginUndo();
                m_pGraph->RecordUndo();
                pPort->bSelected = true;

                // If trying to drag output port, disconnect input parameter and drag it.
                if (pPort->bInput)
                {
                    CHyperEdge* pEdge = m_pGraph->FindEdge(pNode, pPort);
                    if (pEdge)
                    {
                        // Disconnect this edge.
                        pNode = (CHyperNode*)m_pGraph->FindNode(pEdge->nodeOut);
                        if (pNode)
                        {
                            pPort = &pNode->GetOutputs()->at(pEdge->nPortOut);
                            InvalidateEdge(pEdge);
                            // on input ports holding down shift has the effect of *cloning* rather than *moving* an edge.
                            if (!bShiftClick)
                            {
                                m_pGraph->RemoveEdge(pEdge);
                            }
                        }
                        else
                        {
                            pPort = 0;
                        }
                        m_bDraggingFixedOutput = true;
                    }
                    else
                    {
                        m_bDraggingFixedOutput = false;
                    }
                }
                else
                {
                    // on output ports holding down shift has the effect of "fixing" the input and moving the output
                    // of the edges originating from the clicked output port.
                    m_bDraggingFixedOutput = !bShiftClick;
                }
                if (!pPort || !pNode)
                {
                    GetIEditor()->CancelUndo();
                    return;
                }

                m_pSrcDragNode = pNode;
                m_mode = PortDragMode;
                m_draggingEdges.clear();

                // if shift pressed and we are editing an output port then batch move edges
                if (bShiftClick && !m_bDraggingFixedOutput)
                {
                    std::vector<CHyperEdge*> edges;
                    m_pGraph->FindEdges(pNode, edges);
                    for (size_t i = 0; i < edges.size(); ++i)
                    {
                        CHyperEdge* edge = edges[i];
                        if (edge->nPortOut != pPort->nPortIndex || edge->nodeOut != pNode->GetId())
                        {
                            continue;
                        }
                        // we create a new edge and remove the old one because the
                        // HyperGraph::m_edgesMap needs to be updated as well
                        _smart_ptr<CHyperEdge> pNewEdge = m_pGraph->CreateEdge();
                        m_draggingEdges.push_back(pNewEdge);
                        m_edgesToReroute.insert(pNewEdge);
                        pNewEdge->nodeOut = -1;
                        pNewEdge->nPortOut = -1;
                        pNewEdge->pointOut = edge->pointOut;
                        pNewEdge->nodeIn = edge->nodeIn;
                        pNewEdge->nPortIn = edge->nPortIn;
                        pNewEdge->pointIn = edge->pointIn;
                        m_pGraph->RemoveEdge(edge);
                    }
                    pPort->bSelected = false;
                }
                else // if shift not pressed operate normally
                {
                    _smart_ptr<CHyperEdge> pNewEdge = m_pGraph->CreateEdge();
                    m_draggingEdges.push_back(pNewEdge);
                    m_edgesToReroute.insert(pNewEdge);

                    pNewEdge->nodeIn = -1;
                    pNewEdge->nPortIn = -1;
                    pNewEdge->nodeOut = -1;
                    pNewEdge->nPortOut = -1;

                    if (m_bDraggingFixedOutput)
                    {
                        pNewEdge->nodeOut = pNode->GetId();
                        pNewEdge->nPortOut = pPort->nPortIndex;
                        pNewEdge->pointIn = ViewToLocal(point);
                    }
                    else
                    {
                        pNewEdge->nodeIn = pNode->GetId();
                        pNewEdge->nPortIn = pPort->nPortIndex;
                        pNewEdge->pointOut = ViewToLocal(point);
                    }
                    pPort->bSelected = true;
                }
                InvalidateNode(pNode, true);
            }
            else
            {
                QMenu menu;
                std::vector<CHyperEdge*> edges;
                m_pGraph->GetAllEdges(edges);
                for (size_t i = 0; i < edges.size(); ++i)
                {
                    if (edges[i]->nodeIn == pNode->GetId() && 0 == QString::compare(edges[i]->portIn.toUtf8().data(), pPort->GetName(), Qt::CaseInsensitive))
                    {
                        CHyperNode* pToNode = (CHyperNode*)m_pGraph->FindNode(edges[i]->nodeOut);
                        QVariant v;
                        v.setValue(i + 1);
                        menu.addAction(tr("Remove %1:%2").arg(pToNode->GetClassName()).arg(edges[i]->portOut))->setData(v);
                    }
                }
                QAction* action = menu.exec(QCursor::pos());
                int nEdge = action ? action->data().toInt() : 0;
                if (nEdge)
                {
                    nEdge--;
                    m_pGraph->RemoveEdge(edges[nEdge]);
                }
            }
            break;
        }
    }
    else
    {
        if (!bAltClick && !bCtrlClick && m_pGraph->IsSelectedAny())
        {
            CUndo undo("Unselect Graph Node(s)");
            m_pGraph->RecordUndo();
            m_pGraph->UnselectAll();
            OnSelectionChange();
        }
        m_mode = SelectMode;
    }
}

////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnLButtonUp(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    if (!m_pGraph)
    {
        return;
    }

    if (m_bIsFrozen)
    {
        return;
    }

    bool bAltClick = modifiers & Qt::AltModifier;
    bool bCtrlClick = modifiers & Qt::ControlModifier;
    bool bShiftClick = modifiers & Qt::ShiftModifier;

    switch (m_mode)
    {
    case SelectMode:
    {
        if (m_rcSelect->isVisible())
        {
            bool bUnselect = bAltClick;

            CUndo undo("Select Graph Node(s)");
            bool isUndoRecorded = false;
            if (!bAltClick && !bCtrlClick && m_pGraph->IsSelectedAny())
            {
                m_pGraph->RecordUndo();
                isUndoRecorded = true;
                m_pGraph->UnselectAll();
            }

            std::multimap<int, CHyperNode*> nodes;
            GetNodesInRect(m_rcSelect->geometry(), nodes, true);
            for (std::multimap<int, CHyperNode*>::reverse_iterator iter = nodes.rbegin(); iter != nodes.rend(); ++iter)
            {
                CHyperNode* pNode = iter->second;
                IHyperGraphEnumerator* pEnum = pNode->GetRelatedNodesEnumerator();
                for (IHyperNode* pRelative = pEnum->GetFirst(); pRelative; pRelative = pEnum->GetNext())
                {
                    if (!isUndoRecorded)
                    {
                        m_pGraph->RecordUndo();
                        isUndoRecorded = true;
                    }
                    static_cast<CHyperNode*>(pRelative)->SetSelected(!bUnselect);
                }
            }
            OnSelectionChange();
            update();
        }
    }
    break;
    case MoveMode:
    {
        m_moveHelper.clear();
        GetIEditor()->AcceptUndo("Move Graph Node(s)");
        SetScrollExtents();
        update();
        std::vector<CHyperNode*> apNodes;
        GetSelectedNodes(apNodes);
        for (int i = 0, iNodeCount(apNodes.size()); i < iNodeCount; ++i)
        {
            m_pGraph->SendNotifyEvent(apNodes[i], EHG_NODE_MOVE);
        }
    }
    break;
    case EdgePointDragMode:
    {
    }
    break;
    case NodeBorderDragMode:
    {
        m_DraggingBorderNodeInfo.m_pNode = 0;
    }
    break;
    case PortDragMode:
    {
        for (size_t i = 0; i < m_draggingEdges.size(); ++i)
        {
            if (CHyperNode* pNode = (CHyperNode*)m_pGraph->FindNode(m_draggingEdges[i]->nodeIn))
            {
                pNode->UnselectAllPorts();
            }
            if (CHyperNode* pNode = (CHyperNode*)m_pGraph->FindNode(m_draggingEdges[i]->nodeOut))
            {
                pNode->UnselectAllPorts();
            }
        }

        // unselect the port of the node from which the edges where dragged away
        if (m_pSrcDragNode)
        {
            m_pSrcDragNode->UnselectAllPorts();
            m_pSrcDragNode = 0;
        }

        CHyperNode* pTrgNode = GetNodeAtPoint(point);
        if (pTrgNode)
        {
            int objectClicked = pTrgNode->GetObjectAt(ViewToLocal(point));
            bool specialDrag = false;
            if (objectClicked >= eSOID_ShiftFirstOutputPort && objectClicked <= eSOID_ShiftLastOutputPort)
            {
                if (bShiftClick)
                {
                    objectClicked = eSOID_FirstInputPort + (objectClicked - eSOID_ShiftFirstOutputPort);
                }
                else if (bAltClick)
                {
                    objectClicked = eSOID_FirstInputPort + (objectClicked - eSOID_ShiftFirstOutputPort);
                    specialDrag = true;
                }
                else
                {
                    objectClicked = eSOID_Background;
                }
            }
            CHyperNodePort* pTrgPort = NULL;
            if (objectClicked >= eSOID_FirstInputPort && objectClicked <= eSOID_LastInputPort && !pTrgNode->GetInputs()->empty())
            {
                pTrgPort = &pTrgNode->GetInputs()->at(objectClicked - eSOID_FirstInputPort);
            }
            else if (objectClicked >= eSOID_FirstOutputPort && objectClicked <= eSOID_LastOutputPort && !pTrgNode->GetOutputs()->empty())
            {
                pTrgPort = &pTrgNode->GetOutputs()->at(objectClicked - eSOID_FirstOutputPort);
            }
            else if (pTrgNode->GetClassName() == CBlackBoxNode::GetClassType())
            {
                pTrgPort = ((CBlackBoxNode*)(pTrgNode))->GetPortAtPoint2(ViewToLocal(point));
                if (pTrgPort)
                {
                    pTrgNode = ((CBlackBoxNode*)(pTrgNode))->GetNode(pTrgPort);
                }
            }

            if (pTrgPort)
            {
                CHyperNode* pOutNode = 0, * pInNode = 0;
                CHyperNodePort* pOutPort = 0, * pInPort = 0;
                for (size_t i = 0; i < m_draggingEdges.size(); ++i)
                {
                    if (m_bDraggingFixedOutput)
                    {
                        pOutNode = (CHyperNode*)m_pGraph->FindNode(m_draggingEdges[i]->nodeOut);
                        pOutPort = &pOutNode->GetOutputs()->at(m_draggingEdges[i]->nPortOut);
                        pInNode = pTrgNode;
                        pInPort = pTrgPort;
                    }
                    else
                    {
                        pOutNode = pTrgNode;
                        pOutPort = pTrgPort;
                        pInNode = (CHyperNode*)m_pGraph->FindNode(m_draggingEdges[i]->nodeIn);
                        pInPort = &pInNode->GetInputs()->at(m_draggingEdges[i]->nPortIn);
                    }

                    CHyperEdge* pExistingEdge = NULL;
                    if (m_pGraph->CanConnectPorts(pOutNode, pOutPort, pInNode, pInPort, pExistingEdge))
                    {
                        // Connect
                        m_pGraph->RecordUndo();
                        m_pGraph->ConnectPorts(pOutNode, pOutPort, pInNode, pInPort, specialDrag);
                        InvalidateNode(pOutNode, true);
                        InvalidateNode(pInNode, true);
                        OnNodesConnected();
                    }
                    else
                    {
                        if (pExistingEdge && pExistingEdge->enabled == false)
                        {
                            m_pGraph->EnableEdge(pExistingEdge, true);
                        }
                    }
                }
            }
        }
        update();

        GetIEditor()->AcceptUndo("Connect Graph Node");
        m_draggingEdges.clear();
    }
    break;
    }
    m_rcSelect->setVisible(false);

    if (m_mode == ScrollModeDrag)
    {
        update();
        GetIEditor()->AcceptUndo("Connect Graph Node");
        m_draggingEdges.clear();
        m_mode = ScrollMode;
    }
    else
    {
        m_mode = NothingMode;
    }
}

////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnRButtonDown(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    if (!m_pGraph)
    {
        return;
    }

    HideEditPort();

    CHyperEdge* pHitEdge = 0;
    int nHitEdgePoint = 0;
    if (!m_bIsFrozen && HitTestEdge(point, pHitEdge, nHitEdgePoint))
    {
        QMenu menu;
        menu.addAction(tr("Remove"))->setData(1);
        if (pHitEdge->IsEditable())
        {
            menu.addAction(tr("Edit"))->setData(2);
        }
        if (m_pGraph->IsFlowGraph())
        {
            menu.addAction(pHitEdge->enabled ? tr("Disable") : tr("Enable"))->setData(3);
            menu.addAction(tr("Delay"))->setData(5);
            menu.addAction(tr("Any"))->setData(6);
        }

        // makes only sense for non-AIActions and while fg system is updated
        if (m_pGraph->IsFlowGraph() && m_pGraph->GetAIAction() == 0 && GetIEditor()->GetGameEngine()->IsFlowSystemUpdateEnabled())
        {
            menu.addAction(tr("Simulate flow [Double click]"))->setData(4);
        }

        QAction* action = menu.exec(QCursor::pos());
        switch (action == 0 ? 0 : action->data().toInt())
        {
        case 1:
        {
            CUndo undo("Remove Graph Edge");
            m_pGraph->RecordUndo();
            m_pGraph->RemoveEdge(pHitEdge);
            OnEdgeRemoved();
        }
        break;
        case 2:
        {
            CUndo undo("Edit Graph Edge");
            m_pGraph->RecordUndo();
            m_pGraph->EditEdge(pHitEdge);
        }
        break;
        case 3:
        {
            CUndo undo(pHitEdge->enabled ? "Disable Edge" : "Enable Edge");
            m_pGraph->RecordUndo();
            m_pGraph->EnableEdge(pHitEdge, !pHitEdge->enabled);
            InvalidateEdge(pHitEdge);
        }
        break;
        case 4:
            SimulateFlow(pHitEdge);
            break;
        case 5:
        {
            CUndo undo("Add Delay");
            CHyperNode* pDelayNode = CreateNode("Time:Delay", point);

            QRectF rect = pDelayNode->GetRect();
            QPointF adjustedPoint(rect.x() - (rect.width() / 2.0f), rect.y());

            pDelayNode->SetPos(adjustedPoint);

            CHyperNodePort* pDelayOutputPort = &pDelayNode->GetOutputs()->at(0);
            CHyperNodePort* pDelayInputPort = &pDelayNode->GetInputs()->at(0);

            CHyperNode* nodeIn = static_cast<CHyperNode*>(m_pGraph->FindNode(pHitEdge->nodeIn));
            CHyperNode* nodeOut = static_cast<CHyperNode*>(m_pGraph->FindNode(pHitEdge->nodeOut));

            if (pDelayInputPort && pDelayOutputPort && nodeIn && nodeOut)
            {
                m_pGraph->ConnectPorts(nodeOut, nodeOut->FindPort(pHitEdge->portOut, false), pDelayNode, pDelayInputPort, false);
                m_pGraph->ConnectPorts(pDelayNode, pDelayOutputPort, nodeIn, nodeIn->FindPort(pHitEdge->portIn, true), false);
            }
        }
        break;
        case 6:
        {
            CUndo undo("Add Logic:Any");
            CHyperNode* pLogicAnyNode = CreateNode("Logic:Any", point);

            QRectF rect = pLogicAnyNode->GetRect();
            QPointF adjustedPoint;

            adjustedPoint.setX(rect.x() - (rect.width() / 2.0f));
            adjustedPoint.setY(rect.y());

            pLogicAnyNode->SetPos(adjustedPoint);

            CHyperNodePort* pLogicAnyOutputPort = &pLogicAnyNode->GetOutputs()->at(0);
            CHyperNodePort* pLogicAnyInputPort = &pLogicAnyNode->GetInputs()->at(0);

            CHyperNode* nodeIn = static_cast<CHyperNode*>(m_pGraph->FindNode(pHitEdge->nodeIn));
            CHyperNode* nodeOut = static_cast<CHyperNode*>(m_pGraph->FindNode(pHitEdge->nodeOut));

            if (pLogicAnyInputPort && pLogicAnyOutputPort && nodeIn && nodeOut)
            {
                m_pGraph->ConnectPorts(nodeOut, nodeOut->FindPort(pHitEdge->portOut, false), pLogicAnyNode, pLogicAnyInputPort, false);
                m_pGraph->ConnectPorts(pLogicAnyNode, pLogicAnyOutputPort, nodeIn, nodeIn->FindPort(pHitEdge->portIn, true), false);
            }
            break;
        }
        }
        return;
    }

    setFocus();
    if (m_mode == PortDragMode)
    {
        m_mode = ScrollModeDrag;
    }
    else
    {
        if (m_mode == MoveMode)
        {
            GetIEditor()->AcceptUndo("Move Graph Node(s)");
        }

        m_mode = ScrollMode;
    }
    m_RButtonDownPos = point;
    m_mouseDownPos = point;
}

////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnRButtonUp(Qt::KeyboardModifiers, const QPoint& point)
{
    if (!m_pGraph)
    {
        return;
    }

    // make sure to reset the mode, otherwise we can end up in scrollmode while the context menu is open
    if (m_mode == ScrollModeDrag)
    {
        m_mode = PortDragMode;
    }
    else
    {
        m_mode = NothingMode;
    }

    if (m_bIsFrozen)
    {
        return;
    }

    if (abs(m_RButtonDownPos.x() - point.x()) < 3 && abs(m_RButtonDownPos.y() - point.y()) < 3)
    {
        // Show context menu if right clicked on the same point.
        CHyperNode* pNode = GetMouseSensibleNodeAtPoint(point);
        ShowContextMenu(point, pNode);
        return;
    }
}

////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnMButtonDown(Qt::KeyboardModifiers, const QPoint& point)
{
    if (!m_pGraph)
    {
        return;
    }

    HideEditPort();
    setFocus();
    if (m_mode == PortDragMode)
    {
        m_mode = ScrollModeDrag;
    }
    else
    {
        m_mode = ScrollMode;
    }
    m_RButtonDownPos = point;
    m_mouseDownPos = point;
}

////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnMButtonUp(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    if (!m_pGraph)
    {
        return;
    }

    if (m_mode == ScrollModeDrag)
    {
        m_mode = PortDragMode;
    }
    else
    {
        m_mode = NothingMode;
    }

    if (modifiers & Qt::ControlModifier && m_pGraph->IsFlowGraph())
    {
        CFlowNode* pNode = static_cast<CFlowNode*>(GetMouseSensibleNodeAtPoint(point));
        if (pNode)
        {
            CHyperNodePort* pPort = pNode->GetPortAtPoint(ViewToLocal(point));

            if (pPort)
            {
                CFlowGraphDebuggerEditor* pFlowgraphDebugger = GetIEditor()->GetFlowGraphDebuggerEditor();

                if (pFlowgraphDebugger)
                {
                    const bool hasBreakPoint = pFlowgraphDebugger->HasBreakpoint(pNode, pPort);
                    const bool invalidateNode = hasBreakPoint || (pPort->nConnected || !pPort->bInput);

                    if (hasBreakPoint)
                    {
                        pFlowgraphDebugger->RemoveBreakpoint(pNode, pPort);
                    }
                    // if the input is not connected at all with an edge, don't add a breakpoint
                    else if (pPort->nConnected || !pPort->bInput)
                    {
                        pFlowgraphDebugger->AddBreakpoint(pNode, pPort);
                    }

                    // redraw the node to update the "visual" breakpoint, otherwise you have to move the mouse first
                    // to trigger a redraw
                    if (invalidateNode)
                    {
                        InvalidateNode(pNode, true, false);
                    }
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnMouseMove(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    if (!m_pGraph)
    {
        return;
    }

    if (m_pMouseOverNode)
    {
        m_pMouseOverNode->UnselectAllPorts();
    }

    m_pMouseOverNode = NULL;
    m_pMouseOverPort = NULL;

    switch (m_mode)
    {
    case MoveMode:
    {
        GetIEditor()->RestoreUndo();
        const QPoint offset = point - m_mouseDownPos;
        MoveSelectedNodes(offset);
    }
    break;
    case ScrollMode:
        OnMouseMoveScrollMode(modifiers, point);
        break;
    case SelectMode:
    {
        // Update select rectangle.
        QRect rc(m_mouseDownPos, point);
        rc = rc.normalized().intersected(rect());
        m_rcSelect->setGeometry(rc);
        m_rcSelect->lower();
        m_rcSelect->setVisible(true);
    }
    break;
    case PortDragMode:
        OnMouseMovePortDragMode(modifiers, point);
        break;
    case NodeBorderDragMode:
        OnMouseMoveBorderDragMode(modifiers, point);
        break;
    case EdgePointDragMode:
        if (m_pHitEdge)
        {
            if (m_bSplineArrows)
            {
                const QPointF p1 = ViewToLocal(point);
                m_pHitEdge->cornerPoints[0] = p1;
                m_pHitEdge->cornerModified = true;
            }
            else
            {
                const QPointF p1 = ViewToLocal(point);
                const QPointF p2 = ViewToLocal(m_mouseDownPos);
                if (m_nHitEdgePoint < 2)
                {
                    float w = p1.x() - p2.x();
                    m_pHitEdge->cornerW = m_prevCornerW + w;
                    m_pHitEdge->cornerModified = true;
                }
                else
                {
                    float h = p1.y() - p2.y();
                    m_pHitEdge->cornerH = m_prevCornerH + h;
                    m_pHitEdge->cornerModified = true;
                }
            }
            update();
        }
        break;
    case ScrollModeDrag:
        OnMouseMoveScrollMode(modifiers, point);
        OnMouseMovePortDragMode(modifiers, point);
        break;
    default:
    {
        CHyperNodePort* pPort = NULL;
        CHyperNode* pNode = GetNodeAtPoint(point);

        if (pNode)
        {
            m_pMouseOverNode = pNode;
            pPort = pNode->GetPortAtPoint(ViewToLocal(point));

            if (pPort)
            {
                pPort->bSelected = true;
            }

            m_pMouseOverPort = pPort;
        }

        if (prevMovePoint != point)
        {
            UpdateTooltip(pNode, pPort);
            prevMovePoint = point;
        }
    }
    }
}

////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnMouseMoveScrollMode(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    if (!((abs(m_RButtonDownPos.x() - point.x()) < 3 && abs(m_RButtonDownPos.y() - point.y()) < 3)))
    {
        const QPoint offset = point - m_mouseDownPos;
        m_scrollOffset += offset;
        m_mouseDownPos = point;
        SetScrollExtents(false);
        update();
    }
}

////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnMouseMoveBorderDragMode(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    if (m_DraggingBorderNodeInfo.m_pNode)
    {
        const QPointF mousePt = ViewToLocal(point);
        const QPointF moved = mousePt - m_DraggingBorderNodeInfo.m_clickPoint;

        if (moved.y() == 0 && moved.x() == 0)
        {
            return;
        }

        bool bUp = false;
        bool bDown = false;
        bool bLeft = false;
        bool bRight = false;

        QPointF nodePos = m_DraggingBorderNodeInfo.m_origNodePos;
        QRectF borderRect = m_DraggingBorderNodeInfo.m_origBorderRect;

        switch (m_DraggingBorderNodeInfo.m_border)
        {
        case eSOID_Border_UpRight:
            bUp = true;
            bRight = true;
            break;

        case eSOID_Border_Right:
            bRight = true;
            break;

        case eSOID_Border_DownRight:
            bDown = true;
            bRight = true;
            break;

        case eSOID_Border_Down:
            bDown = true;
            break;

        case eSOID_Border_DownLeft:
            bDown = true;
            bLeft = true;
            break;

        case eSOID_Border_Left:
            bLeft = true;
            break;

        case eSOID_Border_UpLeft:
            bUp = true;
            bLeft = true;
            break;

        case eSOID_Border_Up:
            bUp = true;
            break;
        }

        if (bUp)
        {
            const float inc = -moved.y();
            nodePos.ry() -= inc;
            borderRect.setHeight(borderRect.height() + inc);
        }

        if (bDown)
        {
            const float inc = moved.y();
            borderRect.setHeight(borderRect.height() + inc);
        }

        if (bRight)
        {
            const float inc = moved.x();
            borderRect.setWidth(borderRect.width() + inc);
        }

        if (bLeft)
        {
            const float inc = -moved.x();
            nodePos.rx() -= inc;
            borderRect.setWidth(borderRect.width() + inc);
        }

        if (borderRect.width() < 10 || borderRect.height() < 10)
        {
            return;
        }

        if (bLeft || bUp)
        {
            m_DraggingBorderNodeInfo.m_pNode->SetPos(nodePos);
        }
        m_DraggingBorderNodeInfo.m_pNode->SetResizeBorderRect(borderRect);
        m_DraggingBorderNodeInfo.m_pNode->Invalidate(true);
        update();
    }
}

////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnMouseMovePortDragMode(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    CHyperNode* pNode = GetNodeAtPoint(point);

    for (size_t i = 0; i < m_draggingEdges.size(); ++i)
    {
        InvalidateEdgeRect(LocalToView(m_draggingEdges[i]->pointOut), LocalToView(m_draggingEdges[i]->pointIn));

        /*
        if (CHyperNode * pNode = (CHyperNode*) m_pGraph->FindNode( m_pDraggingEdge->nodeIn ))
        {
        pNode->GetInputs()->at(m_pDraggingEdge->nPortIn).bSelected = false;
        InvalidateNode( pNode );
        }
        if (CHyperNode * pNode = (CHyperNode*) m_pGraph->FindNode( m_pDraggingEdge->nodeOut ))
        InvalidateNode( pNode );
        */

        CHyperNode* pNodeIn = (CHyperNode*)m_pGraph->FindNode(m_draggingEdges[i]->nodeIn);
        CHyperNode* pNodeOut = (CHyperNode*)m_pGraph->FindNode(m_draggingEdges[i]->nodeOut);
        if (pNodeIn)
        {
            CHyperNode::Ports* ports = pNodeIn->GetInputs();
            if (ports->size() > m_draggingEdges[i]->nPortIn)
            {
                ports->at(m_draggingEdges[i]->nPortIn).bSelected = false;
            }
        }

        if (m_bDraggingFixedOutput)
        {
            m_draggingEdges[i]->pointIn = ViewToLocal(point);
            m_draggingEdges[i]->nodeIn = -1;
        }
        else
        {
            m_draggingEdges[i]->pointOut = ViewToLocal(point);
            m_draggingEdges[i]->nodeOut = -1;
        }

        if (pNode)
        {
            if (CHyperNodePort* pPort = pNode->GetPortAtPoint(ViewToLocal(point)))
            {
                if (m_bDraggingFixedOutput && pPort->bInput)
                {
                    m_draggingEdges[i]->nodeIn = pNode->GetId();
                    m_draggingEdges[i]->nPortIn = pPort->nPortIndex;
                    pNode->GetInputs()->at(m_draggingEdges[i]->nPortIn).bSelected = true;
                }
                else
                if (!m_bDraggingFixedOutput && !pPort->bInput)
                {
                    pNode->UnselectAllPorts();
                    m_draggingEdges[i]->nodeOut = pNode->GetId();
                    m_draggingEdges[i]->nPortOut = pPort->nPortIndex;
                    pNode->GetOutputs()->at(m_draggingEdges[i]->nPortOut).bSelected = true;
                }
            }
        }
        m_edgesToReroute.insert(m_draggingEdges[i]);

        if (pNode)
        {
            InvalidateNode(pNode, true);
        }
        if (pNodeIn)
        {
            InvalidateNode(pNodeIn, true);
        }
        if (pNodeOut)
        {
            if (m_draggingEdges[i]->nPortOut >= 0 && m_draggingEdges[i]->nPortOut < pNodeOut->GetOutputs()->size())
            {
                pNodeOut->GetOutputs()->at(m_draggingEdges[i]->nPortOut).bSelected = true;
            }
            InvalidateNode(pNodeOut, true);
        }
    }
}

////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::UpdateTooltip(CHyperNode* pNode, CHyperNodePort* pPort)
{
    if (pNode && pNode->IsTooltipShowable() && gSettings.bFlowGraphShowToolTip)
    {
        QString tip;
        if (pPort)
        {
            // Port tooltip.
            QString type;
            switch (pPort->pVar->GetType())
            {
            case IVariable::INT:
                type = "Integer";
                break;
            case IVariable::FLOAT:
                type = "Float";
                break;
            case IVariable::BOOL:
                type = "Boolean";
                break;
            case IVariable::VECTOR:
                type = "Vector";
                break;
            case IVariable::STRING:
                type = "String";
                break;
            //case IVariable::VOID: type = "Void"; break;
            default:
                type = "Any";
                break;
            }
            QString desc = pPort->pVar->GetDescription();
            if (!desc.isEmpty())
            {
                tip = tr("[%1] %2").arg(type, desc);
            }
            else
            {
                tip = tr("[%1] %2").arg(type, pNode->GetDescription());
            }
        }
        else
        {
            // Node tooltip.
            if (pNode->IsEditorSpecialNode())
            {
                tip = tr("Name: %1\nClass: %2\n%3").arg(pNode->GetName(), pNode->GetUIClassName(), pNode->GetDescription());
            }
            else
            {
                CFlowNode* pFlowNode = static_cast<CFlowNode*> (pNode);
                const QString cat = pFlowNode->GetCategoryName();
                const uint32 usageFlags = pFlowNode->GetUsageFlags();
                // TODO: something with Usage flags
                tip = tr("Name: %1\nClass: %2\nCategory: %3\nDescription: %4").arg(pFlowNode->GetName(), pFlowNode->GetUIClassName(), cat, pFlowNode->GetDescription());
            }
        }
        QToolTip::showText(QCursor::pos(), tip);
    }
    else
    {
        QToolTip::hideText();
    }
}

////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnLButtonDblClk(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    if (m_pGraph == 0)
    {
        return;
    }

    if (m_bIsFrozen)
    {
        return;
    }

    CHyperEdge* pHitEdge = 0;
    int nHitEdgePoint = 0;
    if (HitTestEdge(point, pHitEdge, nHitEdgePoint))
    {
        // makes only sense for non-AIActions and while fg system is updated
        if (m_pGraph->IsFlowGraph() && m_pGraph->GetAIAction() == 0 && GetIEditor()->GetGameEngine()->IsFlowSystemUpdateEnabled())
        {
            SimulateFlow(pHitEdge);
        }
    }
    else
    {
        CHyperNode* pNode = GetMouseSensibleNodeAtPoint(point);
        const QString nodeClassName = pNode ? pNode->GetClassName() : QStringLiteral("");

        if (pNode && pNode->IsEditorSpecialNode())
        {
            if (!nodeClassName.compare("QuickSearch"))
            {
                QuickSearchNode(pNode);
            }
            else
            {
                RenameNode(pNode);
            }
        }
        else
        {
            if (m_pEditPort)
            {
                ShowEditPort(pNode, m_pEditPort);
            }
            else if (!nodeClassName.isEmpty() && nodeClassName.indexOf("Module:Call_") != -1)
            {
                if (CFlowGraphManager* pFlowGraphManager = GetIEditor()->GetFlowGraphManager())
                {
                    if (CEditorFlowGraphModuleManager* pModuleManager = GetIEditor()->GetFlowGraphModuleManager())
                    {
                        const QString moduleGraphName = nodeClassName.mid(nodeClassName.indexOf('_') + 1);
                        IFlowGraphPtr moduleGraph = pModuleManager->GetModuleFlowGraph(moduleGraphName.toUtf8().data());

                        if (CFlowGraph* moduleFlowGraph = pFlowGraphManager->FindGraph(moduleGraph))
                        {
                            pFlowGraphManager->OpenView(moduleFlowGraph);
                        }
                    }
                }
            }
            else
            {
                if (m_pGraph->IsFlowGraph())
                {
                    OnSelectEntity();
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////

void QHyperGraphWidget::OnShortcutZoomIn()
{
    float multiplier = 0.1f;
    SetZoom(m_zoom + m_zoom * multiplier);
    InvalidateView();
}

void QHyperGraphWidget::OnShortcutZoomOut()
{
    float multiplier = 0.1f;
    SetZoom(m_zoom - m_zoom * multiplier);
    InvalidateView();
}

void QHyperGraphWidget::OnShortcutQuickNode()
{
    if (m_pGraph && m_pGraph->IsFlowGraph())
    {
        if (!m_pQuickSearchNode)
        {
            m_pQuickSearchNode = CreateNode(CQuickSearchNode::GetClassType(), mapFromGlobal(QCursor::pos()));
            if (m_pQuickSearchNode)
            {
                QuickSearchNode(m_pQuickSearchNode);
            }
        }

        InvalidateView();
    }
}

void QHyperGraphWidget::OnShortcutRefresh()
{
    update();
}

////////////////////////////////////////////////////////////////////////////perforce
void QHyperGraphWidget::OnSetCursor()
{
    if (m_mode == NothingMode)
    {
        const QPoint point = mapFromGlobal(QCursor::pos());

        CHyperEdge* pHitEdge = 0;
        int nHitEdgePoint = 0;
        if (HitTestEdge(point, pHitEdge, nHitEdgePoint))
        {
            setCursor(Qt::ArrowCursor);
            return;
        }

        CHyperNode* pNode = GetNodeAtPoint(point);
        if (pNode)
        {
            int object = pNode->GetObjectAt(ViewToLocal(point));
            switch (object)
            {
            case eSOID_Border_UpRight:
            case eSOID_Border_DownLeft:
                setCursor(Qt::SizeBDiagCursor);
                return;

            case eSOID_Border_Right:
            case eSOID_Border_Left:
                setCursor(Qt::SizeHorCursor);
                return;

            case eSOID_Border_DownRight:
            case eSOID_Border_UpLeft:
                setCursor(Qt::SizeFDiagCursor);
                return;

            case eSOID_Border_Up:
            case eSOID_Border_Down:
                setCursor(Qt::SizeVerCursor);
                return;
            }
        }
    }
    setCursor(Qt::ArrowCursor);
}

//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::InvalidateEdgeRect(const QPoint& p1, const QPoint& p2)
{
    //  QRect rc(p1, p2);
    //  rc = rc.normalized();
    //  rc.adjust(20, 7, 20, 7);
    //  update(rc);
    // KDAB_PORT, do a full update
    update();
}

//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::InvalidateEdge(CHyperEdge* pEdge)
{
    m_edgesToReroute.insert(pEdge);
    InvalidateEdgeRect(LocalToView(pEdge->pointIn), LocalToView(pEdge->pointOut));
}

//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::InvalidateNode(CHyperNode* pHNode, bool bRedraw, bool bIsModified /*=true*/)
{
    CHyperNode* pNode = (CHyperNode*)pHNode;
    assert(pNode);
    if (!m_pGraph)
    {
        return;
    }

    //CDC* pDC = GetDC();
    //if (pDC)
    //{
    QRectF rc = pNode->GetRect();
    if (pNode->IsEditorSpecialNode())
    {
        update(LocalToViewRect(QRectF(rc.x(), rc.y(), rc.width(), rc.height())));
    }

    QSizeF sz = pNode->CalculateSize();
    rc.setWidth(sz.width());
    rc.setHeight(sz.height());

    pNode->SetRect(rc);
    rc.adjust(-GRID_SIZE, -GRID_SIZE, GRID_SIZE, GRID_SIZE);
    update(LocalToViewRect(rc));

    if (bRedraw)
    {
        pNode->Invalidate(true, bIsModified);
    }

    // Invalidate all edges connected to this node.
    std::vector<CHyperEdge*> edges;
    if (m_pGraph->FindEdges(pNode, edges))
    {
        // Invalidate all node edges.
        for (int i = 0; i < edges.size(); i++)
        {
            InvalidateEdge(edges[i]);
        }
    }
    //}
    //else
    //{
    //  assert(false);
    //}

    //  // KDAB_PORT, do a full update
    //  update();
}

void QHyperGraphWidget::UpdateDebugCount(CHyperNode* pNode)
{
    if (!m_pGraph || !pNode)
    {
        return;
    }

    std::vector<CHyperEdge*> edges;
    int count = pNode->GetDebugCount();

    if (m_pGraph->FindEdges(pNode, edges))
    {
        for (int i = 0; i < edges.size(); ++i)
        {
            CHyperNode* nodeIn = static_cast<CHyperNode*>(m_pGraph->FindNode(edges[i]->nodeIn));
            if (!nodeIn)
            {
                continue;
            }

            CHyperNodePort* portIn = nodeIn->FindPort(edges[i]->portIn, true);
            if (!portIn)
            {
                continue;
            }

            if (nodeIn->IsDebugPortActivated(portIn))
            {
                edges[i]->debugCount++;
                nodeIn->ResetDebugPortActivation(portIn);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::DrawEdges(QPainter* gr, const QRect& updateRect)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_EDITOR);

    std::vector<CHyperEdge*> edges;
    m_pGraph->GetAllEdges(edges);

    CHyperEdge* pHitEdge = 0;
    int nHitEdgePoint = 0;

    QPoint qcursorPos = this->mapFromGlobal(QCursor::pos());
    HitTestEdge(qcursorPos, pHitEdge, nHitEdgePoint);

    for (std::vector<CHyperEdge*>::const_iterator it = edges.begin(), end = edges.end(); it != end; ++it)
    {
        CHyperEdge* pEdge = *it;

        CHyperNode* nodeIn = (CHyperNode*)m_pGraph->FindNode(pEdge->nodeIn);
        CHyperNode* nodeOut = (CHyperNode*)m_pGraph->FindNode(pEdge->nodeOut);
        if (!nodeIn || !nodeOut)
        {
            continue;
        }
        CHyperNodePort* portIn = nodeIn->FindPort(pEdge->portIn, true);
        CHyperNodePort* portOut = nodeOut->FindPort(pEdge->portOut, false);
        if (!portIn || !portOut)
        {
            continue;
        }

        // Draw arrow.
        int selection = 0;
        selection |= (int)(gSettings.bFlowGraphHighlightEdges && nodeIn->IsSelected());
        selection |= (int)(gSettings.bFlowGraphHighlightEdges && nodeOut->IsSelected()) << 1;
        if (selection == 0)
        {
            selection = (nodeIn->IsPortActivationModified(portIn)) ? 4 : 0;
        }

        if (m_pMouseOverPort == portIn || m_pMouseOverPort == portOut || pHitEdge == pEdge)
        {
            selection = 8;
        }

        // Check for custom selection mode here
        int customSelMode = pEdge->GetCustomSelectionMode();
        if (selection == 0 && customSelMode != -1)
        {
            selection = customSelMode;
        }

        if (nodeIn->GetBlackBoxWhenMinimized() || nodeOut->GetBlackBoxWhenMinimized())
        {
            if (nodeIn->GetBlackBoxWhenMinimized())
            {
                pEdge->pointIn = ((CBlackBoxNode*)(nodeIn->GetBlackBoxWhenMinimized()))->GetPointForPort(portIn);
            }

            if (nodeOut->GetBlackBoxWhenMinimized())
            {
                pEdge->pointOut = ((CBlackBoxNode*)(nodeOut->GetBlackBoxWhenMinimized()))->GetPointForPort(portOut);
            }

            QRectF box = DrawArrow(gr, pEdge, true, selection);
        }
        else
        {
            QRectF box = DrawArrow(gr, pEdge, true, selection);
            // this below does nothing in the flow graph case, it's overloaded in SelectionTreeEdge in BSTEditor
            //          pEdge->DrawSpecial(&gr, Gdiplus::PointF(box.X + box.Width / 2, box.Y + box.Height / 2));
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::CenterViewAroundNode(CHyperNode* poNode, bool boFitZoom, float fZoom)
{
    QRect rc = geometry();
    m_scrollOffset = QPoint(0, 0);

    QRectF nodeRect = poNode->GetRect();
    QRect rcGraph = LocalToViewRect(nodeRect);

    if (boFitZoom)
    {
        float zoom = (float)max(rc.width(), rc.height()) / (max(rcGraph.width(), rcGraph.height()) + GRID_SIZE * 4);
        if (zoom < 1)
        {
            SetZoom(zoom);
        }
    }

    if (fZoom > 0.0f)
    {
        SetZoom(fZoom);
    }

    rcGraph = LocalToViewRect(nodeRect);
    if (rcGraph.width() < rc.width())
    {
        m_scrollOffset.setX(m_scrollOffset.x() + (rc.width() - rcGraph.width()) / 2);
    }
    if (rcGraph.height() < rc.height())
    {
        m_scrollOffset.setY(m_scrollOffset.y() + (rc.height() - rcGraph.height()) / 2);
    }

    m_scrollOffset += QPoint(-rcGraph.left() + GRID_SIZE, -rcGraph.top() + GRID_SIZE);

    InvalidateView();
    update();
}

//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::PropagateChangesToGroupsAndPrefabs()
{
    if (m_pGraph && m_pGraph->IsFlowGraph())
    {
        if (CEntityObject* pGraphEntity = static_cast<CFlowGraph*>(m_pGraph)->GetEntity())
        {
            pGraphEntity->UpdateGroup();
            pGraphEntity->UpdatePrefab();
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::DrawNodes(QPainter* gr, const QRect& rc)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_EDITOR);

    std::multimap<int, CHyperNode*> nodes;
    GetNodesInRect(rc, nodes);

    std::vector<CHyperNode*> selectedNodes;
    const bool bNodesSelected = GetSelectedNodes(selectedNodes);

    for (std::multimap<int, CHyperNode*>::iterator iter = nodes.begin(); iter != nodes.end(); ++iter)
    {
        CHyperNode* pNode = (CHyperNode*)iter->second;

        if ((false == bNodesSelected) || (false == stl::find(selectedNodes, pNode)))
        {
            pNode->Draw(this, gr, true);
        }
    }

    if (bNodesSelected)
    {
        for (std::vector<CHyperNode*>::const_iterator iter = selectedNodes.begin(); iter != selectedNodes.end(); ++iter)
        {
            CHyperNode* pNode = (CHyperNode*)(*iter);
            pNode->Draw(this, gr, true);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
QRect QHyperGraphWidget::LocalToViewRect(const QRectF& localRect) const
{
    QRectF temp = localRect;
    temp.moveLeft(temp.x() * m_zoom);
    temp.moveTop(temp.y() * m_zoom);
    temp.setWidth(temp.width() * m_zoom);
    temp.setHeight(temp.height() * m_zoom);
    temp.translate(m_scrollOffset.x(), m_scrollOffset.y());
    return QRect(temp.x() - 1.0f, temp.y() - 1.0f, temp.width() + 2.05f, temp.height() + 2.0f);
}

//////////////////////////////////////////////////////////////////////////
QRectF QHyperGraphWidget::ViewToLocalRect(const QRect& viewRect) const
{
    QRectF rc(viewRect.x(), viewRect.y(), viewRect.width(), viewRect.height());
    rc.translate(-m_scrollOffset.x(), -m_scrollOffset.y());
    rc.moveLeft(rc.x() / m_zoom);
    rc.moveTop(rc.y() / m_zoom);
    rc.setWidth(rc.width() / m_zoom);
    rc.setHeight(rc.height() / m_zoom);
    return rc;
}

//////////////////////////////////////////////////////////////////////////
QPoint QHyperGraphWidget::LocalToView(const QPointF& point)
{
    return QPoint(point.x() * m_zoom + m_scrollOffset.x(), point.y() * m_zoom + m_scrollOffset.y());
}

//////////////////////////////////////////////////////////////////////////
QPointF QHyperGraphWidget::ViewToLocal(const QPoint& point)
{
    return QPointF((point.x() - m_scrollOffset.x()) / m_zoom, (point.y() - m_scrollOffset.y()) / m_zoom);
}

//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnHyperGraphEvent(IHyperNode* pINode, EHyperGraphEvent event)
{
    if (m_bIgnoreHyperGraphEvents)
    {
        return;
    }

    CHyperNode* pNode = static_cast<CHyperNode*>(pINode);
    switch (event)
    {
    case EHG_GRAPH_REMOVED:
        if (!m_pGraph->GetManager())
        {
            SetGraph(NULL);
        }
        break;
    case EHG_GRAPH_INVALIDATE:
        m_edgesToReroute.clear();
        m_pEditedNode = 0;
        m_pMultiEditedNodes.resize(0);
        m_pMouseOverNode = NULL;
        m_pMouseOverPort = NULL;
        m_DraggingBorderNodeInfo.m_pNode = 0;
        m_moveHelper.clear();
        InvalidateView();
        OnSelectionChange();
        break;
    case EHG_NODE_ADD:
        InvalidateView();
        break;
    case EHG_NODE_DELETE:
        if (pINode == static_cast<CHyperNode*>(m_pMouseOverNode))
        {
            m_pMouseOverNode = NULL;
            m_pMouseOverPort = NULL;
        }
        InvalidateView();
        break;
    case EHG_NODE_CHANGE:
    {
        InvalidateNode(pNode);
    }
    break;
    case EHG_NODE_CHANGE_DEBUG_PORT:
        InvalidateNode(pNode, true);
        if (m_pGraph && m_pGraph->FindNode(pNode->GetId()))
        {
            UpdateDebugCount(pNode);
        }
        break;
    }

    // Something in the graph changes so update group/prefab
    if (
        /* GRAPH EVENTS*/
        event == EHG_GRAPH_ADDED || event == EHG_GRAPH_UNDO_REDO || event == EHG_GRAPH_TOKENS_UPDATED ||
        event == EHG_GRAPH_INVALIDATE || event == EHG_GRAPH_EDGE_STATE_CHANGED || event == EHG_GRAPH_PROPERTIES_CHANGED ||
        /* NODE CHANGE EVENTS */
        event == EHG_NODE_DELETED || event == EHG_NODE_SET_NAME || event == EHG_NODE_ADD ||
        event == EHG_NODE_RESIZE || event == EHG_NODE_UPDATE_ENTITY || event == EHG_NODE_PROPERTIES_CHANGED ||
        event == EHG_NODES_PASTED
        )
    {
        PropagateChangesToGroupsAndPrefabs();
    }
}


//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::SetGraph(CHyperGraph* pGraph)
{
    if (pGraph == nullptr && m_pGraph == nullptr)
    {
        return;
    }

    if (m_pGraph == pGraph)
    {
        InvalidateView(true);
        return;
    }

    HideEditPort();

    m_edgesToReroute.clear();
    m_pEditedNode = 0;
    m_pMultiEditedNodes.resize(0);
    m_pMouseOverNode = NULL;
    m_pMouseOverPort = NULL;
    m_DraggingBorderNodeInfo.m_pNode = 0;
    m_moveHelper.clear();

    if (m_pGraph)
    {
        // CryLogAlways("QHyperGraphWidget: (1) Removing as listener from 0x%p before switching to 0x%p", m_pGraph, pGraph);
        m_pGraph->RemoveListener(this);
        // CryLogAlways("QHyperGraphWidget: (2) Removing as listener from 0x%p before switching to 0x%p", m_pGraph, pGraph);
    }

    bool bFitAll = true;

    m_pGraph = pGraph;
    if (m_pGraph)
    {
        QPoint soffset;
        if (m_pGraph->GetViewPosition(soffset, m_zoom))
        {
            bFitAll = false;
            InvalidateView();
        }
        m_scrollOffset = soffset;

        RerouteAllEdges();
        m_pGraph->AddListener(this);
        NotifyZoomChangeToNodes();

        // Invalidate all view.
        if (isVisible())
        {
            if (!m_bIsClosing)
            {
                if (bFitAll)
                {
                    InvalidateView(true);
                    OnCommandFitAll();
                }

                OnSelectionChange();
            }
        }

        UpdateFrozen();

        if (m_pGraph->IsNodeActivationModified())
        {
            update();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnCommandFitAll()
{
    QRect rc = this->geometry();

    if (rc.isEmpty())
    {
        m_bRefitGraphInOnSize = true;
        return;
    }

    SetZoom(1);
    m_scrollOffset = QPoint(0, 0);
    UpdateWorkingArea();
    QRect rcGraph = LocalToViewRect(m_workingArea);

    float zoom = min(float(rc.width()) / rcGraph.width(), float(rc.height()) / rcGraph.height());
    if (zoom < 1)
    {
        SetZoom(zoom);
        rcGraph = LocalToViewRect(m_workingArea); // used new zoom
    }

    if (rcGraph.width() < rc.width())
    {
        m_scrollOffset.setX(m_scrollOffset.x() + (rc.width() - rcGraph.width()) / 2);
    }
    if (rcGraph.height() < rc.height())
    {
        m_scrollOffset.setY(m_scrollOffset.y() + (rc.height() - rcGraph.height()) / 2);
    }

    m_scrollOffset.setX(m_scrollOffset.x() - rcGraph.left());
    m_scrollOffset.setY(m_scrollOffset.y() - rcGraph.top());

    InvalidateView();
}

////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::ShowAndSelectNode(CHyperNode* pToNode, bool bSelect)
{
    assert(pToNode != 0);

    if (!m_pGraph)
    {
        return;
    }

    QRect rc = geometry();
    QRectF cnodeRect = pToNode->GetRect();
    QRect nodeRect = LocalToViewRect(cnodeRect);

    //// check if fully inside
    //if (nodeRect.left >= rc.left &&
    //  nodeRect.top >= rc.top &&
    //  nodeRect.bottom <= rc.bottom &&
    //  nodeRect.right <= rc.right)
    //{
    //  // fully inside
    //  // do nothing yet...
    //}
    //else
    {
        if (rc.isEmpty())
        {
            m_bRefitGraphInOnSize = true;
        }
        else
        {
            m_scrollOffset = QPoint(0, 0);

            QRectF stRect = pToNode->GetRect();

            if (stRect.isEmpty())
            {
                pToNode->CalculateSize();
                stRect = pToNode->GetRect();
            }

            QRectF rcGraph = stRect;

            int maxWH = max(rcGraph.width(), rcGraph.height());
            if (maxWH > 0)
            {
                float fZoom = (float)(max(rc.width(), rc.height())) * 0.33f / maxWH;
                SetZoom(fZoom);
            }
            else
            {
                SetZoom(1.0f);
            }

            QRect qrcGraph = LocalToViewRect(cnodeRect);

            if (rcGraph.width() < rc.width())
            {
                m_scrollOffset.setX(m_scrollOffset.x() + (rc.width() - qrcGraph.width()) / 2);
            }
            if (rcGraph.height() < rc.height())
            {
                m_scrollOffset.setY(m_scrollOffset.y() + (rc.height() - qrcGraph.height()) / 2);
            }
            m_scrollOffset += QPoint(-qrcGraph.left() + GRID_SIZE, -qrcGraph.top() + GRID_SIZE);

            //m_scrollOffset.SetPoint(0,0);
            //CRect rcGraph = LocalToViewRect(pToNode->GetRect());
            //float zoom = (float)max(rc.Width(),rc.Height()) / (max(rcGraph.Width(),rcGraph.Height()) + GRID_SIZE*4);
            //if (zoom < 1)
            //  m_zoom = CLAMP( zoom, MIN_ZOOM, MAX_ZOOM );
            //rcGraph = LocalToViewRect(pToNode->GetRect());

            //if (rcGraph.Width() < rc.Width())
            //  m_scrollOffset.x += (rc.Width() - rcGraph.Width()) / 2;
            //if (rcGraph.Height() < rc.Height())
            //  m_scrollOffset.y += (rc.Height() - rcGraph.Height()) / 2;

            //m_scrollOffset.x += -rcGraph.left + GRID_SIZE;
            //m_scrollOffset.y += -rcGraph.top + GRID_SIZE;
        }
    }

    if (bSelect)
    {
        IHyperGraphEnumerator* pEnum = m_pGraph->GetNodesEnumerator();
        for (IHyperNode* pINode = pEnum->GetFirst(); pINode; pINode = pEnum->GetNext())
        {
            CHyperNode* pNode = (CHyperNode*)pINode;
            if (pNode->IsSelected())
            {
                pNode->SetSelected(false);
            }
        }
        pToNode->SetSelected(true);

        OnSelectionChange();
        pEnum->Release();
    }
    InvalidateView();
}

//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::DrawGrid(QPainter* gr, const QRect& updateRect)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_EDITOR);

    int gridStep = 10;

    float z = m_zoom;
    assert(z >= MIN_ZOOM);
    while (z < 0.99f)
    {
        z *= 2;
        gridStep *= 2;
    }

    // Draw grid line every 5 pixels.
    QRectF updLocalRect = ViewToLocalRect(updateRect);
    float startX = updLocalRect.x() - fmodf(updLocalRect.x(), gridStep);
    float startY = updLocalRect.y() - fmodf(updLocalRect.y(), gridStep);
    float stopX = startX + updLocalRect.width() * z;
    float stopY = startY + updLocalRect.height() * z;

    QPen gridPen(GRID_COLOR);
    gr->setPen(gridPen);

    // Draw vertical grid lines.
    for (float x = startX; x < stopX; x += gridStep)
    {
        gr->drawLine(x, startY, x, stopY);
    }

    // Draw horizontal grid lines.
    for (float y = startY; y < stopY; y += gridStep)
    {
        gr->drawLine(startX, y, stopX, y);
    }
}

//////////////////////////////////////////////////////////////////////////
QRectF QHyperGraphWidget::DrawArrow(QPainter* gr, CHyperEdge* pEdge, bool helper, int selection)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_EDITOR);

    // optimization: don't draw helper or label text when zoomed out
    static const float fHelperDrawMinimumZoom = 0.4f;
    QPointF pout = pEdge->pointOut;
    QPointF pin = pEdge->pointIn;
    EHyperEdgeDirection dout = pEdge->dirOut;
    EHyperEdgeDirection din = pEdge->dirIn;

    struct
    {
        float x1, y1;
        float x2, y2;
    }
    h[] =
    {
        { 0, -0.5f, 0, -0.5f },
        { 0, 0.5f, 0, 0.5f },
        { -0.5f, 0, -0.5f, 0 },
        { 0.5f, 0, 0.5f, 0 }
    };

    float dx = CLAMP(fabsf(pout.x() - pin.x()), 20.0f, 150.0f);
    float dy = CLAMP(fabsf(pout.y() - pin.y()), 20.0f, 150.0f);

    if (fabsf(pout.x() - pin.x()) < 0.0001f && fabsf(pout.y() - pin.y()) < 0.0001f)
    {
        return QRectF(0, 0, 0, 0);
    }

    QPointF pnts[6];

    pnts[0] = QPointF(pout.x(), pout.y());
    pnts[1] = QPointF(pnts[0].x() + h[dout].x1 * dx, pnts[0].y() + h[dout].y1 * dy);
    pnts[3] = QPointF(pin.x(), pin.y());
    pnts[2] = QPointF(pnts[3].x() + h[din].x2 * dx, pnts[3].y() + h[din].y2 * dy);
    QPointF center((pnts[1].x() + pnts[2].x()) * 0.5f, (pnts[1].y() + pnts[2].y()) * 0.5f);

    float zoom = m_zoom;
    if (zoom > 1.0f)
    {
        zoom = 1.0f;
    }

    QColor color;
    QPen pen(color);

    switch (selection)
    {
    case 1: // incoming
        color = ARROW_SEL_COLOR_IN;
        break;
    case 2: // outgoing
        color = ARROW_SEL_COLOR_OUT;
        break;
    case 3: // incoming+outgoing
        color = QColor(244, 244, 244);
        break;
    case 4: // debugging port activation
        color = gSettings.hyperGraphColors.colorPortDebugging;
        break;
    case 5: // AG Modifier Link Color
        color = QColor(44, 44, 90);
        //      pen.SetDashStyle(Gdiplus::DashStyleDot);
        pen.setStyle(Qt::DotLine);
        pen.setWidthF(1.5f);
        break;
    case 6: // AG Regular Link Color
        color = QColor(40, 40, 44);
        break;
    case 7: // Selection Tree Link Color
        color = QColor(0, 0, 0);
    case 8:
        color = PORT_EDGE_HIGHLIGHT;
        pen.setWidthF(1.5f);
        break;
    default:
        color = ARROW_COLOR;
        break;
    }

    if (!pEdge->enabled)
    {
        color = ARROW_DIS_COLOR;
        pen.setWidthF(2.0f);
        pen.setStyle(Qt::DotLine);
        //      pen.SetDashStyle(Gdiplus::DashStyleDot);
    }

    pen.setColor(color);

    pen.setCapStyle(Qt::RoundCap);
    gr->save();
    gr->setPen(pen);

    const float HELPERSIZE = 4.0f;
    QRectF helperRect(center.x() - HELPERSIZE / 2, center.y() - HELPERSIZE / 2, HELPERSIZE, HELPERSIZE);

    if (m_bSplineArrows)
    {
        if (pEdge->cornerModified)
        {
            float cdx = pEdge->cornerPoints[0].x() - center.x();
            float cdy = pEdge->cornerPoints[0].y() - center.y();

            dx += cdx;
            dy += cdy;

            pnts[0] = QPointF(pout.x(), pout.y());
            pnts[1] = QPointF(pnts[0].x() + h[dout].x1 * dx, pnts[0].y() + h[dout].y1 * dy);
            pnts[3] = QPointF(pin.x(), pin.y());
            pnts[2] = QPointF(pnts[3].x() + h[din].x2 * dx, pnts[3].y() + h[din].y2 * dy);
        }
        else
        {
            pEdge->cornerPoints[0].rx() = center.x();
            pEdge->cornerPoints[0].ry() = center.y();
            pEdge->originalCenter = pEdge->cornerPoints[0];
        }

        QPainterPath p;
        p.moveTo(pnts[0]);
        p.cubicTo(pnts[1], pnts[2], pnts[3]);
        gr->drawPath(p);

        qreal a = -p.angleAtPercent(.95) * M_PI / 180.;
        p = QPainterPath();
        const qreal arrowSize = 10;
        const qreal arrowAngle = 0.5235; // 30deg
        p.moveTo(pnts[3]);
        p.lineTo(pnts[3].x() - std::cos(a + arrowAngle) * arrowSize, pnts[3].y() - std::sin(a + arrowAngle) * arrowSize);
        p.lineTo(pnts[3].x() - std::cos(a - arrowAngle) * arrowSize, pnts[3].y() - std::sin(a - arrowAngle) * arrowSize);
        gr->fillPath(p, color);

        if (helper && m_zoom > fHelperDrawMinimumZoom)
        {
            gr->drawEllipse(helperRect);
        }

        if (4 == selection && pEdge && m_zoom > fHelperDrawMinimumZoom)
        {
            if (pEdge->debugCount > 1)
            {
                QFont font("Tahoma", 9.0f);
                QPointF point(center.x(), center.y() - 15.0f);
                QBrush brush(color);

                gr->setFont(font);
                gr->setBrush(brush);
                gr->drawText(point, QString("%1").arg(pEdge->debugCount));
            }
        }
    }
    else
    {
        float w = 20 + pEdge->nPortOut * 10;
        if (w > fabs((pout.x() - pin.x()) / 2))
        {
            w = fabs((pout.x() - pin.x()) / 2);
        }

        if (!pEdge->cornerModified)
        {
            pEdge->cornerW = w;
            pEdge->cornerH = 40;
        }
        w = pEdge->cornerW;
        float ph = pEdge->cornerH;

        if (pin.x() >= pout.x())
        {
            pnts[0] = pout;
            pnts[1] = QPointF(pout.x() + w, pout.y());
            pnts[2] = QPointF(pout.x() + w, pin.y());
            pnts[3] = pin;
            gr->drawPolyline(pnts, 4);

            pEdge->cornerPoints[0] = QPointF(pnts[1].x(), pnts[1].y());
            pEdge->cornerPoints[1] = QPointF(pnts[2].x(), pnts[2].y());
            pEdge->cornerPoints[2] = QPointF(0, 0);
            pEdge->cornerPoints[3] = QPointF(0, 0);
        }
        else
        {
            pnts[0] = pout;
            pnts[1] = QPointF(pout.x() + w, pout.y());
            pnts[2] = QPointF(pout.x() + w, pout.y() + ph);
            pnts[3] = QPointF(pin.x() - w, pout.y() + ph);
            pnts[4] = QPointF(pin.x() - w, pin.y());
            pnts[5] = pin;
            gr->drawPolyline(pnts, 6);

            pEdge->cornerPoints[0] = QPointF(pnts[1].x(), pnts[1].y());
            ;
            pEdge->cornerPoints[1] = QPointF(pnts[2].x(), pnts[2].y());
            ;
            pEdge->cornerPoints[2] = QPointF(pnts[3].x(), pnts[3].y());
            ;
            pEdge->cornerPoints[3] = QPointF(pnts[4].x(), pnts[4].y());
            ;
        }
        if (helper)
        {
            gr->drawRect(pnts[1].x() - HELPERSIZE / 2, pnts[1].y() - HELPERSIZE / 2, HELPERSIZE, HELPERSIZE);
            gr->drawRect(pnts[2].x() - HELPERSIZE / 2, pnts[2].y() - HELPERSIZE / 2, HELPERSIZE, HELPERSIZE);
        }
    }

    gr->restore();

    return helperRect;
}


////////////////////////////////////////////////////////////////////////////
CHyperNode* QHyperGraphWidget::GetNodeAtPoint(const QPoint& point)
{
    if (!m_pGraph)
    {
        return 0;
    }
    std::multimap<int, CHyperNode*> nodes;
    if (!GetNodesInRect(QRect(point, QSize(1, 1)), nodes))
    {
        return 0;
    }

    return nodes.rbegin()->second;
}


////////////////////////////////////////////////////////////////////////////
CHyperNode* QHyperGraphWidget::GetMouseSensibleNodeAtPoint(const QPoint& point)
{
    CHyperNode* pNode = GetNodeAtPoint(point);
    if (pNode && pNode->GetObjectAt(ViewToLocal(point)) == eSOID_InputTransparent)
    {
        pNode = NULL;
    }

    return pNode;
}

//////////////////////////////////////////////////////////////////////////
bool QHyperGraphWidget::GetNodesInRect(const QRect& viewRect, std::multimap<int, CHyperNode*>& nodes, bool bFullInside)
{
    if (!m_pGraph)
    {
        return false;
    }
    bool bFirst = true;
    QRectF localRect = ViewToLocalRect(viewRect);
    IHyperGraphEnumerator* pEnum = m_pGraph->GetNodesEnumerator();
    nodes.clear();

    for (IHyperNode* pINode = pEnum->GetFirst(); pINode; pINode = pEnum->GetNext())
    {
        CHyperNode* pNode = (CHyperNode*)pINode;
        const QRectF& qitemRect = pNode->GetRect();

        if (!localRect.intersects(qitemRect))
        {
            continue;
        }
        if (bFullInside && !localRect.contains(qitemRect))
        {
            continue;
        }

        nodes.insert(std::make_pair(pNode->GetDrawPriority(), pNode));
    }
    pEnum->Release();

    return !nodes.empty();
}

//////////////////////////////////////////////////////////////////////////
bool QHyperGraphWidget::GetSelectedNodes(std::vector<CHyperNode*>& nodes, SelectionSetType setType)
{
    if (!m_pGraph)
    {
        return false;
    }

    bool onlyParents = false;
    bool includeRelatives = false;
    switch (setType)
    {
    case SELECTION_SET_INCLUDE_RELATIVES:
        includeRelatives = true;
        break;
    case SELECTION_SET_ONLY_PARENTS:
        onlyParents = true;
        break;
    case SELECTION_SET_NORMAL:
        break;
    }

    if (includeRelatives)
    {
        std::set<CHyperNode*> nodeSet;

        IHyperGraphEnumerator* pEnum = m_pGraph->GetNodesEnumerator();
        for (IHyperNode* pINode = pEnum->GetFirst(); pINode; pINode = pEnum->GetNext())
        {
            CHyperNode* pNode = (CHyperNode*)(pINode);
            if (pNode->IsSelected())
            {
                //nodes.push_back( pNode );
                IHyperGraphEnumerator* pRelativesEnum = pNode->GetRelatedNodesEnumerator();
                for (IHyperNode* pRelative = pRelativesEnum->GetFirst(); pRelative; pRelative = pRelativesEnum->GetNext())
                {
                    nodeSet.insert(static_cast<CHyperNode*>(pRelative));
                }
                pRelativesEnum->Release();
            }
        }
        pEnum->Release();

        nodes.clear();
        nodes.reserve(nodeSet.size());
        std::copy(nodeSet.begin(), nodeSet.end(), std::back_inserter(nodes));
    }
    else
    {
        IHyperGraphEnumerator* pEnum = m_pGraph->GetNodesEnumerator();
        nodes.clear();
        for (IHyperNode* pINode = pEnum->GetFirst(); pINode; pINode = pEnum->GetNext())
        {
            CHyperNode* pNode = (CHyperNode*)pINode;
            if (pNode->IsSelected() && (!onlyParents || pNode->GetParent() == 0))
            {
                nodes.push_back(pNode);
            }
        }
        pEnum->Release();
    }

    return !nodes.empty();
}

////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::ClearSelection()
{
    if (!m_pGraph)
    {
        return;
    }

    IHyperGraphEnumerator* pEnum = m_pGraph->GetNodesEnumerator();
    for (IHyperNode* pINode = pEnum->GetFirst(); pINode; pINode = pEnum->GetNext())
    {
        CHyperNode* pNode = (CHyperNode*)pINode;
        if (pNode->IsSelected())
        {
            pNode->SetSelected(false);
        }
    }
    OnSelectionChange();
    pEnum->Release();
}

//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::MoveSelectedNodes(const QPoint& offset)
{
    if (offset.isNull())
    {
        return;
    }

    std::vector<CHyperNode*> nodes;
    if (!GetSelectedNodes(nodes))
    {
        return;
    }

    QRectF bounds;

    for (int i = 0; i < nodes.size(); i++)
    {
        // Only move parent nodes.
        if (nodes[i]->GetParent() == 0)
        {
            if (m_moveHelper.find(nodes[i]) == m_moveHelper.end())
            {
                m_moveHelper[nodes[i]] = nodes[i]->GetPos();
            }

            if (!i)
            {
                bounds = nodes[i]->GetRect();
            }
            else
            {
                bounds = bounds.united(nodes[i]->GetRect());
            }
            //InvalidateRect( LocalToViewRect(rect),FALSE );

            QPointF pos = nodes[i]->GetPos();
            QPointF firstPos = m_moveHelper[nodes[i]];
            pos = firstPos + offset / m_zoom;

            // Snap rectangle to the grid.
            if (nodes[i]->IsGridBound())
            {
                pos.setX(floor(((float)pos.x() / GRID_SIZE) + 0.5f) * GRID_SIZE);
                pos.setY(floor(((float)pos.y() / GRID_SIZE) + 0.5f) * GRID_SIZE);
            }

            nodes[i]->SetPos(pos);
            InvalidateNode(nodes[i]);
            IHyperGraphEnumerator* pRelativesEnum = nodes[i]->GetRelatedNodesEnumerator();
            for (IHyperNode* pRelative = pRelativesEnum->GetFirst(); pRelative; pRelative = pRelativesEnum->GetNext())
            {
                CHyperNode* pRelativeNode = static_cast<CHyperNode*>(pRelative);
                m_workingArea = m_workingArea.united(pRelativeNode->GetRect());
                bounds = bounds.united(pRelativeNode->GetRect());
            }
            pRelativesEnum->Release();
        }
    }
    QRect invRect = LocalToViewRect(bounds);
    invRect.adjust(-32, -32, 32, 32);
    update(invRect);
    SetScrollExtents();
}

////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnSelectEntity()
{
    std::vector<CHyperNode*> nodes;
    if (!GetSelectedNodes(nodes))
    {
        return;
    }

    CUndo undo("Select Object(s)");
    GetIEditor()->ClearSelection();
    for (int i = 0; i < nodes.size(); i++)
    {
        // only can CFlowNode* if not a comment, argh...
        if (!nodes[i]->IsEditorSpecialNode())
        {
            CFlowNode* pFlowNode = (CFlowNode*)nodes[i];
            if (pFlowNode->GetEntity())
            {
                GetIEditor()->SelectObject(pFlowNode->GetEntity());
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////
//KDAB_PORT_UNUSED
//void QHyperGraphWidget::OnToggleMinimize()
//{
//  std::vector<CHyperNode*> nodes;
//  if (!GetSelectedNodes(nodes))
//      return;
//
//  for (int i = 0; i < nodes.size(); i++)
//  {
//      OnToggleMinimizeNode(nodes[i]);
//  }
//}

//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnCreateGroupFromSelection()
{
    CUndo undo("Group operation");

    // Fetch the vector of all selected nodes
    std::vector<CHyperNode*> selectedNodes;
    GetSelectedNodes(selectedNodes, SELECTION_SET_ONLY_PARENTS);

    // Maintains the count of groups that are a part of the current selection
    int groupCount = 0;

    // If there is a valid group in the current selection this will hold a pointer to it
    // It is important to note that this is only valid if there is only one group in the
    // current selection of nodes as multiple groups in a selection is an error condition
    CHyperNode* currentGroup = nullptr;

    // If there are some selected nodes
    if (!selectedNodes.empty())
    {
        for (auto currentNode : selectedNodes)
        {
            // If any of the nodes in the current selection are already in a Group
            // they cannot be added to another group
            if (currentNode->GetBlackBox())
            {
                Warning("One of the selected nodes is already in a Group");
                return;
            }

            // If the current node is a group
            if (currentNode->GetClassName() == CBlackBoxNode::GetClassType())
            {
                // Increment the group count
                groupCount++;

                // Set the current group
                currentGroup = currentNode;
            }

            // If there are multiple groups in this selection
            if (groupCount > 1)
            {
                Warning("You cannot add one Group to another Group");
                return;
            }
        }

        // If there were no groups in the current selection
        if (groupCount == 0)
        {
            // A new Group must be created
            CHyperNode* createdGroupNode = CreateNode(CBlackBoxNode::GetClassType(), QPoint(0, 0));

            if (createdGroupNode)
            {
                auto createdGroup = static_cast<CBlackBoxNode*>(createdGroupNode);

                // Add all selected nodes to this new Group
                for (auto currentNode : selectedNodes)
                {
                    createdGroup->AddNode(currentNode);
                }
            }
        }

        // If there is ONE selected group
        else if (groupCount == 1)
        {
            // If only a group has been selected
            if (selectedNodes.size() == 1)
            {
                Warning("You must select at least one node in addition to the Group");
                return;
            }
            // get the Selected group as a Group Node
            auto selectedGroup = static_cast<CBlackBoxNode*>(currentGroup);

            // Add all other selected nodes to this Group
            for (auto currentNode : selectedNodes)
            {
                if (currentNode != currentGroup)
                {
                    selectedGroup->AddNode(currentNode);
                }
            }

            // Invalidate the currently selected group to force a redraw
            selectedGroup->Invalidate(true);
        }
    }
    // If there are no nodes selected
    else
    {
        // Display a warning message
        Warning("You must select nodes to add to group");
    }

    // View will be modified by this action and needs to be repainted
    InvalidateView();
}

//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnRemoveGroupsInSelection()
{
    CUndo undo("Ungroup operation");

    // Fetch the vector of all selected nodes
    std::vector<CHyperNode*> selectedNodes;
    GetSelectedNodes(selectedNodes, SELECTION_SET_ONLY_PARENTS);

    // Groups in current selection
    std::vector<CHyperNode*> groupsInSelection;

    // Children of Groups in selection
    std::vector<CHyperNode*> nodesInSelection;

    if (!selectedNodes.empty())
    {
        // Sort selected nodes into black box nodes and children nodes
        for (auto currentNode : selectedNodes)
        {
            // If the node is the child of a black box
            if (currentNode->GetBlackBox())
            {
                nodesInSelection.push_back(currentNode);
            }
            // Or if the node is a back box
            else if (currentNode->GetClassName() == CBlackBoxNode::GetClassType())
            {
                groupsInSelection.push_back(currentNode);
            }
            else
            {
                Warning("One of the selected nodes is not a Group nor a part of a Group");
                return;
            }
        }

        CUndo undo("Ungroup action");
        m_pGraph->RecordUndo();

        // Remove entire groups
        for (auto currentGroup : groupsInSelection)
        {
            m_pGraph->RemoveNode(currentGroup);
        }

        // Remove nodes from group
        for (auto looseNode : nodesInSelection)
        {
            if (looseNode->GetBlackBox())
            {
                looseNode->GetBlackBox()->RemoveNode(looseNode);
            }
        }

        // Selection will be modified by this action
        OnSelectionChange();

        // View will be modified by this action and needs to be repainted
        InvalidateView();
    }
    else
    {
        // Display a warning message
        Warning("You must select nodes to Ungroup");
    }
}


////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnToggleMinimizeNode(CHyperNode* pNode)
{
    bool bMinimize = false;
    if (!bMinimize)
    {
        for (int i = 0; i < pNode->GetInputs()->size(); i++)
        {
            CHyperNodePort* pPort = &pNode->GetInputs()->at(i);
            if (pPort->bVisible && (pPort->nConnected == 0))
            {
                bMinimize = true;
                break;
            }
        }
    }
    if (!bMinimize)
    {
        for (int i = 0; i < pNode->GetOutputs()->size(); i++)
        {
            CHyperNodePort* pPort = &pNode->GetOutputs()->at(i);
            if (pPort->bVisible && (pPort->nConnected == 0))
            {
                bMinimize = true;
                break;
            }
        }
    }

    bool bVisible = !bMinimize;
    {
        for (int i = 0; i < pNode->GetInputs()->size(); i++)
        {
            CHyperNodePort* pPort = &pNode->GetInputs()->at(i);
            pPort->bVisible = bVisible;
        }
    }
    {
        for (int i = 0; i < pNode->GetOutputs()->size(); i++)
        {
            CHyperNodePort* pPort = &pNode->GetOutputs()->at(i);
            pPort->bVisible = bVisible;
        }
    }
    InvalidateNode(pNode, true);
    update();
}

void QHyperGraphWidget::AddEntityMenu(CHyperNode* pNode, QMenu& menu)
{
    bool bAddMenu = pNode->CheckFlag(EHYPER_NODE_ENTITY);

    std::vector<CHyperNode*> nodes;
    if (!bAddMenu && GetSelectedNodes(nodes))
    {
        std::vector<CHyperNode*>::const_iterator iter = nodes.begin();
        for (; iter != nodes.end(); ++iter)
        {
            CHyperNode* const pNode = (*iter);
            if (pNode->CheckFlag(EHYPER_NODE_ENTITY))
            {
                bAddMenu = true;
                break;
            }
        }
    }

    if (bAddMenu)
    {
        menu.addAction(tr("Assign selected entity"))->setData(ID_GRAPHVIEW_TARGET_SELECTED_ENTITY);
        menu.addAction(tr("Assign graph entity"))->setData(ID_GRAPHVIEW_TARGET_GRAPH_ENTITY);
        menu.addSeparator();
        menu.addAction(tr("Unassign entity"))->setData(ID_GRAPHVIEW_TARGET_UNSELECT_ENTITY);
        menu.addSeparator();
    }
}

void QHyperGraphWidget::HandleEntityMenu(CHyperNode* pNode, const uint cmd)
{
    switch (cmd)
    {
    case ID_GRAPHVIEW_TARGET_SELECTED_ENTITY:
    case ID_GRAPHVIEW_TARGET_GRAPH_ENTITY:
    case ID_GRAPHVIEW_TARGET_UNSELECT_ENTITY:
    {
        // update node under cursor
        UpdateNodeEntity(pNode, cmd);

        // update all selected nodes
        std::vector<CHyperNode*> nodes;
        if (GetSelectedNodes(nodes))
        {
            std::vector<CHyperNode*>::const_iterator iter = nodes.begin();
            for (; iter != nodes.end(); ++iter)
            {
                UpdateNodeEntity((*iter), cmd);
            }
        }
    }
    break;
    }
}
//
////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::ShowContextMenu(const QPoint& point, CHyperNode* pNode)
{
    if (!m_pGraph)
    {
        return;
    }

    QMenu menu;

    QStringList classes;
    SClassMenuGroup classMenu;
    QMenu inputsMenu, outputsMenu, commentsMenu, newFGModuleFromSelectionMenu, selectionMenu;

    const QPoint screenPoint = mapToGlobal(point);

    // only add cut, delete... entries to the context menu if it is not the port context menu
    bool isPort = false;

    if (pNode && pNode->IsFlowNode())
    {
        CHyperNodePort* pPort = pNode->GetPortAtPoint(ViewToLocal(point));

        if (pPort)
        {
            isPort = true;
            QMenu menu;

            if (pPort->bInput == 1 && pPort->nPortIndex == 0)
            {
                if (m_pGraph->GetAIAction() != 0)
                {
                    menu.addAction(tr("Assign User entity"))->setData(ID_GRAPHVIEW_TARGET_GRAPH_ENTITY);
                    menu.addAction(tr("Assign Object entity"))->setData(ID_GRAPHVIEW_TARGET_GRAPH_ENTITY2);
                    menu.addSeparator();
                    menu.addAction(tr("Unassign entity"))->setData(ID_GRAPHVIEW_TARGET_UNSELECT_ENTITY);
                    menu.addSeparator();
                }
            }

            AddEntityMenu(pNode, menu);

            CFlowNode* pFlowNode = static_cast<CFlowNode*>(pNode);
            CFlowGraphDebuggerEditor* pFlowgraphDebuggerEditor = GetIEditor()->GetFlowGraphDebuggerEditor();
            assert(pFlowgraphDebuggerEditor);

            const bool portHasBreakPoint = pFlowgraphDebuggerEditor->HasBreakpoint(pFlowNode, pPort);
            const bool nodeHasBreakPoints = pFlowgraphDebuggerEditor->HasBreakpoint(pFlowNode->GetIFlowGraph(), pFlowNode->GetFlowNodeId());
            const bool graphHasBreakPoints = pFlowgraphDebuggerEditor->HasBreakpoint(pFlowNode->GetIFlowGraph());

            QAction* action = menu.addAction(tr("Add Breakpoint"));
            action->setData(ID_GRAPHVIEW_ADD_BREAKPOINT);
            action->setEnabled(!(portHasBreakPoint || (pPort->nConnected == 0 && pPort->bInput)));
            action = menu.addAction(tr("Remove Breakpoint"));
            action->setData(ID_GRAPHVIEW_REMOVE_BREAKPOINT);
            action->setEnabled(portHasBreakPoint);
            menu.addSeparator();
            action = menu.addAction(tr("Remove Breakpoints For Node"));
            action->setData(ID_GRAPHVIEW_REMOVE_BREAKPOINTS_FOR_NODE);
            action->setEnabled(nodeHasBreakPoints);
            action = menu.addAction(tr("Remove Breakpoint For Graph"));
            action->setData(ID_GRAPHVIEW_REMOVE_ALL_BREAKPOINTS);
            action->setEnabled(graphHasBreakPoints);

            menu.addSeparator();

            SFlowAddress addr;
            addr.node = pFlowNode->GetFlowNodeId();
            addr.port = pPort->nPortIndex;
            addr.isOutput = !pPort->bInput;

            IFlowGraphDebuggerConstPtr pFlowgraphDebugger = GetIFlowGraphDebuggerPtr();
            const bool bIsEnabled = pFlowgraphDebugger ? pFlowgraphDebugger->IsBreakpointEnabled(pFlowNode->GetIFlowGraph(), addr) : false;
            const bool bIsTracepoint = pFlowgraphDebugger ? pFlowgraphDebugger->IsTracepoint(pFlowNode->GetIFlowGraph(), addr) : false;

            action = menu.addAction(tr("Enabled"));
            action->setData(ID_BREAKPOINT_ENABLE);
            action->setCheckable(true);
            action->setChecked(bIsEnabled && portHasBreakPoint);
            action->setEnabled(portHasBreakPoint);
            action = menu.addAction(tr("Tracepoint"));
            action->setData(ID_TRACEPOINT_ENABLE);
            action->setCheckable(true);
            action->setChecked(bIsTracepoint && portHasBreakPoint);
            action->setEnabled(portHasBreakPoint);

            action = menu.exec(screenPoint);
            const uint cmd = action ? action->data().toInt() : 0;
            switch (cmd)
            {
            case ID_GRAPHVIEW_ADD_BREAKPOINT:
            {
                if (false == portHasBreakPoint && (pPort->nConnected || !pPort->bInput))
                {
                    pFlowgraphDebuggerEditor->AddBreakpoint(pFlowNode, pPort);
                }
            }
            break;
            case ID_GRAPHVIEW_REMOVE_BREAKPOINT:
            {
                if (portHasBreakPoint)
                {
                    pFlowgraphDebuggerEditor->RemoveBreakpoint(pFlowNode, pPort);
                }
            }
            break;
            case ID_GRAPHVIEW_REMOVE_BREAKPOINTS_FOR_NODE:
            {
                pFlowgraphDebuggerEditor->RemoveAllBreakpointsForNode(pFlowNode);
            }
            break;
            case ID_GRAPHVIEW_REMOVE_ALL_BREAKPOINTS:
            {
                pFlowgraphDebuggerEditor->RemoveAllBreakpointsForGraph(pFlowNode->GetIFlowGraph());
            }
            break;
            case ID_BREAKPOINT_ENABLE:
            {
                pFlowgraphDebuggerEditor->EnableBreakpoint(pFlowNode, pPort, !bIsEnabled);
            }
            break;
            case ID_TRACEPOINT_ENABLE:
            {
                pFlowgraphDebuggerEditor->EnableTracepoint(pFlowNode, pPort, !bIsTracepoint);
            }
            break;
            case ID_GRAPHVIEW_TARGET_GRAPH_ENTITY2:
            {
                CFlowNode* pFlowNode = (CFlowNode*)pNode;
                pFlowNode->SetFlag(EHYPER_NODE_GRAPH_ENTITY, false);
                pFlowNode->SetFlag(EHYPER_NODE_GRAPH_ENTITY2, true);
            }
            break;
            default:
                HandleEntityMenu(pNode, cmd);
                break;
            }
        }
        else
        {
            AddEntityMenu(pNode, menu);

            menu.addAction(tr("Rename"))->setData(ID_GRAPHVIEW_RENAME);
            menu.addSeparator();

            inputsMenu.setTitle(tr("Inputs"));
            outputsMenu.setTitle(tr("Outputs"));
            menu.addMenu(&inputsMenu);
            menu.addMenu(&outputsMenu);

            //////////////////////////////////////////////////////////////////////////
            // Inputs.
            //////////////////////////////////////////////////////////////////////////
            for (int i = 0; i < pNode->GetInputs()->size(); i++)
            {
                CHyperNodePort* pPort = &pNode->GetInputs()->at(i);
                QAction* action = inputsMenu.addAction(pPort->GetHumanName());
                action->setData(BASE_INPUTS_CMD + i);
                action->setCheckable(true);
                action->setChecked(pPort->bVisible || pPort->nConnected != 0);
                action->setEnabled(pPort->nConnected == 0);
            }
            {
                int numEnabled = 0;
                int numDisabled = 0;
                std::vector<CHyperEdge*> edges;
                if (m_pGraph->FindEdges(pNode, edges))
                {
                    for (int i = 0; i < edges.size(); ++i)
                    {
                        if (pNode == m_pGraph->FindNode(edges[i]->nodeIn))
                        {
                            if (edges[i]->enabled)
                            {
                                numEnabled++;
                            }
                            else
                            {
                                numDisabled++;
                            }
                        }
                    }
                }

                bool isPaste = false;
                CClipboard clipboard(this);
                XmlNodeRef node = clipboard.Get();
                if (node && node->isTag("GraphNodeInputLinks"))
                {
                    isPaste = true;
                }

                inputsMenu.addSeparator();
                QAction* action = inputsMenu.addAction(tr("Disable Links"));
                action->setData(ID_INPUTS_DISABLE_LINKS);
                action->setEnabled(numEnabled);
                action = inputsMenu.addAction(tr("Enable Links"));
                action->setData(ID_INPUTS_ENABLE_LINKS);
                action->setEnabled(numDisabled);
                inputsMenu.addSeparator();
                action = inputsMenu.addAction(tr("Cut Links"));
                action->setData(ID_INPUTS_CUT_LINKS);
                action->setEnabled(numEnabled || numDisabled);
                action = inputsMenu.addAction(tr("Copy Links"));
                action->setData(ID_INPUTS_COPY_LINKS);
                action->setEnabled(numEnabled || numDisabled);
                action = inputsMenu.addAction(tr("Paste Links"));
                action->setData(ID_INPUTS_PASTE_LINKS);
                action->setEnabled(isPaste);
                action = inputsMenu.addAction(tr("Delete Links"));
                action->setData(ID_INPUTS_DELETE_LINKS);
                action->setEnabled(numEnabled || numDisabled);
            }
            inputsMenu.addSeparator();
            inputsMenu.addAction(tr("Show All"))->setData(ID_INPUTS_SHOW_ALL);
            inputsMenu.addAction(tr("Hide All"))->setData(ID_INPUTS_HIDE_ALL);
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Outputs.
            //////////////////////////////////////////////////////////////////////////
            for (int i = 0; i < pNode->GetOutputs()->size(); i++)
            {
                CHyperNodePort* pPort = &pNode->GetOutputs()->at(i);
                QAction* action = outputsMenu.addAction(pPort->GetHumanName());
                action->setData(BASE_OUTPUTS_CMD + i);
                action->setCheckable(true);
                action->setChecked(pPort->bVisible || pPort->nConnected != 0);
                action->setEnabled(pPort->nConnected == 0);
            }

            {
                int numEnabled = 0;
                int numDisabled = 0;
                std::vector<CHyperEdge*> edges;
                if (m_pGraph->FindEdges(pNode, edges))
                {
                    for (int i = 0; i < edges.size(); ++i)
                    {
                        if (pNode == m_pGraph->FindNode(edges[i]->nodeOut))
                        {
                            if (edges[i]->enabled)
                            {
                                numEnabled++;
                            }
                            else
                            {
                                numDisabled++;
                            }
                        }
                    }
                }

                bool isPaste = false;
                CClipboard clipboard(this);
                XmlNodeRef node = clipboard.Get();
                if (node && node->isTag("GraphNodeOutputLinks"))
                {
                    isPaste = true;
                }

                outputsMenu.addSeparator();
                QAction* action = outputsMenu.addAction(tr("Disable Links"));
                action->setData(ID_OUTPUTS_DISABLE_LINKS);
                action->setEnabled(numEnabled);
                action = outputsMenu.addAction(tr("Enable Links"));
                action->setData(ID_OUTPUTS_ENABLE_LINKS);
                action->setEnabled(numDisabled);
                outputsMenu.addSeparator();
                action = outputsMenu.addAction(tr("Cut Links"));
                action->setData(ID_OUTPUTS_CUT_LINKS);
                action->setEnabled(numEnabled || numDisabled);
                action = outputsMenu.addAction(tr("Copy Links"));
                action->setData(ID_OUTPUTS_COPY_LINKS);
                action->setEnabled(numEnabled || numDisabled);
                action = outputsMenu.addAction(tr("Paste Links"));
                action->setData(ID_OUTPUTS_PASTE_LINKS);
                action->setEnabled(isPaste);
                action = outputsMenu.addAction(tr("Delete Links"));
                action->setData(ID_OUTPUTS_DELETE_LINKS);
                action->setEnabled(numEnabled || numDisabled);
            }

            outputsMenu.addSeparator();
            outputsMenu.addAction(tr("Show All"))->setData(ID_OUTPUTS_SHOW_ALL);
            outputsMenu.addAction(tr("Hide All"))->setData(ID_OUTPUTS_HIDE_ALL);
            //////////////////////////////////////////////////////////////////////////

            menu.addSeparator();
        }
    }
    else
    {
        PopulateClassMenu(classMenu, classes);
        classMenu.pMenu->setTitle(tr("Add Node"));
        menu.addMenu(classMenu.pMenu.get());
        menu.addAction(tr("Add Selected Entity"))->setData(ID_GRAPHVIEW_ADD_SELECTED_ENTITY);
        if (m_pGraph->GetAIAction() == 0)
        {
            menu.addAction(tr("Add Graph Default Entity"))->setData(ID_GRAPHVIEW_ADD_DEFAULT_ENTITY);
        }
        commentsMenu.setTitle(tr("Add Comment"));
        menu.addMenu(&commentsMenu);

        commentsMenu.addAction(tr("Add Simple Comment"))->setData(ID_GRAPHVIEW_ADD_COMMENT);
        commentsMenu.addAction(tr("Add Comment Box"))->setData(ID_GRAPHVIEW_ADD_COMMENTBOX);
        //commentsMenu.addAction(tr("Add BlackBox"))->setData(ID_GRAPHVIEW_ADD_BLACK_BOX);

        menu.addAction(tr("Add Track Event Node"))->setData(ID_GRAPHVIEW_ADD_TRACKEVENT);
        menu.addAction(tr("Add Start Node"))->setData(ID_GRAPHVIEW_ADD_STARTNODE);
        menu.addSeparator();

        menu.addAction(tr("Group"))->setData(ID_GRAPHVIEW_ADD_BLACK_BOX);
        menu.addAction(tr("Ungroup"))->setData(ID_GRAPHVIEW_UNGROUP);
    }

    // not doing this right now -> ensuring consistent (=broken!) UI
    // CClipboard clipboard(this);
    // int pasteFlags = clipboard.IsEmpty() ? MF_GRAYED : 0;

    if (false == isPort)
    {
        menu.addAction(tr("Cut"))->setData(ID_EDIT_CUT);
        menu.addAction(tr("Copy"))->setData(ID_EDIT_COPY);
        menu.addAction(tr("Paste"))->setData(ID_EDIT_PASTE);
        menu.addAction(tr("Paste with Links"))->setData(ID_EDIT_PASTE_WITH_LINKS);
        menu.addAction(tr("Delete"))->setData(ID_EDIT_DELETE);
        menu.addSeparator();
        //menu.addAction(tr("Export Selected"))->setData(ID_FILE_EXPORTSELECTION);

        if (m_pGraph->IsSelectedAny())
        {
            selectionMenu.setTitle(tr("Selection"));
            menu.addMenu(&selectionMenu);

            selectionMenu.addAction(tr("Export Selected Nodes"))->setData(ID_FILE_EXPORTSELECTION);
            selectionMenu.addAction(tr("Select Assigned Entity"))->setData(ID_GRAPHVIEW_SELECT_ENTITY);
            selectionMenu.addAction(tr("Select and Goto Assigned Entity"))->setData(ID_GRAPHVIEW_SELECT_AND_GOTO_ENTITY);

            newFGModuleFromSelectionMenu.setTitle(tr("Create Module from Selection"));
            newFGModuleFromSelectionMenu.addAction(tr("Global"))->setData(ID_CREATE_GLOBAL_FG_MODULE_FROM_SELECTION);
            newFGModuleFromSelectionMenu.addAction(tr("Level"))->setData(ID_CREATE_LEVEL_FG_MODULE_FROM_SELECTION);
        }

        menu.addAction(tr("Import"))->setData(ID_FILE_IMPORT);
        //menu.addAction(tr("Select Assigned Entity"))->setData(ID_GRAPHVIEW_SELECT_ENTITY);

        QAction* action = menu.addAction(tr("Show Spline Arrows"));
        action->setData(ID_GRAPHVIEW_SPLINES);
        action->setCheckable(true);
        action->setChecked(m_bSplineArrows);
        action = menu.addAction(tr("Show Edges On Top Of Nodes"));
        action->setData(ID_GRAPHVIEW_EDGES_ON_TOP_OF_NODES);
        action->setCheckable(true);
        action->setChecked(gSettings.bFlowGraphEdgesOnTopOfNodes);
        menu.addAction(tr("Fit Graph to View"))->setData(ID_GRAPHVIEW_FIT_TOVIEW);
    }

    QAction* action = menu.exec(screenPoint);
    int cmd = action ? action->data().toInt() : 0;

    if (cmd >= BASE_NEW_NODE_CMD && cmd < BASE_NEW_NODE_CMD + classes.size())
    {
        QString cls = classes[cmd - BASE_NEW_NODE_CMD];
        CreateNode(cls, point);
        return;
    }

    if (cmd >= BASE_INPUTS_CMD && cmd < BASE_INPUTS_CMD + 1000)
    {
        if (pNode)
        {
            CUndo undo("Graph Port Visibilty");
            pNode->RecordUndo();
            int nPort = cmd - BASE_INPUTS_CMD;
            pNode->GetInputs()->at(nPort).bVisible = !pNode->GetInputs()->at(nPort).bVisible;
            InvalidateNode(pNode, true);
            update();
        }
        return;
    }
    if (cmd >= BASE_OUTPUTS_CMD && cmd < BASE_OUTPUTS_CMD + 1000)
    {
        if (pNode)
        {
            CUndo undo("Graph Port Visibilty");
            pNode->RecordUndo();
            int nPort = cmd - BASE_OUTPUTS_CMD;
            pNode->GetOutputs()->at(nPort).bVisible = !pNode->GetOutputs()->at(nPort).bVisible;
            InvalidateNode(pNode, true);
            update();
        }
        return;
    }


    switch (cmd)
    {
    case ID_EDIT_CUT:
        OnCommandCut();
        break;
    case ID_EDIT_COPY:
        OnCommandCopy();
        break;
    case ID_EDIT_PASTE:
        InternalPaste(false, point);
        break;
    case ID_EDIT_PASTE_WITH_LINKS:
        InternalPaste(true, point);
        break;
    case ID_EDIT_DELETE:
        OnCommandDelete();
        break;
    case ID_GRAPHVIEW_RENAME:
        RenameNode(pNode);
        break;
    case ID_FILE_EXPORTSELECTION:
        emit exportSelectionRequested();
        break;
    case ID_FILE_IMPORT:
        void importRequested();
        break;
    case ID_GRAPHVIEW_ADD_SELECTED_ENTITY:
    {
        CreateNode("selected_entity", point);
    }
    break;
    case ID_GRAPHVIEW_SPLINES:
        m_bSplineArrows = !m_bSplineArrows;
        update();
        break;
    case ID_GRAPHVIEW_EDGES_ON_TOP_OF_NODES:
        gSettings.bFlowGraphEdgesOnTopOfNodes = !gSettings.bFlowGraphEdgesOnTopOfNodes;
        update();
        break;
    case ID_GRAPHVIEW_ADD_DEFAULT_ENTITY:
    {
        CreateNode("default_entity", point);
    }
    break;
    case ID_GRAPHVIEW_ADD_COMMENT:
    {
        CHyperNode* pNode = CreateNode(CCommentNode::GetClassType(), point);
        if (pNode)
        {
            RenameNode(pNode);
        }
    }
    break;
    case ID_GRAPHVIEW_ADD_COMMENTBOX:
    {
        std::vector<CHyperNode*> nodes;
        GetSelectedNodes(nodes, SELECTION_SET_ONLY_PARENTS);

        CCommentBoxNode* pNewNode = static_cast<CCommentBoxNode*>(CreateNode(CCommentBoxNode::GetClassType(), point));

        // if there are selected nodes, and they are on view, the created commentBox encloses all of them
        if (nodes.size() > 1)
        {
            QRectF totalRect = nodes[0]->GetRect();
            for (int i = 1; i < nodes.size(); ++i)
            {
                CHyperNode* pSelNode = nodes[i];
                totalRect = totalRect.united(pSelNode->GetRect());
            }
            totalRect.setWidth(ceilf(totalRect.width()));
            totalRect.setHeight(ceilf(totalRect.height()));

            const QRect screenRc = rect();
            const QRect selectRc = LocalToViewRect(totalRect);
            bool selectedNodesAreOnView = screenRc.intersects(selectRc);

            if (selectedNodesAreOnView)
            {
                const float MARGIN = 10;
                totalRect.adjust(-MARGIN, -MARGIN, MARGIN, MARGIN);
                pNewNode->SetBorderRect(totalRect);
            }
        }

        if (pNewNode)
        {
            RenameNode(pNewNode);
        }
    }
    break;
    case ID_GRAPHVIEW_ADD_BLACK_BOX:
    {
        m_pGraph->RecordUndo();
        OnCreateGroupFromSelection();
        break;
    }
    case ID_GRAPHVIEW_UNGROUP:
    {
        m_pGraph->RecordUndo();
        OnRemoveGroupsInSelection();
        break;
    }
    case ID_GRAPHVIEW_ADD_TRACKEVENT:
    {
        // Make CTrackEventNode
        CreateNode(CTrackEventNode::GetClassType(), point);
    }
    break;
    case ID_GRAPHVIEW_ADD_STARTNODE:
    {
        CreateNode("Game:Start", point);
    }
    break;
    case ID_GRAPHVIEW_FIT_TOVIEW:
        OnCommandFitAll();
        break;
    case ID_GRAPHVIEW_SELECT_ENTITY:
        OnSelectEntity();
        break;
    case ID_GRAPHVIEW_SELECT_AND_GOTO_ENTITY:
    {
        OnSelectEntity();
        CViewport* vp = GetIEditor()->GetActiveView();
        if (vp)
        {
            vp->CenterOnSelection();
        }
    }
    break;
    case ID_OUTPUTS_SHOW_ALL:
    case ID_OUTPUTS_HIDE_ALL:
        if (pNode)
        {
            CUndo undo("Graph Port Visibilty");
            pNode->RecordUndo();
            for (int i = 0; i < pNode->GetOutputs()->size(); i++)
            {
                pNode->GetOutputs()->at(i).bVisible = (cmd == ID_OUTPUTS_SHOW_ALL) || (pNode->GetOutputs()->at(i).nConnected != 0);
            }
            InvalidateNode(pNode, true);
            update();
        }
        break;
    case ID_INPUTS_SHOW_ALL:
    case ID_INPUTS_HIDE_ALL:
        if (pNode)
        {
            CUndo undo("Graph Port Visibilty");
            pNode->RecordUndo();
            for (int i = 0; i < pNode->GetInputs()->size(); i++)
            {
                pNode->GetInputs()->at(i).bVisible = (cmd == ID_INPUTS_SHOW_ALL) || (pNode->GetInputs()->at(i).nConnected != 0);
            }
            InvalidateNode(pNode, true);
            update();
        }
        break;
    case ID_OUTPUTS_DISABLE_LINKS:
    case ID_OUTPUTS_ENABLE_LINKS:
        if (pNode)
        {
            CUndo undo("Enabling Graph Node Links");
            pNode->RecordUndo();

            std::vector<CHyperEdge*> edges;
            if (m_pGraph->FindEdges(pNode, edges))
            {
                for (int i = 0; i < edges.size(); ++i)
                {
                    if (pNode == m_pGraph->FindNode(edges[i]->nodeOut))
                    {
                        m_pGraph->EnableEdge(edges[i], cmd == ID_OUTPUTS_ENABLE_LINKS);
                        InvalidateEdge(edges[i]);
                    }
                }
            }
        }
        break;
    case ID_INPUTS_DISABLE_LINKS:
    case ID_INPUTS_ENABLE_LINKS:
        if (pNode)
        {
            CUndo undo("Enabling Graph Node Links");
            pNode->RecordUndo();

            std::vector<CHyperEdge*> edges;
            if (m_pGraph->FindEdges(pNode, edges))
            {
                for (int i = 0; i < edges.size(); ++i)
                {
                    if (pNode == m_pGraph->FindNode(edges[i]->nodeIn))
                    {
                        m_pGraph->EnableEdge(edges[i], cmd == ID_INPUTS_ENABLE_LINKS);
                        InvalidateEdge(edges[i]);
                    }
                }
            }
        }
        break;
    case ID_INPUTS_CUT_LINKS:
    case ID_OUTPUTS_CUT_LINKS:
    {
        CUndo undo("Cut Graph Node Links");
        CopyLinks(pNode, cmd == ID_INPUTS_CUT_LINKS);
        DeleteLinks(pNode, cmd == ID_INPUTS_CUT_LINKS);
    }
    break;
    case ID_INPUTS_COPY_LINKS:
    case ID_OUTPUTS_COPY_LINKS:
        CopyLinks(pNode, cmd == ID_INPUTS_COPY_LINKS);
        break;
    case ID_INPUTS_PASTE_LINKS:
    case ID_OUTPUTS_PASTE_LINKS:
    {
        CUndo undo("Paste Graph Node Links");
        m_pGraph->RecordUndo();
        PasteLinks(pNode, cmd == ID_INPUTS_PASTE_LINKS);
    }
    break;
    case ID_INPUTS_DELETE_LINKS:
    case ID_OUTPUTS_DELETE_LINKS:
    {
        CUndo undo("Delete Graph Node Links");
        DeleteLinks(pNode, cmd == ID_INPUTS_DELETE_LINKS);
    }
    break;
    case ID_CREATE_GLOBAL_FG_MODULE_FROM_SELECTION:
    {
        //OnCreateModuleFromSelection(true);
        break;
    }
    case ID_CREATE_LEVEL_FG_MODULE_FROM_SELECTION:
    {
        //OnCreateModuleFromSelection(false);
        break;
    }
    default:
        HandleEntityMenu(pNode, cmd);
        break;
    }
}

struct NodeFilter
{
public:
    NodeFilter(uint32 mask)
        : mask(mask) {}
    bool Visit(CHyperNode* pNode)
    {
        if (pNode->IsEditorSpecialNode())
        {
            return false;
        }
        CFlowNode* pFlowNode = static_cast<CFlowNode*> (pNode);
        if ((pFlowNode->GetCategory() & mask) == 0)
        {
            return false;
        }
        return true;

        // Only if the usage mask is set check if fulfilled -> this is an exclusive thing
        if ((mask & EFLN_USAGE_MASK) != 0 && (pFlowNode->GetUsageFlags() & mask) == 0)
        {
            return false;
        }
        return true;
    }
    uint32 mask;
};

////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::PopulateClassMenu(SClassMenuGroup& classMenu, QStringList& classes)
{
    NodeFilter filter(m_componentViewMask);
    std::vector<_smart_ptr<CHyperNode> > prototypes;
    m_pGraph->GetManager()->GetPrototypesEx(prototypes, true, functor_ret(filter, &NodeFilter::Visit));
    classes.clear();
    for (std::vector<_smart_ptr<CHyperNode> >::iterator iter = prototypes.begin();
         iter != prototypes.end(); ++iter)
    {
        if ((*iter)->IsFlowNode())
        {
            const QString fullname = (*iter)->GetUIClassName();
            if (fullname.contains(':'))
            {
                classes.push_back(fullname);
            }
            else
            {
                classes.push_back(QString::fromLatin1("Misc:%10x2410").arg(fullname));
            }
        }
    }
    std::sort(classes.begin(), classes.end());

    PopulateSubClassMenu(classMenu, classes, BASE_NEW_NODE_CMD);
}

//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::PopulateSubClassMenu(SClassMenuGroup& classMenu, QStringList& classes, int baseIndex)
{
    std::map<QString, SClassMenuGroup*> groupMap;
    QStringList subGroups;
    for (int i = 0; i < classes.size(); i++)
    {
        const QString& fullname = classes[i];
        QString group, node;
        if (fullname.contains(':'))
        {
            group = fullname.left(fullname.indexOf(":"));
            node = fullname.mid(group.length() + 1);
            const int marker = node.indexOf("0x2410");
            if (marker > 0)
            {
                node = node.left(marker);
                classes[i] = node;
            }
        }
        else
        {
            group = "_UNKNOWN_"; // should never happen
            node = fullname;
            // assert (false);
        }

        SClassMenuGroup* pGroupMenu = &classMenu;
        if (!group.isEmpty())
        {
            pGroupMenu = stl::find_in_map(groupMap, group, (SClassMenuGroup*)0);
            if (!pGroupMenu)
            {
                pGroupMenu = classMenu.CreateSubMenu();
                pGroupMenu->pMenu->setTitle(group);
                classMenu.pMenu->addMenu(pGroupMenu->pMenu.get());
                groupMap[group] = pGroupMenu;
            }
        }
        else
        {
            assert(false); // should never happen
            continue;
        }

        if (node.contains(':'))
        {
            const QString parentGroup = node.left(node.indexOf(":"));
            if (!stl::find(subGroups, parentGroup))
            {
                subGroups.push_back(parentGroup);
                QStringList subclasses;
                for (QStringList::const_iterator iter = classes.begin(); iter != classes.end(); ++iter)
                {
                    const QString subclass = iter->mid(group.length() + 1);
                    if (subclass.contains(':') && subclass.left(subclass.indexOf(":")) == parentGroup)
                    {
                        subclasses.push_back(subclass);
                    }
                }
                if (subclasses.size() > 0)
                {
                    PopulateSubClassMenu(*pGroupMenu, subclasses, i + baseIndex);
                }
            }
        }
        else
        {
            pGroupMenu->pMenu->addAction(node)->setData(i + baseIndex);
        }
    }
}

////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::ShowPortsConfigMenu(const QPoint& point, bool bInput, CHyperNode* pNode)
{
    QMenu menu;

    CHyperNode::Ports* pPorts;
    if (bInput)
    {
        pPorts = pNode->GetInputs();
        menu.addAction(tr("Inputs"))->setEnabled(false);
    }
    else
    {
        pPorts = pNode->GetOutputs();
        menu.addAction(tr("Output"))->setEnabled(false);
    }
    menu.addSeparator();

    for (int i = 0; i < pPorts->size(); i++)
    {
        CHyperNodePort* pPort = &(*pPorts)[i];
        QAction* action = menu.addAction(pPort->pVar->GetName());
        int flags = MF_STRING;
        action->setCheckable(true);
        action->setChecked(pPort->bVisible || pPort->nConnected != 0);
        action->setEnabled(pPort->nConnected == 0);
        action->setData(i + 1);
    }
    menu.addSeparator();
    QAction* actionInputsShowAll = 0, * actionInputsHideAll = 0, * actionOutputsShowAll = 0, * actionOutputsHideAll = 0;
    if (bInput)
    {
        actionInputsShowAll = menu.addAction(tr("Show All"));
        actionInputsHideAll = menu.addAction(tr("Hide All"));
    }
    else
    {
        actionOutputsShowAll = menu.addAction(tr("Show All"));
        actionOutputsHideAll = menu.addAction(tr("Hide All"));
    }


    QAction* cmd = menu.exec(mapToGlobal(point));
    if (cmd == actionOutputsShowAll || cmd == actionOutputsHideAll)
    {
        if (pNode)
        {
            CUndo undo("Graph Port Visibilty");
            pNode->RecordUndo();
            for (int i = 0; i < pNode->GetOutputs()->size(); i++)
            {
                pNode->GetOutputs()->at(i).bVisible = (cmd == actionOutputsShowAll) || (pNode->GetOutputs()->at(i).nConnected != 0);
            }
            InvalidateNode(pNode, true);
            update();
            return;
        }
    }
    else if (cmd == actionInputsShowAll || cmd == actionOutputsHideAll)
    {
        if (pNode)
        {
            CUndo undo("Graph Port Visibilty");
            pNode->RecordUndo();
            for (int i = 0; i < pNode->GetInputs()->size(); i++)
            {
                pNode->GetInputs()->at(i).bVisible = (cmd == actionInputsShowAll) || (pNode->GetInputs()->at(i).nConnected != 0);
            }
            InvalidateNode(pNode, true);
            update();
            return;
        }
    }
    if (cmd->data().toInt() > 0)
    {
        CUndo undo("Graph Port Visibilty");
        pNode->RecordUndo();
        CHyperNodePort* pPort = &(*pPorts)[cmd->data().toInt() - 1];
        pPort->bVisible = !pPort->bVisible || (pPort->nConnected != 0);
    }
    InvalidateNode(pNode, true);
    update();
}


//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::UpdateWorkingArea()
{
    m_workingArea = QRect();
    if (!m_pGraph)
    {
        m_scrollOffset = QPoint(0, 0);
        return;
    }

    bool bFirst = true;
    IHyperGraphEnumerator* pEnum = m_pGraph->GetNodesEnumerator();
    for (IHyperNode* pINode = pEnum->GetFirst(); pINode; pINode = pEnum->GetNext())
    {
        CHyperNode* fn = (CHyperNode*)(pINode);
        const QRectF& qitemRect = fn->GetRect();
        if (bFirst)
        {
            m_workingArea = qitemRect;
            bFirst = false;
        }
        else
        {
            m_workingArea |= qitemRect;
        }
    }
    pEnum->Release();

    m_workingArea.translate(-GRID_SIZE, -GRID_SIZE);
    m_workingArea.setWidth(m_workingArea.width() + 2 * GRID_SIZE);
    m_workingArea.setHeight(m_workingArea.height() + 2 * GRID_SIZE);
}

//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::SetScrollExtents(bool isUpdateWorkingArea)
{
    if (isUpdateWorkingArea)
    {
        UpdateWorkingArea();
    }

    static bool bNoRecurse = false;
    if (bNoRecurse)
    {
        return;
    }
    bNoRecurse = true;

    // Update scroll.
    QRect rcClient = rect();

    QRectF rect = ViewToLocalRect(rcClient);
    QRectF totalRect = rect | m_workingArea;
    QRect rc = LocalToViewRect(totalRect);

    // avoid rounding errors
    if (totalRect.x() == rect.x())
    {
        rc.moveLeft(rcClient.x());
    }
    if (totalRect.y() == rect.y())
    {
        rc.moveTop(rcClient.y());
    }
    if (totalRect.right() == rect.right())
    {
        rc.setRight(rcClient.right());
    }
    if (totalRect.bottom() == rect.bottom())
    {
        rc.setBottom(rcClient.bottom());
    }

    const QSignalBlocker horizontalBlocker(m_horizontalScrollBar);
    m_horizontalScrollBarPos = -rc.left();
    m_horizontalScrollBar->setRange(0., rc.width() - rcClient.width());
    m_horizontalScrollBar->setValue(m_horizontalScrollBarPos);
    m_horizontalScrollBar->setPageStep(rcClient.width());
    const QSignalBlocker verticalBlocker(m_verticalScrollBar);
    m_verticalScrollBarPos = -rc.top();
    m_verticalScrollBar->setRange(0., rc.height() - rcClient.height());
    m_verticalScrollBar->setValue(m_verticalScrollBarPos);
    m_verticalScrollBar->setPageStep(rcClient.height());

    bNoRecurse = false;

    if (m_pGraph && !rcClient.isEmpty())
    {
        m_pGraph->SetViewPosition(m_scrollOffset, m_zoom);
    }

    const QSize horizontalSizeHint = m_horizontalScrollBar->sizeHint();
    const QSize verticalSizeHint = m_verticalScrollBar->sizeHint();

    m_horizontalScrollBar->setVisible(rc.width() > rcClient.width());
    m_verticalScrollBar->setVisible(rc.height() > rcClient.height());

    m_horizontalScrollBar->resize(width() - (m_verticalScrollBar->isVisible() ? verticalSizeHint.width() : 0), horizontalSizeHint.height());
    m_horizontalScrollBar->move(0, height() - m_horizontalScrollBar->height());
    m_verticalScrollBar->resize(verticalSizeHint.width(), height() - (m_horizontalScrollBar->isVisible() ? horizontalSizeHint.height() : 0));
    m_verticalScrollBar->move(width() - m_verticalScrollBar->width(), 0);
}

////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnHScroll()
{
    HideEditPort();

    int v = m_horizontalScrollBar->value();
    m_scrollOffset.setX(m_scrollOffset.x() + m_horizontalScrollBarPos - v);
    SetScrollExtents(false);
    update();
}

//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnVScroll()
{
    HideEditPort();

    int v = m_verticalScrollBar->value();
    m_scrollOffset.setY(m_scrollOffset.y() + m_verticalScrollBarPos - v);
    SetScrollExtents(false);
    update();
}

////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::RenameNode(CHyperNode* pNode)
{
    assert(pNode);
    if (HandleRenameNode(pNode))
    {
        return;
    }

    m_pRenameNode = pNode;
    QRect rc = LocalToViewRect(pNode->GetRect());
    rc = rc.adjusted(1, 1, -1, -1);
    rc.setHeight(DEFAULT_EDIT_HEIGHT);
    if (m_renameEdit)
    {
        delete m_renameEdit;
    }

    int nEditFlags = WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL;
    if (pNode->IsEditorSpecialNode())
    {
        nEditFlags |= ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN;
        rc.setHeight(rc.height() + 42);
        m_renameEdit = new QHyperGraphWidgetMultilineEdit(this);
    }
    else
    {
        m_renameEdit = new QHyperGraphWidgetSinglelineEdit(this);
    }
    connect(m_renameEdit, SIGNAL(editingFinished()), this, SLOT(OnAcceptRename()));
    connect(m_renameEdit, SIGNAL(editingCancelled()), this, SLOT(OnCancelRename()));
    m_renameEdit->setGeometry(rc);
    m_renameEdit->setProperty("text", m_pRenameNode->GetName());
    QMetaObject::invokeMethod(m_renameEdit, "selectAll");
    m_renameEdit->show();
    m_renameEdit->setFocus();
}
//
////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnAcceptRename()
{
    QString text = m_renameEdit->property("text").toString();
    if (m_pRenameNode)
    {
        text.replace("\n", "\\n");
        text.replace("\r", "");
        m_pRenameNode->SetName(text);
        InvalidateNode(m_pRenameNode);
    }
    m_pRenameNode = NULL;
    disconnect(m_renameEdit, nullptr, this, nullptr);
    m_renameEdit->deleteLater();
    m_renameEdit = 0;
    OnNodeRenamed();
}

////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnCancelRename()
{
    m_pRenameNode = NULL;
    disconnect(m_renameEdit, nullptr, this, nullptr);
    m_renameEdit->deleteLater();
    m_renameEdit = 0;
}

//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnSelectionChange()
{
    if (!m_pPropertiesCtrl)
    {
        return;
    }

    m_pEditedNode = 0;
    m_pMultiEditedNodes.resize(0);
    m_pMultiEditVars = 0;

    std::vector<CHyperNode*> nodes;
    GetSelectedNodes(nodes, SELECTION_SET_ONLY_PARENTS);
    m_pPropertiesCtrl->RemoveAllItems();

    if (nodes.size() >= 1 && !m_bIsFrozen)
    {
        CHyperNode* pNode = nodes[0];
        _smart_ptr<CVarBlock> pVarBlock = pNode->GetInputsVarBlock();
        if (pVarBlock && pNode->GetTypeId() == gEnv->pFlowSystem->GetTypeId("Engine:LayerSwitch"))
        {
            SetupLayerList(pVarBlock);
        }

        if (nodes.size() == 1)
        {
            m_pEditedNode = pNode;

            if (pVarBlock)
            {
                m_pPropertiesCtrl->AddVarBlock(pVarBlock);
                //force rebuild now because we will resize ourself based on height just after this call
                m_pPropertiesCtrl->RebuildCtrl(false);
            }
        }
        else
        {
            bool isMultiselect = false;
            // Find if we have nodes the same type like the first one
            for (int i = 1; i < nodes.size(); ++i)
            {
                if (pNode->GetTypeId() == nodes[i]->GetTypeId())
                {
                    isMultiselect = true;
                    break;
                }
            }

            if (isMultiselect)
            {
                if (pVarBlock)
                {
                    m_pMultiEditVars = pVarBlock->Clone(true);
                    m_pPropertiesCtrl->AddVarBlock(m_pMultiEditVars);
                    //force rebuild now because we will resize ourself based on height just after this call
                    m_pPropertiesCtrl->RebuildCtrl(false);
                }

                for (int i = 0; i < nodes.size(); ++i)
                {
                    if (pNode->GetTypeId() == nodes[i]->GetTypeId())
                    {
                        m_pMultiEditedNodes.push_back(nodes[i]);
                        m_pMultiEditVars->Wire(nodes[i]->GetInputsVarBlock());
                    }
                }
            }
        }
    }

    if (CHyperGraphDialog::instance())
    {
        CHyperGraphDialog::instance()->OnViewSelectionChange();
    }
}

//////////////////////////////////////////////////////////////////////////
CHyperNode* QHyperGraphWidget::CreateNode(const QString& sNodeClass, const QPoint& point)
{
    if (!m_pGraph)
    {
        return 0;
    }

    QPointF p = ViewToLocal(point);
    CHyperNode* pNode = NULL;
    {
        CUndo undo("New Graph Node");
        m_pGraph->UnselectAll();
        pNode = (CHyperNode*)m_pGraph->CreateNode(sNodeClass.toUtf8().data(), p);
    }
    if (pNode)
    {
        pNode->SetSelected(true);
        OnSelectionChange();
    }
    InvalidateView();
    return pNode;
}

////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::UpdateNodeProperties(CHyperNode* pNode, IVariable* pVar)
{
    if (pVar->GetDataType() == IVariable::DT_UIENUM && pNode->GetTypeId() == gEnv->pFlowSystem->GetTypeId("Engine:LayerSwitch"))
    {
        QString val;
        pVar->Get(val);
        _smart_ptr<CVarBlock> pVarBlock = pNode->GetInputsVarBlock();
        IVariable* pVarInternal = pVarBlock->FindVariable("Layer");
        if (pVarInternal)
        {
            pVarInternal->Set(val);
        }
    }

    // TODO: ugliest way to solve this! I should find a better solution... [Dejan]
    if (pVar->GetDataType() == IVariable::DT_SOCLASS)
    {
        QString className;
        pVar->Get(className);
        CHyperNodePort* pHelperInputPort = pNode->FindPort("sohelper_helper", true);
        if (!pHelperInputPort)
        {
            pHelperInputPort = pNode->FindPort("sonavhelper_helper", true);
        }
        if (pHelperInputPort && pHelperInputPort->pVar)
        {
            QString helperName;
            pHelperInputPort->pVar->Get(helperName);
            int f = helperName.indexOf(':');
            if (f <= 0 || className != helperName.left(f))
            {
                helperName = className + ':';
                pHelperInputPort->pVar->Set(helperName);
            }
        }
    }

    pNode->OnInputsChanged();
    InvalidateNode(pNode, true); // force redraw node as param values have changed

    if (m_pGraph)
    {
        m_pGraph->SendNotifyEvent(pNode, EHG_NODE_PROPERTIES_CHANGED);
    }
}


////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnUpdateProperties(IVariable* pVar)
{
    if (m_pMultiEditedNodes.size())
    {
        for (std::vector<CHyperNode*>::iterator ppNode = m_pMultiEditedNodes.begin(); ppNode != m_pMultiEditedNodes.end(); ++ppNode)
        {
            if (*ppNode)
            {
                UpdateNodeProperties(*ppNode, pVar);
            }
        }
    }
    else if (m_pEditedNode)
    {
        UpdateNodeProperties(m_pEditedNode, pVar);
    }

    if (m_pGraph && m_pGraph->IsFlowGraph())
    {
        CFlowGraph* pFlowGraph = static_cast<CFlowGraph*> (m_pGraph);
        CEntityObject* pEntity = pFlowGraph->GetEntity();
        if (pEntity)
        {
            pEntity->SetLayerModified();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::InvalidateView(bool bComplete)
{
    std::vector<CHyperEdge*> edges;
    if (m_pGraph)
    {
        m_pGraph->GetAllEdges(edges);
    }
    std::copy(edges.begin(), edges.end(), std::inserter(m_edgesToReroute, m_edgesToReroute.end()));

    if (bComplete && m_pGraph)
    {
        IHyperGraphEnumerator* pEnum = m_pGraph->GetNodesEnumerator();
        for (IHyperNode* pINode = pEnum->GetFirst(); pINode; pINode = pEnum->GetNext())
        {
            CHyperNode* pNode = (CHyperNode*)pINode;
            pNode->Invalidate(true);
        }
        pEnum->Release();
    }

    //  Invalidate();
    SetScrollExtents();
    this->update();
}

//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::FitFlowGraphToView()
{
    OnCommandFitAll();
}

////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnCommandDelete()
{
    if (!m_pGraph)
    {
        return;
    }

    CUndo undo("HyperGraph Delete Node(s)");
    m_pGraph->RecordUndo();
    std::vector<CHyperNode*> nodes;
    if (!GetSelectedNodes(nodes, SELECTION_SET_INCLUDE_RELATIVES))
    {
        return;
    }

    for (int i = 0; i < nodes.size(); i++)
    {
        if (!nodes[i]->CheckFlag(EHYPER_NODE_UNREMOVEABLE))
        {
            m_pGraph->RemoveNode(nodes[i]);
        }
    }
    OnSelectionChange();
    InvalidateView();
    PropagateChangesToGroupsAndPrefabs();
}

////////////////////////////////////////////////////////////////////////////
//KDAB_PORT_UNUSED
//void QHyperGraphWidget::OnCommandDeleteKeepLinks()
//{
//  if (!m_pGraph)
//      return;
//
//  CUndo undo("HyperGraph Hide Node(s)");
//  m_pGraph->RecordUndo();
//  std::vector<CHyperNode*> nodes;
//  if (!GetSelectedNodes(nodes, SELECTION_SET_INCLUDE_RELATIVES))
//      return;
//
//  for (int i = 0; i < nodes.size(); i++)
//  {
//      m_pGraph->RemoveNodeKeepLinks(nodes[i]);
//  }
//  OnSelectionChange();
//  InvalidateView();
//  PropagateChangesToGroupsAndPrefabs();
//}
//

//////////////////////////////////////////////////////////////////////////
//void QHyperGraphWidget::OnCreateModuleFromSelection(bool isGlobal)
//{
//  if (!m_pGraph->IsSelectedAny())
//  {
//      Warning("You must have selected graph nodes to export");
//      return;
//  }
//
//  // Copying the currently selected nodes
//  OnCommandCopy();
//
//  // Create a new FG module
//  GetIEditor()->GetFlowGraphManager()->SendNotifyEvent(
//      isGlobal ? EHG_CREATE_GLOBAL_FG_MODULE_FROM_SELECTION : EHG_CREATE_LEVEL_FG_MODULE_FROM_SELECTION, 0, 0);
//
//  // Paste the selected nodes with links in the newly created FG module
//   OnCommandPasteWithLinks();
//}

////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnCommandCopy()
{
    if (!m_pGraph)
    {
        return;
    }

    std::vector<CHyperNode*> nodes;
    if (!GetSelectedNodes(nodes))
    {
        return;
    }

    m_bCopiedBlackBox = false;
    if (nodes.size() == 1 && nodes[0]->GetClassName() == CBlackBoxNode::GetClassType())
    {
        CBlackBoxNode* pBlackBox = (CBlackBoxNode*)nodes[0];
        nodes.clear();
        nodes = *(pBlackBox->GetNodes());
        m_bCopiedBlackBox = true;
    }

    CClipboard clipboard(this);
    //clipboard.Put( )
    //clipboard.

    CHyperGraphSerializer serializer(m_pGraph, 0);

    for (int i = 0; i < nodes.size(); i++)
    {
        CHyperNode* pNode = nodes[i];

        if (!pNode->CheckFlag(EHYPER_NODE_UNREMOVEABLE))
        {
            serializer.SaveNode(pNode, true);
        }
    }
    if (nodes.size() > 0)
    {
        XmlNodeRef node = XmlHelpers::CreateXmlNode("Graph");
        serializer.Serialize(node, false);
        clipboard.Put(node, "Graph");
    }
    InvalidateView();
}


//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnCommandPaste()
{
    if (m_bIsFrozen)
    {
        return;
    }

    QPoint point = mapFromGlobal(QCursor::pos());

    CClipboard clipboard(this);
    XmlNodeRef node = clipboard.Get();

    if (!node)
    {
        return;
    }

    if (node->isTag("Graph"))
    {
        InternalPaste(false, point);
    }
    else
    {
        bool isInput = node->isTag("GraphNodeInputLinks");
        if (isInput || node->isTag("GraphNodeOutputLinks"))
        {
            std::vector<CHyperNode*> nodes;
            if (!GetSelectedNodes(nodes))
            {
                return;
            }
            CUndo undo("Paste Graph Node Links");
            m_pGraph->RecordUndo();
            for (int i = 0; i < nodes.size(); i++)
            {
                PasteLinks(nodes[i], isInput);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnCommandPasteWithLinks()
{
    if (m_bIsFrozen)
    {
        return;
    }
    QPoint point = mapFromGlobal(QCursor::pos());
    InternalPaste(true, point);
    PropagateChangesToGroupsAndPrefabs();
}

////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::InternalPaste(bool bWithLinks, const QPoint& point, std::vector<CHyperNode*>* pPastedNodes)
{
    if (m_bIsFrozen)
    {
        return;
    }

    if (!m_pGraph)
    {
        return;
    }

    CClipboard clipboard(this);
    XmlNodeRef node = clipboard.Get();

    if (node != NULL && node->isTag("Graph"))
    {
        CUndo undo("HyperGraph Paste Node(s)");
        m_pGraph->UnselectAll();

        if (m_bCopiedBlackBox)
        {
            bWithLinks = true;
        }

        CHyperGraphSerializer serializer(m_pGraph, 0);
        serializer.SelectLoaded(true);
        serializer.Serialize(node, true, bWithLinks, true); // Paste==true -> don't screw up graph specific data if just pasting nodes

        std::vector<CHyperNode*> nodes;
        serializer.GetLoadedNodes(nodes);
        if (pPastedNodes)
        {
            serializer.GetLoadedNodes(*pPastedNodes);
        }

        if (m_bCopiedBlackBox)
        {
            CBlackBoxNode* pBB = (CBlackBoxNode*)(CreateNode(CBlackBoxNode::GetClassType(), point));
            if (pBB)
            {
                RenameNode(pBB);
            }
            for (int i = 0; i < nodes.size(); ++i)
            {
                pBB->AddNode(nodes[i]);
            }
        }

        QPointF offset(100, 100);
        if (rect().contains(point))
        {
            QRectF bounds;
            // Calculate bounds.
            for (int i = 0; i < nodes.size(); i++)
            {
                if (i == 0)
                {
                    bounds = nodes[i]->GetRect();
                }
                else
                {
                    bounds = bounds.united(nodes[i]->GetRect());
                }
            }
            QPointF locPoint = ViewToLocal(point);
            offset = QPointF(locPoint.x() - bounds.x(), locPoint.y() - bounds.y());
        }
        // Offset all pasted nodes.
        for (int i = 0; i < nodes.size(); i++)
        {
            QRectF rc = nodes[i]->GetRect();
            rc.translate(offset);
            nodes[i]->SetRect(rc);
        }
    }
    OnSelectionChange();
    InvalidateView();
}

////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::OnCommandCut()
{
    OnCommandCopy();
    OnCommandDelete();
}

////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::SimulateFlow(CHyperEdge* pEdge)
{
    // re-set target node's port value
    assert(m_pGraph != 0);
    CHyperNode* pNode = (CHyperNode*)m_pGraph->FindNode(pEdge->nodeIn);
    assert(pNode != 0);
    CFlowNode* pFlowNode = (CFlowNode*)pNode;
    IFlowGraph* pIGraph = pFlowNode->GetIFlowGraph();
    const TFlowInputData* pValue = pIGraph->GetInputValue(pFlowNode->GetFlowNodeId(), pEdge->nPortIn);
    assert(pValue != 0);
    pIGraph->ActivatePort(SFlowAddress(pFlowNode->GetFlowNodeId(), pEdge->nPortIn, false), *pValue);
}

//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::SetZoom(float zoom)
{
    m_zoom = CLAMP(zoom, MIN_ZOOM, MAX_ZOOM);
    NotifyZoomChangeToNodes();
}

//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::NotifyZoomChangeToNodes()
{
    if (m_pGraph)
    {
        HideEditPort();
        IHyperGraphEnumerator* pEnum = m_pGraph->GetNodesEnumerator();
        for (IHyperNode* pINode = pEnum->GetFirst(); pINode; pINode = pEnum->GetNext())
        {
            CHyperNode* pNode = (CHyperNode*)pINode;
            pNode->OnZoomChange(m_zoom);
        }
        pEnum->Release();
    }
}

////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::CopyLinks(CHyperNode* pNode, bool isInput)
{
    if (!m_pGraph || !pNode)
    {
        return;
    }

    CHyperGraphSerializer serializer(m_pGraph, 0);

    CClipboard clipboard(this);
    XmlNodeRef node = XmlHelpers::CreateXmlNode(isInput ? "GraphNodeInputLinks" : "GraphNodeOutputLinks");

    std::vector<CHyperEdge*> edges;
    if (m_pGraph->FindEdges(pNode, edges))
    {
        for (int i = 0; i < edges.size(); ++i)
        {
            if (isInput && pNode == m_pGraph->FindNode(edges[i]->nodeIn) ||
                !isInput && pNode == m_pGraph->FindNode(edges[i]->nodeOut))
            {
                serializer.SaveEdge(edges[i]);
            }
        }
    }
    serializer.Serialize(node, false);
    clipboard.Put(node, "Graph");
}


//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::PasteLinks(CHyperNode* pNode, bool isInput)
{
    if (!m_pGraph || !pNode)
    {
        return;
    }

    CClipboard clipboard(this);
    XmlNodeRef node = clipboard.Get();
    if (!node ||
        isInput  && !node->isTag("GraphNodeInputLinks") ||
        !isInput && !node->isTag("GraphNodeOutputLinks"))
    {
        return;
    }

    XmlNodeRef edgesXml = node->findChild("Edges");
    if (edgesXml)
    {
        HyperNodeID nodeIn, nodeOut;
        QString portIn, portOut;
        bool bEnabled;
        bool bAskReassign = true;
        bool bIsReassign = false;
        for (int i = 0; i < edgesXml->getChildCount(); i++)
        {
            XmlNodeRef edgeXml = edgesXml->getChild(i);

            edgeXml->getAttr("nodeIn", nodeIn);
            edgeXml->getAttr("nodeOut", nodeOut);
            edgeXml->getAttr("portIn", portIn);
            edgeXml->getAttr("portOut", portOut);
            if (edgeXml->getAttr("enabled", bEnabled) == false)
            {
                bEnabled = true;
            }

            if (isInput)
            {
                nodeIn = pNode->GetId();
            }
            else
            {
                nodeOut = pNode->GetId();
            }

            CHyperNode* pNodeIn = (CHyperNode*)m_pGraph->FindNode(nodeIn);
            CHyperNode* pNodeOut = (CHyperNode*)m_pGraph->FindNode(nodeOut);
            if (!pNodeIn || !pNodeOut)
            {
                continue;
            }
            CHyperNodePort* pPortIn = pNodeIn->FindPort(portIn, true);
            CHyperNodePort* pPortOut = pNodeOut->FindPort(portOut, false);
            if (!pPortIn || !pPortOut)
            {
                continue;
            }

            // check of reassigning inputs
            if (isInput)
            {
                bool bFound = false;
                std::vector<CHyperEdge*> edges;
                if (m_pGraph->FindEdges(pNodeIn, edges))
                {
                    for (int i = 0; i < edges.size(); ++i)
                    {
                        if (pNodeIn == m_pGraph->FindNode(edges[i]->nodeIn) &&
                            !QString::compare(portIn, edges[i]->portIn, Qt::CaseInsensitive))
                        {
                            bFound = true;
                            break;
                        }
                    }
                }
                if (bFound)
                {
                    if (bAskReassign)
                    {
                        if (QMessageBox::question(this, tr("Reassign input ports"), tr("This operation tries to reassign some not empty input port(s). Do you want to reassign?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
                        {
                            bIsReassign = true;
                        }
                        bAskReassign = false;
                    }
                    if (!bIsReassign)
                    {
                        continue;
                    }
                }
            }

            m_pGraph->ConnectPorts(pNodeOut, pPortOut, pNodeIn, pPortIn, false);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::DeleteLinks(CHyperNode* pNode, bool isInput)
{
    if (!m_pGraph || !pNode)
    {
        return;
    }

    m_pGraph->RecordUndo();

    std::vector<CHyperEdge*> edges;
    if (m_pGraph->FindEdges(pNode, edges))
    {
        for (int i = 0; i < edges.size(); ++i)
        {
            if (isInput && pNode == m_pGraph->FindNode(edges[i]->nodeIn) ||
                !isInput && pNode == m_pGraph->FindNode(edges[i]->nodeOut))
            {
                m_pGraph->RemoveEdge(edges[i]);
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::ShowEditPort(CHyperNode* pNode, CHyperNodePort* pPort)
{
    m_editParamCtrl.RemoveAllItems();
    _smart_ptr<CVarBlock> pVarBlock = pNode->GetInputsVarBlock();
    if (pVarBlock)
    {
        QFont f(m_editParamCtrl.font());
        if (m_zoom > MIN_ZOOM_CHANGE_HEIGHT)
        {
            f.setPointSizeF(abs(gSettings.gui.nDefaultFontHieght * m_zoom));
        }
        else
        {
            f.setPointSizeF(abs(gSettings.gui.nDefaultFontHieght));
        }
        m_editParamCtrl.setFont(f);

        _smart_ptr<CVarBlock> pEditVarBlock = new CVarBlock;
        pEditVarBlock->AddVariable(pPort->pVar);
        const TFlowNodeId typeId = pNode->GetTypeId();
        if (typeId == gEnv->pFlowSystem->GetTypeId("Engine:LayerSwitch"))
        {
            SetupLayerList(pEditVarBlock);
            m_editParamCtrl.SetUpdateCallback(functor(*this, &QHyperGraphWidget::OnUpdateProperties));
            m_editParamCtrl.EnableUpdateCallback(true);
        }
        else if (typeId == gEnv->pFlowSystem->GetTypeId("TrackEvent"))
        {
            m_editParamCtrl.SetUpdateCallback(functor(*this, &QHyperGraphWidget::OnUpdateProperties));
            m_editParamCtrl.EnableUpdateCallback(true);
        }
        m_editParamCtrl.AddVarBlock(pEditVarBlock);
        m_editParamCtrl.setVisible(true);
        //force rebuild now because we will resize ourself based on height just after this call
        m_editParamCtrl.RebuildCtrl(false);

        QRectF rect = pNode->GetRect();
        QRectF rectPort;
        pNode->GetAttachQRect(pPort->nPortIndex, &rectPort);
        rectPort.setY(rectPort.y() - PORT_HEIGHT / 2);
        rectPort.setHeight(PORT_HEIGHT);
        rectPort.translate(pNode->GetPos());

        QPoint LeftTop = LocalToView(rectPort.topLeft());
        QPoint RightBottom = LocalToView(QPointF(rect.right(), rectPort.bottom()));
        int w = MAX(RightBottom.x() - LeftTop.x() - 2, m_editParamCtrl.sizeHint().width());
        int h = m_editParamCtrl.GetVisibleHeight() + 5;
        QPoint globalPos = mapToGlobal(LeftTop);
        m_editParamCtrl.setGeometry(globalPos.x() + 1, globalPos.y(), w, h + 1);
        ReflectedPropertyItem* pItem = m_editParamCtrl.FindItemByVar(pPort->pVar);
        if (pItem)
        {
            m_editParamCtrl.SelectItem(pItem);
        }

        m_isEditPort = true;
    }
}


//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::HideEditPort()
{
    m_pEditPort = 0;
    if (!m_isEditPort)
    {
        return;
    }

    // Kill focus from control before Removing items to assign changes
    setFocus();

    m_editParamCtrl.RemoveAllItems();
    m_editParamCtrl.setVisible(false);
    m_isEditPort = false;
}


//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::UpdateFrozen()
{
    bool bOldIsFrozen = m_bIsFrozen;
    m_bIsFrozen = false;
    HideEditPort();
    if (m_pGraph && m_pGraph->IsFlowGraph())
    {
        CFlowGraph* pFlowGraph = static_cast<CFlowGraph*> (m_pGraph);
        CEntityObject* pEntity = pFlowGraph->GetEntity();
        if ((pEntity && pEntity->IsFrozen()) || GetIEditor()->GetGameEngine()->IsFlowSystemUpdateEnabled())
        {
            m_pGraph->UnselectAll();
            OnSelectionChange();
            m_bIsFrozen = true;
        }
        if (bOldIsFrozen != m_bIsFrozen)
        {
            InvalidateView();
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::SetupLayerList(CVarBlock* pVarBlock)
{
    // In the Game a Layer port has a string type, for editing we replace a port variable to the enum type variable
    IVariable* pVar = pVarBlock->FindVariable("Layer");
    if (!pVar)
    {
        return;
    }

    QString val;
    pVar->Get(val);
    pVarBlock->DeleteVariable(pVar);

    CVariableFlowNodeEnum<QString>* pEnumVar = new CVariableFlowNodeEnum<QString>;
    pEnumVar->SetDataType(IVariable::DT_UIENUM);
    pEnumVar->SetName("Layer");

    std::vector<CObjectLayer*> layers;
    GetIEditor()->GetObjectManager()->GetLayersManager()->GetLayers(layers);
    for (int i = 0; i < layers.size(); ++i)
    {
        if (layers[i])
        {
            pEnumVar->AddEnumItem(layers[i]->GetName(), layers[i]->GetName());
        }
    }
    pEnumVar->Set(val);
    pVarBlock->AddVariable(pEnumVar);
}

void QHyperGraphWidget::QuickSearchNode(CHyperNode* pNode)
{
    assert(pNode);
    m_pQuickSearchNode = pNode;
    QRect rc = LocalToViewRect(pNode->GetRect());
    rc.adjust(1, 1, -1, -1);
    rc.setHeight(DEFAULT_EDIT_HEIGHT * m_zoom);

    if (m_quickSearchEdit)
    {
        delete m_quickSearchEdit;
    }

    m_quickSearchEdit = new QHyperGraphWidgetSinglelineEdit(this);
    connect(m_quickSearchEdit, &QHyperGraphWidgetSinglelineEdit::editingFinished, this, &QHyperGraphWidget::OnAcceptSearch);
    connect(m_quickSearchEdit, &QHyperGraphWidgetSinglelineEdit::editingCancelled, this, &QHyperGraphWidget::OnCancelSearch);
    connect(m_quickSearchEdit, &QHyperGraphWidgetSinglelineEdit::textChanged, this, &QHyperGraphWidget::OnSearchNodes);
    connect(m_quickSearchEdit, &QHyperGraphWidgetSinglelineEdit::selectNextSearchResult, this, &QHyperGraphWidget::OnSelectNextSearchResult);

    m_quickSearchEdit->setFrame(false);
    m_quickSearchEdit->setText(m_pQuickSearchNode->GetName());
    m_quickSearchEdit->setGeometry(rc);

    const QFont font("Tahoma", 8 * m_zoom, QFont::Bold);
    m_quickSearchEdit->setFont(font);
    m_quickSearchEdit->selectAll();
    m_quickSearchEdit->show();
    m_quickSearchEdit->setFocus();
}

void QHyperGraphWidget::OnAcceptSearch()
{
    if (m_pQuickSearchNode)
    {
        QString sName = m_pQuickSearchNode->GetName();

        if (sName.startsWith("Misc:"))
        {
            sName.remove(0, 5);
        }

        CQuickSearchNode* pQuickSearchNode = static_cast<CQuickSearchNode*>(m_pQuickSearchNode.get());
        if (pQuickSearchNode && pQuickSearchNode->GetSearchResultCount())
        {
            CreateNode(sName, mapFromGlobal(QCursor::pos()));
        }
        m_pGraph->RemoveNode(m_pQuickSearchNode);
        m_pQuickSearchNode = NULL;
    }

    disconnect(m_quickSearchEdit, nullptr, this, nullptr);
    m_quickSearchEdit->hide();
    m_quickSearchEdit->deleteLater();
    m_quickSearchEdit = 0;
    m_SearchResults.clear();
    m_sCurrentSearchSelection = "";

    update();
}

void QHyperGraphWidget::OnCancelSearch()
{
    if (m_pQuickSearchNode)
    {
        m_pGraph->RemoveNode(m_pQuickSearchNode);
    }

    disconnect(m_quickSearchEdit, nullptr, this, nullptr);
    m_quickSearchEdit->hide();
    m_quickSearchEdit->deleteLater();
    m_quickSearchEdit = 0;
    m_SearchResults.clear();
    m_sCurrentSearchSelection = "";
    m_pQuickSearchNode = NULL;

    update();
}

void QHyperGraphWidget::OnSearchNodes(const QString& text)
{
    if (!m_pGraph || !m_pGraph->GetManager() || !m_pQuickSearchNode)
    {
        return;
    }

    NodeFilter filter(m_componentViewMask);
    std::vector<_smart_ptr<CHyperNode> > prototypes;
    m_pGraph->GetManager()->GetPrototypesEx(prototypes, true, functor_ret(filter, &NodeFilter::Visit));

    m_SearchResults.clear();
    m_sCurrentSearchSelection = "";
    QString fullname = "";
    QString fullnameLower = "";
    QString quickSearchtext = text.toLower();

    QStringList quickSearchTokens = quickSearchtext.split(" ");

    for (std::vector<_smart_ptr<CHyperNode> >::iterator iter = prototypes.begin(); iter != prototypes.end(); ++iter)
    {
        CHyperNode* pPrototypeNode = (*iter).get();

        if (pPrototypeNode && (pPrototypeNode->IsFlowNode() || pPrototypeNode->IsEditorSpecialNode()) && (!pPrototypeNode->IsObsolete() || m_componentViewMask & EFLN_OBSOLETE))
        {
            fullname = pPrototypeNode->GetUIClassName();
            fullnameLower = fullname.toLower();

            if (!fullnameLower.contains(":"))
            {
                fullnameLower = QString("misc:") + fullnameLower;
                fullname = QString("Misc:") + fullname;
            }

            if (QuickSearchMatch(quickSearchTokens, fullnameLower) && !m_SearchResults.contains(fullname))
            {
                m_SearchResults.push_back(fullname);
            }
        }
    }

    CQuickSearchNode* pQuickSearchNode = static_cast<CQuickSearchNode*>(m_pQuickSearchNode.get());

    if (!pQuickSearchNode)
    {
        return;
    }

    if (m_SearchResults.size() == 0)
    {
        m_sCurrentSearchSelection = "";
        pQuickSearchNode->SetSearchResultCount(0);
        pQuickSearchNode->SetIndex(0);
        m_pQuickSearchNode->SetName("NO SEARCH RESULTS...");
    }
    else
    {
        pQuickSearchNode->SetSearchResultCount(m_SearchResults.size());
        pQuickSearchNode->SetIndex(1);

        fullname = m_SearchResults.front();
        m_pQuickSearchNode->SetName(fullname);
        m_sCurrentSearchSelection = fullname;
    }
}

bool QHyperGraphWidget::QuickSearchMatch(const QStringList& tokens, const QString& name)
{
    //If each token exists, show the result
    const int count = tokens.size();
    for (int i = 0; i < count; i++)
    {
        if (!name.contains(tokens[i]))
        {
            return false;
        }
    }

    return true;
}

void QHyperGraphWidget::OnSelectNextSearchResult(bool bUp)
{
    if (!m_pQuickSearchNode)
    {
        return;
    }
    if (m_SearchResults.empty())
    {
        return;
    }

    CQuickSearchNode* pQuickSearchNode = static_cast<CQuickSearchNode*>(m_pQuickSearchNode.get());

    if (!pQuickSearchNode)
    {
        return;
    }

    if (bUp)
    {
        QStringList::iterator iter = m_SearchResults.end();
        while (iter != m_SearchResults.begin())
        {
            --iter;
            QString name = (*iter);

            if (name == m_sCurrentSearchSelection)
            {
                if (iter != m_SearchResults.begin())
                {
                    --iter;
                }
                pQuickSearchNode->SetIndex(pQuickSearchNode->GetIndex() - 1);
                if (iter == m_SearchResults.begin())
                {
                    iter = m_SearchResults.end();
                    --iter;
                    pQuickSearchNode->SetIndex(m_SearchResults.size());
                }

                m_pQuickSearchNode->SetName(*iter);
                m_sCurrentSearchSelection = (*iter);
                break;
            }
        }
    }
    else
    {
        for (QStringList::iterator iter = m_SearchResults.begin(); iter != m_SearchResults.end(); ++iter)
        {
            QString name = (*iter);

            if (name == m_sCurrentSearchSelection)
            {
                ++iter;
                pQuickSearchNode->SetIndex(pQuickSearchNode->GetIndex() + 1);

                if (iter == m_SearchResults.end())
                {
                    iter = m_SearchResults.begin();
                    pQuickSearchNode->SetIndex(1);
                }

                m_pQuickSearchNode->SetName(*iter);
                m_sCurrentSearchSelection = (*iter);
                break;
            }
        }
    }
}

void QHyperGraphWidget::UpdateNodeEntity(CHyperNode* pNode, const uint cmd)
{
    if (pNode->CheckFlag(EHYPER_NODE_ENTITY))
    {
        CFlowNode* pFlowNode = static_cast<CFlowNode*>(pNode);

        if (cmd == ID_GRAPHVIEW_TARGET_SELECTED_ENTITY)
        {
            pFlowNode->SetSelectedEntity();
        }
        else if (cmd == ID_GRAPHVIEW_TARGET_GRAPH_ENTITY)
        {
            pFlowNode->SetDefaultEntity();
        }
        else
        {
            pFlowNode->SetEntity(NULL);
        }

        pFlowNode->Invalidate(true);
        if (m_pGraph)
        {
            GetIEditor()->GetFlowGraphManager()->SendNotifyEvent(EHG_NODE_UPDATE_ENTITY, 0, pFlowNode);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (m_pGraph && event->mimeData()->hasFormat("application/x-component-prototype"))
    {
        event->acceptProposedAction();
    }
}

//////////////////////////////////////////////////////////////////////////
void QHyperGraphWidget::dropEvent(QDropEvent* event)
{
    QString componentType;
    QByteArray data = event->mimeData()->data("application/x-component-prototype");
    QDataStream stream(&data, QIODevice::ReadOnly);
    stream >> componentType;

    CreateNode(componentType, event->pos());
}


#include <HyperGraph/HyperGraphWidget.moc>
