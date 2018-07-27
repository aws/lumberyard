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
#include "ObjectLayerManager.h"
#include "ObjectLoader.h"
#include "GameEngine.h"
#include "ErrorReport.h"
#include "CheckOutDialog.h"

#include <IGameFramework.h>
#include <ISourceControl.h>
#include "EntityObject.h"
#include "SplineObject.h"
#include "HyperGraph/FlowGraphManager.h"
#include "HyperGraph/FlowGraph.h"
#include "HyperGraph/FlowGraphNode.h"

#include "Util/BoostPythonHelpers.h"
#include "Material/Material.h"
#include <StringUtils.h>
#include "Util/FileUtil.h"
#include "RoadObject.h"

#include <Editor/StringDlg.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

//////////////////////////////////////////////////////////////////////////
//! Undo Delete Layer
class CUndoLayerDelete
    : public IUndoObject
{
public:
    CUndoLayerDelete(CObjectLayer* pLayer)
    {
        m_pLayer = pLayer;
        m_undo = XmlHelpers::CreateXmlNode("Undo");
        pLayer->SerializeBase(m_undo, false);
        pLayer->Serialize(m_undo, false);
    }
protected:
    virtual int GetSize() { return sizeof(*this); }; // Return size of xml state.
    virtual QString GetDescription() { return "Delete Layer"; };

    virtual void Undo(bool bUndo)
    {
        TSmartPtr<CObjectLayer> pLayer = new CObjectLayer();
        pLayer->SerializeBase(m_undo, true);
        pLayer->Serialize(m_undo, true);

        if (!GetIEditor()->GetObjectManager()->GetLayersManager()->FindLayer(pLayer->GetGUID()))
        {
            GetIEditor()->GetObjectManager()->GetLayersManager()->AddLayer(pLayer);
        }

        GetIEditor()->GetObjectManager()->GetLayersManager()->ResolveLayerLinks();
    }

    virtual void Redo()
    {
        GetIEditor()->GetObjectManager()->GetLayersManager()->DeleteLayer(m_pLayer);
    }

private:
    TSmartPtr<CObjectLayer> m_pLayer;
    XmlNodeRef m_undo;
};



//////////////////////////////////////////////////////////////////////////
// class CUndoSelectLayer
class CUndoCurrentLayer
    : public IUndoObject
{
public:
    CUndoCurrentLayer(CObjectLayer* pLayer)
    {
        m_undoName = pLayer->GetName();
    }
protected:
    virtual int GetSize() { return sizeof(*this); };
    virtual QString GetDescription() { return "Select Layer"; };

    virtual void Undo(bool bUndo)
    {
        CObjectLayerManager* pLayerMan = GetIEditor()->GetObjectManager()->GetLayersManager();
        if (bUndo)
        {
            CObjectLayer* pLayer = pLayerMan->GetCurrentLayer();
            if (pLayer)
            {
                m_redoName = pLayer->GetName();
            }
        }

        CObjectLayer* pLayer = pLayerMan->FindLayerByName(m_undoName);
        if (pLayer)
        {
            pLayerMan->SetCurrentLayer(pLayer);
        }
    }

    virtual void Redo()
    {
        CObjectLayerManager* pLayerMan = GetIEditor()->GetObjectManager()->GetLayersManager();
        CObjectLayer* pLayer = pLayerMan->FindLayerByName(m_redoName);
        if (pLayer)
        {
            pLayerMan->SetCurrentLayer(pLayer);
        }
    }

private:
    QString m_undoName;
    QString m_redoName;
};



//////////////////////////////////////////////////////////////////////////
// class CObjectLayerManager


//////////////////////////////////////////////////////////////////////////
CObjectLayerManager::CObjectLayerManager(CObjectManager* pObjectManager)
    : m_pObjectManager(pObjectManager)
    , m_layersPath(LAYER_PATH)
    , m_bLevelLoading(false)
    , m_pMainLayer(0)
    , m_bOverwriteDuplicates(false)
    , m_visibleSetLayer(GuidUtil::NullGuid)
{
    GetIEditor()->RegisterNotifyListener(this);
}

CObjectLayerManager::~CObjectLayerManager()
{
    GetIEditor()->UnregisterNotifyListener(this);
}

void CObjectLayerManager::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginSceneOpen:
        m_bLevelLoading = true;
        break;
    case eNotify_OnEndSceneOpen:
        m_bLevelLoading = false;
        break;
    case eNotify_OnLayerImportBegin:
        m_bLevelLoading = true;
        break;
    case eNotify_OnLayerImportEnd:
        m_bLevelLoading = false;
        break;
    }
}

bool CObjectLayerManager::CanModifyLayers()
{
    return !m_bLevelLoading; // if we are level loading we cannot set layers to modified
}

void CObjectLayerManager::GetLayers(std::vector<CObjectLayer*>& layers) const
{
    layers.clear();
    layers.reserve(m_layersMap.size());
    for (LayersMap::const_iterator it = m_layersMap.begin(); it != m_layersMap.end(); ++it)
    {
        layers.push_back(it->second);
    }
}

void CObjectLayerManager::CreateMainLayer(const GUID* pGUID)
{
    if (m_pMainLayer)
    {
        return;
    }
    m_pMainLayer = new CObjectLayer();
    m_layersMap[m_pMainLayer->GetGUID()] = m_pMainLayer;
    m_pMainLayer->SetExternal(false);
    m_pMainLayer->SetName("Main");
    m_pMainLayer->Expand(true);
    m_pCurrentLayer = m_pMainLayer;
    NotifyListeners(ON_LAYER_UPDATEALL, NULL);
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayerManager::ClearLayers()
{
    // Erase all removable layers.
    LayersMap::iterator it, itnext;
    for (LayersMap::iterator it = m_layersMap.begin(); it != m_layersMap.end(); it = itnext)
    {
        CObjectLayer* pLayer = it->second;
        itnext = it;
        itnext++;

        NotifyListeners(ON_LAYER_REMOVE, pLayer);
        m_layersMap.erase(it);
    }

    m_pMainLayer = 0;

    m_pCurrentLayer = 0;
    if (!m_layersMap.empty())
    {
        SetCurrentLayer(m_layersMap.begin()->second);
    }
    NotifyListeners(ON_LAYER_UPDATEALL, NULL);
}

//////////////////////////////////////////////////////////////////////////
CObjectLayer* CObjectLayerManager::CreateLayer(const GUID* pGUID)
{
    if (pGUID)
    {
        CObjectLayer* pLayer = FindLayer(*pGUID);
        if (pLayer)
        {
            return pLayer;
        }
    }

    CObjectLayer* pLayer = new CObjectLayer(pGUID);
    m_layersMap[pLayer->GetGUID()] = pLayer;
    NotifyListeners(ON_LAYER_ADD, pLayer);
    return pLayer;
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayerManager::AddLayer(CObjectLayer* pLayer)
{
    assert(pLayer);

    if (!m_bOverwriteDuplicates)
    {
        CObjectLayer* pPrevLayer = FindLayerByName(pLayer->GetName());
        if (pPrevLayer)
        {
            CErrorRecord err;
            err.error = QObject::tr("Duplicate Layer Name <%1>").arg(pLayer->GetName());
            err.severity = CErrorRecord::ESEVERITY_ERROR;
            GetIEditor()->GetErrorReport()->ReportError(err);
            return;
        }

        if (m_layersMap.find(pLayer->GetGUID()) != m_layersMap.end())
        {
            pPrevLayer = FindLayer(pLayer->GetGUID());

            CErrorRecord err;
            err.error = QObject::tr("Duplicate Layer GUID,Layer <%1> collides with layer <%2>").
                arg(pPrevLayer->GetName()).arg(pLayer->GetName());
            err.severity = CErrorRecord::ESEVERITY_ERROR;
            GetIEditor()->GetErrorReport()->ReportError(err);
            return;
        }
    }

    m_layersMap[pLayer->GetGUID()] = pLayer;
    NotifyListeners(ON_LAYER_ADD, pLayer);
}

bool CObjectLayerManager::CanDeleteLayer(CObjectLayer* pLayer)
{
    assert(pLayer);

    if (pLayer->GetParent())
    {
        return true;
    }

    // one another root layer must exist.
    for (auto it = m_layersMap.begin(); it != m_layersMap.end(); ++it)
    {
        CObjectLayer* pCmpLayer = it->second;
        if (pCmpLayer != pLayer && !pCmpLayer->GetParent())
        {
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayerManager::DeleteLayer(CObjectLayer* pLayer)
{
    assert(pLayer);

    if (!CanDeleteLayer(pLayer))
    {
        return;
    }

    if (pLayer == m_pMainLayer)
    {
        m_pMainLayer = 0;
    }

    // First delete all child layers.  Start at the end of the list so that if CanDeleteLayer() above returns or it fails for another reason we are not in infinite loop
    for (int i = pLayer->GetChildCount() - 1; i >= 0 ; --i)
    {
        DeleteLayer(pLayer->GetChild(i));
    }

    // prevent reference counted layer to be released before this function ends.
    TSmartPtr<CObjectLayer> pLayerHolder = pLayer;

    // Delete all objects for this layer.
    std::vector<CBaseObjectPtr> objects;
    m_pObjectManager->GetAllObjects(objects);
    for (int k = 0; k < objects.size(); k++)
    {
        // Skip objects part of prefabs
        if (objects[k]->CheckFlags(OBJFLAG_PREFAB))
        {
            continue;
        }

        if (objects[k]->GetLayer() == pLayer)
        {
            m_pObjectManager->DeleteObject(objects[k]);
        }
    }

    if (GetIEditor()->IsUndoRecording())
    {
        GetIEditor()->RecordUndo(new CUndoLayerDelete(pLayer));
    }

    // remove child from parent after serialization for storing Parent GUID
    CObjectLayer* pParent = pLayer->GetParent();
    if (pParent)
    {
        pParent->RemoveChild(pLayer);
    }

    m_layersMap.erase(pLayer->GetGUID());

    if (pParent)
    {
        SetCurrentLayer(pParent);
    }
    else
    {
        assert(!m_layersMap.empty());
        SetCurrentLayer(m_layersMap.begin()->second);
    }

    NotifyListeners(ON_LAYER_REMOVE, pLayer);
    GetIEditor()->GetFlowGraphManager()->SendNotifyEvent(EHG_GRAPH_INVALIDATE);
}

//////////////////////////////////////////////////////////////////////////
CObjectLayer* CObjectLayerManager::FindLayer(REFGUID guid) const
{
    CObjectLayer* pLayer = stl::find_in_map(m_layersMap, guid, (CObjectLayer*)0);
    return pLayer;
}

//////////////////////////////////////////////////////////////////////////
CObjectLayer* CObjectLayerManager::FindLayerByName(const QString& layerName) const
{
    for (LayersMap::const_iterator it = m_layersMap.begin(); it != m_layersMap.end(); ++it)
    {
        if (QString::compare(it->second->GetName(), layerName, Qt::CaseInsensitive) == 0)
        {
            return it->second;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayerManager::NotifyLayerChange(CObjectLayer* pLayer)
{
    assert(pLayer);
    // check if this layers is already registered in manager.
    if (m_layersMap.find(pLayer->GetGUID()) != m_layersMap.end())
    {
        // Notify listeners of this change.
        NotifyListeners(ON_LAYER_MODIFY, pLayer);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayerManager::AddUpdateListener(const LayersUpdateCallback& cb)
{
    m_listeners.Add(cb);
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayerManager::RemoveUpdateListener(const LayersUpdateCallback& cb)
{
    m_listeners.Remove(cb);
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayerManager::NotifyListeners(EUpdateType type, CObjectLayer* pLayer)
{
    m_listeners.Call(type, pLayer);
}

//////////////////////////////////////////////////////////////////////////
bool CObjectLayerManager::PromptForLayerName(QString& layerName) const
{
    StringDlg dlg(QObject::tr("New Layer Name"));
    dlg.SetString(layerName);

    // Give the user multiple attempts in order to find a name that can be accepted, without losing their current input.
    // this will loop and keep trying until the checks pass, or they hit ESC or CANCEL
    int dlgResult = dlg.exec();
    while (dlgResult == QDialog::Accepted)
    {
        if (dlg.GetString().isEmpty())
        {
            QMessageBox::critical(AzToolsFramework::GetActiveWindow(), QObject::tr("Invalid name"), QObject::tr("Please pick a new name for the imported layer"));
            dlgResult = dlg.exec();
            continue;
        }

        QString fullName = dlg.GetString();
        if (FindLayerByName(fullName))
        {
            QString message = QObject::tr("Item with name %1 already exists").arg(fullName);
            QMessageBox::critical(AzToolsFramework::GetActiveWindow(), QObject::tr("Invalid name"), message);
            dlgResult = dlg.exec();
            continue;
        }

        break;
    }

    if (dlgResult != IDOK)
    {
        return false;
    }

    layerName = dlg.GetString();
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayerManager::Serialize(CObjectArchive& ar)
{
    XmlNodeRef xmlNode = ar.node;
    if (ar.bLoading)
    {
        ClearLayers();
        // Loading from archive.

        XmlNodeRef layers = xmlNode->findChild("ObjectLayers");
        if (layers)
        {
            int nVers = 0;
            layers->getAttr("LayerVersion", nVers);
            if (nVers < 2)
            {
                CreateMainLayer();
            }

            for (int i = 0; i < layers->getChildCount(); i++)
            {
                XmlNodeRef layerNode = layers->getChild(i);
                TSmartPtr<CObjectLayer> pLayer = new CObjectLayer();
                pLayer->SerializeBase(layerNode, true);
                QString fullName;
                layerNode->getAttr("FullName", fullName);
                if (pLayer->IsExternal())
                {
                    LoadExternalLayer(ar, pLayer, fullName);
                }
                else
                {
                    pLayer->Serialize(layerNode, true);
                    // Check if this layer is already loaded.
                    if (!FindLayer(pLayer->GetGUID()))
                    {
                        AddLayer(pLayer);
                    }
                }
            }

            GUID currentLayerGUID;
            if (layers->getAttr("CurrentGUID", currentLayerGUID))
            {
                CObjectLayer* pLayer = FindLayer(currentLayerGUID);
                if (pLayer)
                {
                    SetCurrentLayer(pLayer);
                }
            }
        }

        if (!m_pCurrentLayer)
        {
            CreateMainLayer();
        }

        ResolveLayerLinks();
    }
    else
    {
        // Saving to archive.
        XmlNodeRef layersNode = xmlNode->newChild("ObjectLayers");
        int nVers = 2;
        layersNode->setAttr("LayerVersion", nVers);

        //! Store currently selected layer.
        if (m_pCurrentLayer)
        {
            layersNode->setAttr("Current", m_pCurrentLayer->GetName().toUtf8().data());
            layersNode->setAttr("CurrentGUID", m_pCurrentLayer->GetGUID());
        }

        CAutoCheckOutDialogEnableForAll enableForAll;

        // keep track of external layer files
        std::set<string> externalLayers;

        CObjectLayerManager* pLayerManager = GetIEditor()->GetObjectManager()->GetLayersManager();
        QString layerPath = Path::AddSlash(Path::AddSlash(GetIEditor()->GetGameEngine()->GetLevelPath()) + pLayerManager->GetLayersPath());

        for (LayersMap::const_iterator it = m_layersMap.begin(); it != m_layersMap.end(); ++it)
        {
            CObjectLayer* pLayer = it->second;

            XmlNodeRef layerNode = layersNode->newChild("Layer");
            pLayer->SerializeBase(layerNode, false);

            if (pLayer->IsExternal())
            {
                if (!gSettings.saveOnlyModified || pLayer->IsModified())
                {
                    // Save external level additionally to file.
                    SaveExternalLayer(&ar, pLayer);
                }
                externalLayers.insert(CryStringUtils::ToLower((layerPath + pLayer->GetFullName() + LAYER_FILE_EXTENSION).toUtf8().data()));
            }
            else
            {
                pLayer->Serialize(layerNode, false);
                pLayer->SetModified(false);
            }
        }

        // recursively find layer files in level's layer folder
        CFileUtil::ForEach(layerPath, [&externalLayers](const QString& filepath)
        {
            // ignore if this is not a layer file
            if (Path::GetExt(filepath).compare(LAYER_FILE_EXTENSION + 1, Qt::CaseInsensitive) != 0)
            {
                return;
            }

            // check if this is a layer we care about
            if (externalLayers.find(CryStringUtils::ToLower(filepath.toUtf8().data())) == externalLayers.end())
            {
                using namespace AzToolsFramework;

                char resolvedPath[AZ_MAX_PATH_LEN] = { 0 };
                AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(filepath.toUtf8().data(), resolvedPath, AZ_MAX_PATH_LEN);
                SourceControlCommandBus::Broadcast(&SourceControlCommandBus::Events::RequestDelete, resolvedPath, [](bool, const AzToolsFramework::SourceControlFileInfo&) {});
            }
        }, true);
    }
    ar.node = xmlNode;
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayerManager::SaveExternalLayers()
{
    bool bContainsModifiedInternalLayers = false;
    uint32 nModifiedExtLayers = 0;
    uint32 nSavedCount = 0;

    for (LayersMap::const_iterator it = m_layersMap.begin(); it != m_layersMap.end(); ++it)
    {
        CObjectLayer* pLayer = it->second;
        if (pLayer->IsModified())
        {
            if (pLayer->IsExternal())
            {
                ++nModifiedExtLayers;
                if (SaveExternalLayer(nullptr, pLayer))
                {
                    ++nSavedCount;
                }
            }
            else
            {
                bContainsModifiedInternalLayers = true;
            }
        }
    }

    if (nModifiedExtLayers == 0)
    {
        CryMessageBox("No modified external layers found", "Information", MB_ICONINFORMATION | MB_OK);
        return;
    }

    bool warning = false;
    AZStd::string message = AZStd::string::format("%u external layer(s) saved", nSavedCount);
    if (nModifiedExtLayers - nSavedCount)
    {
        message.append(AZStd::string::format("\r\n%u external layer(s) still need saving", nModifiedExtLayers - nSavedCount));
        warning = true;
    }

    if (bContainsModifiedInternalLayers)
    {
        message.append("\r\n\r\nSome internal layers still contain unsaved changes");
        warning = true;
    }

    if (warning)
    {
        CryMessageBox(message.c_str(), "Warning", MB_ICONWARNING | MB_OK);
    }
    else
    {
        CryMessageBox(message.c_str(), "Information", MB_ICONINFORMATION | MB_OK);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CObjectLayerManager::SaveExternalLayer(CObjectArchive* pArchive, CObjectLayer* pLayer)
{
    // Form file name from layer name.
    QString path = Path::AddPathSlash(GetIEditor()->GetGameEngine()->GetLevelPath()) + m_layersPath;
    path = Path::GamePathToFullPath(path); 
    QString file = pLayer->GetExternalLayerPath();

    if (CFileUtil::OverwriteFile(file))
    {
        CFileUtil::CreateDirectory(path.toUtf8().data());

        // Serialize this layer.
        XmlNodeRef rootFileNode = XmlHelpers::CreateXmlNode("ObjectLayer");
        XmlNodeRef layerNode = rootFileNode->newChild("Layer");

        CObjectArchive tempArchive(m_pObjectManager, layerNode, FALSE);

        if (pArchive)
        {
            pArchive->node = layerNode;
        }

        ExportLayer((pArchive ? *pArchive : tempArchive), pLayer, false); // Also save all childs but not external layers.
        // Save xml file to disk.
        XmlHelpers::SaveXmlNode(GetIEditor()->GetFileUtil(), rootFileNode, file.toUtf8().data());
        pLayer->SetModified(false);
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayerManager::LoadExternalLayer(CObjectArchive& ar, CObjectLayer* pLayer, const QString& fullName)
{
    assert(pLayer);
    // Form file name from layer name.
    QString file = Path::AddPathSlash(GetIEditor()->GetGameEngine()->GetLevelPath());
    file += m_layersPath + fullName + LAYER_FILE_EXTENSION;

    XmlNodeRef root;
    if (CFileUtil::FileExists(file))
    {
        root = XmlHelpers::LoadXmlFromFile(file.toUtf8().data());
        if (!root)
        {
            CErrorRecord err;
            err.error = QObject::tr("Failed to import external object layer <%1> from File %2").arg(pLayer->GetName(), file);
            err.file = file;
            err.severity = CErrorRecord::ESEVERITY_ERROR;
            ar.ReportError(err);
            return;
        }
    }
    else
    {
        // Backwards compatibility for old structure with all layers in root layer folder
        QString oldFile = Path::AddPathSlash(GetIEditor()->GetGameEngine()->GetLevelPath());
        oldFile += m_layersPath + pLayer->GetName() + LAYER_FILE_EXTENSION;
        root = XmlHelpers::LoadXmlFromFile(oldFile.toUtf8().data());
        if (!root)
        {
            CErrorRecord err;
            err.error = QObject::tr("Failed to import external object layer <%1> from File %2").arg(pLayer->GetName(), oldFile);
            err.file = file;
            err.severity = CErrorRecord::ESEVERITY_ERROR;
            ar.ReportError(err);
            return;
        }
    }

    XmlNodeRef layerDesc = root->findChild("Layer");
    if (layerDesc)
    {
        XmlNodeRef prevRoot = ar.node;
        ar.node = layerDesc;
        m_bOverwriteDuplicates = true;
        ImportLayer(ar, pLayer);
        m_bOverwriteDuplicates = false;
        ar.node = prevRoot;

        uint32 attr = CFileUtil::GetAttributes(file.toUtf8().data());
        if (gSettings.freezeReadOnly && (attr & SCC_FILE_ATTRIBUTE_READONLY))
        {
            pLayer->SetFrozen(true);
        }

        // Backwards compatibility with SDK branch: Load layers recursively.
        XmlNodeRef childLayers = layerDesc->findChild("ChildLayers");
        if (childLayers)
        {
            const int numChildLayers = childLayers->getChildCount();
            for (int i = 0; i < numChildLayers; ++i)
            {
                XmlNodeRef layerNode = childLayers->getChild(i);
                TSmartPtr<CObjectLayer> pLayer = new CObjectLayer();
                pLayer->SerializeBase(layerNode, true);
                QString fullName;
                if (layerNode->getAttr("FullName", fullName))
                {
                    LoadExternalLayer(ar, pLayer, fullName);
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayerManager::ExportLayer(CObjectArchive& ar, CObjectLayer* pLayer, bool bExportExternalChilds)
{
    pLayer->Serialize(ar.node, false);

    XmlNodeRef orgNode = ar.node;

    CBaseObjectsArray objects;
    m_pObjectManager->GetObjects(objects, pLayer);
    size_t numObjects = objects.size();

    if (numObjects)
    {
        XmlNodeRef layerObjects = orgNode->newChild("LayerObjects");
        ar.node = layerObjects;

        for (int i = 0; i < numObjects; ++i)
        {
            ar.SaveObject(objects[i]);
        }
    }

    if (pLayer->GetChildCount() > 0 && bExportExternalChilds)
    {
        XmlNodeRef childLayers = orgNode->newChild("ChildLayers");
        for (int i = 0; i < pLayer->GetChildCount(); i++)
        {
            CObjectLayer* pChildLayer = pLayer->GetChild(i);
            if (pChildLayer->IsExternal() && !bExportExternalChilds)
            {
                continue;
            }

            XmlNodeRef childLayer = childLayers->newChild("Layer");
            ar.node = childLayer;
            ExportLayer(ar, pChildLayer, bExportExternalChilds);
        }
    }

    ar.node = orgNode;
}

//////////////////////////////////////////////////////////////////////////
CObjectLayer* CObjectLayerManager::ImportLayer(CObjectArchive& ar, CObjectLayer* pInitLayer)
{
    XmlNodeRef layerNode = ar.node;
    if (_stricmp(layerNode->getTag(), "Layer") != 0)
    {
        CErrorRecord err;
        err.error = QObject::tr("Not a valid layer XML node <%1>, must be <Layer>").arg(layerNode->getTag());
        err.severity = CErrorRecord::ESEVERITY_WARNING;
        ar.ReportError(err);
        return 0;
    }
    TSmartPtr<CObjectLayer> pLayer = pInitLayer ? pInitLayer : new CObjectLayer();

    if (!pInitLayer)
    {
        pLayer->SerializeBase(layerNode, true);
    }
    pLayer->Serialize(layerNode, true);

    CObjectLayer* pPrevLayer = FindLayer(pLayer->GetGUID());
    if (pPrevLayer)
    {
        if (m_bOverwriteDuplicates)
        {
            pLayer = pPrevLayer;
            pLayer->Serialize(layerNode, true); // Serialize it again.
        }
        else
        {
            // Duplicate layer imported.
            QString str = QObject::tr("Replace Layer %1?\r\nAll object of replaced layer will be deleted.").arg(pPrevLayer->GetName());
            if (QMessageBox::question(QApplication::activeWindow(), QObject::tr("Confirm Replace Layer"), str) == QMessageBox::No)
            {
                return 0;
            }
            DeleteLayer(pPrevLayer);
        }
    }

    pPrevLayer = FindLayerByName(pLayer->GetName());
    if (pPrevLayer && pPrevLayer != pLayer)
    {
        bool nameConflict = true;
        QString conflictMsg = QObject::tr("Layer name '%1' already in use.\r\nWould you like to rename the layer being imported?").arg(pLayer->GetName());
        if (QMessageBox::question(QApplication::activeWindow(), QObject::tr("Duplicate Layer Name"), conflictMsg) == QMessageBox::Yes)
        {
            QString newLayerName;
            if (PromptForLayerName(newLayerName))
            {
                pLayer->SetName(newLayerName);
                pLayer->BuildGuid();
                nameConflict = false;
            }
        }

        if (nameConflict)
        {
            QString message = QObject::tr("Layer Import Canceled (Layer '%1' already exists)").arg(pLayer->GetName());
            QMessageBox::critical(QApplication::activeWindow(), QObject::tr("Invalid name"), message);
            return 0;
        }
    }

    if (pLayer)
    {
        AddLayer(pLayer);
    }

    XmlNodeRef layerObjects = layerNode->findChild("LayerObjects");
    if (layerObjects)
    {
        int numObjects = layerObjects->getChildCount();

        TSmartPtr<CObjectLayer> pCurLayer = m_pCurrentLayer;
        m_pCurrentLayer = pLayer;

        ar.LoadObjects(layerObjects);
        m_pCurrentLayer = pCurLayer;
    }

    XmlNodeRef childLayers = layerNode->findChild("ChildLayers");
    if (childLayers)
    {
        // Import child layers.
        for (int i = 0; i < childLayers->getChildCount(); i++)
        {
            XmlNodeRef childLayer = childLayers->getChild(i);
            ar.node = childLayer;
            CObjectLayer* pChildLayer = ImportLayer(ar, nullptr);
            if (pChildLayer)
            {
                // Import child layers.
                pLayer->AddChild(pChildLayer);
            }
        }
        ar.node = layerNode;
    }

    return pLayer;
}


//////////////////////////////////////////////////////////////////////////
CObjectLayer* CObjectLayerManager::ImportLayerFromFile(const char* filename)
{
    CObjectLayer* pRetLayer = nullptr;
    XmlNodeRef root = XmlHelpers::LoadXmlFromFile(filename);
    if (!root)
    {
        return nullptr;
    }

    CObjectArchive archive(GetIEditor()->GetObjectManager(), root, true);
    int child_count = root->getChildCount();
    for (int i = 0; i < child_count; ++i)
    {
        XmlNodeRef layerDesc = root->getChild(i);
        if (_stricmp(layerDesc->getTag(), "Layer") != 0)
        {
            continue;
        }

        archive.node = layerDesc;
        pRetLayer = ImportLayer(archive, nullptr);
    }
    ResolveLayerLinks();
    archive.ResolveObjects();
    return pRetLayer;
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayerManager::SetCurrentLayer(CObjectLayer* pCurrLayer)
{
    assert(pCurrLayer);
    if (pCurrLayer == m_pCurrentLayer)
    {
        return;
    }

    if (CUndo::IsRecording())
    {
        CUndo::Record(new CUndoCurrentLayer(m_pCurrentLayer));
    }

    m_pCurrentLayer = pCurrLayer;
    NotifyListeners(ON_LAYER_SELECT, m_pCurrentLayer);
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayerManager::ResolveLayerLinks()
{
    for (LayersMap::const_iterator it = m_layersMap.begin(); it != m_layersMap.end(); ++it)
    {
        CObjectLayer* pLayer = it->second;

        // Try to connect to parent layer.
        CObjectLayer* pNewParent = FindLayer(pLayer->m_parentGUID);

        if (pLayer->GetParent() != NULL && pLayer->GetParent() != pNewParent)
        {
            // Deatch from old parent layer.
            pLayer->GetParent()->RemoveChild(pLayer);
        }
        if (pNewParent)
        {
            // Attach to new parent layer.
            pNewParent->AddChild(pLayer);
        }
    }
    NotifyListeners(ON_LAYER_UPDATEALL, 0);
}

//////////////////////////////////////////////////////////////////////////
bool CObjectLayerManager::InitLayerSwitches(bool isOnlyClear)
{
    if (isOnlyClear && m_layerSwitches.size())
    {
        std::vector<CBaseObjectPtr> objects;
        m_pObjectManager->GetAllObjects(objects);

        for (std::list<CObjectLayer*>::iterator it = m_layerSwitches.begin(); it != m_layerSwitches.end(); ++it)
        {
            CObjectLayer* pLayer = (*it);

            if (!pLayer->GetLayerID())
            {
                continue;
            }

            if (gEnv->pEntitySystem)
            {
                gEnv->pEntitySystem->EnableLayer(pLayer->GetName().toUtf8().data(), true);
            }

            for (int k = 0; k < objects.size(); k++)
            {
                CBaseObject* pObj = objects[k];
                if (pObj->GetLayer() == pLayer)
                {
                    if (qobject_cast<CSplineObject*>(pObj))
                    {
                        CSplineObject* pSpline = (CSplineObject*)pObj;
                        pSpline->SetLayerId(0);
                        if (!pLayer->IsPhysics())
                        {
                            pSpline->SetPhysics(true); // restore init state
                        }
                    }
                    else if (!qobject_cast<CEntityObject*>(pObj) || !((CEntityObject*)pObj)->GetIEntity())
                    {
                        IRenderNode* pRenderNode = pObj->GetEngineNode();
                        if (pRenderNode)
                        {
                            pRenderNode->SetLayerId(0);
                            pRenderNode->SetRndFlags(pRenderNode->GetRndFlags() & ~ERF_NO_PHYSICS);
                        }
                    }
                }
            }
            pLayer->SetLayerID(0);
        }
    }

    m_layerSwitches.clear();
    if (isOnlyClear)
    {
        return true;
    }


    uint16 curLayerId = 0;
    bool bRet = false;
    for (LayersMap::const_iterator it = m_layersMap.begin(); it != m_layersMap.end(); ++it)
    {
        CObjectLayer* pLayer = it->second;
        pLayer->SetLayerID(++curLayerId);
        m_layerSwitches.push_back(pLayer);
        bRet = true;
    }
    return bRet;
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayerManager::ExportLayerSwitches(XmlNodeRef& node)
{
    if (m_layerSwitches.size() == 0)
    {
        return;
    }

    XmlNodeRef layersNode = node->newChild("Layers");

    for (std::list<CObjectLayer*>::iterator it = m_layerSwitches.begin(); it != m_layerSwitches.end(); ++it)
    {
        CObjectLayer* pLayer = *it;

        XmlNodeRef layerNode = layersNode->newChild("Layer");
        layerNode->setAttr("Name", pLayer->GetName().toUtf8().data());
        layerNode->setAttr("Parent", pLayer->GetParent() ? pLayer->GetParent()->GetName().toUtf8().data() : "");
        layerNode->setAttr("Id", pLayer->GetLayerID());
        layerNode->setAttr("Specs", pLayer->GetSpecs());
        if (!pLayer->IsPhysics())
        {
            layerNode->setAttr("Physics", 0);
        }
        if (pLayer->IsDefaultLoaded())
        {
            layerNode->setAttr("DefaultLoaded", 1);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayerManager::AssignLayerIDsToRenderNodes()
{
    std::vector<CBaseObjectPtr> objects;
    m_pObjectManager->GetAllObjects(objects);

    for (int k = 0; k < objects.size(); k++)
    {
        CBaseObject* pObj = objects[k];
        CObjectLayer* pLayer = pObj->GetLayer();
        if (pLayer != NULL)
        {
            if (qobject_cast<CRoadObject*>(pObj))
            {
                CRoadObject* pRoad = (CRoadObject*)pObj;
                pRoad->SetLayerId(pLayer->GetLayerID());
            }
            else if (!qobject_cast<CEntityObject*>(pObj) || !((CEntityObject*)pObj)->GetIEntity())
            {
                IRenderNode* pRenderNode = pObj->GetEngineNode();
                if (NULL != pRenderNode)
                {
                    pRenderNode->SetLayerId(pLayer->GetLayerID());
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayerManager::SetupLayerSwitches(bool isOnlyClear, bool isOnlyRenderNodes)
{
    if (!isOnlyRenderNodes)
    {
        if (!gEnv->pEntitySystem)
        {
            return;
        }

        gEnv->pEntitySystem->ClearLayers();
    }
    std::vector<CBaseObjectPtr> objects;
    m_pObjectManager->GetAllObjects(objects);

    if (!isOnlyRenderNodes && isOnlyClear)
    {
        // Show brashes in the end of game mode
        gEnv->p3DEngine->ActivateObjectsLayer(~0, true, true, true, true, "ALL_OBJECTS");

        // Update visibility for hidden brushes
        for (int k = 0; k < objects.size(); k++)
        {
            CBaseObject* pObj = objects[k];
            if (!qobject_cast<CEntityObject*>(pObj) && pObj->IsHidden())
            {
                pObj->UpdateVisibility(true); // need for invalidation
                pObj->UpdateVisibility(false);
            }
        }
        return;
    }

    for (std::list<CObjectLayer*>::iterator it = m_layerSwitches.begin(); it != m_layerSwitches.end(); ++it)
    {
        CObjectLayer* pLayer = (*it);

        for (int k = 0; k < objects.size(); k++)
        {
            CBaseObject* pObj = objects[k];

            // Skip hidden brushes in Game mode
            if (!isOnlyRenderNodes && (pObj->IsHidden() || pObj->IsHiddenBySpec()))
            {
                continue;
            }

            if (pObj->GetLayer() == pLayer)
            {
                if (qobject_cast<CSplineObject*>(pObj))
                {
                    CSplineObject* pSpline = (CSplineObject*)pObj;
                    pSpline->SetLayerId(pLayer->GetLayerID());
                    if (!pLayer->IsPhysics())
                    {
                        pSpline->SetPhysics(false);
                    }
                }
                else if (!qobject_cast<CEntityObject*>(pObj) || !((CEntityObject*)pObj)->GetIEntity())
                {
                    IRenderNode* pRenderNode = pObj->GetEngineNode();
                    if (pRenderNode)
                    {
                        pRenderNode->SetLayerId(pLayer->GetLayerID());
                        if (!pLayer->IsPhysics())
                        {
                            pRenderNode->SetRndFlags(pRenderNode->GetRndFlags() | ERF_NO_PHYSICS);
                        }
                    }
                }
            }
        }

        if (!isOnlyRenderNodes)
        {
            if (gEnv->pEntitySystem)
            {
                gEnv->pEntitySystem->AddLayer(pLayer->GetName().toUtf8().data(), pLayer->GetParent() ? pLayer->GetParent()->GetName().toUtf8().data() : "", pLayer->GetLayerID(), pLayer->IsPhysics(), pLayer->GetSpecs(), pLayer->IsDefaultLoaded());
            }

            for (int k = 0; k < objects.size(); k++)
            {
                CBaseObject* pObj = objects[k];
                if (pObj->GetLayer() == pLayer)
                {
                    if (qobject_cast<CEntityObject*>(pObj) && ((CEntityObject*)pObj)->GetIEntity())
                    {
                        if (IRenderNode* node = pObj->GetEngineNode())
                        {
                            node->SetLayerId(pLayer->GetLayerID());
                        }

                        if (gEnv->pEntitySystem)
                        {
                            gEnv->pEntitySystem->AddEntityToLayer(pLayer->GetName().toUtf8().data(), ((CEntityObject*)pObj)->GetEntityId());
                        }
                    }
                }
            }
        }
    }
    // make sure parent - child relation is valid in editor
    if (gEnv->pEntitySystem)
    {
        gEnv->pEntitySystem->LinkLayerChildren();
    }

    // Hide brashes when Game mode is started
    if (!isOnlyRenderNodes)
    {
        gEnv->p3DEngine->ActivateObjectsLayer(~0, false, true, true, true,  "ALL_OBJECTS");
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayerManager::SetGameMode(bool inGame)
{
    if (InitLayerSwitches(!inGame))
    {
        SetupLayerSwitches(!inGame);
    }
}


//////////////////////////////////////////////////////////////////////////
void CObjectLayerManager::FreezeROnly()
{
    for (LayersMap::const_iterator it = m_layersMap.begin(); it != m_layersMap.end(); ++it)
    {
        CObjectLayer* pLayer = it->second;
        if (pLayer->IsExternal() && !pLayer->IsFrozen())
        {
            QString file = pLayer->GetExternalLayerPath();
            uint32 attr = CFileUtil::GetAttributes(file.toUtf8().data());
            if (attr & SCC_FILE_ATTRIBUTE_READONLY)
            {
                pLayer->SetFrozen(true);
            }
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CObjectLayerManager::Isolate(CObjectLayer* pLayer)
{
    // Try to restore states if only specified layer is visible
    if (m_visibleSetLayer == pLayer->GetGUID() && pLayer->IsVisible())
    {
        bool bIsRestoreStates = true;

        for (LayersMap::const_iterator it = m_layersMap.begin(); it != m_layersMap.end(); ++it)
        {
            CObjectLayer* pObjectLayer = it->second;
            if (pLayer != pObjectLayer && pObjectLayer->IsVisible())
            {
                bIsRestoreStates = false;
                break;
            }
        }

        if (bIsRestoreStates)
        {
            // Unhide layers, which were hidden by previous call of Isolate()
            for (LayersMap::const_iterator it = m_layersMap.begin(); it != m_layersMap.end(); ++it)
            {
                CObjectLayer* pObjectLayer = it->second;
                pObjectLayer->SetVisible(m_visibleSet.find(pObjectLayer->GetGUID()) != m_visibleSet.end());
            }

            m_visibleSet.clear();
            m_visibleSetLayer = GuidUtil::NullGuid;
            return;
        }
    }

    // Store visible states
    m_visibleSet.clear();
    for (LayersMap::const_iterator it = m_layersMap.begin(); it != m_layersMap.end(); ++it)
    {
        CObjectLayer* pObjectLayer = it->second;
        if (pObjectLayer->IsVisible())
        {
            m_visibleSet.insert(pObjectLayer->GetGUID());
        }
    }
    m_visibleSetLayer = pLayer->GetGUID();


    // Hide layers
    for (LayersMap::const_iterator it = m_layersMap.begin(); it != m_layersMap.end(); ++it)
    {
        CObjectLayer* pObjectLayer = it->second;
        if (pLayer != pObjectLayer && pObjectLayer->IsVisible())
        {
            pObjectLayer->SetVisible(false);
        }
    }
    if (!pLayer->IsVisible())
    {
        pLayer->SetVisible(true);
    }
}


//////////////////////////////////////////////////////////////////////////
bool CObjectLayerManager::ReloadLayer(CObjectLayer* pLayer)
{
    if (!pLayer->IsExternal())
    {
        return false;
    }

    QString path = pLayer->GetExternalLayerPath();
    CObjectLayer* pParentLayer = pLayer->GetParent();

    DeleteLayer(pLayer);

    CObjectLayer* pNewLayer = ImportLayerFromFile(path.toUtf8().data());
    if (!pNewLayer)
    {
        return false;
    }

    if (pParentLayer)
    {
        pParentLayer->AddChild(pNewLayer);
    }

    SetCurrentLayer(pNewLayer);

    return true;
}

//////////////////////////////////////////////////////////////////////////
namespace
{
    bool PyCreateLayer(const char* pName)
    {
        CObjectLayerManager* pLayerManager = GetIEditor()->GetObjectManager()->GetLayersManager();
        if (!pLayerManager)
        {
            AZ_Error("LayerManager", false, "The Layer Manager is invalid.\nIt could be corrupted, or you may be attempting to gather layer information before it has been initialzied.");
            return false;
        }

        CObjectLayer* pLayer = pLayerManager->FindLayerByName(pName);
        if (pLayer)
        {
            return false;
        }

        pLayer = pLayerManager->CreateLayer();
        pLayer->SetName(pName);
        pLayerManager->SetCurrentLayer(pLayer);
        pLayerManager->NotifyLayerChange(pLayer);
        return true;
    }

    bool PyIsLayerExist(const char* pName)
    {
        if (GetIEditor()->GetObjectManager()->GetLayersManager()->FindLayerByName(pName))
        {
            return true;
        }
        return false;
    }

    void PyFreezeLayer(const char* pName)
    {
        CObjectLayerManager* pLayerManager = GetIEditor()->GetObjectManager()->GetLayersManager();
        CObjectLayer* pLayer = pLayerManager->FindLayerByName(pName);

        if (!pLayer)
        {
            AZ_Error("LayerManager", false, "Could not resolve layer name \"%s\" to a layer.\nMake sure it exists before attempting to freeze the layer.", pName);
            return;
        }

        CUndo undo("Freeze Layer");
        pLayer->SetFrozen(true);
    }

    void PyUnfreezeLayer(const char* pName)
    {
        CObjectLayerManager* pLayerManager = GetIEditor()->GetObjectManager()->GetLayersManager();
        CObjectLayer* pLayer = pLayerManager->FindLayerByName(pName);

        if (!pLayer)
        {
            AZ_Error("LayerManager", false, "Could not resolve layer name \"%s\" to a layer.\nMake sure it exists before attempting to unfreeze the layer.", pName);
            return;
        }

        CUndo undo("Unfreeze Layer");
        pLayer->SetFrozen(false);
    }

    void PyHideLayer(const char* pName)
    {
        CObjectLayerManager* pLayerManager = GetIEditor()->GetObjectManager()->GetLayersManager();
        CObjectLayer* pLayer = pLayerManager->FindLayerByName(pName);
        if (!pLayer)
        {
            AZ_Error("LayerManager", false, "Could not resolve layer name \"%s\" to a layer.\nMake sure it exists before attempting to hide the layer.", pName);
            return;
        }

        CUndo undo("Hide Layer");
        pLayer->SetVisible(false);
    }

    void PyUnhideLayer(const char* pName)
    {
        CObjectLayerManager* pLayerManager = GetIEditor()->GetObjectManager()->GetLayersManager();
        CObjectLayer* pLayer = pLayerManager->FindLayerByName(pName);
        if (!pLayer)
        {
            AZ_Error("LayerManager", false, "Could not resolve layer name \"%s\" to a layer.\nMake sure it exists before attempting to show the layer.", pName);
            return;
        }

        CUndo undo("Unhide Layer");
        pLayer->SetVisible(true);
    }

    void PyRenameLayer(const char* pNameOld, const char* pNameNew)
    {
        CObjectLayerManager* pLayerManager = GetIEditor()->GetObjectManager()->GetLayersManager();
        CObjectLayer* pLayer = pLayerManager->FindLayerByName(pNameOld);
        if (!pLayer)
        {
            AZ_Error("LayerManager", false, "Could not resolve layer name \"%s\" to a layer.\nMake sure it exists before attempting to rename the layer.", pNameOld);
            return;
        }

        CUndo undo("Rename Layer");
        pLayer->SetName(pNameNew, true);
        pLayerManager->NotifyLayerChange(pLayer);
    }

    QString PyGetNameOfSelectedLayer()
    {
        CObjectLayerManager* pLayerManager = GetIEditor()->GetObjectManager()->GetLayersManager();
        CObjectLayer* pLayer = pLayerManager->GetCurrentLayer();
        if (!pLayer)
        {
            AZ_Error("LayerManager", false, "Could not get the layer name of the selected layer.\nMake sure you have a layer selected.");
            return QString();
        }
        return pLayer->GetName();
    }

    void PySelectLayer(const char* pName)
    {
        CObjectLayerManager* pLayerManager = GetIEditor()->GetObjectManager()->GetLayersManager();
        CObjectLayer* pLayer = pLayerManager->FindLayerByName(pName);
        if (!pLayer)
        {
            AZ_Error("LayerManager", false, "Could not resolve layer name \"%s\" to a layer.\nMake sure it exists before attempting to select the layer by name.", pName);
            return;
        }
        pLayerManager->SetCurrentLayer(pLayer);
        pLayerManager->NotifyLayerChange(pLayer);
    }

    std::vector<std::string> PyGetAllLayerNames()
    {
        CObjectLayerManager* pLayerMgr = GetIEditor()->GetObjectManager()->GetLayersManager();
        if (!pLayerMgr)
        {
            AZ_Error("LayerManager", false, "The Layer Manager is invalid.\nIt could be corrupted, or you may be attempting to gather layer information before it has been initialzied.");
            return std::vector<std::string>();
        }

        std::vector<std::string> result;
        std::vector<CObjectLayer*> layers;
        pLayerMgr->GetLayers(layers);
        for (size_t i = 0; i < layers.size(); ++i)
        {
            result.push_back(layers[i]->GetName().toUtf8().data());
        }
        return result;
    }
}

REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyCreateLayer, general, create_layer,
    "Creates new layer with specific name and returns bool value if layer was created or not.",
    "general.create_layer(str layerName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyFreezeLayer, general, freeze_layer,
    "Freezes layer with specific name.",
    "general.freeze_layer(str layerName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyUnfreezeLayer, general, unfreeze_layer,
    "Unfreezes layer with specific name.",
    "general.unfreeze_layer(str layerName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyHideLayer, general, hide_layer,
    "Hides layer with specific name.",
    "general.hide_layer(str layerName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyUnhideLayer, general, unhide_layer,
    "Unhides layer with specific name.",
    "general.unhide_layer(str layerName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyRenameLayer, general, rename_layer,
    "Renames layer with specific name.",
    "general.rename_layer(str layerName, str newLayerName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyIsLayerExist, general, is_layer_exist,
    "Checks existence of layer with specific name.",
    "general.is_layer_exist(str layerName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyGetNameOfSelectedLayer, general, get_name_of_selected_layer,
    "Returns the name of the selected layer.",
    "general.get_name_of_selected_layer(str layerName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PySelectLayer, general, select_layer,
    "Selects the layer with the given name.",
    "general.select_layer(str layerName)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyGetAllLayerNames, general, get_names_of_all_layers,
    "Get a list of all layer names in the level.",
    "general.get_names_of_all_layers()");
