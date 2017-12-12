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
#include "stdafx.h"

#include <Editor/AzAssetBrowser/AzAssetBrowserWindow.h>
#include <Editor/AzAssetBrowser/Preview/PreviewWidget.h>

#include <AzAssetBrowser/ui_AzAssetBrowserWindow.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserTreeView.h>
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
    AssetBrowserComponentRequestsBus::BroadcastResult(m_assetBrowserModel, &AssetBrowserComponentRequests::GetAssetBrowserModel);
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

    connect(m_ui->m_searchWidget->GetFilter().data(), &AzToolsFramework::AssetBrowser::AssetBrowserEntryFilter::updatedSignal,
        m_filterModel.data(), &AzToolsFramework::AssetBrowser::AssetBrowserFilterModel::filterUpdatedSlot);
    connect(m_ui->m_assetBrowserTreeViewWidget, &AzToolsFramework::AssetBrowser::AssetBrowserTreeView::selectionChangedSignal, 
        this, &AzAssetBrowserWindow::SelectionChangedSlot);
    connect(m_ui->m_searchParametersWidget, &AzToolsFramework::AssetBrowser::SearchParametersWidget::ClearAllSignal, 
        [=]() { m_ui->m_searchWidget->ClearAssetTypeFilter(); });

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

#include <AzAssetBrowser/AzAssetBrowserWindow.moc>