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
#include "TerrainPainterPanel.h"
#include "TerrainTexturePainter.h"

#include "CryEditDoc.h"
#include "./Terrain/Layer.h"
#include "./TerrainPainterPanel.h"

#include "Terrain/TerrainManager.h"

#include <cmath>
#include <QAbstractListModel>
#include <QSortFilterProxyModel>

#include <ui_TerrainPainterPanel.h>

Q_DECLARE_METATYPE(CLayer*);

static QColor fromColorF(const ColorF& source)
{
    return QColor::fromRgbF(source.r, source.g, source.b, source.a);
}

static ColorF fromQColor(const QColor& source)
{
    return ColorF(source.redF(), source.greenF(), source.blueF(), source.alphaF());
}

// These functions use an exponential curve set up so that for x in [0, 1.0],
// y ~ 0.1 @ x = 0.5, y ~ 0.25 @ x = 0.75, y ~ 0.75 @ x = 0.95 & y = 1 @ x = 1

static int sliderPositionFromRadius(QSlider* slider, double min, double max, double value)
{
    double y = (value - min) / (max - min);
    double x = std::log(y * (std::exp(5.0) - 1.0) + 1.0) / 5.0;
    return slider->maximum() * x;
}

static double radiusFromSliderPosition(QSlider* slider, double min, double max)
{
    double v = static_cast<double>(slider->value());
    double x = v / slider->maximum();
    return ((std::exp(5.0 * x) - 1.0) / (std::exp(5.0) - 1)) * (max - min) + min;
}


class TerrainTextureLayerModel
    : public QAbstractListModel
{
public:
    TerrainTextureLayerModel(const CTerrainManager& terrainManager, QObject* parent)
        : QAbstractListModel(parent)
        , m_terrainManager(terrainManager)
    {
    }

    virtual ~TerrainTextureLayerModel()
    {
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return m_terrainManager.GetLayerCount() + 1;
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (index.row() < 0 || index.row() > m_terrainManager.GetLayerCount())
        {
            return {};
        }

        // We generate a fake "row 0"
        if (index.row() == 0)
        {
            switch (role)
            {
            case Qt::DisplayRole:
                return tr("<none>");
            case Qt::UserRole:
            default:
                return {};
            }
        }

        CLayer* layer = m_terrainManager.GetLayer(index.row() - 1);

        switch (role)
        {
        case Qt::DisplayRole:
            return layer->GetLayerName();
        case Qt::UserRole:
            return QVariant().fromValue<CLayer*>(layer);
        default:
            return {};
        }
    }

    void Update()
    {
        beginResetModel();
        endResetModel();
    }

private:
    const CTerrainManager& m_terrainManager;
};

class NoneFilterProxyModel
    : public QSortFilterProxyModel
{
public:
    NoneFilterProxyModel(QObject* parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
    };

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override
    {
        return source_row != 0;
    }
};

/////////////////////////////////////////////////////////////////////////////
// CTerrainPainterPanel dialog

CTerrainPainterPanel::CTerrainPainterPanel(CTerrainTexturePainter& tool, QWidget* pParent /* = widget */)
    : QWidget(pParent)
    , m_ui(new Ui::TerrainPainterPanel)
    , m_model(new TerrainTextureLayerModel(*GetIEditor()->GetTerrainManager(), this))
    , m_tool(tool)
    , m_bIgnoreNotify(false)
{
    m_ui->setupUi(this);
    m_ui->maskLayerIdCombo->setModel(m_model);
    auto filter = new NoneFilterProxyModel(this);
    filter->setSourceModel(m_model);
    m_ui->layerListView->setModel(filter);
    m_ui->brushColorButton->SetColor(Qt::white);

    // Fill layers.
    ReloadLayers();

    GetIEditor()->RegisterNotifyListener(this);

    auto doubleChanged = static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);
    auto intChanged = static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged);

    connect(m_ui->brushRadiusSpin, doubleChanged, this, &CTerrainPainterPanel::UpdateTextureBrushSettings);
    connect(m_ui->brushRadiusSlider, &QSlider::valueChanged, this, &CTerrainPainterPanel::OnSliderChange);
    connect(m_ui->brushColorHardnessSpin, doubleChanged, this, &CTerrainPainterPanel::UpdateTextureBrushSettings);
    connect(m_ui->brushColorHardnessSlider, &QSlider::valueChanged, this, &CTerrainPainterPanel::OnSliderChange);
    connect(m_ui->brushDetailHardnessSpin, doubleChanged, this, &CTerrainPainterPanel::UpdateTextureBrushSettings);
    connect(m_ui->brushDetailHardnessSlider, &QSlider::valueChanged, this, &CTerrainPainterPanel::OnSliderChange);
    connect(m_ui->paintMaskByLayerSettingsCheck, &QCheckBox::stateChanged, this, &CTerrainPainterPanel::UpdateTextureBrushSettings);
    connect(m_ui->maskLayerIdCombo, &QComboBox::currentTextChanged, this, &CTerrainPainterPanel::UpdateTextureBrushSettings);
    connect(m_ui->brushColorButton, &ColorButton::ColorChanged, this, &CTerrainPainterPanel::UpdateTextureBrushSettings);

    connect(m_ui->brushBrightnessSlider, &QSlider::valueChanged, this, &CTerrainPainterPanel::OnSliderChange);
    connect(m_ui->resetBrightnessButton, &QPushButton::clicked, this, &CTerrainPainterPanel::OnBrushResetBrightness);
    connect(m_ui->saveLayerButton, &QPushButton::clicked, this, &CTerrainPainterPanel::OnBnClickedBrushSettolayer);

    connect(m_ui->brushLayerAltitudeMin, intChanged, this, &CTerrainPainterPanel::GetLayerMaskSettingsFromLayer);
    connect(m_ui->brushLayerAltitudeMax, intChanged, this, &CTerrainPainterPanel::GetLayerMaskSettingsFromLayer);
    connect(m_ui->brushLayerSlopeMin, intChanged, this, &CTerrainPainterPanel::GetLayerMaskSettingsFromLayer);
    connect(m_ui->brushLayerSlopeMax, intChanged, this, &CTerrainPainterPanel::GetLayerMaskSettingsFromLayer);

    connect(m_ui->layerListView, &QListView::doubleClicked, this, &CTerrainPainterPanel::OnLayersDblClk);
    connect(m_ui->layerListView, &QListView::clicked, this, &CTerrainPainterPanel::OnLayersClick);
    connect(m_ui->layerFloodButton, &QPushButton::clicked, this, &CTerrainPainterPanel::OnFloodLayer);
}

CTerrainPainterPanel::~CTerrainPainterPanel()
{
    GetIEditor()->UnregisterNotifyListener(this);
}

/////////////////////////////////////////////////////////////////////////////
// CTerrainPainterPanel message handlers

//////////////////////////////////////////////////////////////////////////
void CTerrainPainterPanel::SetBrush(CTextureBrush& br)
{
    QSignalBlocker brushRadiusSpinBlocker(m_ui->brushRadiusSpin);
    QSignalBlocker brushColorHardnessSpinBlocker(m_ui->brushColorHardnessSpin);
    QSignalBlocker brushDetailHardnessSpinBlocker(m_ui->brushDetailHardnessSpin);
    QSignalBlocker paintMaskByLayerSettingsCheckBlocker(m_ui->paintMaskByLayerSettingsCheck);
    QSignalBlocker brushRadiusSliderBlocker(m_ui->brushRadiusSlider);
    QSignalBlocker brushColorHardnessSliderBlocker(m_ui->brushColorHardnessSlider);
    QSignalBlocker brushDetailHardnessSliderBlocker(m_ui->brushDetailHardnessSlider);
    QSignalBlocker brushBrightnessSliderBlocker(m_ui->brushBrightnessSlider);

    m_ui->brushRadiusSpin->setRange(br.minRadius, br.maxRadius);
    m_ui->brushRadiusSpin->setValue(br.radius);
    m_ui->brushColorHardnessSpin->setValue(br.colorHardness);
    m_ui->brushDetailHardnessSpin->setValue(br.detailHardness);
    m_ui->paintMaskByLayerSettingsCheck->setChecked(br.bMaskByLayerSettings ? Qt::Checked : Qt::Unchecked);
    m_ui->brushColorButton->SetColor(fromColorF(br.m_cFilterColor));

    m_ui->brushRadiusSlider->setValue(sliderPositionFromRadius(m_ui->brushRadiusSlider, br.minRadius, br.maxRadius, br.radius));
    m_ui->brushColorHardnessSlider->setValue(br.colorHardness * 100.0);
    m_ui->brushDetailHardnessSlider->setValue(br.detailHardness * 100.0);
    m_ui->brushBrightnessSlider->setValue(br.m_fBrightness * 255.0);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainPainterPanel::OnSliderChange()
{
    CTextureBrush br;
    m_tool.GetBrush(br);
    br.colorHardness = m_ui->brushColorHardnessSlider->value() / 100.0f;
    br.detailHardness = m_ui->brushDetailHardnessSlider->value() / 100.0f;
    br.m_fBrightness = m_ui->brushBrightnessSlider->value() / 255.0f;
    br.radius = radiusFromSliderPosition(m_ui->brushRadiusSlider, br.minRadius, br.maxRadius);
    SetBrush(br);
    m_tool.SetBrush(br);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainPainterPanel::SelectLayer(const CLayer* pLayer)
{
    for (int i = 0; i < GetIEditor()->GetTerrainManager()->GetLayerCount(); i++)
    {
        CLayer* pLayerIt = GetIEditor()->GetTerrainManager()->GetLayer(i);
        pLayerIt->SetSelected(pLayerIt == pLayer);
    }

    ReloadLayers();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainPainterPanel::ReloadLayers()
{
    m_model->Update();

    m_ui->maskLayerIdCombo->setCurrentIndex(m_ui->maskLayerIdCombo->findText(tr("<none>")));

    CTextureBrush br;
    m_tool.GetBrush(br);
    br.m_dwMaskLayerId = 0xffffffff;
    m_tool.SetBrush(br);

    // Restore the previously selected layer, or select the default if this is
    // the first time opening the terrain layer painter
    int rows = m_ui->layerListView->model()->rowCount();
    if (rows > 0)
    {
        int layerIndex = 0;
        for (int i = 0; i < GetIEditor()->GetTerrainManager()->GetLayerCount(); ++i)
        {
            CLayer* layer = GetIEditor()->GetTerrainManager()->GetLayer(i);
            if (layer->IsSelected())
            {
                layerIndex = i;
                break;
            }
        }

        QModelIndex index = m_ui->layerListView->model()->index(layerIndex, 0, {});
        m_ui->layerListView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
    }

    SetLayerMaskSettingsToLayer();
}

//////////////////////////////////////////////////////////////////////////
CLayer* CTerrainPainterPanel::GetSelectedLayer()
{
    auto selection = m_ui->layerListView->selectionModel()->selectedIndexes();
    if (selection.isEmpty())
    {
        return nullptr;
    }
    return selection.first().data(Qt::UserRole).value<CLayer*>();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainPainterPanel::OnBrushResetBrightness()
{
    m_ui->brushBrightnessSlider->setValue(127);

    CTextureBrush br;
    m_tool.GetBrush(br);
    br.m_fBrightness = 0.5;
    SetBrush(br);
    m_tool.SetBrush(br);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainPainterPanel::UpdateTextureBrushSettings()
{
    CTextureBrush br;
    m_tool.GetBrush(br);

    br.bMaskByLayerSettings = m_ui->paintMaskByLayerSettingsCheck->isChecked();
    br.radius = m_ui->brushRadiusSpin->value();
    br.colorHardness = m_ui->brushColorHardnessSpin->value();
    br.detailHardness = m_ui->brushDetailHardnessSpin->value();
    br.m_cFilterColor = fromQColor(m_ui->brushColorButton->Color());

    br.m_dwMaskLayerId = 0xffffffff;

    {
        CLayer* current = m_ui->maskLayerIdCombo->currentData().value<CLayer*>();
        if (nullptr != current)
        {
            br.m_dwMaskLayerId = current->GetOrRequestLayerId();
        }
    }

    SetBrush(br);

    m_tool.SetBrush(br);
}



void CTerrainPainterPanel::SetLayerMaskSettingsToLayer()
{
    QSignalBlocker brushLayerAltitudeMinBlocker(m_ui->brushLayerAltitudeMin);
    QSignalBlocker brushLayerAltitudeMaxBlocker(m_ui->brushLayerAltitudeMax);
    QSignalBlocker brushLayerSlopeMinBlocker(m_ui->brushLayerSlopeMin);
    QSignalBlocker brushLayerSlopeMaxBlocker(m_ui->brushLayerSlopeMax);
    QSignalBlocker brushColorButtonBlocker(m_ui->brushColorButton);
    QSignalBlocker brushBrightnessSliderBlocker(m_ui->brushBrightnessSlider);

    CLayer* pLayer = GetSelectedLayer();

    m_ui->brushLayerAltitudeMin->setEnabled(pLayer != 0);
    m_ui->brushLayerAltitudeMax->setEnabled(pLayer != 0);
    m_ui->brushLayerSlopeMin->setEnabled(pLayer != 0);
    m_ui->brushLayerSlopeMax->setEnabled(pLayer != 0);

    if (pLayer)
    {
        m_ui->brushLayerAltitudeMin->setValue(pLayer->GetLayerStart());
        m_ui->brushLayerAltitudeMax->setValue(pLayer->GetLayerEnd());
        m_ui->brushLayerSlopeMin->setValue(pLayer->GetLayerMinSlopeAngle());
        m_ui->brushLayerSlopeMax->setValue(pLayer->GetLayerMaxSlopeAngle());

        CTextureBrush br;
        m_tool.GetBrush(br);
        br.m_cFilterColor = pLayer->m_cLayerFilterColor;
        br.m_fBrightness = pLayer->m_fLayerBrightness;
        m_tool.SetBrush(br);

        m_ui->brushColorButton->SetColor(fromColorF(br.m_cFilterColor));
        m_ui->brushBrightnessSlider->setValue(br.m_fBrightness * 255.0f);
    }
    else
    {
        m_ui->brushLayerAltitudeMin->setValue(0);
        m_ui->brushLayerAltitudeMax->setValue(255);
        m_ui->brushLayerSlopeMin->setValue(0);
        m_ui->brushLayerSlopeMax->setValue(90);
    }

    CCryEditDoc* pDoc = GetIEditor()->GetDocument();
    for (int i = 0; i < GetIEditor()->GetTerrainManager()->GetLayerCount(); i++)
    {
        GetIEditor()->GetTerrainManager()->GetLayer(i)->SetSelected(false);
    }
    if (pLayer)
    {
        pLayer->SetSelected(true);
    }

    m_bIgnoreNotify = true;
    GetIEditor()->Notify(eNotify_OnSelectionChange);
    m_bIgnoreNotify = false;
}


void CTerrainPainterPanel::OnLayersDblClk()
{
    GetIEditor()->OpenView(LyViewPane::TerrainTextureLayers);
}

void CTerrainPainterPanel::OnLayersClick()
{
    SetLayerMaskSettingsToLayer();
}


void CTerrainPainterPanel::GetLayerMaskSettingsFromLayer()
{
    CLayer* pLayer = GetSelectedLayer();

    if (pLayer)
    {
        pLayer->SetLayerStart(m_ui->brushLayerAltitudeMin->value());
        pLayer->SetLayerEnd(m_ui->brushLayerAltitudeMax->value());
        pLayer->SetLayerMinSlopeAngle(m_ui->brushLayerSlopeMin->value());
        pLayer->SetLayerMaxSlopeAngle(m_ui->brushLayerSlopeMax->value());
        GetIEditor()->GetDocument()->SetModifiedFlag(true);
        GetIEditor()->SetModifiedModule(eModifiedTerrain);
    }

    m_bIgnoreNotify = true;
    GetIEditor()->Notify(eNotify_OnInvalidateControls);
    m_bIgnoreNotify = false;
}




void CTerrainPainterPanel::OnBnClickedBrushSettolayer()
{
    CLayer* pLayer = GetSelectedLayer();

    if (!pLayer)
    {
        return;
    }

    CTextureBrush br;
    m_tool.GetBrush(br);

    pLayer->m_cLayerFilterColor = br.m_cFilterColor;
    pLayer->m_fLayerBrightness = br.m_fBrightness;

    m_bIgnoreNotify = true;
    GetIEditor()->Notify(eNotify_OnInvalidateControls);
    m_bIgnoreNotify = false;
}

void CTerrainPainterPanel::OnFloodLayer()
{
    m_tool.Action_StopUndo();
    m_tool.Action_Flood();
    m_tool.Action_StopUndo();
}

/*
//////////////////////////////////////////////////////////////////////////
void CTerrainPainterPanel::OnExport()
{
    m_tool->ImportExport(false, false);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainPainterPanel::OnImport()
{
    m_tool->ImportExport(true, false);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainPainterPanel::OnFileBrowse()
{
    gSettings.BrowseTerrainTexture();
    GetDlgItem(IDC_FILE)->SetWindowText( gSettings.terrainTextureExport);
}
*/

//////////////////////////////////////////////////////////////////////////
void CTerrainPainterPanel::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    if (m_bIgnoreNotify)
    {
        return;
    }
    switch (event)
    {
    case eNotify_OnInvalidateControls:
        ReloadLayers();
        break;
    case eNotify_OnSelectionChange:
    {
        CCryEditDoc* pDoc = GetIEditor()->GetDocument();
        for (int i = 0; i < GetIEditor()->GetTerrainManager()->GetLayerCount(); i++)
        {
            if (GetIEditor()->GetTerrainManager()->GetLayer(i)->IsSelected())
            {
                SelectLayer(GetIEditor()->GetTerrainManager()->GetLayer(i));
                break;
            }
        }
    }
    break;
    }
}

#include <TerrainPainterPanel.moc>