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
#include "LayersListBox.h"
#include "Objects/ObjectLayerManager.h"
#include "Viewport.h"

#include "IEditor.h"
#include "BaseLibrary.h"
#include "BaseLibraryItem.h"
#include "BaseLibraryManager.h"
#include "ISourceControl.h"
#include "GameEngine.h"
#include "QtUtilWin.h"
#include <QThreadPool>
#include <QRunnable>
#include <QMutexLocker>
#include <QHeaderView>
#include <QAbstractTableModel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QMouseEvent>

class LayerDelegate;

// CLayersListBox dialog
#define INDENT_SIZE 16
#define ITEM_VISIBLE_BITMAP 0
#define ITEM_USABLE_BITMAP 1
#define ITEM_EXPANDED_BITMAP 2
#define ITEM_COLLAPSED_BITMAP 3
#define ITEM_LEAF_BITMAP 4
#define ITEM_LEAF_NOTEXPORTABLE_BITMAP 5
#define ITEM_BLANK_BITMAP 6

const int IMAGE_LIST_ICON_SIZE = 16;

#define ITEM_SOURCE_CONTROL_MANAGED 0
#define ITEM_SOURCE_CONTROL_NOTMANAGED 1
#define ITEM_SOURCE_CONTROL_CHECKEDOUTBY_ME 2
#define ITEM_SOURCE_CONTROL_NOTATHEAD 3
#define ITEM_SOURCE_CONTROL_ADD 4
#define ITEM_SOURCE_CONTROL_NOFILE 5
#define ITEM_SOURCE_CONTROL_CHECKEDOUTBY_OTHER 6
#define ITEM_SOURCE_CONTROL_LOCKED 7

namespace
{
    bool IsNearColors(const QColor& col1, const QColor& col2)
    {
        int nNearCount = 0;
        if (abs(col1.red() - col2.red()) < 127)
        {
            nNearCount++;
        }
        if (abs(col1.green() - col2.green()) < 127)
        {
            nNearCount++;
        }
        if (abs(col1.blue() - col2.blue()) < 127)
        {
            nNearCount++;
        }

        if (nNearCount >= 2) // if at least two components are near
        {
            return true;
        }
        return false;
    }
}

//////////////////////////////////////////////////////////////////////////
// CLayerButtons

const int buttonPadding = 1;

class LayerModel
    : public QAbstractTableModel
{
public:
    LayerModel(QMutex& mapMutex, QObject* parent = nullptr);
    ~LayerModel();

    enum Columns
    {
        VisibleColumn = 0,
        UsableColumn,
        DetailsColumn
    };

    enum Roles
    {
        NameRole = Qt::DisplayRole,
        VisbleRole = Qt::UserRole,
        UsableRole,
        ExportableRole,
        ExpandedRole,
        HasChildrenRole,
        IndentRole,
        SourceControlRole,
        ExternalRole
    };

    void SetLayers(const std::vector<CLayersListBox::SLayerInfo>& layers);
    void UpdateAttributeMap(const CLayersListBox::LayersAttributeMap& layerAttributeMap);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    int RowForLayer(const QString layerName) const;
    QModelIndex IndexForLayer(const QString& layerName) const;

    void SetLayerInfo(int row, const CLayersListBox::SLayerInfo& info);

    void HandleDataChange(int row, int column, const QVector<int> roles, bool recursive);
    void InvalidateFileAttributes();

    void SetDraggingItem(int layerNo);

private:
    std::vector<CLayersListBox::SLayerInfo> m_layersInfo;
    CLayersListBox::LayersAttributeMap m_layersAttributeMap;
    int m_draggingItemNo;
    QMutex& m_mutex;
};


LayerModel::LayerModel(QMutex& mapMutex, QObject* parent)
    : QAbstractTableModel(parent)
    , m_draggingItemNo(-1)
    , m_mutex(mapMutex)
{
}

LayerModel::~LayerModel()
{
}

void LayerModel::SetLayers(const std::vector<CLayersListBox::SLayerInfo>& layers)
{
    beginResetModel();
    m_layersInfo = layers;
    endResetModel();
}

void LayerModel::UpdateAttributeMap(const CLayersListBox::LayersAttributeMap& layerAttributeMap)
{
    QMutexLocker locker(&m_mutex);
    m_layersAttributeMap = layerAttributeMap;
}


int LayerModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_layersInfo.size();
}

int LayerModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : 3;
}

QVariant LayerModel::data(const QModelIndex& index, int role) const
{
    Q_ASSERT(index.row() < m_layersInfo.size());
    Q_ASSERT(index.column() < 3);

    const CLayersListBox::SLayerInfo& layer = m_layersInfo.at(index.row());

    if (index.column() == VisibleColumn && role == VisbleRole)
    {
        return layer.visible;
    }
    else if (index.column() == UsableColumn && role == UsableRole)
    {
        return layer.usable;
    }
    else if (index.column() == DetailsColumn)
    {
        if (role == NameRole)
        {
            QString name = layer.name;
            if (layer.isModified)
            {
                name += "*";
            }
            return name;
        }

        else if (role == HasChildrenRole)
        {
            return layer.childCount > 0;
        }
        else if (role == ExpandedRole)
        {
            return layer.expanded;
        }
        else if (role == ExportableRole)
        {
            return layer.pLayer->IsExportable();
        }
        else if (role == Qt::BackgroundRole)
        {
            return layer.pLayer->IsDefaultColor() ? QVariant() : layer.pLayer->GetColor();
        }
        else if (role == IndentRole)
        {
            return layer.indent;
        }
        else if (role == ExternalRole)
        {
            return layer.pLayer->IsExternal();
        }
        else if (role == SourceControlRole)
        {
            //draw some source control information on the layers if its available
            if (GetIEditor()->IsSourceControlAvailable())
            {
                //Read Layer Attributes from the Cache
                QMutexLocker locker(&m_mutex);
                auto found = m_layersAttributeMap.find(layer.name);
                if (found != m_layersAttributeMap.end())
                {
                    CLayersListBox::SCacheLayerAttributes layerAttributes = found->second;
                    return layerAttributes.fileAttributes;
                }
                return ESccFileAttributes::SCC_FILE_ATTRIBUTE_INVALID;
            }
        }
    }
    return QVariant();
}

int LayerModel::RowForLayer(const QString layerName) const
{
    for (int i = 0; i < m_layersInfo.size(); i++)
    {
        if (layerName == m_layersInfo[i].name)
        {
            return i;
        }
    }
    return -1;
}

QModelIndex LayerModel::IndexForLayer(const QString& layerName) const
{
    int row = RowForLayer(layerName);
    return row == -1 ? QModelIndex() : index(row, 2);
}

void LayerModel::HandleDataChange(int row, int column, const QVector<int> roles, bool recursive)
{
    Q_ASSERT(row < m_layersInfo.size());
    Q_ASSERT(column < columnCount());
    int startRow = row;
    int endRow = startRow + 1;
    if (recursive)
    {
        const int indent = m_layersInfo[row].indent;
        while (endRow < rowCount() && m_layersInfo[endRow].indent > indent)
        {
            ++endRow;
        }
    }
    Q_EMIT dataChanged(index(startRow, column), index(endRow - 1, column));
}

void LayerModel::InvalidateFileAttributes()
{
    Q_EMIT dataChanged(index(0, DetailsColumn), index(rowCount() - 1, DetailsColumn), { SourceControlRole });
}


void LayerModel::SetLayerInfo(int row, const CLayersListBox::SLayerInfo& info)
{
    Q_ASSERT(row < m_layersInfo.size() && row >= 0);

    m_layersInfo[row] = info;
    Q_EMIT dataChanged(index(row, 0), index(row, columnCount() - 1));
}

Qt::ItemFlags LayerModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return Qt::NoItemFlags;
    }

    const CLayersListBox::SLayerInfo& layer = m_layersInfo.at(index.row());
    Qt::ItemFlags flags(Qt::NoItemFlags);
    if (index.column() == DetailsColumn)
    {
        //make items selectable otherwise we can't select with mouse or using QItemSelectionModel
        flags |= Qt::ItemIsSelectable;

        // User can't select frozen layer
        if (gSettings.frozenSelectable || !layer.pLayer->IsFrozen())
        {
            flags |= Qt::ItemIsEnabled;
        }
    }
    if (m_draggingItemNo != -1 && !layer.pLayer->IsChildOf(m_layersInfo.at(m_draggingItemNo).pLayer))
    {
        flags |= Qt::ItemIsDropEnabled;
    }
    if (index.row() == m_draggingItemNo && layer.usable)
    {
        flags |= Qt::ItemIsDragEnabled;
    }
    return flags;
}

void LayerModel::SetDraggingItem(int layerNo)
{
    m_draggingItemNo = layerNo;
}

class LayerDelegate
    : public QStyledItemDelegate
{
public:
    LayerDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent)
    {
        m_imageList = {
            QPixmap(":/LayerEditor/LayerButtons/visible.png"),
            QPixmap(":/LayerEditor/LayerButtons/usable.png"),
            QPixmap(":/LayerEditor/LayerButtons/expanded.png"),
            QPixmap(":/LayerEditor/LayerButtons/collapsed.png"),
            QPixmap(":/LayerEditor/LayerButtons/leaf.png"),
            QPixmap(":/LayerEditor/LayerButtons/not_exportable.png"),
            QPixmap(":/LayerEditor/LayerButtons/blank.png"),
        };
        m_sourceControlImageList = {
            QPixmap(":/LayerEditor/SourceControlIcons/managed.png"),
            QPixmap(":/LayerEditor/SourceControlIcons/not_managed.png"),
            QPixmap(":/LayerEditor/SourceControlIcons/checked_out_by_me.png"),
            QPixmap(":/LayerEditor/SourceControlIcons/not_at_head.png"),
            QPixmap(":/LayerEditor/SourceControlIcons/added.png"),
            QPixmap(":/LayerEditor/SourceControlIcons/unknown.png"),
            QPixmap(":/LayerEditor/SourceControlIcons/checked_out_by_other.png"),
            QPixmap(":/LayerEditor/SourceControlIcons/locked.png")
        };
    }

    static int CalculateExpandButtonLeftPos(int indentAmount)
    {
        return 2 + indentAmount * INDENT_SIZE;
    }

    static QRect CalculateExpandButtonRect(int indentAmount, int rowHeight)
    {
        return QRect(CalculateExpandButtonLeftPos(indentAmount), (rowHeight - IMAGE_LIST_ICON_SIZE) / 2, IMAGE_LIST_ICON_SIZE, IMAGE_LIST_ICON_SIZE);
    }

    QPoint GetButtonCoordinate(int startX, const QRect& optionRect, const QPixmap& pixmap) const
    {
        return QPoint(startX, optionRect.top() + (optionRect.height() - pixmap.height()) / 2);
    }

    static void DrawLayerButton(QPainter* painter, const QRect& rect, const QPalette& palette, const QPixmap& icon, bool bEnabled)
    {
        painter->setPen(palette.dark().color());
        const QSize boxSize(IMAGE_LIST_ICON_SIZE + 2 * buttonPadding, IMAGE_LIST_ICON_SIZE + 2 * buttonPadding);
        const int left = rect.left() + (rect.width() - boxSize.width()) / 2;
        const int top = rect.top() + (rect.height() - boxSize.height()) / 2;
        painter->drawRect(left, top, boxSize.width(), boxSize.height());
        if (bEnabled)
        {
            painter->drawPixmap(left + buttonPadding, top + buttonPadding, icon);
        }
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        if (index.column() == LayerModel::VisibleColumn)
        {
            DrawLayerButton(painter, option.rect, option.palette, m_imageList[ITEM_VISIBLE_BITMAP], index.data(LayerModel::VisbleRole).toBool());
        }
        else if (index.column() == LayerModel::UsableColumn)
        {
            DrawLayerButton(painter, option.rect, option.palette, m_imageList[ITEM_USABLE_BITMAP], index.data(LayerModel::UsableRole).toBool());
        }
        else if (index.column() == LayerModel::DetailsColumn)
        {
            const bool bSelected = (option.state & QStyle::State_Selected);

            QColor bgColor;
            if (bSelected)
            {
                bgColor = option.palette.highlight().color();
            }
            else
            {
                QVariant background = index.data(Qt::BackgroundRole);
                if (!background.isNull())
                {
                    bgColor = qvariant_cast<QColor>(background);
                }
                else
                {
                    bgColor = option.palette.window().color();
                }
            }

            //fill in background
            painter->fillRect(option.rect, bgColor);

            int textX = option.rect.left() + CalculateExpandButtonLeftPos(index.data(LayerModel::IndentRole).toInt());

            if (index.data(LayerModel::HasChildrenRole).toBool())
            {
                const QPoint expandPosition = GetButtonCoordinate(textX, option.rect, m_imageList[ITEM_EXPANDED_BITMAP]);
                // Draw Expand icon.
                if (index.data(LayerModel::ExpandedRole).toBool())
                {
                    painter->drawPixmap(expandPosition, m_imageList[ITEM_EXPANDED_BITMAP]);
                }
                else
                {
                    painter->drawPixmap(expandPosition, m_imageList[ITEM_COLLAPSED_BITMAP]);
                }
            }
            textX += IMAGE_LIST_ICON_SIZE + 2;

            QFont f = painter->font();
            f.setUnderline(index.data(LayerModel::ExternalRole).toBool());
            painter->setFont(f);

            //draw the leaf node pixmap
            const QPoint leafPixmapPosition = GetButtonCoordinate(textX, option.rect, m_imageList[ITEM_LEAF_BITMAP]);
            if (index.data(LayerModel::ExportableRole).toBool())
            {
                painter->drawPixmap(leafPixmapPosition, m_imageList[ITEM_LEAF_BITMAP]);
            }
            else
            {
                painter->drawPixmap(leafPixmapPosition, m_imageList[ITEM_LEAF_NOTEXPORTABLE_BITMAP]);
            }

            //draw some source control information on the layers if its available
            const QVariant sourceControlAttribs = index.data(LayerModel::SourceControlRole);
            if (sourceControlAttribs.isValid())
            {
                quint32 eFileAttribs = sourceControlAttribs.toUInt();

                //if the file is invalid draw an ?, meaning this file does not exist
                if (eFileAttribs == SCC_FILE_ATTRIBUTE_INVALID)
                {
                    painter->drawPixmap(leafPixmapPosition, m_sourceControlImageList[ITEM_SOURCE_CONTROL_NOFILE]);
                }
                else
                {
                    //if the file is under source control draw a green dot, other wise draw a red dot
                    if (eFileAttribs & SCC_FILE_ATTRIBUTE_MANAGED)
                    {
                        painter->drawPixmap(leafPixmapPosition, m_sourceControlImageList[ITEM_SOURCE_CONTROL_MANAGED]);
                    }
                    else
                    {
                        //Not managed by source control
                        painter->drawPixmap(leafPixmapPosition, m_sourceControlImageList[ITEM_SOURCE_CONTROL_NOTMANAGED]);
                    }

                    //if the file is checked out by us draw a red check
                    if (eFileAttribs & SCC_FILE_ATTRIBUTE_CHECKEDOUT)
                    {
                        painter->drawPixmap(leafPixmapPosition, m_sourceControlImageList[ITEM_SOURCE_CONTROL_CHECKEDOUTBY_ME]);
                    }

                    //if the file is checked out to someone else draw a blue check
                    if (eFileAttribs & SCC_FILE_ATTRIBUTE_BYANOTHER)
                    {
                        painter->drawPixmap(leafPixmapPosition, m_sourceControlImageList[ITEM_SOURCE_CONTROL_CHECKEDOUTBY_OTHER]);
                    }

                    //if the file is locked by someone else draw a lock icon
                    if (eFileAttribs & SCC_FILE_ATTRIBUTE_LOCKEDBYANOTHER)
                    {
                        painter->drawPixmap(leafPixmapPosition, m_sourceControlImageList[ITEM_SOURCE_CONTROL_LOCKED]);
                    }

                    //if the file is not the head version by someone else draw a yellow triangle
                    if (eFileAttribs & SCC_FILE_ATTRIBUTE_NOTATHEAD)
                    {
                        painter->drawPixmap(leafPixmapPosition, m_sourceControlImageList[ITEM_SOURCE_CONTROL_NOTATHEAD]);
                    }
                }
            }

            // Make text color readable
            QColor textColor = bSelected ? option.palette.highlightedText().color() : option.palette.text().color();
            if (IsNearColors(textColor, bgColor))
            {
                textColor = QColor(255, 255, 255);
                if (IsNearColors(textColor, bgColor))
                {
                    textColor = QColor(0, 0, 0);
                }
            }
            painter->setPen(textColor);
            textX += IMAGE_LIST_ICON_SIZE + 4;
            const QRect textRect(textX, option.rect.top(), option.rect.right() - textX, option.rect.height());
            painter->drawText(textRect, index.data(Qt::DisplayRole).toString(), QTextOption(Qt::AlignVCenter));
        }

        // Draw a separator line
        painter->setPen(option.palette.dark().color());
        painter->drawLine(option.rect.left(), option.rect.bottom(), option.rect.right(), option.rect.bottom());
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        switch (index.column())
        {
        case LayerModel::VisibleColumn:
            return QSize(IMAGE_LIST_ICON_SIZE + 4, IMAGE_LIST_ICON_SIZE + 4);
        case LayerModel::UsableColumn:
            return QSize(IMAGE_LIST_ICON_SIZE + 4, IMAGE_LIST_ICON_SIZE + 4);
        case LayerModel::DetailsColumn:
            return QSize(160, option.fontMetrics.height() + 10);
        }
        return QSize();
    }

private:
    QVector<QPixmap> m_imageList;
    QVector<QPixmap> m_sourceControlImageList;
    QPixmap m_visibleUncheckedPixmap;
    QPixmap m_visibleCheckedPixmap;
    QPixmap m_usableUncheckedPixmap;
    QPixmap m_usableCheckedPixmap;
};


//////////////////////////////////////////////////////////////////////////
CLayersListBox::SCacheLayerAttributes::SCacheLayerAttributes()
{
    time = AZStd::GetTimeNowTicks();
}

CLayersListBox::CLayersListBox(QWidget* parent)
    : QTreeView(parent)
    , m_mutex(QMutex::Recursive)
{
    qRegisterMetaType<CLayersListBox::SCacheLayerAttributes*>("SCacheLayerAttributes*");

    QObject::connect(&m_timer, &QTimer::timeout, this, &CLayersListBox::OnTimerCompleted);
    m_timer.setInterval(1000);
    m_timer.start();

    setIndentation(2);

    m_model = new LayerModel(m_mutex, this);
    setModel(m_model);

    LayerDelegate* delegate = new LayerDelegate(this);
    setItemDelegate(delegate);
    resizeColumnToContents(0);
    resizeColumnToContents(1);

    header()->hide();

    setMouseTracking(true);

    m_dragCursorBitmap = QPixmap(":/LayerEditor/LayerButtons/added.png");

    setDragEnabled(true);
    setDragDropMode(InternalMove);

    {
        using SCRequest = AzToolsFramework::SourceControlConnectionRequestBus;
        SCRequest::BroadcastResult(m_SCState, &SCRequest::Events::GetSourceControlState);
    }

    m_workerThread = AZStd::thread(AZStd::bind(&CLayersListBox::ThreadWorker, this));
}

//////////////////////////////////////////////////////////////////////////
CLayersListBox::~CLayersListBox()
{
    m_timer.stop();
    m_shutdownThread = true;
    m_workerSemaphore.release();
    m_workerThread.join();
}

void CLayersListBox::InvalidateListBoxFileAttributes()
{
    m_model->InvalidateFileAttributes();
}

bool CLayersListBox::isPointInExpandButton(const QPoint& point, const QModelIndex& index) const
{
    if (index.column() == LayerModel::DetailsColumn && index.data(LayerModel::HasChildrenRole).toBool())
    {
        const QRect vRect = visualRect(index);
        QRect expandRect = LayerDelegate::CalculateExpandButtonRect(index.data(LayerModel::IndentRole).toInt(), vRect.height());
        expandRect.moveTopLeft(vRect.topLeft() + expandRect.topLeft());
        return expandRect.contains(point);
    }
    return false;
}

void CLayersListBox::UpdateLayersFromConnection()
{
    QMutexLocker locker(&m_mutex);
    bool invalidateLayerAttribs = m_invalidateLayerAttribs;
    bool refreshAllLayers = m_refreshAllLayers;

    m_invalidateLayerAttribs = false;
    m_refreshAllLayers = false;

    if (invalidateLayerAttribs || refreshAllLayers)
    {
        for (auto iter = m_layersAttributeMap.begin(); iter != m_layersAttributeMap.end(); ++iter)
        {
            if (invalidateLayerAttribs)
            {
                iter->second.fileAttributes = ESccFileAttributes::SCC_FILE_ATTRIBUTE_INVALID;
            }

            if (refreshAllLayers)
            {
                iter->second.isLayerNew = true;
            }
        }

        if (invalidateLayerAttribs)
        {
            InvalidateListBoxFileAttributes();
        }
    }
}

void CLayersListBox::QueueLayersForUpdate()
{
    QMutexLocker locker(&m_mutex);

    AZStd::sys_time_t timeNow = AZStd::GetTimeNowTicks();
    AZStd::sys_time_t ticksPerMin = AZStd::GetTimeTicksPerSecond() * 60;

    for (auto iter = m_layersAttributeMap.begin(); iter != m_layersAttributeMap.end(); ++iter)
    {
        if (iter->second.queuedRefresh)
        {
            continue;
        }

        if (iter->second.isLayerNew)
        {
            StartFileAttributeUpdateJob(iter->first, iter->second);
            continue;
        }

        if (timeNow > (iter->second.time + ticksPerMin))
        {
            StartFileAttributeUpdateJob(iter->first, iter->second);
        }
    }
}

// CLayersListBox message handlers
void CLayersListBox::mousePressEvent(QMouseEvent* event)
{
    event->ignore();
    if (event->button() == Qt::LeftButton)
    {
        OnLButtonDown(event);
    }
    else if (event->button() == Qt::RightButton)
    {
        OnRButtonDown(event);
    }

    if (!event->isAccepted())
    {
        QTreeView::mousePressEvent(event);
    }
}

void CLayersListBox::mouseReleaseEvent(QMouseEvent* event)
{
    event->ignore();
    if (event->button() == Qt::LeftButton)
    {
        OnLButtonUp(event);
    }
    if (event->button() == Qt::RightButton)
    {
        OnRButtonUp(event);
    }

    if (!event->isAccepted())
    {
        QTreeView::mouseReleaseEvent(event);
    }
}

/////////////////////////////////////////////////////////////////////////
void CLayersListBox::OnLButtonDown(QMouseEvent* event)
{
    // Find item where we clicked.
    QModelIndex index = indexAt(event->pos());
    if (index.isValid())
    {
        const int item = index.row();
        CObjectLayer* pLayer = m_layersInfo[item].pLayer;

        if (index.column() == LayerModel::VisibleColumn || index.column() == LayerModel::UsableColumn)
        {
            if (index.column() == LayerModel::VisibleColumn)
            {
                if (event->modifiers() & Qt::AltModifier)
                {
                    CUndo undo("Isolate Layer");
                    GetIEditor()->GetObjectManager()->GetLayersManager()->Isolate(pLayer);
                    event->accept();
                    return;
                }

                ToggleLayerStates(item, LayerToggleFlagVisibility);
            }
            else
            {
                ToggleLayerStates(item, LayerToggleFlagUsability);
            }
            CViewport* pViewport = GetIEditor()->GetActiveView();
            if (pViewport)
            {
                // Call UpdateViews two times to update visible list first
                pViewport->Update();
                pViewport->Update();
            }
            event->accept();
            return;
        }
        else if (index.column() == LayerModel::DetailsColumn)
        {
            if (isPointInExpandButton(event->pos(), index))
            {
                pLayer->Expand(!pLayer->IsExpanded());
                ReloadLayers();
                event->accept();
                return;
            }
        }

        if (!(index.flags() & Qt::ItemIsEnabled))
        {
            event->accept();
            return;
        }

        m_lclickedItem = item;

        if (event->modifiers() & Qt::AltModifier)
        {
            CUndo undo("Isolate Layer");
            GetIEditor()->GetObjectManager()->GetLayersManager()->SetCurrentLayer(pLayer);
            GetIEditor()->GetObjectManager()->GetLayersManager()->Isolate(pLayer);
        }

        if (event->modifiers() & Qt::ControlModifier)
        {
            // Can only drag removable layers.
            if (GetIEditor()->GetObjectManager()->GetLayersManager()->CanDeleteLayer(pLayer))
            {
                // Start dragging.
                m_draggingItem = item;
                m_model->SetDraggingItem(m_draggingItem);
            }
        }
    }
}

void CLayersListBox::OnLButtonUp(QMouseEvent* event)
{
    if (m_draggingItem >= 0)
    {
        unsetCursor();
        // Find item where we clicked.
        QModelIndex index = indexAt(event->pos());
        if (index.isValid())
        {
            const int item = index.row();
            CObjectLayer* pLayer = m_layersInfo[item].pLayer;
            if (item != m_draggingItem)
            {
                CObjectLayer* pSrcLayer = m_layersInfo[m_draggingItem].pLayer;
                CObjectLayer* pTrgLayer = m_layersInfo[item].pLayer;
                if (!pTrgLayer->IsChildOf(pSrcLayer))
                {
                    pTrgLayer->AddChild(pSrcLayer);
                    pTrgLayer->Expand(true);
                    ReloadLayers();
                }
                m_draggingItem = -1;
            }
        }
        else
        {
            CObjectLayer* pSrcLayer = m_layersInfo[m_draggingItem].pLayer;
            if (pSrcLayer->GetParent())
            {
                pSrcLayer->GetParent()->RemoveChild(pSrcLayer);
                ReloadLayers();
            }
        }
        m_draggingItem = -1;
        event->accept();
    }
    m_model->SetDraggingItem(m_draggingItem);

    if (m_lclickedItem != -1)
    {
        emit itemSelected();
        m_lclickedItem = -1;
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayersListBox::mouseDoubleClickEvent(QMouseEvent* event)
{
    // Find item where we clicked.
    QModelIndex index = indexAt(event->pos());
    const int item = index.row();
    if (index.isValid())
    {
        if (index.column() == LayerModel::DetailsColumn)
        {
            // Toggle expand status.
            m_layersInfo[item].pLayer->Expand(!m_layersInfo[item].pLayer->IsExpanded());
            ReloadLayers();
        }
        else if (index.column() == LayerModel::VisibleColumn && gSettings.bLayerDoubleClicking)
        {
            CUndo undo("Layer Modify");
            bool bValue = m_layersInfo[item].visible;
            // Set all layers to this value.
            std::vector<CObjectLayer*> layers;
            GetIEditor()->GetObjectManager()->GetLayersManager()->GetLayers(layers);
            for (int i = 0; i < layers.size(); i++)
            {
                layers[i]->SetVisible(bValue, !layers[i]->IsExpanded());
                //layers[i]->SetModified();
            }
            ReloadLayers();
        }
        else if (index.column() == LayerModel::UsableColumn && gSettings.bLayerDoubleClicking)
        {
            CUndo undo("Layer Modify");
            bool bUsable = m_layersInfo[item].usable;
            // Set all layers to this value.
            std::vector<CObjectLayer*> layers;
            GetIEditor()->GetObjectManager()->GetLayersManager()->GetLayers(layers);
            for (int i = 0; i < layers.size(); i++)
            {
                layers[i]->SetFrozen(!bUsable, !layers[i]->IsExpanded());
                //layers[i]->SetModified();
            }
            ReloadLayers();
        }
    }
    QTreeView::mouseDoubleClickEvent(event);
}

//////////////////////////////////////////////////////////////////////////
void CLayersListBox::SetUpdateCallback(UpdateCallback& cb)
{
    m_updateCalback = cb;
}

void CLayersListBox::mouseMoveEvent(QMouseEvent* event)
{
    const QModelIndex index = indexAt(event->pos());
    if (m_draggingItem >= 0)
    {
        if (index.flags() & Qt::ItemIsDropEnabled)
        {
            setCursor(m_dragCursorBitmap);
        }
        else
        {
            setCursor(Qt::ForbiddenCursor);
        }
        return;
    }

    const bool mouseOnButton = (index.column() == LayerModel::VisibleColumn ||
                                index.column() == LayerModel::UsableColumn ||
                                isPointInExpandButton(event->pos(), index));

    if (!mouseOnButton)
    {
        setCursor(Qt::PointingHandCursor);
    }
    else
    {
        unsetCursor();
    }

    QTreeView::mouseMoveEvent(event);
}

//////////////////////////////////////////////////////////////////////////
void CLayersListBox::SelectLayer(const QString& layerName)
{
    SelectLayer(m_model->IndexForLayer(layerName));
}

void CLayersListBox::SelectLayer(const QModelIndex& index)
{
    if (index.isValid())
    {
        selectionModel()->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }
    else
    {
        selectionModel()->clear();
    }
}

inline bool CompareLayers(CObjectLayer* l1, CObjectLayer* l2)
{
    return QString::compare(l1->GetName(), l2->GetName(), Qt::CaseInsensitive) < 0;
}

//////////////////////////////////////////////////////////////////////////
void CLayersListBox::ReloadLayers()
{
    if (m_noReload)
    {
        return;
    }

    m_layersInfo.clear();

    CObjectLayerManager* pLayerManager = GetIEditor()->GetObjectManager()->GetLayersManager();
    std::vector<CObjectLayer*> layers;
    pLayerManager->GetLayers(layers);

    std::sort(layers.begin(), layers.end(), CompareLayers);

    for (int i = 0; i < layers.size(); i++)
    {
        CObjectLayer* pLayer = layers[i];
        if (pLayer->GetParent())
        {
            continue;
        }

        AddLayerRecursively(pLayer, 0);
    }

    //Sync Cache after Layer Info vector is repopulated
    SyncAttributeMap();

    m_model->SetLayers(m_layersInfo);


    ReloadListItems();
}

//////////////////////////////////////////////////////////////////////////
void CLayersListBox::AddLayerRecursively(CObjectLayer* pLayer, int level)
{
    CLayersListBox::SLayerInfo layerInfo;
    layerInfo.name = pLayer->GetName();
    layerInfo.visible = pLayer->IsVisible();
    layerInfo.usable = !pLayer->IsFrozen();
    layerInfo.pLayer = pLayer;
    layerInfo.indent = level;
    layerInfo.childCount = pLayer->GetChildCount();
    layerInfo.expanded = pLayer->IsExpanded();
    layerInfo.isModified = pLayer->IsModified();
    m_layersInfo.push_back(layerInfo);

    if (pLayer->IsExpanded() && pLayer->GetChildCount())
    {
        int numLayers = pLayer->GetChildCount();

        std::vector<CObjectLayer*> layers;
        for (int i = 0; i < numLayers; i++)
        {
            layers.push_back(pLayer->GetChild(i));
        }
        std::sort(layers.begin(), layers.end(), CompareLayers);

        for (int i = 0; i < numLayers; i++)
        {
            AddLayerRecursively(layers[i], level + 1);

            if (i == numLayers - 1)
            {
                m_layersInfo[m_layersInfo.size() - 1].lastchild = true;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayersListBox::ReloadListItems()
{
    CObjectLayerManager* pLayerManager = GetIEditor()->GetObjectManager()->GetLayersManager();
    if (!pLayerManager->GetCurrentLayer())
    {
        return;
    }
    QString selectedLayerName = pLayerManager->GetCurrentLayer()->GetName();

    SelectLayer(selectedLayerName);
}

//////////////////////////////////////////////////////////////////////////
void CLayersListBox::OnModifyLayer(int index)
{
    assert(index >= 0 && index < m_layersInfo.size());

    CUndo undo("Layer Modify");
    m_noReload = true;
    SLayerInfo& li = m_layersInfo[index];
    bool bRecursive = !(li.pLayer->IsExpanded());
    li.pLayer->SetVisible(li.visible, bRecursive);
    li.pLayer->SetFrozen(!li.usable, bRecursive);
    m_noReload = false;

    m_model->SetLayerInfo(index, li);
}

//////////////////////////////////////////////////////////////////////////
void CLayersListBox::OnRButtonDown(QMouseEvent* event)
{
    m_rclickedItem = -1;
    QModelIndex index = indexAt(event->pos());
    const int item = index.row();
    if (index.isValid())
    {
        m_rclickedItem = item;
        SelectLayer(index);
        CObjectLayer* pLayer = GetCurrentLayer();
        if (pLayer)
        {
            CUndo undo("Set Current Layer");
            GetIEditor()->GetObjectManager()->GetLayersManager()->SetCurrentLayer(pLayer);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayersListBox::OnRButtonUp(QMouseEvent* event)
{
    QModelIndex index = indexAt(event->pos());
    const int item = index.row();
    if (index.isValid())
    {
        if (item == m_rclickedItem)
        {
            Q_EMIT contextMenuRequested(m_rclickedItem);
        }
    }
    event->accept();
    m_rclickedItem = -1;
}

//////////////////////////////////////////////////////////////////////////
CObjectLayer* CLayersListBox::GetCurrentLayer()
{
    const QModelIndexList selectedRows = selectionModel()->selectedRows(0);
    return selectedRows.empty() ? 0 : m_layersInfo[selectedRows.first().row()].pLayer;
}

//////////////////////////////////////////////////////////////////////////
void CLayersListBox::ExpandAll(bool isExpand)
{
    std::vector<CObjectLayer*> layers;
    GetIEditor()->GetObjectManager()->GetLayersManager()->GetLayers(layers);

    for (int i = 0; i < layers.size(); i++)
    {
        if (layers[i])
        {
            layers[i]->Expand(isExpand);
        }
    }

    ReloadLayers();
}

void CLayersListBox::StartFileAttributeUpdateJob(QString layerName, SCacheLayerAttributes& attribute)
{
    AZStd::lock_guard<AZStd::mutex> locker(m_layerRequestMutex);
    LayerAttribRequest request(layerName);

    CGameEngine* pEngine = GetIEditor() ? GetIEditor()->GetGameEngine() : nullptr;
    if (pEngine && attribute.pLayer)
    {
        if (attribute.pLayer->IsExternal())
        {
            request.layerPath = attribute.pLayer->GetExternalLayerPath().toUtf8().data();
        }
        else
        {
            request.layerPath = AZStd::string::format("%s\\%s%s", pEngine->GetLevelPath().toUtf8().data(), pEngine->GetLevelName().toUtf8().data(), pEngine->GetLevelExtension().toUtf8().data());
        }

        char resolvedPath[AZ_MAX_PATH_LEN] = { 0 };
        AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(request.layerPath.c_str(), resolvedPath, AZ_MAX_PATH_LEN);
        request.layerPath = resolvedPath;

        m_layerRequestQueue.push(request);
        attribute.queuedRefresh = true;
        m_workerSemaphore.release();
    }
}

void CLayersListBox::OnTimerCompleted()
{
    if (m_layersAttributeMap.empty())
    {
        return;
    }

    // Drain results queue
    LayerAttribResult attribs;
    while (1)
    {
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_layerResultMutex);
            if (m_layerResultQueue.empty())
            {
                break;
            }

            attribs = AZStd::move(m_layerResultQueue.front());
            m_layerResultQueue.pop();
        }
        UpdateLayerAttribute(attribs.layerName, attribs.fileAttributes);
    }

    UpdateLayersFromConnection();
    if (m_SCState == AzToolsFramework::SourceControlState::Active)
    {
        QueueLayersForUpdate();
    }
}

void CLayersListBox::SyncAttributeMap()
{
    QMutexLocker locker(&m_mutex);
    LayersAttributeMap tempMap;
    SCacheLayerAttributes layerAttributes;

    if (m_layersInfo.empty())
    {
        return;
    }
    tempMap.swap(m_layersAttributeMap);

    for (int idx = 0; idx < m_layersInfo.size(); idx++)
    {
        CObjectLayer* pLayer = m_layersInfo[idx].pLayer;
        const QString layerName = pLayer->GetName();
        QString nameToCheck;

        if (m_layersAttributeMap.find(layerName) == m_layersAttributeMap.end())
        {
            //try to find that layer by name
            auto itr = tempMap.find(layerName);
            if (itr != tempMap.end())
            {
                //Found-Copy the previous entry
                m_layersAttributeMap[layerName] = itr->second;
            }
            else
            {
                //Not Found-Add a new entry
                layerAttributes.name = layerName;
                layerAttributes.isLayerNew = true;
                layerAttributes.fileAttributes = ESccFileAttributes::SCC_FILE_ATTRIBUTE_INVALID;
                layerAttributes.time = AZStd::GetTimeNowTicks();
                layerAttributes.pLayer = pLayer;
                m_layersAttributeMap[layerName] = layerAttributes;
            }
        }
    }

    m_model->UpdateAttributeMap(m_layersAttributeMap);
}

void CLayersListBox::UpdateLayerAttribute(QString layerName, uint32 fileAttributes)
{
    QMutexLocker locker(&m_mutex);
    auto iter = m_layersAttributeMap.find(layerName);

    uint32 currentAttrib = ESccFileAttributes::SCC_FILE_ATTRIBUTE_INVALID;
    if (m_SCState == AzToolsFramework::SourceControlState::Active)
    {
        currentAttrib = fileAttributes;
    }

    bool invalidate = false;
    if (iter != m_layersAttributeMap.end())
    {
        //Update CacheData
        iter->second.time = AZStd::GetTimeNowTicks();
        iter->second.isLayerNew = false;
        iter->second.queuedRefresh = false;
        if (iter->second.fileAttributes != currentAttrib)
        {
            iter->second.fileAttributes = currentAttrib;
            invalidate = true;
        }
    }

    if (invalidate)
    {
        //unlock mutex because for now model shares mutex
        m_model->UpdateAttributeMap(m_layersAttributeMap);
        locker.unlock();
        const int row = m_model->RowForLayer(layerName);
        m_model->HandleDataChange(row, LayerModel::DetailsColumn, { LayerModel::SourceControlRole }, false);
    }
}

void CLayersListBox::ConnectivityStateChanged(const AzToolsFramework::SourceControlState state)
{
    if (state != m_SCState)
    {
        if (state == AzToolsFramework::SourceControlState::Active)
        {
            m_refreshAllLayers = true;
        }
        else
        {
            m_invalidateLayerAttribs = true;
        }

        m_SCState = state;
    }
}

void CLayersListBox::ToggleLayerStates(int layerIndex, LayerToggleFlag toggleFlag)
{
    if (toggleFlag == LayerToggleFlagNone || layerIndex < 0 || m_layersInfo.size() <= layerIndex)
    {
        return;
    }

    SLayerInfo* layerInfo = &m_layersInfo[layerIndex];

    bool updateVisibility = toggleFlag & LayerToggleFlagVisibility;
    bool targetVisibleState = !layerInfo->visible;

    bool updateUsability = toggleFlag & LayerToggleFlagUsability;
    bool targetUsableState = !layerInfo->usable;


    for (int count = 1, i = 0; i < count; i++)
    {
        int index = layerIndex + i;
        layerInfo = &m_layersInfo[index];

        if (updateVisibility)
        {
            layerInfo->visible = targetVisibleState;
        }
        if (updateUsability)
        {
            layerInfo->usable = targetUsableState;
        }
        OnModifyLayer(index);

        //Check if this layer has any child layers and step through them if needed.
        if (_smart_ptr<CObjectLayer> layer = layerInfo->pLayer)
        {
            int childCount = layer->GetChildCount();
            if (childCount > 0 && layerInfo->expanded)
            {
                count += childCount;
                count = min(count, (int)m_layersInfo.size());       //bounds clamp
            }
        }
    }
}

void CLayersListBox::ThreadWorker()
{
    while (1)
    {
        m_workerSemaphore.acquire();
        if (m_shutdownThread)
        {
            break;
        }

        LayerAttribRequest request;
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_layerRequestMutex);
            if (m_layerRequestQueue.empty())
            {
                continue;
            }

            request = m_layerRequestQueue.front();
            m_layerRequestQueue.pop();
        }

        uint32 attrib = CFileUtil::GetAttributes(request.layerPath.c_str());
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_layerResultMutex);
            m_layerResultQueue.push(LayerAttribResult(request.layerName, attrib));
        }
    }
}

int CLayersListBox::GetCount() const
{
    return m_model->rowCount();
}

#include <Controls/LayersListBox.moc>

