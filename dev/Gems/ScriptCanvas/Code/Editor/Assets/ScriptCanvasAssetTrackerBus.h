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

#pragma once

#include <QWidget>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/vector.h>

#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>

#include <Editor/Assets/ScriptCanvasAssetTrackerDefinitions.h>
#include <Editor/Assets/ScriptCanvasMemoryAsset.h>

namespace AZ 
{ 
    class EntityId; 
}

namespace ScriptCanvas
{
    class ScriptCanvasAssetBase;
}

namespace ScriptCanvasEditor
{
    class ScriptCanvasAssetHandler;
    
    class AssetTrackerRequests
        : public AZ::EBusTraits
    {
    public:

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Callback used to know when a save operation failed or succeeded
        using OnSave = AZStd::function<void(bool saveSuccess, AZ::Data::Asset<ScriptCanvas::ScriptCanvasAssetBase>)>;

        //! Callback used when the asset has been loaded and is ready for use
        using OnAssetReadyCallback = AZStd::function<void(ScriptCanvasMemoryAsset&)>;

        //! Callback used when the asset is newly created
        using OnAssetCreatedCallback = OnAssetReadyCallback;

        // Operations

        //! Creates a new Script Canvas asset and tracks it
        virtual AZ::Data::AssetId Create(AZStd::string_view assetAbsolutePath, AZ::Data::AssetType assetType, Callbacks::OnAssetCreatedCallback onAssetCreatedCallback) { return AZ::Data::AssetId(); }

        //! Saves a Script Canvas asset to a new file, once the save is complete it will use the source Id (not the Id of the in-memory asset)
        virtual void SaveAs(AZ::Data::AssetId assetId, const AZStd::string& path, Callbacks::OnSave onSaveCallback) {}

        //! Saves a previously loaded Script Canvas asset to file
        virtual void Save(AZ::Data::AssetId assetId, Callbacks::OnSave onSaveCallback) {}

        //! Returns whether or not the specified asset is currently saving
        virtual bool IsSaving(AZ::Data::AssetId assetId) const { return false; }

        //! Loads a Script Canvas graph
        virtual bool Load(AZ::Data::AssetId assetId, AZ::Data::AssetType assetType, Callbacks::OnAssetReadyCallback onAssetReadyCallback) { return false; }

        //! Closes and unloads a Script Canvas graph from the tracker
        virtual void Close(AZ::Data::AssetId assetId) {}

        //! Creates the asset's view
        virtual void CreateView(AZ::Data::AssetId assetId, QWidget* parent) {}

        //! Releases the asset's view
        virtual void ClearView(AZ::Data::AssetId assetId) {}

        //! Used to make sure assets that are unloaded also get removed from tracking
        virtual void UntrackAsset(AZ::Data::AssetId assetId) {}

        using AssetList = AZStd::vector<ScriptCanvasMemoryAsset::pointer>;

        // Accessors
        virtual ScriptCanvasMemoryAsset::pointer GetAsset(AZ::Data::AssetId assetId) { return nullptr; }
        virtual ScriptCanvas::ScriptCanvasId GetScriptCanvasId(AZ::Data::AssetId assetId) { return AZ::EntityId(); }
        virtual ScriptCanvas::ScriptCanvasId GetScriptCanvasIdFromGraphId(AZ::EntityId graphId) { return AZ::EntityId(); }
        virtual ScriptCanvas::ScriptCanvasId GetGraphCanvasId(AZ::EntityId) { return AZ::EntityId(); }
        virtual ScriptCanvas::ScriptCanvasId GetGraphId(AZ::Data::AssetId assetId) { return ScriptCanvas::ScriptCanvasId(); }
        virtual Tracker::ScriptCanvasFileState GetFileState(AZ::Data::AssetId assetId) { return Tracker::ScriptCanvasFileState::INVALID; }
        virtual AZ::Data::AssetId GetAssetId(ScriptCanvas::ScriptCanvasId scriptCanvasSceneId) { return {}; }
        virtual AZ::Data::AssetType GetAssetType(ScriptCanvas::ScriptCanvasId scriptCanvasSceneId) { return {}; }
        virtual AZStd::string GetTabName(AZ::Data::AssetId assetId) { return {}; }
        virtual AssetList GetUnsavedAssets() { return {}; }
        virtual AssetList GetAssets() { return {}; }
        virtual AssetList GetAssetsIf(AZStd::function<bool(ScriptCanvasMemoryAsset::pointer asset)>) { return {}; }

        virtual AZ::EntityId GetSceneEntityIdFromEditorEntityId(AZ::Data::AssetId assetId, AZ::EntityId editorEntityId) { return AZ::EntityId(); }
        virtual AZ::EntityId GetEditorEntityIdFromSceneEntityId(AZ::Data::AssetId assetId, AZ::EntityId sceneEntityId) { return AZ::EntityId(); }

        // Setters / Updates
        virtual void UpdateFileState(AZ::Data::AssetId assetId, Tracker::ScriptCanvasFileState state) {}

        // Helpers
        virtual ScriptCanvasAssetHandler* GetAssetHandlerForType(AZ::Data::AssetType assetType) { return nullptr; }
    };

    using AssetTrackerRequestBus = AZ::EBus<AssetTrackerRequests>;

    //! These are the notifications sent by the AssetTracker only.
    //! We use these to communicate the asset status, do not use the AssetBus directly, all Script Canvas
    //! assets are managed by the AssetTracker
    class AssetTrackerNotifications
        : public AZ::EBusTraits
    {
    public:

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::Data::AssetId;

        // These will be forwarded as a result of the Asset System's events
        // this is deliberate in order to keep the AssetTracker as the only
        // place that interacts directly with the asset bus, but allowing
        // other systems to know the status of tracked assets
        virtual void OnAssetReady(const ScriptCanvasMemoryAsset::pointer asset) {}
        virtual void OnAssetReloaded(const ScriptCanvasMemoryAsset::pointer asset) {}
        virtual void OnAssetUnloaded(const AZ::Data::AssetId assetId, const AZ::Data::AssetType assetType) {}
        virtual void OnAssetSaved(const ScriptCanvasMemoryAsset::pointer asset, bool isSuccessful) {}
        virtual void OnAssetError(const ScriptCanvasMemoryAsset::pointer asset) {}

    };
    using AssetTrackerNotificationBus = AZ::EBus<AssetTrackerNotifications>;

    namespace Internal
    {
        class MemoryAssetNotifications
            : public AZ::EBusTraits
        {
        public:

            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

            virtual void OnAssetReady(const ScriptCanvasMemoryAsset* asset) {}
            virtual void OnAssetReloaded(const ScriptCanvasMemoryAsset* asset) {}
            virtual void OnAssetSaved(const ScriptCanvasMemoryAsset* asset, bool isSuccessful) {}
            virtual void OnAssetError(const ScriptCanvasMemoryAsset* asset) {}

        };
        using MemoryAssetNotificationBus = AZ::EBus<MemoryAssetNotifications>;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////

}
