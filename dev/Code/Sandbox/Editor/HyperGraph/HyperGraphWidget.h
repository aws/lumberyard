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
#ifndef CRYINCLUDE_EDITOR_HYPERGRAPH_HYPERGRAPHWIDGET_H
#define CRYINCLUDE_EDITOR_HYPERGRAPH_HYPERGRAPHWIDGET_H
#pragma once


#include "HyperGraph.h"
#include <Controls/ReflectedPropertyControl/ReflectedPropertiesPanel.h>

#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QTextEdit>

class QRubberBand;
class CHyperGraphDialog;

class CEditPropertyCtrl
    : public ReflectedPropertyControl
{
public:
    CEditPropertyCtrl(QWidget *parent = nullptr)
        : ReflectedPropertyControl(parent)
        , m_pQView(0)
        {
            setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
        }
    void SetView(QHyperGraphWidget* pView) { m_pQView = pView; };
    void SelectItem(ReflectedPropertyItem* item) override;
    void Expand(ReflectedPropertyItem* item, bool bExpand) override;

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    QHyperGraphWidget* m_pQView;
};

class QHyperGraphWidgetSinglelineEdit
    : public QLineEdit
{
    Q_OBJECT
public:
    QHyperGraphWidgetSinglelineEdit(QWidget* parent = 0)
        : QLineEdit(parent) {}
signals:
    void editingCancelled();
    void selectNextSearchResult(bool);

protected:
    void keyPressEvent(QKeyEvent* event)
    {
        switch (event->key())
        {
        case Qt::Key_Escape:
            editingCancelled();
            break;
        case Qt::Key_Up:
            selectNextSearchResult(true);
            break;
        case Qt::Key_Down:
        case Qt::Key_Tab:
            selectNextSearchResult(false);
            break;
        }
        QLineEdit::keyPressEvent(event);
    }

    void keyReleaseEvent(QKeyEvent* event)
    {
        // Prevent propagation of this event further up the chain by accepting the event
        event->accept();
    }
};

class QHyperGraphWidgetMultilineEdit
    : public QTextEdit
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText);
public:
    QHyperGraphWidgetMultilineEdit(QWidget* parent = 0)
        : QTextEdit(parent) {}

    QString text() const { return toPlainText(); }
    void setText(const QString& text) { setPlainText(text); }

signals:
    void editingFinished();
    void editingCancelled();

protected:
    void focusOutEvent(QFocusEvent*) { editingFinished(); }
    void keyPressEvent(QKeyEvent* event)
    {
        if (event->key() == Qt::Key_Escape)
        {
            editingCancelled();
        }
        QTextEdit::keyPressEvent(event);
    }
    void keyReleaseEvent(QKeyEvent* event)
    {
        QTextEdit::keyReleaseEvent(event);
        // After the base class has a chance at it, prevent propagation of this event further up the chain by accepting the event
        event->accept();
    }
};

class QHyperGraphWidget
    : public QWidget
    , public IHyperGraphListener
{
    Q_OBJECT
public:
    QHyperGraphWidget(QWidget* parent = 0);
    ~QHyperGraphWidget();

    // Assign graph to the view.
    void SetGraph(CHyperGraph* pGraph);
    CHyperGraph* GetGraph() const { return m_pGraph; }

    void SetPropertyCtrl(ReflectedPropertyControl* propsWnd);

    // Transform local rectangle into the view rectangle.
    QRect LocalToViewRect(const QRectF& localRect) const;
    QRectF ViewToLocalRect(const QRect& viewRect) const;

    QPoint LocalToView(const QPointF& point);
    QPointF ViewToLocal(const QPoint& point);

    CHyperNode* CreateNode(const QString& sNodeClass, const QPoint& point);

    // Retrieve all nodes in give view rectangle, return true if any found.
    bool GetNodesInRect(const QRect& viewRect, std::multimap<int, CHyperNode*>& nodes, bool bFullInside = false);
    // Retrieve all selected nodes in graph, return true if any selected.
    enum SelectionSetType
    {
        SELECTION_SET_ONLY_PARENTS,
        SELECTION_SET_NORMAL,
        SELECTION_SET_INCLUDE_RELATIVES
    };
    bool GetSelectedNodes(std::vector<CHyperNode*>& nodes, SelectionSetType setType = SELECTION_SET_NORMAL);

    void ClearSelection();

    // scroll/zoom pToNode (of current graph) into view and optionally select it
    void ShowAndSelectNode(CHyperNode* pToNode, bool bSelect = true);

    // Invalidates hypergraph view.
    void InvalidateView(bool bComplete = false);

    void FitFlowGraphToView();

    //////////////////////////////////////////////////////////////////////////
    // IHyperGraphListener implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void OnHyperGraphEvent(IHyperNode* pNode, EHyperGraphEvent event);
    //////////////////////////////////////////////////////////////////////////

    void SetComponentViewMask(uint32 mask)  { m_componentViewMask = mask; }

    CHyperNode* GetNodeAtPoint(const QPoint& point);
    CHyperNode* GetMouseSensibleNodeAtPoint(const QPoint& point);

    void ShowEditPort(CHyperNode* pNode, CHyperNodePort* pPort);
    void HideEditPort();
    void UpdateFrozen();

    // Centers the view over an specific node.
    void CenterViewAroundNode(CHyperNode* poNode, bool boFitZoom = false, float fZoom = -1.0f);

public slots:
    void OnCommandDelete();
    //  void OnCommandDeleteKeepLinks();
    void OnCommandCopy();
    void OnCommandPaste();
    void OnCommandPasteWithLinks();
    void OnCommandCut();
    void OnCreateGroupFromSelection();
    void OnRemoveGroupsInSelection();
    void SetIgnoreHyperGraphEvents(bool ignore) { m_bIgnoreHyperGraphEvents = ignore; }
    void PropagateChangesToGroupsAndPrefabs();

signals:
    void exportSelectionRequested();
    void importRequested();

protected:
    friend class CHyperGraphDialog;

    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

    void OnMouseMove(Qt::KeyboardModifiers, const QPoint& point);
    void OnMouseWheel(Qt::KeyboardModifiers modifiers, short zDelta, const QPoint& pt);
    void OnLButtonDown(Qt::KeyboardModifiers modifiers, const QPoint& point);
    void OnLButtonUp(Qt::KeyboardModifiers modifiers, const QPoint& point);
    void OnRButtonDown(Qt::KeyboardModifiers modifiers, const QPoint& point);
    void OnRButtonUp(Qt::KeyboardModifiers modifiers, const QPoint& point);
    void OnMButtonDown(Qt::KeyboardModifiers modifiers, const QPoint& point);
    void OnMButtonUp(Qt::KeyboardModifiers modifiers, const QPoint& point);
    void OnLButtonDblClk(Qt::KeyboardModifiers modifiers, const QPoint& point);
    void OnSetCursor();

protected slots:
    void OnHScroll();
    void OnVScroll();

protected:
    void OnCommandFitAll();
    void OnSelectEntity();
    //void OnToggleMinimize();

    virtual void ShowContextMenu(const QPoint& point, CHyperNode* pNode);
    virtual void UpdateTooltip(CHyperNode* pNode, CHyperNodePort* pPort);

    void OnMouseMoveScrollMode(Qt::KeyboardModifiers modifiers, const QPoint& point);
    void OnMouseMovePortDragMode(Qt::KeyboardModifiers modifiers, const QPoint& point);
    void OnMouseMoveBorderDragMode(Qt::KeyboardModifiers modifiers, const QPoint& point);

    //// this function is called before the common rename edit box shows up
    //// return true if you handled the rename by yourself,
    //// return false if it should bring up the common edit box
    virtual bool HandleRenameNode(CHyperNode* pNode) { return false; }
    virtual void OnNodesConnected() {};
    virtual void OnEdgeRemoved() {};
    virtual void OnNodeRenamed() {};

    void InternalPaste(bool bWithLinks, const QPoint& point, std::vector<CHyperNode*>* pPastedNodes = NULL);
    void RenameNode(CHyperNode* pNode);

private:
    // Draw graph nodes in rectangle.
    void DrawNodes(QPainter* gr, const QRect& rc);
    void DrawEdges(QPainter* gr, const QRect& rc);
    // return the helper rectangle
    QRectF DrawArrow(QPainter* gr, CHyperEdge* pEdge, bool helper, int selection = 0);
    void ReroutePendingEdges();
    void RerouteAllEdges();

    //////////////////////////////////////////////////////////////////////////
    void MoveSelectedNodes(const QPoint& offset);

    void DrawGrid(QPainter* gr, const QRect& updateRect);

    struct SClassMenuGroup
    {
        SClassMenuGroup()
            : pMenu(new QMenu) { }

        SClassMenuGroup* CreateSubMenu()
        {
            lSubMenus.push_back(SClassMenuGroup());
            return &(*lSubMenus.rbegin());
        }

        DECLARE_SMART_POINTERS(QMenu);
        QMenuPtr pMenu;
        std::list<SClassMenuGroup> lSubMenus;
    };

    void PopulateClassMenu(SClassMenuGroup& classMenu, QStringList& classes);
    void PopulateSubClassMenu(SClassMenuGroup& classMenu, QStringList& classes, int baseIndex);
    void ShowPortsConfigMenu(const QPoint& point, bool bInputs, CHyperNode* pNode);
    void AddEntityMenu(CHyperNode* pNode, QMenu& menu);
    void HandleEntityMenu(CHyperNode* pNode, const uint entry);
    void UpdateNodeEntity(CHyperNode* pNode, const uint cmd);

    void InvalidateEdgeRect(const QPoint& p1, const QPoint& p2);
    void InvalidateEdge(CHyperEdge* pEdge);
    void InvalidateNode(CHyperNode* pNode, bool bRedraw = false, bool bIsModified = true);

    //////////////////////////////////////////////////////////////////////////
    void UpdateWorkingArea();
    void SetScrollExtents(bool isUpdateWorkingArea = true);
    //////////////////////////////////////////////////////////////////////////

private Q_SLOTS:
    void OnAcceptRename();
    void OnCancelRename();
    void OnAcceptSearch();
    void OnCancelSearch();
    void OnSearchNodes(const QString& text);
    void OnSelectNextSearchResult(bool bUp);
    void OnShortcutZoomIn();
    void OnShortcutZoomOut();
    void OnShortcutQuickNode();
    void OnShortcutRefresh();

private:
    void QuickSearchNode(CHyperNode* pNode);
    void OnSelectionChange();
    void UpdateNodeProperties(CHyperNode* pNode, IVariable* pVar);
    void OnUpdateProperties(IVariable* pVar);
    void OnToggleMinimizeNode(CHyperNode* pNode);

    bool HitTestEdge(const QPoint& pnt, CHyperEdge*& pEdge, int& nIndex);

    // Simulate a flow (trigger input of target node)
    void SimulateFlow(CHyperEdge* pEdge);
    void UpdateDebugCount(CHyperNode* pNode);

    void SetZoom(float zoom);
    void NotifyZoomChangeToNodes();

    void CopyLinks(CHyperNode* pNode, bool isInput);
    void PasteLinks(CHyperNode* pNode, bool isInput);
    void DeleteLinks(CHyperNode* pNode, bool isInput);

    void SetupLayerList(CVarBlock* pVarBlock);

    bool QuickSearchMatch(const QStringList& tokens, const QString& name);

    void OnCreateModuleFromSelection(bool isGlobal);
private:
    enum EOperationMode
    {
        NothingMode = 0,
        ScrollZoomMode,
        ScrollMode,
        ZoomMode,
        MoveMode,
        SelectMode,
        PortDragMode,
        EdgePointDragMode,
        ScrollModeDrag,
        NodeBorderDragMode,
    };

    bool   m_bRefitGraphInOnSize;
    QPoint m_RButtonDownPos;
    QPoint m_mouseDownPos;
    QPoint m_scrollOffset;
    QRubberBand* m_rcSelect;
    EOperationMode m_mode;

    // Port we are dragging now.
    std::vector< _smart_ptr<CHyperEdge> > m_draggingEdges;
    bool m_bDraggingFixedOutput;

    _smart_ptr<CHyperEdge> m_pHitEdge;
    int m_nHitEdgePoint;
    float m_prevCornerW, m_prevCornerH;

    typedef std::set< _smart_ptr<CHyperEdge> > PendingEdges;
    PendingEdges m_edgesToReroute;

    std::map<_smart_ptr<CHyperNode>, QPointF> m_moveHelper;

    QRectF m_workingArea;
    float m_zoom;

    bool m_bSplineArrows;
    bool    m_bCopiedBlackBox;

    QWidget* m_renameEdit;
    _smart_ptr<CHyperNode> m_pRenameNode;
    _smart_ptr<CHyperNode> m_pEditedNode;
    std::vector<CHyperNode*> m_pMultiEditedNodes;
    _smart_ptr<CVarBlock> m_pMultiEditVars;

    QHyperGraphWidgetSinglelineEdit* m_quickSearchEdit;
    _smart_ptr<CHyperNode> m_pQuickSearchNode;
    QStringList m_SearchResults;
    QString m_sCurrentSearchSelection;

    _smart_ptr<CHyperNode> m_pMouseOverNode;
    CHyperNodePort* m_pMouseOverPort;

    ReflectedPropertyControl* m_pPropertiesCtrl;

    // Current graph.
    CHyperGraph* m_pGraph;

    uint32 m_componentViewMask;

    QScrollBar* m_horizontalScrollBar;
    int m_horizontalScrollBarPos;
    QScrollBar* m_verticalScrollBar;
    int m_verticalScrollBarPos;

    // used when draging the border of a node
    struct TDraggingBorderNodeInfo
    {
        _smart_ptr<CHyperNode> m_pNode;
        QPointF m_origNodePos;
        QRectF m_origBorderRect;
        QPointF m_clickPoint;
        int m_border; // actually is an EHyperNodeSubObjectId, but is managed as int in many other places, so
    } m_DraggingBorderNodeInfo;

    QPoint prevMovePoint;

    CEditPropertyCtrl m_editParamCtrl;
    bool m_isEditPort;
    CHyperNodePort* m_pEditPort;
    CHyperNode* m_pSrcDragNode;

    bool m_bIsFrozen;
    bool m_bIgnoreHyperGraphEvents;
    bool m_bIsClosing;
};


#endif
