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
#include "VegetationPanel.h"
#include "VegetationTool.h"
#include "Terrain/SurfaceType.h"
#include "Terrain/TerrainManager.h"
#include "Terrain/Layer.h"
#include "VegetationMap.h"
#include "VegetationObject.h"
#include "StringDlg.h"
#include "PanelPreview.h"
#include "Material/MaterialManager.h"
#include "GameEngine.h"
#include "Clipboard.h"
#include "MainWindow.h"

#include <map>
#include <vector>
#include <numeric>

#include <QApplication>
#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QFileInfo>
#include <QMenu>
#include <QMetaType>
#include <QMimeData>
#include <QAbstractButton>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>

#include <ui_VegetationPanel.h>

#include "QtUtil.h"

#define ID_PANEL_VEG_RANDOM_ROTATE 20000
#define ID_PANEL_VEG_CLEAR_ROTATE 20001

#define VEGETATION_OBJECT_MODEL_MIME_TYPE QStringLiteral("application/x-lumberyard-cvegetationobject-addresses")

enum Role
{
    CategoryNameRole = Qt::UserRole,
    VegetationObjectRole
};

class VegetationCategoryTreeModel
    : public QAbstractItemModel
{
private:
    class Category
    {
    public:
        Category(const QString& name, std::vector<CVegetationObject*>&& objects)
            : m_name(name)
            , m_objects(objects)
        {
        }

        Category(const QString& name, const std::vector<CVegetationObject*>& objects)
            : m_name(name)
            , m_objects(objects)
        {
        }

        Category(const QString& name)
            : m_name(name)
        {
        }

        QString m_name;
        std::vector<CVegetationObject*> m_objects;
    };

    using CategoryMap = std::map<QString, Category*>;

public:
    VegetationCategoryTreeModel(CVegetationMap& vegetationMap, QObject* parent = nullptr)
        : QAbstractItemModel(parent)
        , m_vegetationMap(vegetationMap)
    { }

    ~VegetationCategoryTreeModel()
    {
        Clear();
    }

    void Reload();

    QModelIndex AddCategory(const QString& category, const std::vector<CVegetationObject*>& objects = std::vector<CVegetationObject*>());
    void RemoveCategory(const QString& category);
    void RenameCategory(const QString& previous, const QString& updated);

    QModelIndex AddObject(CVegetationObject* object);
    void RemoveObject(CVegetationObject* object);

    void UpdateData(CVegetationObject* object, const QVector<int>& roles = { Qt::DisplayRole, Qt::CheckStateRole });

    QModelIndex ObjectIndex(const CVegetationObject* object) const;

    void Clear()
    {
        for (auto t : m_categories)
        {
            delete t.second;
        }

        m_categories.clear();
    }

    QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override { return 1; }

    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    Qt::DropActions supportedDragActions() const override { return Qt::MoveAction; }
    Qt::DropActions supportedDropActions() const override { return Qt::MoveAction; }
    QStringList mimeTypes() const override
    {
        return {
                   VEGETATION_OBJECT_MODEL_MIME_TYPE
        };
    }
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

    bool removeRows(int row, int count, const QModelIndex& parent = {}) override;

private:
    CVegetationMap& m_vegetationMap;

    CategoryMap m_categories;
};

void VegetationCategoryTreeModel::Reload()
{
    emit beginResetModel();
    Clear();

    int count = m_vegetationMap.GetObjectCount();
    for (int i = 0; i < count; ++i)
    {
        CVegetationObject* object = m_vegetationMap.GetObject(i);
        QString category(object->GetCategory());
        auto search = m_categories.find(category);
        if (search == m_categories.end())
        {
            m_categories[category] = new Category(category);
        }
        m_categories[category]->m_objects.push_back(object);
    }
    emit endResetModel();
}

QModelIndex VegetationCategoryTreeModel::AddCategory(const QString& category, const std::vector<CVegetationObject*>& objects /* = {} */)
{
    if (m_categories.find(category) != m_categories.end())
    {
        return QModelIndex();
    }

    int row = std::distance(m_categories.begin(), m_categories.lower_bound(category));
    emit beginInsertRows({}, row, row);
    m_categories[category] = new Category(category, objects);
    emit endInsertRows();
    return createIndex(row, 0, nullptr);
}

void VegetationCategoryTreeModel::RemoveCategory(const QString& category)
{
    auto entry = m_categories.find(category);
    if (entry == m_categories.end())
    {
        return;
    }

    int row = std::distance(m_categories.begin(), entry);
    emit beginRemoveRows({}, row, row);
    delete entry->second;
    m_categories.erase(entry);
    emit endRemoveRows();
}

void VegetationCategoryTreeModel::RenameCategory(const QString& previous, const QString& updated)
{
    auto entry = m_categories.find(previous);
    if (entry == m_categories.end())
    {
        return;
    }

    // check if the new category name is in there already
    assert(m_categories.find(updated) == m_categories.end());

    int previousRow = std::distance(m_categories.begin(), entry);
    int updatedRow = std::distance(m_categories.begin(), m_categories.lower_bound(updated));

    int rowDelta = updatedRow - previousRow;
    if (rowDelta == 0 || rowDelta == 1)
    {
        m_categories[updated] = entry->second;
        entry->second->m_name = updated;
        m_categories.erase(entry);

        QModelIndex changedRowIndex = index(previousRow, 0);
        dataChanged(changedRowIndex, changedRowIndex);
    }
    else
    {
        beginMoveRows({}, previousRow, previousRow, {}, updatedRow);

        m_categories[updated] = entry->second;
        entry->second->m_name = updated;
        m_categories.erase(entry);

        endMoveRows();
    }
}

QModelIndex VegetationCategoryTreeModel::AddObject(CVegetationObject* object)
{
    QString category = object->GetCategory();
    AddCategory(category);
    auto entry = m_categories.find(category);

    auto& objects = entry->second->m_objects;
    if (std::find(objects.cbegin(), objects.cend(), object) == objects.cend())
    {
        int parentRow = std::distance(m_categories.begin(), entry);
        int row = objects.size();
        emit beginInsertRows(createIndex(parentRow, 0, nullptr), row, row);
        objects.push_back(object);
        emit endInsertRows();

        return createIndex(row, 0, entry->second);
    }
    return {};
}

void VegetationCategoryTreeModel::RemoveObject(CVegetationObject* object)
{
    QString category = object->GetCategory();
    auto entry = m_categories.find(category);
    if (entry == m_categories.end())
    {
        return;
    }

    auto& objects = entry->second->m_objects;

    auto position = std::find(objects.begin(), objects.end(), object);
    if (position != objects.end())
    {
        int parentRow = std::distance(m_categories.begin(), entry);
        int row = std::distance(objects.begin(), position);
        emit beginRemoveRows(index(parentRow, 0), row, row);
        objects.erase(position);
        emit endRemoveRows();
    }

    // clean up any empty categories
    if (objects.empty())
    {
        RemoveCategory(category);
    }
}

void VegetationCategoryTreeModel::UpdateData(CVegetationObject* object, const QVector<int>& roles /* = QVector<int>() */)
{
    QModelIndex i = object != nullptr ? ObjectIndex(object) : QModelIndex();
    emit dataChanged(i, i, { Qt::DisplayRole, Qt::CheckStateRole, Qt::DecorationRole });
}

QModelIndex VegetationCategoryTreeModel::ObjectIndex(const CVegetationObject* object) const
{
    auto entry = m_categories.find(object->GetCategory());
    if (entry == m_categories.end())
    {
        return {};
    }

    auto objects = entry->second->m_objects;
    auto position = std::find(objects.cbegin(), objects.cend(), object);
    if (position != objects.cend())
    {
        int parentRow = std::distance(m_categories.begin(), entry);
        return index(std::distance(objects.cbegin(), position), 0, createIndex(parentRow, 0, const_cast<Category*>(entry->second)));
    }
    return {};
}

QModelIndex VegetationCategoryTreeModel::index(int row, int column, const QModelIndex& parent /* = {} */) const
{
    if (parent.model() && parent.model() != this)
    {
        return {};
    }

    if (!parent.isValid() && row >= 0 && row < m_categories.size() && column == 0)
    {
        //auto entry = m_categories.begin();
        //std::advance( entry, row );
        return createIndex(row, column, nullptr);
    }
    else if (parent.isValid() && parent.row() >= 0 && parent.row() < m_categories.size() && column == 0)
    {
        auto entry = m_categories.begin();
        std::advance(entry, parent.row());

        if (row >= 0 && row < entry->second->m_objects.size())
        {
            return createIndex(row, column, const_cast<Category*>(entry->second));
        }
    }

    return {};
}

QModelIndex VegetationCategoryTreeModel::parent(const QModelIndex& index) const
{
    if (index.isValid() && index.model() == this && index.internalPointer() != nullptr)
    {
        auto entry = reinterpret_cast<Category*>(index.internalPointer());
        auto position = m_categories.find(entry->m_name);
        return createIndex(std::distance(m_categories.cbegin(), position), 0, nullptr);
    }
    return {};
}

int VegetationCategoryTreeModel::rowCount(const QModelIndex& parent /* = {} */) const
{
    if (parent.model() && parent.model() != this)
    {
        return 0;
    }

    if (!parent.isValid())
    {
        return m_categories.size();
    }
    else if (parent.internalPointer() == nullptr)
    {
        auto entry = m_categories.begin();
        std::advance(entry, parent.row());
        return entry->second->m_objects.size();
    }
    return 0;
}

QVariant VegetationCategoryTreeModel::data(const QModelIndex& index, int role /* = Qt::DisplayRole */) const
{
    if (index.isValid() && index.model() == this)
    {
        auto entry = reinterpret_cast<Category*>(index.internalPointer());

        if (entry == nullptr)
        {
            auto category = m_categories.cbegin();
            std::advance(category, index.row());
            switch (role)
            {
            case Qt::DisplayRole:

                return category->first;
            case Qt::FontRole:
            {
                QFont font;
                font.setBold(true);
                return font;
            }
            case Qt::CheckStateRole:
            {
                auto& objects = category->second->m_objects;
                size_t visible = std::accumulate(objects.cbegin(), objects.cend(), 0, [](size_t a, const CVegetationObject* o) { return a + (o->IsHidden() ? 0 : 1); });
                size_t size = objects.size();
                if (size > 0 && visible == size)
                {
                    return Qt::Checked;
                }
                else if (visible > 0)
                {
                    return Qt::PartiallyChecked;
                }
                return Qt::Unchecked;
            }
            case Role::CategoryNameRole:
                return category->first;
            case Role::VegetationObjectRole:
                return QVariant::fromValue<CVegetationObject*>(nullptr);
            default:
                return {};
            }
        }
        else
        {
            switch (role)
            {
            case Qt::DisplayRole:
            {
                auto object = entry->m_objects[index.row()];
                QFileInfo info(object->GetFileName());
                return QString("%1 (%2)").arg(info.baseName()).arg(object->GetNumInstances());
            }
            case Qt::CheckStateRole:
                return entry->m_objects[index.row()]->IsHidden() ? Qt::Unchecked : Qt::Checked;
            case Role::CategoryNameRole:
                return entry->m_name;
            case Role::VegetationObjectRole:
                return QVariant::fromValue(entry->m_objects[index.row()]);
            default:
                return {};
            }
        }
    }
    return {};
}

Qt::ItemFlags VegetationCategoryTreeModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);

    if (index.isValid() && index.model() == this)
    {
        if (index.data(Role::VegetationObjectRole).value<CVegetationObject*>())
        {
            return Qt::ItemIsDragEnabled | Qt::ItemIsUserCheckable | defaultFlags;
        }
        else
        {
            auto category = m_categories.cbegin();
            std::advance(category, index.row());

            defaultFlags |= Qt::ItemIsDropEnabled | Qt::ItemIsEditable;
            if (category->second->m_objects.size() > 0)
            {
                defaultFlags |= Qt::ItemIsUserCheckable;
            }
            return defaultFlags;
        }
    }

    return defaultFlags;
}

bool VegetationCategoryTreeModel::setData(const QModelIndex& index, const QVariant& value, int role /* = Qt::EditRole */)
{
    if (!index.isValid() || index.model() != this)
    {
        return false;
    }

    auto object = index.data(Role::VegetationObjectRole).value<CVegetationObject*>();
    if (object == nullptr)
    {
        auto category = m_categories.cbegin();
        std::advance(category, index.row());

        switch (role)
        {
        case Qt::CheckStateRole:
        {
            for (auto object : category->second->m_objects)
            {
                object->SetHidden(value.toInt() == Qt::Unchecked);
            }
            emit dataChanged(index.child(0, 0), index.child(category->second->m_objects.size() - 1, 0), { Qt::CheckStateRole });
            return true;
        }
        }
    }
    else
    {
        switch (role)
        {
        case Qt::CheckStateRole:
        {
            object->SetHidden(value.toInt() == Qt::Unchecked);
            emit dataChanged(index.parent(), index.parent(), { Qt::CheckStateRole });
            return true;
        }
        }
    }

    return false;
}

bool VegetationCategoryTreeModel::removeRows(int row, int count, const QModelIndex& parent /* = {} */)
{
    if (!parent.isValid() || (parent.model() && parent.model() != this))
    {
        return false;
    }

    beginRemoveRows(parent, row, row + count - 1);

    auto entry = m_categories.begin();
    std::advance(entry, parent.row());
    auto& category = entry->second->m_objects;
    category.erase(category.begin() + row, category.begin() + row + count);

    endRemoveRows();

    if (category.size() == 0)
    {
        beginRemoveRows({}, parent.row(), parent.row());
        delete entry->second;
        m_categories.erase(entry);
        endRemoveRows();
    }

    return true;
}

QMimeData* VegetationCategoryTreeModel::mimeData(const QModelIndexList& indexes) const
{
    foreach(auto & index, indexes)
    {
        if (!index.isValid() || index.model() != this)
        {
            return new QMimeData();
        }
    }

    QMimeData* mime = new QMimeData();
    QByteArray array;
    foreach(auto & index, indexes)
    {
        auto object = index.data(Role::VegetationObjectRole).value<CVegetationObject*>();
        array.append(reinterpret_cast<const char*>(&object), sizeof(CVegetationObject*));
    }

    mime->setData(VEGETATION_OBJECT_MODEL_MIME_TYPE, array);

    return mime;
}

bool VegetationCategoryTreeModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    if (!parent.isValid() || parent.model() != this)
    {
        return false;
    }

    auto entry = m_categories.begin();
    std::advance(entry, parent.row());
    auto& category = entry->second->m_objects;

    auto buffer = data->data(VEGETATION_OBJECT_MODEL_MIME_TYPE);
    size_t count = buffer.size() / sizeof(CVegetationObject*);
    CVegetationObject** access = reinterpret_cast<CVegetationObject**>(buffer.data());

    if (row == -1)
    {
        emit beginInsertRows(parent, category.size(), category.size() + count - 1);
        category.insert(category.end(), access, access + count);
        emit endInsertRows();
    }
    else
    {
        emit beginInsertRows(parent, row, row + count - 1);
        category.insert(category.begin() + row, access, access + count);
        emit endInsertRows();
    }

    if (access[0]->GetCategory() != entry->first)
    {
        for (size_t i = 0; i < count; i++)
        {
            access[i]->SetCategory(entry->first);
        }
    }

    return true;
}

class CategoryItemSelectionModel
    : public QItemSelectionModel
{
public:
    CategoryItemSelectionModel(QAbstractItemModel* model, QObject* parent = nullptr)
        : QItemSelectionModel(model, parent)
    { }

    void select(const QItemSelection& selection, QItemSelectionModel::SelectionFlags command) override;
};

void CategoryItemSelectionModel::select(const QItemSelection& selection, QItemSelectionModel::SelectionFlags command)
{
    // Stuff is being deselected
    if (command == 0)
    {
        QItemSelectionModel::select(selection, command);
        return;
    }

    // Get the currently selected or first new category
    QString category;
    if (hasSelection() && !(command & QItemSelectionModel::Clear))
    {
        category = selectedIndexes().first().data(Role::CategoryNameRole).toString();
    }
    else if (!selection.isEmpty())
    {
        category = selection.indexes().first().data(Role::CategoryNameRole).toString();
    }

    // Only include ranges that match the category
    QItemSelection updatedSelection;
    auto scan = selection.cbegin();
    while (scan != selection.cend() && scan->topLeft().data(Role::CategoryNameRole).toString() == category)
    {
        updatedSelection.append(*scan);
        ++scan;
    }

    // If we find a category in the selected ranges, we select that and only that
    foreach(auto & s, updatedSelection)
    {
        auto first = s.topLeft();
        if (first.data(Role::VegetationObjectRole).value<CVegetationObject*>() == nullptr)
        {
            QItemSelection newSelection;
            newSelection.append({ first, first });

            int rows = model()->rowCount(first);
            if (rows > 0)
            {
                newSelection.append({ first.child(0, 0), first.child(rows - 1, 0) });
            }
            QItemSelectionModel::select(newSelection, command | QItemSelectionModel::Clear);
            return;
        }
    }

    // No category entries were selected so we clear any that were before
    for (int row = 0; row < model()->rowCount(); ++row)
    {
        auto index = model()->index(row, 0);
        QItemSelectionModel::select({ index, index }, QItemSelectionModel::Deselect);
    }

    QItemSelectionModel::select(updatedSelection, command);
}

enum
{
    VEGETATION_MENU_HIDE_ALL = 1,
    VEGETATION_MENU_UNHIDE_ALL,
    VEGETATION_MENU_GOTO_MATERIAL,
    VEGETATION_MENU_COPY_NAME,
};

namespace
{
    inline void ToggleEnabledState(const QSet<QObject*>& uiObjects, bool enabled)
    {
        for (QObject* uiObject : uiObjects)
        {
            QAction* action = qobject_cast<QAction*>(uiObject);
            if (action)
            {
                action->setEnabled(enabled);
            }
            else
            {
                QAbstractButton* button = qobject_cast<QAbstractButton*>(uiObject);
                if (button)
                {
                    button->setEnabled(enabled);
                }
            }
        }
    }

    inline QAction* CreateMenuAction(QMenu* menu, const QString& text, int id)
    {
        QAction* action = menu->addAction(text);
        action->setData(id);
        return action;
    }
}

/////////////////////////////////////////////////////////////////////////////
// CVegetationPanel dialog
//////////////////////////////////////////////////////////////////////////
CVegetationPanel::CVegetationPanel(CVegetationTool* tool, QWidget* parent /* = nullptr */)
    : QWidget(parent)
    , m_ui(new Ui::VegetationPanel())
    , m_model(new VegetationCategoryTreeModel(*GetIEditor()->GetVegetationMap(), this))
    , m_tool(tool)
    , m_previewPanel(nullptr)
    , m_vegetationMap(GetIEditor()->GetVegetationMap())
    , m_bIgnoreSelChange(false)
{
    assert(tool);
    assert(m_vegetationMap);

    OnInitDialog();

    GetIEditor()->RegisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
CVegetationPanel::~CVegetationPanel()
{
    GetIEditor()->UnregisterNotifyListener(this);
}

/////////////////////////////////////////////////////////////////////////////
// CVegetationPanel message handlers

void CVegetationPanel::OnBrushRadiusChange()
{
    double radius = m_ui->brushRadiusSpin->value();
    m_ui->brushRadiusSlider->setValue(radius * 100);
    m_tool->SetBrushRadius(radius);
}

void CVegetationPanel::OnBrushRadiusSliderChange(int value)
{
    double radius = value / 100.0;
    m_ui->brushRadiusSpin->setValue(radius);
    m_tool->SetBrushRadius(radius);
}

void CVegetationPanel::OnPaint()
{
    m_tool->SetPaintingMode(m_ui->paintObjectsButton->isChecked());
    MainWindow::instance()->setFocus();
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::UpdateUI()
{
    bool bPainting = m_tool->GetPaintingMode();

    if (bPainting)
    {
        m_ui->paintObjectsButton->setChecked(true);
#ifdef AZ_PLATFORM_APPLE
        GetIEditor()->SetStatusText(tr("Hold ⌘ to Remove Vegetation"));
#else
        GetIEditor()->SetStatusText(tr("Hold Ctrl to Remove Vegetation"));
#endif
    }
    else
    {
        m_ui->paintObjectsButton->setChecked(false);
        GetIEditor()->SetStatusText(tr("Push Paint button to start painting"));
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::OnRemoveDuplVegetation()
{
    m_tool->ClearThingSelection();
    m_vegetationMap->RemoveDuplVegetation();
    m_model->Reload();
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::OnInitDialog()
{
    m_ui->setupUi(this);
    m_ui->objectPropertiesControl->Setup();
    CreateObjectsContextMenu();

    m_ui->objectTreeView->setModel(m_model);
    m_ui->objectTreeView->setSelectionModel(new CategoryItemSelectionModel(m_model));
    m_ui->objectTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_ui->objectTreeView, &QTreeView::customContextMenuRequested, this, &CVegetationPanel::OnObjectsRClick);
    connect(m_ui->objectTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CVegetationPanel::OnObjectSelectionChanged);

    connect(m_ui->addObjectButton, &QToolButton::clicked, this, &CVegetationPanel::OnAdd);
    connect(m_ui->cloneObjectButton, &QToolButton::clicked, this, &CVegetationPanel::OnClone);
    m_enabledOnObjectSelected << m_ui->cloneObjectButton;
    connect(m_ui->replaceObjectButton, &QToolButton::clicked, this, &CVegetationPanel::OnReplace);
    m_enabledOnOneObjectSelected << m_ui->replaceObjectButton;
    connect(m_ui->addCategoryButton, &QToolButton::clicked, this, &CVegetationPanel::OnNewCategory);
    connect(m_ui->removeObjectButton, &QToolButton::clicked, this, &CVegetationPanel::OnRemove);
    connect(m_ui->exportButton, &QToolButton::clicked, this, &CVegetationPanel::OnBnClickedExport);

    connect(m_ui->importButton, &QToolButton::clicked, this, &CVegetationPanel::OnBnClickedImport);
    connect(m_ui->distributeOnTerrainButton, &QToolButton::clicked, this, &CVegetationPanel::OnDistribute);
    m_enabledOnObjectSelected << m_ui->distributeOnTerrainButton;
    connect(m_ui->clearButton, &QToolButton::clicked, this, &CVegetationPanel::OnClear);
    connect(m_ui->scaleButton, &QToolButton::clicked, this, &CVegetationPanel::OnBnClickedScale);
    connect(m_ui->mergeButton, &QToolButton::clicked, this, &CVegetationPanel::OnBnClickedMerge);
    m_enabledOnObjectSelected << m_ui->mergeButton;
    connect(m_ui->putSelectionToCategoryButton, &QToolButton::clicked, this, &CVegetationPanel::OnInstancesToCategory);

    connect(m_ui->brushRadiusSpin, &QDoubleSpinBox::editingFinished, this, &CVegetationPanel::OnBrushRadiusChange);
    connect(m_ui->brushRadiusSlider, &QSlider::sliderMoved, this, &CVegetationPanel::OnBrushRadiusSliderChange);
    connect(m_ui->paintObjectsButton, &QPushButton::clicked, this, &CVegetationPanel::OnPaint);
    connect(m_ui->removeDuplicatedButton, &QPushButton::clicked, this, &CVegetationPanel::OnRemoveDuplVegetation);


    // make sure that all of our buttons and menus have been properly disabled until a selection is made
    ToggleEnabledState(m_enabledOnObjectSelected, false);
    ToggleEnabledState(m_enabledOnCategorySelected, false);
    ToggleEnabledState(m_enabledOnOneObjectSelected, false);

    // the remove object button applies to both categories and objects, so have to special case it
    m_ui->removeObjectButton->setEnabled(false);

    SetBrush(m_tool->GetBrushRadius());

    // need this otherwise the reload objects will cause a message to be sent in CPropertyCtrl::SendNotify before CPropertyCtrl::Create is called
    ensurePolished();

    ReloadObjects();

    /*
    CRect rc;
    GetDlgItem(IDC_PREVIEW)->GetWindowRect(rc);
    ScreenToClient( rc );
    GetDlgItem(IDC_PREVIEW)->ShowWindow(SW_HIDE);
    if (gSettings.bPreviewGeometryWindow)
        m_previewCtrl.Create( this,rc,WS_VISIBLE|WS_CHILD|WS_BORDER );
        */

    SendToControls();
    UpdateInfo();
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::GetObjectsInCategory(const QString& category, Selection& objects)
{
    objects.clear();
    for (int i = 0; i < m_vegetationMap->GetObjectCount(); i++)
    {
        CVegetationObject* obj = m_vegetationMap->GetObject(i);
        if (category == obj->GetCategory())
        {
            objects.push_back(obj);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::OnNewCategory()
{
    bool ok = false;
    QString text = QInputDialog::getText(this, tr("New Category"), QStringLiteral(""), QLineEdit::Normal, QStringLiteral(""), &ok);
    if (ok)
    {
        m_model->AddCategory(text);
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::OnRenameCategory()
{
    auto selected = m_ui->objectTreeView->selectionModel()->selectedIndexes();
    if (selected.isEmpty())
    {
        return;
    }
    QString category = selected.first().data(Role::CategoryNameRole).toString();

    bool ok = false;
    QString newCategoryName = QInputDialog::getText(this, tr("Rename Category"), QStringLiteral(""), QLineEdit::Normal, category, &ok);

    if (ok && category != newCategoryName && !newCategoryName.isEmpty())
    {
        Selection newObjects;
        GetObjectsInCategory(category, newObjects);

        for (int i = 0; i < newObjects.size(); i++)
        {
            newObjects[i]->SetCategory(newCategoryName);
        }
        m_model->RenameCategory(category, newCategoryName);
    }
}

//////////////////////////////////////////////////////////////////////////

void CVegetationPanel::OnRemoveCategory()
{
    auto selected = m_ui->objectTreeView->selectionModel()->selectedIndexes();
    if (selected.isEmpty())
    {
        return;
    }
    QString category = selected.first().data(Role::CategoryNameRole).toString();

    Selection delObjects;
    GetObjectsInCategory(category, delObjects);

    if (delObjects.size() > 0)
    {
        const QString message = tr(R"(Delete vegetation category "%1" and all its objects?)").arg(category);
        if (QMessageBox::Yes != QMessageBox::question(this, tr("Delete Category?"), message))
        {
            return;
        }
    }

    m_tool->ClearThingSelection();

    CUndo undo(tr("Remove VegObject(s)").toUtf8());

    m_bIgnoreSelChange = true;

    for (int i = 0; i < delObjects.size(); i++)
    {
        m_model->RemoveObject(delObjects[i]);
        m_vegetationMap->RemoveObject(delObjects[i]);
    }

    m_model->RemoveCategory(category);

    GetIEditor()->GetTerrainManager()->ReloadSurfaceTypes();

    m_bIgnoreSelChange = false;

    // force leave paint mode
    m_tool->SetPaintingMode(false);
    m_ui->paintObjectsButton->setChecked(false);
    MainWindow::instance()->setFocus();

    SendToControls();
    GetIEditor()->UpdateViews(eUpdateStatObj);
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::OnDistribute()
{
    if (QMessageBox::Yes == QMessageBox::question(this, tr("Vegetation Distribute"), tr("Are you sure you want to Distribute?")))
    {
        CUndo undo(tr("Vegetation Distribute").toUtf8());
        QApplication::setOverrideCursor(Qt::WaitCursor);
        if (m_tool)
        {
            m_tool->Distribute();
        }
        QApplication::restoreOverrideCursor();
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::OnClear()
{
    if (QMessageBox::Yes == QMessageBox::question(this, tr("Vegetation Clear"), tr("Are you sure you want to Clear?")))
    {
        CUndo undo(tr("Vegetation Clear").toUtf8());
        QApplication::setOverrideCursor(Qt::WaitCursor);
        if (m_tool)
        {
            m_tool->Clear();
        }
        QApplication::restoreOverrideCursor();
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::OnBnClickedScale()
{
    CUndo undo(tr("Vegetation Scale").toUtf8());
    if (m_tool)
    {
        m_tool->ScaleObjects();
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::OnBnClickedImport()
{
    QString file;
    if (CFileUtil::SelectFile("Vegetation Objects (*.veg);;All Files (*)", GetIEditor()->GetLevelFolder(), file))
    {
        XmlNodeRef root = XmlHelpers::LoadXmlFromFile(file.toUtf8().data());
        if (!root)
        {
            return;
        }

        QWaitCursor wait;

        m_tool->ClearThingSelection();
        CUndo undo(tr("Import Vegetation ").toUtf8());

        m_ui->objectPropertiesControl->RemoveAllItems();

        for (int i = 0; i < root->getChildCount(); i++)
        {
            auto c = root->getChild(i);
            m_vegetationMap->ImportObject(c);
        }
        ReloadObjects();
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::OnBnClickedMerge()
{
    Selection objects;
    m_tool->GetSelectedObjects(objects);
    if (objects.size() < 2)
    {
        QMessageBox::warning(this, tr("Merge Vegetation"), tr("Select two or more vegetation objects to merge"));
        return;
    }

    if (QMessageBox::Yes == QMessageBox::question(this, tr("Merge Vegetation"), tr("Are you sure you want to merge?")))
    {
        QWaitCursor wait;
        CUndo undo(tr("Vegetation Merge").toUtf8());
        std::vector<CVegetationObject*> delObjects;

        CVegetationObject* pObject = objects[0];

        for (int j = 1; j < objects.size(); j++)
        {
            CVegetationObject* object = objects[j];

            m_vegetationMap->MergeObjects(pObject, object);
            delObjects.push_back(object);
        }

        for (int k = 0; k < delObjects.size(); k++)
        {
            m_vegetationMap->RemoveObject(delObjects[k]);
        }
        ReloadObjects();
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::OnBnClickedExport()
{
    CUndo undo(tr("Vegetation Export").toUtf8());
    Selection objects;
    m_tool->GetSelectedObjects(objects);
    if (objects.empty())
    {
        QMessageBox::warning(this, tr("Select Objects for Export"), tr("Please select some objects to export"));
        return;
    }

    QString fileName;
    if (CFileUtil::SelectSaveFile("Vegetation Objects (*.veg)", "veg", GetIEditor()->GetLevelFolder(), fileName))
    {
        QWaitCursor wait;
        XmlNodeRef root = XmlHelpers::CreateXmlNode("Vegetation");
        for (int i = 0; i < objects.size(); i++)
        {
            XmlNodeRef node = root->newChild("VegetationObject");
            m_vegetationMap->ExportObject(objects[i], node);
        }
        XmlHelpers::SaveXmlNode(GetIEditor()->GetFileUtil(), root, fileName.toUtf8().data());
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::OnBnClickedRandomRotate()
{
    CUndo undo(tr("Vegetation Random Rotate").toUtf8());
    if (m_tool)
    {
        m_tool->DoRandomRotate();
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::OnBnClickedClearRotate()
{
    CUndo undo(tr("Vegetation Clear Rotate").toUtf8());
    if (m_tool)
    {
        m_tool->DoClearRotate();
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::ReloadObjects()
{
    m_bIgnoreSelChange = true;
    // Clear all selections.
    for (int i = 0; i < m_vegetationMap->GetObjectCount(); i++)
    {
        m_vegetationMap->GetObject(i)->SetSelected(false);
    }
    m_bIgnoreSelChange = false;

    m_model->Reload();

    m_ui->objectTreeView->expandAll();

    SendToControls();

    // Give focus back to main view.
    MainWindow::instance()->setFocus();
}

void CVegetationPanel::AddObjectToTree(CVegetationObject* object, bool bExpandCategory)
{
    auto index = m_model->AddObject(object);

    if (bExpandCategory)
    {
        m_ui->objectTreeView->expand(index.parent());
    }
}

void CVegetationPanel::UpdateObjectInTree(CVegetationObject* object, bool bUpdateInfo)
{
    m_model->UpdateData(object);

    if (bUpdateInfo)
    {
        UpdateInfo();
    }
}

void CVegetationPanel::UpdateAllObjectsInTree()
{
    UpdateInfo();
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::SelectObject(CVegetationObject* object, bool bAddToSelection)
{
    m_bIgnoreSelChange = true;
    QModelIndex i = m_model->ObjectIndex(object);
    if (!i.isValid())
    {
        return;
    }
    m_ui->objectTreeView->expand(i.parent());
    m_ui->objectTreeView->selectionModel()->select(i, QItemSelectionModel::ClearAndSelect);

    //Scroll to selected object in vegetation panel
    m_ui->objectTreeView->scrollTo(i);

    if (!object->IsSelected())
    {
        object->SetSelected(true);
        SendToControls();
    }
    m_bIgnoreSelChange = false;
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::SetBrush(float r)
{
    QSignalBlocker sliderBlocker(m_ui->brushRadiusSlider);
    m_ui->brushRadiusSpin->setValue(r);
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::UnselectAll()
{
    m_ui->objectTreeView->clearSelection();
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::UpdateInfo()
{
    m_ui->totalObjectsLabel->setText(tr("Total Objects: %1").arg(m_vegetationMap->GetObjectCount()));
    m_ui->totalInstancesLabel->setText(tr("Total Instances: %1").arg(m_vegetationMap->GetNumInstances()));
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::OnAdd()
{
    ////////////////////////////////////////////////////////////////////////
    // Add another static object to the list
    ////////////////////////////////////////////////////////////////////////
    AssetSelectionModel selection = AssetSelectionModel::AssetGroupSelection("Geometry", true);
    AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);
    if (!selection.IsValid())
    {
        return;
    }

    QString category;
    auto selected = m_ui->objectTreeView->selectionModel()->selectedIndexes();
    if (!selected.isEmpty())
    {
        category = selected.first().data(Role::CategoryNameRole).toString();
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);

    CUndo undo(tr("Add VegObject(s)").toUtf8());
    for (const AssetBrowserEntry* selectionResults : selection.GetResults())
    {
        AZStd::vector<const ProductAssetBrowserEntry*> products;
        selectionResults->GetChildrenRecursively<ProductAssetBrowserEntry>(products);
        for (const ProductAssetBrowserEntry*  product : products)
        {
           
            // Create a new static object settings class
            CVegetationObject* obj = m_vegetationMap->CreateObject();
            if (!obj)
            {
                continue;
            }
            AZStd::string legacyAssetID;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(legacyAssetID, &AZ::Data::AssetCatalogRequests::GetAssetPathById, product->GetAssetId());
            obj->SetFileName(legacyAssetID.c_str());
            if (!category.isEmpty())
            {
                obj->SetCategory(category);
            }

            AddObjectToTree(obj, true);
        }
    }

    QApplication::restoreOverrideCursor();
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::OnClone()
{
    auto selected = m_ui->objectTreeView->selectionModel()->selectedIndexes();
    if (selected.isEmpty())
    {
        return;
    }

    QModelIndex first = selected.first();

    // Check if category selected.
    if (first.data(Role::VegetationObjectRole).value<CVegetationObject*>() == nullptr)
    {
        QString oldCategory = first.data(Role::CategoryNameRole).toString();
        // clone all elements in a new group / category
        bool ok = false;
        QString clonedCategory = QInputDialog::getText(this, tr("Clone Category"), QStringLiteral(""), QLineEdit::Normal, oldCategory, &ok);

        if (ok && oldCategory != clonedCategory)
        {
            Selection objects;
            GetObjectsInCategory(oldCategory, objects);
            for (int i = 0; i < objects.size(); i++)
            {
                CVegetationObject* object = objects[i];
                CVegetationObject* newObject = m_vegetationMap->CreateObject(object);
                if (newObject)
                {
                    AddObjectToTree(newObject);
                    newObject->SetCategory(clonedCategory);
                }
            }
            ReloadObjects();
        }
    }
    else
    {
        // clone all selected elements in current group / category
        CUndo undo(tr("Clone VegObject").toUtf8());
        foreach(auto & selection, selected)
        {
            CVegetationObject* object = selection.data(Role::VegetationObjectRole).value<CVegetationObject*>();
            CVegetationObject* newObject = m_vegetationMap->CreateObject(object);
            if (newObject)
            {
                AddObjectToTree(newObject);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::OnReplace()
{
    Selection objects;
    m_tool->GetSelectedObjects(objects);
    if (objects.size() != 1)
    {
        return;
    }

    // Check if category selected.
    CVegetationObject* object = objects[0];

    AssetSelectionModel selection = AssetSelectionModel::AssetGroupSelection("Geometry");
    AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);
    if (!selection.IsValid())
    {
        return;
    }


    QApplication::setOverrideCursor(Qt::WaitCursor);
    CUndo undo(tr("Replace VegObject").toUtf8());
    object->SetFileName(selection.GetResult()->GetFullPath().c_str());
    m_vegetationMap->RepositionObject(object);

    m_model->UpdateData(object);

    QApplication::restoreOverrideCursor();
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::OnRemove()
{
    std::vector<CVegetationObject*> objects;
    m_tool->GetSelectedObjects(objects);

    if (objects.empty())
    {
        // if the list of selected objects is empty, it's most likely that the user
        // has selected a category that's empty. So let them remove that instead.
        OnRemoveCategory();

        return;
    }

    // validate
    if (QMessageBox::Yes != QMessageBox::question(this, tr("Delete Vegetation Object(s)?"), tr("Are you sure you'd like to delete the selected object(s)?")))
    {
        return;
    }

    // Unselect all instances.
    m_tool->ClearThingSelection();

    QWaitCursor wait;
    CUndo undo(tr("Remove VegObject(s)").toUtf8());

    m_bIgnoreSelChange = true;
    for (auto object : objects)
    {
        m_model->RemoveObject(object);
        m_vegetationMap->RemoveObject(object);
    }
    GetIEditor()->GetTerrainManager()->ReloadSurfaceTypes();
    m_bIgnoreSelChange = false;

    SendToControls();
    UpdateInfo();

    GetIEditor()->UpdateViews(eUpdateStatObj);
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::OnInstancesToCategory()
{
    //CUndo undo("Move instances" );
    if (m_tool->GetCountSelectedInstances())
    {
        bool ok = false;
        QString category = QInputDialog::getText(this, tr("Enter Category Name"), QStringLiteral(""), QLineEdit::Normal, QStringLiteral(""), &ok);
        if (ok)
        {
            m_tool->MoveSelectedInstancesToCategory(category);
            //ReloadObjects();
            GetIEditor()->Notify(eNotify_OnVegetationPanelUpdate);
        }
        MainWindow::instance()->setFocus();
    }
    else
    {
        QMessageBox::warning(this, tr("Can't Move Instances"), tr("Please select some instances to move to the category"));
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::OnObjectSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    if (m_bIgnoreSelChange)
    {
        return;
    }

    static bool bNoRecurse = false;
    if (bNoRecurse)
    {
        return;
    }
    bNoRecurse = true;

    m_bIgnoreSelChange = true;
    // Clear all selections.
    for (int i = 0; i < m_vegetationMap->GetObjectCount(); i++)
    {
        m_vegetationMap->GetObject(i)->SetSelected(false);
    }

    bool isCategorySelected = false;
    int selectedObjectCount = 0;

    auto indices = m_ui->objectTreeView->selectionModel()->selectedIndexes();
    foreach(auto index, indices)
    {
        CVegetationObject* object = index.data(Role::VegetationObjectRole).value<CVegetationObject*>();
        if (object)
        {
            object->SetSelected(true);

            selectedObjectCount++;
        }
        else
        {
            isCategorySelected = true;
        }
    }
    m_bIgnoreSelChange = false;

    ToggleEnabledState(m_enabledOnObjectSelected, selectedObjectCount > 0);
    ToggleEnabledState(m_enabledOnCategorySelected, isCategorySelected);
    ToggleEnabledState(m_enabledOnOneObjectSelected, selectedObjectCount == 1);

    // special case the merge button, because it needs more than one object selected
    m_ui->mergeButton->setEnabled(selectedObjectCount > 1);

    // the remove object button applies to both categories and objects, so have to special case it
    m_ui->removeObjectButton->setEnabled((selectedObjectCount > 0) || isCategorySelected);

    SendToControls();
    bNoRecurse = false;
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::SendToControls()
{
    Selection objects;
    m_tool->GetSelectedObjects(objects);

    // Delete all var block.
    m_varBlock = 0;
    m_ui->objectPropertiesControl->RemoveAllItems();

    if (objects.empty())
    {
        if (m_previewPanel)
        {
            m_previewPanel->LoadFile("");
        }
        m_ui->objectPropertiesControl->setEnabled(false);
        return;
    }
    else
    {
        m_ui->objectPropertiesControl->setEnabled(true);
    }

    /*
    if (objects.size() == 1)
    {
        CVegetationObject *object = objects[0];
        if (m_previewPanel)
            m_previewPanel->LoadFile( object->GetFileName() );

        m_ui->objectPropertiesControl->m_props.DeleteAllItems();
        m_ui->objectPropertiesControl->m_props.AddVarBlock( object->GetVarBlock() );
        m_ui->objectPropertiesControl->m_props.ExpandAll();

        m_ui->objectPropertiesControl->m_props.SetDisplayOnlyModified(false);
    }
    else
    */
    {
        m_varBlock = objects[0]->GetVarBlock()->Clone(true);
        for (int i = 0; i < objects.size(); i++)
        {
            m_varBlock->Wire(objects[i]->GetVarBlock());
        }

        // Add Surface types to varblock.
        if (objects.size() == 1)
        {
            AddLayerVars(m_varBlock, objects[0]);
        }
        else
        {
            AddLayerVars(m_varBlock);
        }

        // Add variable blocks of all objects.
        m_ui->objectPropertiesControl->AddVarBlock(m_varBlock);
        m_ui->objectPropertiesControl->ExpandAll();
    }
    if (objects.size() == 1)
    {
        CVegetationObject* object = objects[0];
        if (m_previewPanel)
        {
            m_previewPanel->LoadFile(object->GetFileName());
        }
    }
    else
    {
        if (m_previewPanel)
        {
            m_previewPanel->LoadFile("");
        }
    }

    SendTextureLayersToControls();
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::AddLayerVars(CVarBlock* pVarBlock, CVegetationObject* pObject)
{
    IVariable* pTable = new CVariableArray();
    pTable->SetName(tr("Use On Terrain Layers"));
    pVarBlock->AddVariable(pTable);
    int num = GetIEditor()->GetTerrainManager()->GetSurfaceTypeCount();

    QStringList layers;
    if (pObject)
    {
        pObject->GetTerrainLayers(layers);
    }

    for (int i = 0; i < num; i++)
    {
        CSurfaceType* pType = GetIEditor()->GetTerrainManager()->GetSurfaceTypePtr(i);

        if (!pType)
        {
            continue;
        }

        CVariable<bool>* pBoolVar = new CVariable<bool>();
        pBoolVar->SetName(pType->GetName());
        pBoolVar->AddOnSetCallback(functor(*this, &CVegetationPanel::OnLayerVarChange));
        if (stl::find(layers, pType->GetName()))
        {
            pBoolVar->Set((bool)true);
        }
        else
        {
            pBoolVar->Set((bool)false);
        }

        pTable->AddVariable(pBoolVar);
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::OnLayerVarChange(IVariable* pVar)
{
    if (!pVar)
    {
        return;
    }
    QString layerName = pVar->GetName();
    bool bValue;

    std::vector<CVegetationObject*> objects;
    m_tool->GetSelectedObjects(objects);

    pVar->Get(bValue);

    if (bValue)
    {
        for (int i = 0; i < objects.size(); i++)
        {
            CVegetationObject* pObject = objects[i];
            QStringList layers;
            pObject->GetTerrainLayers(layers);
            stl::push_back_unique(layers, layerName);
            pObject->SetTerrainLayers(layers);
        }
    }
    else
    {
        for (int i = 0; i < objects.size(); i++)
        {
            CVegetationObject* pObject = objects[i];
            QStringList layers;
            pObject->GetTerrainLayers(layers);
            stl::find_and_erase(layers, layerName);
            pObject->SetTerrainLayers(layers);
        }
    }

    GetIEditor()->GetTerrainManager()->ReloadSurfaceTypes();

    for (int i = 0; i < objects.size(); i++)
    {
        CVegetationObject* pObject = objects[i];
        pObject->SetEngineParams();
    }
    GetIEditor()->SetModifiedFlag();
    GetIEditor()->SetModifiedModule(eModifiedTerrain);
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::SendTextureLayersToControls()
{
    QStringList layerNames;
    if (GetTerrainLayerNames(layerNames))
    {
        const QString propertyName = tr("Get Slope and Elevation from");
        m_ui->objectPropertiesControl->RemoveCustomPopupMenuPopup(propertyName);
        m_ui->objectPropertiesControl->AddCustomPopupMenuPopup(
            propertyName, functor(*this, &CVegetationPanel::OnGetSettingFromTerrainLayer), layerNames);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CVegetationPanel::GetTerrainLayerNames(QStringList& layerNames)
{
    CTerrainManager* pTerrainManager = GetIEditor()->GetTerrainManager();
    const int nLayers = pTerrainManager->GetLayerCount();

    for (int i = 0; i < nLayers; ++i)
    {
        CLayer* pLayer = pTerrainManager->GetLayer(i);
        CSurfaceType* pSurfaceType = pLayer->GetSurfaceType();

        if (pSurfaceType)
        {
            layerNames.push_back(
                tr("%1 - %2").arg(
                    pLayer->GetLayerName(),
                    pSurfaceType->GetMaterial()
                    )
                );
        }
    }
    return nLayers != 0;
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::OnGetSettingFromTerrainLayer(int layerId)
{
    if (!m_varBlock)
    {
        return;
    }

    CTerrainManager* pTerrainManager = GetIEditor()->GetTerrainManager();
    const int nLayers = pTerrainManager->GetLayerCount();

    if (layerId < nLayers)
    {
        CLayer* pLayer = pTerrainManager->GetLayer(layerId);

        const int nVariables = m_varBlock->GetNumVariables();

        for (int i = 0; i < nVariables; ++i)
        {
            IVariable* pVariable = m_varBlock->GetVariable(i);

            if (!pVariable)
            {
                continue;
            }

            QString name = pVariable->GetName();

            if (name.isEmpty())
            {
                continue;
            }

            if (QString::compare(name, VEGETATION_ELEVATION_MIN) == 0)
            {
                pVariable->Set(pLayer->GetLayerStart());
            }
            else if (QString::compare(name, VEGETATION_ELEVATION_MAX) == 0)
            {
                pVariable->Set(pLayer->GetLayerEnd());
            }
            else if (QString::compare(name, VEGETATION_SLOPE_MIN) == 0)
            {
                pVariable->Set(pLayer->GetLayerMinSlopeAngle());
            }
            else if (QString::compare(name, VEGETATION_SLOPE_MAX) == 0)
            {
                pVariable->Set(pLayer->GetLayerMaxSlopeAngle());
            }
        }
    }
}

void CVegetationPanel::CreateObjectsContextMenu()
{
    m_menu = new QMenu(this);

    CreateMenuAction(m_menu, tr("Add Category"), ID_PANEL_VEG_ADDCATEGORY);
    m_enabledOnCategorySelected << CreateMenuAction(m_menu, tr("Rename Category"), ID_PANEL_VEG_RENAMECATEGORY);
    m_enabledOnCategorySelected << CreateMenuAction(m_menu, tr("Remove Category"), ID_PANEL_VEG_REMOVECATEGORY);

    m_menu->addSeparator();

    CreateMenuAction(m_menu, tr("Add Object"), ID_PANEL_VEG_ADD);
    m_enabledOnObjectSelected << CreateMenuAction(m_menu, tr("Clone Object"), ID_PANEL_VEG_CLONE);
    m_enabledOnOneObjectSelected << CreateMenuAction(m_menu, tr("Replace Object"), ID_PANEL_VEG_REPLACE);
    m_enabledOnObjectSelected << CreateMenuAction(m_menu, tr("Remove Object"), ID_PANEL_VEG_REMOVE);

    m_menu->addSeparator();

    m_enabledOnObjectSelected << CreateMenuAction(m_menu, tr("Auto Distribute Object"), ID_PANEL_VEG_DISTRIBUTE);
    m_enabledOnObjectSelected << CreateMenuAction(m_menu, tr("Clear All Instances"), ID_PANEL_VEG_CLEAR);
    CreateMenuAction(m_menu, tr("Selected Instances to Category"), ID_PANEL_VEG_CREATE_SEL);
    CreateMenuAction(m_menu, tr("Randomly Rotate All Instances"), ID_PANEL_VEG_RANDOM_ROTATE);
    CreateMenuAction(m_menu, tr("Clear Rotation"), ID_PANEL_VEG_CLEAR_ROTATE);
    CreateMenuAction(m_menu, tr("Scale Selected Instances"), ID_PANEL_VEG_SCALE);
    CreateMenuAction(m_menu, tr("Go To Object Material"), VEGETATION_MENU_GOTO_MATERIAL);
    CreateMenuAction(m_menu, tr("Copy Name"), VEGETATION_MENU_COPY_NAME);

    m_menu->addSeparator();

    CreateMenuAction(m_menu, tr("Hide All Objects"), VEGETATION_MENU_HIDE_ALL);
    CreateMenuAction(m_menu, tr("Unhide All Objects"), VEGETATION_MENU_UNHIDE_ALL);

    m_menu->addSeparator();

    CreateMenuAction(m_menu, tr("Import Objects"), ID_PANEL_VEG_IMPORT);
    CreateMenuAction(m_menu, tr("Export Objects"), ID_PANEL_VEG_EXPORT);
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::OnObjectsRClick(const QPoint& point)
{
    QModelIndex index = m_ui->objectTreeView->indexAt(point);

    QAction* action = m_menu->exec(m_ui->objectTreeView->mapToGlobal(point));
    if (action != nullptr)
    {
        switch (action->data().toInt())
        {
        case ID_PANEL_VEG_ADDCATEGORY:
            OnNewCategory();
            break;
        case ID_PANEL_VEG_RENAMECATEGORY:
            OnRenameCategory();
            break;
        case ID_PANEL_VEG_REMOVECATEGORY:
            OnRemoveCategory();
            break;
        case ID_PANEL_VEG_ADD:
            OnAdd();
            break;
        case ID_PANEL_VEG_CLONE:
            OnClone();
            break;
        case ID_PANEL_VEG_REPLACE:
            OnReplace();
            break;
        case ID_PANEL_VEG_REMOVE:
            OnRemove();
            break;
        case ID_PANEL_VEG_DISTRIBUTE:
            OnDistribute();
            break;
        case ID_PANEL_VEG_CLEAR:
            OnClear();
            break;
        case ID_PANEL_VEG_CREATE_SEL:
            OnInstancesToCategory();
            break;
        case ID_PANEL_VEG_RANDOM_ROTATE:
            OnBnClickedRandomRotate();
            break;
        case ID_PANEL_VEG_CLEAR_ROTATE:
            OnBnClickedClearRotate();
            break;
        case ID_PANEL_VEG_SCALE:
            OnBnClickedScale();
            break;
        case ID_PANEL_VEG_IMPORT:
            OnBnClickedImport();
            break;
        case ID_PANEL_VEG_EXPORT:
            OnBnClickedExport();
            break;
        case VEGETATION_MENU_HIDE_ALL:
            m_vegetationMap->HideAllObjects(true);
            //m_model->UpdateData();
            break;
        case VEGETATION_MENU_UNHIDE_ALL:
            m_vegetationMap->HideAllObjects(false);
            //m_model->UpdateData();
            break;
        case VEGETATION_MENU_GOTO_MATERIAL:
            GotoObjectMaterial();
            break;
        case VEGETATION_MENU_COPY_NAME:
        {
            std::vector<CVegetationObject*> objects;
            m_tool->GetSelectedObjects(objects);
            if (!objects.empty())
            {
                CVegetationObject* pVegetationObject = objects[0];
                CClipboard clipboard(this);
                clipboard.PutString(Path::GetFileName(pVegetationObject->GetFileName()), "Vegetation Object Name");
            }
        }
        break;
        default:
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::GotoObjectMaterial()
{
    std::vector<CVegetationObject*> objects;
    m_tool->GetSelectedObjects(objects);
    if (!objects.empty())
    {
        CVegetationObject* pVegObj = objects[0];
        IStatObj* pStatObj = pVegObj->GetObject();
        if (pStatObj)
        {
            GetIEditor()->GetMaterialManager()->GotoMaterial(pStatObj->GetMaterial());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationPanel::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnVegetationObjectSelection:
        if (!m_bIgnoreSelChange)
        {
            Selection objects;
            m_tool->GetSelectedObjects(objects);

            m_bIgnoreSelChange = true;
            m_ui->objectTreeView->clearSelection();
            for (int i = 0; i < objects.size(); i++)
            {
                SelectObject(objects[i], true);
            }
            m_bIgnoreSelChange = false;
            SendToControls();
        }
        break;

    case eNotify_OnVegetationPanelUpdate:
        ReloadObjects();
        break;

    case eNotify_OnTextureLayerChange:
        SendTextureLayersToControls();
        break;
    }
}
