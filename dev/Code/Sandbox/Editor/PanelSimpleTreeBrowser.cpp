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
#include "PanelSimpleTreeBrowser.h"

#include "ViewManager.h"
#include "IThreadTask.h"
#include "QtUI/WaitCursor.h"

#include <io.h>
#include <StringUtils.h>

BEGIN_MESSAGE_MAP(CSimpleTreeBrowser, CXTResizeDialog)
ON_EN_CHANGE(IDC_FILTER, OnFilterChange)
ON_WM_DESTROY()
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////////////
CSimpleTreeBrowser::CSimpleTreeBrowser(int Height /*= 0*/, int iddImageList /*= 0*/, CWnd* pParent /*= NULL*/)
    : CXTResizeDialog(CSimpleTreeBrowser::IDD, pParent)
    , m_pScanner(NULL)
    , m_iHeight(Height)
    , m_iddImageList(iddImageList)
    , m_onSelectCallback(NULL)
    , m_onDblClickCallback(NULL)
    , m_CMCallbackCreate(NULL)
    , m_CMCallbackCommand(NULL)
    , m_bOnSelectCallbackEnabled(true)
    , m_bOnDblClickCallbackEnabled(true)
    , m_bOnDragAndDropCallbackEnabled(true)
    , m_bContextMenuCallbacksEnabled(true)
    , m_lastSelectedRecord("")
{
}

//////////////////////////////////////////////////////////////////////////
CSimpleTreeBrowser::~CSimpleTreeBrowser()
{
    ClearTreeItems();
}

//////////////////////////////////////////////////////////////////////////
void CSimpleTreeBrowser::DoDataExchange(CDataExchange* pDX)
{
    CXTResizeDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_FILTER, m_filter);
}

/////////////////////////////////////////////////////////////////////////////
BOOL CSimpleTreeBrowser::OnInitDialog()
{
    CXTResizeDialog::OnInitDialog();
    CMFCUtils::LoadTrueColorImageList(m_cImageList, m_iddImageList, 16, RGB(255, 0, 255));

    CRect rc;
    GetClientRect(rc);
    rc.DeflateRect(5, 5, 5, 64);

    m_treeCtrl.Create(WS_CHILD | WS_VISIBLE | WS_BORDER, rc, this, IDC_BROWSER_TREE);
    m_treeCtrl.EnableAutoNameGrouping(true, 0);
    m_treeCtrl.SetImageList(&m_cImageList);
    m_treeCtrl.SetCallback(CTreeCtrlReport::eCB_OnSelect, functor(*this, &CSimpleTreeBrowser::OnSelectionChanged));
    m_treeCtrl.SetCallback(CTreeCtrlReport::eCB_OnDragAndDrop, functor(*this, &CSimpleTreeBrowser::OnDragAndDrop));
    m_treeCtrl.SetCallback(CTreeCtrlReport::eCB_OnDblClick, functor(*this, &CSimpleTreeBrowser::OnDblclkBrowserTree));

    CXTPReportColumn* pCol1 = m_treeCtrl.AddTreeColumn("");

    pCol1->SetSortable(TRUE);
    m_treeCtrl.GetColumns()->SetSortColumn(pCol1, TRUE);
    m_treeCtrl.SetExpandOnDblClick(true);

    SetResize(IDC_BROWSER_TREE, SZ_RESIZE(1));
    SetResize(IDC_STATIC, SZ_BOTTOM_LEFT, SZ_BOTTOM_LEFT);
    SetResize(IDC_FILTER, SZ_BOTTOM_LEFT, SZ_BOTTOM_RIGHT);


    if (m_iHeight > 0)
    {
        GetWindowRect(&rc);
        SetWindowPos(NULL, 0, 0, (long)rc.Width(), (long)m_iHeight, SWP_NOMOVE);
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
void CSimpleTreeBrowser::OnDestroy()
{
    CXTResizeDialog::OnDestroy();
}

//////////////////////////////////////////////////////////////////////////
void CSimpleTreeBrowser::Create(ISimpleTreeBrowserScanner* pScanner, CWnd* parent)
{
    CXTResizeDialog::Create(IDD, parent);
    m_pScanner = pScanner;
    Refresh();
}

//////////////////////////////////////////////////////////////////////////
void CSimpleTreeBrowser::SetAutoResize(bool bEnabled, int minHeight /*= 0*/, int maxHeight /*= 0*/)
{
    m_bAutoResize = bEnabled;
    m_iMinHeight = minHeight;
    m_iMaxHeight = maxHeight;
    AutoResize();
}

//////////////////////////////////////////////////////////////////////////
void CSimpleTreeBrowser::OnFilterChange()
{
    CString filter;

    m_filter.GetWindowText(filter);
    m_treeCtrl.SetFilterText(filter);
    ReselectLastSelectedTreeItem();
}

//////////////////////////////////////////////////////////////////////////
void CSimpleTreeBrowser::Refresh()
{
    if (m_pScanner)
    {
        WaitCursor wait;

        m_pScanner->Scan();
        LoadFilesFromScanning();
    }
}

//////////////////////////////////////////////////////////////////////////
void CSimpleTreeBrowser::ClearTreeItems()
{
    m_treeCtrl.DeleteAllItems();
    m_treeCtrl.GetRecords()->RemoveAll();
    m_CurrItems.clear();
    m_lastSelectedRecord = "";
}

//////////////////////////////////////////////////////////////////////////
void CSimpleTreeBrowser::LoadFilesFromScanning()
{
    if (m_pScanner && m_pScanner->HasNewFiles())
    {
        ClearTreeItems();

        TSimpleTreeBrowserItems items;
        m_pScanner->GetScannedFiles(items);
        m_CurrItems = items;

        m_treeCtrl.BeginUpdate();
        AddItemsToTree(m_CurrItems);
        m_treeCtrl.EndUpdate();
        m_treeCtrl.Populate();

        AutoResize();
        ReselectLastSelectedTreeItem();
    }
}

//////////////////////////////////////////////////////////////////////////
void CSimpleTreeBrowser::AddItemsToTree(const TSimpleTreeBrowserItems& items, CTreeItemRecord* pParent)
{
    for (size_t i = 0, iCount = items.size(); i < iCount; i++)
    {
        const SSimpleTreeBrowserItem& item = items[i];

        CTreeItemRecord* pRec = new CTreeItemRecord(false, item.DisplayName);
        pRec->SetUserData((void*) &item);
        pRec->SetIcon(item.Icon);
        pRec->SetUserString(string().Format("%s_%i", item.DisplayName.GetString(), (int) item.UserData).c_str());
        pRec->GetItem(0)->SetTooltip(item.Tooltip);

        m_treeCtrl.AddTreeRecord(pRec, pParent);
        AddItemsToTree(item.Children, pRec);
    }
}

//////////////////////////////////////////////////////////////////////////
void CSimpleTreeBrowser::ReselectLastSelectedTreeItem()
{
    if (!m_lastSelectedRecord.IsEmpty())
    {
        m_treeCtrl.SelectRecordByUserString(m_lastSelectedRecord);
    }
}

//////////////////////////////////////////////////////////////////////////
void CSimpleTreeBrowser::AutoResize()
{
    if (m_bAutoResize && m_iMinHeight > 0 && m_iMaxHeight > 0)
    {
        int newHeight = m_CurrItems.size() * 28 + 50;
        newHeight = max(min(newHeight, m_iMaxHeight), m_iMinHeight);

        CRect rc;
        GetWindowRect(&rc);
        SetWindowPos(NULL, 0, 0, (long)rc.Width(), (long)newHeight, SWP_NOMOVE);
    }
}

//////////////////////////////////////////////////////////////////////////
void CSimpleTreeBrowser::OnSelectionChanged(CTreeItemRecord* pRec)
{
    m_lastSelectedRecord = pRec ? pRec->GetUserString() : "";
    if (m_bOnSelectCallbackEnabled && pRec && m_onSelectCallback)
    {
        m_onSelectCallback((SSimpleTreeBrowserItem*) pRec->GetUserData());
    }
}

//////////////////////////////////////////////////////////////////////////
void CSimpleTreeBrowser::OnDragAndDrop(CTreeItemRecord* pRec)
{
    if (m_bOnDragAndDropCallbackEnabled)
    {
        CPoint pt;
        GetCursorPos(&pt);
        CViewport* pView = GetIEditor()->GetViewManager()->GetViewportAtPoint(pt);
        if (pView)
        {
            TEventCallback& cb = m_onDragAndDropCallbacks[pView->GetType()];
            if (cb)
            {
                cb((SSimpleTreeBrowserItem*) pRec->GetUserData());
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CSimpleTreeBrowser::OnDblclkBrowserTree(CTreeItemRecord* pRec)
{
    if (m_bOnDblClickCallbackEnabled && pRec && m_onDblClickCallback)
    {
        m_onDblClickCallback((SSimpleTreeBrowserItem*) pRec->GetUserData());
    }
}

