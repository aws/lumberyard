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

#include <AWSProjectModel.h>

#include <AWSProjectModel.moc>

#include <QDir>
#include <QFont>
#include <QList>
#include <QMovie>
#include <QVector>

#include <assert.h>

#include <StandardItemChildManager.h>
#include <FileContentModel.h>
#include <CloudFormationTemplateModel.h>
#include <AWSResourceManager.h>
#include <DeploymentStatusModel.h>
#include <DeploymentListStatusModel.h>

#include <ResourceGroupStatusModel.h>
#include <ResourceGroupListStatusModel.h>
#include <ProjectStatusModel.h>
#include <ResourceParsing.h>

// EBUS to retrieve game folder
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
///////////////////////////////////////////////////////////////////////////////

template<>
const QMap<AWSProjectColumn, QString> ColumnEnumToNameMap<AWSProjectColumn>::s_columnEnumToNameMap =
{
    {
        AWSProjectColumn::Node, "Node"
    }
};

///////////////////////////////////////////////////////////////////////////////

namespace
{
    const QIcon g_fileIcon {
        "Editor/Icons/CloudCanvas/file_icon.png"
    };
    const QIcon g_folderIcon {
        "Editor/Icons/CloudCanvas/folder_icon.png"
    };
    const QIcon g_groupedResourcesIcon {
        "Editor/Icons/CloudCanvas/grouped_resources_icon.png"
    };
    const QIcon g_resourceIcon {
        "Editor/Icons/CloudCanvas/resource_icon.png"
    };
    QMovie g_inProgressMovie {
        "Editor/Icons/CloudCanvas/in_progress.gif"
    };
}

///////////////////////////////////////////////////////////////////////////////

Node::Node(const QString& name, AWSProjectModel* projectModel, const QIcon& icon)
    : QStandardItemObject{}
    , m_isUpdating{false}
    , m_projectModel{projectModel}
    , m_defaultIcon{icon}
{
    setData(name, Qt::DisplayRole);
    setIcon(icon);

    connect(m_projectModel->ResourceManager(), &AWSResourceManager::OperationInProgressChanged, this, &Node::OnOperationChanged);
}

void Node::VisitDetail(IAWSProjectDetailVisitor* visitor)
{
    visitor->VisitProjectDetailNone();
}

AWSProjectModel* Node::ProjectModel() const
{
    return m_projectModel;
}

Node* Node::parent()
{
    return static_cast<Node*>(QStandardItem::parent());
}

QSharedPointer<IStackStatusModel> Node::GetStackStatusModel()
{
    if (parent())
    {
        return parent()->GetStackStatusModel();
    }
    else
    {
        return QSharedPointer<IStackStatusModel>{};
    }
}

void Node::SetIsUpdating(bool isUpdating)
{
    if (m_isUpdating == isUpdating)
    {
        return;
    }

    m_isUpdating = isUpdating;

    SetUpIcon();
}

QString Node::GetName() const
{
    return data(Qt::DisplayRole).toString();
}

QString Node::GetParentName() const
{
    const Node* parentNode = azdynamic_cast<const Node*>(QStandardItem::parent());
    if (parentNode)
    {
        return parentNode->GetName();
    }
    return{};
}

void Node::OnOperationChanged()
{
    // Only check for transition to not-in-progress. Transitions to in-progress are set explicitly.
    if (!m_projectModel->ResourceManager()->IsOperationInProgress())
    {
        SetIsUpdating(false);
    }
}

bool Node::ShouldAppearInProgress() const
{
    return m_isUpdating;
}

void Node::SetUpIcon()
{
    auto iconToSet = ShouldAppearInProgress() ? QIcon {
        g_inProgressMovie.currentPixmap()
    } : m_defaultIcon;
    setIcon(iconToSet);

    // Only listen for frame change events when actually updating.
    // Note that we *always* want to listen when an update is in progress, even if !ShouldAppearInProgress()
    if (m_isUpdating)
    {
        connect(&g_inProgressMovie, &QMovie::frameChanged, this, &Node::OnProgressIndicatorAnimationUpdate);
    }
    else
    {
        disconnect(&g_inProgressMovie, &QMovie::frameChanged, this, &Node::OnProgressIndicatorAnimationUpdate);
    }
}

void Node::OnProgressIndicatorAnimationUpdate(int frame)
{
    if (ShouldAppearInProgress())
    {
        setIcon(QIcon { g_inProgressMovie.currentPixmap() });
    }
}

///////////////////////////////////////////////////////////////////////////////

PathNode::PathNode(const QString& path, AWSProjectModel* projectModel, QIcon icon, bool doNotDelete)
    : Node{ GetFileName(path), projectModel, icon }
    , m_doNotDelete{ doNotDelete }
{
    assert(!path.isEmpty());
    setData(path, AWSProjectModel::PathRole);
}

QString PathNode::GetPath() const
{
    return data(AWSProjectModel::PathRole).toString();
}

bool PathNode::GetDoNotDelete() const
{
    return m_doNotDelete;
}

QString PathNode::GetFileName(const QString& path)
{
    QFileInfo fileInfo{
        path
    };
    return fileInfo.fileName();
}


///////////////////////////////////////////////////////////////////////////////

FileNode::FileNode(const QString& path, AWSProjectModel* projectModel, bool doNotDelete)
    : PathNode{ path, projectModel, g_fileIcon, doNotDelete }
{
}

QVariant FileNode::data(int role) const
{
    auto result = PathNode::data(role);
    if (result.isValid() && m_fileContentModel && m_fileContentModel->IsModified())
    {
        switch (role)
        {
        case Qt::DisplayRole:
            result = result.toString() + "*";
            break;

        case Qt::FontRole:
            auto font = result.value<QFont>();
            font.setItalic(true);
            result = font;
            break;
        }
    }
    return result;
}

QSharedPointer<IFileContentModel> FileNode::GetFileContentModel()
{
    if (!m_fileContentModel)
    {
        // If this node object is deleted before the FileContentModel and this
        // lambda is then called by the FileContentModel things will blow up. We
        // don't expect that to happen (but if your reading this it may have).
        //
        // We use a lambda for this because GetStackStatusModel searches up the
        // parent hierarcy to find a node that provides a StackStatusModel. If
        // that search is done before the node is added to the tree then the
        // search doesn't work. This situation occurs when GetFileContentModel
        // is called by the CloudFormationTemplateNode's constructor.
        m_fileContentModel.reset(new FileContentModel{ GetPath(), GetDoNotDelete(), [this]() { return GetStackStatusModel(); } });
        QObject::connect(m_fileContentModel.data(), &FileContentModel::dataChanged, this, &FileNode::OnFileContentModelDataChanged, Qt::QueuedConnection);
    }
    return m_fileContentModel;
}

void FileNode::VisitDetail(IAWSProjectDetailVisitor* visitor)
{
    visitor->VisitProjectDetail(GetFileContentModel());
}


void FileNode::OnFileContentModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    if (roles.contains(FileContentModel::ModifiedRole))
    {
        emitDataChanged();
    }
}

SourceControlFileNode::SourceControlFileNode(const QString& path, AWSProjectModel* projectModel)
    : FileNode{ path, projectModel, true /* do not delete */ }
{
    QObject::connect(projectModel, &AWSProjectModel::RequestFilesForUploadResources, this, &SourceControlFileNode::HandleRequestForUploadResourcesFiles, Qt::DirectConnection);
    QObject::connect(projectModel, &IAWSProjectModel::RequestCheckoutWriteCheckResources, this, &SourceControlFileNode::CheckOutFile);
    QObject::connect(projectModel, &IAWSProjectModel::FileSourceStatusUpdated, this, &SourceControlFileNode::OnFileSourceStatusUpdated);

    setToolTip("File node with source control monitoring");

    m_sourceStatus.reset(new SourceControlStatusModel{ });

    UpdateSourceControlStatus();
}

void SourceControlFileNode::CheckOutFile()
{
    QSharedPointer<SourceControlStatusModel> statusPtr = m_sourceStatus;
    using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
    SCCommandBus::Broadcast(&SCCommandBus::Events::RequestEdit, GetPath().toStdString().c_str(), true, [statusPtr](bool wasSuccess, const AzToolsFramework::SourceControlFileInfo& fileInfo)
    {
        statusPtr->SetFlags(fileInfo.m_flags);
        statusPtr->SetStatus(fileInfo.m_status);
    }
    );
}

void SourceControlFileNode::OnFileSourceStatusUpdated(const QString& filePath)
{
    if (filePath == GetPath())
    {
        UpdateSourceControlStatus();
    }
}

void SourceControlFileNode::UpdateSourceControlStatus()
{
    QSharedPointer<SourceControlStatusModel> statusPtr = m_sourceStatus;
    using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
    SCCommandBus::Broadcast(&SCCommandBus::Events::GetFileInfo, GetPath().toStdString().c_str(), [statusPtr](bool wasSuccess, const AzToolsFramework::SourceControlFileInfo& fileInfo)
    {
        statusPtr->SetFlags(fileInfo.m_flags);
        statusPtr->SetStatus(fileInfo.m_status);
    }
    );
}

void SourceControlFileNode::HandleRequestForUploadResourcesFiles(QVector<QString>& fileList)
{
    if (m_sourceStatus->FileNeedsCheckout())
    {
        fileList.push_back(GetPath());
    }
}

class ResourceGroupSettingsNode
    : public FileNode
{
public:

    ResourceGroupSettingsNode(const QString& path, AWSProjectModel* projectModel, IAWSProjectModel::ResourceGroupSettingPriority settingType)
        : FileNode{ path, projectModel, true /* do not delete */ },
        m_settingType{settingType}
    {
        setToolTip(
            "File containing configuration settings for this resource group.");

        if (settingType == IAWSProjectModel::ResourceGroupSettingPriority::BASE)
        {
            setData("(Base)" + GetName(), Qt::DisplayRole);
        }
        else if (settingType == IAWSProjectModel::ResourceGroupSettingPriority::GAME)
        {
            setData("(Game)" + GetName(), Qt::DisplayRole);
        }

        QObject::connect(GetFileContentModel().data(), &FileContentModel::dataChanged, this, &ResourceGroupSettingsNode::FileContentChanged);
        QObject::connect(projectModel, &AWSProjectModel::RequestResourceGroupSetting, this, &ResourceGroupSettingsNode::HandleRequestResourceGroupSetting, Qt::DirectConnection);

        LoadSettings();
        LoadChildren();
    }

    void FileContentChanged()
    {
        if (rowCount())
        {
            removeRows(0, rowCount());
        }
        LoadSettings();
        LoadChildren();
    }

    void LoadChildren()
    {
        QVector<QString> myList = ResourceParsing::ParseStringList(GetSettingsMap(), "ValidateWritableOnUploadResources");

        const char* resultValue = nullptr;
        EBUS_EVENT_RESULT(resultValue, AzToolsFramework::AssetSystemRequestBus, GetAbsoluteDevGameFolderPath);
        QString gameFolder{ resultValue };

        for (auto thisElement : myList)
        {
            QString testFile = gameFolder + "/" + thisElement;
            if (!testFile.length())
            {
                return;
            }
            QFileInfo fileInfo{ testFile };
            if (!fileInfo.exists())
            {
                return;
            }
            appendRow(new SourceControlFileNode{ testFile, ProjectModel() });
        }
    }

    void HandleRequestResourceGroupSetting(const QString& resourceGroupName, const QString& settingName, IAWSProjectModel::ResourceGroupSettingPriority settingPriority, QJsonValue& resultValue)
    {
        QString parentName = GetParentName();
        // We verify we belong to this resource group by it being our parent
        if (parentName != resourceGroupName)
        {
            return;
        }
        if (settingPriority != m_settingType)
        {
            return;
        }

        const QVariantMap& settingsMap = GetSettingsMap();
        QVariant settingsValue = settingsMap.value(settingName);

        if (!settingsValue.isNull())
        {
            resultValue = QJsonValue::fromVariant(settingsValue);
        }
    }

    const QVariantMap& GetSettingsMap() const
    {
        return m_settings;
    }

private:
    void LoadSettings()
    {
        m_settings = ResourceParsing::ParseResourceSettingsMap(GetFileContentModel()->GetContent().toUtf8());
    }
    QVariantMap m_settings;
    IAWSProjectModel::ResourceGroupSettingPriority m_settingType;
};

///////////////////////////////////////////////////////////////////////////////

template<typename ChildDirectoryNodeType, typename ChildFileNodeType>
class DirectoryNode
    : public PathNode
{
public:

    DirectoryNode(const QString& path, AWSProjectModel* projectModel, bool doNotDelete = false)
        : PathNode{path, projectModel, g_folderIcon, doNotDelete}
        , m_fileSystemWatcher{}
    {
        connect(&m_fileSystemWatcher, &QFileSystemWatcher::directoryChanged, this, &DirectoryNode::OnDirectoryChanged);
        m_fileSystemWatcher.addPath(path);

        UpdateChildren();
    }


protected:

    QFileSystemWatcher m_fileSystemWatcher;

    void OnDirectoryChanged(const QString& path)
    {
        UpdateChildren();
    }

    class ChildManager
        : public StandardItemChildManager<DirectoryNode, PathNode, QFileInfo>
    {
    public:

        using StandardItemChildManager<DirectoryNode, PathNode, QFileInfo>::Parent;
        using StandardItemChildManager<DirectoryNode, PathNode, QFileInfo>::CompareStrings;

        ChildManager(DirectoryNode* parentItem)
            : StandardItemChildManager<DirectoryNode, PathNode, QFileInfo>(parentItem)
        {
        }

        PathNode* Create(const QFileInfo& source) override
        {
            if (source.isDir())
            {
                return new ChildDirectoryNodeType {
                           QDir::toNativeSeparators(source.absoluteFilePath()), Parent()->ProjectModel()
                };
            }
            else
            {
                return new ChildFileNodeType {
                           QDir::toNativeSeparators(source.absoluteFilePath()), Parent()->ProjectModel()
                };
            }
        }

        void Update(const QFileInfo& source, PathNode* target) override
        {
            // Nothing needs to be done. If the deployment name changes nodes are added
            // and removed, not updated.
        }

        int Compare(const QFileInfo& source, const PathNode* targetNode) const override
        {
            QFileInfo target {
                targetNode->GetPath()
            };

            if (source.isDir())
            {
                if (target.isDir())
                {
                    // both dirs, compare names
                    return CompareStrings(source.fileName(), target.fileName());
                }
                else
                {
                    // all dirs come before any files
                    return -1;
                }
            }
            else
            {
                if (target.isDir())
                {
                    // all dirs come before any files
                    return 1;
                }
                else
                {
                    // both files, compare names
                    return CompareStrings(source.fileName(), target.fileName());
                }
            }
        }

        void ChildAdded(PathNode* newChild) override
        {
            Parent()->ProjectModel()->PathAdded(newChild->index(), newChild->GetPath());
        }
    };

    ChildManager m_childManager {
        this
    };

    void UpdateChildren()
    {
        // TODO: read file system on a background thread and monitor for changes. The
        // plan is to proxy an QFileSystemModel and let it do all the work. This is just
        // a temporary solution.

        QDir dir {
            GetPath()
        };
        if (!GetPath().isEmpty() && dir.exists()) // QDir{QString{}}.exists() returns true!
        {
            QFileInfoList fullList;

            auto dirEntries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (auto it = dirEntries.constBegin(); it != dirEntries.constEnd(); ++it)
            {
                fullList.append(*it);
            }

            auto fileEntries = dir.entryInfoList(QDir::Files);
            for (auto it = fileEntries.constBegin(); it != fileEntries.constEnd(); ++it)
            {
                fullList.append(*it);
            }

            m_childManager.UpdateChildren(fullList);
        }
    }
};


///////////////////////////////////////////////////////////////////////////////

class ResourceNode
    : public Node
{
public:

    ResourceNode(const QString& resourceName, QSharedPointer<ICloudFormationTemplateModel> sharedModel, AWSProjectModel* projectModel)
        : Node{resourceName, projectModel, g_resourceIcon}
        , m_cloudFormationResourceModel{sharedModel}
    {
        setToolTip(
            "AWS resource defined by an AWS CloudFormation stack template.");
    }

    QSharedPointer<ICloudFormationTemplateModel> GetCloudFormationResourceModel()
    {
        return m_cloudFormationResourceModel;
    }

    void VisitDetail(IAWSProjectDetailVisitor* visitor) override
    {
        visitor->VisitProjectDetail(GetCloudFormationResourceModel(), GetName());
    }

    QSharedPointer<ICloudFormationTemplateModel> m_cloudFormationResourceModel;
};

///////////////////////////////////////////////////////////////////////////////

class CloudFormationTemplateFileNode
    : public FileNode
{
public:

    CloudFormationTemplateFileNode(const QString& path, AWSProjectModel* projectModel)
        : FileNode{path, projectModel, true /* do not delete */}
    {
        setToolTip(
            "AWS CloudFormation stack template file in JSON format.");

        QObject::connect(GetFileContentModel().data(), &FileContentModel::dataChanged, this, &CloudFormationTemplateFileNode::FileContentChanged);

        ParseResourceNodes();
    }

    QSharedPointer<ICloudFormationTemplateModel> GetCloudFormationTemplateModel()
    {
        if (!m_cloudFormationTemplateModel)
        {
            m_cloudFormationTemplateModel.reset(new CloudFormationTemplateModel {GetFileContentModel()});
        }
        m_cloudFormationTemplateModel->SetCanAddResources(CanAddResources());
        return m_cloudFormationTemplateModel;
    }

    void FileContentChanged()
    {
        auto resourceGroupStatusModel = ProjectModel()->ResourceGroupStatusModel(index());
        if (resourceGroupStatusModel)
        {
            resourceGroupStatusModel->Refresh();
        }
        ParseResourceNodes();
    }

    void ParseResourceNodes()
    {
        m_childManager.UpdateChildren(ResourceParsing::ParseResourceList(GetFileContentModel()->GetContent().toUtf8()));
    }

    void VisitDetail(IAWSProjectDetailVisitor* visitor) override
    {
        visitor->VisitProjectDetail(GetCloudFormationTemplateModel());
    }

    virtual bool CanAddResources() { return false; }
private:

    class ChildManager
        : public StandardItemChildManager<CloudFormationTemplateFileNode, ResourceNode, QString>
    {
    public:

        ChildManager(CloudFormationTemplateFileNode* parentItem)
            : StandardItemChildManager{parentItem}
        {
        }

        ResourceNode* Create(const QString& source) override
        {
            return new ResourceNode {
                       source, Parent()->GetCloudFormationTemplateModel(), Parent()->ProjectModel()
            };
        }

        void Update(const QString& source, ResourceNode* target) override
        {
        }

        int Compare(const QString& source, const ResourceNode* target) const override
        {
            return CompareVariants(source, target->data(Qt::DisplayRole));
        }

        void ChildAdded(ResourceNode* newChild) override
        {
            Parent()->ProjectModel()->ResourceAdded(newChild->index());
        }
    };

    ChildManager m_childManager {
        this
    };

    QSharedPointer<ICloudFormationTemplateModel> m_cloudFormationTemplateModel;
};


///////////////////////////////////////////////////////////////////////////////

class CodeFileNode
    : public FileNode
{
public:

    CodeFileNode(const QString& path, AWSProjectModel* projectModel)
        : FileNode{path, projectModel}
    {
        setToolTip(
            "File used in the implementation of an AWS Lambda function resource.");
    }
};


///////////////////////////////////////////////////////////////////////////////

class CodeDirectoryNode
    : public DirectoryNode<CodeDirectoryNode, CodeFileNode>
{
public:

    CodeDirectoryNode (const QString& path, AWSProjectModel* projectModel, bool doNotDelete = false)
        : DirectoryNode(path, projectModel, doNotDelete)
    {
        setToolTip(
            "Directory used in the implementation of an AWS Lambda function resource.");
    }

    QSharedPointer<ICodeDirectoryModel> GetCodeDirectoryModel()
    {
        if (!m_codeDirectoryModel)
        {
            m_codeDirectoryModel.reset(new CodeDirectoryModel {GetPath(), GetDoNotDelete()});
        }
        return m_codeDirectoryModel;
    }

    void VisitDetail(IAWSProjectDetailVisitor* visitor) override
    {
        visitor->VisitProjectDetail(GetCodeDirectoryModel());
    }

private:

    QSharedPointer<ICodeDirectoryModel> m_codeDirectoryModel;
    bool m_isRoot;

    class CodeDirectoryModel
        : public ICodeDirectoryModel
    {
    public:

        CodeDirectoryModel(const QString& path, bool doNotDelete)
        {
            appendRow(new QStandardItem {path});
            item(0)->setData(doNotDelete, DoNotDeleteRole);
        }
    };
};


///////////////////////////////////////////////////////////////////////////////

class DeploymentNode
    : public Node
{
public:

    DeploymentNode(const QVariantMap& map, AWSProjectModel* projectModel)
        : Node{map["Name"].toString(), projectModel, g_groupedResourcesIcon}
        , m_deploymentName{map["Name"].toString()}
    {
        setToolTip(
            "An independent copy of all the AWS resources needed by the Lumberyard project.");
    }

    QSharedPointer<IDeploymentStatusModel> GetDeploymentStatusModel()
    {
        return ProjectModel()->ResourceManager()->GetDeploymentStatusModel(m_deploymentName);
    }

    void VisitDetail(IAWSProjectDetailVisitor* visitor) override
    {
        visitor->VisitProjectDetail(GetDeploymentStatusModel());
    }

    QSharedPointer<IStackStatusModel> GetStackStatusModel() override
    {
        return GetDeploymentStatusModel();
    }

private:

    QString m_deploymentName;
};


///////////////////////////////////////////////////////////////////////////////

class DeploymentListNode
    : public Node
{
public:

    DeploymentListNode(AWSProjectModel* projectModel)
        : Node{"Deployments", projectModel, g_folderIcon}
    {
        setToolTip(
            "Deployments each contain an independent copy of all the AWS resources needed by the Lumberyard project.");
    }

    virtual QSharedPointer<QAbstractItemModel> GetDetailModel()
    {
        return ProjectModel()->ResourceManager()->GetDeploymentModel();
    }

    void ProcessOutputDeploymentList(const QVariant& value)
    {
        auto list = value.toMap()["Deployments"].toList();
        AWSProjectModel::Sort(list, "Name");

        m_childManager.UpdateChildren(list);
    }

    void AddDeployment(const QString& deploymentName)
    {
        QVariantMap map;
        map["Name"] = deploymentName;
        m_childManager.InsertChild(map);
    }

    void RemoveDeployment(const QString& deploymentName)
    {
        QVariantMap map;
        map["Name"] = deploymentName;
        m_childManager.RemoveChild(map);
    }

    void VisitDetail(IAWSProjectDetailVisitor* visitor) override
    {
        visitor->VisitProjectDetail(ProjectModel()->ResourceManager()->GetDeploymentListStatusModel().staticCast<IDeploymentListStatusModel>());
    }

    DeploymentNode* GetDeploymentNode(const QString& deploymentName)
    {
        QVariantMap source;
        source["Name"] = deploymentName;
        return m_childManager.FindChild(source);
    }

private:

    class ChildManager
        : public StandardItemChildManager<DeploymentListNode, DeploymentNode>
    {
    public:

        ChildManager(DeploymentListNode* parentItem)
            : StandardItemChildManager(parentItem)
        {
        }

        DeploymentNode* Create(const QVariant& source) override
        {
            return new DeploymentNode {
                       source.toMap(), Parent()->ProjectModel()
            };
        }

        void Update(const QVariant& source, DeploymentNode* target) override
        {
            // Nothing needs to be done. If the deployment name changes nodes are added
            // and removed, not updated.
        }

        int Compare(const QVariant& source, const DeploymentNode* target) const override
        {
            return CompareVariants(source.toMap()["Name"], target->data(Qt::DisplayRole));
        }

        void ChildAdded(DeploymentNode* newChild) override
        {
            Parent()->ProjectModel()->DeploymentAdded(newChild->index());
        }
    };

    ChildManager m_childManager {
        this
    };
};


///////////////////////////////////////////////////////////////////////////////

class ResourceGroupTemplateNode
    : public CloudFormationTemplateFileNode
{
public:

    ResourceGroupTemplateNode(const QString& path, AWSProjectModel* projectModel)
        : CloudFormationTemplateFileNode{path, projectModel}
    {
        setToolTip(
            "AWS CloudFormation stack template that defines a set of related AWS resources that are used by the Lumberyard project.");
    }

    virtual bool CanAddResources() { return true; }
};


///////////////////////////////////////////////////////////////////////////////

class LambdaFunctionCodeNode
    : public CodeDirectoryNode
{
public:

    LambdaFunctionCodeNode(const QString& path, AWSProjectModel* projectModel)
        : CodeDirectoryNode{path, projectModel, true /* do not delete */}
    {
        setToolTip(
            "Directory containing code that implements an AWS Lambda function resource.");
    }
};

///////////////////////////////////////////////////////////////////////////////

class CliPluginCodeDirectoryNode
    : public CodeDirectoryNode
{
public:

    CliPluginCodeDirectoryNode(const QString& path, AWSProjectModel* projectModel)
        : CodeDirectoryNode{path, projectModel, true /* do not delete */}
    {
        setToolTip(
            "Directory containing code which supports the lmbr_aws command line tool.");
    }
};


///////////////////////////////////////////////////////////////////////////////

ResourceGroupNode::ResourceGroupNode(const QVariantMap& map, AWSProjectModel* projectModel)
    : Node{map["Name"].toString(), projectModel, g_groupedResourcesIcon}
    , m_resourceGroupName{map["Name"].toString()}
    , m_isDeploymentUpdating{false}
    , m_updatingDeployment{""}
{
    //
    // Ordering should match ResourceGroupNode::ChildNodes Enum
    // ResourceGroupTemplateFilePathNode
    if (map["ResourceGroupTemplateFilePath"].isValid())
    {
        m_templateNode = new ResourceGroupTemplateNode {
            map["ResourceGroupTemplateFilePath"].toString(), ProjectModel()
        };
        appendRow(m_templateNode);
    }

    // LambdaFunctionCodeDirectoryPathNode
    UpdateLambdaFolders(map);

    // CliPluginCodeDirectoryPath
    if (map["CliPluginCodeDirectoryPath"].isValid())
    {
        appendRow(new CliPluginCodeDirectoryNode { map["CliPluginCodeDirectoryPath"].toString(), ProjectModel() });
    }
    // Base resource group settings
    if (map["BaseSettingsFilePath"].isValid())
    {
        LoadResourceGroupSettings(map["BaseSettingsFilePath"].toString(), IAWSProjectModel::ResourceGroupSettingPriority::BASE);
    }
    // Game specific resource group settings
    if (map["GameSettingsFilePath"].isValid())
    {
        LoadResourceGroupSettings(map["GameSettingsFilePath"].toString(), IAWSProjectModel::ResourceGroupSettingPriority::GAME);
    }
    setToolTip(
        "Defines a set of related AWS resources that are used by the Lumberyard project.");

    connect(ProjectModel()->ResourceManager(), &AWSResourceManager::ActiveDeploymentChanged, this, &ResourceGroupNode::OnActiveDeploymentChanged);
}

void ResourceGroupNode::UpdateLambdaFolders(const QVariantMap& map)
{
    if (map["LambdaFunctionCodeDirectoryPaths"].isValid())
    {
        auto paths = map["LambdaFunctionCodeDirectoryPaths"].toStringList();
        for (int i = 0; i < paths.length(); i++)
        {
            int childIndex = 0;
            bool exists = false;
            auto expectedName = QFileInfo(paths[i]).fileName();
            while(GetChild(childIndex) != nullptr)
            {
                auto childName = GetChild(childIndex)->GetName();
                if (childName == expectedName)
                {
                    exists = true;
                    break;
                }
                // Check for nodes we know come after lambda-code nodes
                // Ordering should match ResourceGroupNode::ChildNodes Enum
                if (childName == "cli-plugin-code" ||
                    childName == "resource-group-settings.json")
                {
                    break;
                }
                childIndex++;
            }
            if (!exists)
            {
                insertRow(childIndex, new LambdaFunctionCodeNode{ paths[i], ProjectModel() });
            }
        }
    }
}

QSharedPointer<IResourceGroupStatusModel> ResourceGroupNode::GetResourceGroupStatusModel()
{
    return ProjectModel()->ResourceManager()->GetResourceGroupStatusModel(m_resourceGroupName).staticCast<IResourceGroupStatusModel>();
}

QSharedPointer<ICloudFormationTemplateModel> ResourceGroupNode::GetTemplateModel()
{
    if (m_templateNode)
    {
        return m_templateNode->GetCloudFormationTemplateModel();
    }
    else
    {
        return QSharedPointer<ICloudFormationTemplateModel>{};
    }
}

void ResourceGroupNode::VisitDetail(IAWSProjectDetailVisitor* visitor)
{
    visitor->VisitProjectDetail(GetResourceGroupStatusModel());
}

QSharedPointer<IStackStatusModel> ResourceGroupNode::GetStackStatusModel()
{
    return GetResourceGroupStatusModel();
}

void ResourceGroupNode::SetUpdatingDeploymentName(const QString& deploymentName)
{
    m_updatingDeployment = deploymentName;
    // If the deployment model does not yet have an active deployment set, then any updates will be for the soon-to-be-set deployment.
    // Otherwise, just compare the updating deployment against the current one.
    m_isDeploymentUpdating = !ProjectModel()->ResourceManager()->GetDeploymentModel()->IsActiveDeploymentSet() ||
        (m_updatingDeployment == ProjectModel()->ResourceManager()->GetDeploymentModel()->GetActiveDeploymentName());
}

bool ResourceGroupNode::ShouldAppearInProgress() const
{
    return m_isUpdating && m_isDeploymentUpdating;
}

void ResourceGroupNode::OnActiveDeploymentChanged(const QString& activeDeploymentName)
{
    // Note that GetActiveDeploymentName() won't yet return the correct value at this moment, so we need activeDeploymentName
    m_isDeploymentUpdating = (m_updatingDeployment == activeDeploymentName);
    if (m_isUpdating && !m_isDeploymentUpdating)  // We only need to run the below code if currently in the updating state
    {
        // Set the icon back to the default icon (replaces the last frame of the progress icon)
        SetUpIcon();
    }
}

void ResourceGroupNode::LoadResourceGroupSettings(const QString& filePath, IAWSProjectModel::ResourceGroupSettingPriority settingType)
{
    if (!filePath.length())
    {
        return;
    }
    QFileInfo fileInfo{ filePath };
    if (!fileInfo.exists())
    {
        return;
    }
    Node* newNode = new ResourceGroupSettingsNode{ filePath, ProjectModel(), settingType };
    appendRow(newNode);
}


///////////////////////////////////////////////////////////////////////////////

class ResourceGroupListNode
    : public Node
{
public:

    ResourceGroupListNode(AWSProjectModel* projectModel)
        : Node{"Resource Groups", projectModel, g_folderIcon}
    {
        setToolTip(
            "The resource groups used by the Lumberyard project.");
    }

    void ProcessOutputResourceGroupList(const QVariant& value)
    {
        auto list = value.toMap()["ResourceGroups"].toList();
        AWSProjectModel::Sort(list, "Name");

        m_childManager.UpdateChildren(list);
    }

    void VisitDetail(IAWSProjectDetailVisitor* visitor) override
    {
        visitor->VisitProjectDetail(ProjectModel()->ResourceManager()->GetResourceGroupListStatusModel().staticCast<IResourceGroupListStatusModel>());
    }

    ResourceGroupNode* GetResourceGroupNode(const QString& resoruceGroupName)
    {
        QVariantMap source;
        source["Name"] = resoruceGroupName;
        return m_childManager.FindChild(source);
    }

private:

    class ChildManager
        : public StandardItemChildManager<ResourceGroupListNode, ResourceGroupNode>
    {
    public:

        ChildManager(ResourceGroupListNode* parentItem)
            : StandardItemChildManager{parentItem}
        {
        }

        ResourceGroupNode* Create(const QVariant& source) override
        {
            return new ResourceGroupNode {
                       source.toMap(), Parent()->ProjectModel()
            };
        }

        void Update(const QVariant& source, ResourceGroupNode* target) override
        {
            // It is possible that the update is from adding a lambda function resource,
            // this will make sure all lambda-code folders are showing in the resource manager
            target->UpdateLambdaFolders(source.toMap());
        }

        int Compare(const QVariant& source, const ResourceGroupNode* target) const override
        {
            return CompareVariants(source.toMap()["Name"], target->data(Qt::DisplayRole));
        }

        void ChildAdded(ResourceGroupNode* newChild) override
        {
            Parent()->ProjectModel()->ResourceGroupAdded(newChild->index());
        }
    };

    ChildManager m_childManager {
        this
    };
};


///////////////////////////////////////////////////////////////////////////////

class ProjectSettingsNode
    : public FileNode
{
public:

    ProjectSettingsNode (const QString& path, AWSProjectModel* projectModel)
        : FileNode{path, projectModel, true /* do not delete */}
    {
        setToolTip(
            "File containing the configuration for this Lumberyard project.");

    }

};


///////////////////////////////////////////////////////////////////////////////

class UserSettingsNode
    : public FileNode
{
public:

    UserSettingsNode (const QString& path, AWSProjectModel* projectModel)
        : FileNode{path, projectModel, true /* do not delete */}
    {
        setToolTip(
            "File containing user specific settings for this Lumberyard project.");
    }
};


///////////////////////////////////////////////////////////////////////////////

class ProjectTemplateNode
    : public CloudFormationTemplateFileNode
{
public:

    ProjectTemplateNode(const QString& path, AWSProjectModel* projectModel)
        : CloudFormationTemplateFileNode{path, projectModel}
    {
        setToolTip(
            "AWS CloudFormation stack template that defines the AWS resources required by Resource Manager.");
    }

    QSharedPointer<IStackStatusModel> GetStackStatusModel() override
    {
        return ProjectModel()->ResourceManager()->GetProjectStatusModel();
    }

};


///////////////////////////////////////////////////////////////////////////////

class DeploymentTemplateNode
    : public CloudFormationTemplateFileNode
{
public:

    DeploymentTemplateNode(const QString& path, AWSProjectModel* projectModel)
        : CloudFormationTemplateFileNode{path, projectModel}
    {
        setToolTip(
            "AWS CloudFormation stack template that defines the Lumberyard project's resource groups.");
    }

    QSharedPointer<IStackStatusModel> GetStackStatusModel() override
    {
        return ProjectModel()->ResourceManager()->GetActiveDeploymentStatusModel();
    }

};


///////////////////////////////////////////////////////////////////////////////

class DeploymentAccessTemplateNode
    : public CloudFormationTemplateFileNode
{
public:

    DeploymentAccessTemplateNode(const QString& path, AWSProjectModel* projectModel)
        : CloudFormationTemplateFileNode{path, projectModel}
    {
        setToolTip(
            "AWS CloudFormation stack template that controls access to the Lumberyard project's AWS resources.");
    }
};


///////////////////////////////////////////////////////////////////////////////

class ProjectCodeNode
    : public CodeDirectoryNode
{
public:

    ProjectCodeNode(const QString& path, AWSProjectModel* projectModel)
        : CodeDirectoryNode{path, projectModel, true /* do not delete */}
    {
        setToolTip(
            "Directory containing code that implements AWS Lambda function resources.");
    }
};


///////////////////////////////////////////////////////////////////////////////

class ProjectNode
    : public Node
{
public:

    ProjectNode(AWSProjectModel* projectModel)
        : Node{"Project stack", projectModel, g_groupedResourcesIcon}
    {
        setToolTip(
            "AWS CloudFormation stack defining the resources needed by Resource Manager.");
    }

    QSharedPointer<IProjectStatusModel> GetProjectStatusModel()
    {
        return ProjectModel()->ResourceManager()->GetProjectStatusModel().staticCast<IProjectStatusModel>();
    }

    void VisitDetail(IAWSProjectDetailVisitor* visitor) override
    {
        visitor->VisitProjectDetail(GetProjectStatusModel());
    }

    QSharedPointer<IStackStatusModel> GetStackStatusModel() override
    {
        return GetProjectStatusModel();
    }
};


///////////////////////////////////////////////////////////////////////////////

class ConfigurationNode
    : public Node
{
public:

    ConfigurationNode(AWSProjectModel* projectModel)
        : Node{"Administration (advanced)", projectModel, g_folderIcon}
    {
        setToolTip(
            "Provides access to all Cloud Canvas configuration files for the project.");

        m_projectNode = new ProjectNode(ProjectModel());
        appendRow(m_projectNode);

        m_deploymentListNode = new DeploymentListNode(ProjectModel());
        appendRow(m_deploymentListNode);
    }

    void ProcessOutputProjectDescription(const QVariant& value)
    {
        QVariantMap map = value.value<QVariantMap>();
        QVariantMap details = map["ProjectDetails"].value<QVariantMap>();

        if (rowCount() == 2)
        {
            // These paths are not expected to change... however they may go from non-existence to existence
            // (and back again).
            //

            m_projectSettingsNode = new ProjectSettingsNode(details["ProjectSettingsFilePath"].toString(), ProjectModel());
            appendRow(m_projectSettingsNode);

            appendRow(new ProjectTemplateNode(details["ProjectTemplateFilePath"].toString(), ProjectModel()));

            m_deploymentTemplateNode = new DeploymentTemplateNode(details["DeploymentTemplateFilePath"].toString(), ProjectModel());
            appendRow(m_deploymentTemplateNode);

            appendRow(new DeploymentAccessTemplateNode(details["DeploymentAccessTemplateFilePath"].toString(), ProjectModel()));
            appendRow(new UserSettingsNode(details["UserSettingsFilePath"].toString(), ProjectModel()));
        }
    }

    DeploymentListNode* GetDeploymentListNode()
    {
        return m_deploymentListNode;
    }

    ProjectNode* GetProjectNode()
    {
        return m_projectNode;
    }

    ProjectSettingsNode* GetProjectSettingsNode()
    {
        return m_projectSettingsNode;
    }

    DeploymentTemplateNode* GetDeploymentTemplateNode()
    {
        return m_deploymentTemplateNode;
    }

private:

    DeploymentListNode* m_deploymentListNode;
    ProjectNode* m_projectNode;

    ProjectSettingsNode* m_projectSettingsNode {
        nullptr
    };
    DeploymentTemplateNode* m_deploymentTemplateNode {
        nullptr
    };
};


///////////////////////////////////////////////////////////////////////////////

AWSProjectModel::AWSProjectModel(AWSResourceManager* resourceManager)
    : AWSResourceManagerModel(resourceManager)
{
    setRowCount(RowCount);

    setItem(ResourceGroupListRow, new ResourceGroupListNode {this});
    setItem(ConfigurationRow, new ConfigurationNode {this});

    // Initial progress icon setup
    if (g_inProgressMovie.loopCount() != -1)    // Ensure animation will loop
    {
        connect(&g_inProgressMovie, &QMovie::finished, &g_inProgressMovie, &QMovie::start);
    }
    g_inProgressMovie.start();
}


///////////////////////////////////////////////////////////////////////////////

template<typename T>
T* AWSProjectModel::GetNode(const QModelIndex& index) const
{
    assert(index.model() == this); // index from some other model?
    return static_cast<T*>(itemFromIndex(index));
}


///////////////////////////////////////////////////////////////////////////////

void AWSProjectModel::VisitDetail(const QModelIndex& index, IAWSProjectDetailVisitor* visitor)
{
    GetNode(index)->VisitDetail(visitor);
}

///////////////////////////////////////////////////////////////////////////////

ResourceGroupListNode* AWSProjectModel::GetResourceGroupListNode() const
{
    return GetNode<ResourceGroupListNode>(index(ResourceGroupListRow, 0));
}

///////////////////////////////////////////////////////////////////////////////

ProjectSettingsNode* AWSProjectModel::GetProjectSettingsNode() const
{
    return GetConfigurationNode()->GetProjectSettingsNode();
}

///////////////////////////////////////////////////////////////////////////////

DeploymentTemplateNode* AWSProjectModel::GetDeploymentTemplateNode() const
{
    return GetConfigurationNode()->GetDeploymentTemplateNode();
}

///////////////////////////////////////////////////////////////////////////////

DeploymentListNode* AWSProjectModel::GetDeploymentListNode() const
{
    auto configurationNode = GetConfigurationNode();
    return configurationNode->GetDeploymentListNode();
}

///////////////////////////////////////////////////////////////////////////////

ConfigurationNode* AWSProjectModel::GetConfigurationNode() const
{
    return GetNode<ConfigurationNode>(index(ConfigurationRow, 0));
}

///////////////////////////////////////////////////////////////////////////////

void AWSProjectModel::ProcessOutputDeploymentList(const QVariant& value)
{
    GetDeploymentListNode()->ProcessOutputDeploymentList(value);
}


///////////////////////////////////////////////////////////////////////////////

void AWSProjectModel::ProcessOutputResourceGroupList(const QVariant& value)
{
    GetResourceGroupListNode()->ProcessOutputResourceGroupList(value);
}


///////////////////////////////////////////////////////////////////////////////

void AWSProjectModel::ProcessOutputProjectDescription(const QVariant& value)
{
    GetConfigurationNode()->ProcessOutputProjectDescription(value);

    QVariantMap map = value.value<QVariantMap>();
    QVariantMap details = map["ProjectDetails"].value<QVariantMap>();
    m_gemsFilePath = details["GemsFilePath"].toString();

}

///////////////////////////////////////////////////////////////////////////////

QModelIndex AWSProjectModel::ResourceGroupListIndex() const
{
    return GetResourceGroupListNode()->index();
}

///////////////////////////////////////////////////////////////////////////////

QModelIndex AWSProjectModel::DeploymentListIndex() const
{
    return GetDeploymentListNode()->index();
}

///////////////////////////////////////////////////////////////////////////////

QModelIndex AWSProjectModel::ProjectStackIndex() const
{
    return GetConfigurationNode()->GetProjectNode()->index();
}

///////////////////////////////////////////////////////////////////////////////

void AWSProjectModel::AddResourceGroup(const QString& resourceGroupName, bool includeExampleResources, AsyncOperationCallback callback)
{

    auto _callback = [callback](const QString& key, const QVariant value)
    {
        if (key == "success")
        {
            callback(QString{});
        }
        else if (key == "error")
        {
            callback(value.toString());
        }
    };

    QVariantMap args;
    args["resource_group"] = resourceGroupName;
    args["include_example_resources"] = includeExampleResources;
    ResourceManager()->ExecuteAsync(_callback, "add-resource-group", args);

}

///////////////////////////////////////////////////////////////////////////////

void AWSProjectModel::DisableResourceGroup(const QString& resourceGroupName, AsyncOperationCallback callback)
{

    auto _callback = [callback](const QString& key, const QVariant value)
    {
        if (key == "success")
        {
            callback(QString{});
        }
        else if (key == "error")
        {
            callback(value.toString());
        }
    };

    QVariantMap args;
    args["resource_group"] = resourceGroupName;
    ResourceManager()->ExecuteAsync(_callback, "disable-resource-group", args);

}

///////////////////////////////////////////////////////////////////////////////

void AWSProjectModel::EnableResourceGroup(const QString& resourceGroupName, AsyncOperationCallback callback)
{

    auto _callback = [callback](const QString& key, const QVariant value)
    {
        if (key == "success")
        {
            callback(QString{});
        }
        else if (key == "error")
        {
            callback(value.toString());
        }
    };

    QVariantMap args;
    args["resource_group"] = resourceGroupName;
    ResourceManager()->ExecuteAsync(_callback, "enable-resource-group", args);

}

///////////////////////////////////////////////////////////////////////////////

void AWSProjectModel::AddServiceApi(const QString& resourceGroupName, AsyncOperationCallback callback)
{

    auto _callback = [callback](const QString& key, const QVariant value)
    {
        if (key == "success")
        {
            callback(QString{});
        }
        else if (key == "error")
        {
            callback(value.toString());
        }
    };

    QVariantMap args;
    args["resource_group"] = resourceGroupName;
    ResourceManager()->ExecuteAsync(_callback, "add-service-api-resources", args);


}

///////////////////////////////////////////////////////////////////////////////

void AWSProjectModel::CreateDeploymentStack(const QString& deploymentName, bool shouldMakeProjectDefault)
{
    GetDeploymentListNode()->AddDeployment(deploymentName);

    OnDeploymentInProgress(deploymentName); // This call must come after the above AddDeployment() call
    OnAllResourceGroupsInProgress(deploymentName);

    QVariantMap args;
    args["deployment"] = deploymentName;
    args["make_project_default"] = shouldMakeProjectDefault;
    args["confirm_aws_usage"] = true;
    args["confirm_security_change"] = true;
    ResourceManager()->ExecuteAsync(
        ResourceManager()->GetDeploymentStatusModel(deploymentName)->GetStackEventsModelInternal()->GetRequestId(),
        "create-deployment-stack", args);
}

///////////////////////////////////////////////////////////////////////////////

void AWSProjectModel::DeleteDeploymentStack(const QString& deploymentName)
{
    auto model = ResourceManager()->GetDeploymentStatusModel(deploymentName);
    QVariantMap args;
    args["deployment"] = deploymentName;
    args["confirm_resource_deletion"] = true;
    ResourceManager()->ExecuteAsync(
        model->GetStackEventsModelInternal()->GetRequestId(),
        "delete-deployment-stack", args);
}

///////////////////////////////////////////////////////////////////////////////

QSharedPointer<IResourceGroupStatusModel> AWSProjectModel::ResourceGroupStatusModel(const QString& resourceGroupName)
{
    return ResourceManager()->GetResourceGroupStatusModel(resourceGroupName).staticCast<IResourceGroupStatusModel>();
}

///////////////////////////////////////////////////////////////////////////////

QSharedPointer<IResourceGroupStatusModel> AWSProjectModel::ResourceGroupStatusModel(const QModelIndex& index)
{
    assert(index.model() == this);

    auto resourceGroupListItem = GetResourceGroupListNode();
    auto item = itemFromIndex(index);
    while (item->parent() != nullptr)
    {
        if (item->parent() == resourceGroupListItem)
        {
            auto resourceGroupNode = static_cast<ResourceGroupNode*>(item);
            return resourceGroupNode->GetResourceGroupStatusModel();
        }
        else
        {
            item = item->parent();
        }
    }

    return QSharedPointer<IResourceGroupStatusModel>{};
}

// Helpers

QString AWSProjectModel::GetProjectSettingsFile() const
{
    ProjectSettingsNode* projectNode = GetProjectSettingsNode();
    QString returnString;

    if (projectNode)
    {
        returnString = projectNode->GetPath();
    }
    return returnString;
}

QString AWSProjectModel::GetDeploymentTemplateFile() const
{
    DeploymentTemplateNode* deploymentNode = GetDeploymentTemplateNode();
    QString returnString;

    if (deploymentNode)
    {
        returnString = deploymentNode->GetPath();
    }
    return returnString;
}

QString AWSProjectModel::GetGemsFile() const
{
    return m_gemsFilePath;
}

QVector<QString> AWSProjectModel::GetWritableFilesforUploadResources() const
{
    QVector<QString > fileList;

    RequestFilesForUploadResources(fileList);

    return fileList;
}

QJsonValue AWSProjectModel::GetResourceGroupSetting(const QString& resourceGroupName, const QString& settingName, IAWSProjectModel::ResourceGroupSettingPriority settingPriority) const
{
    QJsonValue settingValue;

    switch (settingPriority)
    {
    case IAWSProjectModel::ResourceGroupSettingPriority::GAME_OR_BASE:
    {
        RequestResourceGroupSetting(resourceGroupName, settingName, IAWSProjectModel::ResourceGroupSettingPriority::GAME, settingValue);
        if (!settingValue.isNull())
        {
            return settingValue;
        }

        RequestResourceGroupSetting(resourceGroupName, settingName, IAWSProjectModel::ResourceGroupSettingPriority::BASE, settingValue);

        return settingValue;
    }
    break;

    case IAWSProjectModel::ResourceGroupSettingPriority::BASE_OR_GAME:
    {
        RequestResourceGroupSetting(resourceGroupName, settingName, IAWSProjectModel::ResourceGroupSettingPriority::BASE, settingValue);
        if (!settingValue.isNull())
        {
            return settingValue;
        }

        RequestResourceGroupSetting(resourceGroupName, settingName, IAWSProjectModel::ResourceGroupSettingPriority::GAME, settingValue);

        return settingValue;
    }
    break;

    case IAWSProjectModel::ResourceGroupSettingPriority::GAME:
    {
        RequestResourceGroupSetting(resourceGroupName, settingName, IAWSProjectModel::ResourceGroupSettingPriority::GAME, settingValue);

        return settingValue;
    }
    break;

    case IAWSProjectModel::ResourceGroupSettingPriority::BASE:
    {
        RequestResourceGroupSetting(resourceGroupName, settingName, IAWSProjectModel::ResourceGroupSettingPriority::BASE, settingValue);

        return settingValue;
    }
    break;

    }


    return settingValue;
}
///////////////////////////////////////////////////////////////////////////////

void AWSProjectModel::OnResourceGroupInProgress(const QString& deploymentName, const QString& resourceGroupName)
{
    ResourceGroupListNode* resourceGroupListNode = static_cast<ResourceGroupListNode*>(itemFromIndex(ResourceGroupListIndex()));
    ResourceGroupNode* resourceGroupNode = resourceGroupListNode->GetResourceGroupNode(resourceGroupName);

    resourceGroupNode->SetUpdatingDeploymentName(deploymentName);
    resourceGroupNode->SetIsUpdating(true);
}

void AWSProjectModel::OnAllResourceGroupsInProgress(const QString& deploymentName)
{
    ResourceGroupListNode* resourceGroupListNode = static_cast<ResourceGroupListNode*>(itemFromIndex(ResourceGroupListIndex()));

    // Visit all children (Resource Groups) of the Resource Group List
    int numRows = resourceGroupListNode->rowCount();

    for (int r = 0; r < numRows; ++r)
    {
        ResourceGroupNode* child = static_cast<ResourceGroupNode*>(resourceGroupListNode->child(r, 0)); // The child item that represents the tree node is always column 0.
        if (child != nullptr)
        {
            child->SetUpdatingDeploymentName(deploymentName);
            child->SetIsUpdating(true);
        }
    }
}

void AWSProjectModel::OnProjectStackInProgress()
{
    Node* node = static_cast<Node*>(itemFromIndex(ProjectStackIndex()));
    node->SetIsUpdating(true);
}

void AWSProjectModel::OnDeploymentInProgress(const QString& deploymentName)
{
    DeploymentListNode* deploymentListNode = static_cast<DeploymentListNode*>(itemFromIndex(DeploymentListIndex()));
    Node* deploymentNode = static_cast<Node*>(deploymentListNode->GetDeploymentNode(deploymentName));

    deploymentNode->SetIsUpdating(true);
}

///////////////////////////////////////////////////////////////////////////////

QSharedPointer<IDeploymentStatusModel> AWSProjectModel::DeploymentStatusModel(const QString& deploymentName)
{
    return ResourceManager()->GetDeploymentStatusModel(deploymentName);
}

QSharedPointer<IProjectStatusModel> AWSProjectModel::ProjectStatusModel()
{
    return ResourceManager()->GetProjectStatusModel().staticCast<IProjectStatusModel>();
}

QSharedPointer<ICloudFormationTemplateModel> AWSProjectModel::GetResourceGroupTemplateModel(const QString& resourceGroupName)
{
    auto listNode = GetResourceGroupListNode();
    auto groupNode = listNode->GetResourceGroupNode(resourceGroupName);
    assert(groupNode);
    return groupNode->GetTemplateModel();
}


void AWSProjectModel::CreateFunctionFolder(const QString& functionName, const QString& resourceGroupName, AsyncOperationCallback callback) const
{

    QVariantMap args;
    args["function"] = functionName;
    args["resource_group"] = resourceGroupName;
    args["force"] = true;

    auto _callback = [this, callback](const QString& key, const QVariant value)
    {
        if (key == "success")
        {
            ResourceManager()->GetResourceGroupListStatusModel()->Refresh(true);
            callback(QString{});
        }
        else if (key == "error")
        {
            callback(value.toString());
        }
    };

    ResourceManager()->ExecuteAsync(_callback, "create-function-folder", args);

}
