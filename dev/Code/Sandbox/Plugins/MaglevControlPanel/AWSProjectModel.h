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

#include <QObject>
#include <QSharedPointer>
#include <QScopedPointer>
#include <QPersistentModelIndex>
#include <QVector>

#include <AWSResourceManagerModel.h>
#include <AWSResourceManager.h>

#include <FileSourceControlModel.h>

class Node;
class ResourceGroupListNode;
class DeploymentListNode;
class ConfigurationNode;
class ResourceGroupTemplateNode;
class ProjectSettingsNode;
class DeploymentTemplateNode;
class SourceControlFileNode;

class QStandardItemObject
    : public QObject
    , public QStandardItem
{
    Q_OBJECT
};

class AWSProjectModel
    : public AWSResourceManagerModel<IAWSProjectModel>
{
    Q_OBJECT

    typedef AWSResourceManagerModel<IAWSProjectModel> Base;

public:

    AWSProjectModel(AWSResourceManager* resourceManager);

    void ProcessOutputDeploymentList(const QVariant& value);
    void ProcessOutputResourceGroupList(const QVariant& value);
    void ProcessOutputProjectDescription(const QVariant& value);

    // IAWSProjectModel overrides
    QModelIndex ResourceGroupListIndex() const override;
    QModelIndex DeploymentListIndex() const override;
    QModelIndex ProjectStackIndex() const override;
    void VisitDetail(const QModelIndex& index, IAWSProjectDetailVisitor* visitor) override;
    void AddResourceGroup(const QString& resourceGroupName, bool includeExampleResources, AsyncOperationCallback callback) override;
    void AddServiceApi(const QString& resourceGroupName, AsyncOperationCallback callback) override;
    void DisableResourceGroup(const QString& resourceGroupName, AsyncOperationCallback callback) override;
    void EnableResourceGroup(const QString& resourceGroupName, AsyncOperationCallback callback) override;
    void CreateDeploymentStack(const QString& deploymentName, bool shouldMakeProjectDefault) override;
    void DeleteDeploymentStack(const QString& deploymentName) override;
    QSharedPointer<IResourceGroupStatusModel> ResourceGroupStatusModel(const QString& resourceGroupName) override;
    QSharedPointer<IResourceGroupStatusModel> ResourceGroupStatusModel(const QModelIndex& index) override;
    QSharedPointer<IDeploymentStatusModel> DeploymentStatusModel(const QString& deploymentName) override;
    QSharedPointer<IProjectStatusModel> ProjectStatusModel() override;

    QSharedPointer<ICloudFormationTemplateModel> GetResourceGroupTemplateModel(const QString& resourceGroupName);

    // Used to notify nodes that what they represent is being updated
    void OnResourceGroupInProgress(const QString& deploymentName, const QString& resourceGroupName);
    void OnAllResourceGroupsInProgress(const QString& deploymentName);
    void OnProjectStackInProgress();
    void OnDeploymentInProgress(const QString& deploymentName);

    // Helpers to grab file names for check out
    QString GetProjectSettingsFile() const override;
    QString GetDeploymentTemplateFile() const override;
    QString GetGemsFile() const override;

    QVector<QString> GetWritableFilesforUploadResources() const override;
    QJsonValue GetResourceGroupSetting(const QString& resourceGroupName, const QString& settingName, IAWSProjectModel::ResourceGroupSettingPriority settingPriority) const override;

    void CreateFunctionFolder(const QString& functionName, const QString& resourceGroupName, AsyncOperationCallback callback) const override;

signals:

    void RequestFilesForUploadResources(QVector<QString>& fileList) const;
    void RequestResourceGroupSetting(const QString& resourceGroupName, const QString& settingName, IAWSProjectModel::ResourceGroupSettingPriority settingPriority, QJsonValue& resultValue) const;

private:

    enum Rows
    {
        ResourceGroupListRow,
        ConfigurationRow,

        RowCount
    };

    template<typename T = Node>
    T* GetNode(const QModelIndex& index) const;

    ResourceGroupListNode* GetResourceGroupListNode() const;
    DeploymentListNode* GetDeploymentListNode() const;
    ConfigurationNode* GetConfigurationNode() const;

    ProjectSettingsNode* GetProjectSettingsNode() const;
    DeploymentTemplateNode* GetDeploymentTemplateNode() const;

    QString m_gemsFilePath;

    friend class Node;

};

///////////////////////////////////////////////////////////////////////////////

class Node
    : public QStandardItemObject
{
    Q_OBJECT

public:

    Node(const QString& name, AWSProjectModel* projectModel, const QIcon& icon);

    virtual void VisitDetail(IAWSProjectDetailVisitor* visitor);

    AWSProjectModel* ProjectModel() const;

    Node* parent();

    virtual QSharedPointer<IStackStatusModel> GetStackStatusModel();

    void SetIsUpdating(bool isUpdating);

    QString GetName() const;

protected:

    template<typename T = Node>
    T* GetChild(int row) const
    {
        return static_cast<T*>(child(row));
    }

    QString GetParentName() const;

    virtual bool ShouldAppearInProgress() const;
    void SetUpIcon();

    bool m_isUpdating;

private slots:

    void OnOperationChanged();
    void OnProgressIndicatorAnimationUpdate(int frame);

private:

    AWSProjectModel* m_projectModel;
    QIcon m_defaultIcon;
};

///////////////////////////////////////////////////////////////////////////////

class PathNode
    : public Node
{
    Q_OBJECT

public:

    PathNode(const QString& path, AWSProjectModel* projectModel, QIcon icon, bool doNotDelete = false);

    QString GetPath() const;

    bool GetDoNotDelete() const;

private:

    static QString GetFileName(const QString& path);

    bool m_doNotDelete;
};


///////////////////////////////////////////////////////////////////////////////

class FileNode
    : public PathNode
{
    Q_OBJECT
public:

    FileNode(const QString& path, AWSProjectModel* projectModel, bool doNotDelete = false);

    QVariant data(int role) const override;


    QSharedPointer<IFileContentModel> GetFileContentModel();

    void VisitDetail(IAWSProjectDetailVisitor* visitor) override;

private:

    QSharedPointer<IFileContentModel> m_fileContentModel;

    void OnFileContentModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
};


///////////////////////////////////////////////////////////////////////////////

class ResourceGroupNode
    : public Node
{
    Q_OBJECT

public:

    enum ChildNodes
    {
        ResourceGroupTemplateFilePathNode = 0,
        LambdaFunctionCodeDirectoryPathNode,
        CliPluginCodeDirectoryPathNode,
        ResourceGroupSettingsFileNode
    };

    ResourceGroupNode(const QVariantMap& map, AWSProjectModel* projectModel);

    QSharedPointer<IResourceGroupStatusModel> GetResourceGroupStatusModel();

    QSharedPointer<ICloudFormationTemplateModel> GetTemplateModel();

    void VisitDetail(IAWSProjectDetailVisitor* visitor) override;

    QSharedPointer<IStackStatusModel> GetStackStatusModel() override;

    void SetUpdatingDeploymentName(const QString& deploymentName);

    void UpdateLambdaFolders(const QVariantMap& map);

protected:

    bool ShouldAppearInProgress() const override;

private:

    void LoadResourceGroupSettings(const QString& filePath, IAWSProjectModel::ResourceGroupSettingPriority settingType = IAWSProjectModel::ResourceGroupSettingPriority::GAME_OR_BASE);
    QString m_resourceGroupName;
    ResourceGroupTemplateNode* m_templateNode {
        nullptr
    };
    bool m_isDeploymentUpdating;
    QString m_updatingDeployment; // If there's an update in progress, this is the name of the deployment being updated

private slots:

    void OnActiveDeploymentChanged(const QString& activeDeploymentName);
};

class SourceControlFileNode
    : public FileNode
{
    Q_OBJECT

public:
    SourceControlFileNode(const QString& path, AWSProjectModel* projectModel);

    void UpdateSourceControlStatus();
    void HandleRequestForUploadResourcesFiles(QVector<QString>& fileList);

    void OnFileSourceStatusUpdated(const QString& filePath);
    void CheckOutFile();
private:
    QSharedPointer<SourceControlStatusModel> m_sourceStatus;
};