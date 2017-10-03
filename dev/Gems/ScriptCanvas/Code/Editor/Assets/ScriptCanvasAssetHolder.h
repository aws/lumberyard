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

#include <AzCore/Component/EntityId.h>
#include <AzCore/Asset/AssetCommon.h>

namespace ScriptCanvasEditor
{
    class ScriptCanvasAsset;

    /*! ScriptCanvasAssetHolder
    Wraps a ScriptCanvasAsset reference and registers for the individual AssetBus events
    for saving, loading and unloading the asset.
    The ScriptCanvasAsset Holder contains functionality for activating the ScriptCanvasEntity stored on the reference asset
    as well as attempting to open the ScriptCanvasAsset within the ScriptCanvas Editor.
    It also provides the EditContext reflection for opening the asset in the ScriptCanvas Editor via a button
    */
    class ScriptCanvasAssetHolder
        : private AZ::Data::AssetBus::Handler
    {
    public:
        AZ_RTTI(ScriptCanvasAssetHolder, "{3E80CEE3-2932-4DC1-AADF-398FDDC6DEFE}");
        AZ_CLASS_ALLOCATOR(ScriptCanvasAssetHolder, AZ::SystemAllocator, 0);

        ScriptCanvasAssetHolder();
        ScriptCanvasAssetHolder(AZ::Data::Asset<ScriptCanvasAsset> asset);
        ~ScriptCanvasAssetHolder() override;
        
        static void Reflect(AZ::ReflectContext* context);
        void Init();

        void SetAsset(const AZ::Data::Asset<ScriptCanvasAsset>& asset);
        AZ::Data::Asset<ScriptCanvasAsset> GetAsset() const;

        AZ::EntityId GetGraphId() const;
        void ActivateAssetData();

        void LaunchScriptCanvasEditor(const AZ::Data::AssetId&, const AZ::Data::AssetType&) const;
        void OpenEditor() const;

    protected:

        //=====================================================================
        // AZ::Data::AssetBus
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetUnloaded(const AZ::Data::AssetId assetId, const AZ::Data::AssetType assetType) override;
        void OnAssetSaved(AZ::Data::Asset<AZ::Data::AssetData> asset, bool isSuccessful) override;
        //=====================================================================

        //! Reloads the Script From the AssetData if it has changed
        AZ::u32 OnScriptChanged();

        AZ::Data::Asset<ScriptCanvasAsset> m_scriptCanvasAsset;

        // virtual property to implement button function
        bool m_openEditorButton = false;
    };

}