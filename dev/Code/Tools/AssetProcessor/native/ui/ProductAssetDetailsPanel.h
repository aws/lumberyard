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

#include "AssetDetailsPanel.h"
#include <QScopedPointer>

class QItemSelection;

namespace AZ
{
    namespace Data
    {
        struct AssetId;
    }
}

namespace Ui
{
    class ProductAssetDetailsPanel;
}

namespace AssetProcessor
{
    class AssetDatabaseConnection;
    class ProductAssetTreeItemData;

    class ProductAssetDetailsPanel
        : public AssetDetailsPanel
    {
        Q_OBJECT
    public:
        explicit ProductAssetDetailsPanel(QWidget* parent = nullptr);
        ~ProductAssetDetailsPanel() override;

    public Q_SLOTS:
        void AssetDataSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

    protected:
        void ResetText();
        void SetDetailsVisible(bool visible);

        void BuildOutgoingProductDependencies(
            AssetDatabaseConnection& assetDatabaseConnection,
            const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData,
            const AZStd::string& platform);

        void BuildincomingProductDependencies(
            AssetDatabaseConnection& assetDatabaseConnection,
            const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData,
            const AZ::Data::AssetId& assetId,
            const AZStd::string& platform);

        QScopedPointer<Ui::ProductAssetDetailsPanel> m_ui;
    };
} // namespace AssetProcessor
