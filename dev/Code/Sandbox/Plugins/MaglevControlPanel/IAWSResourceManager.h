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

#include "IAWSResourceManagerModel.h"

#include <QSharedPointer>
#include <QObject>
#include <QDateTime>
#include <QVector>

#include <assert.h>

#include <AzCore/std/functional.h>

class QSortFilterProxyModel;

class IStackStatusModel;
class IFileSourceControlModel;
class PythonWorker;
class SourceControlFileNode;

/////////////////////////////////////////////////////////////////////////////////
// AWS Importer Model

class IAWSImporterModel
    : public QStandardItemModel
{
    Q_OBJECT

public:

    enum Columns
    {
        CheckableResourceColumn,
        ResourceColumn,
        TypeColumn,
        ArnColumn,
        NameColumn,

        SelectedStateColumn
    };

    virtual void ImportResource(const QString& resourceGroup, const QString& resourceName, const QString& resourceArn) = 0;
    virtual void ListImportableResources(QString region) = 0;
    
signals:

    void ImporterOutput(const QVariant& output, const char* outputType);
    void ListFinished();
};

///////////////////////////////////////////////////////////////////////////////
// AWS Deployment Model

enum class AWSDeploymentColumn
{
    Name,                   // QString - deployment name
    Default,                // bool - true if this is the default deployment. The deployment is also either the user
                            //        default or the project default. The user default deployment takes precedence over
                            //        the project default deployment.
    UserDefault,            // bool - true if this is the user's default deployment
    ProjectDefault,         // bool - true if this is the project's default deployment
    Release,                // bool - true if this is the release deployment
    PhysicalResourceId,     // QString - the deployment stacks arn. May be null.
    ResourceStatus,         // QString - the deployment stack's cloud formation status (CREATE_COMPLETE, etc.). May be null.
    Timestamp,              // QDateTime - the time the deployment stack's last event. May be null.
    Protected               // bool - true if this deployment is protected from development use
};

class IAWSDeploymentModel
    : public IAWSResourceManagerModel<AWSDeploymentColumn>
{
public:
    virtual void SetUserDefaultDeployment(const QString& deploymetName) = 0;
    virtual void SetProjectDefaultDeployment(const QString& deploymetName) = 0;
    virtual void SetReleaseDeployment(const QString& deploymentName) = 0;
    virtual void ExportLauncherMapping(const QString& deploymentName) = 0;
    virtual void ProtectDeployment(const QString& deploymentName, bool set) = 0;
    virtual QString GetActiveDeploymentName() const = 0;
    virtual bool IsActiveDeploymentSet() const = 0;
    virtual QString GetActiveDeploymentRegion() const = 0;
};

///////////////////////////////////////////////////////////////////////////////
// AWS Profile Model

enum class AWSProfileColumn
{
    Name,       // QString -- profile name
    SecretKey,  // QString -- the AWS secret key
    AccessKey,  // QString -- the AWS access key
    Default     // bool -- True if this the user's default profile.
};

class IAWSProfileModel
    : public IAWSResourceManagerModel<AWSProfileColumn>
{
    Q_OBJECT

public:

    virtual void SetDefaultProfile(const QString& profileName) = 0;
    virtual QString GetDefaultProfile() const = 0;
    virtual void AddProfile(const QString& profileName, const QString& secretKey, const QString& accessKey, bool makeDefault) = 0;
    virtual void UpdateProfile(const QString& oldName, const QString& newName, const QString& secretKey, const QString& accessKey) = 0;
    virtual void DeleteProfile(const QString& profileName) = 0;

    virtual bool AWSCredentialsFileExists() const = 0;

signals:

    void AddProfileSucceeded();
    void AddProfileFailed(const QString& message);

    void UpdateProfileSucceeded();
    void UpdateProfileFailed(const QString& message);

    void DeleteProfileSucceeded();
    void DeleteProfileFailed(const QString& message);
};


///////////////////////////////////////////////////////////////////////////////

class ICodeDirectoryModel
    : public QStandardItemModel
{
public:

    enum Columns
    {
        PathColumn
    };

    QString Path() const { return index(0, 0).data().toString(); }

    enum Roles
    {
        // bool - true if this directory should not be deleted. For
        // example, the project-code and lambda-function-code directories.
        DoNotDeleteRole = Qt::UserRole
    };

    bool DoNotDelete() const { return index(0, 0).data(DoNotDeleteRole).toBool(); }
};

///////////////////////////////////////////////////////////////////////////////

class IFileContentModel
    : public QStandardItemModel
{
public:

    enum Columns
    {
        ContentColumn = 0
    };

    enum Roles
    {
        ModifiedRole = Qt::UserRole,    // bool: true if content set since save or reload
        FileTimestampRole,              // QDateTime: file last modified time
        PathRole,                       // QString: the full file path

        // bool - true if this file should not be deleted. For
        // example, the project-settings.json, project-template.json,
        // etc.
        DoNotDeleteRole,

        NextRole
    };

    // Returns the index of the file content.
    QModelIndex ContentIndex() const { return index(0, ContentColumn); }

    // Save content to disk.
    virtual bool Save() = 0;

    // Read content from disk. Content that was previously set but not saved is lost.
    virtual void Reload() = 0;

    // Get the content.
    virtual QString GetContent() const { return ContentIndex().data(Qt::DisplayRole).toString(); }

    // Set the content. Does not save to disk.
    virtual void SetContent(const QString& content) { setData(ContentIndex(), content, Qt::DisplayRole); }

    // Determines if the content has been set since last saved or reloaded.
    bool IsModified() const { return data(ContentIndex(), ModifiedRole).toBool(); }

    void SetModified(bool modified) { setData(ContentIndex(), modified, ModifiedRole); }

    QString Path() const { return data(ContentIndex(), PathRole).toString(); }

    bool DoNotDelete() const { return index(0, 0).data(DoNotDeleteRole).toBool(); }

    virtual QSharedPointer<IStackStatusModel> GetStackStatusModel() const = 0;
};

///////////////////////////////////////////////////////////////////////////////

class ICloudFormationTemplateModel
    : public IFileContentModel
{
public:

    enum Roles
    {
        // TBD describing the error(s) that occurred when validating the cached
        // content. Use ValidateContent to begin the async validation process.
        // When this process is complete, a dataChanged signal is emitted for
        // this role.
        ValidationErrorRole = IFileContentModel::NextRole
    };

    // Validates the current content.
    virtual void ValidateContent() = 0;

    virtual bool SetResourceContent(const QString& resourceName, const QString& contentData) = 0;
    virtual QString GetResourceContent(const QString& resourceName, bool addRelated) const = 0;
    virtual bool DeleteResource(const QString& resourceName) = 0;
    virtual bool ReadResources(const QString& resourceName) = 0;
    virtual bool AddResource(const QString& resourceName, const QVariantMap& variantMap) = 0;
    virtual bool AddLambdaAccess(const QString& resourceName, const QString& functionName) = 0;
    virtual bool AddParameters(const QVariantMap& variantMap) = 0;
    virtual bool HasResource(const QString& resourceName) = 0;
    virtual bool CanAddResources() = 0;
    virtual void SetCanAddResources(bool canAdd) = 0;

};

///////////////////////////////////////////////////////////////////////////////

class ICloudFormationResourceModel
    : public ICloudFormationTemplateModel
{
public:

    enum Roles
    {
        // TBD describing the error(s) that occurred when validating the cached
        // content. Use ValidateContent to begin the async validation process.
        // When this process is complete, a dataChanged signal is emitted for
        // this role.
        ValidationErrorRole = IFileContentModel::NextRole
    };

    // Validates the current content.
    virtual void ValidateContent() = 0;
};

///////////////////////////////////////////////////////////////////////////////

class IStackEventsModel
    : public QStandardItemModel
{
    Q_OBJECT

public:

    enum Columns
    {
        OperationColumn,
        StatusColumn,
        TimeColumn,

        ColumnCount
    };

    enum CommandStatus
    {
        NoCommandStatus,
        CommandInProgress,
        CommandSucceeded,
        CommandFailed
    };

    virtual CommandStatus GetCommandStatus() const = 0;

    bool IsCommandInProgress() const { return GetCommandStatus() == CommandInProgress; }

signals:

    void CommandStatusChanged(CommandStatus commandStatus);
};

///////////////////////////////////////////////////////////////////////////////

class IStackResourcesModel
    : public QStandardItemModel
{
    Q_OBJECT

public:

    enum Columns
    {

        PendingActionColumn,
        ChangeImpactColumn,
        NameColumn,
        ResourceTypeColumn,
        ResourceStatusColumn,
        TimestampColumn,
        PhysicalResourceIdColumn,

        ColumnCount

    };

    virtual void Refresh(bool force = false) = 0;
    virtual bool IsRefreshing() const = 0;

    virtual bool IsReady() const = 0; // true until after first refresh is complete

    virtual QSharedPointer<QSortFilterProxyModel> GetPendingChangeFilterProxy() = 0;
    virtual bool ContainsChanges() = 0;
    virtual bool ContainsDeletionChanges() = 0;
    virtual bool ContainsSecurityChanges() = 0;
    virtual QString GetStackResourceRegion() const = 0;

signals:

    void RefreshStatusChanged(bool refreshing);
};

///////////////////////////////////////////////////////////////////////////////

class IStackStatusModel
    : public QStandardItemModel
{
    Q_OBJECT

public:

    enum Columns
    {

        PendingActionColumn,
        StackStatusColumn,
        CreationTimeColumn,
        LastUpdatedTimeColumn,
        StackIdColumn,

        NextColumn
    };

    enum Roles
    {
        ActualValueRole = Qt::UserRole
    };

    virtual QSharedPointer<IStackEventsModel> GetStackEventsModel() const = 0;
    virtual QSharedPointer<IStackResourcesModel> GetStackResourcesModel() const = 0;

    virtual void Refresh(bool force = false) = 0; // happens in background
    virtual bool IsRefreshing() const = 0; // true if currently refreshing

    virtual bool IsReady() const = 0; // true until after first refresh is complete

    bool StackExists() const { return !StackId().isEmpty(); }
    QString StackId() const { return index(0, StackIdColumn).data(ActualValueRole).toString(); }
    QString StackStatus() const { return index(0, StackStatusColumn).data(ActualValueRole).toString(); }
    QDateTime CreationTime() const { return index(0, CreationTimeColumn).data().toDateTime(); }
    QDateTime LastUpdatedTime() const { return index(0, LastUpdatedTimeColumn).data().toDateTime(); }
    QString PendingAction() const { return index(0, PendingActionColumn).data().toString(); }
    bool IsPendingDelete() const { return PendingAction() == "Delete"; }
    bool IsPendingCreate() const { return PendingAction() == "Create"; }
    bool IsPendingUpdate() const { return PendingAction() == "Update"; }

    virtual bool UpdateStack() = 0;
    virtual bool UploadLambdaCode(QString functionName) { return false; };
    virtual bool DeleteStack() = 0;
    virtual bool CanDeleteStack() const = 0;

    virtual bool StackIsBusy() const = 0;

    virtual QString GetUpdateButtonText() const = 0;
    virtual QString GetUpdateButtonToolTip() const = 0;
    virtual QString GetUpdateConfirmationTitle() const = 0;
    virtual QString GetUpdateConfirmationMessage() const = 0;
    virtual QString GetStatusTitleText() const = 0;
    virtual QString GetMainMessageText() const = 0;
    virtual QString GetDeleteButtonText() const = 0;
    virtual QString GetDeleteButtonToolTip() const = 0;
    virtual QString GetDeleteConfirmationTitle() const = 0;
    virtual QString GetDeleteConfirmationMessage() const = 0;

signals:

    void RefreshStatusChanged(bool refreshing);
    void StackBusyStatusChanged(bool stackIsBusy);
};

///////////////////////////////////////////////////////////////////////////////

class IProjectStatusModel
    : public IStackStatusModel
{
public:

    enum Rows
    {
        ColumnCount = IStackStatusModel::NextColumn
    };
};

///////////////////////////////////////////////////////////////////////////////

class IDeploymentStatusModel
    : public IStackStatusModel
{
public:

    enum Rows
    {
        DeploymentNameColumn = IStackStatusModel::NextColumn,

        ColumnCount
    };

    virtual QString GetExportMappingButtonText() const = 0;
    virtual QString GetExportMappingToolTipText() const = 0;

    virtual QString GetProtectedMappingCheckboxText() const = 0;
    virtual QString GetProtectedMappingToolTipText() const = 0;


    QString DeploymentName() const { return index(0, DeploymentNameColumn).data().toString(); }
};

///////////////////////////////////////////////////////////////////////////////

class IResourceGroupStatusModel
    : public IStackStatusModel
{
    Q_OBJECT

public:

    enum Columns
    {
        ActiveDeploymentNameColumn = IStackStatusModel::NextColumn,
        ResourceGroupNameColumn,
        UserDefinedResourceCountColumn,

        ColumnCount
    };

    QString ActiveDeploymentName() const { return index(0, ActiveDeploymentNameColumn).data().toString(); }
    QString ResourceGroupName() const { return index(0, ResourceGroupNameColumn).data().toString(); }
    int UserDefinedResourceCount() const { return index(0, UserDefinedResourceCountColumn).data().toInt(); }
    bool ContainsUserDefinedResources() const { return UserDefinedResourceCount() > 0; }

    virtual QSharedPointer<ICloudFormationTemplateModel> GetTemplateModel() = 0;

    virtual QString GetEnableButtonText() const = 0;
    virtual QString GetEnableButtonToolTip() const = 0;
    virtual bool EnableResourceGroup() = 0;

signals:

    void ActiveDeploymentChanged(const QString& activeDeploymentName);
};

///////////////////////////////////////////////////////////////////////////////

class IStackStatusListModel
    : public QStandardItemModel
{
    Q_OBJECT

public:

    enum Columns
    {

        PendingActionColumn,
        NameColumn,
        ResourceStatusColumn,
        TimestampColumn,
        PhysicalResourceIdColumn,

        NextColumn
    };

    virtual void Refresh(bool force = false) = 0;
    virtual bool IsRefreshing() const = 0;

    virtual bool IsReady() const = 0; // true until after first refresh is complete

    virtual QString GetMainTitleText() const = 0;
    virtual QString GetMainMessageText() const = 0;
    virtual QString GetListTitleText() const = 0;
    virtual QString GetListMessageText() const = 0;

signals:

    void RefreshStatusChanged(bool refreshing);
};

///////////////////////////////////////////////////////////////////////////////

class IDeploymentListStatusModel
    : public IStackStatusListModel
{
public:

    enum Columns
    {
        UserDefaultColumn = NextColumn, // bool - true if this is the user default deployment
        ProjectDefaultColumn,           // bool - true if is the project wide default deployment
        ReleaseColumn,                  // bool - true if is the release deployment
        ActiveColumn,                   // bool - true if this is the active deployment

        ColumnCount
    };

    virtual QString GetAddButtonText() const = 0;
    virtual QString GetAddButtonToolTip() const = 0;
};

///////////////////////////////////////////////////////////////////////////////

class IResourceGroupListStatusModel
    : public IStackStatusListModel
{
    Q_OBJECT

public:

    enum Columns
    {
        ResourceGroupTemplateFilePathColumn = NextColumn,
        LambdaFunctionCodeDirectoryPathColumn,

        ColumnCount
    };

    virtual QSharedPointer<IDeploymentStatusModel> GetActiveDeploymentStatusModel() const = 0;

    virtual QString GetUpdateButtonText() const = 0;
    virtual QString GetUpdateButtonToolTip() const = 0;
    virtual QString GetAddButtonText() const = 0;
    virtual QString GetAddButtonToolTip() const = 0;

signals:

    void ActiveDeploymentChanged(const QString& activeDeploymentName);
};

///////////////////////////////////////////////////////////////////////////////

enum class AWSProjectColumn
{
    Node
};

class IAWSProjectDetailVisitor
{
public:

    virtual void VisitProjectDetail(QSharedPointer<IFileContentModel>) { VisitProjectDetailNone(); }
    virtual void VisitProjectDetail(QSharedPointer<ICloudFormationTemplateModel>) { VisitProjectDetailNone(); }
    virtual void VisitProjectDetail(QSharedPointer<ICloudFormationTemplateModel>, const QString& resourceSection) { VisitProjectDetailNone(); }
    virtual void VisitProjectDetail(QSharedPointer<IAWSDeploymentModel>) { VisitProjectDetailNone(); }
    virtual void VisitProjectDetail(QSharedPointer<IProjectStatusModel>) { VisitProjectDetailNone(); }
    virtual void VisitProjectDetail(QSharedPointer<IDeploymentStatusModel>) { VisitProjectDetailNone(); }
    virtual void VisitProjectDetail(QSharedPointer<IResourceGroupStatusModel>) { VisitProjectDetailNone(); }
    virtual void VisitProjectDetail(QSharedPointer<IDeploymentListStatusModel>) { VisitProjectDetailNone(); }
    virtual void VisitProjectDetail(QSharedPointer<IResourceGroupListStatusModel>) { VisitProjectDetailNone(); }
    virtual void VisitProjectDetail(QSharedPointer<ICodeDirectoryModel>) { VisitProjectDetailNone(); }
    virtual void VisitProjectDetailNone() {}
};

class IAWSProjectModel
    : public IAWSResourceManagerModel<AWSProjectColumn>
{
    Q_OBJECT

public:
    
    enum Roles
    {
        PathRole = Qt::UserRole    // QString: file/directory path for file/directory types
    };

    using AsyncOperationCallback = AZStd::function<void(const QString& errorMessage)>;

    virtual void VisitDetail(const QModelIndex& index, IAWSProjectDetailVisitor* visitor) = 0;

    virtual QModelIndex ResourceGroupListIndex() const = 0;
    virtual QModelIndex DeploymentListIndex() const = 0;
    virtual QModelIndex ProjectStackIndex() const = 0;

    virtual void AddResourceGroup(const QString& resourceGroupName, bool includeExampleResources, AsyncOperationCallback callback) = 0;
    virtual void DisableResourceGroup(const QString& resourceGroupName, AsyncOperationCallback callback) = 0;
    virtual void EnableResourceGroup(const QString& resourceGroupName, AsyncOperationCallback callback) = 0;

    virtual void AddServiceApi(const QString& resourceGroupName, AsyncOperationCallback callback) = 0;

    virtual void CreateDeploymentStack(const QString& deploymentName, bool shouldMakeProjectDefault) = 0;
    virtual void DeleteDeploymentStack(const QString& deploymentName) = 0;

    virtual QSharedPointer<IResourceGroupStatusModel> ResourceGroupStatusModel(const QString& resourceGroupName) = 0;
    virtual QSharedPointer<IResourceGroupStatusModel> ResourceGroupStatusModel(const QModelIndex& index) = 0;
    virtual QSharedPointer<IDeploymentStatusModel> DeploymentStatusModel(const QString& deploymentName) = 0;
    virtual QSharedPointer<IProjectStatusModel> ProjectStatusModel() = 0;

    // Helpers to grab file names for check out
    virtual QString GetProjectSettingsFile() const = 0;
    virtual QString GetDeploymentTemplateFile() const = 0;
    virtual QString GetGemsFile() const = 0;

    enum class ResourceGroupSettingPriority
    {
        GAME_OR_BASE,
        BASE_OR_GAME,
        GAME,
        BASE
    };
    // 
    virtual QJsonValue GetResourceGroupSetting(const QString& resourceGroupName, const QString& settingName, ResourceGroupSettingPriority settingPriority = ResourceGroupSettingPriority::GAME_OR_BASE) const = 0;

    virtual QVector<QString> GetWritableFilesforUploadResources() const = 0;

    virtual void CreateFunctionFolder(const QString& functionName, const QString& resourceGroupName, AsyncOperationCallback callback) const = 0;

signals:
    void RequestCheckoutWriteCheckResources();
    void FileSourceStatusUpdated(const QString& path);

    void FindResourceGroupSetting(const QString& resourceGroupName, const QString& settingName, ResourceGroupSettingPriority settingPriority);

    void ResourceGroupAdded(const QModelIndex& index);
    void DeploymentAdded(const QModelIndex& index);
    void PathAdded(const QModelIndex& index, const QString& path);
    void ResourceAdded(const QModelIndex& index);
};

///////////////////////////////////////////////////////////////////////////////

class IAWSResourceManager
    : public QObject
{
    Q_OBJECT

public:

    virtual QSharedPointer<IAWSImporterModel> GetImporterModel() = 0;
    virtual QSharedPointer<IAWSDeploymentModel> GetDeploymentModel() = 0;
    virtual QSharedPointer<IAWSProfileModel> GetProfileModel() = 0;
    virtual QSharedPointer<IAWSProjectModel> GetProjectModel() = 0;
    virtual QSharedPointer<IFileSourceControlModel> GetProjectSettingsSourceModel() = 0;
    virtual QSharedPointer<IFileSourceControlModel> GetDeploymentTemplateSourceModel() = 0;
    virtual QSharedPointer<IFileSourceControlModel> GetGemsFileSourceModel() = 0;

    enum InitializationState
    {
        UnknownState,
        InitializingState,
        InitializedState,
        UninitializedState,
        NoProfileState,          // there is a project stack id, but the user has no default profile setup.
        ErrorLoadingState
    };

    virtual bool IsProjectInitialized() const = 0;

    virtual InitializationState GetInitializationState() = 0;
    virtual QString GetErrorString() const = 0;
    virtual void RetryLoading() = 0;

    virtual void GetRegionList() = 0;
    virtual bool InitializeProject(const QString& region, const QString& stackName, const QString& accessKey = "", const QString& secretKey = "") = 0;

    virtual void RequestEditProjectSettings() = 0;
    virtual void RequestEditDeploymentTemplate() = 0;
    virtual void RequestEditGemsFile() = 0;
    virtual void RequestAddFile(const QString& path) = 0;
    virtual void RequestDeleteFile(const QString& path) = 0;

    virtual bool ProjectSettingsNeedsCheckout() const = 0;
    virtual bool DeploymentTemplateNeedsCheckout() const = 0;
    virtual bool GemsFileNeedsCheckout() const = 0;

    virtual void UpdateSourceControlStates() = 0;

    virtual bool IsOperationInProgress() = 0;

    virtual void OpenCGP() = 0;

Q_SIGNALS:

    void ProjectInitializedChanged();
    void OperationInProgressChanged();
    void SupportedRegionsObtained(QStringList regionList);
};

