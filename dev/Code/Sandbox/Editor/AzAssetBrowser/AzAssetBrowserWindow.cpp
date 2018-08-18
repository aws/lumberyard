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
#include "StdAfx.h"

#include <AzAssetBrowser/ui_AzAssetBrowserWindow.h>

#include <Editor/AzAssetBrowser/AzAssetBrowserWindow.h>
#include <Editor/AzAssetBrowser/Preview/PreviewWidget.h>

#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeView.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/Search/SearchParametersWidget.h>

AzAssetBrowserWindow::AzAssetBrowserWindow(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::AzAssetBrowserWindowClass())
    , m_filterModel(new AzToolsFramework::AssetBrowser::AssetBrowserFilterModel(parent))
{
    m_ui->setupUi(this);
    m_ui->m_searchWidget->Setup(true, true);

    using namespace AzToolsFramework::AssetBrowser;
    AssetBrowserComponentRequestBus::BroadcastResult(m_assetBrowserModel, &AssetBrowserComponentRequests::GetAssetBrowserModel);
    AZ_Assert(m_assetBrowserModel, "Failed to get filebrowser model");
    m_filterModel->setSourceModel(m_assetBrowserModel);
    m_filterModel->SetFilter(m_ui->m_searchWidget->GetFilter());

    m_ui->m_assetBrowserTreeViewWidget->setModel(m_filterModel.data());
    m_ui->m_assetBrowserTreeViewWidget->SetThumbnailContext("AssetBrowser");

    // set up search parameters widget
    auto& filters = m_ui->m_searchWidget->GetFilter()->GetSubFilters();
    auto it = AZStd::find_if(filters.begin(), filters.end(),
            [](FilterConstType filter)
            {
                return filter->GetTag() == "AssetTypes";
            });
    if (it != filters.end())
    {
        m_ui->m_searchParametersWidget->SetFilter(*it);
    }

    connect(m_ui->m_searchWidget->GetFilter().data(), &AssetBrowserEntryFilter::updatedSignal,
        m_filterModel.data(), &AssetBrowserFilterModel::filterUpdatedSlot);
    connect(m_ui->m_assetBrowserTreeViewWidget, &AssetBrowserTreeView::selectionChangedSignal,
        this, &AzAssetBrowserWindow::SelectionChangedSlot);
    connect(m_ui->m_searchParametersWidget, &SearchParametersWidget::ClearAllSignal, this,
        [=]() { m_ui->m_searchWidget->ClearAssetTypeFilter(); });
    connect(m_ui->m_assetBrowserTreeViewWidget, &QAbstractItemView::doubleClicked, this, &AzAssetBrowserWindow::DoubleClickedItem);

    m_ui->m_assetBrowserTreeViewWidget->LoadState("AssetBrowserTreeView_main");
}

AzAssetBrowserWindow::~AzAssetBrowserWindow()
{
    m_ui->m_assetBrowserTreeViewWidget->SaveState();
}

void AzAssetBrowserWindow::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions options;
    options.preferedDockingArea = Qt::LeftDockWidgetArea;
    options.sendViewPaneNameBackToAmazonAnalyticsServers = true;
    AzToolsFramework::RegisterViewPane<AzAssetBrowserWindow>(LyViewPane::AssetBrowser, LyViewPane::CategoryTools, options);
}

void AzAssetBrowserWindow::UpdatePreview() const
{
    auto selectedAssets = m_ui->m_assetBrowserTreeViewWidget->GetSelectedAssets();
    if (selectedAssets.size() != 1)
    {
        m_ui->m_previewWidget->Clear();
        return;
    }
    m_ui->m_previewWidget->Display(selectedAssets.front());
}

void AzAssetBrowserWindow::SelectionChangedSlot(const QItemSelection& /*selected*/, const QItemSelection& /*deselected*/) const
{
    UpdatePreview();
}

// while its tempting to use Activated here, we dont actually want it to count as activation
// just becuase on some OS clicking once is activation.
void AzAssetBrowserWindow::DoubleClickedItem(const QModelIndex& element)
{
    using namespace AzToolsFramework;
    using namespace AzToolsFramework::AssetBrowser;
    // assumption: Double clicking an item selects it before telling us we double clicked it.
    auto selectedAssets = m_ui->m_assetBrowserTreeViewWidget->GetSelectedAssets();
    for (const AssetBrowserEntry* entry : selectedAssets)
    {
        AZ::Data::AssetId assetIdToOpen;

        if (const ProductAssetBrowserEntry* productEntry = azrtti_cast<const ProductAssetBrowserEntry*>(entry))
        {
            assetIdToOpen = productEntry->GetAssetId();
        }
        else if (const SourceAssetBrowserEntry* sourceEntry = azrtti_cast<const SourceAssetBrowserEntry*>(entry))
        {
            // manufacture an empty AssetID with the source's UUID
            assetIdToOpen = AZ::Data::AssetId(sourceEntry->GetSourceUuid(), 0);
        }
        
        if (assetIdToOpen.IsValid())
        {
            bool handledBySomeone = false;
            AssetBrowserInteractionNotificationBus::Broadcast(&AssetBrowserInteractionNotifications::OpenAssetInAssociatedEditor, assetIdToOpen, handledBySomeone);
        }
    }

}

#include <AzAssetBrowser/AzAssetBrowserWindow.moc>
