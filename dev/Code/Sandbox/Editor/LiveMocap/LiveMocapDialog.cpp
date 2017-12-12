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
#include "LiveMocapDialog.h"

#include "../Objects/EntityObject.h"

#include "../Dialogs/Generic/StringInputDialog.h"

#include <QVBoxLayout>

/*

  CLiveMocapTreeConnection

*/

CLiveMocapTreeConnection::CLiveMocapTreeConnection()
{
}

CLiveMocapTreeConnection::~CLiveMocapTreeConnection()
{
}

//

CLiveMocapConnection* CLiveMocapTreeConnection::IsConnection(HTREEITEM hItem)
{
    if (GetParentItem(hItem))
    {
        return NULL;
    }

    return (CLiveMocapConnection*)GetItemData(hItem);
}

CLiveMocapActor* CLiveMocapTreeConnection::IsActor(HTREEITEM hItem)
{
    if (!GetParentItem(hItem))
    {
        return NULL;
    }

    return (CLiveMocapActor*)GetItemData(hItem);
}

HTREEITEM CLiveMocapTreeConnection::FindConnectionItem(const CLiveMocapConnection* pConnection)
{
    if (!pConnection)
    {
        return NULL;
    }

    HTREEITEM hItem = GetFirstVisibleItem();
    while (hItem)
    {
        HTREEITEM hNext = GetNextItem(hItem, TVGN_NEXT);
        if (pConnection == IsConnection(hItem))
        {
            return hItem;
        }
        hItem = hNext;
    }

    return hItem;
}

HTREEITEM CLiveMocapTreeConnection::FindConnectionItem(const CLiveMocapScene* pScene)
{
    if (!pScene)
    {
        return NULL;
    }

    HTREEITEM hItem = GetFirstVisibleItem();
    while (hItem)
    {
        HTREEITEM hNext = GetNextItem(hItem, TVGN_NEXT);
        if (CLiveMocapConnection* pConnection = IsConnection(hItem))
        {
            if (pScene == &pConnection->GetScene())
            {
                return hItem;
            }
        }
        hItem = hNext;
    }

    return hItem;
}

CLiveMocapConnection* CLiveMocapTreeConnection::GetSelectedConnection()
{
    HTREEITEM hItem = GetSelectedItem();
    if (!hItem)
    {
        return NULL;
    }

    return IsConnection(hItem);
}

CLiveMocapActor* CLiveMocapTreeConnection::GetSelectedActor()
{
    HTREEITEM hItem = GetSelectedItem();
    if (!hItem)
    {
        return NULL;
    }

    return IsActor(hItem);
}

IEntity* CLiveMocapTreeConnection::GetEditorSelectedEntity()
{
    CBaseObject* pObject = ::GetIEditor()->GetSelectedObject();
    if (!pObject)
    {
        return NULL;
    }

    if (!pObject->IsKindOf(RUNTIME_CLASS(CEntityObject)))
    {
        return NULL;
    }

    return ((CEntityObject*)pObject)->GetIEntity();
}

void CLiveMocapTreeConnection::RefreshConnection(HTREEITEM hConnection, bool bForceExpand)
{
    CLiveMocapConnection* pConnection = IsConnection(hConnection);
    if (!pConnection)
    {
        return;
    }

    HTREEITEM hChild = GetChildItem(hConnection);
    while (hChild)
    {
        HTREEITEM hNext = GetNextItem(hChild, TVGN_NEXT);
        DeleteItem(hChild);
        hChild = hNext;
    }

    const CLiveMocapScene& scene = pConnection->GetScene();
    uint32 actorCount = scene.GetActorCount();
    for (uint32 i = 0; i < actorCount; ++i)
    {
        const CLiveMocapActor* pActor = scene.GetActor(i);
        string entryText = GetSubjectName(pActor);
        hChild = InsertItem(entryText.c_str(), hConnection, TVI_LAST);
        SetItemData(hChild, (DWORD_PTR)pActor);
    }

    if (bForceExpand)
    {
        Expand(hConnection, TVE_EXPAND);
    }

    UpdateWindow();
    SetRedraw(TRUE);
}

void CLiveMocapTreeConnection::RefreshConnections()
{
    CLiveMocap& liveMocap = CLiveMocap::Instance();

    DeleteAllItems();
    size_t connectionCount = liveMocap.GetConnectionCount();
    for (size_t i = 0; i < connectionCount; ++i)
    {
        CLiveMocapConnection* pConnection = liveMocap.GetConnection(i);
        HTREEITEM hConnection = InsertItem(pConnection->GetName(), TVI_ROOT, TVI_LAST);
        SetItemState(hConnection, TVIS_EXPANDPARTIAL, TVIS_EXPANDPARTIAL);
        SetItemData(hConnection, (DWORD_PTR)pConnection);

        RefreshConnection(hConnection, false);
    }
}

//

void CLiveMocapTreeConnection::CreateMenuConnection(CLiveMocapConnection& connection, CMenu& menu)
{
    menu.CreatePopupMenu();

    menu.AppendMenu(NULL, eID_Connection_SetEntity, "SetEntity");
    menu.AppendMenu(MF_SEPARATOR);

    if (connection.IsConnected())
    {
        menu.AppendMenu(NULL, eID_Connection_Disconnect, "Disconnect");
    }
    else
    {
        menu.AppendMenu(NULL, eID_Connection_Connect, "Connect");
    }
}

void CLiveMocapTreeConnection::Create(CWnd* pParent, UINT nID)
{
    CTreeCtrl::Create(WS_CHILD | WS_VISIBLE | TVS_HASBUTTONS | TVS_LINESATROOT, CRect(0, 0, 0, 0), pParent, nID);

    m_menuConnection.CreatePopupMenu();
    m_menuConnection.AppendMenu(NULL, eID_Connection_SetEntity, "SetEntity");
    m_menuConnection.AppendMenu(MF_SEPARATOR);
    m_menuConnection.AppendMenu(NULL, eID_Connection_Connect, "Connect");

    m_menuActor.CreatePopupMenu();
    m_menuActor.AppendMenu(MF_STRING, eID_Actor_SetEntity, "SetEntity");
    m_menuActor.AppendMenu(MF_STRING, eID_Actor_UnSetEntity, "UnsetEntity");
    m_menuActor.AppendMenu(MF_SEPARATOR);

    m_menuMappingGraph.CreatePopupMenu();
    m_menuMappingGraph.AppendMenu(MF_STRING, eID_Actor_MappingGraph_New, "New");
    m_menuMappingGraph.AppendMenu(MF_STRING, eID_Actor_MappingGraph_Open, "Open");
    m_menuActor.AppendMenu(MF_POPUP, (UINT_PTR)m_menuMappingGraph.GetSafeHmenu(), "Mapping Graph");
}

//

BEGIN_MESSAGE_MAP(CLiveMocapTreeConnection, CTreeCtrl)
ON_WM_RBUTTONDOWN()
ON_WM_RBUTTONUP()

ON_COMMAND_RANGE(eID_AddConnection, eID_AddConnectionLast, &OnAddConnection)

ON_BN_CLICKED(eID_Connection_Connect, &OnConnection_Connect)
ON_BN_CLICKED(eID_Connection_Disconnect, &OnConnection_Disconnect)
ON_BN_CLICKED(eID_Connection_SetEntity, &OnConnection_SetEntity)

ON_BN_CLICKED(eID_Actor_SetEntity, &OnActor_SetEntity)
ON_BN_CLICKED(eID_Actor_UnSetEntity, &OnActor_UnSetEntity)
ON_BN_CLICKED(eID_Actor_MappingGraph_New, &OnActor_MappingGraph_New)
ON_BN_CLICKED(eID_Actor_MappingGraph_Open, &OnActor_MappingGraph_Open)
END_MESSAGE_MAP()

afx_msg void CLiveMocapTreeConnection::OnRButtonDown(UINT nFlags, CPoint point)
{
    HTREEITEM hItem = HitTest(point);
    if (!hItem)
    {
        return;
    }

    SelectItem(hItem);
}

static std::vector<string> g_foundPlugins;
static void EnumeratePlugins()
{
    // Enumerate Available MoCap Plugins
#ifdef WIN64
    string mocapPluginPath = BINFOLDER_NAME "/LiveMocap";
#else
    string mocapPluginPath = "Bin32/LiveMocap";
#endif
    if (!PathFileExists(mocapPluginPath.c_str()))
    {
        CryLogAlways("Can't find MoCap plugin directory '%s'", mocapPluginPath.c_str());
        return;
    }

    CFileEnum cDLLFiles;
    __finddata64_t sFile;
    g_foundPlugins.clear();
    if (cDLLFiles.StartEnumeration(mocapPluginPath.c_str(), "*.DLL", &sFile))
    {
        int i = 0;
        do
        {
            ++i;
            // store every find in a vector
            string newFileName = sFile.name;
            newFileName.erase(newFileName.length() - 4, 4);
            bool is64BitDLL = newFileName.find("64", newFileName.length() - 3) != string::npos;
            ;
#ifdef WIN64
            if (is64BitDLL)
#else
            if (!is64BitDLL)
#endif
            {
                g_foundPlugins.push_back(newFileName);
            }
        } while (cDLLFiles.GetNextFile(&sFile) && (i <= LIVEMOCAP_CONNECTION_MAX));
    }
}

afx_msg void CLiveMocapTreeConnection::OnRButtonUp(UINT nFlags, CPoint point)
{
    UINT uFlags;
    HTREEITEM hItem = HitTest(point, &uFlags);

    // select so the item will be highlighted in blue
    SelectItem(hItem);

    ClientToScreen(&point);

    if (!hItem) // clicked on white space
    {
        CMenu menu;
        menu.CreatePopupMenu();

        // Enumerate Available MoCap Plugins
        EnumeratePlugins();

        // add a menu entry for each one
        string connectionName;
        for (int i = 0; (i < g_foundPlugins.size()) && (eID_AddConnection + i < eID_AddConnectionLast); ++i)
        {
            connectionName.Format("Add %s Connection", g_foundPlugins[i].c_str());
            menu.AppendMenu(NULL, eID_AddConnection + i, connectionName.c_str());
        }

        if (g_foundPlugins.size() == 0)
        {
            CryLogAlways("No Live-MoCap plugins found.");
        }
        else
        {
            menu.TrackPopupMenu(NULL, point.x, point.y, this);
        }
    }
    else if (CLiveMocapConnection* pConnection = IsConnection(hItem))
    {
        //      m_menuConnection.TrackPopupMenu(NULL, point.x, point.y, this);
        CMenu menu;
        CreateMenuConnection(*pConnection, menu);
        menu.TrackPopupMenu(NULL, point.x, point.y, this);
    }
    else if (IsActor(hItem))
    {
        m_menuActor.TrackPopupMenu(NULL, point.x, point.y, this);
    }
}

afx_msg void CLiveMocapTreeConnection::OnAddConnection(UINT nID)
{
    int index = nID - eID_AddConnection;
    if (index < g_foundPlugins.size())
    {
        CLiveMocap& liveMocap = CLiveMocap::Instance();
        liveMocap.AddConnection(g_foundPlugins[index]);
        RefreshConnections();
    }
}

afx_msg void CLiveMocapTreeConnection::OnConnection_Connect()
{
    CLiveMocapConnection* pConnection = GetSelectedConnection();
    if (!pConnection)
    {
        return;
    }

    CStringInputDialog address;
    address.SetTitle(CString(MAKEINTRESOURCE(IDS_LIVE_MOCAP_DIALOG_CONNECTION_TITLE)));
    address.SetText(pConnection->m_lastConnection.c_str());
    if (address.DoModal() != 1)
    {
        return;
    }

    if (!pConnection->Connect(address.GetResultingText()))
    {
        QMessageBox:::warning(nullptr, QString(), "Was unable to connect!");
        return;
    }

    pConnection->m_lastConnection = address.GetResultingText();
    pConnection->GetScene().AddListener(this);

    if (HTREEITEM hItem = GetSelectedItem())
    {
        Expand(hItem, TVE_EXPAND);
    }
}

afx_msg void CLiveMocapTreeConnection::OnConnection_Disconnect()
{
    CLiveMocapConnection* pConnection = GetSelectedConnection();
    if (!pConnection)
    {
        return;
    }

    pConnection->Disconnect();
    //  pConnection->GetScene().DeleteActors();
    //  pConnection->Reset();

    HTREEITEM hItem = FindConnectionItem(pConnection);
    if (!hItem)
    {
        return;
    }

    RefreshConnection(hItem, false);
}

afx_msg void CLiveMocapTreeConnection::OnConnection_SetEntity()
{
    CLiveMocapConnection* pConnection = GetSelectedConnection();
    if (!pConnection)
    {
        return;
    }

    IEntity* pEntity = GetEditorSelectedEntity();
    if (!pEntity)
    {
        return;
    }

    pConnection->GetScene().SetEntity(pEntity);
}

afx_msg void CLiveMocapTreeConnection::OnActor_SetEntity()
{
    CLiveMocapActor* pActor = GetSelectedActor();
    if (!pActor)
    {
        return;
    }

    IEntity* pEntity = GetEditorSelectedEntity();
    if (!pEntity)
    {
        return;
    }

    pActor->SetEntity(pEntity);

    // update the name of the assigned entity in the tree list
    //RefreshConnections();

    HTREEITEM hItem = GetSelectedItem();
    if (hItem)
    {
        string entryText = GetSubjectName(pActor);
        SetItemText(hItem, entryText);
    }
}

afx_msg void CLiveMocapTreeConnection::OnActor_UnSetEntity()
{
    CLiveMocapActor* pActor = GetSelectedActor();
    if (!pActor)
    {
        return;
    }

    IEntity* pEntity = pActor->GetEntity();
    pActor->SetEntity(NULL);

    HTREEITEM hItem = GetSelectedItem();
    if (hItem)
    {
        string entryText = GetSubjectName(pActor);
        SetItemText(hItem, entryText);
    }
}


afx_msg void CLiveMocapTreeConnection::OnActor_MappingGraph_New()
{
    CLiveMocapActor* pActor = GetSelectedActor();
    if (!pActor)
    {
        return;
    }

    Skeleton::CMapperGraph* pGraph = pActor->CreateSkeletonMapperGraph();
    m_pGraphView->SetGraph(pGraph);
}

afx_msg void CLiveMocapTreeConnection::OnActor_MappingGraph_Open()
{
    CLiveMocapActor* pActor = GetSelectedActor();
    if (!pActor)
    {
        return;
    }

    CFileDialog fileDialog(true);
    if (fileDialog.DoModal() == IDCANCEL)
    {
        return;
    }

    Skeleton::CMapperGraph* pGraph =
        Skeleton::CMapperGraphManager::Instance().Load(fileDialog.GetPathName());
    pActor->SetSkeletonMapperGraph(pGraph);

    m_pGraphView->SetGraph(pGraph);
}

// ILiveMocapSceneListener
void CLiveMocapTreeConnection::OnCreateActor(const CLiveMocapScene& scene, const CLiveMocapActor& actor)
{
    HTREEITEM hConnection = FindConnectionItem(&scene);
    if (!hConnection)
    {
        return;
    }

    RefreshConnection(hConnection);
}

string CLiveMocapTreeConnection::GetSubjectName(const CLiveMocapActor* pActor)
{
    string entryText(pActor->GetName());
    entryText.append(" - ");
    entryText.append(pActor->GetEntityName());
    return entryText;
}

/*

  CLiveMocapDialog

*/

void CLiveMocapDialog::RegisterViewClass()
{
    class CViewClass
        : public TRefCountBase<CViewPaneClass>
    {
        virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; };
        virtual REFGUID ClassID()
        {
            // {CD9DEEBE-C417-482b-8A6E-2D4E01EA67B3}
            static const GUID guid = {
                0xcd9deebe, 0xc417, 0x482b, { 0x8a, 0x6e, 0x2d, 0x4e, 0x1, 0xea, 0x67, 0xb3 }
            };
            return guid;
        }
        virtual const char* ClassName() { return "LiveMocap"; };
        virtual const char* Category() { return "LiveMocap"; };
        virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CLiveMocapDialog); };
        virtual unsigned int GetPaneTitleID() const { return IDS_LIVE_MOCAP_WINDOW_TITLE; }
        virtual EDockingDirection GetDockingDirection() { return DOCK_FLOAT; };
        virtual CRect GetPaneRect() { return CRect(100, 100, 1000, 800); };
        virtual bool SinglePane() { return false; };
        virtual bool WantIdleUpdate() { return true; };
    };

    ::GetIEditor()->GetClassFactory()->RegisterClass(new CViewClass());
}

//

IMPLEMENT_DYNCREATE(CLiveMocapDialog, CBaseFrameWnd)

BEGIN_MESSAGE_MAP(CLiveMocapDialog, CBaseFrameWnd)
ON_NOTIFY(TVN_SELCHANGED, AFX_IDW_PANE_FIRST, OnTreeSelection)     // TEMP FIX!
END_MESSAGE_MAP()

//

CLiveMocapDialog::CLiveMocapDialog()
{
    Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), ::AfxGetMainWnd());
}

CLiveMocapDialog::~CLiveMocapDialog()
{
}

//

void CLiveMocapDialog::DoDataExchange(CDataExchange* pDX)
{
    CBaseFrameWnd::DoDataExchange(pDX);
}

BOOL CLiveMocapDialog::OnInitDialog()
{
    if (!CBaseFrameWnd::OnInitDialog())
    {
        return false;
    }

    m_splitters[0].CreateStatic(this, 1, 2, WS_CHILD | WS_VISIBLE);
    m_splitters[0].SetSplitterStyle(XT_SPLIT_NOFULLDRAG | XT_SPLIT_NOBORDER | XT_SPLIT_DOTTRACKER);
    m_splitters[0].SetColumnInfo(0, 160, 0);

    m_splitters[1].CreateStatic(&m_splitters[0], 3, 1, WS_CHILD | WS_VISIBLE);
    m_splitters[1].SetSplitterStyle(XT_SPLIT_NOFULLDRAG | XT_SPLIT_NOBORDER | XT_SPLIT_DOTTRACKER);
    m_splitters[1].SetRowInfo(0, 160, 0);
    m_splitters[1].SetRowInfo(1, 160, 0);
    m_splitters[1].SetDlgCtrlID(m_splitters[0].IdFromRowCol(0, 0));

    m_tree.Create(&m_splitters[1], eID_Tree);
    m_tree.SetDlgCtrlID(m_splitters[1].IdFromRowCol(0, 0));

    m_properties.SetParent(&m_splitters[1]);
    m_properties.SetDlgCtrlID(m_splitters[1].IdFromRowCol(1, 0));
    m_properties.ShowWindow(SW_SHOWDEFAULT);

    m_graphProperties.SetParent(&m_splitters[1]);
    m_graphProperties.SetDlgCtrlID(m_splitters[1].IdFromRowCol(2, 0));
    m_graphProperties.ShowWindow(SW_SHOWDEFAULT);

    m_graphViewWrapper.reset(new Skeleton::CMapperGraphViewWrapper(&m_splitters[0]));
    m_graphViewWrapper->SetDlgCtrlID(m_splitters[0].IdFromRowCol(0, 1));
    m_graphViewWrapper->ShowWindow(SW_SHOWDEFAULT);
    m_graphView = m_graphViewWrapper->m_view;
    m_graphView->AddListener(this);

    m_tree.m_pGraphView = m_graphView;
    m_tree.RefreshConnections();

    return true;
}

//

void CLiveMocapDialog::DisplayConnectioProperties(CLiveMocapConnection* pConnection)
{
}

void CLiveMocapDialog::DisplayActorProperties(CLiveMocapActor* pActor)
{
    m_splitters[1].HideRow(1);

    CVariableArray* pLocations = new CVariableArray();
    pLocations->SetHumanName("Locations");
    pLocations->AddVariable(pActor->m_locationsShow);
    pLocations->AddVariable(pActor->m_locationsShowName);
    pLocations->AddVariable(pActor->m_locationsShowHierarchy);
    pLocations->AddVariable(pActor->m_locationsFreeze);

    CVariableArray* pEntity = new CVariableArray();
    pEntity->AddVariable(pActor->m_entityUpdate);
    pEntity->AddVariable(pActor->m_entityShowHierarchy);
    pEntity->AddVariable(pActor->m_entityScale);

    CVarBlockPtr varBlock = new CVarBlock();
    varBlock->AddVariable(pLocations);
    varBlock->AddVariable(pEntity);
    m_properties.AddVars(varBlock);

    m_splitters[1].ShowRow();
}

void CLiveMocapDialog::OnTreeSelection(NMHDR*, LRESULT*)
{
    m_properties.DeleteVars();

    if (CLiveMocapConnection* pConnection = m_tree.GetSelectedConnection())
    {
        DisplayConnectioProperties(pConnection);
        return;
    }

    if (CLiveMocapActor* pActor = m_tree.GetSelectedActor())
    {
        DisplayActorProperties(pActor);
        m_graphView->SetGraph(pActor->GetSkeletonMapperGraph());
        return;
    }
}

// Skeleton::IMapperGraphViewListener

void CLiveMocapDialog::OnSelection(const std::vector<CHyperNode*>& previous, const std::vector<CHyperNode*>& current)
{
    m_graphProperties.DeleteVars();
    if (current.empty())
    {
        return;
    }

    if (current[0]->GetClassName().compare("Node", Qt::CaseInsensitive) == 0)
    {
        return;
    }

    Skeleton::CMapperOperator* pOperator =
        ((Skeleton::CMapperGraphManager::COperator*)current[0])->GetOperator();
    if (!pOperator)
    {
        return;
    }

    uint32 parameterCount = pOperator->GetParameterCount();
    if (!parameterCount)
    {
        return;
    }

    CVarBlockPtr varBlock = new CVarBlock();
    for (uint32 i = 0; i < parameterCount; ++i)
    {
        varBlock->AddVariable(pOperator->GetParameter(i));
    }
    m_graphProperties.AddVars(varBlock);
}
