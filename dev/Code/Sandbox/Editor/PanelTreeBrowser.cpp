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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "StdAfx.h"
#include "PanelTreeBrowser.h"
#include "PanelPreview.h"

#include "ViewManager.h"
#include "Viewport.h"

#include "Objects/EntityScript.h"

#include "EntityPrototype.h"
#include "EntityPrototypeLibrary.h"
#include "EntityPrototypeManager.h"
#include "Prefabs/PrefabManager.h"
#include "IThreadTask.h"
#include "Particles/ParticleItem.h"
#include "Particles/ParticleManager.h"
#include "Util/IndexedFiles.h"
#include "Objects/ParticleEffectObject.h"
#include "Objects/BrushObject.h"
#include "IObjectManager.h"

#include <QWidget>
#include <QScopedValueRollback>
#include <QSortFilterProxyModel>
#include <QRegExp>
#include <QTimer>

#include <ui_PanelTreeBrowser.h>
#include "Dialogs/QT/NewEntityDialog.h"

//-----------------------------------------------------------------------------

CObjectBrowserFileScanner::CObjectBrowserFileScanner()
{
    m_bScanning = false;
    m_bNewFiles = false;
}

void CObjectBrowserFileScanner::Scan(const QString& searchSpec)
{
    if (!m_bScanning)
    {
        m_searchSpec = searchSpec;
        m_bScanning = true;
        m_bNewFiles = false;

        QString searchPath = Path::GetPath(m_searchSpec);
        searchPath.replace('/', '\\');
        if (!searchPath.isEmpty() && !searchPath.endsWith('\\'))
        {
            searchPath += '\\';
        }
        QString fileSpec = m_searchSpec.mid(searchPath.length());
        int nToken = 0;
        QStringList tokens = fileSpec.split(';', QString::SkipEmptyParts);

        m_files.reserve(kDefaultAllocFileCount);

        while (!tokens.isEmpty())
        {
            const QString& token = tokens.first();
            QString fileSearchSpec = searchPath;

            fileSearchSpec += token;
            CFileUtil::ScanDirectory("", fileSearchSpec, m_files, true, false);

            if (!m_files.empty())
            {
                CryAutoCriticalSection lock(m_lock);

                for (size_t i = 0, iCount = m_files.size(); i < iCount; ++i)
                {
                    m_files[i].filename = Path::ToUnixPath(m_files[i].filename);
                }

                m_filesForUser.insert(m_filesForUser.end(), m_files.begin(), m_files.end());
                m_files.clear();
                m_bNewFiles = true;
            }

            tokens.pop_front();
        }

        m_bScanning = false;
    }
}

bool CObjectBrowserFileScanner::HasNewFiles()
{
    return m_bNewFiles;
}

void CObjectBrowserFileScanner::GetScannedFiles(IFileUtil::FileArray& files)
{
    CryAutoCriticalSection lock(m_lock);

    files.swap(m_filesForUser);
    m_filesForUser.clear();
    m_bNewFiles = false;
}

bool CObjectBrowserFileScanner::IsScanningForFiles()
{
    return m_bScanning;
}

/////////////////////////////////////////////////////////////////////////////
// CPanelTreeBrowserTreeView - custom tree view for CPanelTreeBrowser
// to handle drag and drop

//////////////////////////////////////////////////////////////////////////
static void PanelTreeBrowserTreeViewDropWrapper(CViewport* viewport, int ptx, int pty, void* custom)
{
    CPanelTreeBrowserTreeView* treeView = static_cast<CPanelTreeBrowserTreeView*>(custom);
    treeView->endDrop();

    //  Clear the drop callback
    int numViews = GetIEditor()->GetViewManager()->GetViewCount();
    for (int i = 0; i < numViews; ++i)
    {
        GetIEditor()->GetViewManager()->GetView(i)->SetGlobalDropCallback(nullptr, nullptr);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPanelTreeBrowserTreeView::startDrag(Qt::DropActions supportedActions)
{
    //  Also, register drag/drop callback
    int numViews = GetIEditor()->GetViewManager()->GetViewCount();
    for (int i = 0; i < numViews; ++i)
    {
        GetIEditor()->GetViewManager()->GetView(i)->SetGlobalDropCallback(&PanelTreeBrowserTreeViewDropWrapper, this);
    }

    QTreeView::startDrag(supportedActions);
}

/////////////////////////////////////////////////////////////////////////////
// CPanelTreeBrowser's sorting & filtering proxy model

CPanelTreeBrowser::SortFilterProxyModel::SortFilterProxyModel(QObject* parent /* = nullptr */)
    : QSortFilterProxyModel(parent)
{
}

CPanelTreeBrowser::SortFilterProxyModel::~SortFilterProxyModel()
{
}

bool CPanelTreeBrowser::SortFilterProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    bool leftLeaf = !sourceModel()->data(left, PathTreeModel::Role::NonLeafNodeRole).toBool();
    bool rightLeaf = !sourceModel()->data(right, PathTreeModel::Role::NonLeafNodeRole).toBool();
    if (leftLeaf ^ rightLeaf)
    {
        int leftChildren = sourceModel()->data(left, PathTreeModel::Role::ChildCountRole).toInt();
        int rightChildren = sourceModel()->data(right, PathTreeModel::Role::ChildCountRole).toInt();

        return (leftChildren - rightChildren) < 0;
    }


    QString leftLabel = sourceModel()->data(left, Qt::DisplayRole).toString();
    QString rightLabel = sourceModel()->data(right, Qt::DisplayRole).toString();
    return leftLabel > rightLabel;
}

bool CPanelTreeBrowser::SortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    // Check leaves
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    if (sourceModel()->data(index, PathTreeModel::Role::NonLeafNodeRole).toBool() == false)
    {
        return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }
    else
    {
        int childCount = index.model()->rowCount(index);
        for (int row = 0; row < childCount; row++)
        {
            if (filterAcceptsRow(row, index))
            {
                return true;
            }
        }
    }

    return false;
}

/////////////////////////////////////////////////////////////////////////////
// CPanelTreeBrowser dialog

CPanelTreeBrowser::TFileHistory CPanelTreeBrowser::sm_fileHistory;

CPanelTreeBrowser::CPanelTreeBrowser(const TSelectCallback& selectCallback, const QString& searchSpec, const EFlags& flags /* = EFlags() */, bool noExt /* = true */, QWidget* pParent /* = nullptr */)
    : QWidget(pParent)
    , m_ui(new Ui::PanelTreeBrowser)
    , m_searchSpec(searchSpec)
    , m_flags(flags)
    , m_panelPreview(nullptr)
    , m_panelPreviewId(0)
    , m_dialogType(DialogType::DEFAULT_DIALOG)
    , m_bNoExtension(true)
    , m_selectCallback(selectCallback)
{
    // Set up/get cached models before the UI is set up
    auto cachedModel = sm_fileHistory.find(searchSpec);
    if (cachedModel != sm_fileHistory.end())
    {
        m_model = cachedModel->second;
    }
    else
    {
        PathTreeModel::PathDetectionMethod pathDetectionMethod = PathTreeModel::PathSeparators;

        // these editors in particular load up libraries that should be separated by dots, not paths
        if ((m_searchSpec == "*EntityArchetype") || (m_searchSpec == "*Prefabs") || (m_searchSpec == "*ParticleEffects"))
        {
            pathDetectionMethod = PathTreeModel::Periods;
        }

        m_model = new PathTreeModel(pathDetectionMethod);
        sm_fileHistory[searchSpec] = m_model;
    }
    m_proxyModel = new SortFilterProxyModel(this);

    OnInitDialog();
}

CPanelTreeBrowser::~CPanelTreeBrowser()
{
    GetIEditor()->UnregisterNotifyListener(this);
    if (m_panelPreviewId)
    {
        GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS, m_panelPreviewId);
    }
    GetIEditor()->GetObjectManager()->RemoveObjectEventListener(functor(*this, &CPanelTreeBrowser::OnObjectEvent));
    if (gEnv->pEntitySystem)
    {
        gEnv->pEntitySystem->GetClassRegistry()->UnregisterListener(this);
    }
}

void CPanelTreeBrowser::OnEntityClassRegistryEvent(EEntityClassRegistryEvent event, const IEntityClass* pEntityClass)
{
    Refresh(true);
}

/////////////////////////////////////////////////////////////////////////////
// CPanelTreeBrowser message handlers

void CPanelTreeBrowser::OnInitDialog()
{
    m_ui->setupUi(this);
    m_model->SetDirectoryIcon(new QPixmap(":/Controls/PanelTreeBrowser/treeview-00.png"));
    m_proxyModel->setSourceModel(m_model);
    m_ui->entityTreeView->setSortingEnabled(true);
    m_ui->entityTreeView->setModel(m_proxyModel);

    m_bSelectOnClick = m_flags & SELECT_ONCLICK;
    m_ui->entityTreeView->setDragEnabled(!(m_flags & NO_DRAGDROP));

    if (m_dialogType == DialogType::DEFAULT_DIALOG)
    {
        m_ui->customButton->hide();
    }
    else
    {
        if (m_dialogType == DialogType::ME_SELECTION_DIALOG)
        {
            m_ui->customButton->setText(QStringLiteral("Select"));
        }
        else
        if (m_dialogType == DialogType::ME_REPLACE_DIALOG)
        {
            m_ui->customButton->setText(QStringLiteral("Replace"));
        }
    }

    connect(m_ui->filterEdit, &QLineEdit::editingFinished, this, &CPanelTreeBrowser::OnFilterChange);
    connect(m_ui->entityTreeView, &QTreeView::doubleClicked, this, &CPanelTreeBrowser::OnDblclkBrowserTree);
    connect(m_ui->entityTreeView, &QTreeView::clicked, this, &CPanelTreeBrowser::OnSelectionChanged);
    connect(m_ui->reloadButton, &QPushButton::pressed, this, &CPanelTreeBrowser::OnReload);
    connect(m_ui->newLuaEntityButton, &QPushButton::pressed, this, &CPanelTreeBrowser::OnNewEntity);
    // KDAB the custom button never seems to be used
    connect(m_ui->customButton, &QPushButton::pressed, this, &CPanelTreeBrowser::OnCustomBtnClick);

    GetIEditor()->RegisterNotifyListener(this);
    GetIEditor()->GetObjectManager()->AddObjectEventListener(functor(*this, &CPanelTreeBrowser::OnObjectEvent));
    if (gEnv->pEntitySystem)
    {
        gEnv->pEntitySystem->GetClassRegistry()->RegisterListener(this);
    }

    auto entityTreeView = static_cast<CPanelTreeBrowserTreeView*>(m_ui->entityTreeView);
    connect(entityTreeView, &CPanelTreeBrowserTreeView::dropFinished, this, &CPanelTreeBrowser::DropFinished);

    Refresh(false);
}

void CPanelTreeBrowser::LoadFilesFromScanning()
{
    if (m_fileScanner.HasNewFiles())
    {
        IFileUtil::FileArray files;
        m_fileScanner.GetScannedFiles(files);

        FillTreeWithFiles(files);

        UpdateFileCountLabel();
    }
}

//////////////////////////////////////////////////////////////////////////
void CPanelTreeBrowser::Refresh(bool bReloadFiles)
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    m_ui->newLuaEntityButton->hide();

    if (m_searchSpec == "*EntityClass")
    {
        FillEntityScripts(bReloadFiles);
    }
    else if (m_searchSpec == "*EntityArchetype")
    {
        FillDBLibrary(EDB_TYPE_ENTITY_ARCHETYPE);
    }
    else if (m_searchSpec == "*Prefabs")
    {
        FillDBLibrary(EDB_TYPE_PREFAB);
    }
    else if (m_searchSpec == "*ParticleEffects")
    {
        FillParticleEffects();
    }
    else
    {
        TFileHistory::iterator it = sm_fileHistory.find(m_searchSpec);

        if (it->second->LeafCount() == 0 || bReloadFiles)
        {
            if (m_fileScanner.IsScanningForFiles())
            {
                return;
            }

            m_fileScanner.Scan(m_searchSpec);
            LoadFilesFromScanning();
        }
    }

    UpdateFileCountLabel();

    QApplication::restoreOverrideCursor();

    // pretend the current selection has been selected:
    CSelectionGroup* pSel = GetIEditor()->GetSelection();

    if ((pSel) && (pSel->GetCount() == 1)) // if we have exactly one thing selected we may as well highlight it in the tree:
    {
        OnObjectEvent(pSel->GetObject(0), CBaseObject::ON_SELECT);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPanelTreeBrowser::AddPreviewPanel()
{
    if (m_panelPreview)
    {
        return;
    }
    if (!(m_flags & NO_PREVIEW) && gSettings.bPreviewGeometryWindow)
    {
        // Searching geometries.
        if (m_searchSpec.contains(QStringLiteral("*.chr")) || m_searchSpec.contains(QStringLiteral("*.cgf")) || m_searchSpec.contains(QStringLiteral("*.cga")) || m_searchSpec.contains(QStringLiteral("*ParticleEffect")))
        {
            // Create Preview.
            m_panelPreview = new CPanelPreview;
            m_panelPreviewId = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS, "Object Preview", m_panelPreview, 2); // add it after the other panels but before the end of the list.
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPanelTreeBrowser::SelectFile(const QString& filename)
{
    // delay select by a frame to allow the GUI to update its tree and build any remaining data.
    QMetaObject::invokeMethod(this, "OnSelectFile", Qt::QueuedConnection, Q_ARG(QString, filename));
}

void CPanelTreeBrowser::OnSelectFile(QString filename)
{
    QModelIndex index = m_model->IndexOf(filename);
    QModelIndex indexInFilter = m_proxyModel->mapFromSource(index);

    if (indexInFilter.isValid())
    {
        m_ui->entityTreeView->selectionModel()->select(indexInFilter, QItemSelectionModel::ClearAndSelect);
        m_ui->entityTreeView->expand(indexInFilter);
        m_ui->entityTreeView->scrollTo(indexInFilter, QAbstractItemView::PositionAtCenter);
        // its very important that this does NOT affect selection or anything like that.
        // if we allow this to m_bSelectOnClick it will actually change assignments
        QScopedValueRollback<bool> selectOnClickRollback(m_bSelectOnClick, false);
        OnSelectionChanged();
    }
}

inline bool SortEntityScripts(CEntityScript* pScript1, CEntityScript* pScript2)
{
    return pScript1->GetFile() < pScript2->GetFile();
}

//////////////////////////////////////////////////////////////////////////
void CPanelTreeBrowser::FillEntityScripts(bool bReload)
{
    if (bReload)
    {
        CEntityScriptRegistry::Instance()->Reload();
    }

    m_ui->newLuaEntityButton->show();

    m_model->BeginIterativeUpdate();

    // Entity scripts.
    std::vector<CEntityScript*> scripts;
    CEntityScriptRegistry::Instance()->GetScripts(scripts);
    std::sort(scripts.begin(), scripts.end(), SortEntityScripts);

    for (size_t i = 0; i < scripts.size(); i++)
    {
        // If class is not usable simply skip it.
        if (!scripts[i]->IsUsable())
        {
            continue;
        }

        QString name = scripts[i]->GetName();
        QString clsFile = scripts[i]->GetFile();

        const SEditorClassInfo& editorClassInfo = scripts[i]->GetClass()->GetEditorClassInfo();
        QString clsCategory = QtUtil::ToQString(editorClassInfo.sCategory);

        QString entityFilePathStr(clsFile);

        if (entityFilePathStr.contains(QStringLiteral("scripts/entities/sound/"), Qt::CaseInsensitive))
        {
            continue;
        }

        if (!clsCategory.isEmpty())
        {
            clsFile = clsCategory + "/" + name;
        }
        else if (clsFile.isEmpty())
        {
            // no script file: get the path from the Editor script table instead
            clsFile = scripts[i]->GetDisplayPath();
        }
        else
        {
            QFileInfo info(clsFile);
            clsFile = QString("%1/%2.%3").arg(info.path(), name, info.suffix());
            clsFile.replace(QStringLiteral("Scripts/Entities/"), QStringLiteral(""));
        }

        QFileInfo info(clsFile);
        clsFile = QString("%1/%2").arg(info.path(), info.baseName());

        m_model->AddNode(clsFile, scripts[i]->GetName());
    }

    m_model->EndUpdate();

    m_ui->numberOfFilesLabel->setText(QString("%1 Entities").arg(scripts.size()));
}

//////////////////////////////////////////////////////////////////////////
void CPanelTreeBrowser::FillDBLibrary(EDataBaseItemType dbType)
{
    m_model->BeginIterativeUpdate();

    IDataBaseManager* pManager = GetIEditor()->GetDBItemManager(dbType);
    for (int j = 0; j < pManager->GetLibraryCount(); j++)
    {
        IDataBaseLibrary* lib = pManager->GetLibrary(j);

        for (int i = 0; i < lib->GetItemCount(); i++)
        {
            IDataBaseItem* pItem = lib->GetItem(i);
            m_model->AddNode(pItem->GetFullName(), GuidUtil::ToString(pItem->GetGUID()));
        }
    }

    m_model->EndUpdate();

    m_ui->numberOfFilesLabel->setText(QString("%1 Entities").arg(m_model->LeafCount()));
}

void CPanelTreeBrowser::UpdateFileCountLabel()
{
    m_ui->numberOfFilesLabel->setText(QString("%1 Files").arg(m_model->LeafCount()));
}

//////////////////////////////////////////////////////////////////////////
void CPanelTreeBrowser::FillParticleEffects()
{
    m_model->BeginIterativeUpdate();

    IFileUtil::FileArray particleFiles;
    QStringList tags;
    tags.push_back("xml");
    tags.push_back("libs");
    tags.push_back("particles");

    CIndexedFiles::GetDB().GetFilesWithTags(particleFiles, tags);
    CParticleEffectObject::CFastParticleParser parser;

    for (int i = 0; i < particleFiles.size(); ++i)
    {
        const IFileUtil::FileDesc& desc = particleFiles[i];
        parser.ExtractParticlePathes(desc.filename);
        AddExtractedParticles(parser);
    }

    parser.ExtractLevelParticles();
    AddExtractedParticles(parser);

    m_model->EndUpdate();

    m_ui->numberOfFilesLabel->setText(QString("%1 Items").arg(m_model->LeafCount()));
}

int CPanelTreeBrowser::AddExtractedParticles(CParticleEffectObject::CFastParticleParser& parser)
{
    int nNumItems = 0;
    for (int k = 0; k < parser.GetCount(); k++)
    {
        if (parser.HaveParticleChildren(k))
        {
            continue;
        }

        QString itemFullName(parser.GetParticlePath(k));
        m_model->AddNode(itemFullName, itemFullName);

        nNumItems++;
    }

    return nNumItems;
}
//////////////////////////////////////////////////////////////////////////
void CPanelTreeBrowser::FillTreeWithFiles(IFileUtil::FileArray& files)
{
    m_model->BeginIterativeUpdate();

    for (size_t i = 0, iCount = files.size(); i < iCount; i++)
    {
        QString filename = files[i].filename;

        // Do not show _LODn geometries in the object browser
        if (filename.contains(QStringLiteral("_lod"), Qt::CaseInsensitive))
        {
            continue;
        }

        QFileInfo info(filename);

        if (info.suffix().compare(QStringLiteral("chrparams"), Qt::CaseInsensitive) == 0)
        {
            continue;
        }

        if (m_bNoExtension)
        {
            m_model->AddNode(info.path() + "/" + info.baseName(), filename);
        }
        else
        {
            m_model->AddNode(filename, filename);
        }
    }

    m_model->EndUpdate();
}

//////////////////////////////////////////////////////////////////////////
void CPanelTreeBrowser::OnDblclkBrowserTree(const QModelIndex& index)
{
    QString file = index.data(PathTreeModel::DataRole).toString();

    m_addingObject.clear();
    if (m_searchSpec == "*ParticleEffects")
    {
        if (CParticleEffectObject::IsGroup(file))
        {
            return;
        }
        else if (!file.isEmpty())
        {
            m_addingObject = file;
        }
    }

    if (file.isEmpty())
    {
        return;
    }

    AcceptFile(file, false);
}

//////////////////////////////////////////////////////////////////////////
void CPanelTreeBrowser::DropFinished(const QModelIndexList& indexes)
{
    Q_ASSERT(indexes.size() < 2);

    if (indexes.size() > 0)
    {
        QModelIndex index = indexes[0];
        QString file = index.data(PathTreeModel::DataRole).toString();

        // Assign to the m_addingObject param so that OnObjectEvent can copy the ParticleEffect property
        m_addingObject = file;

        // Pass true to indicate that this is a drag and drop since it's a drag and drop
        AcceptFile(file, true);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPanelTreeBrowser::AcceptFile(const QString& file, bool bDragAndDrop)
{
    if (!file.isEmpty())
    {
        // Select this file.
        if (m_selectCallback && !bDragAndDrop)
        {
            m_selectCallback(file);
        }
        if (m_dragAndDropCallback && bDragAndDrop)
        {
            m_dragAndDropCallback(file);
        }

        // Switch on Terrain and Object snapping mode
        GetIEditor()->SetAxisConstraints(AXIS_TERRAIN);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPanelTreeBrowser::OnSelectionChanged()
{
    auto selected = m_ui->entityTreeView->selectionModel()->selection();
    auto selectedItems = selected.indexes();
    if (selectedItems.isEmpty())
    {
        return;
    }

    QString itemID = selectedItems.first().data(PathTreeModel::Role::DataRole).toString();
    if (itemID.isEmpty())
    {
        return;
    }

    // If geometry update preview.
    if (m_panelPreview)
    {
        bool bParticleEffect = m_searchSpec == "*ParticleEffects";
        if (bParticleEffect)
        {
            if (!m_panelPreview->LoadParticleEffect(itemID))
            {
                return;
            }
        }
        else
        {
            m_panelPreview->LoadFile(itemID);
        }
    }

    if (m_bSelectOnClick && m_selectCallback)
    {
        m_selectCallback(itemID);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPanelTreeBrowser::OnNewEntity()
{
    NewEntityDialog EntityDlg;
    EntityDlg.exec();

    CEntityScriptRegistry::Instance()->Reload();
}

//////////////////////////////////////////////////////////////////////////
void CPanelTreeBrowser::OnReload()
{
    m_ui->filterEdit->clear();
    Refresh(true);
}

//////////////////////////////////////////////////////////////////////////
void CPanelTreeBrowser::OnEditorNotifyEvent(EEditorNotifyEvent aEvent)
{
    if (aEvent == eNotify_OnIdleUpdate)
    {
        //TODO: empty, reserved for thread folder structure load
    }
}

void CPanelTreeBrowser::OnCustomBtnClick()
{
}

//////////////////////////////////////////////////////////////////////////
void CPanelTreeBrowser::OnFilterChange()
{
    QString filterText = m_ui->filterEdit->text();
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setFilterRegExp(QString(".*%1.*").arg(QRegExp::escape(m_ui->filterEdit->text())));
    
    // if the filter text is empty, retain whatever tree structure is currently present (ie, don't mess with expansion!)
    if (!filterText.isEmpty())
    {
        // expand the tree nodes that are visible
        QModelIndexList indexes = m_proxyModel->match(m_proxyModel->index(0, 0), Qt::DisplayRole, "*", -1, Qt::MatchWildcard | Qt::MatchRecursive);
        for (QModelIndex index : indexes)
        {
            m_ui->entityTreeView->expand(index);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPanelTreeBrowser::OnObjectEvent(CBaseObject* object, int event)
{
    switch (event)
    {
        case CBaseObject::ON_SELECT:
        {
            IStatObj* statObj = GetIEditor()->GetObjectManager()->GetGeometryFromObject(object);
            if (statObj)
            {
                QString geomFile = statObj->GetFilePath();
                SelectFile(geomFile);
            }
        }
        break;
        case CBaseObject::ON_ADD:
        {
            if (!m_addingObject.isEmpty())
            {
                if (m_searchSpec == "*ParticleEffects")
                {
                    CParticleEffectObject* pEntity = qobject_cast<CParticleEffectObject*>(object);
                    if (pEntity)
                    {
                        QString particleName = m_addingObject;
                        IVariable* pVar = 0;
                        if (pEntity->GetProperties())
                        {
                            pVar = pEntity->GetProperties()->FindVariable("ParticleEffect");
                            if (pVar && pVar->GetType() == IVariable::STRING)
                            {
                                pVar->Set(particleName);
                            }
                        }
                    }
                }
                m_addingObject.clear();
            }
        }
        break;
    }
}

#include <PanelTreeBrowser.moc>
