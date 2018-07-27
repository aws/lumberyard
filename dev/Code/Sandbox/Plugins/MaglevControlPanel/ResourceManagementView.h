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


#include <QDialog>
#include <QMainWindow>
#include <QString>
#include <QTextEdit>
#include <QSharedPointer>
#include <QMenu>
#include <QHelpEvent>
#include <QToolTip>
#include <QTreeView>
#include <QJsonObject>

#include <IAWSResourceManager.h>
#include "EditorCoreAPI.h"

#include "AzCore/Math/Guid.h"

class QHBoxLayout;
class QItemSelection;
class QLineEdit;
class QSplitter;
class QStandardItem;
class QVBoxLayout;
class QLabel;
class QPushButton;

class FilePathLabel;

class ActiveDeployment;
class ProfileSelector;

class DetailWidget;
class EmptyDetailWidget;
class TextDetailWidget;
class FileContentDetailWidget;
class CloudFormationTemplateDetailWidget;
class ProjectDetailWidget;
class DeploymentDetailWidget;
class ResourceGroupDetailWidget;
class DeploymentListDetailWidget;
class ResourceGroupListDetailWidget;
class NoProfileDetailWidget;
class CodeDirectoryDetailWidget;
class ProjectInitializationStateDetailWidget;

/*
* Small wrapper class to fix Ux issue with QTreeView by handling Enter/Return key differently
*/
class RMVTreeView
    : public QTreeView
{
    Q_OBJECT

public:
    RMVTreeView(QWidget* parent = 0) : QTreeView(parent) {}

    virtual void keyPressEvent(QKeyEvent * event);
};
/**
* View class for the Resource Management window
*
*/
class ResourceManagementView
    : public QMainWindow
{
    Q_OBJECT

public:

    // This was too big for current tech, resource names were getting created that were too long and truncating
    // the stack name and causing errors. Eventually we should figure out a way to support longer names
    // but this will do for now to prevent customers from seeing hard to figure out errors
    static const int maxStackNameLength = 20;
    static const int cSearchFrameWidth = 400;

    ResourceManagementView(QWidget* parent = nullptr);

    static const char* GetWindowTitle() { return "Resource Manager"; }

    static const GUID& GetClassID()
    {
        static const GUID guid =
        {
            0x51491275, 0xf142, 0x41ae, { 0xb1, 0x36, 0x83, 0xde, 0xe2, 0xbb, 0x5, 0x85 }
        };
        return guid;
    }

    static bool ValidateStackNameLength(const QString& stackName);
    static bool ValidateStackNameFormat(const QString& stackName);

    // Disallows hyphens
    static bool ValidateStackNameNoHyphen(const QString& stackName);

    static void ShowBasicErrorBox(const QString& boxTitle, const QString& warningText);

    QMenu* CreateAddResourceMenu(QSharedPointer<ICloudFormationTemplateModel> templateModel, QWidget* parent);
    QMenu* CreateImportResourceMenu(QWidget* parent);
    void SetAddResourceMenuTemplateModel(QSharedPointer<ICloudFormationTemplateModel> templateModel);

    QPushButton* EnableSaveButton(const QString& toolTip);
    void DisableSaveButton(const QString& toolTip);

    QPushButton* GetDeleteButton() const;
    QPushButton* EnableDeleteButton(const QString& toolTip);
    void DisableDeleteButton(const QString& toolTip);

    static const char* GetSourceCheckoutString() { return " needs to be checked out in order to perform this operation.  Check out now?"; }

    IAWSResourceManager* GetResourceManager() { return m_resourceManager; }

    QSharedPointer<IAWSDeploymentModel> GetDeploymentModel() { return m_deploymentModel; }

    QString GetDefaultProfile() { return (m_profileModel->GetDefaultProfile()); }

    // Given a resource type name and field name, provides a validation regexp and a help string explaining the validation requirements
    bool GetResourceValidationData(const QString& resourceType, const QString& resourceField, QString& outRegEx, QString& outHelp, int& outMinLen);

    void SendUpdatedSourceStatus(const QString& filePath);

    QString GetSelectedResourceGroupName();

public slots:

    void OnCurrentDeploymentClicked();
    void OnCurrentProfileClicked();

    void OnTreeViewSelectionChanged(const QItemSelection& firstSelection, const QItemSelection& secondSelection);
    void OnTreeContextMenuRequested(QPoint pos);

    void OnDeploymentAdded(const QModelIndex& deploymentName);
    void OnResourceGroupAdded(const QModelIndex& resourceGroupName);
    void OnPathAdded(const QModelIndex& index, const QString& path);
    void OnUpdatingStack(const QModelIndex& index);

    void OnMenuImportUsingArn();
    void OnImportResourceInfoGiven(QPushButton* primaryButton);
    void OnImporterOutput(const QVariant& output, const char* outputType);
    void OnMenuImportFromList();
    void OnImportButtonClicked();
    
    void OnUpdateResourceGroupButtonGainedFocus();
    void OnUpdateResourceGroupButtonLostFocus();

    void OnResourceHelpActivated(const QString& path);

    void OnProjectInitializedChanged();

    void OnMenuNewResourceGroup();
    void OnMenuNewDeployment(bool required = false);
    void OnMenuSwitchProfile();
    void OnMenuSwitchDeployment();
    void OnCreateProjectStack();
    void OnMenuCloudCanvasHelp();

    void SourceUpdatedCreateResourceGroup(const QString& resourceGroupName, bool includeExample);
    void SourceChangedCreateResourceGroup(const QString& resourceGroupName, bool includeExample);

    void OnProfileModelReset();
    void OnDeploymentModelReset();

    void OnSaveAll();

    void OnNewButtonClicked();

    void OnHideSearchBox();

    bool UpdateStack(const QSharedPointer<IStackStatusModel>& model);
    bool UploadLambdaCode(const QSharedPointer<IStackStatusModel>& model, QString functionName);
    bool DeleteStack(const QSharedPointer<IStackStatusModel>& model);

    void OnTextEditResized(const QSize& editSize);

    void OnOperationInProgressChanged();
    void DoImportResource(const QString& resourceGroup, const QString& resourceName, const QString& resourceArn);


private:

    /****   View creation calls  ****/

    // Create or recreate our QT window from scratch
    void Init();

    // Row 1
    QMenuBar* CreateMenuBar();
    void CreateStatusLabels(QHBoxLayout* horizontalLayout);

    // Row 2
    void CreateButtonRow(QVBoxLayout* verticalLayout);
    void AddToolBar(QHBoxLayout* verticalLayout);

    // Row 3
    void CreateBottomRow(QVBoxLayout* verticalLayout);
    void AddTreeView(QSplitter* verticalSplitter);
    void AddTextEditBox(QSplitter* verticalSplitter);

    // Helpers to add generic blank frame spacers
    void AddVerticalSpacer(QHBoxLayout* horizontalLayout);
    void AddHorizontalSpacer(QVBoxLayout* verticalLayout);

    //Helpers to create our current profile and deployment labels
    QString GetDeploymentLabel() const;
    QString GetProfileLabel() const;
    QString GetProjectName() const;

    bool HasDefaultProfile() const;
    bool HasAnyProfiles() const;

    void SetDynamicConnections();

    QString GetDeploymentNameInvalidLengthError(const QString& deploymentName) const;
    QString GetDeploymentNameInvalidFormatError(const QString& deploymentName) const;

    QString GetResourceGroupNameInvalidLengthError(const QString& resourceGroupName) const;
    QString GetResourceGroupNameInvalidFormatError(const QString& resourceGroupName) const;

    void ShowActiveDeploymentWindow();
    QSharedPointer<ActiveDeployment> m_activeDeploymentDialog;

    void ShowCurrentProfileWindow();
    QSharedPointer<ProfileSelector> m_currentProfile;

    void CreateSearchFrame();
    void SetParentFrame(QWidget* parentBox);
    void HideSearchFrame();
    void ShowSearchFrame();

    enum class SourceControlState
    {
        QUERYING = 0,
        UNAVAILABLE_CHECK_OUT,
        ENABLED_CHECK_OUT,
        DISABLED_CHECK_IN,
        DISABLED_ADD,
        ENABLED_ADD,
        NOT_APPLICABLE
    };
    void SetSourceControlState(SourceControlState newState, const QString& tooltipOverride = {});

    void ValidateSourceCreateResourceGroup(const QString& resourceGroupName, bool includeExample);
    void DoCreateResourceGroup(const QString& resourceGroupName, bool includeExample);

    void AddServiceApi();

    // Helper to find the Resource Group corresponding to the currently selected item
    QStandardItem* FindSelectedItemResourceGroup() const;

    // Resource Manager
    IAWSResourceManager* m_resourceManager;

    // Models
    QSharedPointer<IAWSImporterModel> m_importerModel;
    QSharedPointer<IAWSProjectModel> m_projectModel;
    QSharedPointer<IAWSDeploymentModel> m_deploymentModel;
    QSharedPointer<IAWSProfileModel> m_profileModel;

    // Button Bar
    using ToolbarButton = QPushButton;
    ToolbarButton* m_saveButton {
        nullptr
    };
    ToolbarButton* m_deleteButton {
        nullptr
    };
    ToolbarButton* m_sourceControlButton {
        nullptr
    };
    ToolbarButton* m_newButton {
        nullptr
    };
    ToolbarButton* m_importButton{
        nullptr
    };
    ToolbarButton* m_validateTemplateButton {
        nullptr
    };

    // Menu actions
    QAction* m_menuSave {
        nullptr
    };
    QAction* m_menuSaveAs {
        nullptr
    };
    QAction* m_menuCut {
        nullptr
    };
    QAction* m_menuCopy {
        nullptr
    };
    QAction* m_menuPaste {
        nullptr
    };
    QAction* m_menuUndo {
        nullptr
    };
    QAction* m_menuRedo {
        nullptr
    };
    QMenu* m_newMenu {
        nullptr
    };
    QAction* m_menuNewResource {
        nullptr
    };
    QAction* m_menuNewResourceGroup {
        nullptr
    };
    QAction* m_menuNewDeployment {
        nullptr
    };
    QAction* m_menuNewFile {
        nullptr
    };
    QAction* m_menuNewDirectory {
        nullptr
    };

    // Deployment and Profile Labels
    QLabel* m_currentDeploymentLabel {
        nullptr
    };
    QLabel* m_currentProfileLabel {
        nullptr
    };

    // Search
    QAction* m_menuSearch {
        nullptr
    };
    QAction* m_findNextShortcut{
        nullptr
    };
    QAction* m_findPrevShortcut{
        nullptr
    };
    QAction* m_hideSearchShortcut{
        nullptr
    };
    QAction* m_saveShortcut{
        nullptr
    };
    QLineEdit* m_editSearchBox {
        nullptr
    };
    QPushButton* m_clearSearchBox {
        nullptr
    };
    QPushButton* m_hideSearchBox {
        nullptr
    };
    QPushButton* m_findPrevious {
        nullptr
    };
    QPushButton* m_findNext {
        nullptr
    };
    QLabel* m_searchLabel {
        nullptr
    };
    QFrame* m_searchFrame {
        nullptr
    };

    // Tree View
	RMVTreeView* m_treeView {
        nullptr
    };

    //Importer
    QLineEdit* m_nameEdit{
        nullptr
    };
    QLineEdit* m_arnEdit{
        nullptr
    };

    // Add Deployment/ResourceGroup State
    QString m_waitingForResourceGroup;
    QString m_waitingForDeployment;
    QString m_waitingForPath;

    // Resource name validation data stored in @engroot@/Gems/CloudGemFramework/v1/ResourceManager/resource_manager/config/aws_name_validation_rules.json
    QJsonObject m_resourceValidation;
    void LoadResourceValidationData();

    // Detail Widgets

    class DetailWidgetFinder
        : public IAWSProjectDetailVisitor
    {
    public:

        DetailWidgetFinder(ResourceManagementView* view);

        DetailWidget* FindDetailWidget(const QModelIndex& index);

        using IAWSProjectDetailVisitor::VisitProjectDetail;

        void VisitProjectDetail(QSharedPointer<IFileContentModel> model) override;
        void VisitProjectDetail(QSharedPointer<ICloudFormationTemplateModel> model, const QString& resourceSection) override;
        void VisitProjectDetail(QSharedPointer<ICloudFormationTemplateModel> model) override;
        void VisitProjectDetail(QSharedPointer<IProjectStatusModel> model) override;
        void VisitProjectDetail(QSharedPointer<IDeploymentStatusModel> model) override;
        void VisitProjectDetail(QSharedPointer<IResourceGroupStatusModel> model) override;
        void VisitProjectDetail(QSharedPointer<IDeploymentListStatusModel> model) override;
        void VisitProjectDetail(QSharedPointer<IResourceGroupListStatusModel> model) override;
        void VisitProjectDetail(QSharedPointer<ICodeDirectoryModel> model) override;
        void VisitProjectDetailNone() override;

    private:

        ResourceManagementView* m_view;
        DetailWidget* m_detailWidget;
    };

    friend class ResourceManagementView::DetailWidgetFinder;

    DetailWidgetFinder m_detailWidgetFinder {
        this
    };

    void SelectDetailWidget();
    void ShowDetailWidget(DetailWidget* widget);
    QSplitter* m_verticalSplitter {
        nullptr
    };

    // This will reference one of the children of m_verticalSplitter;
    DetailWidget* m_currentDetailWidget {
        nullptr
    };

    // These widgets are children of m_verticalSplitter. It owns them
    // and it is responsible for deleting them.
    //
    // TODO: Currently nothing is cleaning up models and widgets that
    // may no longer be needed. After a long editing session that hit
    // a lot of files, this could become a problem.

    DetailWidget* m_emptyDetailWidget {
        nullptr
    };
    ProjectDetailWidget* m_ProjectDetailWidget {
        nullptr
    };
    QMap<QSharedPointer<IFileContentModel>, FileContentDetailWidget*> m_fileContentDetailWidgets;
    QMap<QSharedPointer<ICloudFormationTemplateModel>, CloudFormationTemplateDetailWidget*> m_cloudFormationTemplateDetailWidgets;
    QMap<QPair<QSharedPointer<ICloudFormationTemplateModel>, QString>, CloudFormationTemplateDetailWidget*> m_cloudFormationTemplateResourceDetailWidgets;
    QMap<QSharedPointer<IDeploymentStatusModel>, DeploymentDetailWidget*> m_deploymentDetailWidgets;
    QMap<QSharedPointer<IResourceGroupStatusModel>, ResourceGroupDetailWidget*> m_resourceGroupDetailWidgets;
    DeploymentListDetailWidget* m_deploymentListDetailWidget {
        nullptr
    };
    ResourceGroupListDetailWidget* m_resourceGroupListDetailWidget {
        nullptr
    };
    QMap<QSharedPointer<ICodeDirectoryModel>, CodeDirectoryDetailWidget*> m_codeDirectoryDetailWidgets;

    QSharedPointer<IStackStatusModel> m_pendingUpdate;
    bool m_pendingCreateDeployment {
        false
    };
    bool ConfirmCreateProjectStack();
    bool ConfirmIsAdmin(const QString& title, const QString& message);
    bool m_adminConfirmed {
        false
    };

    // TODO: factor out an interface for these? Currently the widgets
    // have full access to the view. They need to behave themselves.
    friend class DetailWidget;
    friend class EmptyDetailWidget;
    friend class TextDetailWidget;
    friend class FileContentDetailWidget;
    friend class CloudFormationTemplateDetailWidget;
    friend class CloudFormationTemplateResourceDetailWidget;
    friend class StackStatusDetailWidget;
    friend class ProjectDetailWidget;
    friend class DeploymentListDetailWidget;
    friend class ResourceGroupDetailWidget;
    friend class ResourceGroupListDetailWidget;
    friend class UnitializedProjectDetailWidget;
    friend class NoProfileDetailWidget;
    friend class CodeDirectoryDetailWidget;
    friend class ProjectInitializationStateDetailWidget;
    friend class StackListWidget;
    friend class StackResourcesWidget;

    static const int SEARCH_LIGHTER_VALUE;
    static const int DETAIL_STRETCH_FACTOR;
    static const int TREE_STRETCH_FACTOR;
    static const int CREATE_DEPLOYMENT_DIALOG_MINIMUM_HEIGHT;
    static const int CREATE_DEPLOYMENT_DIALOG_MINIMUM_WIDTH;
};

class ToolTipMenu
    : public QMenu
{
    Q_OBJECT

public:

    ToolTipMenu()
        : QMenu{}
    {}

    ToolTipMenu(const QString& title)
        : QMenu{title}
    {}

    bool event (QEvent* e)
    {
        if (e->type() == QEvent::ToolTip && activeAction() != 0 && activeAction()->text() != activeAction()->toolTip())
        {
            const QHelpEvent* helpEvent = static_cast <QHelpEvent*>(e);
            QToolTip::showText(helpEvent->globalPos(), activeAction()->toolTip());
        }
        else
        {
            QToolTip::hideText();
        }
        return QMenu::event(e);
    }
};


