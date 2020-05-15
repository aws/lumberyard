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

// Description : Manages objects layers.


#ifndef CRYINCLUDE_EDITOR_OBJECTS_OBJECTLAYERMANAGER_H
#define CRYINCLUDE_EDITOR_OBJECTS_OBJECTLAYERMANAGER_H
#pragma once


#define LAYER_FILE_EXTENSION ".lyr"
#define LAYER_PATH "Layers/"

#include <AzCore/PlatformDef.h>
#include "ObjectLayer.h"

class CObjectManager;
class CObjectArchive;
class CLayerNodeAnimator;
struct IAnimSequence;

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
/** Manager of objects layers.
        Instance of these hosted by CObjectManager class.
*/
class SANDBOX_API CObjectLayerManager
    : public IEditorNotifyListener
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
public:
    enum EUpdateType
    {
        ON_LAYER_ADD,
        ON_LAYER_REMOVE,
        ON_LAYER_MODIFY,
        ON_LAYER_SELECT,
        ON_LAYER_UPDATEALL,
    };
    //! Update callback, first integer is one of EUpdateType enums.
    typedef Functor2<int, CObjectLayer*> LayersUpdateCallback;

    CObjectLayerManager(CObjectManager* pObjectManager);
    virtual ~CObjectLayerManager();

    void OnEditorNotifyEvent(EEditorNotifyEvent event);

    //! Ensures that the editor has finished loading a level before allowing layers to be modified.
    bool CanModifyLayers();
    //! Creates a new layer, and associate it with layer manager.
    CObjectLayer* CreateLayer(const GUID* pGUID = 0);
    //! Add already created layer to manager.
    void AddLayer(CObjectLayer* pLayer);
    //! Check if it's possible to delete a layer.
    bool CanDeleteLayer(CObjectLayer* pLayer);
    //! Delete layer from layer manager.
    void DeleteLayer(CObjectLayer* pLayer);
    //! Delete all layers from layer manager.
    void ClearLayers();

    //! Find layer by layer GUID.
    CObjectLayer* FindLayer(REFGUID guid) const;

    //! Search for layer by name.
    CObjectLayer* FindLayerByName(const QString& layerName) const;

    //! Get a list of all managed layers.
    void GetLayers(std::vector<CObjectLayer*>& layers) const;

    //! Set this layer is current.
    void SetCurrentLayer(CObjectLayer* pCurrLayer);
    CObjectLayer* GetCurrentLayer() const { return m_pCurrentLayer; };

    const QString& GetLayersPath() const { return m_layersPath; }
    void SetLayersPath(const QString& layersPath) { m_layersPath = layersPath; }

    //! Associate On Layers update listener.
    //! This callback will be called everytime layers are added/removed or modified.
    void AddUpdateListener(const LayersUpdateCallback& cb);
    //! Remove update listeners.
    void RemoveUpdateListener(const LayersUpdateCallback& cb);

    //! Called when some layers gets modified.
    void NotifyLayerChange(CObjectLayer* pLayer);

    //! Export layer to objects archive.
    void ExportLayer(CObjectArchive& ar, CObjectLayer* pLayer, bool bExportExternalChilds);
    //! Import layer from objects archive.
    CObjectLayer* ImportLayer(CObjectArchive& ar, CObjectLayer* pInitLayer);

    //! Import layer from file.
    CObjectLayer* ImportLayerFromFile(const char* filename);

    //! Save all modified external layers to disk
    void SaveExternalLayers();

    // Serialize layer manager (called from object manager).
    void    Serialize(CObjectArchive& ar);

    //! Resolve links between layers.
    void ResolveLayerLinks();

    void CreateMainLayer(const GUID* pGUID = 0);

    bool InitLayerSwitches(bool isOnlyClear = false);
    void ExportLayerSwitches(XmlNodeRef& node);
    void SetupLayerSwitches(bool isOnlyClear = false,  bool isOnlyRenderNodes = false);

    void SetLayerNodeAnimators(IAnimSequence* pSequence, bool isOnlyClear = false);
    void SetLayerNodeAnimators(IMovieSystem* pMovieSystem, bool isOnlyClear = false);

    void SetGameMode(bool inGame);

    //! Freeze read-only layers.
    void FreezeROnly();

    // hide all layers excluding specified layer or toggle it back
    void Isolate(CObjectLayer* pLayer);

    bool ReloadLayer(CObjectLayer* pLayer);

    // iterate over all layers and make sure all render nodes have valid IDs
    void AssignLayerIDsToRenderNodes();

private:
    bool SaveExternalLayer(CObjectArchive* pArchive, CObjectLayer* pLayer);
    void LoadExternalLayer(CObjectArchive& ar, CObjectLayer* pLayer, const QString& fullName);
    void NotifyListeners(EUpdateType type, CObjectLayer* pLayer);

    //! Prompt for a name of an imported layer.  Includes retry for blank or duplicate names
    bool PromptForLayerName(QString& layerName) const;

    //////////////////////////////////////////////////////////////////////////
    //! Map of layer GUID to layer pointer.
    typedef std::map<GUID, TSmartPtr<CObjectLayer>, guid_less_predicate> LayersMap;
    LayersMap m_layersMap;

    //! Pointer to currently active layer.
    TSmartPtr<CObjectLayer> m_pCurrentLayer;
    //! Main layer, root for all other layers.
    CObjectLayer* m_pMainLayer;
    //////////////////////////////////////////////////////////////////////////

    CFunctorsList<LayersUpdateCallback> m_listeners;
    CObjectManager* m_pObjectManager;
    //! Layers path relative to level folder.
    QString m_layersPath;

    bool m_bOverwriteDuplicates;
    bool m_bLevelLoading;

    // support Layer Switches in Flow Graph
    std::list<CObjectLayer*> m_layerSwitches;

    // visible set for restoring states in Isolate()
    std::set<GUID, guid_less_predicate> m_visibleSet;
    GUID m_visibleSetLayer;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_OBJECTLAYERMANAGER_H
