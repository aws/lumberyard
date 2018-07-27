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

#include "ImportableResourcesWidget.h"
#include "ResourceManagementView.h"
#include "StackResourcesWidget.h"

#include <IEditor.h>

#include <CloudCanvas/ICloudCanvasEditor.h>

#include <aws/core/auth/AWSCredentialsProvider.h>

#include <QDebug>
#include <QLabel>
#include <QHeaderView>
#include <QMenu>
#include <QApplication>
#include <QClipboard>
#include <QRegExp>

#include "DetailWidget/ImportableResourcesWidget.moc"

ImportableResourcesWidget::ImportableResourcesWidget(QString resource_group, QSharedPointer<IAWSImporterModel> importerModel, ResourceManagementView* view)
    : PopupDialogWidget(view)
    , m_importerModel{importerModel}
    , m_resource_group(resource_group)
    , m_listfinished(false)
    , m_view(view)
{
    setObjectName("ResourceImporter");
    setWindowTitle(tr("Import from list"));

    //Create the first row of the window
    CreateRegionRow();

    //Create the second row of the window
    CreateSearchRow();

    //Generate the importable resources table containing resource name and ARN
    CreateResourcesTable();

    GetPrimaryButton()->setText(tr("Configure"));

    GetPrimaryButton()->show();
    EnableConfigureButton(false);
    //Show the import button when some resources are selected
    connect(m_importerModel.data(), &IAWSImporterModel::itemChanged, this, &ImportableResourcesWidget::OnItemsChanged);

    //Keep the list window open so the user can import other resources after the current process finishes
    disconnect(GetPrimaryButton(), &QPushButton::clicked, this, &QDialog::accept);
    connect(GetPrimaryButton(), &QPushButton::clicked, this, &ImportableResourcesWidget::OnPrimaryButtonClick);
    setMinimumSize(GetWidth(m_listTable) * 1.5, GetHeight(m_listTable));
}

void ImportableResourcesWidget::CreateRegionRow()
{
    //Add the labels for the first row
    QHBoxLayout* regionLayout = new QHBoxLayout;
    QLabel* selectLabel = new QLabel("Select resources ");
    selectLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    CreateLoadingLabel();

    regionLayout->addWidget(selectLabel, 1, Qt::AlignLeft);
    regionLayout->addWidget(m_loadingProcess, 1, Qt::AlignRight);
    m_gridLayout.addLayout(regionLayout, m_currentRowNum, 0, Qt::AlignLeft);

    //Add the region combo box    
    m_regionBox = new QComboBox;
    m_regionBox->setObjectName("RegionBox");
    m_regionBox->addItems({ "default", "us-east-1", "us-west-1", "us-west-2", "ap-south-1", "ap-northeast-2",
        "ap-southeast-1", "ap-southeast-2", "ap-northeast-1", "eu-central-1", "eu-west-1", "sa-east-1"});
    m_regionBox->setCurrentIndex(0);
    m_regionBox->setEditable(true);

    QLabel* regionLabel = new QLabel("Region:");
    regionLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QHBoxLayout* regionLayout1 = new QHBoxLayout;
    regionLayout1->addWidget(regionLabel, 0, Qt::AlignLeft);
    regionLayout1->addWidget(m_regionBox, 0, Qt::AlignRight);
    m_gridLayout.addLayout(regionLayout1, m_currentRowNum, 1, Qt::AlignRight);

    ++m_currentRowNum;
}

void ImportableResourcesWidget::CreateSearchRow()
{
    //Add the search box in the second row
    m_searchEdit = new QLineEdit;
    m_searchEdit->setToolTip(tr("Content of the filter"));
    m_searchEdit->setPlaceholderText("Search");
    m_gridLayout.addWidget(m_searchEdit, m_currentRowNum, 0);

    //Add the type button in the second row
    m_typeList = QStringList("All types") << "dynamodb" << "lambda" << "sqs" << "sns" << "s3";

    m_typeButton = new QPushButton{m_typeList[0]};
    m_typeButton->setToolTip("Type of the resource to list");
    m_buttonMenu = new QMenu{};
    for (int typeNum = 1; typeNum < m_typeList.length(); ++typeNum)
        m_buttonMenu->addAction(m_typeList[typeNum]);

    m_typeButton->setMenu(m_buttonMenu);
    m_gridLayout.addWidget(m_typeButton, m_currentRowNum, 1, Qt::AlignRight);

    ++m_currentRowNum;

    connect(m_buttonMenu, &QMenu::triggered, this, &ImportableResourcesWidget::OnTypeButtonClicked);
    connect<void(QComboBox::*)(int)>(m_regionBox, &QComboBox::currentIndexChanged, this, &ImportableResourcesWidget::OnRegionChanged);
    connect(m_searchEdit, &QLineEdit::textEdited, this, &ImportableResourcesWidget::OnFilterEntered);
}

void ImportableResourcesWidget::CreateLoadingLabel()
{
    m_loadingProcess = new QLabel;
    m_loadingProcess->setObjectName("LoadingLabel");
    m_loadingProcess->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    connect(m_importerModel.data(), &IAWSImporterModel::ListFinished, this, &ImportableResourcesWidget::OnListFinished);
}

void ImportableResourcesWidget::ListStart(QLabel* loadingProcess)
{
    loadingProcess->setText("Loading...");
    EnableConfigureButton(false);
    m_regionBox->setDisabled(true);
    m_regionBox->setToolTip(tr("Region cannot be changed when the table is loading"));
}

void ImportableResourcesWidget::OnListFinished()
{
    m_listfinished = true;
    if (ResourceIsChosen())
        EnableConfigureButton(true);
    m_loadingProcess->setText("");
    m_regionBox->setDisabled(false);
    m_regionBox->setToolTip(tr("Region of the resources"));
}

void ImportableResourcesWidget::CreateResourcesTable()
{
    ListStart(m_loadingProcess);
    m_importerModel->ListImportableResources("default");
    //Generate the proxy model for the filter
    m_filterListProxyModel.reset(new QSortFilterProxyModel());
    m_filterListProxyModel->setSourceModel(m_importerModel.data());

    //Generate the table to list all of the importable resources
    m_listTable = new QTableView{this};
    m_listTable->setObjectName("ListTable");
    m_listTable->setModel(m_filterListProxyModel.data());
    m_listTable->verticalHeader()->hide();
    m_listTable->hideColumn(IAWSImporterModel::ResourceColumn);
    m_listTable->hideColumn(IAWSImporterModel::ArnColumn);
    m_listTable->hideColumn(IAWSImporterModel::NameColumn);
    m_listTable->hideColumn(IAWSImporterModel::SelectedStateColumn);
    m_listTable->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
    m_listTable->setWordWrap(true);
    m_listTable->setContextMenuPolicy(Qt::CustomContextMenu);
    m_listTable->setEditTriggers(QTableView::NoEditTriggers);
    m_listTable->setSelectionMode(QTableView::NoSelection);
    m_listTable->setColumnWidth(IAWSImporterModel::TypeColumn, m_listTable->columnWidth(IAWSImporterModel::TypeColumn) * 1.5);

    QHeaderView* headerView = m_listTable->horizontalHeader();
    headerView->setSectionResizeMode(0, QHeaderView::ResizeMode::Stretch);

    AddSpanningWidgetRow(m_listTable);
    
    m_currentState = State::Listing;

    connect(m_listTable, &QTableView::customContextMenuRequested, this, &ImportableResourcesWidget::OnTableContextMenuRequested);
}

void ImportableResourcesWidget::OnTableContextMenuRequested(QPoint pos)
{
    QTableView* currentTable = m_currentState == State::Listing ? m_listTable : m_selectionTable;
    QSharedPointer<QSortFilterProxyModel> currentModel = m_currentState == State::Listing ? m_filterListProxyModel : m_filterConfigurationProxyModel;

    auto index = currentTable->indexAt(pos);
    auto sourceIndex = currentModel->mapToSource(index);
    int row = sourceIndex.row();

    if (!index.isValid())
    {
        return;
    }

    //Create the menu
    auto menu = new ToolTipMenu{};
    if (m_currentState == State::Listing)
    {
        auto copyAction = menu->addAction("Copy");
        connect(copyAction, &QAction::triggered, this, [this, index](){OnMenuCopyClicked(index); });
        auto configureAction = menu->addAction("Configure");
        connect(configureAction, &QAction::triggered, this, [this, row](){OnMenuConfigureClicked(row);});
        if (!m_listfinished)
            configureAction->setDisabled(true);
    }
    else
    {
        auto deleteAction = menu->addAction("Delete");
        connect(deleteAction, &QAction::triggered, this, [this, row](){OnMenuDeleteClicked(row);});
    }

    auto viewResource = menu->addAction("View resource in AWS console");
    QString resourceType = m_importerModel->item(row, IAWSImporterModel::TypeColumn)->text();
    QString physicalResourceId = m_importerModel->item(row, IAWSImporterModel::ResourceColumn)->text();
    QString resourceArn = m_importerModel->item(row, IAWSImporterModel::ArnColumn)->text();
    QString region = resourceType == "AWS::SNS::S3" ? "" : resourceArn.split(":")[3];
    if (resourceType == "AWS::SNS::Topic")
        physicalResourceId = m_importerModel->item(row, IAWSImporterModel::ArnColumn)->text();
    connect(viewResource, &QAction::triggered, this, [this, physicalResourceId, resourceType, region]() {ViewConsoleResource(resourceType, physicalResourceId, region); });

    if (menu && !menu->isEmpty())
    {
        menu->popup(currentTable->viewport()->mapToGlobal(pos));
    }
}

void ImportableResourcesWidget::OnMenuCopyClicked(QModelIndex index)
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(index.data().toString());
}

void ImportableResourcesWidget::OnMenuConfigureClicked(int row)
{    
    //Only import the selected item
    for (int count = 0; count < m_selectedRowList.length(); ++count)
        m_importerModel->item(m_selectedRowList[count], IAWSImporterModel::SelectedStateColumn)->setText("unselected");
    m_selectedRowList.clear();

    m_importerModel->item(row, IAWSImporterModel::NameColumn)->setText("");
    m_importerModel->item(row, IAWSImporterModel::SelectedStateColumn)->setText("selected");
    m_selectedRowList.append(row);
    CreateConfigurationWindow();
    m_importerModel->item(row, IAWSImporterModel::SelectedStateColumn)->setText("unselected");
}

void ImportableResourcesWidget::OnMenuDeleteClicked(int row)
{
    m_selectedRowList.removeOne(row);
    m_importerModel->item(row, IAWSImporterModel::CheckableResourceColumn)->setCheckState(Qt::Unchecked);
    m_importerModel->item(row, IAWSImporterModel::SelectedStateColumn)->setText("unselected");
    if (m_selectedRowList.count() == 0)
        m_dialog->close();
}

void ImportableResourcesWidget::ViewConsoleResource(const QString& resourceType, const QString& resourceName, const QString& region)
{
    QString destString = "https://console.aws.amazon.com/";
    if (resourceType == "AWS::S3::Bucket")
    {
        destString += "s3/?&";
        destString += "bucket=" + resourceName;
    }
    else if (resourceType == "AWS::DynamoDB::Table")
    {
        destString += "dynamodb/home?region=" + region + "#";
        destString += "tables:selected=" + resourceName;
    }
    else if (resourceType == "AWS::Lambda::Function")
    {
        destString += "lambda/home?region=" + region + "#";
        destString += "/functions/" + resourceName;
    }
    else if (resourceType == "AWS::SNS::Topic")
    {
        destString += "sns/home?region=" + region + "#";
        destString += "/topics/" + resourceName;
    }
    else if (resourceType == "AWS::SQS::Queue")
    {
        destString += "sqs/home?region=" + region + "#";
    }

    auto profileName = m_view->GetDefaultProfile();
    EBUS_EVENT(CloudCanvas::CloudCanvasEditorRequestBus, SetCredentials, Aws::MakeShared<Aws::Auth::ProfileConfigFileAWSCredentialsProvider>("AWSManager", profileName.toStdString().c_str(), Aws::Auth::REFRESH_THRESHOLD));
    EBUS_EVENT(CloudCanvas::CloudCanvasEditorRequestBus, ApplyConfiguration);
    GetIEditor()->LaunchAWSConsole(destString);
}

bool ImportableResourcesWidget::ResourceIsChosen()
{
    //Return true if any resource is selected
    for (int row = 0; row < m_importerModel->rowCount(); ++row)
    {
        QStandardItem* resource = m_importerModel->item(row, IAWSImporterModel::CheckableResourceColumn);
        if (resource->checkState() == Qt::Checked)
            return true;
    }

    return false;
}

void ImportableResourcesWidget::OnItemsChanged(QStandardItem* item)
{
    //Show the import button if any resource is chosen
    if (item->checkState() == Qt::Checked && m_listfinished)
    {
        EnableConfigureButton(true);
    }
    else if (!ResourceIsChosen())
    {
        EnableConfigureButton(false);
    }
}

void ImportableResourcesWidget::OnPrimaryButtonClick()
{
    m_selectedRowList.clear();
    //Find all the selected resources and change their states in the model
    for (int row = 0; row < m_importerModel->rowCount(); ++row)
    {
        QStandardItem* resource = m_importerModel->item(row, IAWSImporterModel::CheckableResourceColumn);
        if (resource->checkState() == Qt::Checked)
        {
            m_selectedRowList.append(row);
            //If the resource is chosen, set its name empty and its state selected
            m_importerModel->item(row, IAWSImporterModel::NameColumn)->setText("");
            m_importerModel->item(row, IAWSImporterModel::SelectedStateColumn)->setText("selected");
        }
        else
            m_importerModel->item(row, IAWSImporterModel::SelectedStateColumn)->setText("unselected");
    }
    CreateConfigurationWindow();
}

void ImportableResourcesWidget::CreateConfigurationWindow()
{
    m_currentState = State::Configuring;

    //Ask the user to provide the names used for the resources in the resource group
    m_dialog = new PopupDialogWidget{ this };
    m_dialog->setWindowTitle(tr("Configuration"));
    m_dialog->GetPrimaryButton()->setText(tr("Import"));
    m_dialog->GetPrimaryButton()->show();
    m_dialog->GetPrimaryButton()->setToolTip("Import all the selected resources.");

    //Use the proxy model to filter the selected resources
    m_filterConfigurationProxyModel.reset(new QSortFilterProxyModel());
    m_filterConfigurationProxyModel->setSourceModel(m_importerModel.data());
    m_filterConfigurationProxyModel->setFilterKeyColumn(IAWSImporterModel::SelectedStateColumn);
    m_filterConfigurationProxyModel->setFilterRegExp("^selected$");

    //Show the table of the resources to be imported
    m_selectionTable = new QTableView{};
    m_selectionTable->setObjectName("SelectionTable");
    m_selectionTable->setModel(m_filterConfigurationProxyModel.data());
    m_selectionTable->verticalHeader()->hide();
    m_selectionTable->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
    m_listTable->setWordWrap(true);
    m_selectionTable->setEditTriggers(QTableView::AllEditTriggers);
    m_selectionTable->setSelectionMode(QTableView::NoSelection);
    m_selectionTable->setContextMenuPolicy(Qt::CustomContextMenu);
    m_selectionTable->hideColumn(IAWSImporterModel::CheckableResourceColumn);
    m_selectionTable->hideColumn(IAWSImporterModel::TypeColumn);
    m_selectionTable->hideColumn(IAWSImporterModel::ArnColumn);
    m_selectionTable->hideColumn(IAWSImporterModel::SelectedStateColumn);
    QHeaderView* headerView = m_selectionTable->horizontalHeader();
    headerView->setSectionResizeMode(QHeaderView::ResizeMode::Stretch);

    m_dialog->AddSpanningWidgetRow(m_selectionTable);

    //Set the first row editable
    QModelIndex firstItemIndex = m_filterConfigurationProxyModel->index(0, IAWSImporterModel::NameColumn);
    m_selectionTable->setCurrentIndex(firstItemIndex);
    m_selectionTable->edit(firstItemIndex);
    CheckResourceNameGiven(firstItemIndex, firstItemIndex);
    
    connect(m_selectionTable->selectionModel(), &QItemSelectionModel::currentChanged, this, &ImportableResourcesWidget::CheckResourceNameGiven);
    connect(m_selectionTable, &QTableView::customContextMenuRequested, this, &ImportableResourcesWidget::OnTableContextMenuRequested);
    connect(m_importerModel.data(), &IAWSImporterModel::ImporterOutput, this, &ImportableResourcesWidget::OnImporterOutput);
    connect(m_importerModel.data(), &IAWSImporterModel::dataChanged, this, &ImportableResourcesWidget::CheckResourceNameGiven);

    m_dialog->setMinimumWidth(GetWidth(m_selectionTable));

    if (m_dialog->exec() == QDialog::Accepted)
    {
        ShowImportProcess();
    }

    m_currentState = State::Listing;
}

void ImportableResourcesWidget::CheckResourceNameGiven(QModelIndex current, QModelIndex previous)
{
    //Check whether all the resource names have been given
    bool error = false;
    for (int row = 0; row < m_selectedRowList.length(); ++row)
    {
        QStandardItem* resource = m_importerModel->item(m_selectedRowList[row], IAWSImporterModel::NameColumn);
        if (m_filterConfigurationProxyModel->index(row, IAWSImporterModel::NameColumn) != current && resource->text() == "")
        {
            error = true;
        }

        QStandardItem* resourceType = m_importerModel->item(m_selectedRowList[row], IAWSImporterModel::TypeColumn);
        QString jsonResourceType;

        if (resourceType->text() == "AWS::DynamoDB::Table")
            jsonResourceType = "dynamodb";
        else if (resourceType->text() == "AWS::Lambda::Function")
            jsonResourceType = "lambda";
        else if (resourceType->text() == "AWS::SNS::Topic")
            jsonResourceType = "sns";
        else if (resourceType->text() == "AWS::SQS::Queue")
            jsonResourceType = "sqs";
        else if (resourceType->text() == "AWS::S3::Bucket")
            jsonResourceType = "s3";

        QString regex;
        QString help;
        int minLen;
        if (m_view->GetResourceValidationData(jsonResourceType, "name", regex, help, minLen))
        {
            QRegExp re(regex);
            if (!re.exactMatch(resource->text()))
            {
                resource->setBackground(QBrush(QColor("#300000")));
                resource->setToolTip(QString(help));
                error = true;
            }
            else
            {
                resource->setBackground(QBrush(QColor("#303030")));
            }
        }
    }
    EnableImportButton(!error);
}

void ImportableResourcesWidget::ShowImportProcess()
{
    //Hide the import and cancel buttons
    EnableConfigureButton(false);
    GetCancelButton()->setEnabled(false);

    //Show the progress bar
    m_progress = new QProgressDialog("Importing Resource...", "Cancel", 0, m_selectedRowList.length(), this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    m_progress->setWindowTitle("Importing");
    m_progress->setMinimumDuration(0);
    m_progress->setWindowModality(Qt::WindowModal);
    m_finishedImportRequests = 0;
    m_progress->setValue(m_finishedImportRequests);

    //Start to import the resource
    ImportResource();
}

void ImportableResourcesWidget::ImportResource()
{
    QString resourceName = m_importerModel->item(m_selectedRowList[0], IAWSImporterModel::NameColumn)->text();
    QString resourceArn = m_importerModel->item(m_selectedRowList[0], IAWSImporterModel::ArnColumn)->text();
    m_importerModel->ImportResource(m_resource_group, resourceName, resourceArn);
}

void ImportableResourcesWidget::OnImporterOutput(const QVariant& output, const char* outputType)
{
    if (m_selectedRowList.length() > 0 && output == m_importerModel->item(m_selectedRowList[0], IAWSImporterModel::NameColumn)->text() + " imported successfully")
    {
        //Reset the checkstate for those successfully imported resources
        m_importerModel->item(m_selectedRowList[0], IAWSImporterModel::CheckableResourceColumn)->setCheckState(Qt::Unchecked);
        m_importerModel->item(m_selectedRowList[0], IAWSImporterModel::SelectedStateColumn)->setText("unselected");

        //Remove the first item in the list to get the next resource to import
        m_selectedRowList.removeFirst();

        //Change the progress bar
        if (!m_progress->wasCanceled())
        {
            m_finishedImportRequests++;
            m_progress->setValue(m_finishedImportRequests);

            //Import the next resource if exists
            if (m_selectedRowList.length() > 0)
                ImportResource();
        }
    }

    //If all the resources have been imported successfully, operation is canceled or error occurs, stop
    if (m_listfinished && (m_progress->wasCanceled() || m_selectedRowList.length() <= 0 || QString(outputType) == "error"))
    {
        disconnect(m_importerModel.data(), &IAWSImporterModel::ImporterOutput, this, &ImportableResourcesWidget::OnImporterOutput);
        //Reset the import and cancel button
        if (ResourceIsChosen())
            EnableConfigureButton(true);
        GetCancelButton()->setEnabled(true);

        //If error occurs, show the error in a message box
        if (QString(outputType) == "error")
        {
            m_progress->cancel();
            EnableConfigureButton(true);
            ResourceManagementView::ShowBasicErrorBox("Import Error", output.toString());
        }
    }
}

void ImportableResourcesWidget::OnFilterEntered()
{
    //Get the filter condition
    QString type = m_typeButton->text() == "All types" ? "" : m_typeButton->text();
    QString name = m_searchEdit->text();

    //Set the proxy model to show the corresponding resources
    m_filterListProxyModel->setFilterKeyColumn(IAWSImporterModel::ArnColumn);
    QRegExp myRegExp("^arn:aws:[^:]*" + type + ":[^:]*:[^:]*:.*" + name + "[^:]*$", Qt::CaseInsensitive);
    m_filterListProxyModel->setFilterRegExp(myRegExp);
}

void ImportableResourcesWidget::OnTypeButtonClicked(QAction* action)
{
    //Change the text of the button
    m_typeButton->setText(action->text());

    m_buttonMenu->clear();
    for (int typeNum = 0; typeNum < m_typeList.length(); ++typeNum)
        if (m_typeList[typeNum] != m_typeButton->text())
            m_buttonMenu->addAction(m_typeList[typeNum]);

    //Clear the search box and reset the model
    m_searchEdit->clear();
    OnFilterEntered();
}

void ImportableResourcesWidget::OnRegionChanged()
{
    //Clear the search box and reset the model
    m_searchEdit->clear();
    m_listfinished = false;
    ListStart(m_loadingProcess);
    m_importerModel->ListImportableResources(m_regionBox->currentText());
    OnFilterEntered();
}

int ImportableResourcesWidget::GetWidth(QTableView* table)
{
    //Get the width of the table
    int width = table->frameWidth() * 2 +
        table->verticalHeader()->width() +
        table->horizontalHeader()->length() +
        table->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    return width;
}

int ImportableResourcesWidget::GetHeight(QTableView* table)
{
    //Get the height of the table
    int height = table->horizontalHeader()->width() +
        table->frameWidth() * 2 +
        table->verticalHeader()->length();
    return height;
}

void ImportableResourcesWidget::EnableConfigureButton(bool enable)
{
    GetPrimaryButton()->setEnabled(enable);
    if (enable)
        GetPrimaryButton()->setToolTip("Configure the selected resources for import.");
    else
        GetPrimaryButton()->setToolTip("Select one or more resources after all the importable resources are listed.");
}

void ImportableResourcesWidget::EnableImportButton(bool enable)
{
    m_dialog->GetPrimaryButton()->setEnabled(enable);
    if (enable)
        m_dialog->GetPrimaryButton()->setToolTip("Import all the selected resources");
    else
        m_dialog->GetPrimaryButton()->setToolTip("Provide names for all the resources.");
}