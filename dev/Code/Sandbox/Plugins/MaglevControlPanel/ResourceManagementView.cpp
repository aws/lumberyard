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
#include <stdafx.h>

#include "EditorCoreAPI.h"

#include <AWSUtil.h>
#include <ResourceManagementView.h>
#include <ActiveDeployment.h>
#include <AWSResourceManager.h>
#include <InitializeCloudCanvasProject.h>
#include <RoleSelectionView.h>
#include <QAWSCredentialsEditor.h>
#include <MaglevControlPanelPlugin.h>
#include <AWSProjectModel.h>

#include "AddDynamoDBTable.h"
#include "AddLambdaFunction.h"
#include "AddS3Bucket.h"
#include "AddSNSTopic.h"
#include "AddSQSQueue.h"

#include <QCheckBox>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenuBar>
#include <QPushButton>
#include <QSplitter>
#include <QToolBar>
#include <QTreeView>
#include <QJsonDocument>
#include <QJsonParseError>

#include <QPair>

#include "FilePathLabel.h"

#include "DetailWidget/CloudFormationTemplateDetailWidget.h"
#include "DetailWidget/CloudFormationTemplateResourceDetailWidget.h"
#include "DetailWidget/CodeDirectoryDetailWidget.h"
#include "DetailWidget/DeploymentListDetailWidget.h"
#include "DetailWidget/DeploymentDetailWidget.h"
#include "DetailWidget/EmptyDetailWidget.h"
#include "DetailWidget/PopupDialogWidget.h"
#include "DetailWidget/ResourceGroupListDetailWidget.h"
#include "DetailWidget/ResourceGroupDetailWidget.h"
#include "DetailWidget/FileContentDetailWidget.h"
#include "DetailWidget/ProjectDetailWidget.h"
#include "DetailWidget/ValidatingLineEditWidget.h"
#include "DetailWidget/ImportableResourcesWidget.h"
#include "DetailWidget/MaximumSizedTableView.h"

#include <ResourceManagementView.moc>
#include <DetailWidget/TextDetailWidget.moc>
#include <DetailWidget/FileContentDetailWidget.moc>

#include <AzQtComponents/Components/StyleManager.h>

const int ResourceManagementView::SEARCH_LIGHTER_VALUE {
    70
};                                                           // How much lighter should the block around search text appear - 150 is pretty much saturated
const int ResourceManagementView::DETAIL_STRETCH_FACTOR = 1;
const int ResourceManagementView::TREE_STRETCH_FACTOR = 0;
const int ResourceManagementView::CREATE_DEPLOYMENT_DIALOG_MINIMUM_HEIGHT = 380;
const int ResourceManagementView::CREATE_DEPLOYMENT_DIALOG_MINIMUM_WIDTH = 350;

static const QString CREATE_RESOURCE_DIALOG_NAME = "CreateResourceDialog";

ResourceManagementView::ResourceManagementView(QWidget* parent)
    : QMainWindow(parent)
    , m_importerModel{GetIEditor()->GetAWSResourceManager()->GetImporterModel()}
    , m_projectModel{GetIEditor()->GetAWSResourceManager()->GetProjectModel()}
    , m_deploymentModel{GetIEditor()->GetAWSResourceManager()->GetDeploymentModel()}
    , m_profileModel{GetIEditor()->GetAWSResourceManager()->GetProfileModel()}
    , m_resourceManager{GetIEditor()->GetAWSResourceManager()}
{
    // for debugging layout problems:
    //setStyleSheet("border: 1px solid red");

    setObjectName("ResourceManagement");
    AzQtComponents::StyleManager::setStyleSheet(this, "style:CloudCanvas.qss");

    Init();

    connect(m_resourceManager, &IAWSResourceManager::ProjectInitializedChanged, this, &ResourceManagementView::OnProjectInitializedChanged);
    connect(m_profileModel.data(), &IAWSProfileModel::modelReset, this, &ResourceManagementView::OnProfileModelReset);
    connect(m_deploymentModel.data(), &IAWSDeploymentModel::modelReset, this, &ResourceManagementView::OnDeploymentModelReset);
    connect(m_projectModel.data(), &IAWSProjectModel::ResourceGroupAdded, this, &ResourceManagementView::OnResourceGroupAdded);
    connect(m_projectModel.data(), &IAWSProjectModel::DeploymentAdded, this, &ResourceManagementView::OnDeploymentAdded);
    connect(m_projectModel.data(), &IAWSProjectModel::PathAdded, this, &ResourceManagementView::OnPathAdded);
    connect(m_resourceManager, &IAWSResourceManager::OperationInProgressChanged, this, &ResourceManagementView::OnOperationInProgressChanged);
    OnProjectInitializedChanged();

    m_treeView->setCurrentIndex(m_projectModel->ResourceGroupListIndex());

    LoadResourceValidationData();
}

void ResourceManagementView::LoadResourceValidationData()
{
    CCryFile dataFile;
    const char* filename = "@engroot@/Gems/CloudGemFramework/v1/ResourceManager/resource_manager/config/aws_name_validation_rules.json";
    if (!dataFile.Open(filename, "r"))
    {
        AZ_Error("ResourceManager", false, "Unable to open Cloud Canvas resource validation data file: %s", filename);
        return;
    }

    size_t fileLen = dataFile.GetLength();
    if (fileLen <= 0)
    {
        AZ_Error("ResourceManager", false, "Cloud Canvas resource validation data file is empty %s", filename);
        return;
    }

    std::unique_ptr<char[]> fileContents(new char[fileLen + 1]);
    fileContents[fileLen] = '\0';

    if (dataFile.ReadRaw(fileContents.get(), fileLen) != fileLen)
    {
        AZ_Error("ResourceManager", false, "Unable to read Cloud Canvas validation data file %s", filename);
        return;
    }

    QJsonParseError error;
    auto document = QJsonDocument::fromJson(fileContents.get(), &error);

    if (error.error != QJsonParseError::NoError)
    {
        AZ_Error("ResourceManager", false, "Error parsing Cloud Canvas validation file %s: %s", filename, error.errorString().toUtf8().constData());
        return;
    }

    m_resourceValidation = document.object();
}

void ResourceManagementView::SendUpdatedSourceStatus(const QString& filePath)
{
    if (m_projectModel)
    {
        m_projectModel->FileSourceStatusUpdated(filePath);
    }
}
bool ResourceManagementView::GetResourceValidationData(const QString& resourceType, const QString& resourceField, QString& outRegEx, QString& outHelp, int& outMinLen)
{
    // if can't find type or field, return false
    if (!m_resourceValidation.contains(resourceType))
    {
        AZ_Error("ResourceManager", false, "Cloud Canvas validation data doesn't contain information for resource type %s", resourceType.toUtf8().constData());
        return false;
    }

    QJsonObject typeInfo = m_resourceValidation[resourceType].toObject();

    if (!typeInfo.contains(resourceField))
    {
        AZ_Error("ResourceManager", false, "Cloud Canvas validation data doesn't contain inform for resource field %s in resource type %s", resourceField.toUtf8().constData(), resourceType.toUtf8().constData());
        return false;
    }

    QJsonObject fieldInfo = typeInfo[resourceField].toObject();

    static const QString REGEX_FIELD("regex");
    static const QString HELP_FIELD("help");
    static const QString MINLENGTH_FIELD("min_length");

    QJsonValue regEx = fieldInfo[REGEX_FIELD];
    if (regEx.isUndefined() || !regEx.isString())
    {
        AZ_Warning("ResourceManager", false, "regex string not found in Cloud canvas validation info for resource field %s in resource type %s", resourceField.toUtf8().constData(), resourceType.toUtf8().constData());
    }
    else
    {
        outRegEx = regEx.toString();
    }

    QJsonValue help = fieldInfo[HELP_FIELD];
    if (help.isUndefined() || !help.isString())
    {
        AZ_Warning("ResourceManager", false, "help string not found in Cloud canvas validation info for resource field %s in resource type %s", resourceField.toUtf8().constData(), resourceType.toUtf8().constData());
    }
    else
    {
        outHelp = help.toString();
    }

    QJsonValue minLen = fieldInfo[MINLENGTH_FIELD];
    outMinLen = minLen.toInt(1);    // no need to warn here, most won't have this

    return true;
}


void ResourceManagementView::OnResourceHelpActivated(const QString& linkString)
{
    QDesktopServices::openUrl(linkString);
}

// Full initialization/redraw of the view pane
void ResourceManagementView::Init()
{
    setWindowTitle(QString(GetWindowTitle()));

    // Our overall top to bottom page layout
    // vertical layout will be used throughout our individual "row" calls
    QVBoxLayout* verticalLayout = new QVBoxLayout;
    verticalLayout->setContentsMargins(0, 0, 0, 0);
    verticalLayout->setSpacing(0);

    QWidget* mainWidget = new QWidget(this);
    mainWidget->setLayout(verticalLayout);

    // Row 1
    verticalLayout->addWidget(CreateMenuBar());

    // AddHorizontalSpacer(verticalLayout);

    //Row 2
    CreateButtonRow(verticalLayout);

    // AddHorizontalSpacer(verticalLayout);

    // Row 3
    CreateBottomRow(verticalLayout);

    setCentralWidget(mainWidget);

    CreateSearchFrame();
    SetParentFrame(m_emptyDetailWidget);
    HideSearchFrame();
}

QMenuBar* ResourceManagementView::CreateMenuBar()
{
    QMenuBar* menuBar = new QMenuBar(this);
    //menuBar->setStyleSheet("border: 1px solid orange");

    // File Menu
    QMenu* fileMenu = menuBar->addMenu("File");

    m_menuSave = fileMenu->addAction("Save file");
    m_menuSaveAs = fileMenu->addAction("Save file as...");
    m_menuSaveAs->setVisible(false); // Until implemented


    // Edit Menu
    QMenu* editMenu = menuBar->addMenu("Edit");
    m_menuCut = editMenu->addAction("Cut", nullptr, nullptr, QKeySequence::Cut);
    m_menuCopy = editMenu->addAction("Copy", nullptr, nullptr, QKeySequence::Copy);
    m_menuPaste = editMenu->addAction("Paste", nullptr, nullptr, QKeySequence::Paste);

    editMenu->addSeparator();
    m_menuUndo = editMenu->addAction("Undo", nullptr, nullptr, QKeySequence::Undo);
    m_menuRedo = editMenu->addAction("Redo", nullptr, nullptr, QKeySequence::Redo);

    editMenu->addSeparator();
    m_menuSearch = editMenu->addAction("Search", nullptr, nullptr, QKeySequence::Find);

    m_findNextShortcut = editMenu->addAction("Find Next", nullptr, nullptr, QKeySequence(Qt::Key_F3));
    m_findPrevShortcut = editMenu->addAction("Find Previous", nullptr, nullptr, QKeySequence(Qt::SHIFT + Qt::Key_F3));
    m_hideSearchShortcut = editMenu->addAction("Hide Search", nullptr, nullptr, QKeySequence(Qt::Key_Escape));
    m_saveShortcut = editMenu->addAction("Save", nullptr, nullptr, QKeySequence(Qt::CTRL + Qt::Key_S));

    // add actions to the containing widget so that they get matched when checking for keyboard shortcuts
    addAction(m_menuSearch);
    addAction(m_findNextShortcut);
    addAction(m_findPrevShortcut);
    addAction(m_hideSearchShortcut);
    addAction(m_saveShortcut);

    QMenu* helpMenu = menuBar->addMenu("Help");

    QAction* cloudCanvasHelp = helpMenu->addAction("Cloud Canvas documentation");
    QWidget::connect(cloudCanvasHelp, &QAction::triggered, this, &ResourceManagementView::OnMenuCloudCanvasHelp);

    QAction* cloudCanvasForum = helpMenu->addAction("Cloud Canvas forum");
    QWidget::connect(cloudCanvasForum, &QAction::triggered, this, [](bool checked) { QDesktopServices::openUrl(QUrl(QString("https://gamedev.amazon.com/forums/spaces/133/cloud-canvas.html"))); });

    return menuBar;
}

void ResourceManagementView::CreateStatusLabels(QHBoxLayout* horizontalLayout)
{
    m_currentProfileLabel = new QLabel(GetProfileLabel());
    m_currentProfileLabel->setObjectName("CurrentProfile");
    m_currentProfileLabel->setTextFormat(Qt::AutoText);
    m_currentProfileLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    horizontalLayout->addWidget(m_currentProfileLabel, 0, Qt::AlignRight | Qt::AlignVCenter);
    QWidget::connect(m_currentProfileLabel, &QLabel::linkActivated, this, &ResourceManagementView::OnCurrentProfileClicked);

    AddVerticalSpacer(horizontalLayout);

    m_currentDeploymentLabel = new QLabel(GetDeploymentLabel());
    m_currentDeploymentLabel->setObjectName("CurrentDeployment");
    m_currentDeploymentLabel->setTextFormat(Qt::AutoText);
    m_currentDeploymentLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    horizontalLayout->addWidget(m_currentDeploymentLabel, 0, Qt::AlignRight | Qt::AlignVCenter);
    QWidget::connect(m_currentDeploymentLabel, &QLabel::linkActivated, this, &ResourceManagementView::OnCurrentDeploymentClicked);
}

// Standardized horizontal spacer scaled appropriately used to separate our 3 major rows
void ResourceManagementView::AddHorizontalSpacer(QVBoxLayout* verticalLayout)
{
    // Add a horizontal spacer
    QFrame* fileSpacer = new QFrame;
    //fileSpacer->setStyleSheet("border: 1px solid green");
    fileSpacer->setFrameShape(QFrame::HLine);
    fileSpacer->setFrameShadow(QFrame::Sunken);
    verticalLayout->addWidget(fileSpacer);
}

// Create our Second "button" row - Save/Delete/Check out/Create new/Update/Validate template
void ResourceManagementView::CreateButtonRow(QVBoxLayout* verticalLayout)
{
    auto widget = new QFrame {};
    widget->setObjectName("MainToolbar");

    QHBoxLayout* horizontalLayout = new QHBoxLayout;
    widget->setLayout(horizontalLayout);

    horizontalLayout->setSpacing(0);
    horizontalLayout->setMargin(0);

    AddToolBar(horizontalLayout);

    horizontalLayout->addStretch();

    CreateStatusLabels(horizontalLayout);

    verticalLayout->addWidget(widget);
}

// Create our Second "button" row - Save/Delete/Check out/Create new/Update/Validate template
void ResourceManagementView::AddToolBar(QHBoxLayout* horizontalLayout)
{
    m_newButton = new ToolbarButton {};
    m_newButton->setObjectName("AddNewButton");
    m_newButton->setText(QString::fromStdWString(L"Add new \u25BC"));
    QWidget::connect(m_newButton, &ToolbarButton::clicked, this, &ResourceManagementView::OnNewButtonClicked);
    horizontalLayout->addWidget(m_newButton);

    AddVerticalSpacer(horizontalLayout);

    m_newMenu = new ToolTipMenu {
        "Add new"
    };

    m_menuNewResource = m_newMenu->addAction("Resource");

    m_menuNewResourceGroup = m_newMenu->addAction("Resource group...");
    m_menuNewResourceGroup->setToolTip(
        "Define a set of related resources that are needed by the Lumberyard project.");
    QWidget::connect(m_menuNewResourceGroup, &QAction::triggered, this, &ResourceManagementView::OnMenuNewResourceGroup);

    m_menuNewDeployment = m_newMenu->addAction("Deployment...");
    m_menuNewDeployment->setToolTip(
        "Create an AWS CloudFormation stack for a new deployment.");
    QWidget::connect(m_menuNewDeployment, &QAction::triggered, this, &ResourceManagementView::OnMenuNewDeployment);

    m_newMenu->addSeparator();

    m_menuNewFile = m_newMenu->addAction("File");
    m_menuNewFile->setToolTip(
        "Add a new file to the selected code directory.");

    m_menuNewDirectory = m_newMenu->addAction("Directory");
    m_menuNewDirectory->setToolTip(
        "Add a new directory to the selected code directory.");

    m_saveButton = new ToolbarButton;
    m_saveButton->setObjectName("SaveButton");
    QIcon saveIcon;
    saveIcon.addPixmap(QPixmap {"Editor/Icons/CloudCanvas/save_enabled.png"}, QIcon::Active);
    saveIcon.addPixmap(QPixmap {"Editor/Icons/CloudCanvas/save_disabled.png"}, QIcon::Disabled);
    m_saveButton->setIcon(saveIcon);
    horizontalLayout->addWidget(m_saveButton);
    DisableSaveButton(tr("Selected item cannot be saved."));

    m_deleteButton = new ToolbarButton;
    QIcon deleteIcon;
    deleteIcon.addPixmap(QPixmap {"Editor/Icons/CloudCanvas/delete_enabled.png"}, QIcon::Active);
    deleteIcon.addPixmap(QPixmap {"Editor/Icons/CloudCanvas/delete_disabled.png"}, QIcon::Disabled);
    m_deleteButton->setIcon(deleteIcon);
    m_deleteButton->setObjectName("DeleteButton");
    horizontalLayout->addWidget(m_deleteButton);
    DisableDeleteButton(tr("Selected item cannot be deleted."));

    AddVerticalSpacer(horizontalLayout);

    m_sourceControlButton = new ToolbarButton;
    m_sourceControlButton->setObjectName("SourceControlButton");
    horizontalLayout->addWidget(m_sourceControlButton);
    SetSourceControlState(SourceControlState::NOT_APPLICABLE);

    m_validateTemplateButton = new ToolbarButton;
    m_validateTemplateButton->setText("Validate template");
    m_validateTemplateButton->setVisible(false); // until implemented
    horizontalLayout->addWidget(m_validateTemplateButton);
}

void ResourceManagementView::SetSourceControlState(SourceControlState newState, const QString& tooltipOverride)
{
    m_sourceControlButton->setHidden(false);

    switch (newState)
    {
    case SourceControlState::QUERYING:
    {
        m_sourceControlButton->setEnabled(false);
        m_sourceControlButton->setIcon(QIcon("Editor/Icons/CloudCanvas/source_control_disabled.png"));
        m_sourceControlButton->setToolTip(tooltipOverride.length() ? tooltipOverride : "Querying source control.");
    }
    break;
    case SourceControlState::UNAVAILABLE_CHECK_OUT:
    {
        m_sourceControlButton->setEnabled(false);
        m_sourceControlButton->setIcon(QIcon("Editor/Icons/CloudCanvas/source_control_disabled.png"));
        m_sourceControlButton->setToolTip(tooltipOverride.length() ? tooltipOverride : "Source control is unavailable.");
    }
    break;
    case SourceControlState::ENABLED_CHECK_OUT:
    {
        m_sourceControlButton->setEnabled(true);
        m_sourceControlButton->setIcon(QIcon("Editor/Icons/CloudCanvas/check_out_icon.png"));
        m_sourceControlButton->setToolTip(tooltipOverride.length() ? tooltipOverride : "Check file out of source control.");
    }
    break;
    case SourceControlState::DISABLED_CHECK_IN:
    {
        m_sourceControlButton->setEnabled(false);
        m_sourceControlButton->setIcon(QIcon("Editor/Icons/CloudCanvas/check_out_icon.png"));
        m_sourceControlButton->setToolTip(tooltipOverride.length() ? tooltipOverride : "This file is already editable. Submit changes in your source control tool.");
    }
    break;
    case SourceControlState::DISABLED_ADD:
    {
        m_sourceControlButton->setEnabled(false);
        m_sourceControlButton->setIcon(QIcon("Editor/Icons/CloudCanvas/add_to_source_control.png"));
        m_sourceControlButton->setToolTip(tooltipOverride.length() ? tooltipOverride : "This file is already editable. Submit changes in your source control tool.");
    }
    break;
    case SourceControlState::ENABLED_ADD:
    {
        m_sourceControlButton->setEnabled(true);
        m_sourceControlButton->setIcon(QIcon("Editor/Icons/CloudCanvas/add_to_source_control.png"));
        m_sourceControlButton->setToolTip(tooltipOverride.length() ? tooltipOverride : "Add file to source control.");
    }
    break;
    case SourceControlState::NOT_APPLICABLE:
    {
        m_sourceControlButton->setHidden(true);
    }
    break;
    }
}

QPushButton* ResourceManagementView::EnableSaveButton(const QString& toolTip)
{
    m_saveButton->setToolTip(toolTip);
    m_saveButton->setEnabled(true);
    return m_saveButton;
}

void ResourceManagementView::DisableSaveButton(const QString& toolTip)
{
    m_saveButton->setToolTip(toolTip);
    m_saveButton->setDisabled(true);
}

QPushButton* ResourceManagementView::GetDeleteButton() const
{
    return m_deleteButton;
}

QPushButton* ResourceManagementView::EnableDeleteButton(const QString& toolTip)
{
    m_deleteButton->setToolTip(toolTip);
    m_deleteButton->setEnabled(true);
    return m_deleteButton;
}

void ResourceManagementView::DisableDeleteButton(const QString& toolTip)
{
    m_deleteButton->setToolTip(toolTip);
    m_deleteButton->setDisabled(true);
}

void ResourceManagementView::ShowSearchFrame()
{
    m_searchFrame->setHidden(false);
    if (m_searchFrame->parentWidget())
    {
        OnTextEditResized(m_searchFrame->parentWidget()->size());
    }
}

void ResourceManagementView::SetParentFrame(QWidget* parentBox)
{
    if (!parentBox)
    {
        return;
    }
    if (parentBox != m_searchFrame->parentWidget())
    {
        m_searchFrame->setParent(parentBox);
    }
    OnTextEditResized(parentBox->size());
}

void ResourceManagementView::OnTextEditResized(const QSize& editSize)
{
    // We don't want our close button to be covered by a scroll bar
    static const int scrollWidth = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);

    auto frameGeometry = m_searchFrame->geometry();
    frameGeometry.moveTo(
        editSize.width() - (frameGeometry.width() + scrollWidth),
        0);
    m_searchFrame->setGeometry(frameGeometry);
    m_searchFrame->update();
}

void ResourceManagementView::HideSearchFrame()
{
    m_searchFrame->setHidden(true);
}

void ResourceManagementView::CreateSearchFrame()
{
    m_searchFrame = new QFrame(this);

    QHBoxLayout* searchLayout = new QHBoxLayout;

    m_searchFrame->setMinimumWidth(cSearchFrameWidth);

    m_searchLabel = new QLabel("Find:");
    searchLayout->addWidget(m_searchLabel, 0, Qt::AlignRight);

    m_editSearchBox = new QLineEdit;

    searchLayout->addWidget(m_editSearchBox);

    m_clearSearchBox = new QPushButton();
    m_clearSearchBox->setIcon(QIcon("Editor/Icons/CloudCanvas/clear_search_box.png"));
    m_clearSearchBox->setIconSize(QSize(11, 11));
    m_clearSearchBox->setToolTip("Clear your search.");
    m_clearSearchBox->setFlat(true);
    m_clearSearchBox->setFixedSize(QSize(11, 11));
    m_clearSearchBox->setVisible(false);

    searchLayout->addWidget(m_clearSearchBox);

    m_findPrevious = new QPushButton("Previous");
    m_findPrevious->setToolTip("The keyboard shortcut is SHIFT + F3.");
    searchLayout->addWidget(m_findPrevious);
    m_findPrevious->setDisabled(true);

    m_findNext = new QPushButton("Next");
    m_findNext->setToolTip("The keyboard shortcut is F3.");
    searchLayout->addWidget(m_findNext);
    m_findNext->setDisabled(true);

    m_hideSearchBox = new QPushButton();
    m_hideSearchBox->setIcon(QIcon("Editor/Icons/CloudCanvas/clear_search_box.png"));
    m_hideSearchBox->setIconSize(QSize(11, 11));
    m_hideSearchBox->setToolTip("Hide search function.");
    m_hideSearchBox->setFlat(true);
    m_hideSearchBox->setFixedSize(QSize(11, 11));
    searchLayout->addWidget(m_hideSearchBox);

    m_searchFrame->setLayout(searchLayout);
}

// Bottom row - Tree and Text entry box
void ResourceManagementView::CreateBottomRow(QVBoxLayout* verticalLayout)
{
    // Lower Row (Tree and Text box)
    m_verticalSplitter = new QSplitter;
    m_verticalSplitter->setOpaqueResize(false);
    //m_verticalSplitter->setStyleSheet("border: 1px solid yellow");

    AddTreeView(m_verticalSplitter);

    m_emptyDetailWidget = new EmptyDetailWidget {
        this
    };
    m_currentDetailWidget = m_emptyDetailWidget;
    m_verticalSplitter->addWidget(m_emptyDetailWidget);
    //qDebug() << "visible" << m_currentDetailWidget->isVisible() << "splitter added empty at index" << m_verticalSplitter->indexOf(m_emptyDetailWidget);

    m_verticalSplitter->setStretchFactor(0, TREE_STRETCH_FACTOR);
    m_verticalSplitter->setStretchFactor(1, DETAIL_STRETCH_FACTOR);

    auto initalTreeSize = (size().width() / 3) * 1;
    auto initalDetailSize = (size().width() / 3) * 2;

    m_verticalSplitter->setSizes({initalTreeSize, initalDetailSize});

    // Stretch needs to be 1 on the splitter to allow our lower row to expand properly
    verticalLayout->addWidget(m_verticalSplitter, 1);
}

// Standardized vertical spacer scaled appropriately used in several places in the window
void ResourceManagementView::AddVerticalSpacer(QHBoxLayout* horizontalLayout)
{
    if (!horizontalLayout)
    {
        return;
    }

    QFrame* treeTextSpacer = new QFrame;
    treeTextSpacer->setFrameShape(QFrame::VLine);
    treeTextSpacer->setFrameShadow(QFrame::Sunken);
    horizontalLayout->addWidget(treeTextSpacer);
}

// Add a tree view to our bottom horizontal row - sits on the lower left of the window
void ResourceManagementView::AddTreeView(QSplitter* verticalSplitter)
{
    auto widget = new QWidget {};
    verticalSplitter->addWidget(widget);

    auto layout = new QVBoxLayout {};
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    widget->setLayout(layout);

    auto titleFrame = new QFrame {};
    titleFrame->setFrameShape(QFrame::Shape::Panel);
    titleFrame->setFrameShadow(QFrame::Shadow::Raised);
    layout->addWidget(titleFrame);

    auto titleLayout = new QVBoxLayout {};
    titleLayout->setContentsMargins(6, 3, 6, 3);
    titleLayout->setSpacing(6);
    titleFrame->setLayout(titleLayout);

    auto titleText = new QLabel {
        "Cloud Canvas configuration"
    };
    titleText->setObjectName("TreeTitle");
    titleLayout->addWidget(titleText);

    // Begin setup Tree view
    m_treeView = new RMVTreeView();
    //m_treeView->setStyleSheet("border: 1px solid blue");
    m_treeView->setHeaderHidden(true);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setModel(m_projectModel.data());
    m_treeView->setRootIsDecorated(true);
    m_treeView->expand(m_projectModel->ResourceGroupListIndex());
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &ResourceManagementView::OnTreeContextMenuRequested);

    // Selection control
    QItemSelectionModel* selectionModel = m_treeView->selectionModel();
    connect(selectionModel, &QItemSelectionModel::selectionChanged,
        this, &ResourceManagementView::OnTreeViewSelectionChanged);

    layout->addWidget(m_treeView);

    //qDebug() << "splitter added tree view at index" << m_verticalSplitter->indexOf(m_treeView);
}

QString ResourceManagementView::GetDeploymentLabel() const
{
    QString deploymentLabel("Current deployment: ");
    // TODO: fix this styling
    deploymentLabel += "<a style='color:#4285F4; text-decoration:none' href=\"Current Deployment\" >";
    deploymentLabel += m_deploymentModel->IsActiveDeploymentSet() ? m_deploymentModel->GetActiveDeploymentName() : "(none)";
    deploymentLabel += "</a>";
    return deploymentLabel;
}

QString ResourceManagementView::GetProfileLabel() const
{
    QString profileLabel("Current profile: ");
    // TODO: fix this styling
    profileLabel += "<a style='color:#4285F4; text-decoration:none' href=\"Current Profile\" >";
    profileLabel += HasDefaultProfile() ? m_profileModel->GetDefaultProfile() : "(none)";
    profileLabel += "</a>";
    return profileLabel;
}

bool ResourceManagementView::HasDefaultProfile() const
{
    return (m_profileModel->GetDefaultProfile().isEmpty() == false);
}

bool ResourceManagementView::HasAnyProfiles() const
{
    return (m_profileModel->rowCount());
}

void ResourceManagementView::OnTreeViewSelectionChanged(const QItemSelection& newSelected, const QItemSelection& oldSelected)
{
    SelectDetailWidget();
}

void ResourceManagementView::SelectDetailWidget()
{
    if (m_treeView->currentIndex().isValid())
    {
        ShowDetailWidget(m_detailWidgetFinder.FindDetailWidget(m_treeView->currentIndex()));
    }
    else
    {
        ShowDetailWidget(m_emptyDetailWidget);
    }
}

void ResourceManagementView::ShowDetailWidget(DetailWidget* widget)
{
    // If already showing this widget do nothing.

    if (m_currentDetailWidget == widget)
    {
        return;
    }

    assert(m_currentDetailWidget);

    // prevent unwanted repaints while swapping out the widgets

    setUpdatesEnabled(false);

    // get current sizes

    auto sizes = m_verticalSplitter->sizes();
    auto previousIndex = m_verticalSplitter->indexOf(m_currentDetailWidget);
    //qDebug() << "index" << previousIndex << "sizes before" << sizes;
    auto currentSize = sizes[previousIndex];

    // hide current widget

    m_currentDetailWidget->hide();

    // reset ui to default (generally disabled) state

    m_menuSave->setDisabled(true);
    m_menuSaveAs->setDisabled(true);

    m_menuCut->setDisabled(true);
    m_menuCopy->setDisabled(true);
    m_menuPaste->setDisabled(true);

    m_menuUndo->setDisabled(true);
    m_menuRedo->setDisabled(true);

    m_menuNewDirectory->setDisabled(true);
    m_menuNewFile->setDisabled(true);
    m_menuNewDeployment->setEnabled(!m_resourceManager->IsOperationInProgress());
    m_menuNewResourceGroup->setEnabled(true);
    m_menuNewResource->setDisabled(true);

    DisableSaveButton(tr("The selected item cannot be saved"));
    DisableDeleteButton(tr("The selected item cannot be deleted"));
    m_validateTemplateButton->setVisible(false);

    m_menuSearch->setEnabled(false);
    m_findNextShortcut->setEnabled(false);
    m_findPrevShortcut->setEnabled(false);
    m_hideSearchShortcut->setEnabled(false);
    m_editSearchBox->setEnabled(false);
    m_clearSearchBox->setEnabled(false);
    m_searchLabel->setEnabled(false);
    m_editSearchBox->setVisible(false);
    m_clearSearchBox->setVisible(false);
    m_searchLabel->setVisible(false);

    // make new widget the current widget

    m_currentDetailWidget = widget;

    if (!m_verticalSplitter->children().contains(m_currentDetailWidget))
    {
        m_verticalSplitter->addWidget(m_currentDetailWidget);
        auto newIndex = m_verticalSplitter->indexOf(m_currentDetailWidget);
        m_verticalSplitter->setStretchFactor(newIndex, DETAIL_STRETCH_FACTOR);
        sizes.insert(newIndex, currentSize);
        //qDebug() << "insert at" << newIndex;
    }

    // Initially (when the project status widget is first added) the sizes array
    // will have contain all zeros. In that case we want to let the splitter work
    // out the initial sizes (so it honors the strech factors). But after that we
    // have to make sure the make the new widget has same size as the previous
    // widget or the splitter jumps around as the user selects nodes in the tree.

    if (sizes[0] != 0)
    {
        sizes[m_verticalSplitter->indexOf(m_currentDetailWidget)] = currentSize;
        sizes[previousIndex] = 0;
        //qDebug() << "fixed sizes" << sizes;
        m_verticalSplitter->setSizes(sizes);
    }

    // Default source control state for items in the tree is NOT_APPLICABLE. TextDetailWidgets will change this value when they show themselves.
    this->SetSourceControlState(SourceControlState::NOT_APPLICABLE);

    // Show new widget. This causes the wiget to paint, but this happens before the
    // QSplitter has finalized its size. That is why we disable updates (at the top
    // of this function, just to be safe) and then re-enable them after making the
    // widget visible. After updates are enabled and this function returns, the
    // splitter finishes resizing and painting its widgets.

    //qDebug() << "before showing sizes" << m_verticalSplitter->sizes() << "sizeHint" << m_currentDetailWidget->sizeHint() << "and size" << m_currentDetailWidget->size();
    m_currentDetailWidget->show();
    //qDebug() << "after showing sizes" << m_verticalSplitter->sizes() << "sizeHint" << m_currentDetailWidget->sizeHint() << "and size" << m_currentDetailWidget->size();

    setUpdatesEnabled(true);

    update();

    // qDebug() << "after update sizes" << m_verticalSplitter->sizes() << "sizeHint" << m_currentDetailWidget->sizeHint() << "and size" << m_currentDetailWidget->size();
}

void ResourceManagementView::OnCurrentDeploymentClicked()
{
    ShowActiveDeploymentWindow();
}

void ResourceManagementView::ShowActiveDeploymentWindow()
{
    GetIEditor()->OpenWinWidget(ActiveDeployment::GetWWId());
}

void ResourceManagementView::OnCurrentProfileClicked()
{
    ShowCurrentProfileWindow();
}

void ResourceManagementView::ShowCurrentProfileWindow()
{
    if (HasAnyProfiles())
    {
        GetIEditor()->OpenWinWidget(ProfileSelector::GetWWId());
    }
    else
    {
        ProfileSelector::OpenAddProfileDialog(this);
    }
}

QString ResourceManagementView::GetResourceGroupNameInvalidLengthError(const QString& resourceGroupName) const
{
    assert(!ResourceManagementView::ValidateStackNameLength(resourceGroupName));
    return tr("Resource group name too long (%1) limit %2 characters").arg(resourceGroupName.length()).arg(ResourceManagementView::maxStackNameLength);
}

QString ResourceManagementView::GetResourceGroupNameInvalidFormatError(const QString& resourceGroupName) const
{
    assert(!ResourceManagementView::ValidateStackNameNoHyphen(resourceGroupName));
    return tr("Please provide a name that starts with a letter followed by alphanumeric characters");
}

void ResourceManagementView::ShowBasicErrorBox(const QString& boxTitle, const QString& warningText)
{
    QMessageBox box(QMessageBox::NoIcon,
        boxTitle,
        warningText,
        QMessageBox::Ok,
        Q_NULLPTR,
        Qt::Popup);
    box.exec();
}

void ResourceManagementView::OnMenuNewResourceGroup()
{
    PopupDialogWidget dialog {
        this
    };
    dialog.setWindowTitle(tr("New resource group"));

    ValidatingLineEditWidget nameEdit {
        &ResourceManagementView::ValidateStackNameNoHyphen,
        &ResourceManagementView::ValidateStackNameLength,
        [this](const QString& candidateName) { return this->GetResourceGroupNameInvalidFormatError(candidateName); },
        [this](const QString& candidateName) { return this->GetResourceGroupNameInvalidLengthError(candidateName); }
    };
    connect(&dialog, &PopupDialogWidget::DialogMoved, &nameEdit, &ValidatingLineEditWidget::OnMoveOrResize);
    connect(&dialog, &PopupDialogWidget::DialogResized, &nameEdit, &ValidatingLineEditWidget::OnMoveOrResize);
    nameEdit.setParent(&dialog);
    dialog.AddWidgetPairRow(new QLabel {tr("Resource group name")}, &nameEdit);
    nameEdit.setFocus();

    QComboBox includedContentComboBox {
        &dialog
    };
    includedContentComboBox.addItems({"no-resources", "bucket", "lambda", "api-lambda", "api-lambda-bucket", "api-lambda-dynamodb"});
    includedContentComboBox.setCurrentIndex(0);
    includedContentComboBox.setToolTip(tr("Example resources to be included in the resource group."));
    dialog.AddWidgetPairRow(new QLabel {tr("Example resources")}, &includedContentComboBox);

    dialog.GetPrimaryButton()->setText(tr("Create"));

    if (dialog.exec() == QDialog::Accepted)
    {
        QString resourceGroupName = nameEdit.text();

        if (!ResourceManagementView::ValidateStackNameLength(resourceGroupName))
        {
            ShowBasicErrorBox(tr("Invalid resource group name length"), GetResourceGroupNameInvalidLengthError(resourceGroupName));
            return;
        }

        if (!ResourceManagementView::ValidateStackNameNoHyphen(resourceGroupName))
        {
            ShowBasicErrorBox(tr("Invalid resource group name"), GetResourceGroupNameInvalidFormatError(resourceGroupName));
            return;
        }

        ValidateSourceCreateResourceGroup(resourceGroupName, includedContentComboBox.currentText());
    }
}

void ResourceManagementView::ValidateSourceCreateResourceGroup(const QString& resourceGroupName, const QString& initialContent)
{
    QObject::connect(&*m_resourceManager->GetDeploymentTemplateSourceModel(), &IFileSourceControlModel::SourceControlStatusUpdated, this, [this, resourceGroupName, initialContent]() {SourceUpdatedCreateResourceGroup(resourceGroupName, initialContent); });
    m_resourceManager->UpdateSourceControlStates();
}

void ResourceManagementView::SourceUpdatedCreateResourceGroup(const QString& resourceGroupName, const QString& initialContent)
{
    QObject::disconnect(&*m_resourceManager->GetDeploymentTemplateSourceModel(), &IFileSourceControlModel::SourceControlStatusUpdated, this, 0);

    if (m_resourceManager->GemsFileNeedsCheckout())
    {
        QString fileName = m_resourceManager->GetProjectModel()->GetGemsFile();
        QString checkoutString = fileName + GetSourceCheckoutString();
        auto reply = QMessageBox::question(
                this,
                "File needs checkout",
                checkoutString,
                QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes)
        {
            QObject::connect(&*m_resourceManager->GetGemsFileSourceModel(), &IFileSourceControlModel::SourceControlStatusChanged, this, [this, resourceGroupName, initialContent]() { SourceChangedCreateResourceGroup(resourceGroupName, initialContent); });
            GetIEditor()->GetAWSResourceManager()->RequestEditGemsFile();
        }
        return;
    }
    DoCreateResourceGroup(resourceGroupName, initialContent);
}

void ResourceManagementView::DoCreateResourceGroup(const QString& resourceGroupName, const QString& initialContent)
{
   QMessageBox::critical(this, "Error", "Adding resource groups is no longer supported and was deprecated in Lumberyard 1.11.  Please use the 'lmbr_aws gem create' command.");   
}

void ResourceManagementView::SourceChangedCreateResourceGroup(const QString& resourceGroupName, const QString& initialContent)
{
    QObject::disconnect(&*m_resourceManager->GetDeploymentTemplateSourceModel(), &IFileSourceControlModel::SourceControlStatusUpdated, this, 0);
    QObject::disconnect(&*m_resourceManager->GetDeploymentTemplateSourceModel(), &IFileSourceControlModel::SourceControlStatusChanged, this, 0);
    if (!m_resourceManager->DeploymentTemplateNeedsCheckout())
    {
        DoCreateResourceGroup(resourceGroupName, initialContent);
    }
}

QString ResourceManagementView::GetDeploymentNameInvalidLengthError(const QString& deploymentName) const
{
    assert(!ResourceManagementView::ValidateStackNameLength(deploymentName));
    return tr("Deployment name too long (%1) limit %2 characters.").arg(deploymentName.length()).arg(ResourceManagementView::maxStackNameLength);
}

QString ResourceManagementView::GetDeploymentNameInvalidFormatError(const QString& deploymentName) const
{
    assert(!ResourceManagementView::ValidateStackNameFormat(deploymentName));
    return tr("Please provide a deployment name that starts with a letter followed by alphanumeric characters or hyphens.");
}

void ResourceManagementView::OnMenuNewDeployment(bool required)
{
    if (!m_resourceManager->IsProjectInitialized())
    {
        if (ConfirmCreateProjectStack())
        {
            m_pendingCreateDeployment = true;
            OnCreateProjectStack();
        }
        return;
    }

    PopupDialogWidget dialog {
        this
    };
    dialog.setObjectName("CreateDeployment");
    dialog.setWindowTitle(tr("Create deployment"));

    QLabel requiredLabel {
        tr(
            "You must add a deployment before you can proceed. Only AWS account admins can create deployments. "
            "You will be able to create multiple deployments for your Lumberyard project. Each deployment provides "
            "an independent copy of the AWS resources you create for your game. For example, you create Dev, Test, "
            "and Live deployments for your project. Deployments exist in AWS as a CloudFormation stack that contains "
            "a child stack for each resource group."
            )
    };
    requiredLabel.setObjectName("RequiredMessage");
    requiredLabel.setProperty("class", "Message");
    requiredLabel.setWordWrap(true);
    dialog.AddSpanningWidgetRow(&requiredLabel);
    requiredLabel.setVisible(required);

    if (required)
    {
        dialog.setMinimumSize(CREATE_DEPLOYMENT_DIALOG_MINIMUM_WIDTH, CREATE_DEPLOYMENT_DIALOG_MINIMUM_HEIGHT);
    }

    AZStd::string learnMoreLabelContent = "<a href=\"";
    learnMoreLabelContent.append(MaglevControlPanelPlugin::GetDeploymentHelpLink());
    learnMoreLabelContent.append("\" >Learn more</a>");
    QLabel learnMoreLabel{
        tr(learnMoreLabelContent.c_str())
    };
    learnMoreLabel.setObjectName("LearnMore");
    learnMoreLabel.setProperty("class", "Message");
    connect(&learnMoreLabel, &QLabel::linkActivated, this, []() { QDesktopServices::openUrl(QUrl(QString(MaglevControlPanelPlugin::GetDeploymentHelpLink()))); });
    dialog.AddSpanningWidgetRow(&learnMoreLabel);
    learnMoreLabel.setVisible(required);

    // This version matches the style of the add resource dialog and is visible when requiredLabel is not visible.
    AZStd::string learnMoreLabel2Content = "You must provide the following to create a deployment.  <a href=\"";
    learnMoreLabel2Content.append(MaglevControlPanelPlugin::GetDeploymentHelpLink());
    learnMoreLabel2Content.append("\" >Learn more</a> about deployments.");
    QLabel learnMoreLabel2{
        tr(learnMoreLabel2Content.c_str())
    };
    learnMoreLabel2.setObjectName("LearnMore2");
    learnMoreLabel2.setProperty("class", "Message");
    learnMoreLabel2.setWordWrap(true);
    learnMoreLabel2.setMinimumHeight(55);
    learnMoreLabel2.setAlignment(Qt::AlignTop);
    learnMoreLabel2.setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    connect(&learnMoreLabel2, &QLabel::linkActivated, this, []() { QDesktopServices::openUrl(QUrl(QString(MaglevControlPanelPlugin::GetDeploymentHelpLink()))); });
    dialog.AddSpanningWidgetRow(&learnMoreLabel2);
    learnMoreLabel2.setVisible(!required);

    ValidatingLineEditWidget nameEdit {
        &ResourceManagementView::ValidateStackNameFormat,
        &ResourceManagementView::ValidateStackNameLength,
        [this](const QString& candidateName) { return this->GetDeploymentNameInvalidFormatError(candidateName); },
        [this](const QString& candidateName) { return this->GetDeploymentNameInvalidLengthError(candidateName); }
    };
    connect(&dialog, &PopupDialogWidget::DialogMoved, &nameEdit, &ValidatingLineEditWidget::OnMoveOrResize);
    connect(&dialog, &PopupDialogWidget::DialogResized, &nameEdit, &ValidatingLineEditWidget::OnMoveOrResize);
    dialog.AddWidgetPairRow(new QLabel { tr("Deployment name") }, &nameEdit);
    nameEdit.setFocus();

    QCheckBox makeDefaultDeploymentCheckbox {
        tr("Make default deployment")
    };
    makeDefaultDeploymentCheckbox.setChecked(false);
    dialog.AddRightOnlyWidgetRow(&makeDefaultDeploymentCheckbox);
    // The first created deployment will always be set to be the default
    if (m_projectModel->itemFromIndex(m_projectModel->DeploymentListIndex())->rowCount() == 0)
    {
        makeDefaultDeploymentCheckbox.setChecked(true);
        makeDefaultDeploymentCheckbox.setEnabled(false);
        makeDefaultDeploymentCheckbox.setToolTip(tr("The first deployment is automatically set to be the default"));
    }

    QLabel warningLabel {
        tr("This operation may take a few minutes to complete.")
    };
    warningLabel.setObjectName("WarningMessage");
    warningLabel.setProperty("class", "Message");
    warningLabel.setMinimumHeight(55);
    warningLabel.setWordWrap(true);
    dialog.AddSpanningWidgetRow(&warningLabel);

    dialog.GetPrimaryButton()->setText(tr("Create"));

    while (true)
    {
        if (dialog.exec() == QDialog::Accepted)
        {
            auto deploymentName = nameEdit.text();

            if (!ResourceManagementView::ValidateStackNameLength(deploymentName))
            {
                ShowBasicErrorBox(tr("Invalid deployment name length"), GetDeploymentNameInvalidLengthError(deploymentName));
                continue;
            }

            if (!ResourceManagementView::ValidateStackNameFormat(deploymentName))
            {
                ShowBasicErrorBox(tr("Invalid deployment name"), GetDeploymentNameInvalidFormatError(deploymentName));
                continue;
            }

            m_waitingForDeployment = deploymentName;
            m_projectModel->CreateDeploymentStack(deploymentName, makeDefaultDeploymentCheckbox.isChecked());

        }
        break;
    }
}

void ResourceManagementView::OnMenuSwitchProfile()
{
    GetIEditor()->OpenWinWidget(ProfileSelector::GetWWId());
}

void ResourceManagementView::OnMenuSwitchDeployment()
{
    ShowActiveDeploymentWindow();
}

void ResourceManagementView::OnCreateProjectStack()
{
    GetIEditor()->OpenWinWidget(InitializeCloudCanvasProject::GetWWId());
}

void ResourceManagementView::OnMenuCloudCanvasHelp()
{
    QDesktopServices::openUrl(QUrl(QString(MaglevControlPanelPlugin::GetHelpLink())));
}

void ResourceManagementView::OnProjectInitializedChanged()
{
    if (m_resourceManager->GetInitializationState() == IAWSResourceManager::InitializationState::InitializingState)
    {
        auto index = m_projectModel->ProjectStackIndex();
        m_treeView->expand(index.parent());
        m_treeView->setCurrentIndex(index);
    }

    if (m_resourceManager->GetInitializationState() == IAWSResourceManager::InitializationState::InitializedState)
    {
        if (m_pendingUpdate)
        {
            auto temp = m_pendingUpdate;
            m_pendingUpdate.reset();
            UpdateStack(temp);
        }
        if (m_pendingCreateDeployment)
        {
            m_pendingCreateDeployment = false;
            OnMenuNewDeployment(/* requried */ true);
        }
    }
}

void ResourceManagementView::OnProfileModelReset()
{
    if (m_currentProfileLabel)
    {
        m_currentProfileLabel->setText(GetProfileLabel());
    }
}

void ResourceManagementView::OnDeploymentModelReset()
{
    if (m_currentDeploymentLabel)
    {
        m_currentDeploymentLabel->setText(GetDeploymentLabel());
    }
}

void ResourceManagementView::OnResourceGroupAdded(const QModelIndex& index)
{
    if (m_waitingForResourceGroup != index.data())
    {
        return;
    }

    m_waitingForResourceGroup.clear();

    m_treeView->expand(index.parent());
    m_treeView->setCurrentIndex(index);
}

void ResourceManagementView::OnDeploymentAdded(const QModelIndex& index)
{
    if (m_waitingForDeployment != index.data())
    {
        return;
    }

    m_waitingForDeployment.clear();

    m_treeView->expand(index.parent());
    m_treeView->setCurrentIndex(index);
}

void ResourceManagementView::OnPathAdded(const QModelIndex& index, const QString& path)
{
    // Ignoring case isn't ideal, but the drive letter seems to have a different case
    // in different circumstances.
    if (path.toLower() == m_waitingForPath.toLower())
    {
        m_waitingForPath.clear();
        m_treeView->expand(index.parent());
        m_treeView->setCurrentIndex(index);
    }
}

void ResourceManagementView::OnUpdatingStack(const QModelIndex& index)
{
    m_treeView->expand(index.parent());
    m_treeView->setCurrentIndex(index);
}

void ResourceManagementView::OnUpdateResourceGroupButtonGainedFocus()
{
    QStandardItem* resourceGroup = FindSelectedItemResourceGroup();
    if (resourceGroup != nullptr)
    {
        resourceGroup->setBackground(AWSUtil::MakePrettyColor("Highlight"));  // Highlight the node
    }
}

void ResourceManagementView::OnUpdateResourceGroupButtonLostFocus()
{
    QStandardItem* resourceGroup = FindSelectedItemResourceGroup();
    if (resourceGroup != nullptr)
    {
        resourceGroup->setBackground(Qt::transparent);  // Clear the highlight
    }
}

// Helper to find the Resource Group corresponding to the currently selected item
QStandardItem* ResourceManagementView::FindSelectedItemResourceGroup() const
{
    if (m_treeView->currentIndex().isValid())
    {
        QModelIndex resourceGroupListIndex = m_projectModel->ResourceGroupListIndex(); // Get parent of all resource group nodes

        // Check each parent of the current index until its parent is the parent of the resource group nodes.
        // Once found, the current index is the index of the containing resource group node.
        QModelIndex iterIndex = m_treeView->currentIndex(); // Start with selected item
        while (iterIndex.isValid() && iterIndex.parent() != resourceGroupListIndex)
        {
            iterIndex = iterIndex.parent();
        }

        if (iterIndex.isValid())
        {
            return m_projectModel->itemFromIndex(iterIndex);    // Found the parent Resource Group
        }
    }

    return nullptr; // Failed to find parent resource group
}

void ResourceManagementView::OnSaveAll()
{
    for (auto it = m_fileContentDetailWidgets.constBegin(); it != m_fileContentDetailWidgets.constEnd(); ++it)
    {
        it.value()->Save();
    }

    for (auto it = m_cloudFormationTemplateDetailWidgets.constBegin(); it != m_cloudFormationTemplateDetailWidgets.constEnd(); ++it)
    {
        it.value()->Save();
    }
}

bool ResourceManagementView::DeleteStack(const QSharedPointer<IStackStatusModel>& model)
{
    auto reply = QMessageBox::question(
            this,
            model->GetDeleteConfirmationTitle(),
            model->GetDeleteConfirmationMessage(),
            QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {
        OnSaveAll();
        return model->DeleteStack();
    }

    return false;
}

bool ResourceManagementView::UpdateStack(const QSharedPointer<IStackStatusModel>& model)
{
    if (!model->StackExists())
    {
        // If update is attempted on a non-existant stack, it must be the placeholder model used
        // by group resources when there are no deployments.

        if (!m_resourceManager->IsProjectInitialized())
        {
            if (ConfirmCreateProjectStack())
            {
                // special case to prevent update of project stack right after it is created
                if (model != m_resourceManager->GetProjectModel()->ProjectStatusModel())
                {
                    m_pendingUpdate = model;
                }
                OnCreateProjectStack();
                return false;
            }
            else
            {
                return false;
            }
        }
        else
        {
            if (m_resourceManager->GetDeploymentModel()->rowCount() == 0)
            {
                OnMenuNewDeployment(/* requried */ true);
                return false;
            }
        }
    }

    OnSaveAll();

    PopupDialogWidget dialog { this };
    dialog.setObjectName("UpdateStack");
    dialog.setWindowTitle(model->GetUpdateConfirmationTitle());
    dialog.setContentsMargins(QMargins(10, 10, 10, 10));

    auto stackResourcesModel = model->GetStackResourcesModel();
    auto pendingChangesModel = stackResourcesModel->GetPendingChangeFilterProxy();

    QFrame tableFrame{};
    QVBoxLayout tableLayout{};
    MaximumSizedTableView resourcesTable{&tableFrame};
    QLabel statusLabel{};

    resourcesTable.setObjectName("ResourcesTable");
    resourcesTable.TableView()->setModel(pendingChangesModel.data());
    resourcesTable.TableView()->verticalHeader()->hide();
    resourcesTable.TableView()->setShowGrid(false);
    resourcesTable.TableView()->setEditTriggers(QTableView::NoEditTriggers);
    resourcesTable.TableView()->setSelectionMode(QTableView::NoSelection);
    resourcesTable.TableView()->setFocusPolicy(Qt::NoFocus);
    resourcesTable.TableView()->setWordWrap(false);
    for (int column = 0; column < pendingChangesModel->columnCount(); ++column)
    {
        resourcesTable.TableView()->hideColumn(column);
    }
    resourcesTable.TableView()->showColumn(IStackResourcesModel::Columns::PendingActionColumn);
    resourcesTable.TableView()->showColumn(IStackResourcesModel::Columns::ChangeImpactColumn);
    resourcesTable.TableView()->showColumn(IStackResourcesModel::Columns::NameColumn);
    resourcesTable.TableView()->showColumn(IStackResourcesModel::Columns::PhysicalResourceIdColumn);
    resourcesTable.TableView()->showColumn(IStackResourcesModel::Columns::ResourceStatusColumn);
    resourcesTable.TableView()->showColumn(IStackResourcesModel::Columns::ResourceTypeColumn);
    resourcesTable.TableView()->showColumn(IStackResourcesModel::Columns::TimestampColumn);
    resourcesTable.Resize();

    statusLabel.setObjectName("Status");
    statusLabel.setText(tr("Looking for changes..."));

    tableLayout.setSpacing(0);
    tableLayout.setMargin(0);
    tableLayout.addWidget(&resourcesTable);
    tableLayout.addWidget(&statusLabel);
    tableLayout.addStretch();

    tableFrame.setObjectName("TableFrame");
    tableFrame.setMinimumSize(600, 75);
    tableFrame.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tableFrame.setLayout(&tableLayout);

    dialog.AddSpanningWidgetRow(&tableFrame, 1);

    QLabel messageLabel { model->GetUpdateConfirmationMessage() };
    messageLabel.setObjectName("Message");
    messageLabel.setProperty("class", "Message");
    messageLabel.setWordWrap(true);
    messageLabel.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    dialog.AddSpanningWidgetRow(&messageLabel);

    QCheckBox confirmDeletionCheckBox{};
    confirmDeletionCheckBox.setText("It is OK that this will permanently DELETE resources.");
    confirmDeletionCheckBox.setEnabled(false);
    dialog.AddSpanningWidgetRow(&confirmDeletionCheckBox);

    QCheckBox confirmSecurityCheckBox{};
    confirmSecurityCheckBox.setText("It is OK that this will impact resource SECURITY.");
    confirmSecurityCheckBox.setEnabled(false);
    dialog.AddSpanningWidgetRow(&confirmSecurityCheckBox);

    dialog.GetPrimaryButton()->setText(tr("Yes"));
    dialog.GetPrimaryButton()->setEnabled(false); // Wait until proxy model reset (see below)
    dialog.GetPrimaryButton()->setToolTip(tr("Determining changes. Please wait."));

    dialog.GetCancelButton()->setText(tr("No"));

    // The underlying model will be refreshed automatically and when that is finished
    // the proxy model will be reset and the handler below will be called.

    connect(
        pendingChangesModel.data(),
        &QAbstractItemModel::modelReset,
        &dialog,
        [&resourcesTable, &dialog, &confirmDeletionCheckBox, &confirmSecurityCheckBox, &stackResourcesModel, &statusLabel]
        {

            resourcesTable.Resize();

            dialog.GetBottomLabel()->setText(tr(""));

            confirmDeletionCheckBox.setEnabled(stackResourcesModel->ContainsDeletionChanges());
            confirmSecurityCheckBox.setEnabled(stackResourcesModel->ContainsSecurityChanges());

            dialog.GetPrimaryButton()->setEnabled(
                !confirmSecurityCheckBox.isEnabled() &&
                !confirmDeletionCheckBox.isEnabled()
            );

            dialog.GetPrimaryButton()->setToolTip(
                dialog.GetPrimaryButton()->isEnabled()
                ? tr("")
                : tr("Select enabled confirmation checkboxes to enable.")
            );

            if (stackResourcesModel->ContainsChanges())
            {
                statusLabel.setVisible(false);
            }
            else
            {
                statusLabel.setText("No changes found.");
            }

        }
    );

    connect(&confirmDeletionCheckBox, &QCheckBox::stateChanged, &dialog,
        [&confirmSecurityCheckBox, &dialog](int state)
        {
            auto button = dialog.GetPrimaryButton();
            button->setEnabled(state && (confirmSecurityCheckBox.isChecked() || !confirmSecurityCheckBox.isEnabled()));
            button->setToolTip(
                dialog.GetPrimaryButton()->isEnabled()
                ? tr("")
                : tr("Select enabled confirmation checkboxes to enable.")
            );
        }
    );

    connect(&confirmSecurityCheckBox, &QCheckBox::stateChanged, &dialog,
        [&confirmDeletionCheckBox, &dialog](int state)
        {
            auto button = dialog.GetPrimaryButton();
            button->setEnabled(state && (confirmDeletionCheckBox.isChecked() || !confirmDeletionCheckBox.isEnabled()));
            button->setToolTip(
                dialog.GetPrimaryButton()->isEnabled()
                ? tr("")
                : tr("Select enabled confirmation checkboxes to enable.")
            );
        }
    );

    int result = dialog.exec();

    if (result == QDialog::Accepted)
    {
        return model->UpdateStack();
    }

    return false;

}

bool ResourceManagementView::UploadLambdaCode(const QSharedPointer<IStackStatusModel>& model, QString functionName)
{
    if (!model->StackExists())
    {
        // If update is attempted on a non-existant stack, it must be the placeholder model used
        // by group resources when there are no deployments.

        if (!m_resourceManager->IsProjectInitialized())
        {
            if (!ConfirmCreateProjectStack())
            {
                return false;
            }
            // special case to prevent update of project stack right after it is created
            if (model != m_resourceManager->GetProjectModel()->ProjectStatusModel())
            {
                m_pendingUpdate = model;
            }
            OnCreateProjectStack();
            return false;
        }
        else
        {
            if (m_resourceManager->GetDeploymentModel()->rowCount() == 0)
            {
                OnMenuNewDeployment(/* requried */ true);
                return false;
            }
        }
    }
    return model->UploadLambdaCode(functionName);
}

bool ResourceManagementView::ConfirmCreateProjectStack()
{
    if (!m_adminConfirmed)
    {
        QString title = tr(
                "Initialize Resoure Manager"
                );

        QString message = tr(
                "Before you can create the AWS resources for use in your game, you must have prepared your AWS account "
                "to work with Resource Manager and this Lumberyard Project. When you proceed, we will create the "
                "AWS resources needed by Resource Manager. You must be an admin to run this process."
                "<p>"
                "Are you an AWS account admin?"
                );

        auto reply = QMessageBox::question(
                this,
                title,
                message,
                QMessageBox::Yes | QMessageBox::No);

        m_adminConfirmed = reply == QMessageBox::Yes;
    }

    return m_adminConfirmed;
}

bool ResourceManagementView::ValidateStackNameLength(const QString& stackName)
{
    return (stackName.length() <= ResourceManagementView::maxStackNameLength);
}

bool ResourceManagementView::ValidateStackNameFormat(const QString& stackName)
{
    QRegExp matchStr("^[a-zA-Z][a-zA-Z0-9\\-]*$");

    return matchStr.exactMatch(stackName);
}

bool ResourceManagementView::ValidateStackNameNoHyphen(const QString& stackName)
{
    QRegExp matchStr("^[a-zA-Z][a-zA-Z0-9]*$");

    return matchStr.exactMatch(stackName);
}

ResourceManagementView::DetailWidgetFinder::DetailWidgetFinder(ResourceManagementView* view)
    : m_view{view}
{
}

DetailWidget* ResourceManagementView::DetailWidgetFinder::FindDetailWidget(const QModelIndex& index)
{
    m_detailWidget = nullptr;
    m_view->m_projectModel->VisitDetail(index, this);
    assert(m_detailWidget != nullptr);
    return m_detailWidget;
}

void ResourceManagementView::DetailWidgetFinder::VisitProjectDetailNone()
{
    m_detailWidget = m_view->m_emptyDetailWidget;
}

void ResourceManagementView::DetailWidgetFinder::VisitProjectDetail(QSharedPointer<IFileContentModel> fileContentModel)
{
    if (!m_view->m_fileContentDetailWidgets.contains(fileContentModel))
    {
        m_view->m_fileContentDetailWidgets.insert(fileContentModel, new FileContentDetailWidget(m_view, fileContentModel));
    }
    m_detailWidget = m_view->m_fileContentDetailWidgets[fileContentModel];
}

void ResourceManagementView::DetailWidgetFinder::VisitProjectDetail(QSharedPointer<ICloudFormationTemplateModel> templateModel)
{
    if (!m_view->m_cloudFormationTemplateDetailWidgets.contains(templateModel))
    {
        m_view->m_cloudFormationTemplateDetailWidgets.insert(templateModel, new CloudFormationTemplateDetailWidget(m_view, templateModel));
    }
    m_detailWidget = m_view->m_cloudFormationTemplateDetailWidgets[templateModel];
}

void ResourceManagementView::DetailWidgetFinder::VisitProjectDetail(QSharedPointer<ICloudFormationTemplateModel> templateModel, const QString& resourceSection)
{
    auto thisPair = qMakePair(templateModel, resourceSection);
    if (!m_view->m_cloudFormationTemplateResourceDetailWidgets.contains(thisPair))
    {
        m_view->m_cloudFormationTemplateResourceDetailWidgets.insert(thisPair, new CloudFormationTemplateResourceDetailWidget(m_view, templateModel, resourceSection));
    }
    m_detailWidget = m_view->m_cloudFormationTemplateResourceDetailWidgets[thisPair];
}


void ResourceManagementView::DetailWidgetFinder::VisitProjectDetail(QSharedPointer<IProjectStatusModel> projectStatusModel)
{
    if (!m_view->m_ProjectDetailWidget)
    {
        m_view->m_ProjectDetailWidget = new ProjectDetailWidget {
            m_view, projectStatusModel
        };
    }
    m_detailWidget = m_view->m_ProjectDetailWidget;
}

void ResourceManagementView::DetailWidgetFinder::VisitProjectDetail(QSharedPointer<IDeploymentStatusModel> deploymentStatusModel)
{
    if (!m_view->m_deploymentDetailWidgets.contains(deploymentStatusModel))
    {
        m_view->m_deploymentDetailWidgets.insert(deploymentStatusModel, new DeploymentDetailWidget(m_view, deploymentStatusModel));
    }
    m_detailWidget = m_view->m_deploymentDetailWidgets[deploymentStatusModel];
}

void ResourceManagementView::DetailWidgetFinder::VisitProjectDetail(QSharedPointer<IResourceGroupStatusModel> resourceGroupStatusModel)
{
    if (!m_view->m_resourceGroupDetailWidgets.contains(resourceGroupStatusModel))
    {
        m_view->m_resourceGroupDetailWidgets.insert(resourceGroupStatusModel, new ResourceGroupDetailWidget(m_view, resourceGroupStatusModel, m_view));
    }
    m_detailWidget = m_view->m_resourceGroupDetailWidgets[resourceGroupStatusModel];
}

void ResourceManagementView::DetailWidgetFinder::VisitProjectDetail(QSharedPointer<IDeploymentListStatusModel> deploymentListStatusModel)
{
    if (!m_view->m_deploymentListDetailWidget)
    {
        m_view->m_deploymentListDetailWidget = new DeploymentListDetailWidget(m_view, deploymentListStatusModel);
    }
    m_detailWidget = m_view->m_deploymentListDetailWidget;
}

void ResourceManagementView::DetailWidgetFinder::VisitProjectDetail(QSharedPointer<IResourceGroupListStatusModel> resourceGroupListStatusModel)
{
    if (!m_view->m_resourceGroupListDetailWidget)
    {
        m_view->m_resourceGroupListDetailWidget = new ResourceGroupListDetailWidget(m_view, resourceGroupListStatusModel, m_view);
    }
    m_detailWidget = m_view->m_resourceGroupListDetailWidget;
}

void ResourceManagementView::DetailWidgetFinder::VisitProjectDetail(QSharedPointer<ICodeDirectoryModel> codeDirectoryModel)
{
    if (!m_view->m_codeDirectoryDetailWidgets.contains(codeDirectoryModel))
    {
        m_view->m_codeDirectoryDetailWidgets.insert(codeDirectoryModel, new CodeDirectoryDetailWidget(m_view, codeDirectoryModel));
    }
    m_detailWidget = m_view->m_codeDirectoryDetailWidgets[codeDirectoryModel];
}

void ResourceManagementView::OnTreeContextMenuRequested(QPoint pos)
{
    auto index = m_treeView->indexAt(pos);

    if (!index.isValid())
    {
        return;
    }

    auto detailWidget = m_detailWidgetFinder.FindDetailWidget(index);

    auto menu = detailWidget->GetTreeContextMenu();

    if (menu && !menu->isEmpty())
    {
        menu->popup(m_treeView->viewport()->mapToGlobal(pos));
    }
}

void ResourceManagementView::OnHideSearchBox()
{
    HideSearchFrame();
}

void ResourceManagementView::OnNewButtonClicked()
{
    auto buttonGeometry = m_newButton->geometry();
    auto popupPos = m_newButton->parentWidget()->mapToGlobal(buttonGeometry.bottomLeft());
    popupPos += QPoint {
        10, -1
    };
    m_newMenu->popup(popupPos);
}

void ResourceManagementView::OnImportButtonClicked()
{
    auto buttonGeometry = m_importButton->geometry();
    auto popupPos = m_importButton->parentWidget()->mapToGlobal(buttonGeometry.bottomLeft());
    popupPos += QPoint {
        10, -1
    };
    CreateImportResourceMenu(this)->popup(popupPos);
}

void ResourceManagementView::OnMenuImportUsingArn()
{
    //Require necessary information to import the resource
    PopupDialogWidget dialog{
        this
    };
    dialog.setWindowTitle(tr("Import using ARN"));

    QPushButton* primaryButton = dialog.GetPrimaryButton();
    primaryButton->show();
    primaryButton->setEnabled(false);
    primaryButton->setToolTip("Please provide the resource Name and ARN");

    QString resourceGroup = GetSelectedResourceGroupName();

    m_nameEdit = new QLineEdit;
    m_nameEdit->setParent(&dialog);
    m_nameEdit->setToolTip("The name used for the resource in the resource group.");
    dialog.AddWidgetPairRow(new QLabel{ tr("Resource name") }, m_nameEdit);
    m_nameEdit->setFocus();
    connect(m_nameEdit, &QLineEdit::textEdited, this, [this, primaryButton](){OnImportResourceInfoGiven(primaryButton); });

    m_arnEdit = new QLineEdit;
    m_arnEdit->setParent(&dialog);
    m_arnEdit->setToolTip("ARN of the AWS resource to import.");
    dialog.AddWidgetPairRow(new QLabel{ tr("Resource ARN") }, m_arnEdit);
    connect(m_arnEdit, &QLineEdit::textEdited, this, [this, primaryButton](){OnImportResourceInfoGiven(primaryButton); });

    dialog.GetPrimaryButton()->setText(tr("Import"));

    dialog.setMinimumWidth(dialog.width() * 3);

    connect(m_importerModel.data(), &IAWSImporterModel::ImporterOutput, this, &ResourceManagementView::OnImporterOutput);

    if (dialog.exec() == QDialog::Accepted)
    {
        QString resourceName = m_nameEdit->text();
        QString resourceArn = m_arnEdit->text();
        DoImportResource(resourceGroup, resourceName, resourceArn);
    }
}

void ResourceManagementView::OnImporterOutput(const QVariant& output, const char* outputType)
{
    disconnect(m_importerModel.data(), &IAWSImporterModel::ImporterOutput, this, &ResourceManagementView::OnImporterOutput);
    if (QString(outputType) == "error")
    {
        ShowBasicErrorBox("Import Error", output.toString());
        return;
    }
}

void ResourceManagementView::OnImportResourceInfoGiven(QPushButton* primaryButton)
{
    if (m_nameEdit->text() == "" || m_arnEdit->text() == "")
    {
        primaryButton->setEnabled(false);
        primaryButton->setToolTip("Please provide the resource Name and ARN");
        return;
    }

    primaryButton->setEnabled(true);
    primaryButton->setToolTip("Import the resource");
}

void ResourceManagementView::DoImportResource(const QString& resourceGroup, const QString& resourceName, const QString& resourceArn)
{
    m_importerModel->ImportResource(resourceGroup, resourceName, resourceArn);
}

void ResourceManagementView::OnMenuImportFromList()
{
    QString resourceGroup = GetSelectedResourceGroupName();

    ImportableResourcesWidget dialog{ resourceGroup, m_importerModel, this };
    dialog.exec();
}

template<typename DialogT>
static void AddResourceMenuItem(QMenu* menu, QWidget* parent, ResourceManagementView* view, const QSharedPointer<ICloudFormationTemplateModel>& templateModel, const char* title, const char* toolTip)
{
    auto addItem = menu->addAction(title);
    addItem->setToolTip(toolTip);
    view->connect(addItem, &QAction::triggered, view, [templateModel, parent, view](bool checked)
    {
        if (auto existing = parent->findChild<CreateResourceDialog*>(CREATE_RESOURCE_DIALOG_NAME, Qt::FindChildrenRecursively))
        {
            delete existing;
        }

        CreateResourceDialog* pResource = new DialogT{ parent, templateModel, view };
        pResource->setObjectName(CREATE_RESOURCE_DIALOG_NAME);
        pResource->show();
    });
}

QMenu* ResourceManagementView::CreateAddResourceMenu(QSharedPointer<ICloudFormationTemplateModel> templateModel, QWidget* parent)
{
    auto menu = new QMenu {};
    menu->setToolTip("Select an AWS resource to add to your resource group.");

    AddResourceMenuItem<AddDynamoDBTable>(menu, parent, this, templateModel, "DynamoDB table", "Add a DynamoDB table resource.");
    AddResourceMenuItem<AddLambdaFunction>(menu, parent, this, templateModel, "Lambda function", "Add a Lambda function resource.");
    AddResourceMenuItem<AddS3Bucket>(menu, parent, this, templateModel, "S3 bucket", "Add an S3 bucket resource.");
    AddResourceMenuItem<AddSNSTopic>(menu, parent, this, templateModel, "SNS topic", "Add an SNS topic resource.");
    AddResourceMenuItem<AddSQSQueue>(menu, parent, this, templateModel, "SQS queue", "Add an SQS queue resource.");

    auto addServiceApi = menu->addAction("Service API");
    addServiceApi->setToolTip("Add a service API resource.");
    connect(addServiceApi, &QAction::triggered, this, &ResourceManagementView::AddServiceApi);

    return menu;
}

void ResourceManagementView::AddServiceApi()
{

    auto callback = [this](const QString& errorMessage)
    {
        if (!errorMessage.isEmpty())
        {
            QMessageBox::critical(this, "Error", errorMessage);
        }
    };

    QString resourceGroupName = GetSelectedResourceGroupName();
    m_projectModel->AddServiceApi(resourceGroupName, callback);

}

QMenu* ResourceManagementView::CreateImportResourceMenu(QWidget* parent)
{
    auto menu = new QMenu{};
    menu->setToolTip("Import existing resources");

    auto importUsingArn = menu->addAction("Import using ARN");
    importUsingArn->setToolTip("Import a resource by providing its ARN.");
    connect(importUsingArn, &QAction::triggered, this, &ResourceManagementView::OnMenuImportUsingArn);

    auto importFromList = menu->addAction("Import from list");
    importFromList->setToolTip("Import a resource from a list of importable resources.");
    connect(importFromList, &QAction::triggered, this, &ResourceManagementView::OnMenuImportFromList);

    return menu;
}

void ResourceManagementView::SetAddResourceMenuTemplateModel(QSharedPointer<ICloudFormationTemplateModel> templateModel)
{
    m_menuNewResource->setMenu(CreateAddResourceMenu(templateModel, this));
    m_menuNewResource->setEnabled(true);
}

void ResourceManagementView::OnOperationInProgressChanged()
{
    m_menuNewDeployment->setDisabled(m_resourceManager->IsOperationInProgress());
}

QString ResourceManagementView::GetSelectedResourceGroupName()
{
    QStandardItem* selectedResourceGroup = FindSelectedItemResourceGroup();
    return m_projectModel->indexFromItem(selectedResourceGroup).data().toString();
}

/*
* Small wrapper class to fix Ux issue with QTreeView by handling Enter/Return key differently
*/
void RMVTreeView::keyPressEvent(QKeyEvent * event)
{
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
    {
        QModelIndex curInd = currentIndex();
        if (isExpanded(curInd))
        {
            collapse(curInd);
        }
        else
        {
            expand(curInd);
        }
    }
    else
    {
        QTreeView::keyPressEvent(event);
    }
}

