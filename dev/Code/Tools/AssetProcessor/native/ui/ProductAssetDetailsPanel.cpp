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

#include "ProductAssetDetailsPanel.h"

#include "AssetTreeFilterModel.h"
#include "GoToButton.h"
#include "ProductAssetTreeItemData.h"

#include <AssetDatabase/AssetDatabase.h>

#include <native/ui/ui_GoToButton.h>
#include <native/ui/ui_ProductAssetDetailsPanel.h>
#include <QDateTime>
#include <QHBoxLayout>
#include <QItemSelection>

namespace AssetProcessor
{

    ProductAssetDetailsPanel::ProductAssetDetailsPanel(QWidget* parent) : AssetDetailsPanel(parent), m_ui(new Ui::ProductAssetDetailsPanel)
    {
        m_ui->setupUi(this);
        m_ui->scrollAreaWidgetContents->setLayout(m_ui->scrollableVerticalLayout);
        ResetText();
    }

    ProductAssetDetailsPanel::~ProductAssetDetailsPanel()
    {

    }

    void ProductAssetDetailsPanel::AssetDataSelectionChanged(const QItemSelection& selected, const QItemSelection& /*deselected*/)
    {
        // Even if multi-select is enabled, only display the first selected item.
        if (selected.indexes().count() == 0 || !selected.indexes()[0].isValid())
        {
            ResetText();
            return;
        }

        QModelIndex productModelIndex = m_productFilterModel->mapToSource(selected.indexes()[0]);

        if (!productModelIndex.isValid())
        {
            return;
        }
        AssetTreeItem* childItem = static_cast<AssetTreeItem*>(productModelIndex.internalPointer());

        const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData = AZStd::rtti_pointer_cast<const ProductAssetTreeItemData>(childItem->GetData());

        m_ui->assetNameLabel->setText(childItem->GetData()->m_name);

        if (childItem->GetData()->m_isFolder || !productItemData)
        {
            // Folders don't have details.
            SetDetailsVisible(false);
            return;
        }
        SetDetailsVisible(true);

        AssetDatabaseConnection assetDatabaseConnection;
        assetDatabaseConnection.OpenDatabase();

        AZ::Data::AssetId assetId;

        assetDatabaseConnection.QuerySourceByProductID(
            productItemData->m_databaseInfo.m_productID,
            [&](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry)
        {
            assetId = AZ::Data::AssetId(sourceEntry.m_sourceGuid, productItemData->m_databaseInfo.m_subID);
            // Use a decimal value to display the sub ID and not hex. Lumberyard is not consistent about
            // how sub IDs are displayed, so it's important to double check what format a sub ID is in before using it elsewhere.
            m_ui->productAssetIdValueLabel->setText(assetId.ToString<AZStd::string>(AZ::Data::AssetId::SubIdDisplayType::Decimal).c_str());

            // Make sure this is the only connection to the button.
            m_ui->gotoAssetButton->m_ui->goToPushButton->disconnect();

            connect(m_ui->gotoAssetButton->m_ui->goToPushButton, &QPushButton::clicked, [=] {
                GoToSource(sourceEntry.m_sourceName);
            });

            m_ui->sourceAssetValueLabel->setText(sourceEntry.m_sourceName.c_str());
            return true;
        });

        AZStd::string platform;

        assetDatabaseConnection.QueryJobByProductID(
            productItemData->m_databaseInfo.m_productID,
            [&](AzToolsFramework::AssetDatabase::JobDatabaseEntry& jobEntry)
        {
            QDateTime lastTimeProcessed = QDateTime::fromMSecsSinceEpoch(jobEntry.m_lastLogTime);
            m_ui->lastTimeProcessedValueLabel->setText(lastTimeProcessed.toString());

            m_ui->jobKeyValueLabel->setText(jobEntry.m_jobKey.c_str());
            platform = jobEntry.m_platform;
            m_ui->platformValueLabel->setText(jobEntry.m_platform.c_str());
            return true;
        });

        BuildOutgoingProductDependencies(assetDatabaseConnection, productItemData, platform);
        BuildincomingProductDependencies(assetDatabaseConnection, productItemData, assetId, platform);
    }

    void ProductAssetDetailsPanel::BuildOutgoingProductDependencies(
        AssetDatabaseConnection& assetDatabaseConnection,
        const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData,
        const AZStd::string& platform)
    {
        // Clear & ClearContents leave the table dimensions the same, so set rowCount to zero to reset it.
        m_ui->outgoingProductDependenciesTable->setRowCount(0);

        m_ui->outgoingUnmetPathProductDependenciesList->clear();
        int productDependencyCount = 0;
        int productPathDependencyCount = 0;
        assetDatabaseConnection.QueryProductDependencyByProductId(
            productItemData->m_databaseInfo.m_productID,
            [&](AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry& dependency)
        {
            if (!dependency.m_dependencySourceGuid.IsNull())
            {
                assetDatabaseConnection.QueryProductBySourceGuidSubID(
                    dependency.m_dependencySourceGuid,
                    dependency.m_dependencySubID,
                    [&](AzToolsFramework::AssetDatabase::ProductDatabaseEntry& product)
                {
                    bool platformMatches = false;

                    assetDatabaseConnection.QueryJobByJobID(
                        product.m_jobPK,
                        [&](AzToolsFramework::AssetDatabase::JobDatabaseEntry& jobEntry)
                    {
                        if (platform.compare(jobEntry.m_platform) == 0)
                        {
                            platformMatches = true;
                        }
                        return true;
                    });

                    if (platformMatches)
                    {
                        m_ui->outgoingProductDependenciesTable->insertRow(productDependencyCount);

                        // Qt handles cleanup automatically, setting this as the parent means
                        // when this panel is torn down, these widgets will be destroyed.
                        GoToButton* rowGoToButton = new GoToButton(this);
                        connect(rowGoToButton->m_ui->goToPushButton, &QPushButton::clicked, [=] {
                            GoToProduct(product.m_productName);
                        });
                        m_ui->outgoingProductDependenciesTable->setCellWidget(productDependencyCount, 0, rowGoToButton);

                        QTableWidgetItem* rowName = new QTableWidgetItem(product.m_productName.c_str());
                        m_ui->outgoingProductDependenciesTable->setItem(productDependencyCount, 1, rowName);
                        ++productDependencyCount;
                    }
                    return true;
                });
            }

            // If there is both a path and an asset ID on this dependency, then something has gone wrong.
            // Other tooling should have reported this error. In the UI, show both the asset ID and path.
            if (!dependency.m_unresolvedPath.empty())
            {
                QListWidgetItem* listWidgetItem = new QListWidgetItem();
                listWidgetItem->setText(dependency.m_unresolvedPath.c_str());
                m_ui->outgoingUnmetPathProductDependenciesList->addItem(listWidgetItem);
                ++productPathDependencyCount;
            }
            return true;
        });
        m_ui->outgoingProductDependenciesValueLabel->setText(QString::number(productDependencyCount));
        m_ui->outgoingUnmetPathProductDependenciesValueLabel->setText(QString::number(productPathDependencyCount));

        if (productDependencyCount == 0)
        {
            m_ui->outgoingProductDependenciesTable->insertRow(productDependencyCount);
            QTableWidgetItem* rowName = new QTableWidgetItem(tr("No product dependencies"));
            m_ui->outgoingProductDependenciesTable->setItem(productDependencyCount, 1, rowName);
            ++productDependencyCount;
        }

        if (productPathDependencyCount == 0)
        {
            QListWidgetItem* listWidgetItem = new QListWidgetItem();
            listWidgetItem->setText(tr("No unmet dependencies"));
            m_ui->outgoingUnmetPathProductDependenciesList->addItem(listWidgetItem);
            ++productPathDependencyCount;
        }

        m_ui->outgoingProductDependenciesTable->setMinimumHeight(m_ui->outgoingProductDependenciesTable->rowHeight(0) * productDependencyCount + 2 * m_ui->outgoingProductDependenciesTable->frameWidth());
        m_ui->outgoingProductDependenciesTable->adjustSize();

        m_ui->outgoingUnmetPathProductDependenciesList->setMinimumHeight(m_ui->outgoingUnmetPathProductDependenciesList->sizeHintForRow(0) * productPathDependencyCount + 2 * m_ui->outgoingUnmetPathProductDependenciesList->frameWidth());
        m_ui->outgoingUnmetPathProductDependenciesList->adjustSize();
    }

    void ProductAssetDetailsPanel::BuildincomingProductDependencies(
        AssetDatabaseConnection& assetDatabaseConnection,
        const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData,
        const AZ::Data::AssetId& assetId,
        const AZStd::string& platform)
    {
        // Clear & ClearContents leave the table dimensions the same, so set rowCount to zero to reset it.
        m_ui->incomingProductDependenciesTable->setRowCount(0);

        int incomingProductDependencyCount = 0;
        assetDatabaseConnection.QueryDirectReverseProductDependenciesBySourceGuidSubId(
            assetId.m_guid,
            assetId.m_subId,
            [&](AzToolsFramework::AssetDatabase::ProductDatabaseEntry& incomingDependency)
        {
            bool platformMatches = false;

            assetDatabaseConnection.QueryJobByJobID(
                incomingDependency.m_jobPK,
                [&](AzToolsFramework::AssetDatabase::JobDatabaseEntry& jobEntry)
            {
                if (platform.compare(jobEntry.m_platform) == 0)
                {
                    platformMatches = true;
                }
                return true;
            });
            if (platformMatches)
            {
                m_ui->incomingProductDependenciesTable->insertRow(incomingProductDependencyCount);

                GoToButton* rowGoToButton = new GoToButton(this);
                connect(rowGoToButton->m_ui->goToPushButton, &QPushButton::clicked, [=] {
                    GoToProduct(incomingDependency.m_productName);
                });
                m_ui->incomingProductDependenciesTable->setCellWidget(incomingProductDependencyCount, 0, rowGoToButton);

                QTableWidgetItem* rowName = new QTableWidgetItem(incomingDependency.m_productName.c_str());
                m_ui->incomingProductDependenciesTable->setItem(incomingProductDependencyCount, 1, rowName);

                ++incomingProductDependencyCount;
            }
            return true;
        });

        m_ui->incomingProductDependenciesValueLabel->setText(QString::number(incomingProductDependencyCount));

        if (incomingProductDependencyCount == 0)
        {
            m_ui->incomingProductDependenciesTable->insertRow(incomingProductDependencyCount);
            QTableWidgetItem* rowName = new QTableWidgetItem(tr("No incoming product dependencies"));
            m_ui->incomingProductDependenciesTable->setItem(incomingProductDependencyCount, 1, rowName);
            ++incomingProductDependencyCount;
        }
        m_ui->incomingProductDependenciesTable->setMinimumHeight(m_ui->incomingProductDependenciesTable->rowHeight(0) * incomingProductDependencyCount + 2 * m_ui->incomingProductDependenciesTable->frameWidth());
        m_ui->incomingProductDependenciesTable->adjustSize();
    }

    void ProductAssetDetailsPanel::ResetText()
    {
        m_ui->assetNameLabel->setText(tr("Select an asset to see details"));
        SetDetailsVisible(false);
    }

    void ProductAssetDetailsPanel::SetDetailsVisible(bool visible)
    {
        // The folder selected description has opposite visibility from everything else.
        m_ui->folderSelectedDescription->setVisible(!visible);
        m_ui->productAssetIdTitleLabel->setVisible(visible);
        m_ui->productAssetIdValueLabel->setVisible(visible);

        m_ui->lastTimeProcessedTitleLabel->setVisible(visible);
        m_ui->lastTimeProcessedValueLabel->setVisible(visible);

        m_ui->jobKeyTitleLabel->setVisible(visible);
        m_ui->jobKeyValueLabel->setVisible(visible);

        m_ui->platformTitleLabel->setVisible(visible);
        m_ui->platformValueLabel->setVisible(visible);

        m_ui->sourceAssetTitleLabel->setVisible(visible);
        m_ui->sourceAssetValueLabel->setVisible(visible);
        m_ui->gotoAssetButton->setVisible(visible);

        m_ui->outgoingProductDependenciesTitleLabel->setVisible(visible);
        m_ui->outgoingProductDependenciesValueLabel->setVisible(visible);
        m_ui->outgoingProductDependenciesTable->setVisible(visible);

        m_ui->outgoingUnmetPathProductDependenciesTitleLabel->setVisible(visible);
        m_ui->outgoingUnmetPathProductDependenciesValueLabel->setVisible(visible);
        m_ui->outgoingUnmetPathProductDependenciesList->setVisible(visible);

        m_ui->incomingProductDependenciesTitleLabel->setVisible(visible);
        m_ui->incomingProductDependenciesValueLabel->setVisible(visible);
        m_ui->incomingProductDependenciesTable->setVisible(visible);

        m_ui->DependencySeparatorLine->setVisible(visible);
    }
}

#include <native/ui/ProductAssetDetailsPanel.moc>
