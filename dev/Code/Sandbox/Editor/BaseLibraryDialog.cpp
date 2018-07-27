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
#include "BaseLibraryDialog.h"

#include "StringDlg.h"

#include "BaseLibrary.h"
#include "BaseLibraryItem.h"
#include "BaseLibraryManager.h"
#include "Clipboard.h"
#include "ErrorReport.h"
#include "IEditor.h"
#include "IEditorParticleUtils.h"

#include "Prefabs/PrefabManager.h"
#include "Prefabs/PrefabItem.h"

#include "Particles/ParticleItem.h"

#include <QtUtil.h>

#include "../Plugins/EditorUI_QT/Toolbar.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QDrag>
#include <QMimeData>
#include <QItemDelegate>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>

QDataStream & operator<<(QDataStream& out, CBaseLibraryItem* item)
{
    out.writeBytes(reinterpret_cast<const char*>(&item), sizeof(CBaseLibraryItem*));
    return out;
}
QDataStream& operator>>(QDataStream& in, CBaseLibraryItem*& item)
{
    in.readRawData(reinterpret_cast<char*>(&item), sizeof(CBaseLibraryItem*));
    return in;
}

class BaseLibraryItemDelegate
    : public QItemDelegate
{
public:
    BaseLibraryItemDelegate(QObject* parent = nullptr)
        : QItemDelegate(parent)
    {
    }

    void drawDisplay(QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect, const QString& text) const override
    {
        QStyleOptionViewItemV4 opt(option);
        opt.font.setBold(opt.state & QStyle::State_MouseOver);
        QItemDelegate::drawDisplay(painter, opt, rect, text);
    }
};

BaseLibraryDialogTree::BaseLibraryDialogTree(QWidget* parent)
    : QTreeView(parent)
{
    setMouseTracking(true);
    setItemDelegate(new BaseLibraryItemDelegate(this));
}

bool BaseLibraryDialogTree::viewportEvent(QEvent* event)
{
    switch (event->type())
    {
    case QEvent::DragEnter:
    case QEvent::DragMove:
    case QEvent::DragLeave:
    case QEvent::Drop:
    {
        // special handling by the dialog, if overridden
        QWidget* parent = this;
        while (parent != nullptr)
        {
            if (CBaseLibraryDialog* dialog = qobject_cast<CBaseLibraryDialog*>(parent))
            {
                return dialog->event(event);
            }
            parent = parent->parentWidget();
        }
        break;
    }
    default:
        break;
    }

    return QTreeView::viewportEvent(event);
}

void BaseLibraryDialogTree::startDrag(Qt::DropActions supportedDropActions)
{
    // set drag operation handler for viewports
    GetIEditor()->GetParticleUtils()->SetViewportDragOperation([](CViewport* vp, int dpx, int dpy, void* custom)
        {
            auto that = reinterpret_cast<BaseLibraryDialogTree*>(custom);
            that->droppedOnViewport(that->selectionModel()->selectedRows(), vp);
        }, this);

    QTreeView::startDrag(supportedDropActions);

    // set drag operation handler for viewports
    GetIEditor()->GetParticleUtils()->SetViewportDragOperation(0, 0);
}

DefaultBaseLibraryDialogModel::DefaultBaseLibraryDialogModel(QObject* parent)
    : QAbstractItemModel(parent)
    , m_library(nullptr)
    , m_sortRecursionType(CBaseLibraryDialog::SORT_RECURSION_FULL)
{
    m_imageListPreFab.push_back(QPixmap(":/prefabs_tree_00.png"));
    m_imageListPreFab.push_back(QPixmap(":/prefabs_tree_01.png"));
    m_imageListPreFab.push_back(QPixmap(":/prefabs_tree_02.png"));
    m_imageListPreFab.push_back(QPixmap(":/prefabs_tree_03.png"));
    m_imageListParticles.push_back(QPixmap(":/particles_tree_00.png"));
    m_imageListParticles.push_back(QPixmap(":/particles_tree_01.png"));
    m_imageListParticles.push_back(QPixmap(":/particles_tree_02.png"));
    m_imageListParticles.push_back(QPixmap(":/particles_tree_03.png"));
    m_imageListParticles.push_back(QPixmap(":/particles_tree_04.png"));
    m_imageListParticles.push_back(QPixmap(":/particles_tree_05.png"));
    m_imageListParticles.push_back(QPixmap(":/particles_tree_06.png"));
    m_imageListParticles.push_back(QPixmap(":/particles_tree_07.png"));
}

void DefaultBaseLibraryDialogModel::setLibrary(CBaseLibrary* lib)
{
    beginResetModel();
    m_library = lib;
    m_items.clear();
    for (int i = 0; m_library && i < m_library->GetItemCount(); ++i)
    {
        m_items.push_back(m_library->GetItem(i));
    }
    endResetModel();
}

void DefaultBaseLibraryDialogModel::setSortRecursionType(int recursionType)
{
    m_sortRecursionType = recursionType;
}

bool DefaultBaseLibraryDialogModel::setData(const QModelIndex& index, const QVariant& data, int role)
{
    if (role != Qt::EditRole && role != Qt::DisplayRole)
    {
        return false;
    }
    if (!data.canConvert<QString>())
    {
        return false;
    }
    Q_ASSERT(this->index(index.row(), index.column(), index.parent()).isValid());
    CBaseLibraryItem* item = reinterpret_cast<CBaseLibraryItem*>(index.internalPointer());
    item->SetName(data.toString());
    emit dataChanged(index, index);
    return true;
}

QVariant DefaultBaseLibraryDialogModel::data(const QModelIndex& index, int role) const
{
    Q_ASSERT(this->index(index.row(), index.column(), index.parent()).isValid());
    CBaseLibraryItem* item = reinterpret_cast<CBaseLibraryItem*>(index.internalPointer());
    switch (role)
    {
    case Qt::DisplayRole:
        if (!item->GetGroupName().isEmpty())
        {
            switch (item->GetType())
            {
            case EDB_TYPE_PREFAB:
                return QStringLiteral("%1 (%2)").arg(item->GetName()).arg(GetIEditor()->GetPrefabManager()->GetPrefabInstanceCount(static_cast<CPrefabItem*>(item)));
                break;
            }
        }
        return item->GetName();
    case Qt::DecorationRole:
        switch (item->GetType())
        {
        case EDB_TYPE_PREFAB:
        {
            QIcon icon;
            icon.addPixmap(m_imageListPreFab.value(2), QIcon::Normal);
            icon.addPixmap(m_imageListPreFab.value(3), QIcon::Selected);
            return icon;
        }
        case EDB_TYPE_PARTICLE:
        {
            QIcon icon;
            static int nIconStates[4] = { 6, 7, 4, 5 };
            icon.addPixmap(m_imageListParticles.value(nIconStates[static_cast<CParticleItem*>(item)->GetEnabledState() & 3 ]), QIcon::Normal);
            return icon;
        }
        default:
            return QVariant();
        }
    case CBaseLibraryDialog::BaseLibraryItemRole:
        return QVariant::fromValue(item);
    }
    return QVariant();
}

Qt::ItemFlags DefaultBaseLibraryDialogModel::flags(const QModelIndex& index) const
{
    return QAbstractItemModel::flags(index) | Qt::ItemIsDragEnabled;
}

QModelIndex DefaultBaseLibraryDialogModel::index(int row, int column, const QModelIndex& parent) const
{
    if (row < 0 || row >= rowCount(parent) || column < 0 || column >= columnCount(parent))
    {
        return QModelIndex();
    }
    // the internal pointer is the library item
    return createIndex(row, column, m_items[row]);
}

QModelIndex DefaultBaseLibraryDialogModel::index(CBaseLibraryItem* item) const
{
    auto it = std::find(m_items.begin(), m_items.end(), item);
    if (it == m_items.end())
    {
        return QModelIndex();
    }
    else
    {
        return createIndex(it - m_items.begin(), 0, item);
    }
}

QModelIndex DefaultBaseLibraryDialogModel::parent(const QModelIndex& index) const
{
    return QModelIndex();
}

int DefaultBaseLibraryDialogModel::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid() && m_library)
    {
        return m_items.size();
    }
    return 0;
}

int DefaultBaseLibraryDialogModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 1;
}

bool DefaultBaseLibraryDialogModel::removeRows(int first, int count, const QModelIndex& parent)
{
    const int last = first + count - 1;
    if (first < 0 || count <= 0 || last > rowCount() - 1 || parent.isValid())
    {
        return false;
    }
    beginRemoveRows(QModelIndex(), first, last);
    for (int i = last; i >= first; --i)
    {
        auto item = m_items[i];
        m_library->RemoveItem(item);
        m_items.erase(m_items.begin() + i);
    }
    Q_ASSERT(m_library->GetItemCount() == m_items.size());
    endRemoveRows();
    return true;
}

void DefaultBaseLibraryDialogModel::sort(int column, Qt::SortOrder sortOrder)
{
    if (column != 0)
    {
        return;
    }

    emit layoutAboutToBeChanged();
    std::sort(m_items.begin(), m_items.end(), [&](IDataBaseItem* lhs, IDataBaseItem* rhs)
        {
            static QRegularExpression exp(QStringLiteral("\\\\/\\."));
            QStringList lhsName = lhs->GetName().split(exp);
            QStringList rhsName = lhs->GetName().split(exp);

            if (lhsName.count() > m_sortRecursionType)
            {
                lhsName.erase(lhsName.begin() + m_sortRecursionType, lhsName.end());
            }
            if (rhsName.count() > m_sortRecursionType)
            {
                rhsName.erase(rhsName.begin() + m_sortRecursionType, rhsName.end());
            }

            return lhsName.join(QStringLiteral(".")) < rhsName.join(QStringLiteral("."));
        });

    if (sortOrder == Qt::DescendingOrder)
    {
        std::reverse(m_items.begin(), m_items.end());
    }

    emit layoutChanged();
}

BaseLibraryDialogProxyModel::BaseLibraryDialogProxyModel(QObject* parent, QAbstractItemModel* model, BaseLibraryDialogModel* libraryModel)
    : AbstractGroupProxyModel(parent)
    , m_libraryModel(libraryModel)
{
    setSourceModel(model);
}

QModelIndex BaseLibraryDialogProxyModel::index(CBaseLibraryItem* item) const
{
    return mapFromSource(m_libraryModel->index(item));
}

QVariant BaseLibraryDialogProxyModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        CBaseLibraryItem* item = AbstractGroupProxyModel::data(index, CBaseLibraryDialog::BaseLibraryItemRole).value<CBaseLibraryItem*>();
        if (item)
        {
            return item->GetShortName();
        }
    }
    return AbstractGroupProxyModel::data(index, role);
}

QStringList BaseLibraryDialogProxyModel::GroupForSourceIndex(const QModelIndex& sourceIndex) const
{
    static QRegularExpression exp(QStringLiteral("\\\\/\\."));
    CBaseLibraryItem* item = sourceIndex.data(CBaseLibraryDialog::BaseLibraryItemRole).value<CBaseLibraryItem*>();
    if (item->GetChildCount() == 0)
    {
        return item->GetGroupName().split(exp);
    }
    else
    {
        return item->GetName().split(exp);
    }
}

bool BaseLibraryDialogProxyModel::IsGroupIndex(const QModelIndex& sourceIndex) const
{
    CBaseLibraryItem* item = sourceIndex.data(CBaseLibraryDialog::BaseLibraryItemRole).value<CBaseLibraryItem*>();
    return item->GetChildCount() > 0;
}

//////////////////////////////////////////////////////////////////////////
// CBaseLibraryDialog implementation.
//////////////////////////////////////////////////////////////////////////
CBaseLibraryDialog::CBaseLibraryDialog(UINT nID, QWidget* pParent)
    : CDataBaseDialogPage(nID, pParent)
    , m_libraryCtrl(0)
{
    qRegisterMetaType<CBaseLibraryItem*>();
    qRegisterMetaTypeStreamOperators<CBaseLibraryItem*>();

    m_sortRecursionType = SORT_RECURSION_FULL;

    m_bIgnoreSelectionChange = false;

    m_pItemManager = 0;

    m_bLibsLoaded = false;

    GetIEditor()->RegisterNotifyListener(this);
}

CBaseLibraryDialog::CBaseLibraryDialog(QWidget* pParent)
    : CDataBaseDialogPage(pParent)
{
    qRegisterMetaType<CBaseLibraryItem*>();
    qRegisterMetaTypeStreamOperators<CBaseLibraryItem*>();
    m_sortRecursionType = SORT_RECURSION_FULL;

    m_bIgnoreSelectionChange = false;

    m_pItemManager = 0;

    m_bLibsLoaded = false;

    GetIEditor()->RegisterNotifyListener(this);
}

CBaseLibraryDialog::~CBaseLibraryDialog()
{
    GetIEditor()->UnregisterNotifyListener(this);
    GetIEditor()->GetUndoManager()->RemoveListener(this);
}

// CTVSelectKeyDialog message handlers
void CBaseLibraryDialog::OnInitDialog()
{
}

void CBaseLibraryDialog::keyPressEvent(QKeyEvent* e)
{
    // eat the escape key press; otherwise we get a blank window inside the container that
    // this dialog was parented to
    if (e->key() != Qt::Key_Escape)
    {
        CDataBaseDialogPage::keyPressEvent(e);
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryDialog::InitLibraryToolbar()
{
    // Create Library toolbar.
    IToolbar* pLibToolBar = IToolbar::createToolBar(this, IDR_DB_LIBRARY_BAR);
    //pLibToolBar->EnableCustomization(FALSE);

    m_saveAction = pLibToolBar->getAction("actionLibrarySave");
    connect(pLibToolBar->getAction("actionLibraryAdd"), &QAction::triggered, this, &CBaseLibraryDialog::OnAddLibrary);
    connect(pLibToolBar->getAction("actionLibraryRemove"), &QAction::triggered, this, &CBaseLibraryDialog::OnRemoveLibrary);
    connect(m_saveAction, &QAction::triggered, this, &CBaseLibraryDialog::OnSave);
    connect(pLibToolBar->getAction("actionLibraryReload"), &QAction::triggered, this, &CBaseLibraryDialog::OnReloadLib);
    connect(pLibToolBar->getAction("actionLibraryLoad"), &QAction::triggered, this, &CBaseLibraryDialog::OnLoadLibrary);

    m_libraryCtrl = qobject_cast<QComboBox*>(pLibToolBar->getObject("library_bar"));
    connect(m_libraryCtrl, QtUtil::Select<int>::OverloadOf(&QComboBox::activated), this, &CBaseLibraryDialog::OnChangedLibrary);

    //////////////////////////////////////////////////////////////////////////
    // Create Item toolbar.
    m_pItemToolBar = IToolbar::createToolBar(this, IDR_DB_LIBRARY_ITEM_BAR);
    connect(m_pItemToolBar->getAction("actionItemClone"), &QAction::triggered, this, &CBaseLibraryDialog::OnClone);
    connect(m_pItemToolBar->getAction("actionItemRemove"), &QAction::triggered, this, &CBaseLibraryDialog::OnRemoveItem);

    //////////////////////////////////////////////////////////////////////////
    // Standard toolbar.
    m_pStdToolBar = IToolbar::createToolBar(this, IDR_DB_STANDART);
    m_copyAction = m_pStdToolBar->getAction("actionStandardCopy");
    m_pasteAction = m_pStdToolBar->getAction("actionStandardPaste");
    connect(m_copyAction, &QAction::triggered, this, &CBaseLibraryDialog::OnCopy);
    connect(m_pasteAction, &QAction::triggered, this, &CBaseLibraryDialog::OnPaste);

    m_undoAction = m_pStdToolBar->getAction("actionStandardUndo");
    m_redoAction = m_pStdToolBar->getAction("actionStandardRedo");
    connect(m_undoAction, &QAction::triggered, this, []() { GetIEditor()->GetUndoManager()->Undo(); });
    connect(m_redoAction, &QAction::triggered, this, []() { GetIEditor()->GetUndoManager()->Redo(); });
    
    GetIEditor()->GetUndoManager()->AddListener(this);

    //////////////////////////////////////////////////////////////////////////
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryDialog::InitItemToolbar()
{
}

//////////////////////////////////////////////////////////////////////////
// Create the toolbar
void CBaseLibraryDialog::InitToolbar(UINT nToolbarResID)
{
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryDialog::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginNewScene:
        m_bLibsLoaded = false;
        // Clear all prototypes and libraries.
        SelectItem(0);
        if (m_libraryCtrl)
        {
            m_libraryCtrl->clear();
        }
        m_selectedLib = "";
        break;

    case eNotify_OnEndNewScene:
    case eNotify_OnEndSceneOpen:
        m_bLibsLoaded = false;
        ReloadLibs();
        break;

    case eNotify_OnCloseScene:
        m_bLibsLoaded = false;
        SelectLibrary("");
        SelectItem(0);
        break;

    case eNotify_OnDataBaseUpdate:
        if (m_pLibrary && m_pLibrary->IsModified())
        {
            ReloadItems();
        }
        ReloadLibs();
        break;
    }

    OnUpdateActions();
}

void CBaseLibraryDialog::OnDataBaseItemEvent(IDataBaseItem* pItem, EDataBaseItemEvent event)
{
    OnUpdateActions();
}

//////////////////////////////////////////////////////////////////////////
CBaseLibrary* CBaseLibraryDialog::FindLibrary(const QString& libraryName)
{
    return (CBaseLibrary*)m_pItemManager->FindLibrary(libraryName);
}

CBaseLibrary* CBaseLibraryDialog::NewLibrary(const QString& libraryName)
{
    bool isLevelLibrary = false;
    return (CBaseLibrary*)m_pItemManager->AddLibrary(libraryName, isLevelLibrary);
}

void CBaseLibraryDialog::DeleteLibrary(CBaseLibrary* pLibrary)
{
    m_pItemManager->DeleteLibrary(pLibrary->GetName());
}

void CBaseLibraryDialog::DeleteItem(CBaseLibraryItem* pItem)
{
    GetSourceModel()->removeRow(GetSourceDialogModel()->index(pItem).row());
    m_pItemManager->DeleteItem(pItem);
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryDialog::ReloadLibs()
{
    if (!m_pItemManager)
    {
        return;
    }

    SelectItem(0);

    QString selectedLib = m_libraryCtrl->currentText();
    if (m_libraryCtrl)
    {
        m_libraryCtrl->clear();
    }
    bool bFound = false;
    for (int i = 0; i < m_pItemManager->GetLibraryCount(); i++)
    {
        QString library = m_pItemManager->GetLibrary(i)->GetName();
        if (selectedLib.isEmpty())
        {
            selectedLib = library;
        }
        if (m_libraryCtrl)
        {
            m_libraryCtrl->addItem(library);
        }
    }
    m_selectedLib = "";
    SelectLibrary(selectedLib);
    m_bLibsLoaded = true;
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryDialog::ReloadItems()
{
    m_bIgnoreSelectionChange = true;
    SelectItem(0);
    m_selectedGroup = "";
    m_pCurrentItem = 0;
    m_cpoSelectedLibraryItems.clear();
    if (m_pItemManager)
    {
        m_pItemManager->SetSelectedItem(0);
    }
    ReleasePreviewControl();
    m_bIgnoreSelectionChange = false;

    GetSourceDialogModel()->setLibrary(m_pLibrary);

    SortItems(m_sortRecursionType);

    m_bIgnoreSelectionChange = false;
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryDialog::SelectLibrary(const QString& library, bool bForceSelect)
{
    QWaitCursor wait;
    if (m_selectedLib != library || bForceSelect)
    {
        SelectItem(0);
        m_pLibrary = FindLibrary(library);
        GetSourceDialogModel()->setLibrary(m_pLibrary);
        if (m_pLibrary)
        {
            m_selectedLib = library;
        }
        else
        {
            m_selectedLib = "";
        }
        ReloadItems();
    }
    if (m_libraryCtrl)
    {
        m_libraryCtrl->setCurrentText(m_selectedLib);
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryDialog::OnChangedLibrary()
{
    QString library;
    if (m_libraryCtrl)
    {
        library = m_libraryCtrl->currentText();
    }
    if (library != m_selectedLib)
    {
        SelectLibrary(library);
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryDialog::OnAddLibrary()
{
    StringDlg dlg("New Library Name", this);

    dlg.SetCheckCallback([this](QString library) -> bool
        {
            QString path = m_pItemManager->MakeFilename(library);
            if (CFileUtil::FileExists(path))
            {
                QString dataDir = Path::GetEditingGameDataFolder().c_str();
                QString dataFile = m_pItemManager->MakeFilename(library);
                QMessageBox::warning(this, tr("Library exists"), tr("Library '%1' already exists within %2, see file: %3").arg(path).arg(dataDir).arg(dataFile));
                return false;
            }
            return true;
        });

    if (dlg.exec() == QDialog::Accepted)
    {
        if (!dlg.GetString().isEmpty())
        {
            SelectItem(0);
            // Make new library.
            QString library = dlg.GetString();
            NewLibrary(library);
            ReloadLibs();
            SelectLibrary(library);
            GetIEditor()->SetModifiedFlag();

            // CONFETTI START: Leroy Sikkes
            // Notify others that database changed.
            NotifyExceptSelf(eNotify_OnDataBaseUpdate);
            // CONFETTI END
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryDialog::OnExportLibrary()
{
    if (!m_pLibrary)
    {
        return;
    }

    QString filename;
    if (CFileUtil::SelectSaveFile("Library XML Files (*.xml)", "xml", (Path::GetEditingGameDataFolder() + "/Materials").c_str(), filename))
    {
        XmlNodeRef libNode = XmlHelpers::CreateXmlNode("MaterialLibrary");
        m_pLibrary->Serialize(libNode, false);
        XmlHelpers::SaveXmlNode(GetIEditor()->GetFileUtil(), libNode, filename.toStdString().c_str());
    }
}

bool CBaseLibraryDialog::SetItemName(CBaseLibraryItem* item, const QString& groupName, const QString& itemName)
{
    assert(item);
    // Make prototype name.
    QString name;
    if (!groupName.isEmpty())
    {
        name = groupName + ".";
    }
    name += itemName;
    QString fullName = name;
    if (item->GetLibrary())
    {
        fullName = item->GetLibrary()->GetName() + "." + name;
    }
    IDataBaseItem* pOtherItem = m_pItemManager->FindItemByName(fullName);
    if (pOtherItem && pOtherItem != item)
    {
        // Ensure uniqness of name.
        Warning("Duplicate Item Name %s", name.toUtf8().data());
        return false;
    }
    else
    {
        const QModelIndex index = GetSourceDialogModel()->index(item);
        if (index.isValid())
        {
            GetSourceModel()->setData(index, name);
        }
        else
        {
            item->SetName(name);
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryDialog::OnAddItem()
{
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryDialog::OnRemoveItem()
{
    // When we have no set of selected items, it may be the case that we are
    // dealing with old fashioned selection. If such is the case, let's deal
    // with it using the old code system, which should be deprecated.
    if (m_cpoSelectedLibraryItems.empty())
    {
        if (m_pCurrentItem)
        {
            // Remove prototype from prototype manager and library.
            if (QMessageBox::question(this, tr("Delete confirmation"), tr("Delete %1?").arg(m_pCurrentItem->GetName())) == QMessageBox::Yes)
            {
                CUndo undo("Remove library item");
                TSmartPtr<CBaseLibraryItem> pCurrent = m_pCurrentItem;
                const QModelIndex index = GetSourceDialogModel()->index(pCurrent);
                const QModelIndex parent = GetProxyModel()->parent(GetProxyModel()->mapFromSource(index));
                if (parent.isValid())
                {
                    SelectItem(parent.data(BaseLibraryItemRole).value<CBaseLibraryItem*>());
                }
                GetSourceModel()->removeRow(index.row());
                GetIEditor()->SetModifiedFlag();
            }
        }
    }
    else // This is to be used when deleting multiple items...
    {
        QString strMessageString = tr("Delete the following items:\n");
        int nItemCount(0);

        auto itEnd = m_cpoSelectedLibraryItems.end();
        auto itCurrentIterator = m_cpoSelectedLibraryItems.begin();
        // For now, we have a maximum limit of 7 items per messagebox...
        for (nItemCount = 0; nItemCount < 7; ++nItemCount, itCurrentIterator++)
        {
            if (itCurrentIterator == itEnd)
            {
                // As there were less than 7 items selected, we got to put them all
                // into the formated string for the messagebox.
                break;
            }
            strMessageString.append(QString::fromLatin1("%1\n").arg((*itCurrentIterator)->GetName()));
        }
        if (itCurrentIterator != itEnd)
        {
            strMessageString.append("...");
        }

        if (QMessageBox::question(this, tr("Delete Confirmation"), strMessageString) == QMessageBox::Yes)
        {
            foreach(const auto pCurrent, m_cpoSelectedLibraryItems)
            {
                DeleteItem(pCurrent);
            }
            m_cpoSelectedLibraryItems.clear();
            GetIEditor()->SetModifiedFlag();
            SelectItem(0);
            ReloadItems();
        }
    }

    // CONFETTI START: Leroy Sikkes
    // Notify others that database changed.
    NotifyExceptSelf(eNotify_OnDataBaseUpdate);
    // CONFETTI END
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryDialog::OnRenameItem()
{
    if (m_pCurrentItem)
    {
        StringGroupDlg dlg;
        dlg.SetGroup(m_pCurrentItem->GetGroupName());
        dlg.SetString(m_pCurrentItem->GetShortName());
        if (dlg.exec() == QDialog::Accepted)
        {
            static bool warn = true;
            if (warn)
            {
                warn = false;
                QMessageBox::warning(this, tr("Warning"), tr("Levels referencing this archetype will need to be exported."));
            }

            CUndo undo("Rename library item");
            TSmartPtr<CBaseLibraryItem> curItem = m_pCurrentItem;
            SetItemName(curItem, dlg.GetGroup(), dlg.GetString());
            ReloadItems();
            SelectItem(curItem, true);
            curItem->SetModified();
            //m_pCurrentItem->Update();
        }
        GetIEditor()->SetModifiedFlag();

        // CONFETTI START: Leroy Sikkes
        // Notify others that database changed.
        NotifyExceptSelf(eNotify_OnDataBaseUpdate);
        // CONFETTI END
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryDialog::OnRemoveLibrary()
{
    QString library = m_selectedLib;
    if (library.isEmpty())
    {
        return;
    }
    if (m_pLibrary->IsModified())
    {
        QString ask = tr("Save changes to the library %1?").arg(library);
        if (QMessageBox::question(this, QString(), ask) == QMessageBox::Yes)
        {
            OnSave();
        }
    }
    QString ask = tr("When removing library All items contained in this library will be deleted.\r\nAre you sure you want to remove libarary %1?\r\n(Note: Library file will not be deleted from the disk)").arg(library);
    if (QMessageBox::question(this, QString(), ask) == QMessageBox::Yes)
    {
        SelectItem(0);
        DeleteLibrary(m_pLibrary);
        m_selectedLib = "";
        m_pLibrary = 0;
        ReleasePreviewControl();
        ReloadLibs();
        GetIEditor()->SetModifiedFlag();

        // CONFETTI START: Leroy Sikkes
        // Notify others that database changed.
        NotifyExceptSelf(eNotify_OnDataBaseUpdate);
        // CONFETTI END
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryDialog::OnSelChangedItemTree(const QModelIndex& current, const QModelIndex& previous)
{
    if (m_bIgnoreSelectionChange)
    {
        return;
    }

    // For the time being this is deactivated due to lack of testing.
    //// This might be true always.
    //if (pNMTreeView->itemNew.mask&TVIF_STATE)
    //{
    //  HTREEITEM                   hCurrentItem(pNMTreeView->itemNew.hItem);
    //  CBaseLibraryItem* poBaseTreeItem((CBaseLibraryItem*)GetTreeCtrl().GetItemData(hCurrentItem));

    //  // When the item was not selected and will be selected now...
    //  if (pNMTreeView->itemNew.state&TVIS_SELECTED)
    //  {
    //      m_cpoSelectedLibraryItems.insert(poBaseTreeItem);
    //  }
    //  else // If the item was selected and it will not be selected anymore...
    //  {
    //      m_cpoSelectedLibraryItems.erase(poBaseTreeItem);
    //  }
    //}

    bool bSelected = false;
    m_pCurrentItem = 0;
    if (GetTreeCtrl())
    {
        if (current.isValid())
        {
            // Change currently selected item.
            CBaseLibraryItem* prot = current.data(BaseLibraryItemRole).value<CBaseLibraryItem*>();
            if (prot)
            {
                SelectItem(prot);
                bSelected = true;
            }
            else
            {
                m_selectedGroup = GetItemFullName(current, ".");
            }
        }
    }
    if (!bSelected)
    {
        SelectItem(0);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CBaseLibraryDialog::CanSelectItem(CBaseLibraryItem* pItem)
{
    assert(pItem);
    // Check if this item is in dialogs manager.
    if (m_pItemManager->FindItem(pItem->GetGUID()) == pItem)
    {
        return true;
    }
    return false;
}

void CBaseLibraryDialog::SignalNumUndoRedo(const unsigned int& numUndo, const unsigned int& numRedo)
{
    m_undoAction->setEnabled(numUndo > 0);
    m_redoAction->setEnabled(numRedo > 0);
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryDialog::SelectItem(CBaseLibraryItem* item, bool bForceReload)
{
    if (item == m_pCurrentItem && !bForceReload)
    {
        return;
    }

    if (item)
    {
        // Selecting item from different library.
        if (item->GetLibrary() != m_pLibrary && item->GetLibrary())
        {
            // Select library first.
            SelectLibrary(item->GetLibrary()->GetName());
        }
    }

    m_pCurrentItem = item;

    if (item)
    {
        m_selectedGroup = item->GetGroupName();
    }
    else
    {
        m_selectedGroup = "";
    }

    m_pCurrentItem = item;

    // Set item visible.
    QModelIndex index = GetSourceDialogModel()->index(item);
    index = GetProxyModel()->mapFromSource(index);
    if (index.isValid())
    {
        GetTreeCtrl()->setCurrentIndex(index);
        GetTreeCtrl()->scrollTo(index);
    }
    else
    {
        GetTreeCtrl()->clearSelection();
    }

    // do this after informing the tree control to change the selection
    OnUpdateActions();

    // [MichaelS 17/4/2007] Select this item in the manager so that other systems
    // can find out the selected item without having to access the ui directly.
    m_pItemManager->SetSelectedItem(item);
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryDialog::Update()
{
    // do nothing here.
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryDialog::OnSave()
{
    m_pItemManager->SaveAllLibs();
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryDialog::Reload()
{
    ReloadLibs();
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryDialog::SetActive(bool bActive)
{
    if (bActive && !m_bLibsLoaded)
    {
        ReloadLibs();
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryDialog::OnUpdateActions()
{
    const bool enable = m_pCurrentItem != nullptr;
    if (m_cutAction)
    {
        m_cutAction->setEnabled(enable);
    }

    if (m_copyAction)
    {
        m_copyAction->setEnabled(enable);
    }

    if (m_cloneAction)
    {
        m_cloneAction->setEnabled(enable);
    }

    if (m_assignAction)
    {
        m_assignAction->setEnabled(enable);
    }

    if (m_selectAction)
    {
        m_selectAction->setEnabled(enable);
    }

    if (m_saveAction)
    {
        const bool enableSave = m_pItemManager->GetModifiedLibraryCount() > 0;
        m_saveAction->setEnabled(enableSave);
    }

    if (m_pasteAction)
    {
        CClipboard clipboard(this);
        const bool enablePaste = !clipboard.IsEmpty();
        m_pasteAction->setEnabled(enablePaste);
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryDialog::OnClone()
{
    auto selectedIndexes = GetTreeCtrl()->selectionModel()->selectedIndexes();
    if (selectedIndexes.size() == 1 && !selectedIndexes.first().parent().isValid())
    {
        return;
    }

    OnCopy();
    OnPaste();

    // CONFETTI START: Leroy Sikkes
    // Notify others that database changed.
    NotifyExceptSelf(eNotify_OnDataBaseUpdate);
    // CONFETTI END
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryDialog::OnCopyPath()
{
    if (!m_pCurrentItem)
    {
        return;
    }

    QString str;
    if (m_pCurrentItem->GetLibrary())
    {
        str = m_pCurrentItem->GetLibrary()->GetName() + ".";
    }
    str += m_pCurrentItem->GetName();

    CClipboard clipboard(this);
    clipboard.PutString(str);
}


//////////////////////////////////////////////////////////////////////////
void CBaseLibraryDialog::OnCut()
{
    if (m_pCurrentItem)
    {
        OnCopy();
        OnRemoveItem();
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryDialog::OnReloadLib()
{
    if (!m_pLibrary)
    {
        return;
    }

    QString libname = m_pLibrary->GetName();
    QString file = m_pLibrary->GetFilename();
    if (m_pLibrary->IsModified())
    {
        QString str = tr("Layer %1 was modified.\nReloading layer will discard all modifications to this library!").arg(libname);
        if (QMessageBox::question(this, QString(), str, QMessageBox::Ok | QMessageBox::Cancel) != QMessageBox::Ok)
        {
            return;
        }
    }

    m_pItemManager->DeleteLibrary(libname);
    m_pItemManager->LoadLibrary(file, true);
    ReloadLibs();
    SelectLibrary(libname);

    // CONFETTI START: Leroy Sikkes
    // Notify others that database changed.
    NotifyExceptSelf(eNotify_OnDataBaseUpdate);
    // CONFETTI END
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryDialog::OnLoadLibrary()
{
    assert(m_pItemManager);
    LoadLibrary();
}

void CBaseLibraryDialog::LoadLibrary()
{
    CErrorsRecorder errorRecorder(GetIEditor());

    AssetSelectionModel selection = AssetSelectionModel::AssetTypeSelection(GetLibraryAssetTypeName(), true);
    AzToolsFramework::EditorRequests::Bus::Broadcast(
        &AzToolsFramework::EditorRequests::BrowseForAssets, 
        selection);
    if (selection.IsValid())
    {
        GetIEditor()->SuspendUndo();
        IDataBaseLibrary* matLib = nullptr;

        for (auto result : selection.GetResults())
        {
            QString relpath = Path::RemoveBackslash(result->GetRelativePath().c_str());
            matLib = m_pItemManager->LoadLibrary(relpath);
        }

        GetIEditor()->ResumeUndo();
        ReloadLibs();

        if (matLib)
        {
            SelectLibrary(matLib->GetName());

            // CONFETTI START: Leroy Sikkes
            // Notify others that database changed.
            NotifyExceptSelf(eNotify_OnDataBaseUpdate);
            // CONFETTI END
        }
    }
}

//////////////////////////////////////////////////////////////////////////
QString CBaseLibraryDialog::GetItemFullName(const QModelIndex& index, QString const& sep)
{
    QString name = index.data().toString();
    QModelIndex hParent = index.parent();
    while (hParent.isValid())
    {
        hParent = hParent.parent();
        if (hParent.isValid())
        {
            name = hParent.data().toString() + sep + name;
        }
    }
    return name;
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryDialog::NotifyExceptSelf(EEditorNotifyEvent ev)
{
    GetIEditor()->NotifyExcept(ev, static_cast<IEditorNotifyListener*>(this));
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryDialog::SortItems(SortRecursionType recursionType)
{
    GetSourceDialogModel()->setSortRecursionType(recursionType);
    GetSourceModel()->sort(0);
}

//////////////////////////////////////////////////////////////////////////
bool CBaseLibraryDialog::IsExistItem(const QString& itemName) const
{
    for (int i = 0, iItemCount(m_pLibrary->GetItemCount()); i < iItemCount; ++i)
    {
        IDataBaseItem* pItem = m_pLibrary->GetItem(i);
        if (pItem == NULL)
        {
            continue;
        }
        if (pItem->GetName() == itemName)
        {
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
QString CBaseLibraryDialog::MakeValidName(const QString& candidateName) const
{
    if (!IsExistItem(candidateName))
    {
        return candidateName;
    }

    int nCounter = 0;
    const int nEnoughBigNumber = 1000000;
    do
    {
        QString counterBuffer = QString::number(nCounter);
        QString newName = candidateName + counterBuffer;
        if (!IsExistItem(newName))
        {
            return newName;
        }
    } while (nCounter++ < nEnoughBigNumber);

    assert(0 && "CBaseLibraryDialog::MakeValidName()");
    return candidateName;
}

#include <BaseLibraryDialog.moc>
