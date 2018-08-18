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

#include <Editor/AzAssetBrowser/AzAssetBrowserDialog.h>

#include "AzAssetBrowser/ui_AzAssetBrowserDialog.h"

#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeView.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/UI/UICore/QWidgetSavedState.h>
#include <AzCore/UserSettings/UserSettings.h>

#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QTimer>
#include <QAbstractButton>
#include <AzQtComponents/Components/DockBar.h>

AzAssetBrowserDialog::AzAssetBrowserDialog(AssetSelectionModel& selection, QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::AzAssetBrowserDialogClass())
    , m_filterModel(new AssetBrowserFilterModel(parent))
    , m_selection(selection)
{
    m_ui->setupUi(this);
    m_ui->m_searchWidget->Setup(true, false);

    m_ui->m_searchWidget->GetFilter()->AddFilter(m_selection.GetDisplayFilter());
    
    using namespace AzToolsFramework::AssetBrowser;
    AssetBrowserComponentRequestBus::BroadcastResult(m_assetBrowserModel, &AssetBrowserComponentRequests::GetAssetBrowserModel);
    AZ_Assert(m_assetBrowserModel, "Failed to get asset browser model");
    m_filterModel->setSourceModel(m_assetBrowserModel);
    m_filterModel->SetFilter(m_ui->m_searchWidget->GetFilter());

    QString name = m_selection.GetDisplayFilter()->GetName();

    m_ui->m_assetBrowserTreeViewWidget->setModel(m_filterModel.data());
    m_ui->m_assetBrowserTreeViewWidget->SetThumbnailContext("AssetBrowser");
    m_ui->m_assetBrowserTreeViewWidget->setSelectionMode(selection.GetMultiselect() ?
        QAbstractItemView::SelectionMode::ExtendedSelection : QAbstractItemView::SelectionMode::SingleSelection);
    m_ui->m_assetBrowserTreeViewWidget->setDragEnabled(false);

    // if the current selection is invalid, disable the Ok button
    m_ui->m_buttonBox->buttons().constFirst()->setEnabled(EvaluateSelection());
    m_ui->m_buttonBox->buttons().constFirst()->setProperty("class", "Primary");
    m_ui->m_buttonBox->buttons().constLast()->setProperty("class", "Secondary");

    connect(m_ui->m_searchWidget->GetFilter().data(), &AssetBrowserEntryFilter::updatedSignal, m_filterModel.data(), &AssetBrowserFilterModel::filterUpdatedSlot);
    connect(m_ui->m_assetBrowserTreeViewWidget, &QAbstractItemView::doubleClicked, this, &AzAssetBrowserDialog::DoubleClickedSlot);
    connect(m_ui->m_assetBrowserTreeViewWidget, &AssetBrowserTreeView::selectionChangedSignal, this,
        [this](const QItemSelection&, const QItemSelection&){ AzAssetBrowserDialog::SelectionChangedSlot(); });
    connect(m_ui->m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_ui->m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    m_ui->m_assetBrowserTreeViewWidget->LoadState("AssetBrowserTreeView_" + name);

    for (auto& assetId : selection.GetSelectedAssetIds())
    {
        m_ui->m_assetBrowserTreeViewWidget->SelectProduct(assetId);
    }
    
    setWindowTitle(tr("Pick %1").arg(name));
    
    m_persistentState = AZ::UserSettings::CreateFind<AzToolsFramework::QWidgetSavedState>(AZ::Crc32(("AssetBrowserTreeView_Dialog_" + name).toUtf8().data()), AZ::UserSettings::CT_GLOBAL);

    QTimer::singleShot(0, this, &AzAssetBrowserDialog::RestoreState);
    SelectionChangedSlot();
}

void AzAssetBrowserDialog::RestoreState()
{
    if (m_persistentState)
    {
        auto widget = parentWidget() ? parentWidget() : this;
        m_persistentState->RestoreGeometry(widget);
    }
}

AzAssetBrowserDialog::~AzAssetBrowserDialog()
{
    if (m_persistentState)
    {
        m_persistentState->CaptureGeometry(parentWidget() ? parentWidget() : this);
    }

    m_ui->m_assetBrowserTreeViewWidget->SaveState();
}

void AzAssetBrowserDialog::keyPressEvent(QKeyEvent* e)
{
    // Until search widget is revised, Return key should not close the dialog,
    // it is used in search widget interaction
    if (e->key() == Qt::Key_Return)
    {
        if (EvaluateSelection())
        {
            QDialog::accept();
        }
    }
    else
    {
        QDialog::keyPressEvent(e);
    }
}

void AzAssetBrowserDialog::reject()
{
    m_selection.GetResults().clear();
    QDialog::reject();
}

bool AzAssetBrowserDialog::EvaluateSelection() const
{
    auto selectedAssets = m_ui->m_assetBrowserTreeViewWidget->GetSelectedAssets();
    // exactly one item must be selected, even if multi-select option is disabled, still good practice to check
    if (selectedAssets.size() < 1)
    {
        return false;
    }

    m_selection.GetResults().clear();

    for (auto entry : selectedAssets)
    {
        m_selection.GetSelectionFilter()->Filter(m_selection.GetResults(), entry);
        if (m_selection.IsValid() && !m_selection.GetMultiselect())
        {
            break;
        }
    }
    return m_selection.IsValid();
}

void AzAssetBrowserDialog::DoubleClickedSlot(const QModelIndex& index)
{
    if (EvaluateSelection())
    {
        QDialog::accept();
    }
}

void AzAssetBrowserDialog::UpdatePreview() const
{
    auto selectedAssets = m_ui->m_assetBrowserTreeViewWidget->GetSelectedAssets();
    if (selectedAssets.size() != 1)
    {
        m_ui->m_previewWidget->hide();
        return;
    }

    m_ui->m_previewWidget->Display(selectedAssets.front());
    m_ui->m_previewWidget->show();
}

void AzAssetBrowserDialog::SelectionChangedSlot()
{
    m_ui->m_buttonBox->buttons().first()->setEnabled(EvaluateSelection());
    UpdatePreview();
}

#include <AzAssetBrowser/AzAssetBrowserDialog.moc>