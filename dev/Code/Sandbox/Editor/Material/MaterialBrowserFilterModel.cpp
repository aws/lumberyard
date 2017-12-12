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
#include "MaterialBrowserFilterModel.h"
#include "MaterialBrowserSearchFilters.h"
#include "MaterialManager.h"

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Jobs/JobFunction.h>

#include <QPainter>

namespace
{
    // how often to re-query source control status of an item after querying it, in seconds.
    // note that using a source control operation on an item invalidates the cache and will immediately
    // refresh its status regardless of this value, so it can be pretty high.
    qint64 g_timeRefreshSCCStatus = 60;
}

#define IDC_MATERIAL_TREECTRL 3

//////////////////////////////////////////////////////////////////////////
#define ITEM_IMAGE_SHARED_MATERIAL   0
#define ITEM_IMAGE_SHARED_MATERIAL_SELECTED 1
#define ITEM_IMAGE_FOLDER            2
#define ITEM_IMAGE_FOLDER_OPEN       3
#define ITEM_IMAGE_MATERIAL          4
#define ITEM_IMAGE_MATERIAL_SELECTED 5
#define ITEM_IMAGE_MULTI_MATERIAL    6
#define ITEM_IMAGE_MULTI_MATERIAL_SELECTED 7


#define ITEM_IMAGE_OVERLAY_CGF          8
#define ITEM_IMAGE_OVERLAY_INPAK        9
#define ITEM_IMAGE_OVERLAY_READONLY     10
#define ITEM_IMAGE_OVERLAY_ONDISK       11
#define ITEM_IMAGE_OVERLAY_LOCKED       12
#define ITEM_IMAGE_OVERLAY_CHECKEDOUT   13
#define ITEM_IMAGE_OVERLAY_NO_CHECKOUT  14
//////////////////////////////////////////////////////////////////////////

AZ::Data::AssetId GetMaterialProductAssetIdFromAssetBrowserEntry(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* assetEntry)
{
    AZ::Data::AssetId assetId;
    if (assetEntry && (assetEntry->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Source || assetEntry->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Product))
    {
        // Get the product's assetId
        if (assetEntry->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Product)
        {
            const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* productAssetEntry = static_cast<const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry*>(assetEntry);
            assetId = productAssetEntry->GetAssetId();
        }
        else
        {
            // When filtering for products, the source files still show up in the material browser hierarchy
            // To make them interactive, get the product from the source entry
            const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* sourceAssetEntry = static_cast<const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry*>(assetEntry);
            // Ignore the source asset if it is not a material. It might be an FBX that produces materials, in which case the user should explicitly select the product material rather than the FBX to edit it.
            // Also ignore the source asset if it has no children (there's nothing to edit), or has more than one child (there is not an individual material that can be assumed based on the source) 
            if (sourceAssetEntry->GetPrimaryAssetType() == GetIEditor()->GetMaterialManager()->GetMaterialAssetType() && sourceAssetEntry->GetChildCount() == 1)
            {
                // When selecting a source asset in the material browser, get the id of the first (and only) child, and use that as the lookup value in the material record map
                const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* productAssetEntry = static_cast<const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry*>(sourceAssetEntry->GetChild(0));
                assetId = productAssetEntry->GetAssetId();
            }
        }
    }
    // The default will be an invalid assetId if no appropriate product assetId was found
    return assetId;
}


AZStd::string GetFullSourcePathFromAssetBrowserEntry(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* assetEntry)
{
    if (assetEntry->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Product)
    {
        // Attempt to get the full source path from the relative path
        // the fastest option is to just get it directly from the entry, but we can fallback on asking the AP if that doesn't work
        AzToolsFramework::AssetBrowser::AssetBrowserEntry* parentEntry = assetEntry->GetParent();
        if ((parentEntry) && (parentEntry->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Source))
        {
            AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* sourceAssetEntry = azrtti_cast<AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry*>(parentEntry);
            if (sourceAssetEntry)
            {
                return sourceAssetEntry->GetFullPath();
            }
        }

        // Fall back on asking the AP
        bool pathFound = false;
        AZStd::string fullSourcePath = "";
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(pathFound, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetFullSourcePathFromRelativeProductPath, assetEntry->GetRelativePath(), fullSourcePath);

        if (pathFound)
        {
            return fullSourcePath;
        }
    }

    // For non-product entries, or as a fall back if the two attempts at getting a product entry fail
    return assetEntry->GetFullPath();
}

MaterialBrowserFilterModel::MaterialBrowserFilterModel(QObject* parent)
    : AzToolsFramework::AssetBrowser::AssetBrowserFilterModel(parent)
{
    MaterialBrowserSourceControlBus::Handler::BusConnect();
    AzToolsFramework::AssetBrowser::AssetBrowserModelNotificationsBus::Handler::BusConnect();

    m_imageList.push_back(QPixmap(":/MaterialBrowser/material_00.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/material_01.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/material_02.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/material_03.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/material_04.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/material_05.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/material_06.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/material_07.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/filestatus_00.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/filestatus_01.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/filestatus_02.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/filestatus_03.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/filestatus_04.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/filestatus_05.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/filestatus_06.png"));


    // Create an asset type filter for materials
    AzToolsFramework::AssetBrowser::AssetTypeFilter* assetTypeFilter = new AzToolsFramework::AssetBrowser::AssetTypeFilter();
    assetTypeFilter->SetAssetType(GetIEditor()->GetMaterialManager()->GetMaterialAssetType());

    assetTypeFilter->SetFilterPropagation(AzToolsFramework::AssetBrowser::AssetBrowserEntryFilter::PropagateDirection::Down); //this will make sure folders that contain materials are displayed
    m_assetTypeFilter = AzToolsFramework::AssetBrowser::FilterConstType(assetTypeFilter);

    m_subMaterialSearchFilter = new SubMaterialSearchFilter(this);

    InitializeRecordUpdateJob();
}

MaterialBrowserFilterModel::~MaterialBrowserFilterModel()
{
    SAFE_DELETE(m_jobCancelGroup);
    SAFE_DELETE(m_jobContext);
    SAFE_DELETE(m_jobManager);

    AzToolsFramework::AssetBrowser::AssetBrowserModelNotificationsBus::Handler::BusDisconnect();
    MaterialBrowserSourceControlBus::Handler::BusDisconnect();
}

void MaterialBrowserFilterModel::UpdateRecord(const QModelIndex &filterModelIndex)
{
    if (filterModelIndex.isValid())
    {
        QModelIndex modelIndex = mapToSource(filterModelIndex);

        AzToolsFramework::AssetBrowser::AssetBrowserEntry* assetEntry = static_cast<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>(modelIndex.internalPointer());

        if (assetEntry->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Product)
        {
            AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* productAssetEntry = static_cast<AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry*>(modelIndex.internalPointer());

            MaterialBrowserRecordAssetBrowserData assetBrowserData;
            assetBrowserData.assetId = productAssetEntry->GetAssetId();

            assetBrowserData.relativeFilePath = assetEntry->GetRelativePath();
            assetBrowserData.fullSourcePath = GetFullSourcePathFromAssetBrowserEntry(assetEntry);
            assetBrowserData.modelIndex = modelIndex;
            assetBrowserData.filterModelIndex = filterModelIndex;
            UpdateRecord(assetBrowserData);
        }
    }
}

void MaterialBrowserFilterModel::UpdateRecord(const MaterialBrowserRecordAssetBrowserData &assetBrowserData)
{
    CMaterialBrowserRecord record;
    record.SetAssetBrowserData(assetBrowserData);
    record.m_material = GetIEditor()->GetMaterialManager()->LoadMaterial(record.GetRelativeFilePath().c_str());
    SetRecord(record);
}

void MaterialBrowserFilterModel::UpdateSourceControlFileInfoCallback(const AZ::Data::AssetId &assetId, const AzToolsFramework::SourceControlFileInfo& info)
{
    CMaterialBrowserRecord record;
    bool found = m_materialRecordMap.find(assetId, &record);
    if (found)
    {
        // Update the cached source control attributes for the record
        record.m_lastCachedSCCAttributes = info;
        record.m_lastCachedFileAttributes = static_cast<ESccFileAttributes>(CFileUtil::GetAttributes(record.GetFullSourcePath().c_str(), false));
        record.m_lastCheckedSCCAttributes = QDateTime::currentDateTime();

        // Update the record
        m_materialRecordMap.erase(assetId);
        m_materialRecordMap.insert(AZStd::pair<AZ::Data::AssetId, CMaterialBrowserRecord>(record.GetAssetId(), record));

        QueueDataChangedEvent(record.GetFilterModelIndex());
    }
}

void MaterialBrowserFilterModel::UpdateSourceControlLastCheckedTime(const AZ::Data::AssetId &assetId, const QDateTime &dateTime)
{
    CMaterialBrowserRecord record;
    bool found = m_materialRecordMap.find(assetId, &record);
    if (found)
    {
        // Update the record
        record.m_lastCheckedSCCAttributes = dateTime;
        m_materialRecordMap.erase(assetId);
        m_materialRecordMap.insert(AZStd::pair<AZ::Data::AssetId, CMaterialBrowserRecord>(record.GetAssetId(), record));
    }
}

QVariant MaterialBrowserFilterModel::data(const QModelIndex& index, int role /* = Qt::DisplayRole*/) const
{
    // Should return data from an AssetBrowserEntry, or get material specific info from that data
    if (index.isValid())
    {
        // Use the base AssetBrowserFilterModel::data function for display role
        if (role != Qt::DisplayRole)
        {
            QModelIndex modelIndex = mapToSource(index);
            AzToolsFramework::AssetBrowser::AssetBrowserEntry* assetEntry = static_cast<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>(modelIndex.internalPointer());

            AZ::Data::AssetId assetId = GetMaterialProductAssetIdFromAssetBrowserEntry(assetEntry);
            if (assetId.IsValid())
            {
                CMaterialBrowserRecord record;
                bool found = m_materialRecordMap.find(assetId, &record);

                if (role == Qt::UserRole)
                {
                    if (found)
                    {
                        return QVariant::fromValue(record);
                    }
                    else
                    {
                        return QVariant();
                    }
                }
                else if (role == Qt::DecorationRole)
                {
                    // If this is a material that has already been loaded, get the custom icon for it
                    if (found)
                    {
                        int icon = ITEM_IMAGE_MATERIAL;
                        if (record.m_material && record.m_material->IsMultiSubMaterial())
                        {
                            icon = ITEM_IMAGE_MULTI_MATERIAL;
                        }

                        QPixmap pixmap = m_imageList[icon];


                        icon = -1;
                        if (record.m_material)
                        {
                            int secondsSinceLastUpdated = record.m_lastCheckedSCCAttributes.secsTo(QDateTime::currentDateTime());
                            // Queue an update of the record if it's source control/writable status is out of date
                            if (secondsSinceLastUpdated > g_timeRefreshSCCStatus || secondsSinceLastUpdated < 0)
                            {
                                bool isSourceControlActive = false;
                                AzToolsFramework::SourceControlConnectionRequestBus::BroadcastResult(isSourceControlActive, &AzToolsFramework::SourceControlConnectionRequestBus::Events::IsActive);

                                if (isSourceControlActive && AzToolsFramework::SourceControlCommandBus::FindFirstHandler() != nullptr)
                                {
                                    // Async call to get the source control status
                                    bool sourceControlOperationComplete = false;
                                    AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::GetFileInfo, record.GetFullSourcePath().c_str(), [this, assetId](bool succeeded, const AzToolsFramework::SourceControlFileInfo& fileInfo)
                                    {
                                        // When the source control operation completes, broadcast an event for the filter model to update the underlying record
                                        MaterialBrowserSourceControlBus::Broadcast(&MaterialBrowserSourceControlEvents::UpdateSourceControlFileInfoCallback, assetId, fileInfo);
                                    }
                                    );
                                    // Update the last checked time now instead of waiting for the result so that additional source control requests are not made in the meantime
                                    MaterialBrowserSourceControlBus::Broadcast(&MaterialBrowserSourceControlEvents::UpdateSourceControlLastCheckedTime, assetId, QDateTime::currentDateTime());
                                }
                                else
                                {
                                    // If source control is not available, still update the file info to get the latest writable status and update the last checked time, but with default source control values
                                    MaterialBrowserSourceControlBus::Broadcast(&MaterialBrowserSourceControlEvents::UpdateSourceControlFileInfoCallback, assetId, AzToolsFramework::SourceControlFileInfo());
                                }
                            }

                            // Add an overlay if the file is checked out or read-only
                            if (record.m_lastCachedSCCAttributes.m_status == AzToolsFramework::SourceControlStatus::SCS_OpenByUser)
                            {
                                icon = ITEM_IMAGE_OVERLAY_CHECKEDOUT;
                            }
                            else if (record.m_lastCachedFileAttributes & SCC_FILE_ATTRIBUTE_READONLY || record.m_lastCachedFileAttributes & SCC_FILE_ATTRIBUTE_INPAK)
                            {
                                icon = ITEM_IMAGE_OVERLAY_READONLY;
                            }

                        }
                        // If there is a source control overlay, draw it on top of the material icon
                        if (icon != -1)
                        {
                            const QPixmap pixmapOverlay = m_imageList[icon];
                            QPainter p(&pixmap);
                            p.drawPixmap(pixmap.rect(), pixmapOverlay);
                        }
                        return pixmap;
                    }
                }
            }
        }
    }
    // If the role is Qt::DisplayRole, the item a folder, or the material has not been parsed yet, fall back on the default data result from the underlying AssetBrowserModel
    return AzToolsFramework::AssetBrowser::AssetBrowserFilterModel::data(index, role);
}

void MaterialBrowserFilterModel::GetRelativeFilePaths(AZStd::vector<MaterialBrowserRecordAssetBrowserData>& files) const
{
    GetRelativeFilePathsRecursive(files, this);
}

void MaterialBrowserFilterModel::GetRelativeFilePathsRecursive(AZStd::vector<MaterialBrowserRecordAssetBrowserData>& files, const MaterialBrowserFilterModel* model, const QModelIndex &parent /*= QModelIndex()*/) const
{
    for (int r = 0; r < model->rowCount(parent); ++r)
    {
        QModelIndex index = model->index(r, 0, parent);

        QModelIndex modelIndex = model->mapToSource(index);
        AzToolsFramework::AssetBrowser::AssetBrowserEntry* assetEntry = static_cast<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>(modelIndex.internalPointer());

        if (assetEntry->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Product)
        {
            MaterialBrowserRecordAssetBrowserData item;

            AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* productAssetEntry = static_cast<AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry*>(modelIndex.internalPointer());
            item.assetId = productAssetEntry->GetAssetId();

            item.fullSourcePath = GetFullSourcePathFromAssetBrowserEntry(assetEntry);
            item.relativeFilePath = assetEntry->GetRelativePath();
            item.modelIndex = modelIndex;
            item.filterModelIndex = index;
            files.push_back(item);
        }

        if (model->hasChildren(index))
        {
            GetRelativeFilePathsRecursive(files, model, index);
        }
    }
}

QModelIndex MaterialBrowserFilterModel::GetIndexFromMaterial(_smart_ptr<CMaterial> material) const
{
    CMaterialBrowserRecord record;
    bool found = TryGetRecordFromMaterial(material, record);
    if (found)
    {
        return record.GetFilterModelIndex();
    }
    return QModelIndex();
}

QModelIndex MaterialBrowserFilterModel::GetFilterModelIndex(const AZ::Data::AssetId &assetId) const
{
    QModelIndex filterModelIndex;
    if (TryGetFilterModelIndexRecursive(filterModelIndex, assetId, this))
    {
        return filterModelIndex;
    }

    return QModelIndex();
}

bool MaterialBrowserFilterModel::TryGetFilterModelIndexRecursive(QModelIndex &filterModelIndex, const AZ::Data::AssetId &assetId, const MaterialBrowserFilterModel* model, const QModelIndex &parent /*= QModelIndex()*/) const
{
    // Walk through the filter model to find the product entry with the corresponding assetId
    for (int r = 0; r < model->rowCount(parent); ++r)
    {
        // Set the filter model index for the current entry
        filterModelIndex = model->index(r, 0, parent);

        QModelIndex modelIndex = model->mapToSource(filterModelIndex);
        AzToolsFramework::AssetBrowser::AssetBrowserEntry* assetEntry = static_cast<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>(modelIndex.internalPointer());

        // Check to see if the current entry is the one we're looking for
        if (assetEntry->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Product)
        {
            AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* productAssetEntry = static_cast<AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry*>(modelIndex.internalPointer());
            if (assetId == productAssetEntry->GetAssetId())
            {
                return true;
            }
        }

        // If not, recursively search through the children of the current entry
        if (model->hasChildren(filterModelIndex))
        {
            if (TryGetFilterModelIndexRecursive(filterModelIndex, assetId, model, filterModelIndex))
            {
                return true;
            }
        }
    }

    return false;
}

void MaterialBrowserFilterModel::EntryAdded(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry)
{
    // If the entry is a product material
    if (entry->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Product && m_assetTypeFilter->Match(entry))
    {
        // Create a job to add/update the entry in the underlying map
        AZ::Job* updateEntryJob = AZ::CreateJobFunction([this, entry]()
        {
            MaterialBrowserRecordAssetBrowserData assetBrowserData;
            assetBrowserData.assetId = GetMaterialProductAssetIdFromAssetBrowserEntry(entry);

            assetBrowserData.relativeFilePath = entry->GetRelativePath();
            assetBrowserData.fullSourcePath = GetFullSourcePathFromAssetBrowserEntry(entry);
            assetBrowserData.filterModelIndex = GetFilterModelIndex(assetBrowserData.assetId);

            CMaterialBrowserRecord record;
            record.SetAssetBrowserData(assetBrowserData);
            record.m_material = GetIEditor()->GetMaterialManager()->LoadMaterialWithFullSourcePath(record.GetRelativeFilePath().c_str(), record.GetFullSourcePath().c_str());
            SetRecord(record);

        }, true, m_jobContext);

        // Start the job immediately
        AZ::Job* currentJob = m_jobContext->GetJobManager().GetCurrentJob();
        if (currentJob)
        {
            // Suspend the current job until the new job completes so that
            // if a new material is created by the user it's ready to use sooner
            currentJob->StartAsChild(updateEntryJob);
            currentJob->WaitForChildren();
        }
        else
        {
            updateEntryJob->Start();
        }

        // Force the tree view to refresh with the new entry
        invalidateFilter();
    }
}

void MaterialBrowserFilterModel::EntryRemoved(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry)
{
    // If the entry is a material
    if (m_assetTypeFilter->Match(entry))
    {
        // Force the tree view to refresh without the deleted entry
        invalidateFilter();
    }
}

bool MaterialBrowserFilterModel::TryGetRecordFromMaterial(_smart_ptr<CMaterial> material, CMaterialBrowserRecord& record) const
{
    if (material)
    {
        // Get the relative path for the material
        bool pathFound = false;
        AZStd::string relativePath;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(pathFound, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetRelativeProductPathFromFullSourceOrProductPath, material->GetFilename().toUtf8().data(), relativePath);
        AZ_Assert(pathFound, "Failed to get engine relative path from %s", material->GetFilename().toUtf8().data());

        // Get the assetId from the relative path
        AZ::Data::AssetId assetId;

        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, relativePath.c_str(), GetIEditor()->GetMaterialManager()->GetMaterialAssetType(), false);

        return TryGetRecordFromAssetId(assetId, record);
    }

    return false;
}

bool MaterialBrowserFilterModel::TryGetRecordFromAssetId(const AZ::Data::AssetId &assetId, CMaterialBrowserRecord& record) const
{
    bool recordFound = m_materialRecordMap.find(assetId, &record);
    return recordFound;
}

void MaterialBrowserFilterModel::SetRecord(const CMaterialBrowserRecord& record)
{
    m_materialRecordMap.erase(record.GetAssetId());
    m_materialRecordMap.insert(AZStd::pair<AZ::Data::AssetId, CMaterialBrowserRecord>(record.GetAssetId(), record));    

    QueueDataChangedEvent(record.GetFilterModelIndex());    
}

void MaterialBrowserFilterModel::SetSearchFilter(const AzToolsFramework::AssetBrowser::SearchWidget* searchWidget)
{
    m_searchWidget = searchWidget;

    // Create a search filter where either a standard search OR a search for a sub-material name returns a match
    AzToolsFramework::AssetBrowser::CompositeFilter* compositeSearchFilter = new AzToolsFramework::AssetBrowser::CompositeFilter(AzToolsFramework::AssetBrowser::CompositeFilter::LogicOperatorType::OR);
    compositeSearchFilter->AddFilter(m_searchWidget->GetFilter());
    compositeSearchFilter->AddFilter(AzToolsFramework::AssetBrowser::FilterConstType(m_subMaterialSearchFilter));

    // Create a composite filter where a match must be a material AND match one of the searches
    AzToolsFramework::AssetBrowser::CompositeFilter* isMaterialAndMatchesSearchFilter = new AzToolsFramework::AssetBrowser::CompositeFilter(AzToolsFramework::AssetBrowser::CompositeFilter::LogicOperatorType::AND);
    isMaterialAndMatchesSearchFilter->AddFilter(m_assetTypeFilter);
    isMaterialAndMatchesSearchFilter->AddFilter(AzToolsFramework::AssetBrowser::FilterConstType(compositeSearchFilter));

    // Set the filter for the MaterialBrowserFilterModel
    SetFilter(AzToolsFramework::AssetBrowser::FilterConstType(isMaterialAndMatchesSearchFilter));
}

void MaterialBrowserFilterModel::SearchFilterUpdated()
{
    m_subMaterialSearchFilter->SetFilterString(m_searchWidget->GetFilterString());
    filterUpdatedSlot();
}

void MaterialBrowserFilterModel::QueueDataChangedEvent(const QPersistentModelIndex &filterModelIndex)
{
    // Queue a data changed event to be executed on the main thread
    AZStd::function<void()> emitDataChanged =
        [this, filterModelIndex]()
    {
        if (filterModelIndex.isValid())
        {
            Q_EMIT dataChanged(filterModelIndex, filterModelIndex);
        }
    };

    AZ::TickBus::QueueFunction(emitDataChanged);
}


void MaterialBrowserFilterModel::InitializeRecordUpdateJob()
{
    AZ::JobManagerDesc desc;
    AZ::JobManagerThreadDesc threadDesc;
    for (size_t i = 0; i < AZStd::thread::hardware_concurrency(); ++i)
    {
        desc.m_workerThreads.push_back(threadDesc);
    }

    // Check to ensure these have not already been initialized.
    AZ_Error("Material Browser", !m_jobManager && !m_jobCancelGroup && !m_jobContext, "MaterialBrowserFilterModel::InitializeRecordUpdateJob is being called again after it has already been initialized");
    m_jobManager = aznew AZ::JobManager(desc);
    m_jobCancelGroup = aznew AZ::JobCancelGroup();
    m_jobContext = aznew AZ::JobContext(*m_jobManager, *m_jobCancelGroup);
}

void MaterialBrowserFilterModel::StartRecordUpdateJobs()
{
    m_mainUpdateRecordJob = aznew MaterialBrowserUpdateJobCreator(this, m_jobContext);
    m_mainUpdateRecordJob->Start();
}

void MaterialBrowserFilterModel::CancelRecordUpdateJobs()
{
    m_jobContext->GetCancelGroup()->Cancel();
    m_jobContext->GetCancelGroup()->Reset();
}

void MaterialBrowserFilterModel::ClearRecordMap()
{
    CancelRecordUpdateJobs();
    m_materialRecordMap.clear();
}

MaterialBrowserUpdateJobCreator::MaterialBrowserUpdateJobCreator(MaterialBrowserFilterModel* model, AZ::JobContext* context /*= NULL*/)
    : Job(true, context)
    , m_filterModel(model)
{
}

void MaterialBrowserUpdateJobCreator::Process()
{
    AZStd::vector<MaterialBrowserRecordAssetBrowserData> files;
    m_filterModel->GetRelativeFilePaths(files);

    int numJobs = GetContext()->GetJobManager().GetNumWorkerThreads();
    int materialsPerJob = (files.size() / numJobs) + 1;

    for (auto it = files.begin(); it <= files.end(); it += materialsPerJob)
    {
        // Create a subset of the list of material files to be processed by another job
        auto start = it;
        auto end = it + materialsPerJob;
        if (end > files.end())
        {
            end = files.end();
        }
        AZStd::vector<MaterialBrowserRecordAssetBrowserData> subset(start, end);

        if (subset.size() > 0)
        {
            AZ::Job* materialBrowserUpdateJob = aznew MaterialBrowserUpdateJob(m_filterModel, subset, GetContext());
            StartAsChild(materialBrowserUpdateJob);
        }
    }

    WaitForChildren();
}

MaterialBrowserUpdateJob::MaterialBrowserUpdateJob(MaterialBrowserFilterModel* model, AZStd::vector<MaterialBrowserRecordAssetBrowserData>& files, AZ::JobContext* context /*= NULL*/)
    : Job(true, context)
    , m_filterModel(model)
    , m_files(files)
{
}

void MaterialBrowserUpdateJob::Process()
{
    for (size_t i = 0; i < m_files.size(); ++i)
    {
        // Early out when the job is cancelled
        if (IsCancelled())
        {
            return;
        }
        CMaterialBrowserRecord record;
        record.SetAssetBrowserData(m_files[i]);

        // Get the writable status of the file, but don't update source control status until it is actually needed
        record.m_lastCachedFileAttributes = static_cast<ESccFileAttributes>(CFileUtil::GetAttributes(record.GetFullSourcePath().c_str(), false));

        record.m_material = GetIEditor()->GetMaterialManager()->LoadMaterialWithFullSourcePath(record.GetRelativeFilePath().c_str(), record.GetFullSourcePath().c_str());
        m_filterModel->SetRecord(record);
    }
}
