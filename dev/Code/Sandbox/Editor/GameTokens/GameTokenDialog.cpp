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
#include "GameTokenDialog.h"

#include "StringDlg.h"

#include "GameTokenManager.h"
#include "GameTokenLibrary.h"
#include "GameTokenItem.h"

#include "ViewManager.h"
#include "Clipboard.h"

#include "../Plugins/EditorUI_QT/Toolbar.h"

#include <QHBoxLayout>
#include <QMenu>
#include <GameTokens/ui_GameTokenDialog.h>

namespace
{
    class Vec3Validator
        : public QDoubleValidator
    {
    public:
        Vec3Validator(QObject* parent)
            : QDoubleValidator(parent) {}

        State validate(QString& text, int&) const override
        {
            QStringList parts = text.split(',');
            if (parts.count() > 3)
            {
                return QValidator::Invalid;
            }

            State state = QValidator::Acceptable;
            for (int i = 0; i < parts.count(); i++)
            {
                int posIgnore = 0;
                State partState = QDoubleValidator::validate(parts[i], posIgnore);
                if (partState == QValidator::Invalid)
                {
                    // if any part is entirely invalid, then the whole thing is invalid
                    return QValidator::Invalid;
                }
                else if (partState == QValidator::Intermediate)
                {
                    // if any part is intermediate, then the whole thing is intermediate at least
                    state = QValidator::Intermediate;
                }
            }

            return state;
        }
    };
}

const int TreeItemsColumnSize = 4;

enum TreeItemsColumn
{
    NAME_TREE_ITEMS_COLUMN = 0,
    VALUE_TREE_ITEMS_COLUMN,
    TYPE_TREE_ITEMS_COLUMN,
    DESCRIPTION_TREE_ITEMS_COLUMN,
};

static QString TreeItemsColumnToString(int column)
{
    switch (column)
    {
    case TreeItemsColumn::NAME_TREE_ITEMS_COLUMN:
        return "Name";

    case TreeItemsColumn::VALUE_TREE_ITEMS_COLUMN:
        return "Value";

    case TreeItemsColumn::TYPE_TREE_ITEMS_COLUMN:
        return "Type";

    case TreeItemsColumn::DESCRIPTION_TREE_ITEMS_COLUMN:
        return "Description";

    default:
        Q_ASSERT(false);
        return QString();
    }
}

class GameTokenTypeChangeComboBox
    : public QComboBox
{
public:
    GameTokenTypeChangeComboBox(QTreeView* m_treeWidget, const QStringList& options)
        : QComboBox(m_treeWidget)
        , m_treeWidget(m_treeWidget)
    {
        addItems(options);
        setStyleSheet("QComboBox::drop-down {border-width: 0px;} QComboBox::down-arrow {image: url(noimg); border-width: 0px;}");
    }

    void setCurrentTreeWidgetItem(const QModelIndex& index) { m_treeWidgetIndex = index; }
    QModelIndex treeWidgetItem() const { return m_treeWidgetIndex; }

public:
    void hidePopup()
    {
        if (m_treeWidgetIndex.isValid())
        {
            m_treeWidget->setIndexWidget(m_treeWidgetIndex, 0); // will call GameTokenTypeChangeComboBox destructor
        }

        QComboBox::hidePopup();
    }
private:
    QTreeView* m_treeWidget;
    QModelIndex m_treeWidgetIndex;
};

class GameTokenModel
    : public QAbstractListModel
    , public BaseLibraryDialogModel
{
public:
    GameTokenModel(QObject* parent = nullptr)
        : QAbstractListModel(parent)
        , m_library(nullptr)
    {
    }

    enum Role
    {
        LocalOnlyRole = CBaseLibraryDialog::BaseLibraryItemRole + 1
    };

    using QAbstractListModel::index;
    QModelIndex index(CBaseLibraryItem* item) const override
    {
        for (int i = 0; m_library != nullptr && i < m_library->GetItemCount(); ++i)
        {
            if (m_library->GetItem(i) == item)
            {
                return QAbstractListModel::index(i, 0);
            }
        }
        return QModelIndex();
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() || m_library == nullptr ? 0 : m_library->GetItemCount();
    }

    int columnCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 4;
    }

    Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        Qt::ItemFlags f = QAbstractListModel::flags(index);
        if (index.column() != NAME_TREE_ITEMS_COLUMN)
        {
            f |= Qt::ItemIsEditable;
        }

        return f;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        if (section >= columnCount() || orientation != Qt::Horizontal || role != Qt::DisplayRole)
        {
            return QAbstractListModel::headerData(section, orientation, role);
        }

        return TreeItemsColumnToString(section);
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        Q_ASSERT(index.row() < rowCount(index.parent()) && index.column() < columnCount(index.parent()) && index.model() == this);
        auto item = static_cast<CGameTokenItem*>(m_library->GetItem(index.row()));
        if (role == CBaseLibraryDialog::BaseLibraryItemRole)
        {
            return QVariant::fromValue(item);
        }
        else if (role == LocalOnlyRole)
        {
            return item->GetLocalOnly();
        }
        else if (role == Qt::FontRole)
        {
            QFont f;
            f.setBold(index.column() == NAME_TREE_ITEMS_COLUMN);
            return QVariant::fromValue(f);
        }
        else if (role == Qt::DisplayRole || role == Qt::EditRole)
        {
            switch (index.column())
            {
            case NAME_TREE_ITEMS_COLUMN:
                return item->GetName();
            case VALUE_TREE_ITEMS_COLUMN:
                return item->GetValueString();
            case TYPE_TREE_ITEMS_COLUMN:
                return item->GetTypeName();
            case DESCRIPTION_TREE_ITEMS_COLUMN:
                return item->GetDescription();
            }
        }

        return QVariant();
    }

    bool setData(const QModelIndex& index, const QVariant& data, int role = Qt::EditRole) override
    {
        Q_ASSERT(index.row() < rowCount(index.parent()) && index.column() < columnCount(index.parent()) && index.model() == this);
        auto item = static_cast<CGameTokenItem*>(m_library->GetItem(index.row()));
        if (role == LocalOnlyRole)
        {
            bool localOnly = data.toBool();
            if (item->GetLocalOnly() == localOnly)
            {
                return true;
            }

            item->SetLocalOnly(localOnly);
            return true;
        }

        if (role != Qt::EditRole)
        {
            return false;
        }

        switch (index.column())
        {
        case NAME_TREE_ITEMS_COLUMN:
            if (!data.canConvert<QString>())
            {
                return false;
            }
            if (item->GetName() == data.toString())
            {
                return true;
            }
            item->SetName(data.toString());
            item->Update();
            emit dataChanged(index, index);
            return true;
        case VALUE_TREE_ITEMS_COLUMN:
            if (item->GetValueString() == data.toString())
            {
                return true;
            }
            item->SetValueString(data.toString().toUtf8().data());
            item->Update();
            emit dataChanged(index, index);
            return true;
        case TYPE_TREE_ITEMS_COLUMN:
            if (item->GetTypeName() == data.toString())
            {
                return true;
            }
            item->SetTypeName(data.toString().toUtf8().data());
            item->Update();
            emit dataChanged(index.sibling(index.row(), VALUE_TREE_ITEMS_COLUMN), index);
            return true;
        case DESCRIPTION_TREE_ITEMS_COLUMN:
            if (item->GetDescription() == data.toString())
            {
                return true;
            }
            item->SetDescription(data.toString());
            item->Update();
            emit dataChanged(index, index);
            return true;
        }

        return false;
    }

    void setLibrary(CBaseLibrary* library) override
    {
        beginResetModel();
        m_library = library;
        endResetModel();
    }

    void setSortRecursionType(int recursionType) override
    {
    }

private:
    CBaseLibrary* m_library;
};

class GameTokenProxyModel
    : public AbstractGroupProxyModel
{
public:
    GameTokenProxyModel(QObject* parent = nullptr)
        : AbstractGroupProxyModel(parent)
    {
    }

    QStringList GroupForSourceIndex(const QModelIndex& sourceIndex) const override
    {
        auto item = sourceIndex.data(CBaseLibraryDialog::BaseLibraryItemRole).value<CGameTokenItem*>();
        if (item == nullptr)
        {
            return QStringList();
        }

        return QStringList(tr("Group: %1").arg(item->GetGroupName()));
    }

    QVariant data(const QModelIndex& index, int role) const override
    {
        if ((role == Qt::DisplayRole) && (index.column() == NAME_TREE_ITEMS_COLUMN))
        {
            CGameTokenItem* item = AbstractGroupProxyModel::data(index, CBaseLibraryDialog::BaseLibraryItemRole).value<CGameTokenItem*>();
            if (item)
            {
                return item->GetShortName();
            }
        }
        return AbstractGroupProxyModel::data(index, role);
    }

    bool IsGroupIndex(const QModelIndex& sourceIndex) const override
    {
        auto item = sourceIndex.data(CBaseLibraryDialog::BaseLibraryItemRole).value<CGameTokenItem*>();
        return (item == nullptr);
    }
};


/*
//////////////////////////////////////////////////////////////////////////
class CGameTokenUIDefinition
{
public:
    CVarBlockPtr m_vars;

    CSmartVariableEnum<int> tokenType;
    _smart_ptr<CVariableBase> pTokenValue;

    //////////////////////////////////////////////////////////////////////////
    CVarBlock* CreateVars()
    {
        m_vars = new CVarBlock;

        tokenType.AddEnumItem( "Bool",eFDT_Bool );
        tokenType.AddEnumItem( "Int",eFDT_Int );
        tokenType.AddEnumItem( "Float",eFDT_Float );
        tokenType.AddEnumItem( "String",eFDT_String );
        tokenType.AddEnumItem( "Vec3",eFDT_Vec3 );

        // Create UI vars, using GameTokenParams TypeInfo.
        CVariableArray* pMainTable = AddGroup("Game Token");

        pTokenValue = new CVariable<int>;
        AddVariable( *pMainTable,tokenType,"Type" );
        AddVariable( *pMainTable,*pTokenValue,"Value" );

        return m_vars;
    }

    //////////////////////////////////////////////////////////////////////////
    void WriteToUI( CGameTokenItem *pGameTokens )
    {
    }

    //////////////////////////////////////////////////////////////////////////
    void ReadFromUI( CGameTokenItem *pGameTokens )
    {
        int tp = tokenType;
        switch (tp) {
        case eFDT_Float:
            {
                m_vars->Clear();
                // Create UI vars, using GameTokenParams TypeInfo.
                CVariableArray* pMainTable = AddGroup("Game Token");

                pTokenValue = new CVariable<int>;
                AddVariable( *pMainTable,tokenType,"Type" );
                AddVariable( *pMainTable,*pTokenValue,"Value" );
            }
            break;
        case eFDT_Bool:
            {
                m_vars->Clear();
                // Create UI vars, using GameTokenParams TypeInfo.
                CVariableArray* pMainTable = AddGroup("Game Token");

                pTokenValue = new CVariable<bool>;
                AddVariable( *pMainTable,tokenType,"Type" );
                AddVariable( *pMainTable,*pTokenValue,"Value" );
            }
            break;
        }

        // Update GameTokens.
        pGameTokens->Update();
    }

private:

    //////////////////////////////////////////////////////////////////////////
    CVariableArray* AddGroup( const char *sName )
    {
        CVariableArray* pArray = new CVariableArray;
        pArray->AddRef();
        pArray->SetFlags(IVariable::UI_BOLD);
        if (sName)
            pArray->SetName(sName);
        m_vars->AddVariable(pArray);
        return pArray;
    }

    //////////////////////////////////////////////////////////////////////////
    void AddVariable( CVariableBase &varArray,CVariableBase &var,const char *varName,unsigned char dataType=IVariable::DT_SIMPLE )
    {
        if (varName)
            var.SetName(varName);
        var.SetDataType(dataType);
        varArray.AddChildVar(&var);
    }
    //////////////////////////////////////////////////////////////////////////
    void AddVariable( CVarBlock *vars,CVariableBase &var,const char *varName,unsigned char dataType=IVariable::DT_SIMPLE )
    {
        if (varName)
            var.SetName(varName);
        var.SetDataType(dataType);
        vars->AddVariable(&var);
    }
};

static CGameTokenUIDefinition gGameTokenUI;
//////////////////////////////////////////////////////////////////////////
*/


//////////////////////////////////////////////////////////////////////////
// CGameTokenDialog implementation.
//////////////////////////////////////////////////////////////////////////
CGameTokenDialog::CGameTokenDialog(QWidget* pParent)
    : m_bSkipUpdateItems(false)
    , m_bEditingItems(false)
    , CBaseLibraryDialog(IDD_DB_ENTITY, pParent)
    , m_model(new GameTokenModel(this))
    , m_proxyModel(new GameTokenProxyModel(this))
    , ui(new Ui::GameTokenDialog)
    , m_floatValidator(new QDoubleValidator(this))
    , m_intValidator(new QIntValidator(this))
    , m_boolValidator(new QRegExpValidator(QRegExp("true|false", Qt::CaseInsensitive), this))
    , m_vec3Validator(new Vec3Validator(this))
{
    qRegisterMetaType<CGameTokenItem*>();

    m_pGameTokenManager = GetIEditor()->GetGameTokenManager();
    m_pItemManager = m_pGameTokenManager;

    auto widget = new QWidget();
    ui->setupUi(widget);
    setCentralWidget(widget);

    m_proxyModel->setSourceModel(m_model);
    ui->treeWidgetItems->setModel(m_proxyModel);

    ui->treeWidgetItems->setSelectionMode(QTreeView::SingleSelection);
    ui->treeWidgetItems->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    ui->treeWidgetItems->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->treeWidgetItems->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->treeWidgetItems->sortByColumn(NAME_TREE_ITEMS_COLUMN);

    InitToolbar();
    ReloadLibs();

    // Create context menu and shortcut keys for tree control
    m_treeContextMenu = new QMenu(this);

    QAction* addAction = m_treeContextMenu->addAction(tr("actionItemAdd"));
    addAction->setShortcut(Qt::Key_Insert);
    connect(addAction, &QAction::triggered, this, &CGameTokenDialog::OnAddItem);

    QAction* cloneAction = m_treeContextMenu->addAction(tr("actionItemClone"));
    cloneAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_D));
    connect(cloneAction, &QAction::triggered, this, &CGameTokenDialog::OnClone);

    QAction* deleteAction = m_treeContextMenu->addAction(tr("actionItemRemove"));
    deleteAction->setShortcut(QKeySequence(Qt::Key_Delete));
    connect(deleteAction, &QAction::triggered, this, &CGameTokenDialog::OnRemoveItem);

    ui->treeWidgetItems->addActions(m_treeContextMenu->actions());

    connect(ui->GAME_TOKEN_ADD_LIBRARY, &QLabel::linkActivated, this, &CGameTokenDialog::OnAddLibrary);
    connect(ui->GAME_TOKEN_REMOVE_LIBRARY, &QLabel::linkActivated, this, &CGameTokenDialog::OnRemoveLibrary);
    connect(ui->GAME_TOKEN_LOAD_LIBRARY, &QLabel::linkActivated, this, &CGameTokenDialog::OnLoadLibrary);

    connect(ui->GAME_TOKEN_ADD_ITEM, &QLabel::linkActivated, this, &CGameTokenDialog::OnAddItem);
    connect(ui->GAME_TOKEN_CLONE_LIBRARY_ITEM, &QLabel::linkActivated, this, &CGameTokenDialog::OnClone);
    connect(ui->GAME_TOKEN_REMOVE_ITEM, &QLabel::linkActivated, this, &CGameTokenDialog::OnRemoveItem);
    connect(ui->GAME_TOKEN_RENAME_ITEM, &QLabel::linkActivated, this, &CGameTokenDialog::OnRenameItem);

    void(QComboBox::* currentIndexChanged)(int) = &QComboBox::currentIndexChanged;
    connect(ui->GAME_TOKEN_TOKEN_TYPE_COMBO, currentIndexChanged, this, &CGameTokenDialog::OnTokenTypeChange);
    connect(ui->GAME_TOKEN_VALUE_LINEEDIT, &QLineEdit::textChanged, this, &CGameTokenDialog::OnTokenValueChange);
    connect(ui->GAME_TOKEN_LOCAL_ONLY, &QCheckBox::stateChanged, this, &CGameTokenDialog::OnTokenLocalOnlyChange);
    connect(ui->GAME_TOKEN_DESCRIPTION_TEXTEDIT, &QTextEdit::textChanged, this, &CGameTokenDialog::OnTokenInfoChange);

    // Need to custom watch for the focus out events; that's when we'll update the value the user sees
    ui->GAME_TOKEN_VALUE_LINEEDIT->installEventFilter(this);
    ui->GAME_TOKEN_DESCRIPTION_TEXTEDIT->installEventFilter(this);

    connect(ui->treeWidgetItems->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CGameTokenDialog::OnReportSelChange);

    // Make sure that we force the actions to update when the selection changes! Otherwise, the user can change stuff when nothing
    // is selected, which makes no sense
    connect(ui->treeWidgetItems->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CGameTokenDialog::OnUpdateActions);

    connect(ui->treeWidgetItems, &GameTokenTreeView::doubleClicked, this, &CGameTokenDialog::OnReportDblClick);
    connect(ui->treeWidgetItems->model(), &QAbstractItemModel::dataChanged, this, &CGameTokenDialog::OnTreeWidgetItemChanged);

    connect(this, &CGameTokenDialog::customContextMenuRequested, this, &CGameTokenDialog::OnCustomContextMenuRequested);
    connect(ui->treeWidgetItems, &GameTokenTreeView::customContextMenuRequested, this, &CGameTokenDialog::OnCustomContextMenuRequested);
}

CGameTokenDialog::~CGameTokenDialog()
{
}

//////////////////////////////////////////////////////////////////////////
// Create the toolbar
void CGameTokenDialog::InitToolbar(/* UINT nToolbarResID*/)
{
    InitLibraryToolbar();
    InitItemToolbar();
}

void CGameTokenDialog::InitItemToolbar()
{
    connect(m_pItemToolBar->getAction("actionItemAdd"), &QAction::triggered, this, &CGameTokenDialog::OnAddItem);
}


//////////////////////////////////////////////////////////////////////////
void CGameTokenDialog::ReloadItems()
{
    m_model->setLibrary(m_pLibrary);
    ui->treeWidgetItems->setCurrentIndex(m_proxyModel->mapFromSource(m_model->index(m_pCurrentItem)));

    ui->treeWidgetItems->expandAll();
    ui->treeWidgetItems->resizeColumnToContents(0);

    ui->treeWidgetItems->scrollTo(ui->treeWidgetItems->currentIndex());

    m_bIgnoreSelectionChange = false;
}

//////////////////////////////////////////////////////////////////////////
void CGameTokenDialog::OnAddItem()
{
    if (!m_pLibrary)
    {
        return;
    }

    StringGroupDlg dlg("New GameToken Name", this);
    dlg.SetGroup(m_selectedGroup);

    if (dlg.exec() == QDialog::Accepted)
    {
        QString group = dlg.GetGroup();
        QString name = dlg.GetString();
        QString fullName = m_pItemManager->MakeFullItemName(m_pLibrary, group, name);

        if (m_pItemManager->FindItemByName(fullName))
        {
            Warning("Item with name %s already exist", fullName.toUtf8().data());
            return;
        }

        CGameTokenItem* pItem = (CGameTokenItem*)m_pItemManager->CreateItem(m_pLibrary);
        // Make prototype name.
        SetItemName(pItem, dlg.GetGroup(), dlg.GetString());
        pItem->Update();

        ReloadItems();
        SelectItem(pItem);
    }
}


//////////////////////////////////////////////////////////////////////////
void CGameTokenDialog::SelectItem(CBaseLibraryItem* item, bool bForceReload)
{
    bool bChanged = item != m_pCurrentItem || bForceReload;
    CBaseLibraryDialog::SelectItem(item, bForceReload);

    if (!bChanged)
    {
        return;
    }

    m_pGameTokenManager->SetSelectedItem(item); // alexl: tell manager which item is selected

    if (!item)
    {
        //m_propsCtrl.EnableWindow(FALSE);
        return;
    }

    CGameTokenItem* pGameToken = GetSelectedGameToken();
    if (!pGameToken)
    {
        return;
    }

    m_bSkipUpdateItems = true;
    ui->GAME_TOKEN_SELECTED_TOKEN->setText(pGameToken->GetFullName());
    ui->GAME_TOKEN_VALUE_LINEEDIT->setText(pGameToken->GetValueString());
    int indexType = ui->GAME_TOKEN_TOKEN_TYPE_COMBO->findText(pGameToken->GetTypeName());
    ui->GAME_TOKEN_TOKEN_TYPE_COMBO->setCurrentIndex(indexType);
    ui->GAME_TOKEN_LOCAL_ONLY->setCheckState(pGameToken->GetLocalOnly() ? Qt::Checked : Qt::Unchecked);

    QString text = pGameToken->GetDescription();
    text.replace("\\n", "\r\n");
    ui->GAME_TOKEN_DESCRIPTION_TEXTEDIT->setPlainText(text);
    m_bSkipUpdateItems = false;

    // make sure we also update the UI on/off states and the validators for the value field
    OnUpdateActions();

    /*


    m_propsCtrl.EnableWindow(TRUE);
    m_propsCtrl.EnableUpdateCallback(false);

    CGameTokenItem *pGameToken = GetSelectedGameToken();
    if (pGameToken)
    {
        // Update UI with new item content.
        gGameTokenUI.WriteToUI( pGameToken );
    }
    m_propsCtrl.EnableUpdateCallback(true);
    */
}

const char * CGameTokenDialog::GetLibraryAssetTypeName() const
{
    return "Game Tokens Library";
}

//////////////////////////////////////////////////////////////////////////
CGameTokenItem* CGameTokenDialog::GetSelectedGameToken()
{
    CBaseLibraryItem* pItem = m_pCurrentItem;
    return (CGameTokenItem*)pItem;
}

//////////////////////////////////////////////////////////////////////////
void CGameTokenDialog::OnTreeRClick(const QModelIndex& index)
{
    if (!index.isValid())
    {
        return;
    }

    /* KDAB: No context menu in MFC version
    m_treeContextMenu->exec(QCursor::pos());
    */
}

//////////////////////////////////////////////////////////////////////////
void CGameTokenDialog::OnCopy()
{
    CGameTokenItem* pItem = GetSelectedGameToken();
    if (pItem)
    {
        XmlNodeRef node = XmlHelpers::CreateXmlNode("GameToken");
        CBaseLibraryItem::SerializeContext ctx(node, false);
        ctx.bCopyPaste = true;

        CClipboard clipboard(this);
        pItem->Serialize(ctx);
        clipboard.Put(node);
    }
}

//////////////////////////////////////////////////////////////////////////
void CGameTokenDialog::OnPaste()
{
    if (!m_pLibrary)
    {
        return;
    }

    CGameTokenItem* pItem = GetSelectedGameToken();
    if (!pItem)
    {
        return;
    }

    CClipboard clipboard(this);
    if (clipboard.IsEmpty())
    {
        return;
    }

    XmlNodeRef node = clipboard.Get();
    if (!node)
    {
        return;
    }

    if (strcmp(node->getTag(), "GameToken") == 0)
    {
        CBaseLibraryItem* pItem = (CBaseLibraryItem*)m_pGameTokenManager->CreateItem(m_pLibrary);
        if (pItem)
        {
            CBaseLibraryItem::SerializeContext serCtx(node, true);
            serCtx.bCopyPaste = true;
            serCtx.bUniqName = true; // this ensures that a new GameToken is created because otherwise the old one would be renamed
            pItem->Serialize(serCtx);

            ReloadItems();

            // always select the item after the reload; otherwise, the item may not be properly set up in the AbstractGroupProxyModel.
            SelectItem(pItem);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CGameTokenDialog::OnCustomContextMenuRequested(const QPoint& point)
{
    auto item = ui->treeWidgetItems->indexAt(point);
    if (!item.isValid())
    {
        return;
    }

    OnTreeRClick(item);
}

//////////////////////////////////////////////////////////////////////////
void CGameTokenDialog::OnReportDblClick(const QModelIndex& index)
{
    if (!index.parent().isValid())
    {
        return;
    }

    if (index.column() == TYPE_TREE_ITEMS_COLUMN)
    {
        SetWidgetItemEditable(index); // we display a combobox
    }
}

//////////////////////////////////////////////////////////////////////////
void CGameTokenDialog::OnReportSelChange()
{
    m_cpoSelectedLibraryItems.clear();

    auto selectedItems = ui->treeWidgetItems->selectionModel()->selectedRows();

    if (selectedItems.isEmpty())
    {
        return;
    }

    Q_ASSERT(selectedItems.size() == 1);
    if (!selectedItems.first().parent().isValid())
    {
        return; // top level selected
    }

    foreach(const auto & item, selectedItems)
    {
        auto tokenItem = item.data(CBaseLibraryDialog::BaseLibraryItemRole).value<CGameTokenItem*>();
        if (tokenItem)
        {
            m_cpoSelectedLibraryItems.insert(tokenItem);
            SelectItem(tokenItem);
        }
    }
}



void CGameTokenDialog::OnTreeWidgetItemChanged(const QModelIndex& index)
{
    if (m_bEditingItems)
    {
        return;
    }

    auto item = index.data(CBaseLibraryDialog::BaseLibraryItemRole).value<CGameTokenItem*>();
    if (!item)
    {
        return;
    }

    // we forward the change to the left panel to handle the logic
    ui->GAME_TOKEN_VALUE_LINEEDIT->setText(item->GetValueString());
    ui->GAME_TOKEN_DESCRIPTION_TEXTEDIT->setPlainText(item->GetDescription());
}

QTreeView* CGameTokenDialog::GetTreeCtrl()
{
    return ui->treeWidgetItems;
}

AbstractGroupProxyModel* CGameTokenDialog::GetProxyModel()
{
    return m_proxyModel;
}

//////////////////////////////////////////////////////////////////////////
QAbstractItemModel* CGameTokenDialog::GetSourceModel()
{
    return m_model;
}

//////////////////////////////////////////////////////////////////////////
BaseLibraryDialogModel* CGameTokenDialog::GetSourceDialogModel()
{
    return m_model;
}



inline void SafeEnable(QWidget* widget, bool enable)
{
    if (widget != nullptr)
    {
        widget->setEnabled(enable);
    }
}

void CGameTokenDialog::OnUpdateActions()
{
    CBaseLibraryDialog::OnUpdateActions();

    auto selectedIndex = GetCurrentSelectedTreeWidgetItemRecord();
    const bool enable = selectedIndex.isValid();

    SafeEnable(ui->GAME_TOKEN_TOKEN_TYPE_COMBO, enable);
    SafeEnable(ui->GAME_TOKEN_VALUE_LINEEDIT, enable);
    SafeEnable(ui->GAME_TOKEN_LOCAL_ONLY, enable);
    SafeEnable(ui->GAME_TOKEN_DESCRIPTION_TEXTEDIT, enable);

    if (enable)
    {
        auto gameTokenItem = selectedIndex.data(CBaseLibraryDialog::BaseLibraryItemRole).value<CGameTokenItem*>();
        InstallValidator(gameTokenItem->GetTypeName());
    }
}

//////////////////////////////////////////////////////////////////////////
void CGameTokenDialog::OnTokenTypeChange()
{
    // TODO: Add your control notification handler code here
    if (m_bSkipUpdateItems)
    {
        return;
    }

    if (ui->GAME_TOKEN_TOKEN_TYPE_COMBO->currentIndex() == -1)
    {
        return;
    }

    const QString str = ui->GAME_TOKEN_TOKEN_TYPE_COMBO->currentText();

    auto index = GetCurrentSelectedTreeWidgetItemRecord();
    if (index.isValid())
    {
        ui->treeWidgetItems->model()->setData(index.sibling(index.row(), TYPE_TREE_ITEMS_COLUMN), str);

        InstallValidator(str);
    }
}

//////////////////////////////////////////////////////////////////////////
void CGameTokenDialog::OnTokenValueChange()
{
    if (m_bSkipUpdateItems)
    {
        return;
    }

    const QString str = ui->GAME_TOKEN_VALUE_LINEEDIT->text();
    auto index = GetCurrentSelectedTreeWidgetItemRecord();
    if (index.isValid())
    {
        bool validInput = ui->GAME_TOKEN_VALUE_LINEEDIT->hasAcceptableInput();

        // don't want to change the input field while they're in the middle of editing the field
        m_bEditingItems = true;

        if (validInput)
        {
            ui->treeWidgetItems->model()->setData(index.sibling(index.row(), VALUE_TREE_ITEMS_COLUMN), str);
        }

        m_bEditingItems = false;
    }
}

//////////////////////////////////////////////////////////////////////////
void CGameTokenDialog::OnTokenInfoChange()
{
    if (m_bSkipUpdateItems)
    {
        return;
    }

    // flag that the user is editing, so that we don't refresh while they're still typing
    m_bEditingItems = true;

    // The following will remove newline characters, which will change the text.
    // But if we update the text while the user is still typing, the cursor will
    // move somewhere they aren't expecting.
    // So instead, we handle it on when the GAME_TOKEN_DESCRIPTION_TEXTEDIT widget loses
    // focus.
    QString str = ui->GAME_TOKEN_DESCRIPTION_TEXTEDIT->toPlainText();
    str.replace("\n", " ");
    auto index = GetCurrentSelectedTreeWidgetItemRecord();
    if (index.isValid())
    {
        ui->treeWidgetItems->model()->setData(index.sibling(index.row(), DESCRIPTION_TREE_ITEMS_COLUMN), str);
    }

    // unflag this
    m_bEditingItems = false;
}

//////////////////////////////////////////////////////////////////////////
void CGameTokenDialog::OnTokenLocalOnlyChange()
{
    if (m_bSkipUpdateItems)
    {
        return;
    }

    bool localOnly = ui->GAME_TOKEN_LOCAL_ONLY->isChecked();
    const QModelIndex index = GetCurrentSelectedTreeWidgetItemRecord();
    if (index.isValid())
    {
        ui->treeWidgetItems->model()->setData(index, localOnly, GameTokenModel::LocalOnlyRole);
    }
}

bool CGameTokenDialog::eventFilter(QObject* obj, QEvent* event)
{
    const bool continueProcessingEvent = false;

    // We're using validators, and if you use validators, you don't get the editingFinished signal if the validator doesn't detect acceptable input
    // So we need to specially watch for the focus out event.
    // There also isn't one for the QTextEdit widget, so we want to watch that too.
    if (((obj == ui->GAME_TOKEN_DESCRIPTION_TEXTEDIT) || (obj == ui->GAME_TOKEN_VALUE_LINEEDIT)) && (event->type() == QEvent::FocusOut))
    {
        const QModelIndex index = GetCurrentSelectedTreeWidgetItemRecord();
        if (index.isValid())
        {
            OnTreeWidgetItemChanged(index);
        }
    }

    return continueProcessingEvent;
}

void CGameTokenDialog::SetWidgetItemEditable(const QModelIndex& index)
{
    auto gameTokenTypeChangeComboBox = InitializeTypeChangeComboBox();
    gameTokenTypeChangeComboBox->setCurrentTreeWidgetItem(index);
    gameTokenTypeChangeComboBox->setCurrentIndex(ui->GAME_TOKEN_TOKEN_TYPE_COMBO->currentIndex());

    ui->treeWidgetItems->setIndexWidget(index, gameTokenTypeChangeComboBox);
    gameTokenTypeChangeComboBox->showPopup();
}

void CGameTokenDialog::InstallValidator(const QString& typeName)
{
    if (typeName.compare("Bool", Qt::CaseInsensitive) == 0)
    {
        ui->GAME_TOKEN_VALUE_LINEEDIT->setValidator(m_boolValidator);
    }
    else if (typeName.compare("Int", Qt::CaseInsensitive) == 0)
    {
        ui->GAME_TOKEN_VALUE_LINEEDIT->setValidator(m_intValidator);
    }
    else if (typeName.compare("Float", Qt::CaseInsensitive) == 0)
    {
        ui->GAME_TOKEN_VALUE_LINEEDIT->setValidator(m_floatValidator);
    }
    else if (typeName.compare("Vec3", Qt::CaseInsensitive) == 0)
    {
        ui->GAME_TOKEN_VALUE_LINEEDIT->setValidator(m_vec3Validator);
    }
    else
    {
        ui->GAME_TOKEN_VALUE_LINEEDIT->setValidator(nullptr);
    }
}

GameTokenTypeChangeComboBox* CGameTokenDialog::InitializeTypeChangeComboBox()
{
    QStringList options;
    for (int i = 0; i < ui->GAME_TOKEN_TOKEN_TYPE_COMBO->count(); ++i)
    {
        options << ui->GAME_TOKEN_TOKEN_TYPE_COMBO->itemText(i);
    }

    GameTokenTypeChangeComboBox* gameTokenTypeChangeComboBox = new GameTokenTypeChangeComboBox(ui->treeWidgetItems, options);

    void(QComboBox::* activated)(int) = &QComboBox::activated;
    connect(gameTokenTypeChangeComboBox, activated, this, [this, gameTokenTypeChangeComboBox]()
    {
        auto index = gameTokenTypeChangeComboBox->treeWidgetItem();
        if (index.isValid())
        {
            if (this->GetCurrentSelectedTreeWidgetItemRecord().isValid())
            {
                this->ui->GAME_TOKEN_TOKEN_TYPE_COMBO->setCurrentIndex(gameTokenTypeChangeComboBox->currentIndex());
                OnUpdateActions();
            }
        }
    });

    return gameTokenTypeChangeComboBox;
}

QModelIndex CGameTokenDialog::GetCurrentSelectedTreeWidgetItemRecord()
{
    auto selectedItems = ui->treeWidgetItems->selectionModel()->selectedRows();
    if (selectedItems.isEmpty())
    {
        return QModelIndex();
    }

    Q_ASSERT(selectedItems.size() == 1);

    if (selectedItems.first().data(CBaseLibraryDialog::BaseLibraryItemRole).value<CGameTokenItem*>())
    {
        return selectedItems.first();
    }

    return QModelIndex();
}


#include <GameTokens/GameTokenDialog.moc>
