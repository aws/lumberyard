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
#include "PanelDisplayLayer.h"

#include <ISourceControl.h>
#include "ObjectLayerPropsDialog.h"
#include "Objects/ObjectLayerManager.h"
#include "Controls/LayersListBox.h"
#include "QtViewPaneManager.h"
#include <ui_PanelDisplayLayer.h>
#include "CryEditDoc.h"
#include <QToolButton>
#include <QtUtil.h>
#include <QtUtilWin.h>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QMenu>
#include <QKeyEvent>


#define ITEM_COLOR_NORMAL QColor(0, 0, 0)
#define ITEM_COLOR_SELECTED QColor(255, 0, 0)
#define ITEM_COLOR_FROZEN QColor(196, 196, 196)


enum
{
    ID_LAYERS_SETTINGS = 0,
    ID_LAYERS_DELETE,
    ID_LAYERS_EXPORT,
    ID_LAYERS_RELOAD,
    ID_LAYERS_SEPARATOR,
    ID_LAYERS_ADD,
    ID_LAYERS_IMPORT,
    ID_LAYERS_SEPARATOR2,
    ID_LAYERS_LOADVISPRESET,
    ID_LAYERS_SAVEVISPRESET,
    ID_LAYERS_SEPARATOR3,
    ID_LAYERS_EXPAND,
    ID_LAYERS_COLLAPSE,
};

//////////////////////////////////////////////////////////////////////////
// CPanelDisplayLayer dialog
//
//////////////////////////////////////////////////////////////////////////


#include <QtUtilWin.h>
//////////////////////////////////////////////////////////////////////////
void CPanelDisplayLayer::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions options;
    options.canHaveMultipleInstances = true;
    options.sendViewPaneNameBackToAmazonAnalyticsServers = true;
    options.isLegacy = true;
    AzToolsFramework::RegisterViewPane<CPanelDisplayLayer>(LyViewPane::LegacyLayerEditor, LyViewPane::CategoryTools, options);
    GetIEditor()->GetSettingsManager()->AddToolVersion(LyViewPane::LegacyLayerEditor, LAYER_EDITOR_VER);
}

const GUID& CPanelDisplayLayer::GetClassID()
{
    // {A55E3527-FB72-4af4-B278-12002374DAFB}
    static const GUID guid = {
        0xa55e3527, 0xfb72, 0x4af4, { 0xb2, 0x78, 0x12, 0x0, 0x23, 0x74, 0xda, 0xfb }
    };
    return guid;
}


CPanelDisplayLayer::CPanelDisplayLayer(QWidget* pParent /*=NULL*/)
    : QWidget(pParent)
    , m_bLayersValid(false)
    , m_nPrevHeight(0)
    , ui(new Ui::PanelDisplayLayer)
{
    ui->setupUi(this);

    connect(ui->visibleStateButton, &QToolButton::clicked, this, &CPanelDisplayLayer::OnVisibleButtonClicked);
    connect(ui->usableStateButton, &QToolButton::clicked, this, &CPanelDisplayLayer::OnUsableButtonClicked);

    connect(ui->newButton, &QAbstractButton::clicked, this, &CPanelDisplayLayer::OnBnClickedNew);
    connect(ui->renameButton, &QAbstractButton::clicked, this, &CPanelDisplayLayer::OnBnClickedRename);
    connect(ui->deleteButton, &QAbstractButton::clicked, this, &CPanelDisplayLayer::OnBnClickedDelete);
    connect(ui->exportButton, &QAbstractButton::clicked, this, &CPanelDisplayLayer::OnBnClickedExport);
    connect(ui->importButton, &QAbstractButton::clicked, this, &CPanelDisplayLayer::OnBnClickedImport);
    connect(ui->saveButton, &QAbstractButton::clicked, this, &CPanelDisplayLayer::OnBnClickedSaveExternalLayers);
    connect(ui->freezeButton, &QAbstractButton::clicked, this, &CPanelDisplayLayer::OnBnClickedFreezeROnly);

    connect(ui->m_layersCtrl->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CPanelDisplayLayer::OnSelChanged);
    connect(ui->m_layersCtrl, &CLayersListBox::contextMenuRequested, this, &CPanelDisplayLayer::OnShowFullMenu);
    m_pLayerManager = GetIEditor()->GetObjectManager()->GetLayersManager();

    // Register callback.
    m_pLayerManager->AddUpdateListener(functor(*this, &CPanelDisplayLayer::OnLayerUpdate));

    ReloadLayers();
}

CPanelDisplayLayer::~CPanelDisplayLayer()
{
    m_pLayerManager->RemoveUpdateListener(functor(*this, &CPanelDisplayLayer::OnLayerUpdate));
}

//////////////////////////////////////////////////////////////////////////
void CPanelDisplayLayer::OnLayerUpdate(int event, CObjectLayer* pLayer)
{
    switch (event)
    {
    case CObjectLayerManager::ON_LAYER_SELECT:
        SelectLayer(pLayer);
        break;
    case CObjectLayerManager::ON_LAYER_MODIFY:
        UpdateLayerItem(pLayer);
        break;
    default:
        m_bLayersValid = false;
        ReloadLayers();
    }

    UpdateStates();
}

//////////////////////////////////////////////////////////////////////////
void CPanelDisplayLayer::UpdateLayerItem(CObjectLayer* pLayer)
{
    ui->m_layersCtrl->ReloadLayers();
}

//////////////////////////////////////////////////////////////////////////
void CPanelDisplayLayer::ReloadLayers()
{
    ui->m_layersCtrl->ReloadLayers();
}

//////////////////////////////////////////////////////////////////////////
void CPanelDisplayLayer::SelectLayer(CObjectLayer* pLayer)
{
    ui->m_layersCtrl->SelectLayer(pLayer->GetName());
}

//////////////////////////////////////////////////////////////////////////
void CPanelDisplayLayer::OnSelChanged()
{
    CObjectLayer* pLayer = ui->m_layersCtrl->GetCurrentLayer();
    if (pLayer)
    {
        CUndo undo("Set Current Layer");
        m_pLayerManager->SetCurrentLayer(pLayer);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPanelDisplayLayer::OnBnClickedNew()
{
    if (!GetIEditor()->GetDocument()->IsDocumentReady())
    {
        return;
    }

    // Create a new layer.
    CObjectLayerPropsDialog dlg;
    if (dlg.exec() == QDialog::Accepted)
    {
        if (dlg.m_name.isEmpty())
        {
            return;
        }
        // Check if layer with such name already exists.
        if (m_pLayerManager->FindLayerByName(dlg.m_name))
        {
            QMessageBox::warning(this, tr("Layer Exists!"), tr("Layer %1 already exist, choose different layer name").arg(dlg.m_name));
            return;
        }
        CUndo undo("New Layer");
        CObjectLayer* pLayer = m_pLayerManager->CreateLayer();
        pLayer->SetName(dlg.m_name);
        pLayer->SetExternal(dlg.m_bExternal);
        pLayer->SetExportable(dlg.m_bExportToGame);
        pLayer->SetExportLayerPak(dlg.m_bExportLayerPak);
        pLayer->SetDefaultLoaded(dlg.m_bDefaultLoaded);
        pLayer->SetFrozen(dlg.m_bFrozen);
        pLayer->SetVisible(dlg.m_bVisible);
        pLayer->SetColor(dlg.m_color);
        pLayer->SetHavePhysics(dlg.m_bHavePhysics);
        pLayer->SetSpecs(dlg.m_specs);
        pLayer->SetModified();
        ReloadLayers();
    }
}


//////////////////////////////////////////////////////////////////////////
void CPanelDisplayLayer::OnLoadVisPreset()
{
    QString filename = GetIEditor()->GetLevelFolder();
    if (!CFileUtil::SelectFile("Layer Visible Presets (*.lvp)", filename, filename))
    {
        return;
    }

    XmlNodeRef root = XmlHelpers::LoadXmlFromFile(filename.toStdString().c_str());
    if (!root)
    {
        return;
    }

    for (int i = 0; i < root->getChildCount(); ++i)
    {
        XmlNodeRef layer = root->getChild(i);
        if (_stricmp(layer->getTag(), "Layer") != 0)
        {
            continue;
        }

        GUID guid;
        CObjectLayer* pLayer;
        if (layer->getAttr("GUID", guid) && (pLayer = m_pLayerManager->FindLayer(guid)))
        {
            bool isVisible;
            if (layer->getAttr("Visible", isVisible))
            {
                pLayer->SetVisible(isVisible);
            }
            bool isFrozen;
            if (layer->getAttr("Frozen", isFrozen))
            {
                pLayer->SetFrozen(isFrozen);
            }
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CPanelDisplayLayer::OnSaveVisPreset()
{
    QString filename = QStringLiteral("visibilty.lvp");
    if (!CFileUtil::SelectSaveFile("Layer Visible Presets (*.lvp)", "lvp", GetIEditor()->GetLevelFolder(), filename))
    {
        return;
    }

    QWaitCursor wait;

    std::vector<CObjectLayer*> layers;
    GetIEditor()->GetObjectManager()->GetLayersManager()->GetLayers(layers);

    XmlNodeRef root = XmlHelpers::CreateXmlNode("Visibility");

    for (int i = 0; i < layers.size(); ++i)
    {
        const CObjectLayer* pLayer = layers[i];
        XmlNodeRef layer = root->newChild("Layer");
        layer->setAttr("Name", pLayer->GetName().toLatin1().data());
        layer->setAttr("GUID", pLayer->GetGUID());
        layer->setAttr("Visible", pLayer->IsVisible());
        layer->setAttr("Frozen", pLayer->IsFrozen());
    }

    XmlHelpers::SaveXmlNode(GetIEditor()->GetFileUtil(), root, filename.toStdString().c_str());
}


//////////////////////////////////////////////////////////////////////////
void CPanelDisplayLayer::OnBnClickedDelete()
{
    CObjectLayer* pLayer = m_pLayerManager->GetCurrentLayer();
    if (!pLayer)
    {
        return;
    }

    const int numObjects = pLayer->GetObjectCount();
    QString str;
    str = tr("Delete layer %1 with %2 object%3").arg(pLayer->GetName()).arg(numObjects).arg(numObjects == 1 ? "" : "s");
    if (pLayer->GetChildCount())
    {
        int numChildLayersObjects = 0;
        for (int i = 0; i < pLayer->GetChildCount(); i++)
        {
            numChildLayersObjects += pLayer->GetChild(i)->GetObjectCount();
        }
        QString strChildren;
        strChildren = tr("\r\nand all children layers with %1 object%2").arg(numChildLayersObjects).arg(numChildLayersObjects == 1 ? "" : "s");
        str += strChildren;
    }
    str += "?";

    if (QMessageBox::question(this, "Delete Layer", str, QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        // Delete selected layer.
        CUndo undo("Delete Layer");
        m_pLayerManager->DeleteLayer(pLayer);
        ReloadLayers();
    }
}

//////////////////////////////////////////////////////////////////////////
void CPanelDisplayLayer::OnBnClickedExport()
{
    // Export selected layer.
    CObjectLayer* pLayer = m_pLayerManager->GetCurrentLayer();
    if (!pLayer)
    {
        return;
    }

    QString directory = QString("%1/Layers/").arg(GetIEditor()->GetLevelFolder());
    QString filename = QString("%1.lyr").arg(Path::CaselessPaths(pLayer->GetName()));
    if (pLayer->IsExternal())
    {
        filename = pLayer->GetExternalLayerPath();
    }

    if (CFileUtil::SelectSaveFile("Object Layer (*.lyr)", "lyr", directory, filename))
    {
        QApplication::setOverrideCursor(Qt::WaitCursor);

        GetIEditor()->Notify(eNotify_OnBeginLayerExport);

        XmlNodeRef root = XmlHelpers::CreateXmlNode("ObjectLayer");

        XmlNodeRef layerDesc = root->newChild("Layer");
        CObjectArchive ar(GetIEditor()->GetObjectManager(), layerDesc, FALSE);
        GetIEditor()->GetObjectManager()->GetLayersManager()->ExportLayer(ar, pLayer, true);
        XmlHelpers::SaveXmlNode(GetIEditor()->GetFileUtil(), root, filename.toStdString().c_str());

        GetIEditor()->Notify(eNotify_OnEndLayerExport);
        QApplication::restoreOverrideCursor();
    }
}

//////////////////////////////////////////////////////////////////////////
void CPanelDisplayLayer::OnBnClickedReload()
{
    CObjectLayer* pLayer = m_pLayerManager->GetCurrentLayer();
    if (!pLayer)
    {
        return;
    }

    if (pLayer->IsModified())
    {
        QString msg = tr("Layer %1 has been modified. All changes in this layer and related objects will be lost.\r\nReload anyway?").arg(pLayer->GetName());
        if (QMessageBox::question(this, "Reload Layer", "Confirm Reload Layer", QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        {
            return;
        }
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    GetIEditor()->GetObjectManager()->GetLayersManager()->ReloadLayer(pLayer);
    QApplication::restoreOverrideCursor();
}

//////////////////////////////////////////////////////////////////////////
void CPanelDisplayLayer::OnBnClickedImport()
{
    if (!GetIEditor()->GetDocument()->IsDocumentReady())
    {
        return;
    }

    QStringList files;
    if (CFileUtil::SelectFiles("Object Layer (*.lyr);;All Files (*)", QString("%1/Layers/").arg(GetIEditor()->GetLevelFolder()), files))
    {
        QApplication::setOverrideCursor(Qt::WaitCursor);

        // Start recording errors.
        CErrorsRecorder errorsRecorder(GetIEditor());

        CUndo undo("Import Layer(s)");

        GetIEditor()->Notify(eNotify_OnLayerImportBegin);

        for (QStringList::iterator pFile = files.begin(); pFile != files.end(); ++pFile)
        {
            GetIEditor()->GetObjectManager()->GetLayersManager()->ImportLayerFromFile(pFile->toStdString().c_str());
        }

        GetIEditor()->Notify(eNotify_OnLayerImportEnd);
        QApplication::restoreOverrideCursor();
    }
}


//////////////////////////////////////////////////////////////////////////
void CPanelDisplayLayer::OnBnClickedSaveExternalLayers()
{
    m_pLayerManager->SaveExternalLayers();
}


//////////////////////////////////////////////////////////////////////////
void CPanelDisplayLayer::OnBnClickedFreezeROnly()
{
    GetIEditor()->GetObjectManager()->GetLayersManager()->FreezeROnly();
}


//////////////////////////////////////////////////////////////////////////
void CPanelDisplayLayer::OnBnClickedRename()
{
    // Export selected layer.
    CObjectLayer* pLayer = m_pLayerManager->GetCurrentLayer();
    if (!pLayer)
    {
        return;
    }

    CObjectLayerPropsDialog dlg;
    dlg.m_name = pLayer->GetName();
    dlg.m_bExportToGame = pLayer->IsExportable();
    dlg.m_bExportLayerPak = pLayer->IsExporLayerPak();
    dlg.m_bDefaultLoaded = pLayer->IsDefaultLoaded();
    dlg.m_bExternal = pLayer->IsExternal();
    dlg.m_bFrozen = pLayer->IsFrozen();
    dlg.m_bVisible = pLayer->IsVisible();
    dlg.m_bHavePhysics = pLayer->IsPhysics();
    dlg.m_specs = pLayer->GetSpecs();
    dlg.m_bMainLayer = false;
    dlg.m_color = pLayer->GetColor();
    if (dlg.exec() == QDialog::Accepted)
    {
        if (dlg.m_name.isEmpty())
        {
            return;
        }

        CUndo undo("Rename Layer");

        CObjectLayer* pOtherLayer = m_pLayerManager->FindLayerByName(dlg.m_name);
        // Check if layer with such name already exists.
        if (pOtherLayer && pOtherLayer != pLayer)
        {
            QMessageBox::warning(this, tr("Layer Exists"), tr("Layer %1 already exist, choose different layer name").arg(dlg.m_name));
            return;
        }

        pLayer->SetName(dlg.m_name, true);
        pLayer->SetExternal(dlg.m_bExternal);
        pLayer->SetExportable(dlg.m_bExportToGame);
        pLayer->SetExportLayerPak(dlg.m_bExportLayerPak);
        pLayer->SetDefaultLoaded(dlg.m_bDefaultLoaded);
        pLayer->SetFrozen(dlg.m_bFrozen);
        pLayer->SetVisible(dlg.m_bVisible);
        pLayer->SetColor(dlg.m_color);
        pLayer->SetHavePhysics(dlg.m_bHavePhysics);
        pLayer->SetSpecs(dlg.m_specs);
        pLayer->SetModified();

        GetIEditor()->GetObjectManager()->GetLayersManager()->NotifyLayerChange(pLayer);
        GetIEditor()->Notify(eNotify_OnInvalidateControls);
        ReloadLayers();
    }
}


//////////////////////////////////////////////////////////////////////////
void CPanelDisplayLayer::OnShowFullMenu()
{
    // Assume that layer was already selected.
    QStringList eMItems;
    eMItems.append(tr("Settings"));
    eMItems.append(tr("Delete"));
    eMItems.append(tr("Export"));
    eMItems.append(tr("Reload"));
    eMItems.append("");

    ShowContextMenu(eMItems);
}

//////////////////////////////////////////////////////////////////////////
void CPanelDisplayLayer::ShowContextMenu(const QStringList& extraActions)
{
    if (!GetIEditor()->GetDocument()->IsDocumentReady())
    {
        return;
    }

    CObjectLayer* pLayer = 0;

    int firstMenuID = ID_LAYERS_ADD;
    QStringList menuItems = extraActions;
    if (!extraActions.isEmpty())
    {
        firstMenuID = ID_LAYERS_SETTINGS;
    }

    if (m_pLayerManager->GetCurrentLayer() && m_pLayerManager->GetCurrentLayer()->IsExternal())
    {
        pLayer = m_pLayerManager->GetCurrentLayer();
    }

    menuItems.append(tr("Add New Layer"));
    menuItems.append(tr("Import Layers"));

    menuItems.append("");

    menuItems.append(tr("Load Visibility Preset"));
    menuItems.append(tr("Save Visibility Preset"));

    menuItems.append("");

    menuItems.append(tr("Expand All Layers"));
    menuItems.append(tr("Collapse All Layers"));

    int cmd = -1;
    if (pLayer)
    {
        QString file = pLayer->GetExternalLayerPath();
        uint32 attr = CFileUtil::GetAttributes(file.toLatin1().data());
        const QString selectedItem = CFileUtil::PopupQMenu(Path::GetFile(file), Path::GetPath(file), this, 0, menuItems);

        //find index of menu item that was selected.
        //If not found, it was something from menu in CFileUtil::PopupQMenu and was handled there.
        cmd = firstMenuID + menuItems.indexOf(selectedItem);

        uint32 newAttr = CFileUtil::GetAttributes(file.toLatin1().data());
        if (attr != SCC_FILE_ATTRIBUTE_INVALID && (attr & SCC_FILE_ATTRIBUTE_READONLY) && newAttr != SCC_FILE_ATTRIBUTE_INVALID && !(newAttr & SCC_FILE_ATTRIBUTE_READONLY))
        {
            pLayer->SetFrozen(false);
        }
    }
    else
    {
        QMenu menu;
        for (int i = 0; i < menuItems.size(); i++)
        {
            const QString item = menuItems.at(i);
            if (!item.isEmpty())
            {
                QAction* action = menu.addAction(item);
                action->setData(i);
            }
            else
            {
                menu.addSeparator();
            }
        }
        QAction* action = menu.exec(QCursor::pos());
        cmd = action ? action->data().toInt() : -1;
    }

    if (cmd != -1)
    {
        switch (cmd)
        {
        case ID_LAYERS_ADD:
            OnBnClickedNew();
            break;
        case ID_LAYERS_LOADVISPRESET:
            OnLoadVisPreset();
            break;
        case ID_LAYERS_SAVEVISPRESET:
            OnSaveVisPreset();
            break;
        case ID_LAYERS_EXPAND:
            ui->m_layersCtrl->ExpandAll(true);
            break;
        case ID_LAYERS_COLLAPSE:
            ui->m_layersCtrl->ExpandAll(false);
            break;
        case ID_LAYERS_SETTINGS:
            OnBnClickedRename();
            break;
        case ID_LAYERS_EXPORT:
            OnBnClickedExport();
            break;
        case ID_LAYERS_RELOAD:
            OnBnClickedReload();
            break;
        case ID_LAYERS_IMPORT:
            OnBnClickedImport();
            break;
        case ID_LAYERS_DELETE:
            OnBnClickedDelete();
            break;
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CPanelDisplayLayer::OnVisibleButtonClicked(bool bForceVisibleState)
{
    std::vector<CObjectLayer*> layers;
    m_pLayerManager->GetLayers(layers);
    for (int i = 0; i < layers.size(); i++)
    {
        layers[i]->SetVisible(bForceVisibleState);
    }
}

void CPanelDisplayLayer::OnUsableButtonClicked(bool bForceUsableState)
{
    std::vector<CObjectLayer*> layers;
    m_pLayerManager->GetLayers(layers);
    for (int i = 0; i < layers.size(); i++)
    {
        layers[i]->SetFrozen(!bForceUsableState);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPanelDisplayLayer::contextMenuEvent(QContextMenuEvent* event)
{
    if (!ui->m_layersCtrl->geometry().contains(event->pos()))
    {
        ShowContextMenu();
    }
}

//////////////////////////////////////////////////////////////////////////
void CPanelDisplayLayer::keyPressEvent(QKeyEvent* e)
{
    switch (e->key())
    {
    case Qt::Key_F2:
        OnBnClickedRename();
        break;
    }
    ;

    return QWidget::keyPressEvent(e);
}
//////////////////////////////////////////////////////////////////////////
void CPanelDisplayLayer::UpdateStates()
{
    if (ui->m_layersCtrl->GetCount() < 1)
    {
        return;
    }

    bool bVisibleState = true;
    bool bUsableState = true;

    std::vector<CObjectLayer*> layers;
    m_pLayerManager->GetLayers(layers);

    for (int i = 0; i < layers.size(); i++)
    {
        if (!layers[i]->IsVisible())
        {
            bVisibleState = false;
        }

        if (layers[i]->IsFrozen())
        {
            bUsableState = false;
        }
    }

    ui->visibleStateButton->setChecked(bVisibleState);
    ui->usableStateButton->setChecked(bUsableState);
}
