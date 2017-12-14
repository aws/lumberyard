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

#include <AzCore/Asset/AssetCommon.h>

namespace AzToolsFramework
{
    namespace AssetEditor
    {
        // External interaction with Asset Editor
        class AssetEditorRequests
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            // Open Asset Editor and load asset
            virtual void OpenAssetEditor(const AZ::Data::Asset<AZ::Data::AssetData>& asset) = 0;
        };
        using AssetEditorRequestsBus = AZ::EBus<AssetEditorRequests>;

        // Internal interaction with existing Asset Editor widget
        class AssetEditorWidgetRequests
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            // Set asset within existing Asset Editor window
            virtual void SetAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset) const = 0;
        };
        using AssetEditorWidgetRequestsBus = AZ::EBus<AssetEditorWidgetRequests>;
    } // namespace AssetEditor
} // namespace AzToolsFramework
