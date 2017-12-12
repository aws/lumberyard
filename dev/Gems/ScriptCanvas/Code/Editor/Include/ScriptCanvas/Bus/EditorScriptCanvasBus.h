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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/Entity.h>

#include <Editor/Include/ScriptCanvas/Assets/ScriptCanvasAsset.h>

namespace ScriptCanvasEditor
{
    //=========================================================================
    // EditorGraphBus
    //=========================================================================
    class EditorScriptCanvasRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;


        //! Sets the name of the ScriptCanvas Graph.
        //! \param name value to set
        virtual void SetName(const AZStd::string& name) = 0;

        //! Gets the name of the ScriptCanvas Graph.
        //! \return reference to Graph name
        virtual const AZStd::string& GetName() const = 0;

        //! Will open the graph in the editor.
        virtual void OpenEditor() = 0;

        //! Used to close a graph that is currently opened in the editor.
        virtual void CloseGraph() = 0;

        //! Sets a new asset reference
        virtual void SetAsset(const AZ::Data::Asset<ScriptCanvasAsset>& asset) = 0;

        //! Retrieves script canvas asset reference
        virtual AZ::Data::Asset<ScriptCanvasAsset> GetAsset() const = 0;

        //! Returns the Entity ID of the Editor Entity that owns this graph.
        virtual AZ::EntityId GetEditorEntityId() const = 0;

    };
    using EditorScriptCanvasRequestBus = AZ::EBus<EditorScriptCanvasRequests>;

    // Above bus is keyed off of the graph Id. Which I don't really have access to.
    // This bus is here just so I can tell it to open the Editor.
    class EditorContextMenuRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Gets the GraphId for the EditorScriptCanvasComponent on the given entity.
        virtual AZ::EntityId GetGraphId() const = 0;
    };

    using EditorContextMenuRequestBus = AZ::EBus<EditorContextMenuRequests>;

    class EditorScriptCanvasAssetNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::Data::AssetId;

        /**
        * Custom connection policy to force OnScriptCanvasAssetReady to fire if the asset is already loaded
        * See AssetConnectionPolicy
        */
        template<class Bus>
        struct EditorGraphAssetConnectionPolicy
            : public AZ::EBusConnectionPolicy<Bus>
        {
            static void Connect(typename Bus::BusPtr& busPtr, typename Bus::Context& context, typename Bus::HandlerNode& handler, const typename Bus::BusIdType& id = 0)
            {
                AZ::EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, id);

                AZ::Data::Asset<AZ::Data::AssetData> asset(AZ::Data::AssetInternal::GetAssetData(id));
                if (asset.IsReady())
                {
                    handler->OnScriptCanvasAssetReady(asset);
                    auto scriptCanvasEntity = asset.GetAs<ScriptCanvasAsset>()->GetScriptCanvasEntity();
                    if (scriptCanvasEntity->GetState() == AZ::Entity::ES_ACTIVE)
                    {
                        handler->OnScriptCanvasAssetActivated(asset);
                    }
                }
            }
        };
        template<typename Bus>
        using ConnectionPolicy = EditorGraphAssetConnectionPolicy<Bus>;

        //! Notification which fires after an EditorGraph has received it's on AssetReady callback
        //! \param scriptCanvasAsset Script Canvas asset which is now ready for use in the Editor
        virtual void OnScriptCanvasAssetReady(const AZ::Data::Asset<ScriptCanvasAsset>& /*scriptCanvasAsset*/) {};

        //! Notification which fires after an EditorGraph has received it's on AssetReloaded callback
        //! \param scriptCanvasAsset Script Canvas asset which is now ready for use in the Editor
        virtual void OnScriptCanvasAssetReloaded(const AZ::Data::Asset<ScriptCanvasAsset>& /*scriptCanvaAsset */) {};

        //! Notification which fires after an EditorGraph has received it's on AssetReady callback
        //! \param AssetId AssetId of unloaded ScriptCanvas
        virtual void OnScriptCanvasAssetUnloaded(const AZ::Data::AssetId& /*assetId*/) {};

        //! Notification which fires after an EditorGraph has received an onAssetSaved callback
        //! \param scriptCanvasAsset Script Canvas asset which was attempted to be saved
        //! \param isSuccessful specified where the Script Canvas asset was successfully saved
        virtual void OnScriptCanvasAssetSaved(const AZ::Data::Asset<ScriptCanvasAsset>& /*scriptCanvasAsset*/, bool /*isSuccessful*/) {};

        virtual void OnScriptCanvasAssetActivated(const AZ::Data::Asset<ScriptCanvasAsset>& /*scriptCanvasAsset*/) {};
    };
    using EditorScriptCanvasAssetNotificationBus = AZ::EBus<EditorScriptCanvasAssetNotifications>;
}
