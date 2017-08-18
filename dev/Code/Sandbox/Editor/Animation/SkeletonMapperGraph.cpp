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

#include "stdafx.h"

#include "SkeletonMapperGraph.h"

#include <QMenu>
#include <QAction>
#include <QVBoxLayout>
#include "QtUtil.h"

#include <QtWinMigrate/qwinwidget.h>

using namespace Skeleton;

/*

  CMapperGraphManager::CNode

*/


CMapperGraphManager::CNode::CNode()
{
    SetName("Node");
    SetClass("Node");

    IVariablePtr variable = new CVariableVoid();

    variable->SetName("P");
    variable->SetHumanName("P");
    AddPort(CHyperNodePort(variable, true));

    variable->SetName("O");
    variable->SetHumanName("O");
    AddPort(CHyperNodePort(variable, true));
}

CMapperGraphManager::CNode::~CNode()
{
}

/*

  CMapperGraphManager::COperator

*/

CMapperGraphManager::COperator::COperator(CMapperOperator* pOperator)
{
    SetName(pOperator->GetClassName());
    SetClass(pOperator->GetClassName());

    m_operator = pOperator;

    uint32 positionCount = pOperator->GetPositionCount();
    uint32 orientationCount = pOperator->GetOrientationCount();

    char name[4] = { '\0' };
    for (uint32 i = 0; i < positionCount; ++i)
    {
        ::sprintf_s(name, "P%d", i);
        IVariablePtr variable = new CVariableVoid();
        variable->SetName(name);
        variable->SetHumanName(name);
        AddPort(CHyperNodePort(variable, true));
    }

    for (uint32 i = 0; i < orientationCount; ++i)
    {
        ::sprintf_s(name, "O%d", i);
        IVariablePtr variable = new CVariableVoid();
        variable->SetName(name);
        variable->SetHumanName(name);
        AddPort(CHyperNodePort(variable, true));
    }

    IVariablePtr variable = new CVariableVoid();
    variable->SetName("Out");
    variable->SetHumanName("Out");
    AddPort(CHyperNodePort(variable, false, true));
}

CMapperGraphManager::COperator::~COperator()
{
}

//

bool CMapperGraphManager::COperator::GetPositionIndexFromPort(uint16 port, uint32& index)
{
    if (port >= m_operator->GetPositionCount())
    {
        return false;
    }

    index = port;
    return true;
}

bool CMapperGraphManager::COperator::GetOrientationIndexFromPort(uint16 port, uint32& index)
{
    if (port < m_operator->GetPositionCount())
    {
        return false;
    }
    port -= m_operator->GetPositionCount();
    if (port >= m_operator->GetOrientationCount())
    {
        return false;
    }

    index = port;
    return true;
}
bool CMapperGraphManager::COperator::SetInput(uint16 port, CMapperGraphManager::COperator* pOperator)
{
    uint32 index;
    if (GetPositionIndexFromPort(port, index))
    {
        GetOperator()->SetPosition(index, pOperator->GetOperator());
        return true;
    }
    if (GetOrientationIndexFromPort(port, index))
    {
        GetOperator()->SetOrientation(index, pOperator->GetOperator());
        return true;
    }
    return false;
}

// CHyperNode

void CMapperGraphManager::COperator::Serialize(XmlNodeRef& node, bool bLoading, CObjectArchive* ar)
{
    CHyperNode::Serialize(node, bLoading, ar);

    if (bLoading)
    {
        m_operator->SerializeFrom(node);
    }
    else
    {
        m_operator->SerializeTo(node);
    }
}

/*

  CMapperGraphManager::COperatorIndexed

*/

CMapperGraphManager::COperatorIndexed::COperatorIndexed(uint32 index)
    : COperator(CMapperOperatorDesc::Create(index))
{
    m_index = index;
}

/*

  CMapperGraph

*/

CMapperGraph* CMapperGraph::Create()
{
    CHyperGraph* pGraph = CMapperGraphManager::Instance().CreateGraph();
    if (!pGraph)
    {
        return NULL;
    }

    return (CMapperGraph*)pGraph;
}

//

CMapperGraph::CMapperGraph(CHyperGraphManager* pManager)
    : CHyperGraph(pManager)
{
    SetName("SkeletonMapper");
    AddListener(this);
}

CMapperGraph::~CMapperGraph()
{
}

//

void CMapperGraph::AddNode(const char* name)
{
    if (FindNode_(name) >= 0)
    {
        return;
    }

    m_nodes.push_back(name);
}

int32 CMapperGraph::FindNode_(const char* name)
{
    uint32 nodeCount = uint32(m_nodes.size());
    for (uint32 i = 0; i < nodeCount; ++i)
    {
        if (::_stricmp(m_nodes[i], name))
        {
            continue;
        }

        return int32(i);
    }

    return -1;
}

void CMapperGraph::ClearNodes()
{
    m_nodes.clear();
}

void CMapperGraph::AddLocation(const char* name)
{
    if (FindLocation(name) >= 0)
    {
        return;
    }

    m_locations.push_back(name);
}

int32 CMapperGraph::FindLocation(const char* name)
{
    uint32 locationCount = uint32(m_locations.size());
    for (uint32 i = 0; i < locationCount; ++i)
    {
        if (::_stricmp(m_locations[i], name))
        {
            continue;
        }

        return int32(i);
    }

    return -1;
}

void CMapperGraph::ClearLocations()
{
    m_locations.clear();
}

bool CMapperGraph::Initialize()
{
    const CHierarchy& hierarchy = m_mapper.GetHierarchy();

    ClearNodes();
    uint32 nodeCount = hierarchy.GetNodeCount();
    for (uint32 i = 0; i < nodeCount; ++i)
    {
        const CHierarchy::SNode* pNode = hierarchy.GetNode(i);
        if (!pNode)
        {
            continue;
        }

        AddNode(pNode->name);
    }

    m_locations.clear();
    uint32 locationCount = m_mapper.GetLocationCount();
    for (uint32 i = 0; i < locationCount; ++i)
    {
        AddLocation(m_mapper.GetLocation(i)->GetName());
    }

    return true;
}

void CMapperGraph::UpdateMapper()
{
    m_mapper.ClearLocations();

    std::vector<CHyperEdge*> edges;
    GetAllEdges(edges);
    uint32 edgeCount = uint32(edges.size());
    for (uint32 i = 0; i < edgeCount; ++i)
    {
        CHyperNode* pOut = (CHyperNode*)FindNode(edges[i]->nodeOut);
        if (!pOut)
        {
            continue;
        }

        bool bLocation = false;
        if (pOut->GetClassName().compare("Location", Qt::CaseInsensitive) == 0)
        {
            bLocation = true;
        }

        CHyperNode* pIn = (CHyperNode*)FindNode(edges[i]->nodeIn);
        if (!pIn)
        {
            continue;
        }
        bool bNode = false;
        if (pIn->GetClassName().compare("Node", Qt::CaseInsensitive) == 0)
        {
            bNode = true;
        }

        const QString outName = pOut->GetName();
        const QString inName = pIn->GetName();

        CMapperGraphManager::COperator* pOperator =
            (CMapperGraphManager::COperator*)pOut;

        if (bLocation)
        {
            m_mapper.SetLocation(*(CMapperLocation*)pOperator->GetOperator());
        }

        if (bNode)
        {
            int32 outputIndex = m_mapper.GetHierarchy().FindNodeIndexByName(inName.toLatin1().data());
            if (outputIndex < 0)
            {
                continue;
            }

            if (edges[i]->portIn.compare("P", Qt::CaseInsensitive) == 0)
            {
                m_mapper.GetNode(outputIndex)->position = pOperator->GetOperator();
            }
            else if (edges[i]->portIn.compare("O", Qt::CaseInsensitive) == 0)
            {
                m_mapper.GetNode(outputIndex)->orientation = pOperator->GetOperator();
            }
        }
        else
        {
            ((CMapperGraphManager::COperator*)pIn)->SetInput(edges[i]->nPortIn, pOperator);
        }
    }
}

bool CMapperGraph::SerializeTo(XmlNodeRef& node)
{
    if (!m_mapper.SerializeTo(node))
    {
        return false;
    }

    XmlNodeRef graph = node->newChild("Graph");
    Serialize(graph, false);
    return true;
}

bool CMapperGraph::SerializeFrom(XmlNodeRef& node)
{
    if (!m_mapper.SerializeFrom(node))
    {
        return false;
    }

    XmlNodeRef graph = node->findChild("Graph");
    if (!graph)
    {
        return false;
    }

    Serialize(graph, true);
    return true;
}

/*

  CMapperGraphManager

*/

CMapperGraphManager::CMapperGraphManager()
{
    m_prototypes["Node"] = new CNode();
    m_prototypes["Location"] = new CLocation();

    uint32 operatorCount = CMapperOperatorDesc::GetCount();
    for (uint32 i = 0; i < operatorCount; ++i)
    {
        m_prototypes[CMapperOperatorDesc::GetName(i)] = new COperatorIndexed(i);
    }
}

//

CMapperGraph* CMapperGraphManager::Load(const char* path)
{
    XmlNodeRef node = GetIEditor()->GetSystem()->LoadXmlFromFile(path);
    if (!node)
    {
        return NULL;
    }

    CMapperGraph* pGraph = (CMapperGraph*)CreateGraph();
    if (!pGraph)
    {
        return NULL;
    }

    if (!pGraph->SerializeFrom(node))
    {
        pGraph->Release();
        return NULL;
    }

    return pGraph;
}

// CHyperGraphManager

void CMapperGraphManager::ReloadClasses()
{
    assert(false);
}

CHyperGraph* CMapperGraphManager::CreateGraph()
{
    CMapperGraph* pGraph = new CMapperGraph(this);
    pGraph->AddRef();
    return pGraph;
}

CHyperNode* CMapperGraphManager::CreateNode(CHyperGraph* pGraph, const char* sNodeClass, HyperNodeID nodeId, const QPointF& pos)
{
    return CHyperGraphManager::CreateNode(pGraph, sNodeClass, nodeId, pos);
}


IMPLEMENT_DYNAMIC(CMapperGraphViewWrapper, CWnd)

BEGIN_MESSAGE_MAP(CMapperGraphViewWrapper, CWnd)
//{{AFX_MSG_MAP(CWnd)
ON_WM_CREATE()
ON_WM_SIZE()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
// CMapperGraphViewWrapper
//////////////////////////////////////////////////////////////////////////
CMapperGraphViewWrapper::CMapperGraphViewWrapper(CWnd* parent)
    : m_view(0)
{
    Create(NULL, "CMapperGraphViewWrapper", WS_CHILD | WS_VISIBLE, CRect(0, 0, 10, 10), parent, NULL);
}

int CMapperGraphViewWrapper::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
    {
        return -1;
    }
    m_widget.reset(new QWinWidget(this));
    m_view = new CMapperGraphView;
    QVBoxLayout* l = new QVBoxLayout;
    l->setMargin(0);
    l->addWidget(m_view);
    m_widget->setLayout(l);
    m_widget->show();
    return 0;
}

void CMapperGraphViewWrapper::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    RECT rect;
    GetClientRect(&rect);
    m_widget->setGeometry(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
}


/*

  CMapperGraphView

*/

static const uint32 mapperGraphViewIgnoreOperatorCount = 2;

//

CMapperGraphView::CMapperGraphView()
{
}

CMapperGraphView::~CMapperGraphView()
{
}

//

bool CMapperGraphView::CreateMenuLocation(QMenu& menu)
{
    CMapperGraph* pGraph = GetGraph();
    if (!pGraph)
    {
        return false;
    }

    uint32 count = pGraph->GetLocationCount();
    if (!count)
    {
        return false;
    }

    for (uint32 i = 0; i < count; ++i)
    {
        menu.addAction(pGraph->GetLocationName(i))->setData(eID_Location + i);
    }
    return true;
}

bool CMapperGraphView::CreateMenuNode(QMenu& menu)
{
    CMapperGraph* pGraph = GetGraph();
    if (!pGraph)
    {
        return false;
    }

    uint32 count = pGraph->GetNodeCount();
    if (!count)
    {
        return false;
    }

    for (uint32 i = 0; i < count; ++i)
    {
        menu.addAction(pGraph->GetNodeName(i))->setData(eID_Node + i);
    }
    return true;
}

bool CMapperGraphView::CreateMenuOperator(QMenu& menu)
{
    CMapperGraph* pGraph = GetGraph();
    if (!pGraph)
    {
        return false;
    }

    QStringList prototypes;
    pGraph->GetManager()->GetPrototypes(prototypes, true);
    uint32 prorotypeCount = uint32(prototypes.size());

    for (uint32 i = ::mapperGraphViewIgnoreOperatorCount; i < prorotypeCount; ++i)
    {
        menu.addAction(prototypes[i])->setData(eID_Operator + i - ::mapperGraphViewIgnoreOperatorCount);
    }

    return true;
}

//

CHyperNode* CMapperGraphView::FindNode(const char* className, const char* name)
{
    CMapperGraph* pGraph = GetGraph();
    if (!pGraph)
    {
        return false;
    }

    IHyperGraphEnumerator* pEnumerator = pGraph->GetNodesEnumerator();
    CHyperNode* pNode = (CHyperNode*)pEnumerator->GetFirst();
    while (pNode)
    {
        if (pNode->GetClassName().compare(className, Qt::CaseInsensitive) == 0 &&
            pNode->GetName().compare(name, Qt::CaseInsensitive) == 0)
        {
            break;
        }

        pNode = (CHyperNode*)pEnumerator->GetNext();
    }
    pEnumerator->Release();

    return pNode;
}

CHyperNode* CMapperGraphView::CreateNode(const char* className, const char* name, const QPoint& point, bool bUnique)
{
    CMapperGraph* pGraph = GetGraph();
    if (!pGraph)
    {
        return NULL;
    }

    if (bUnique)
    {
        if (CHyperNode* pNode = FindNode(className, name))
        {
            pNode->SetPos(QPointF(point.x(), point.y()));
            return pNode;
        }
    }

    CHyperNode* pNode = QHyperGraphWidget::CreateNode(className, point);
    if (!pNode)
    {
        return pNode;
    }

    pNode->SetName(name);
    return pNode;
}

//

bool CMapperGraphView::CreateLocation(uint32 index, const QPoint& point)
{
    CMapperGraph* pGraph = GetGraph();
    if (!pGraph)
    {
        return false;
    }

    uint32 count = pGraph->GetLocationCount();
    if (index >= count)
    {
        return false;
    }

    const char* name = pGraph->GetLocationName(index);
    if (!name)
    {
        return false;
    }

    CHyperNode* pNode = FindNode("Location", name);
    if (pNode)
    {
        pNode->SetPos(QPointF(point.x(), point.y()));
        return true;
    }

    pNode = CreateOperator("Location", name, new CMapperLocation(), point, true);
    if (!pNode)
    {
        return false;
    }

    pNode->SetName(name);
    pNode->SetClass("Location");
    return true;
}

bool CMapperGraphView::CreateNode(uint32 index, const QPoint& point)
{
    CMapperGraph* pGraph = GetGraph();
    if (!pGraph)
    {
        return false;
    }

    uint32 count = pGraph->GetNodeCount();
    if (index >= count)
    {
        return false;
    }

    const char* name = pGraph->GetNodeName(index);
    if (!name)
    {
        return false;
    }

    return CreateNode("Node", name, point) != NULL;
}

CMapperGraphManager::COperator* CMapperGraphView::CreateOperator(const char* className, const char* name, CMapperOperator* pOperator, const QPoint& point, bool bUnique)
{
    CMapperGraph* pGraph = GetGraph();
    if (!pGraph)
    {
        return NULL;
    }

    CHyperNode* pNode = CreateNode(className, name, point, bUnique);
    if (!pNode)
    {
        return NULL;
    }

    CMapperGraphManager::COperator* pGraphOperator =
        (CMapperGraphManager::COperator*)pNode;
    return pGraphOperator;
}

CMapperGraphManager::COperator* CMapperGraphView::CreateOperator(uint32 index, const QPoint& point)
{
    index += ::mapperGraphViewIgnoreOperatorCount;

    CMapperGraph* pGraph = GetGraph();
    if (!pGraph)
    {
        return NULL;
    }

    QStringList prototypes;
    pGraph->GetManager()->GetPrototypes(prototypes, true);
    uint32 prototypeCount = uint32(prototypes.size());
    if (index >= prototypeCount)
    {
        return NULL;
    }

    CHyperNode* pNode = QHyperGraphWidget::CreateNode(prototypes[index], point);
    return (CMapperGraphManager::COperator*)pNode;
}

void CMapperGraphView::Save()
{
    CMapperGraph* pGraph = GetGraph();
    if (!pGraph)
    {
        return;
    }

    CFileDialog fileDialog(false);
    if (fileDialog.DoModal() == IDCANCEL)
    {
        return;
    }

    XmlNodeRef node = GetIEditor()->GetSystem()->CreateXmlNode("Mapper");
    pGraph->SerializeTo(node);
    /*
        XmlNodeRef locations = node->newChild("HierarchyLocations");
        CHierarchy hierarchy;
        pGraph->GetMapper().CreateLocationsHierarchy(hierarchy);
        hierarchy.SerializeTo(locations);
    */
    node->saveToFile(fileDialog.GetPathName());
}
/*
void CMapperGraphView::Open()
{
    CMapperGraph* pGraph = GetGraph();
    if (!pGraph)
        return;

    CFileDialog fileDialog(true);
    if (fileDialog.DoModal() == IDCANCEL)
        return;

    XmlNodeRef node = GetIEditor()->GetSystem()->LoadXmlFromFile(fileDialog.GetPathName());
    pGraph->SerializeFrom(node);
}
*/

void CMapperGraphView::ShowContextMenu(const QPoint& point, CHyperNode* pNode)
{
    CMapperGraph* pGraph = GetGraph();
    if (!pGraph)
    {
        return;
    }

    QMenu menu;
    QMenu menuLocation("Location");
    QMenu menuNode("Node");
    QMenu menuOperator("Operator");
    if (pNode)
    {
        menu.addAction("Delete")->setData(eID_Delete);
    }
    else
    {
        if (CreateMenuLocation(menuLocation))
        {
            menu.addMenu(&menuLocation);
        }
        if (CreateMenuNode(menuNode))
        {
            menu.addMenu(&menuNode);
        }
        if (CreateMenuOperator(menuOperator))
        {
            menu.addMenu(&menuOperator);
        }
        menu.addSeparator();
        menu.addAction("Save")->setData(eID_Save);
        //      menu.addAction("Open")->setData(eID_Open);
    }

    int command = menu.exec(QCursor::pos())->data().toInt();

    switch (command)
    {
    case eID_Delete:
        OnCommandDelete();
        break;

    case eID_Save:
        Save();
        break;
    /*
        case eID_Open:
            Open();
            break;
    */
    default:
        if (command >= eID_Location && command < eID_LocationEnd)
        {
            CreateLocation(command - eID_Location, point);
        }
        else if (command >= eID_Node && command < eID_NodeEnd)
        {
            CreateNode(command - eID_Node, point);
        }
        else if (command >= eID_Operator && command < eID_OperatorEnd)
        {
            CreateOperator(command - eID_Operator, point);
        }
    }
}

// IHyperGraphListener

void CMapperGraphView::OnHyperGraphEvent(IHyperNode* pNode, EHyperGraphEvent event)
{
    QHyperGraphWidget::OnHyperGraphEvent(pNode, event);

    if (m_listeners.empty())
    {
        return;
    }

    if (event != EHG_NODE_CHANGE)
    {
        return;
    }

    std::vector<CHyperNode*> nodes;
    if (!GetSelectedNodes(nodes))
    {
        if (m_selection.empty())
        {
            return;
        }

        uint32 listenerCount = uint32(m_listeners.size());
        for (uint32 i = 0; i < listenerCount; ++i)
        {
            m_listeners[i]->OnSelection(m_selection, nodes);
        }
        m_selection.clear();
        return;
    }

    bool bChanged = m_selection.size() != nodes.size();
    if (!bChanged)
    {
        uint32 minCount = min(m_selection.size(), nodes.size());
        for (uint32 i = 0; i < minCount; ++i)
        {
            if (m_selection[i] == nodes[i])
            {
                continue;
            }

            bChanged = true;
            break;
        }
    }

    if (!bChanged)
    {
        return;
    }

    uint32 listenerCount = uint32(m_listeners.size());
    for (uint32 i = 0; i < listenerCount; ++i)
    {
        m_listeners[i]->OnSelection(m_selection, nodes);
    }

    m_selection.clear();
    uint32 count = uint32(nodes.size());
    for (uint32 i = 0; i < count; ++i)
    {
        m_selection.push_back(nodes[i]);
    }
}

/*

  CMapperDialog

*/

IMPLEMENT_DYNCREATE(CMapperDialog, CBaseFrameWnd)

//

CMapperDialog::CMapperDialog()
{
}

CMapperDialog::~CMapperDialog()
{
}

//

BOOL CMapperDialog::OnInitDialog()
{
    if (!CBaseFrameWnd::OnInitDialog())
    {
        return false;
    }

    m_splitter.CreateStatic(this, 1, 2);
    m_splitter.SetSplitterStyle(XT_SPLIT_NOFULLDRAG | XT_SPLIT_NOBORDER);
    m_splitter.SetColumnInfo(0, 160, 0);

    m_properties.SetParent(&m_splitter);
    m_properties.SetDlgCtrlID(m_splitter.IdFromRowCol(0, 0));
    m_properties.ShowWindow(SW_SHOWDEFAULT);

    m_graphViewWrapper.reset(new Skeleton::CMapperGraphViewWrapper(&m_splitter));
    m_graphViewWrapper->SetDlgCtrlID(m_splitter.IdFromRowCol(0, 1));
    m_graphViewWrapper->ShowWindow(SW_SHOWDEFAULT);
    m_graphView = m_graphViewWrapper->m_view;
    m_graphView->AddListener(this);

    return true;
}

void CMapperDialog::RegisterViewClass()
{
    class CViewClass
        : public TRefCountBase<CViewPaneClass>
    {
        virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; };
        virtual REFGUID ClassID()
        {
            // {F60742A0-44FB-4a63-B469-6F163688AD0D}
            static const GUID guid = {
                0xf60742a0, 0x44fb, 0x4a63, { 0xb4, 0x69, 0x6f, 0x16, 0x36, 0x88, 0xad, 0xd }
            };
            return guid;
        }
        virtual const char* ClassName() { return "SkeletonMapper"; };
        virtual const char* Category() { return "SkeletonMapper"; };
        virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CMapperDialog); };
        virtual unsigned int GetPaneTitleID() const { return IDS_SKELETON_MAPPER_TITLE; }
        virtual EDockingDirection GetDockingDirection() { return DOCK_FLOAT; };
        virtual CRect GetPaneRect() { return CRect(100, 100, 1000, 800); };
        virtual bool SinglePane() { return false; };
        virtual bool WantIdleUpdate() { return true; };
    };

    ::GetIEditor()->GetClassFactory()->RegisterClass(new CViewClass());
}
