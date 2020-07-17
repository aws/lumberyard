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
#include "TerrainTexture.h"
#include "CryEditDoc.h"
#include "DimensionsDialog.h"
#include "Terrain/SurfaceType.h"
#include "Terrain/Layer.h"
#include "Terrain/TerrainTexGen.h"
#include "Terrain/TerrainManager.h"
#include "Material/MaterialManager.h"
#include "QtViewPaneManager.h"
#include "Util/AutoDirectoryRestoreFileDialog.h"

#include <Terrain/Bus/LegacyTerrainBus.h>

#include "QtUtilWin.h"
#include "QtUI/ClickableLabel.h"

#include <QAbstractTableModel>
#include <QDesktopServices>
#include <QItemSelectionModel>
#include <QInputDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QMouseEvent>
#include <QDebug>
#include <AzCore/Casting/numeric_cast.h>

#include <ui_TerrainTexture.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzFramework/Physics/Material.h>

#include <AzCore/RTTI/BehaviorContext.h>

enum Columns
{
    ColumnLayerName = 0,
    ColumnMaterial,
    ColumnSplatMap,
    ColumnMinHeight,
    ColumnMaxHeight,
    ColumnMinAngle,
    ColumnMaxAngle,
    ColumnBrightness,
    ColumnBaseTiling,
    ColumnSortOrder,
    ColumnSpecularAmount,
    ColumnUseRemesh,
    ColumnCount
};

static const int TEXTURE_PREVIEW_SIZE = 64;

Q_DECLARE_METATYPE(CLayer*);

static QPixmap generatePreview(CLayer* layer, QSize size)
{
    CImageEx& preview = layer->GetTexturePreviewImage();
    if (!preview.IsValid())
    {
        QPixmap unavailable(size);
        QPainter painter(&unavailable);
        painter.fillRect(unavailable.rect(), QColor(255, 0, 0));
        painter.drawText(unavailable.rect(), "Preview Unavailable", Qt::AlignHCenter | Qt::AlignVCenter);

        return unavailable;
    }

    QImage image(reinterpret_cast<const quint8*>(preview.GetData()), preview.GetWidth(), preview.GetHeight(), QImage::Format::Format_ARGB32);
    return QPixmap::fromImage(image.rgbSwapped()).scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}


/////////////////////////////////////////////////////////////////////////////
// CTerrainTextureDialog dialog


//////////////////////////////////////////////////////////////////////////
class CTerrainLayersUndoObject
    : public IUndoObject
{
public:
    CTerrainLayersUndoObject()
    {
        m_undo.root = GetISystem()->CreateXmlNode("Undo");
        m_redo.root = GetISystem()->CreateXmlNode("Redo");
        m_undo.bLoading = false;
        GetIEditor()->GetTerrainManager()->SerializeLayerSettings(m_undo);
    }
protected:
    virtual int GetSize() { return sizeof(*this); }
    virtual QString GetDescription() { return "Terrain Layers"; };

    virtual void Undo(bool bUndo)
    {
        if (bUndo)
        {
            m_redo.bLoading = false; // save redo
            GetIEditor()->GetTerrainManager()->SerializeLayerSettings(m_redo);
        }
        m_undo.bLoading = true; // load undo
        GetIEditor()->GetTerrainManager()->SerializeLayerSettings(m_undo);

        UpdateTerrainTextureView();
        GetIEditor()->Notify(eNotify_OnInvalidateControls);
    }
    virtual void Redo()
    {
        m_redo.bLoading = true; // load redo
        GetIEditor()->GetTerrainManager()->SerializeLayerSettings(m_redo);

        UpdateTerrainTextureView();
        GetIEditor()->Notify(eNotify_OnInvalidateControls);
    }
    void UpdateTerrainTextureView()
    {
        QtViewPane* pane = QtViewPaneManager::instance()->GetPane(LyViewPane::TerrainTextureLayers);
        if (!pane)
        {
            return;
        }

        CTerrainTextureDialog* dialog = qobject_cast<CTerrainTextureDialog*>(pane->Widget());
        if (!dialog || dialog->isHidden())
        {
            return;
        }

        dialog->OnUndoUpdate();
    }

private:
    CXmlArchive m_undo;
    CXmlArchive m_redo;
};
//////////////////////////////////////////////////////////////////////////

class TerrainTextureLayerEditModel
    : public QAbstractTableModel
{
public:
    TerrainTextureLayerEditModel(QObject* parent)
        : QAbstractTableModel(parent)
    {}

    virtual ~TerrainTextureLayerEditModel()
    {}

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return m_layers.size();
    }

    int columnCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return ColumnCount;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        {
            return {};
        }

        switch (section)
        {
        case ColumnLayerName:
            return "Layer";
        case ColumnMaterial:
            return "Material";
        case ColumnSplatMap:
            return "Splat Map Path";
        case ColumnMinHeight:
            return "Min Height";
        case ColumnMaxHeight:
            return "Max Height";
        case ColumnMinAngle:
            return "Min Angle";
        case ColumnMaxAngle:
            return "Max Angle";
        case ColumnBrightness:
            return "Brightness";
        case ColumnBaseTiling:
            return "Base Tiling (test)";
        case ColumnSortOrder:
            return "Sort Order (test)";
        case ColumnSpecularAmount:
            return "Specular Amount (test)";
        case ColumnUseRemesh:
            return "UseRemeshing";
        default:
            return {};
        }
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (index.row() < 0 || index.row() >= m_layers.size())
        {
            return {};
        }

        CLayer* layer = m_layers[index.row()];

        switch (role)
        {
        case Qt::DisplayRole:
        case Qt::EditRole:
            switch (index.column())
            {
            case ColumnLayerName:
                return layer->GetLayerName();
            case ColumnMaterial:
            {
                CSurfaceType* pSurfaceType = layer->GetSurfaceType();
                if (pSurfaceType)
                {
                    return pSurfaceType->GetMaterial();
                }
                else
                {
                    return {};
                }
            }
            case ColumnMinHeight:
                return layer->GetLayerStart();
            case ColumnMaxHeight:
                return layer->GetLayerEnd();
            case ColumnMinAngle:
                return layer->GetLayerMinSlopeAngle();
            case ColumnMaxAngle:
                return layer->GetLayerMaxSlopeAngle();
            case ColumnBrightness:
                return layer->GetLayerBrightness();
            case ColumnBaseTiling:
                return layer->GetLayerTiling();
            case ColumnSortOrder:
                return layer->GetLayerSortOrder();
            case ColumnSpecularAmount:
                return layer->GetLayerSpecularAmount();
            case ColumnUseRemesh:
                return layer->GetLayerUseRemeshing();
            case ColumnSplatMap:
                return layer->GetSplatMapPath();
            default:
                return {};
            }
        case Qt::BackgroundColorRole:
            switch (index.column())
            {
            case ColumnMinHeight:
            case ColumnMaxHeight:
                return QColor(230, 255, 230);
            case ColumnMinAngle:
            case ColumnMaxAngle:
                return QColor(230, 255, 255);
            case ColumnBrightness:
            case ColumnBaseTiling:
            case ColumnSortOrder:
                return QColor(230, 230, 255);
            case ColumnSpecularAmount:
            case ColumnUseRemesh:
                return QColor(255, 230, 255);
            default:
                return {};
            }
        case Qt::DecorationRole:
            if (index.column() == ColumnLayerName && m_previewsEnabled)
            {
                return generatePreview(layer, QSize(TEXTURE_PREVIEW_SIZE, TEXTURE_PREVIEW_SIZE));
            }
            return {};
        case Qt::FontRole:
            if (index.column() == ColumnLayerName)
            {
                QFont font;
                font.setBold(true);
                return font;
            }
            else if (index.column() == ColumnMaterial)
            {
                QFont font;
                font.setUnderline(m_hovered == layer);
                return font;
            }
            return {};
        case Qt::TextAlignmentRole:
            if (index.column() >= ColumnMinHeight)
            {
                return QVariant(Qt::AlignHCenter | Qt::AlignVCenter);
            }
            else
            {
                return {};
            }
        case Qt::SizeHintRole:
            if (index.column() == ColumnLayerName && m_previewsEnabled)
            {
                QFont font;
                font.setBold(true);
                QFontMetrics metrics(font);
                QSize size(TEXTURE_PREVIEW_SIZE + 20 + metrics.boundingRect(layer->GetLayerName()).width(), TEXTURE_PREVIEW_SIZE);
                return size;
            }
            return {};
        case Qt::UserRole:
            return QVariant().fromValue<CLayer*>(layer);
        default:
            return {};
        }
    }

    Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        if (index.column() != ColumnMaterial && index.column() != ColumnSplatMap)
        {
            return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
        }
        else
        {
            return QAbstractTableModel::flags(index);
        }
    }

    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override
    {
        if (role != Qt::EditRole || index.row() < 0 || index.row() >= m_layers.size())
        {
            return false;
        }

        CLayer* layer = m_layers[index.row()];

        switch (index.column())
        {
        case ColumnLayerName:
            if (!value.toString().isEmpty())
            {
                layer->SetLayerName(GetIEditor()->GetTerrainManager()->GenerateUniqueLayerName(value.toString()));
            }
            break;
        case ColumnMinHeight:
            layer->SetLayerStart(value.toFloat());
            break;
        case ColumnMaxHeight:
            layer->SetLayerEnd(value.toFloat());
            break;
        case ColumnMinAngle:
            layer->SetLayerMinSlopeAngle(value.toFloat());
            break;
        case ColumnMaxAngle:
            layer->SetLayerMaxSlopeAngle(value.toFloat());
            break;
        case ColumnBrightness:
            layer->SetLayerBrightness(value.toFloat());
            break;
        case ColumnBaseTiling:
            layer->SetLayerTiling(value.toFloat());
            break;
        case ColumnSortOrder:
            layer->SetLayerSortOrder(value.toFloat());
            break;
        case ColumnSpecularAmount:
            layer->SetLayerSpecularAmount(value.toFloat());
            break;
        case ColumnUseRemesh:
            layer->SetLayerUseRemeshing(value.toFloat());
            break;
        default:
            return false;
        }

        emit dataChanged(index, index);
        GetIEditor()->Notify(eNotify_OnInvalidateControls);
        return true;
    }

    QModelIndex add(CLayer* layer)
    {
        beginInsertRows({}, m_layers.size(), m_layers.size());
        m_layers.push_back(layer);
        endInsertRows();
        return createIndex(m_layers.size() - 1, 0);
    }

    void remove(CLayer* layer)
    {
        int index = m_layers.indexOf(layer);
        beginRemoveRows({}, index, index);
        m_layers.remove(index);
        endRemoveRows();
    }

    void moveUp(CLayer* layer)
    {
        int row = m_layers.indexOf(layer);
        if (row > 0)
        {
            beginMoveRows({}, row, row, {}, row - 1);
            std::swap(m_layers[row], m_layers[row - 1]);
            endMoveRows();
        }
    }

    void moveDown(CLayer* layer)
    {
        int row = m_layers.indexOf(layer);
        if (row < m_layers.size() - 1)
        {
            beginMoveRows({}, row, row, {}, row + 2);
            std::swap(m_layers[row], m_layers[row + 1]);
            endMoveRows();
        }
    }

    QModelIndex indexOf(CLayer* layer, int column) const
    {
        int row = m_layers.indexOf(layer);
        if (row != -1 && column >= 0 && column < ColumnCount)
        {
            return createIndex(row, column);
        }
        else
        {
            return {};
        }
    }

    QItemSelection selectionForRow(CLayer* layer) const
    {
        int row = m_layers.indexOf(layer);
        if (row != -1)
        {
            return QItemSelection(createIndex(row, ColumnLayerName), createIndex(row, ColumnUseRemesh));
        }
        else
        {
            return {};
        }
    }

    int size() const
    {
        return m_layers.size();
    }

    void clear()
    {
        if (m_layers.size() == 0)
        {
            return;
        }

        beginRemoveRows({}, 0, m_layers.size() - 1);
        m_layers.clear();
        endRemoveRows();
    }

    void hover(const QModelIndex& index)
    {
        CLayer* layer;

        if (index.row() < 0 || index.row() >= m_layers.size())
        {
            layer = nullptr;
        }
        else
        {
            layer = m_layers[index.row()];
        }

        if (m_hovered != nullptr)
        {
            QModelIndex index = indexOf(layer, ColumnMaterial);
            emit dataChanged(index, index, { Qt::FontRole });
        }
        m_hovered = layer;
        if (m_hovered != nullptr)
        {
            QModelIndex index = indexOf(layer, ColumnMaterial);
            emit dataChanged(index, index, { Qt::FontRole });
        }
    }

public slots:
    void enablePreviews(bool enabled)
    {
        if (enabled != m_previewsEnabled)
        {
            m_previewsEnabled = enabled;

            emit dataChanged(
                createIndex(0, 0),
                createIndex(m_layers.size() - 1, 0),
                { Qt::DecorationRole });
        }
    }

private:
    QVector<CLayer*> m_layers;
    bool m_previewsEnabled = true;
    CLayer* m_hovered;
};

class LinkDelegate
    : public QStyledItemDelegate
{
public:
    LinkDelegate(QObject* parent, TerrainTextureLayerEditModel& model, QTableView& view, std::function<void(CLayer*)> linkClicked)
        : QStyledItemDelegate(parent)
        , m_model(model)
        , m_view(view)
        , m_linkClicked(linkClicked)
    {
    }

    virtual ~LinkDelegate() {}

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        const QFont& font = m_model.data(index, Qt::FontRole).value<QFont>();
        const QFontMetrics& metrics = option.fontMetrics;

        QRect bounding = metrics.boundingRect(m_model.data(index, Qt::DisplayRole).toString());

        return {
                   bounding.width() + s_contentMargin* 2 + 3, bounding.height() + s_contentMargin* 2 + 1
        };
    }


    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        painter->save();

        QString text = m_model.data(index, Qt::DisplayRole).toString();

        const QFont& font = m_model.data(index, Qt::FontRole).value<QFont>();
        const QFontMetrics& metrics = option.fontMetrics;

        painter->setFont(font);

        //insert zero-width space after forward slashes
        text = text.replace('/', QString("/") + QChar(0x200b));
        //replace spaces with non-breaking spaces
        text = text.replace(' ', QChar(0xa0));

        auto availableRect = option.rect.adjusted(s_contentMargin, s_contentMargin, 0, -s_contentMargin);

        //only word wrap if there is space for more than one line
        int flags = option.displayAlignment;
        if (availableRect.height() > 2 * metrics.height())
        {
            flags |= Qt::TextWordWrap;
        }

        auto textRect = metrics.boundingRect(availableRect, flags, text);
        if (textRect.width() > availableRect.width())
        {
            //create gradient brush to show that there is more text to the right
            QLinearGradient gradient(availableRect.topLeft(), availableRect.topRight());
            auto pen = painter->pen();
            auto penColor = pen.color();
            gradient.setColorAt(0, penColor);
            gradient.setColorAt(double(availableRect.width() - 10) / availableRect.width(), penColor);
            gradient.setColorAt(1, Qt::transparent);
            pen.setBrush(QBrush(gradient));
            painter->setPen(pen);
        }

        //render text
        painter->drawText(availableRect, flags, text, &textRect);

        m_hit[index] = textRect;
        painter->restore();
    }

    bool isHit(const QPoint& point, const QModelIndex& index) const
    {
        return m_hit[index].contains(point);
    }

    bool editorEvent(QEvent* event, QAbstractItemModel*, const QStyleOptionViewItem& option, const QModelIndex& index)
    {
        if (event->type() == QEvent::MouseMove)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

            if (isHit(mouseEvent->pos(), index))
            {
                m_view.setCursor(Qt::PointingHandCursor);
                m_model.hover(index);
            }
            else
            {
                m_view.setCursor(Qt::ArrowCursor);
                m_model.hover({});
            }
        }
        else if (event->type() == QEvent::MouseButtonRelease)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

            if (isHit(mouseEvent->pos(), index))
            {
                m_linkClicked(m_model.data(index, Qt::UserRole).value<CLayer*>());
                return true;
            }
        }
        else if (!(option.state & QStyle::State_MouseOver))
        {
            m_view.setCursor(Qt::ArrowCursor);
            m_model.hover({});
        }
        return false;
    }

private:
    TerrainTextureLayerEditModel& m_model;
    QTableView& m_view;
    std::function<void(CLayer*)> m_linkClicked;

    mutable QHash<QModelIndex, QRect> m_hit;

    static const int s_contentMargin = 3;
};

//////////////////////////////////////////////////////////////////////////
class BoolGuard
{
public:
    BoolGuard(bool& guarded) noexcept : m_guarded{guarded}
    {
        m_guarded = true;
    }

    ~BoolGuard() noexcept
    {
        m_guarded = false;
    }

private:
    bool& m_guarded;
};
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
AZStd::unique_ptr<CLayer> CreateDefaultLayer()
{
    IEditor* editor = GetIEditor();

    if (!editor)
    {
        return {};
    }

    CTerrainManager* terrainManager = editor->GetTerrainManager();

    if (!terrainManager)
    {
        return {};
    }

    auto newLayer = AZStd::make_unique<CLayer>();
    newLayer->SetLayerName(terrainManager->GenerateUniqueLayerName(QStringLiteral("NewLayer")));
    newLayer->LoadTexture("engineassets/textures/grey.dds");
    newLayer->AssignMaterial("Materials/material_terrain_default");
    newLayer->GetOrRequestLayerId();

    return newLayer;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions opts;
    opts.paneRect = QRect(QPoint(0, 0), QSize(1000, 650));
    opts.sendViewPaneNameBackToAmazonAnalyticsServers = true;

    AzToolsFramework::RegisterViewPane<CTerrainTextureDialog>(LyViewPane::TerrainTextureLayers, LyViewPane::CategoryOther, opts);
}

const GUID& CTerrainTextureDialog::GetClassID()
{
    // {A5665E6A-A02B-485f-BC12-7B157DBA841A}
    static const GUID guid = {
        0xa5665e6a, 0xa02b, 0x485f, { 0xbc, 0x12, 0x7b, 0x15, 0x7d, 0xba, 0x84, 0x1a }
    };
    return guid;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnUndoUpdate()
{
    if (m_alive)
    {
        ReloadLayerList();
    }
}

//////////////////////////////////////////////////////////////////////////
CTerrainTextureDialog::CTerrainTextureDialog(QWidget* parent /* = nullptr */)
    : QDialog(parent)
    , m_ui(new Ui::TerrainTextureDialog)
    , m_model(new TerrainTextureLayerEditModel(this))
{
    m_ui->setupUi(this);
    m_model->enablePreviews(true);

    setContextMenuPolicy(Qt::NoContextMenu);

    m_ignoreNotify = false;
    GetIEditor()->RegisterNotifyListener(this);
    GetIEditor()->GetMaterialManager()->AddListener(this);

    OnInitDialog();

    Physics::EditorTerrainComponentNotificationBus::Handler::BusConnect();
}

//////////////////////////////////////////////////////////////////////////
CTerrainTextureDialog::~CTerrainTextureDialog()
{
    Physics::EditorTerrainComponentNotificationBus::Handler::BusDisconnect();
    m_alive = false;

    
    GetIEditor()->GetMaterialManager()->RemoveListener(this);
    GetIEditor()->UnregisterNotifyListener(this);
    ClearData();

    GetIEditor()->UpdateViews(eUpdateHeightmap);
}

void CTerrainTextureDialog::ClearData()
{
    m_model->clear();

    // Release all layer masks.
    for (int i = 0; i < GetIEditor()->GetTerrainManager()->GetLayerCount(); i++)
    {
        GetIEditor()->GetTerrainManager()->GetLayer(i)->ReleaseTempResources();
    }
}

/////////////////////////////////////////////////////////////////////////////
// CTerrainTextureDialog message handlers

void CTerrainTextureDialog::OnInitDialog()
{
    m_ui->layerTableView->setModel(m_model);

    QApplication::setOverrideCursor(Qt::WaitCursor);

    connect(m_ui->addLayerClickable, &QLabel::linkActivated, this, &CTerrainTextureDialog::OnLayersNewItem);
    connect(m_ui->deleteLayerClickable, &QLabel::linkActivated, this, &CTerrainTextureDialog::OnLayersDeleteItem);
    connect(m_ui->moveLayerUpClickable, &QLabel::linkActivated, this, &CTerrainTextureDialog::OnLayersMoveItemUp);
    connect(m_ui->moveLayerDownClickable, &QLabel::linkActivated, this, &CTerrainTextureDialog::OnLayersMoveItemDown);
    connect(m_ui->assignMaterialClickable, &QLabel::linkActivated, this, &CTerrainTextureDialog::OnAssignMaterial);
    connect(m_ui->assignSplatMapClickable, &QLabel::linkActivated, this, &CTerrainTextureDialog::OnAssignSplatMap);
    connect(m_ui->importSplatMapsClickable, &QLabel::linkActivated, this, &CTerrainTextureDialog::OnImportSplatMaps);
    connect(m_ui->exportSplatMapClickable, &QLabel::linkActivated, this, &CTerrainTextureDialog::OnExportSplatMap);

    connect(m_ui->changeLayerTextureClickable, &QLabel::linkActivated, this, &CTerrainTextureDialog::OnLoadTexture);

    connect(m_ui->showPreviewCheckBox, &QCheckBox::stateChanged, this, [&](int state)
        {
            m_model->enablePreviews(state == Qt::Checked);
            m_ui->layerTableView->resizeColumnToContents(ColumnLayerName);
        });

    connect(m_ui->layerTableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CTerrainTextureDialog::OnReportSelChange);

    connect(m_ui->loadTextureAction, &QAction::triggered, this, &CTerrainTextureDialog::OnLoadTexture);
    connect(m_ui->exportTextureAction, &QAction::triggered, this, &CTerrainTextureDialog::OnLayerExportTexture);
    connect(m_ui->importLayersAction, &QAction::triggered, this, &CTerrainTextureDialog::OnImport);
    connect(m_ui->exportLayersAction, &QAction::triggered, this, &CTerrainTextureDialog::OnExport);
    connect(m_ui->refineTerrainTextureTilesAction, &QAction::triggered, this, &CTerrainTextureDialog::OnRefineTerrainTextureTiles);
    connect(m_ui->showLargePreviewAction, &QAction::triggered, this, &CTerrainTextureDialog::OnFileExportLargePreview);
    connect(m_ui->applyLightingAction, &QAction::toggled, this, &CTerrainTextureDialog::OnApplyLighting);
    connect(m_ui->showWaterAction, &QAction::toggled, this, &CTerrainTextureDialog::OnShowWater);
    // No obvious target for this
    //connect(m_ui->actionEdit_Surface_Types, &QAction::triggered, this, &CTerrainTextureDialog::OnUpdateShowWater);

    m_ui->layerTableView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_ui->layerTableView->setItemDelegateForColumn(ColumnMaterial, new LinkDelegate(this, *m_model, *m_ui->layerTableView, [&](CLayer* layer) { OnReportHyperlink(layer); }));
    m_ui->layerTableView->viewport()->setMouseTracking(true);
    m_ui->layerTableView->viewport()->setAttribute(Qt::WA_Hover, true);

    AZ::SerializeContext* m_serializeContext;
    AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
    AZ_Assert(m_serializeContext, "Failed to retrieve serialize context.");

    m_propertyEditor = new AzToolsFramework::ReflectedPropertyEditor(m_ui->widget);
    m_propertyEditor->Setup(m_serializeContext, this, false, 150);
    m_propertyEditor->show();
    m_ui->materialSelection->addWidget(m_propertyEditor);
    m_ui->terrainNotFoundMessage->setText("No terrain component found");
    m_ui->terrainNotFoundMessage->setStyleSheet("color: red;");

    m_physicsMaterialSelection = AZStd::make_unique<Physics::MaterialSelection>();

    // Load the layer list from the document
    ReloadLayerList();

    EnableControls();

    OnGeneratePreview();

    QApplication::restoreOverrideCursor();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::ReloadLayerList()
{
    ////////////////////////////////////////////////////////////////////////
    // Fill the layer list box with the data from the document
    ////////////////////////////////////////////////////////////////////////

    m_model->clear();

    for (quint32 i = 0; i < GetIEditor()->GetTerrainManager()->GetLayerCount(); i++)
    {
        CLayer* pLayer = GetIEditor()->GetTerrainManager()->GetLayer(i);
        m_model->add(pLayer);
    }

    m_ui->layerTableView->resizeColumnsToContents();

    UpdateControlData();

    GetIEditor()->GetTerrainManager()->ReloadSurfaceTypes();
}


void CTerrainTextureDialog::EnableControls()
{
    ////////////////////////////////////////////////////////////////////////
    // Enable / disable the current based of if at least one layer is
    // present and activated
    ////////////////////////////////////////////////////////////////////////

    bool selection = m_ui->layerTableView->selectionModel()->selectedIndexes().size() > 0;
    m_ui->loadTextureAction->setEnabled(selection);
    m_ui->exportTextureAction->setEnabled(selection);
    m_ui->deleteLayerClickable->setEnabled(selection);
    m_ui->moveLayerUpClickable->setEnabled(selection);
    m_ui->moveLayerDownClickable->setEnabled(selection);
    m_ui->changeLayerTextureClickable->setEnabled(selection);

    if (!selection)
    {
        blockSignals(true);
        m_propertyEditor->ClearInstances();
        m_propertyEditor->InvalidateAll();
        blockSignals(false);
    }

    if (Physics::EditorTerrainMaterialRequestsBus::HasHandlers())
    {
        m_propertyEditor->ExpandAll();
        m_propertyEditor->setVisible(true);
        m_ui->terrainNotFoundMessage->setVisible(false);
    }
    else
    {
        m_propertyEditor->CollapseAll();
        m_propertyEditor->setVisible(false);
        m_ui->terrainNotFoundMessage->setVisible(true);
    }
    
    m_ui->exportLayersAction->setEnabled(m_model->size() > 0);
    m_ui->showLargePreviewAction->setEnabled(m_model->size() > 0);

    UpdateAssignMaterialItem();
    UpdateAssignSplatMapItem();
    UpdateExportSplatMapItem();
}

void CTerrainTextureDialog::UpdateControlData()
{
    ////////////////////////////////////////////////////////////////////////
    // Update the controls with the data from the current layer
    ////////////////////////////////////////////////////////////////////////

    Layers layers = GetSelectedLayers();

    if (layers.size() == 0)
    {
        m_ui->layerInfoLabel->setText("No layer selected");
        m_ui->texturePreviewLabel->setText("No layer selected");
        m_ui->layerTextureInfoLabel->setText("");
    }
    else if (layers.size() == 1)
    {
        CLayer* pSelLayer = layers[0];

        m_ui->texturePreviewLabel->setPixmap(generatePreview(pSelLayer, QSize(128, 128)));

        QString layerIdCaption = tr("LayerId");
        QString surfaceIdCaption = tr("Surface (Physics Material) Id");
        QString textureInfo = QString("%1\n%2x%3 %4 %5\n%6 %7")
                .arg(pSelLayer->GetTextureFilename())
                .arg(pSelLayer->GetTextureWidth())
                .arg(pSelLayer->GetTextureHeight())
                .arg(layerIdCaption)
                .arg(pSelLayer->GetCurrentLayerId())
                .arg(surfaceIdCaption)
                .arg(pSelLayer->GetEngineSurfaceTypeId())
            ;
        m_ui->layerTextureInfoLabel->setText(textureInfo);

        int nNumSurfaceTypes = GetIEditor()->GetTerrainManager()->GetSurfaceTypeCount();
        QString layerSizeCaption = tr("Layer Size:");
        QString surfaceTypeCountCaption = tr("Surface Type Count");
        QString layerInfo = QString("%1 %2x%2\n%3 %4")
                .arg(layerSizeCaption)
                .arg(pSelLayer->GetSectorInfoSurfaceTextureSize())
                .arg(surfaceTypeCountCaption)
                .arg(nNumSurfaceTypes);
        m_ui->layerInfoLabel->setText(layerInfo);

        bool materialFound = false;
        Physics::MaterialSelection selection;
        selection.SetMaterialSlots({});
        int surfaceId = pSelLayer->GetEngineSurfaceTypeId();
        Physics::EditorTerrainMaterialRequestsBus::BroadcastResult(
            materialFound, &Physics::EditorTerrainMaterialRequests::GetMaterialSelectionForSurfaceId, surfaceId, selection);
        *m_physicsMaterialSelection.get() = selection;

        blockSignals(true);
        m_propertyEditor->ClearInstances();
        m_propertyEditor->AddInstance(m_physicsMaterialSelection.get());
        m_propertyEditor->InvalidateAll();
        blockSignals(false);
    }
    else
    {
        m_ui->layerInfoLabel->setText("Multiple layers selected");
        m_ui->texturePreviewLabel->setText("Multiple layers selected");
        m_ui->layerTextureInfoLabel->setText("");
    }

    // Update the controls
    EnableControls();
}

void CTerrainTextureDialog::OnLoadTexture()
{
    Layers layers = GetSelectedLayers();

    if (layers.size() == 0)
    {
        return;
    }

    GetIEditor()->SetEditTool(0);
    ////////////////////////////////////////////////////////////////////////
    // Load a texture from a BMP file
    ////////////////////////////////////////////////////////////////////////

    AssetSelectionModel selection = AssetSelectionModel::AssetGroupSelection("Texture");
    AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);
    if (selection.IsValid())
    {
        CUndo undo("Load Layer Texture");
        GetIEditor()->RecordUndo(new CTerrainLayersUndoObject());

        // Load the texture
        if (!layers[0]->LoadTexture(selection.GetResult()->GetFullPath().c_str()))
        {
            QMessageBox::warning(this, "Failed to load texture", "Error while loading the texture !");
        }

        ReloadLayerList();
    }

    // Regenerate the preview
    OnGeneratePreview();

    // Update the controls
    EnableControls();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnGeneratePreview()
{
    ////////////////////////////////////////////////////////////////////////
    // Generate all layer mask and create a preview of the texture
    ////////////////////////////////////////////////////////////////////////
}

void CTerrainTextureDialog::OnFileExportLargePreview()
{
    ////////////////////////////////////////////////////////////////////////
    // Show a large preview of the final texture
    ////////////////////////////////////////////////////////////////////////

    const bool isLegacyTerrainActive = LegacyTerrain::LegacyTerrainDataRequestBus::HasHandlers();
    if (!isLegacyTerrainActive)
    {
        QMessageBox::warning(this, "No Terrain", "Terrain is not presented in the current level.");
        return;
    }

    CDimensionsDialog cDialog;
    cDialog.SetDimensions(1024); // 1024x1024 is default

    // Query the size of the preview
    if (cDialog.exec() == QDialog::Rejected)
    {
        return;
    }

    CLogFile::FormatLine("Exporting large terrain texture preview (%ix%i)...",
        cDialog.GetDimensions(), cDialog.GetDimensions());

    // Allocate the memory for the texture
    CImageEx image;
    if (!image.Allocate(cDialog.GetDimensions(), cDialog.GetDimensions()))
    {
        return;
    }

    // Generate the terrain texture
    int tflags = ETTG_INVALIDATE_LAYERS | ETTG_STATOBJ_SHADOWS | ETTG_BAKELIGHTING;
    if (m_ui->applyLightingAction->isChecked())
    {
        tflags |= ETTG_LIGHTING;
    }
    if (m_ui->showWaterAction->isChecked())
    {
        tflags |= ETTG_SHOW_WATER;
    }

    CTerrainTexGen texGen;
    bool bReturn = texGen.GenerateSurfaceTexture(tflags, image);

    if (!bReturn)
    {
        CLogFile::WriteLine("Error while generating terrain texture preview");
        QMessageBox::warning(this, "Terrain Texture Error", "Can't generate terrain texture.");
        return;
    }

    GetIEditor()->SetStatusText("Saving preview...");

    // Save the texture to disk
    QString tempDirectory = Path::AddPathSlash(gSettings.strStandardTempDirectory);
    CFileUtil::CreateDirectory(tempDirectory.toUtf8().data());

    QString imageName = "TexturePreview.bmp";
    bool bOk = CImageUtil::SaveImage(tempDirectory + imageName, image);
    if (bOk)
    {
        // Show the heightmap
        QDesktopServices::openUrl(QUrl::fromLocalFile(tempDirectory + imageName));
    }
    else
    {
        QMessageBox::warning(this, "Preview Error", "Can't save preview bitmap!");
    }

    GetIEditor()->SetStatusText("Ready");
}

void CTerrainTextureDialog::OnImport()
{
    ////////////////////////////////////////////////////////////////////////
    // Import layer settings from a file
    ////////////////////////////////////////////////////////////////////////

    char szFilters[] = "Layer Files (*.lay)";
    CAutoDirectoryRestoreFileDialog dlg(QFileDialog::AcceptOpen, QFileDialog::ExistingFile, "lay", {}, szFilters, {}, {}, this);
    if (dlg.exec())
    {
        QString file = dlg.selectedFiles().first();
        CLogFile::FormatLine("Importing layer settings from %s", file.toUtf8().data());

        CUndo undo("Import Texture Layers");
        GetIEditor()->RecordUndo(new CTerrainLayersUndoObject());

        CXmlArchive ar;
        if (ar.Load(file.toUtf8().data()))
        {
            GetIEditor()->GetTerrainManager()->SerializeSurfaceTypes(ar);
            GetIEditor()->GetTerrainManager()->SerializeLayerSettings(ar);
        }

        // Load the layers into the dialog
        ReloadLayerList();
    }

    // Notify terrain painter panel to update layers
    BoolGuard guard{m_ignoreNotify};
    GetIEditor()->Notify(eNotify_OnInvalidateControls);
}


void CTerrainTextureDialog::OnRefineTerrainTextureTiles()
{
    auto answer = QMessageBox::question(
            this,
            "Refine Terrain Texture?",
            "Refine Terrain Texture? All terrain texture tiles become split in 4 parts so a tile with 2048x2048 no longer limits the resolution."
            "\n\nYou need to save afterwards!");
    if (answer == QMessageBox::Yes)
    {
        if (!GetIEditor()->GetTerrainManager()->GetRGBLayer()->RefineTiles())
        {
            QMessageBox::warning(this, "Refine Terrain Texture Failed", "Terrain Texture refine failed (make sure current data is saved)");
        }
        else
        {
            QMessageBox::warning(this, "Refine Terrain Succeeded", "Successfully refined Terrain Texture - Save is now required!");
        }
    }
}

void CTerrainTextureDialog::OnExport()
{
    ////////////////////////////////////////////////////////////////////////
    // Export layer settings to a file
    ////////////////////////////////////////////////////////////////////////

    char szFilters[] = "Layer Files (*.lay)";
    CAutoDirectoryRestoreFileDialog dlg(QFileDialog::AcceptSave, QFileDialog::AnyFile, "lay", "ExportedLayers.lay", szFilters);
    if (dlg.exec())
    {
        QString file = dlg.selectedFiles().constFirst();
        CLogFile::FormatLine("Exporting layer settings to %s", file.toUtf8().data());

        CXmlArchive ar("LayerSettings");
        GetIEditor()->GetTerrainManager()->SerializeSurfaceTypes(ar);
        GetIEditor()->GetTerrainManager()->SerializeLayerSettings(ar);
        ar.Save(file.toUtf8().data());
    }
}

void CTerrainTextureDialog::OnApplyLighting()
{
    ////////////////////////////////////////////////////////////////////////
    // Toggle between the on / off for the apply lighting state
    ////////////////////////////////////////////////////////////////////////

    OnGeneratePreview();
}


void CTerrainTextureDialog::OnShowWater()
{
    ////////////////////////////////////////////////////////////////////////
    // Toggle between the on / off for the show water state
    ////////////////////////////////////////////////////////////////////////

    OnGeneratePreview();
}

void CTerrainTextureDialog::OnSetOceanLevel()
{
    ////////////////////////////////////////////////////////////////////////
    // Let the user change the current ocean level
    ////////////////////////////////////////////////////////////////////////

    float level = GetIEditor()->GetHeightmap()->GetOceanLevel();

    bool ok = false;
    int fractionalDigitCount = 2;
    level = aznumeric_caster(QInputDialog::getDouble(this, tr("Set Water Level"), QStringLiteral(""), level, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max(), fractionalDigitCount, &ok));
    if (ok)
    {
        // Retrieve the new ocean level from the dialog and save it in the document
        GetIEditor()->GetHeightmap()->SetOceanLevel(level);

        // We modified the document
        GetIEditor()->SetModifiedFlag();
        GetIEditor()->SetModifiedModule(eModifiedTerrain);

        OnGeneratePreview();
    }
}

void CTerrainTextureDialog::OnLayerExportTexture()
{
    ////////////////////////////////////////////////////////////////////////
    // Export the texture data, which is associated with the current layer,
    // to a bitmap file
    ////////////////////////////////////////////////////////////////////////
    Layers layers = GetSelectedLayers();

    if (layers.size() == 0)
    {
        return;
    }

    CLayer* layer = layers[0];

    // load m_texture is needed/possible
    layer->PrecacheTexture();

    // Does the current layer have texture data ?
    if (!layer->HasTexture())
    {
        QMessageBox::warning(this, tr("Can't Export Texture"), tr("Current layer does not have a texture, can't export!"));
        return;
    }

    CAutoDirectoryRestoreFileDialog dlg(QFileDialog::AcceptSave, QFileDialog::AnyFile, "png", {},
                                        "PNG Files (*.png);;BMP Files (*.bmp);;JPEG Files (*.jpg);;PGM Files (*.pgm);;All files (*.*)", {}, {}, this);
    if (dlg.exec())
    {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        // Tell the layer to export its texture
        bool success = layer->ExportTexture(dlg.selectedFiles().first());
        QApplication::restoreOverrideCursor();
        if (!success)
        {
            QMessageBox::warning(this, tr("Can't Export Texture"), tr("Texture failed to export successfully."));
            return;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnLayersNewItem()
{
    CUndo undo("New Terrain Layer");
    GetIEditor()->RecordUndo(new CTerrainLayersUndoObject());

    // Add the layer
    auto terrainManager = GetIEditor()->GetTerrainManager();

    auto newLayer = CreateDefaultLayer();

    if (!newLayer)
    {
        return;
    }

    CLayer* newLayerRaw = newLayer.get();

    terrainManager->AddLayer(newLayer.release());

    QModelIndex newIndex = m_model->add(newLayerRaw);
    m_ui->layerTableView->scrollTo(newIndex);
    m_ui->layerTableView->resizeColumnToContents(0);

    SelectLayer(newLayerRaw);

    // Update the controls with the data from the layer
    UpdateControlData();

    // Regenerate the preview
    OnGeneratePreview();

    BoolGuard guard{m_ignoreNotify};
    GetIEditor()->Notify(eNotify_OnInvalidateControls);
    GetIEditor()->Notify(eNotify_OnTextureLayerChange);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnLayersDeleteItem()
{
    Layers selected = GetSelectedLayers();

    if (selected.empty())
    {
        QMessageBox::warning(this, "Can't Delete Layers", "No target layers selected");
        return;
    }

    if ((m_model->rowCount() - selected.size()) == 0)
    {
        QMessageBox::warning(this, "Can't Delete Layers", "Must have at least one layer leftover");
        //Exit early if the user tried to delete the last layer. Having no terrain layers will result in a crash
        return;
    }

    CLayer* layer = selected.front();

    if (layer)
    {
        QString message = QString("Are you sure you want to delete layer %1?").arg(layer->GetLayerName());
        auto answer = QMessageBox::question(this, "Confirm Delete Layer", message);
        if (answer == QMessageBox::Yes)
        {
            DeleteLayerItem(layer);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnLayersMoveItemUp()
{
    IEditor* editor = GetIEditor();

    if (!editor)
    {
        return;
    }

    CTerrainManager* terrainManager = editor->GetTerrainManager();

    if (!terrainManager)
    {
        return;
    }

    CUndo undo("Move Terrain Layer Up");

    editor->RecordUndo(new CTerrainLayersUndoObject());

    Layers selected = GetSelectedLayers();

    if (selected.empty())
    {
        return;
    }

    CLayer* layer = selected.front();

    int indexCur = -1;
    for (int i = 0; i < terrainManager->GetLayerCount(); i++)
    {
        if (terrainManager->GetLayer(i) == layer)
        {
            indexCur = i;
            break;
        }
    }

    if (indexCur < 1)
    {
        return;
    }
    
    terrainManager->SwapLayers(indexCur, indexCur - 1);
    m_model->moveUp(layer);
    SelectLayer(layer);

    BoolGuard guard{m_ignoreNotify};
    editor->Notify(eNotify_OnTextureLayerChange);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnLayersMoveItemDown()
{
    IEditor* editor = GetIEditor();

    if (!editor)
    {
        return;
    }

    CTerrainManager* terrainManager = editor->GetTerrainManager();

    if (!terrainManager)
    {
        return;
    }

    CUndo undo("Move Terrain Layer Down");
    editor->RecordUndo(new CTerrainLayersUndoObject());

    Layers selected = GetSelectedLayers();

    if (selected.empty())
    {
        return;
    }

    CLayer* layer = selected.front();

    int indexCur = -1;
    for (int i = 0; i < terrainManager->GetLayerCount(); i++)
    {
        if (terrainManager->GetLayer(i) == layer)
        {
            indexCur = i;
            break;
        }
    }

    if (indexCur < 0 || indexCur >= terrainManager->GetLayerCount() - 1)
    {
        return;
    }

    terrainManager->SwapLayers(indexCur, indexCur + 1);
    m_model->moveDown(layer);
    SelectLayer(layer);

    BoolGuard guard{m_ignoreNotify};
    editor->Notify(eNotify_OnTextureLayerChange);
}


//////////////////////////////////////////////////////////////////////////
CTerrainTextureDialog::Layers CTerrainTextureDialog::GetSelectedLayers()
{
    Layers layers;

    auto selection = m_ui->layerTableView->selectionModel();
    QModelIndexList selected = selection->selectedIndexes();
    foreach (QModelIndex index, selected) {
        if (index.column() == 0)
        {
            CLayer* layer = m_model->data(index, Qt::UserRole).value<CLayer*>();
            layers.push_back(layer);
        }
    }

    return layers;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnReportSelChange(const QItemSelection& selected, const QItemSelection& deselected)
{
    QModelIndexList selectedIndices = selected.indexes();
    foreach(QModelIndex index, selectedIndices) {
        if (index.column() == 0)
        {
            CLayer* layer = m_model->data(index, Qt::UserRole).value<CLayer*>();
            layer->SetSelected(true);
        }
    }

    QModelIndexList deselectedIndices = deselected.indexes();
    foreach(QModelIndex index, deselectedIndices) {
        if (index.column() == 0)
        {
            CLayer* layer = m_model->data(index, Qt::UserRole).value<CLayer*>();
            layer->SetSelected(false);
        }
    }

    EnableControls();
    UpdateControlData();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnReportHyperlink(CLayer* layer)
{
    QString mtlName;
    CSurfaceType* surfaceType = layer->GetSurfaceType();
    if (surfaceType)
    {
        mtlName = surfaceType->GetMaterial();
    }

    _smart_ptr<IMaterial> mtl = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(mtlName.toUtf8().data(), false);
    if (mtl)
    {
        GetIEditor()->GetMaterialManager()->GotoMaterial(mtl);
    }
}

void CTerrainTextureDialog::BeforePropertyModified(AzToolsFramework::InstanceDataNode*)
{
}

void CTerrainTextureDialog::AfterPropertyModified(AzToolsFramework::InstanceDataNode*)
{
    Layers layers = GetSelectedLayers();
    if (layers.size())
    {
        CLayer* selLayer = layers.front();
        if (selLayer)
        {
            AZ_Warning("Physics", Physics::EditorTerrainMaterialRequestsBus::HasHandlers(),
                "There is no Handler attached to the EditorTerrainMaterialRequestsBus - This is"
                " most likely caused by not having a PhysX Terrain Component in the scene. Please"
                " ensure one is added.");

            Physics::EditorTerrainMaterialRequestsBus::Broadcast(
                &Physics::EditorTerrainMaterialRequests::SetMaterialSelectionForSurfaceId,
                selLayer->GetEngineSurfaceTypeId(), *m_physicsMaterialSelection);
        }
    }
}

void CTerrainTextureDialog::SetPropertyEditingActive(AzToolsFramework::InstanceDataNode*)
{
}

void CTerrainTextureDialog::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode*)
{
}

void CTerrainTextureDialog::SealUndoStack()
{
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::DeleteLayerItem(CLayer* layer)
{
    if (layer)
    {
        CUndo undo("Delete Terrain Layer");
        GetIEditor()->RecordUndo(new CTerrainLayersUndoObject());

        // Find the layer inside the layer list in the document and remove it.
        m_model->remove(layer);

        GetIEditor()->GetTerrainManager()->RemoveLayer(layer);

        // Regenerate the preview
        OnGeneratePreview();

        BoolGuard guard{m_ignoreNotify};
        GetIEditor()->Notify(eNotify_OnInvalidateControls);
        GetIEditor()->Notify(eNotify_OnTextureLayerChange);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::SelectLayer(CLayer* layer)
{
    // Unselect all layers.
    for (int i = 0; i < GetIEditor()->GetTerrainManager()->GetLayerCount(); i++)
    {
        GetIEditor()->GetTerrainManager()->GetLayer(i)->SetSelected(false);
    }
    layer->SetSelected(true);

    //  m_bMaskPreviewValid = false;

    auto selection = m_ui->layerTableView->selectionModel();
    selection->select(m_model->selectionForRow(layer), QItemSelectionModel::ClearAndSelect);

    // Update the controls with the data from the layer
    UpdateControlData();

    BoolGuard guard{m_ignoreNotify};
    GetIEditor()->Notify(eNotify_OnSelectionChange);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnAssignMaterial()
{
    Layers layers = GetSelectedLayers();
    CUndo undo("Assign Layer Material");
    GetIEditor()->RecordUndo(new CTerrainLayersUndoObject());

    CMaterial* pMaterial = GetIEditor()->GetMaterialManager()->GetCurrentMaterial();
    assert(pMaterial != nullptr);
    if (pMaterial == nullptr)
    {
        return;
    }

    for (int i = 0; i < layers.size(); i++)
    {
        layers[i]->AssignMaterial(pMaterial->GetName());
    }

    ReloadLayerList();

    GetIEditor()->GetTerrainManager()->ReloadSurfaceTypes();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnAssignSplatMap()
{
    Layers layers = GetSelectedLayers();

    // No layers selected, so nothing to assign.
    if (layers.size() != 1)
    {
        QMessageBox::warning(this, "Can't Assign Splat Map", "Select a target layer before assigning a splat map.");
        return;
    }

    CUndo undo("Assign Layer Mask");
    GetIEditor()->RecordUndo(new CTerrainLayersUndoObject());

    CLayer* layer = layers[0];
    QString filePath = layer->GetSplatMapPath();

    if (!filePath.isEmpty())
    {
        filePath = Path::GamePathToFullPath(filePath);
    }

    QString selectedFile = filePath;

    if (CFileUtil::SelectFile(QStringLiteral("Bitmap Image File (*.bmp)"), filePath, selectedFile))
    {
        QString newPath = Path::FullPathToGamePath(selectedFile);
        layer->SetSplatMapPath(newPath);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnExportSplatMap()
{
    Layers layers = GetSelectedLayers();

    // No layers selected, so nothing to assign.
    if (layers.size() != 1)
    {
        QMessageBox::warning(this, "Can't Export Splat Map", "Select a target layer before exporting a splat map.");
        return;
    }

    auto editor = GetIEditor();
    CHeightmap* heightMap = editor->GetHeightmap();
    if (!heightMap->IsAllocated())
    {
        QMessageBox::warning(this, "Can't Export Splat Map", "Terrain isn't properly initialized in this level.");
        return;
    }

    CLayer* layer = layers[0];
    QString filePath = layer->GetSplatMapPath();

    if (!filePath.isEmpty())
    {
        filePath = Path::GamePathToFullPath(filePath);
    }

    QString selectedFile = filePath;

    char szFilters[] = "BMP Files (*.bmp);;PNG Files (*.png);;JPEG Files (*.jpg);;PGM Files (*.pgm);;All files (*.*)";
    CAutoDirectoryRestoreFileDialog dlg(QFileDialog::AcceptSave, QFileDialog::AnyFile, "bmp", selectedFile, szFilters, {}, {}, this);
    if (dlg.exec())
    {
        QWaitCursor wait;
        CLogFile::WriteLine("Exporting splat map...");

        QString newPath = dlg.selectedFiles().first();
        if (!ExportSplatMap(layer->GetCurrentLayerId(), newPath))
        {
            QMessageBox::critical(QApplication::activeWindow(), QString(), QObject::tr("Error: Can't save image file."));
            return;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CTerrainTextureDialog::ExportSplatMap(uint32 layerId, const QString& imagePath)
{
    auto editor = GetIEditor();
    if (editor)
    {
        CHeightmap* heightMap = editor->GetHeightmap();
        if (heightMap && heightMap->IsAllocated())
        {
            // Get the splatmap, then rotate it for export.  (All data inside of heightmap is stored rotated 270 degrees)
            CImageEx unrotatedSplatImage;
            CImageEx splatImage;
            heightMap->GetLayerWeights(layerId, &unrotatedSplatImage);
            splatImage.RotateOrt(unrotatedSplatImage, ImageRotationDegrees::Rotate90);

            if (CImageUtil::SaveImage(imagePath, splatImage))
            {
                return true;
            }
        }
    }

    return false;
}




//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnDataBaseItemEvent(IDataBaseItem* item, EDataBaseItemEvent event)
{
    if (event == EDB_ITEM_EVENT_SELECTED)
    {
        UpdateAssignMaterialItem();
        UpdateAssignSplatMapItem();
        UpdateExportSplatMapItem();
    }
}

void CTerrainTextureDialog::OnTerrainComponentActive()
{
    UpdateControlData();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::UpdateAssignMaterialItem()
{
    bool layerSelected = m_ui->layerTableView->selectionModel()->selectedIndexes().size() > 0;
    CMaterial* material = GetIEditor()->GetMaterialManager()->GetCurrentMaterial();
    bool materialSelected = (material != nullptr);

    m_ui->assignMaterialClickable->setEnabled(layerSelected && materialSelected);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::UpdateAssignSplatMapItem()
{
    Layers layers = GetSelectedLayers();
    bool layerSelected = layers.size() == 1;

    m_ui->assignSplatMapClickable->setEnabled(layerSelected);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::UpdateExportSplatMapItem()
{
    Layers layers = GetSelectedLayers();
    bool layerSelected = layers.size() == 1;

    m_ui->exportSplatMapClickable->setEnabled(layerSelected);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnImportSplatMaps()
{
    // Make sure we have an allocated heightmap and at least one masked layer before we import
    auto editor = GetIEditor();
    CHeightmap* heightMap = editor->GetHeightmap();
    if (!heightMap->IsAllocated())
    {
        return;
    }

    assert(heightMap->GetHeight() > 0 && heightMap->GetWidth() > 0);

    auto terrainManager = editor->GetTerrainManager();
    auto layerCount = terrainManager->GetLayerCount();
    bool foundOne = false;

    for (int i = 0; i < layerCount; ++i)
    {
        auto layer = terrainManager->GetLayer(i);
        if (!layer->GetSplatMapPath().isEmpty())
        {
            foundOne = true;
            break;
        }
    }

    if (!foundOne)
    {
        return;
    }

    // Import our data and mark things as modified
    if (!ImportSplatMaps())
    {
        QMessageBox::critical(this, QString(), tr("Error: Can't load BMP file. Probably out of memory."));
    }
}

//////////////////////////////////////////////////////////////////////////
bool CTerrainTextureDialog::ImportSplatMaps()
{
    auto editor = GetIEditor();
    auto terrainManager = editor->GetTerrainManager();
    auto layerCount = terrainManager->GetLayerCount();
    auto heightMap = editor->GetHeightmap();

    // Walk through the layers, skipping any non-mask layers
    AZStd::vector<uint8> layerIds;
    AZStd::vector<CImageEx> splatMaps(layerCount);

    for (int i = 0; i < layerCount; ++i)
    {
        auto layer = terrainManager->GetLayer(i);
        if (layer->GetSplatMapPath().isEmpty())
        {
            continue;
        }

        // Load the mask's BMP and rotate by 270-degrees to match loaded PGM heightmap and orientation of the image in content creation tools
        auto path = Path::GamePathToFullPath(layer->GetSplatMapPath());
        CImageEx splat;

        if (!CImageUtil::LoadBmp(path, splat))
        {
            return false;
        }

        splatMaps[layerIds.size()].RotateOrt(splat, ImageRotationDegrees::Rotate270);

        // Remember this layer because it is real
        layerIds.push_back(aznumeric_caster(layer->GetOrRequestLayerId()));
    }

    // Now build the weight map using the masked layers
    heightMap->SetLayerWeights(layerIds, splatMaps.data(), layerIds.size());

    editor->SetModifiedFlag();
    editor->SetModifiedModule(eModifiedTerrain);

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    if (m_ignoreNotify)
    {
        return;
    }
    switch (event)
    {
    case eNotify_OnBeginNewScene:
    case eNotify_OnBeginSceneOpen:
    case eNotify_OnCloseScene:
        ClearData();
        break;
    case eNotify_OnEndNewScene:
    case eNotify_OnEndSceneOpen:
        ReloadLayerList();
        break;

    case eNotify_OnSelectionChange:
    {
        for (int i = 0, cnt = GetIEditor()->GetTerrainManager()->GetLayerCount(); i < cnt; ++i)
        {
            CLayer* layer = GetIEditor()->GetTerrainManager()->GetLayer(i);
            if (layer && layer->IsSelected())
            {
                SelectLayer(layer);
                break;
            }
        }
    }
    break;

    case eNotify_OnInvalidateControls:
    {
        CLayer* layer = 0;
        for (int i = 0; i < GetIEditor()->GetTerrainManager()->GetLayerCount(); i++)
        {
            if (GetIEditor()->GetTerrainManager()->GetLayer(i)->IsSelected())
            {
                layer = GetIEditor()->GetTerrainManager()->GetLayer(i);
                break;
            }
        }

        if (layer)
        {
            SelectLayer(layer);
        }
    }
    break;

    case eNotify_OnTextureLayerChange:
        ReloadLayerList();
        break;

    case eNotify_OnSplatmapImport:
        OnImportSplatMaps();
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
class TextureScriptBindings
{
public:
    //////////////////////////////////////////////////////////////////////////
    static void OpenTool()
    {
        QtViewPaneManager::instance()->OpenPane(LyViewPane::TerrainTextureLayers);
    }

    //////////////////////////////////////////////////////////////////////////
    static void CreateLayer(int index, const char* layerName)
    {
        if (!layerName)
        {
            return;
        }

        IEditor* editor = GetIEditor();

        if (!editor)
        {
            return;
        }

        CTerrainManager* terrainManager = editor->GetTerrainManager();

        if (!terrainManager)
        {
            return;
        }

        if (index < 0 || index > terrainManager->GetLayerCount())
        {
            return;
        }

        auto newLayer = CreateDefaultLayer();

        if (!newLayer)
        {
            return;
        }

        newLayer->SetLayerName(layerName);
        terrainManager->AddLayer(newLayer.release());
        terrainManager->MoveLayer(terrainManager->GetLayerCount() - 1, index);
        editor->Notify(eNotify_OnTextureLayerChange);
    }

    //////////////////////////////////////////////////////////////////////////
    static void DeleteLayer(int index)
    {
        IEditor* editor = GetIEditor();

        if (!editor)
        {
            return;
        }

        CTerrainManager* terrainManager = editor->GetTerrainManager();

        if (terrainManager)
        {
            terrainManager->RemoveLayer(GetLayer(index));
            editor->Notify(eNotify_OnTextureLayerChange);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    static void MoveLayer(int oldIndex, int newIndex)
    {
        if (oldIndex == newIndex)
        {
            return;
        }

        IEditor* editor = GetIEditor();

        if (!editor)
        {
            return;
        }

        CTerrainManager* terrainManager = editor->GetTerrainManager();

        if (terrainManager)
        {
            terrainManager->MoveLayer(oldIndex, newIndex);
            editor->Notify(eNotify_OnTextureLayerChange);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    static int GetTextureLayerIndex(const char* layerName)
    {
        constexpr int error_index = -1;

        if (!layerName)
        {
            return error_index;
        }

        IEditor* editor = GetIEditor();

        if (!editor)
        {
            return error_index;
        }

        CTerrainManager* terrainManager = editor->GetTerrainManager();

        if (!terrainManager)
        {
            return error_index;
        }

        for (int i = 0; i < terrainManager->GetLayerCount(); ++i)
        {
            if (terrainManager->GetLayer(i)->GetLayerName() == layerName)
            {
                return i;
            }
        }

        return error_index;
    }

    //////////////////////////////////////////////////////////////////////////
    static void SetMaterial(int index, const char* materialName)
    {
        if (!materialName)
        {
            return;
        }

        CLayer* layer = GetLayer(index);

        if (!layer)
        {
            return;
        }

        IEditor* editor = GetIEditor();

        if (!editor)
        {
            return;
        }

        CTerrainManager* terrainManager = editor->GetTerrainManager();

        if (!terrainManager)
        {
            return;
        }

        layer->AssignMaterial(materialName);
        editor->Notify(eNotify_OnTextureLayerChange);
        terrainManager->ReloadSurfaceTypes();
    }

    //////////////////////////////////////////////////////////////////////////
    static void SetSplatMap(int index, const char* splatMapPath)
    {
        IEditor* editor = GetIEditor();

        if (!editor)
        {
            return;
        }

        if (splatMapPath)
        {
            CLayer* layer = GetLayer(index);

            if (layer)
            {
                layer->SetSplatMapPath(splatMapPath);
                editor->Notify(eNotify_OnTextureLayerChange);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    static void ImportSplatMaps()
    {
        IEditor* editor = GetIEditor();

        if (!editor)
        {
            return;
        }

        CTerrainTextureDialog::ImportSplatMaps();
    }

    //////////////////////////////////////////////////////////////////////////
    static bool ExportSplatMap(int index, const char* filename)
    {
        QString imagePath(filename);
        IEditor* editor = GetIEditor();

        if (editor)
        {
            CLayer* layer = GetLayer(index);

            if (layer)
            {
                return CTerrainTextureDialog::ExportSplatMap(layer->GetCurrentLayerId(), imagePath);
            }
        }

        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    static void SetTextureLayerName(int index, const char* newName)
    {
        if (!newName)
        {
            return;
        }

        IEditor* editor = GetIEditor();

        if (!editor)
        {
            return;
        }

        CLayer* layer = GetLayer(index);

        if (layer)
        {
            layer->SetLayerName(newName);
            editor->Notify(eNotify_OnTextureLayerChange);
        }
    }

private:
    //////////////////////////////////////////////////////////////////////////
    static CLayer* GetLayer(int index)
    {
        IEditor* editor = GetIEditor();

        if (!editor)
        {
            return nullptr;
        }

        CTerrainManager* terrainManager = editor->GetTerrainManager();

        if (terrainManager)
        {
            return terrainManager->GetLayer(index);
        }
        else
        {
            return nullptr;
        }
    }
};

void AzToolsFramework::TerrainTexturePythonFuncsHandler::Reflect(AZ::ReflectContext* context)
{
    if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
    {
        auto addLegacyTerrain = [](AZ::BehaviorContext::GlobalMethodBuilder methodBuilder)
        {
            methodBuilder->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Legacy/Editor")
                ->Attribute(AZ::Script::Attributes::Module, "legacy.terrain_texture"); // this will put these methods into the 'azlmbr.legacy.terrain_texture' module
        };

        addLegacyTerrain(behaviorContext->Method("open_layers", TextureScriptBindings::OpenTool, nullptr, "Opens the texture layers tool."));
        addLegacyTerrain(behaviorContext->Method("create_layer", TextureScriptBindings::CreateLayer, nullptr, "Creates a new texture layer with the given index and name."));
        addLegacyTerrain(behaviorContext->Method("delete_layer", TextureScriptBindings::DeleteLayer, nullptr, "Deletes the texture layer matching the index provided."));
        addLegacyTerrain(behaviorContext->Method("move_layer", TextureScriptBindings::MoveLayer, nullptr, "Moves the texture layer matching the index provided to a new index."));
        addLegacyTerrain(behaviorContext->Method("get_layer_index", TextureScriptBindings::GetTextureLayerIndex, nullptr, "Returns the index of a named texture layer."));
        addLegacyTerrain(behaviorContext->Method("set_layer_material", TextureScriptBindings::SetMaterial, nullptr, "Sets a material to the texture layer matching the index provided."));
        addLegacyTerrain(behaviorContext->Method("set_layer_splatmap", TextureScriptBindings::SetSplatMap, nullptr, "Sets a splatmap to the texture layer matching the index provided."));
        addLegacyTerrain(behaviorContext->Method("import_layer_splatmaps", TextureScriptBindings::ImportSplatMaps, nullptr, "Imports splatmaps."));
        addLegacyTerrain(behaviorContext->Method("export_layer_splatmap", TextureScriptBindings::ExportSplatMap, nullptr, "Exports a splatmap."));
        addLegacyTerrain(behaviorContext->Method("set_layer_name", TextureScriptBindings::SetTextureLayerName, nullptr, "Renames the texture layer matching the index provided."));
    }
}

//////////////////////////////////////////////////////////////////////////
#include <TerrainTexture.moc>
