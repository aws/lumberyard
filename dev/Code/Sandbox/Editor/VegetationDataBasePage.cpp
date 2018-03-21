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
#include "VegetationDataBasePage.h"
#include "VegetationDataBaseModel.h"
#include "VegetationTool.h"
#include "Terrain/SurfaceType.h"
#include "Terrain/TerrainManager.h"
#include "QtUI/WaitCursor.h"
#include "MainWindow.h"

#include "VegetationMap.h"
#include "VegetationObject.h"
#include "StringDlg.h"
#include "Viewport.h"
#include "Material/MaterialManager.h"
#include "IViewPane.h"

#include "../RenderDll/Common/Shadow_Renderer.h"

#include "Controls/PreviewModelCtrl.h"
#include <QtViewPane.h>

#include <ui_VegetationDataBasePage.h>

#include <QAbstractProxyModel>
#include <QMenu>
#include <QMouseEvent>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>

#define VDB_COMMAND_HIDE    1
#define VDB_COMMAND_UNHIDE  2


const GUID& CVegetationDataBasePage::GetClassID()
{
    // {8903493F-8946-40ab-B7EE-EEF0C0D15E5A}
    static const GUID guid =
    {
        0x8903493f, 0x8946, 0x40ab, { 0xb7, 0xee, 0xee, 0xf0, 0xc0, 0xd1, 0x5e, 0x5a }
    };
    return guid;
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions options;
    options.isLegacyReplacement = true;
    options.sendViewPaneNameBackToAmazonAnalyticsServers = true;
    AzToolsFramework::RegisterViewPane<CVegetationDataBasePage>(LyViewPane::VegetationEditor, LyViewPane::CategoryTools, options);
}

/////////////////////////////////////////////////////////////////////////////
// CVegetationDataBasePage dialog
//////////////////////////////////////////////////////////////////////////
CVegetationDataBasePage::CVegetationDataBasePage(QWidget* pParent /*=NULL*/)
    : CDataBaseDialogPage(pParent)
    , m_ui(new Ui::CVegetationDataBasePage)
    , m_vegetationDataBaseModel(new CVegetationDataBaseModel(this))
{
    m_bIgnoreSelChange = false;
    m_bActive = false;
    m_bSyncSelection = true;

    m_nCommand = 0;

    m_vegetationMap = GetIEditor()->GetVegetationMap();

    m_ui->setupUi(this);
    m_ui->m_propertyCtrl->Setup();

    m_ui->m_splitWnd->setStretchFactor(0, 2);
    m_ui->m_splitWnd->setStretchFactor(1, 1);

    m_ui->m_wndReport->setModel(m_vegetationDataBaseModel);
    m_ui->m_wndReport->AddGroup(CVegetationDataBaseModel::ColumnCategory);
    m_ui->m_wndReport->ShowGroups(true);

    m_ui->m_wndReport->header()->resizeSection(CVegetationDataBaseModel::ColumnVisible, 24);
    m_ui->m_wndReport->header()->resizeSection(CVegetationDataBaseModel::ColumnObject, 200);
    m_ui->m_wndReport->header()->resizeSection(CVegetationDataBaseModel::ColumnCategory, 30);
    m_ui->m_wndReport->header()->resizeSection(CVegetationDataBaseModel::ColumnCount, 40);
    m_ui->m_wndReport->header()->resizeSection(CVegetationDataBaseModel::ColumnTexSize, 40);
    m_ui->m_wndReport->header()->resizeSection(CVegetationDataBaseModel::ColumnMaterial, 100);
    m_ui->m_wndReport->header()->resizeSection(CVegetationDataBaseModel::ColumnElevationMin, 40);
    m_ui->m_wndReport->header()->resizeSection(CVegetationDataBaseModel::ColumnElevationMax, 40);
    m_ui->m_wndReport->header()->resizeSection(CVegetationDataBaseModel::ColumnSlopeMin, 40);
    m_ui->m_wndReport->header()->resizeSection(CVegetationDataBaseModel::ColumnSlopeMax, 40);
    m_ui->m_wndReport->header()->setSectionsMovable(true);
    m_ui->m_wndReport->header()->setSectionHidden(CVegetationDataBaseModel::ColumnCategory, true);
    m_ui->m_wndReport->viewport()->setMouseTracking(true);
    m_ui->m_wndReport->viewport()->installEventFilter(this);

    connect(m_ui->PANEL_VEG_ADD, &ClickableLabel::linkActivated, this, &CVegetationDataBasePage::OnAdd);
    connect(m_ui->PANEL_VEG_CLONE, &ClickableLabel::linkActivated, this, &CVegetationDataBasePage::OnClone);
    connect(m_ui->PANEL_VEG_REPLACE, &ClickableLabel::linkActivated, this, &CVegetationDataBasePage::OnReplace);
    connect(m_ui->PANEL_VEG_REMOVE, &ClickableLabel::linkActivated, this, &CVegetationDataBasePage::OnRemove);
    connect(m_ui->PANEL_VEG_HIDE, &ClickableLabel::linkActivated, this, &CVegetationDataBasePage::OnHide);
    connect(m_ui->PANEL_VEG_UNHIDE, &ClickableLabel::linkActivated, this, &CVegetationDataBasePage::OnUnhide);
    connect(m_ui->PANEL_VEG_HIDEALL, &ClickableLabel::linkActivated, this, &CVegetationDataBasePage::OnHideAll);
    connect(m_ui->PANEL_VEG_UNHIDEALL, &ClickableLabel::linkActivated, this, &CVegetationDataBasePage::OnUnhideAll);

    connect(m_ui->PANEL_VEG_DISTRIBUTE, &ClickableLabel::linkActivated, this, &CVegetationDataBasePage::OnDistribute);
    connect(m_ui->PANEL_VEG_CLEAR, &ClickableLabel::linkActivated, this, &CVegetationDataBasePage::OnClear);
    connect(m_ui->PANEL_VEG_SCALE, &ClickableLabel::linkActivated, this, &CVegetationDataBasePage::OnBnClickedScale);

    connect(m_ui->PANEL_VEG_ADDCATEGORY, &ClickableLabel::linkActivated, this, &CVegetationDataBasePage::OnNewCategory);
    connect(m_ui->PANEL_VEG_CREATE_SEL, &ClickableLabel::linkActivated, this, &CVegetationDataBasePage::OnBnClickedCreateSel);
    connect(m_ui->PANEL_VEG_MERGE, &ClickableLabel::linkActivated, this, &CVegetationDataBasePage::OnBnClickedMerge);
    connect(m_ui->PANEL_VEG_IMPORT, &ClickableLabel::linkActivated, this, &CVegetationDataBasePage::OnBnClickedImport);
    connect(m_ui->PANEL_VEG_EXPORT, &ClickableLabel::linkActivated, this, &CVegetationDataBasePage::OnBnClickedExport);

    connect(m_ui->m_wndReport->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CVegetationDataBasePage::OnReportSelChange);
    connect(m_ui->m_wndReport, &QWidget::customContextMenuRequested, this, &CVegetationDataBasePage::OnReportRClick);
    connect(m_ui->m_wndReport, &QAbstractItemView::clicked, this, &CVegetationDataBasePage::OnReportClick);

    // When the vegetation editor is created as a standalone tool, we need to
    // set it as active so that it will receive updates. When it's created
    // as one of the pages in the database view, its parent is initially a
    // tab widget and gets activated through that, whereas it has no parent
    // initially when created as its own tool.
    if (!pParent)
    {
        SetActive(true);
    }
}

//////////////////////////////////////////////////////////////////////////
CVegetationDataBasePage::~CVegetationDataBasePage()
{
    GetIEditor()->UnregisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::SetActive(bool bActive)
{
    if (bActive != m_bActive)
    {
        if (bActive)
        {
            GetIEditor()->RegisterNotifyListener(this);
        }
        else
        {
            GetIEditor()->UnregisterNotifyListener(this);
        }
    }
    m_vegetationMap = GetIEditor()->GetVegetationMap();
    m_bActive = bActive;
    if (m_bActive)
    {
        ReloadObjects();
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::Update()
{
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::OnNewCategory()
{
    Selection objects;
    GetSelectedObjects(objects);
    if (!objects.empty())
    {
        const QString cat = QInputDialog::getText(this, tr("Change Selected Group(s) Category"), QString(), QLineEdit::Normal, objects[0]->GetCategory());
        if (!cat.isEmpty())
        {
            for (int i = 0; i < objects.size(); i++)
            {
                objects[i]->SetCategory(cat);
            }
            ReloadObjects();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::OnDistribute()
{
    if (QMessageBox::question(this, tr("Vegetation Distribute"), tr("Are you sure you want to Distribute?")) == QMessageBox::Yes)
    {
        CUndo undo("Vegetation Distribute");
        WaitCursor wait;

        CEditTool* pTool = GetIEditor()->GetEditTool();
        if (pTool && qobject_cast<CVegetationTool*>(pTool))
        {
            ((CVegetationTool*)pTool)->Distribute();
        }
        else
        {
            CVegetationTool* pTool = new CVegetationTool;
            pTool->Distribute();
            pTool->Release();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::OnClear()
{
    if (QMessageBox::question(this, tr("Vegetation Clear"), tr("Are you sure you want to Clear?")) == QMessageBox::Yes)
    {
        CUndo undo("Vegetation Clear");
        WaitCursor wait;

        CEditTool* pTool = GetIEditor()->GetEditTool();
        if (pTool && qobject_cast<CVegetationTool*>(pTool))
        {
            ((CVegetationTool*)pTool)->Clear();
        }
        else
        {
            CVegetationTool* pTool = new CVegetationTool;
            pTool->Clear();
            pTool->Release();
        }
        gEnv->p3DEngine->SetRecomputeCachedShadows(ShadowMapFrustum::ShadowCacheData::eFullUpdate);
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::OnBnClickedScale()
{
    CUndo undo("Vegetation Scale");

    CEditTool* pTool = GetIEditor()->GetEditTool();
    if (pTool && qobject_cast<CVegetationTool*>(pTool))
    {
        ((CVegetationTool*)pTool)->ScaleObjects();
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::OnBnClickedImport()
{
    CUndo undo("Vegetation Import");
    QString file;
    if (CFileUtil::SelectFile("Vegetation Objects (*.veg);;All Files (*)", GetIEditor()->GetLevelFolder(), file))
    {
        XmlNodeRef root = XmlHelpers::LoadXmlFromFile(file.toUtf8().data());
        if (!root)
        {
            return;
        }

        m_ui->m_propertyCtrl->RemoveAllItems();

        WaitCursor wait;
        CUndo undo("Import Vegetation");
        for (int i = 0; i < root->getChildCount(); i++)
        {
            m_vegetationMap->ImportObject(root->getChild(i));
        }
        ReloadObjects();
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::OnBnClickedMerge()
{
    if (QMessageBox::question(this, tr("Vegetation Merge"), tr("Are you sure you want to Merge?")) == QMessageBox::Yes)
    {
        WaitCursor wait;
        CUndo undo("Vegetation Merge");
        Selection objects;
        std::vector<CVegetationObject*> delObjects;
        GetSelectedObjects(objects);
        for (int i = 0; i < objects.size(); i++)
        {
            for (int j = i + 1; j < objects.size(); j++)
            //if(!strcmp(objects[i]->GetFileName(), objects[j]->GetFileName()) )
            {
                m_vegetationMap->MergeObjects(objects[i], objects[j]);
                delObjects.push_back(objects[j]);
            }
        }
        for (int k = 0; k < delObjects.size(); k++)
        {
            m_vegetationMap->RemoveObject(delObjects[k]);
        }
        ReloadObjects();
        GetIEditor()->Notify(eNotify_OnVegetationPanelUpdate);
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::OnBnClickedCreateSel()
{
    CEditTool* pTool = GetIEditor()->GetEditTool();
    if (pTool && qobject_cast<CVegetationTool*>(pTool) && ((CVegetationTool*)pTool)->GetCountSelectedInstances())
    {
        const QString name = QInputDialog::getText(this, tr("Enter Category name"), QString());
        if (!name.isEmpty())
        {
            ((CVegetationTool*)pTool)->MoveSelectedInstancesToCategory(name);
            ReloadObjects();
            GetIEditor()->Notify(eNotify_OnVegetationPanelUpdate);
        }
        MainWindow::instance()->setFocus();
    }
    else
    {
        QMessageBox::critical(this, QString(), tr("Select Instances first"));
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::OnBnClickedExport()
{
    CUndo undo("Vegetation Export");
    Selection objects;
    GetSelectedObjects(objects);
    if (objects.empty())
    {
        QMessageBox::warning(this, QString(), tr("Select Objects For Export"));
        return;
    }

    QString fileName;
    if (CFileUtil::SelectSaveFile("Vegetation Objects (*.veg)", "veg", GetIEditor()->GetLevelFolder(), fileName))
    {
        WaitCursor wait;
        XmlNodeRef root = XmlHelpers::CreateXmlNode("Vegetation");
        for (int i = 0; i < objects.size(); i++)
        {
            XmlNodeRef node = root->newChild("VegetationObject");
            m_vegetationMap->ExportObject(objects[i], node);
        }
        XmlHelpers::SaveXmlNode(GetIEditor()->GetFileUtil(), root, fileName.toStdString().c_str());
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::OnHide()
{
    WaitCursor wait;
    Selection objects;
    GetSelectedObjects(objects);
    for (int i = 0; i < objects.size(); i++)
    {
        m_vegetationMap->HideObject(objects[i], true);
    }
    ReloadObjects();
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::OnUnhide()
{
    WaitCursor wait;
    Selection objects;
    GetSelectedObjects(objects);
    for (int i = 0; i < objects.size(); i++)
    {
        m_vegetationMap->HideObject(objects[i], false);
    }
    ReloadObjects();
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::OnHideAll()
{
    WaitCursor wait;
    m_vegetationMap->HideAllObjects(true);
    gEnv->p3DEngine->SetRecomputeCachedShadows(ShadowMapFrustum::ShadowCacheData::eFullUpdate);
    ReloadObjects();
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::OnUnhideAll()
{
    WaitCursor wait;
    m_vegetationMap->HideAllObjects(false);
    ReloadObjects();
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::ReloadObjects()
{
    m_vegetationMap = GetIEditor()->GetVegetationMap();

    if (!m_bActive)
    {
        return;
    }

    /*
    // Clear all selections.
    for (int i = 0; i < m_vegetationMap->GetObjectCount(); i++)
    {
        m_vegetationMap->GetObject(i)->SetSelected(false);
    }
    // Clear items within category.
    for (cit = m_categoryMap.begin(); cit != m_categoryMap.end(); ++cit)
    {
        HTREEITEM hRoot = m_objectsTree.InsertItem( cit->first,0,1 );
        m_objectsTree.SetItemData( hRoot,(DWORD_PTR)0 );
        m_objectsTree.SetItemState(hRoot, TVIS_BOLD, TVIS_BOLD);
        cit->second = hRoot;
    }

    for (int i = 0; i < m_vegetationMap->GetObjectCount(); i++)
    {
        CVegetationObject* object = m_vegetationMap->GetObject(i);
        AddObjectToTree( object,false );
    }

    // Set initial category.
    if (m_category.IsEmpty() && !m_categoryMap.empty())
    {
        m_category = m_categoryMap.begin()->first;
    }
    m_objectsTree.Invalidate();
    */


    // sort vegetation instances
    std::vector<CVegetationObject*> vegObjs;
    vegObjs.reserve(m_vegetationMap->GetObjectCount());

    for (int i = 0; i < m_vegetationMap->GetObjectCount(); i++)
    {
        CVegetationObject* pObject = m_vegetationMap->GetObject(i);

        bool bAdded = false;

        const QByteArray chr = pObject->GetFileName().toUtf8();
        const char* chr1 = strrchr(chr, '/');
        const char* chr2 = strrchr(chr, '\\');
        if (chr2 && chr2 > chr1)
        {
            chr1 = chr2;
        }

        if (chr1 && *(chr1 + 1))
        {
            chr1++;
            for (std::vector<CVegetationObject*>::iterator itID = vegObjs.begin(); itID != vegObjs.end(); ++itID)
            {
                const QByteArray chl = (*itID)->GetFileName().toUtf8();
                const char* chl1 = strrchr(chl, '/');
                const char* chl2 = strrchr(chl, '\\');
                if (chl2 && chl2 > chl1)
                {
                    chl1 = chl2;
                }
                if (chl1 && *(chl1 + 1))
                {
                    chl1++;
                    if (strcmp(chr1, chl1) < 0)
                    {
                        vegObjs.insert(itID, pObject);
                        bAdded = true;
                        break;
                    }
                }
            }
        }

        if (!bAdded)
        {
            vegObjs.push_back(pObject);
            continue;
        }
    }

    m_vegetationDataBaseModel->setVegetationObjects(vegObjs);

    SyncSelection();

    // Give focus back to main view.
    MainWindow::instance()->setFocus();
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::SyncSelection()
{
    //////////////////////////////////////////////////////////////////////////
    // Select rows.
    QItemSelectionModel* selModel = m_ui->m_wndReport->selectionModel();
    selModel->clear();
    for (int i = 0; i < m_vegetationDataBaseModel->rowCount(); i++)
    {
        // the tree uses proxy models internally!
        const QModelIndex index = m_ui->m_wndReport->mapFromSource(m_vegetationDataBaseModel->index(i, 0));
        CVegetationObject* pObj = m_vegetationDataBaseModel->data(index, CVegetationDataBaseModel::VegetationObjectRole).value<CVegetationObject*>();
        if (pObj && pObj->IsSelected())
        {
            assert(selModel->model() == index.model());
            selModel->select(index, QItemSelectionModel::Rows | QItemSelectionModel::Select);
        }
    }
    //////////////////////////////////////////////////////////////////////////
    OnSelectionChange();
}

/*
void CVegetationDataBasePage::AddObjectToTree( CVegetationObject *object,bool bExpandCategory )
{
    CString str;
    char filename[_MAX_PATH];

    std::map<CString,HTREEITEM>::iterator cit;
    CString category = object->GetCategory();

    HTREEITEM hRoot = TVI_ROOT;
    cit = m_categoryMap.find(category);
    if (cit != m_categoryMap.end())
    {
        hRoot = cit->second;
    }
    if (hRoot == TVI_ROOT)
    {
        hRoot = m_objectsTree.InsertItem( category,0,1 );
        m_objectsTree.SetItemData( hRoot,(DWORD_PTR)0 );
        m_objectsTree.SetItemState(hRoot, TVIS_BOLD, TVIS_BOLD);
        m_categoryMap[category] = hRoot;
    }

    // Add new category item.
    _splitpath( object->GetFileName(),0,0,filename,0 );
    str.Format( "%s (%d)",filename,object->GetNumInstances() );
    HTREEITEM hItem = m_objectsTree.InsertItem( str,2,3,hRoot );
    m_objectsTree.SetItemData( hItem,(DWORD_PTR)object );
    m_objectsTree.SetCheck( hItem,!object->IsHidden() );

    if (hRoot != TVI_ROOT)
    {
        if (bExpandCategory)
            m_objectsTree.Expand(hRoot,TVE_EXPAND);

        // Determine check state of category.
        std::vector<CVegetationObject*> objects;
        GetObjectsInCategory( category,objects );
        bool anyChecked = false;
        for (int i = 0; i < objects.size(); i++)
        {
            if (!objects[i]->IsHidden())
                anyChecked = true;
        }
        m_objectsTree.SetCheck( hRoot,anyChecked );
    }
}
*/


//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::SelectObject(CVegetationObject* object)
{
    // find row.
    for (int i = 0; i < m_vegetationDataBaseModel->rowCount(); i++)
    {
        const QModelIndex index = m_ui->m_wndReport->mapFromSource(m_vegetationDataBaseModel->index(i, 0));
        CVegetationObject* pObj = index.data(CVegetationDataBaseModel::VegetationObjectRole).value<CVegetationObject*>();
        if (pObj == object)
        {
            m_ui->m_wndReport->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
            return;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::UpdateInfo()
{
    int nSelInstances = 0;
    Selection objects;
    GetSelectedObjects(objects);
    for (int i = 0; i < objects.size(); i++)
    {
        nSelInstances += objects[i]->GetNumInstances();
    }
    QString str;
    int nSelObjTexUsage = m_vegetationMap->GetTexureMemoryUsage(true);
    int nAllObjTexUsage = m_vegetationMap->GetTexureMemoryUsage(false);
    int numObjects = m_vegetationMap->GetObjectCount();
    int nAllInstances = m_vegetationMap->GetNumInstances();
    int nMemUsedBySprites = m_vegetationMap->GetSpritesMemoryUsage(true);
    int nMemUsedBySpritesAll = m_vegetationMap->GetSpritesMemoryUsage(false);
    int numSel = objects.size();

    str = tr("Groups %1 / %2\r\n").arg(numSel).arg(numObjects);
    str += tr("Instance Count:\r\n   %1 / %2\r\n").arg(nSelInstances).arg(nAllInstances);
    str += tr("Texture Usage:\r\n   %1 K / %2 K\r\n").arg(nSelObjTexUsage / 1024).arg(nAllObjTexUsage / 1024);
    str += tr("Sprite Memory Usage:\r\n   %1 K / %2 K\r\n").arg(nMemUsedBySprites / 1024).arg(nMemUsedBySpritesAll / 1024);

    // Update info in task panel.
    m_ui->INFO->setText(str);
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::OnAdd()
{
    ////////////////////////////////////////////////////////////////////////
    // Add another static object to the list
    ////////////////////////////////////////////////////////////////////////
    AssetSelectionModel selection = AssetSelectionModel::AssetGroupSelection("Geometry", true);
    AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);
    if (!selection.IsValid())
    {
        return;
    }

    QString category;
    if (m_vegetationMap->GetObjectCount() > 0)
    {
        category = m_vegetationMap->GetObject(0)->GetCategory();
    }

    Selection objects;
    GetSelectedObjects(objects);
    if (!objects.empty())
    {
        category = objects[0]->GetCategory();
    }

    WaitCursor wait;

    CUndo undo("Add VegObject(s)");
    ClearSelection();
    for (auto product : selection.GetResults())
    {
        // Create a new static object settings class
        CVegetationObject* obj = m_vegetationMap->CreateObject();
        if (!obj)
        {
            continue;
        }
        obj->SetFileName(product->GetFullPath().c_str());
        if (!category.isEmpty())
        {
            obj->SetCategory(category);
        }
        obj->SetSelected(true);
    }
    ReloadObjects();
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::OnClone()
{
    Selection objects;
    GetSelectedObjects(objects);

    CUndo undo("Clone VegObject");
    ClearSelection();
    for (int i = 0; i < objects.size(); i++)
    {
        CVegetationObject* object = objects[i];
        CVegetationObject* newObject = m_vegetationMap->CreateObject(object);
        newObject->SetSelected(true);
    }
    ReloadObjects();
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::OnReplace()
{
    Selection objects;
    GetSelectedObjects(objects);
    if (objects.size() != 1)
    {
        return;
    }

    CVegetationObject* object = objects[0];
    if (!object)
    {
        return;
    }

    AssetSelectionModel selection = AssetSelectionModel::AssetGroupSelection("Geometry");
    AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);
    if (!selection.IsValid())
    {
        return;
    }

    WaitCursor wait;
    CUndo undo("Replace VegObject");
    object->SetFileName(selection.GetResult()->GetFullPath().c_str());
    m_vegetationMap->RepositionObject(object);

    ReloadObjects();
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::OnRemove()
{
    Selection objects;
    GetSelectedObjects(objects);

    if (objects.empty())
    {
        return;
    }

    // validate
    if (QMessageBox::question(this, QString(), tr("Delete Vegetation Group(s)?")) != QMessageBox::Yes)
    {
        return;
    }

    // Unselect all instances.
    CEditTool* pTool = GetIEditor()->GetEditTool();
    if (pTool && qobject_cast<CVegetationTool*>(pTool))
    {
        ((CVegetationTool*)pTool)->ClearThingSelection();
    }

    WaitCursor wait;
    CUndo undo("Remove VegObject(s)");

    m_bIgnoreSelChange = true;
    for (int i = 0; i < objects.size(); i++)
    {
        m_vegetationMap->RemoveObject(objects[i]);
    }
    m_bIgnoreSelChange = false;
    ReloadObjects();

    GetIEditor()->Notify(eNotify_OnVegetationPanelUpdate);

    GetIEditor()->UpdateViews(eUpdateStatObj);
}

/*
//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::OnTvnBeginlabeleditObjects(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);
    if (pTVDispInfo->item.lParam || pTVDispInfo->item.pszText == 0)
    {
        // Not Category.
        // Cancel editing.
        *pResult = TRUE;
        return;
    }
    m_category = pTVDispInfo->item.pszText;
    *pResult = 0;
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::OnTvnEndlabeleditObjects(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);
    if (pTVDispInfo->item.lParam == 0 && pTVDispInfo->item.pszText != 0)
    {
        if (m_categoryMap.find(pTVDispInfo->item.pszText) != m_categoryMap.end())
        {
            // Cancel.
            *pResult = 0;
            return;
        }

        // Accept category name change.
        Selection objects;
        GetObjectsInCategory( m_category,objects );
        for (int i= 0; i < objects.size(); i++)
        {
            objects[i]->SetCategory( pTVDispInfo->item.pszText );
        }
        // Replace item in m_category map.
        m_categoryMap[pTVDispInfo->item.pszText] = m_categoryMap[m_category];
        m_categoryMap.erase( m_category );

        *pResult = TRUE; // Accept change.
        return;
    }
    *pResult = 0;
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::OnTvnHideObjects(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
    // Check if clicked on state image so we may be changed Checked state.
    *pResult = 0;

    HTREEITEM hItem = pNMTreeView->itemNew.hItem;

    WaitCursor wait;

    CVegetationObject *object = (CVegetationObject*)pNMTreeView->itemNew.lParam;
    if (object)
    {
        bool bHidden = m_objectsTree.GetCheck(hItem) != TRUE;
        // Object.
        m_vegetationMap->HideObject( object,bHidden );
    }
    else
    {
        bool bHidden = m_objectsTree.GetCheck(hItem) != TRUE;
        // Category.
        CString category = m_objectsTree.GetItemText(hItem);
        std::vector<CVegetationObject*> objects;
        GetObjectsInCategory( category,objects );
        for (int i = 0; i < objects.size(); i++)
        {
            m_vegetationMap->HideObject( objects[i],bHidden );
        }
        // for all childs of this item set same check.
        hItem = m_objectsTree.GetNextItem(hItem,TVGN_CHILD);
        while (hItem)
        {
            m_objectsTree.SetCheck( hItem,!bHidden );
            hItem = m_objectsTree.GetNextSiblingItem(hItem);
        }
    }
}
*/

void CVegetationDataBasePage::GetSelectedObjects(std::vector<CVegetationObject*>& objects)
{
    objects.clear();
    for (int i = 0; i < m_vegetationMap->GetObjectCount(); i++)
    {
        CVegetationObject* obj = m_vegetationMap->GetObject(i);
        if (obj->IsSelected())
        {
            objects.push_back(obj);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::OnSelectionChange()
{
    Selection objects;
    GetSelectedObjects(objects);

    // Delete all var block.
    m_varBlock = 0;
    m_ui->m_propertyCtrl->RemoveAllItems();

    if (objects.empty())
    {
        if (m_ui->m_previewCtrl->isEnabled() && m_ui->m_previewCtrl->isVisible())
        {
            m_ui->m_previewCtrl->LoadFile("");
        }
        m_ui->m_propertyCtrl->setEnabled(false);
        return;
    }
    else
    {
        m_ui->m_propertyCtrl->setEnabled(true);
    }

    /*
    if (objects.size() == 1)
    {
        CVegetationObject *object = objects[0];
        if (m_previewCtrl)
        m_previewCtrl.LoadFile( object->GetFileName() );

        m_propertyCtrl.DeleteAllItems();
        m_propertyCtrl.AddVarBlock( object->GetVarBlock() );
        m_propertyCtrl.ExpandAll();

        m_propertyCtrl.SetDisplayOnlyModified(false);
    }
    else
    */
    {
        m_varBlock = objects[0]->GetVarBlock()->Clone(true);
        for (int i = 0; i < objects.size(); i++)
        {
            m_varBlock->Wire(objects[i]->GetVarBlock());
        }

        // Add Surface types to varblock.
        if (objects.size() == 1)
        {
            AddLayerVars(m_varBlock, objects[0]);
        }
        else
        {
            AddLayerVars(m_varBlock);
        }

        // Add variable blocks of all objects.
        m_ui->m_propertyCtrl->AddVarBlock(m_varBlock, "Group Properties");
        m_ui->m_propertyCtrl->ExpandAll();
    }
    if (objects.size() == 1)
    {
        CVegetationObject* object = objects[0];
        if (m_ui->m_previewCtrl->isEnabled() && m_ui->m_previewCtrl->isVisible())
        {
            m_ui->m_previewCtrl->LoadFile(object->GetFileName());
        }
    }
    else
    {
        if (m_ui->m_previewCtrl->isEnabled() && m_ui->m_previewCtrl->isVisible())
        {
            m_ui->m_previewCtrl->LoadFile("");
        }
    }
    UpdateInfo();
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::AddLayerVars(CVarBlock* pVarBlock, CVegetationObject* pObject)
{
    IVariable* pTable = new CVariableArray();
    pTable->SetName("Use On Terrain Layers");
    pVarBlock->AddVariable(pTable);
    int num = GetIEditor()->GetTerrainManager()->GetSurfaceTypeCount();

    QStringList layers;
    if (pObject)
    {
        pObject->GetTerrainLayers(layers);
    }

    for (int i = 0; i < num; i++)
    {
        CSurfaceType* pType = GetIEditor()->GetTerrainManager()->GetSurfaceTypePtr(i);

        if (!pType)
        {
            continue;
        }

        CVariable<bool>* pBoolVar = new CVariable<bool>();
        pBoolVar->SetName(pType->GetName());
        pBoolVar->AddOnSetCallback(functor(*this, &CVegetationDataBasePage::OnLayerVarChange));
        if (stl::find(layers, pType->GetName()))
        {
            pBoolVar->Set((bool)true);
        }
        else
        {
            pBoolVar->Set((bool)false);
        }

        pTable->AddVariable(pBoolVar);
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::OnLayerVarChange(IVariable* pVar)
{
    if (!pVar)
    {
        return;
    }
    QString layerName = pVar->GetName();
    bool bValue;

    std::vector<CVegetationObject*> objects;
    GetSelectedObjects(objects);

    pVar->Get(bValue);

    if (bValue)
    {
        for (int i = 0; i < objects.size(); i++)
        {
            CVegetationObject* pObject = objects[i];
            QStringList layers;
            pObject->GetTerrainLayers(layers);
            stl::push_back_unique(layers, layerName);
            pObject->SetTerrainLayers(layers);
        }
    }
    else
    {
        for (int i = 0; i < objects.size(); i++)
        {
            CVegetationObject* pObject = objects[i];
            QStringList layers;
            pObject->GetTerrainLayers(layers);
            stl::find_and_erase(layers, layerName);
            pObject->SetTerrainLayers(layers);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::GotoObjectMaterial()
{
    std::vector<CVegetationObject*> objects;
    GetSelectedObjects(objects);
    if (!objects.empty())
    {
        CVegetationObject* pVegObj = objects[0];
        IStatObj* pStatObj = pVegObj->GetObject();
        if (pStatObj)
        {
            GetIEditor()->GetMaterialManager()->GotoMaterial(pStatObj->GetMaterial());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
CVegetationObject* CVegetationDataBasePage::FindObject(REFGUID guid)
{
    CVegetationObject* pObject = 0;
    for (int i = 0; i < m_vegetationMap->GetObjectCount(); i++)
    {
        CVegetationObject* obj = m_vegetationMap->GetObject(i);
        if (obj->GetGUID() == guid)
        {
            return obj;
        }
    }
    return pObject;
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::ClearSelection()
{
    for (int i = 0; i < m_vegetationMap->GetObjectCount(); i++)
    {
        CVegetationObject* obj = m_vegetationMap->GetObject(i);
        if (obj->IsSelected())
        {
            obj->SetSelected(false);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::OnReportSelChange()
{
    m_bSyncSelection = false;
    ClearSelection();

    // Get selected items.
    const QModelIndexList selectedIndices = m_ui->m_wndReport->selectionModel()->selectedRows();
    Q_FOREACH (const QModelIndex &index, selectedIndices)
    {
        CVegetationObject* pObj = index.data(CVegetationDataBaseModel::VegetationObjectRole).value<CVegetationObject*>();
        if (pObj)
        {
            pObj->SetSelected(true);
        }
    }
    OnSelectionChange();
    m_bSyncSelection = true;
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::OnReportClick(const QModelIndex& index)
{
    CVegetationObject* pObject = index.data(CVegetationDataBaseModel::VegetationObjectRole).value<CVegetationObject*>();
    if (pObject)
    {
        switch (index.column())
        {
        case CVegetationDataBaseModel::ColumnVisible:
        {
            if (!pObject->IsHidden())
            {
                m_nCommand = VDB_COMMAND_HIDE;
            }
            else
            {
                m_nCommand = VDB_COMMAND_UNHIDE;
            }
            break;
        }
        case CVegetationDataBaseModel::ColumnMaterial:
        {
            if (pObject->GetObject())
            {
                GetIEditor()->GetMaterialManager()->GotoMaterial(pObject->GetObject()->GetMaterial());
            }
            break;
        }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::OnReportRClick(const QPoint& pos)
{
    const QModelIndex index = m_ui->m_wndReport->indexAt(pos);

    QMenu menu;

    QAction* addGroup = menu.addAction(tr("Add Group(s)"));
    connect(addGroup, &QAction::triggered, this, &CVegetationDataBasePage::OnAdd);

    QAction* changeCategory = menu.addAction(tr("Change Category"));
    connect(changeCategory, &QAction::triggered, this, &CVegetationDataBasePage::OnNewCategory);

    menu.addSeparator();

    QAction* clone = menu.addAction(tr("Clone"));
    connect(clone, &QAction::triggered, this, &CVegetationDataBasePage::OnClone);

    QAction* replaceGeometry = menu.addAction(tr("Replace Geometry"));
    connect(replaceGeometry, &QAction::triggered, this, &CVegetationDataBasePage::OnReplace);

    QAction* deleteAction = menu.addAction(tr("Delete"));
    connect(deleteAction, &QAction::triggered, this, &CVegetationDataBasePage::OnRemove);

    menu.addSeparator();

    QAction* hideAll = menu.addAction(tr("Hide All"));
    connect(hideAll, &QAction::triggered, this, &CVegetationDataBasePage::OnHideAll);

    QAction* unhideAll = menu.addAction(tr("Unhide All"));
    connect(unhideAll, &QAction::triggered, this, &CVegetationDataBasePage::OnUnhideAll);

    menu.addSeparator();

    QAction* collapseAllGroups = menu.addAction(tr("Collapse &All Groups"));
    connect(collapseAllGroups, &QAction::triggered, m_ui->m_wndReport, &QTreeView::collapseAll);

    QAction* expandAllGroups = menu.addAction(tr("E&xpand All Groups"));
    connect(expandAllGroups, &QAction::triggered, m_ui->m_wndReport, &QTreeView::expandAll);

    menu.exec(QCursor::pos());
}

bool CVegetationDataBasePage::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseMove && watched == m_ui->m_wndReport->viewport())
    {
        QMouseEvent* ev = static_cast<QMouseEvent*>(event);
        const QModelIndex index = m_ui->m_wndReport->indexAt(ev->pos());
        QRect rect = m_ui->m_wndReport->visualRect(index);
        rect.moveTopLeft(m_ui->m_wndReport->viewport()->mapToGlobal(rect.topLeft()));
        const QRect target = fontMetrics().boundingRect(rect, index.data(Qt::TextAlignmentRole).toInt(), index.data().toString());

        if (index.column() == CVegetationDataBaseModel::ColumnMaterial && target.contains(QCursor::pos()))
        {
            m_ui->m_wndReport->viewport()->setCursor(Qt::PointingHandCursor);
        }
        else
        {
            m_ui->m_wndReport->viewport()->setCursor(Qt::ArrowCursor);
        }
    }
    return CDataBaseDialogPage::eventFilter(watched, event);
}

//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::DoIdleUpdate()
{
    m_vegetationMap = GetIEditor()->GetVegetationMap();

    int nRecords = m_vegetationDataBaseModel->rowCount();
    if (nRecords != m_vegetationMap->GetObjectCount())
    {
        ReloadObjects();
        return;
    }

    QItemSelectionModel* selModel = m_ui->m_wndReport->selectionModel();
    bool bReloadObjects = false;
    for (int i = 0; i < m_vegetationDataBaseModel->rowCount(); i++)
    {
        const QModelIndex sourceIndex = m_vegetationDataBaseModel->index(i, 0);
        CVegetationObject* pObj = m_vegetationDataBaseModel->data(sourceIndex, CVegetationDataBaseModel::VegetationObjectRole).value<CVegetationObject*>();
        if (!pObj)
        {
            bReloadObjects = true;
            break;
        }

        const QModelIndex index = m_ui->m_wndReport->mapFromSource(sourceIndex);

        // Select unselect rows.
        if (pObj->IsSelected() != (selModel->isSelected(index) == true))
        {
            if (pObj->IsSelected())
            {
                selModel->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
            }
            else
            {
                selModel->select(index, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
            }
        }
    }

    if (m_nCommand == VDB_COMMAND_HIDE)
    {
        OnHide();
    }
    if (m_nCommand == VDB_COMMAND_UNHIDE)
    {
        OnUnhide();
    }
    m_nCommand = 0;

    //////////////////////////////////////////////////////////////////////////
}


//////////////////////////////////////////////////////////////////////////
void CVegetationDataBasePage::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginLoad:
    case eNotify_OnBeginNewScene:
        // Vegetation objects are about to be deleted
        m_vegetationDataBaseModel->setVegetationObjects({});
        break;
    case eNotify_OnEndSceneOpen:
        ReloadObjects();
        break;
    case eNotify_OnIdleUpdate:
    {
        // If we are active.
        DoIdleUpdate();
    }
    break;

    case eNotify_OnVegetationObjectSelection:
        if (m_bSyncSelection)
        {
            SyncSelection();
        }
        break;
    }
}

#include <VegetationDataBasePage.moc>
