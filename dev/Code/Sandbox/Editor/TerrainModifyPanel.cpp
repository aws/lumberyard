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

#include "stdafx.h"
#include "TerrainModifyPanel.h"
#include "TerrainModifyTool.h"
#include "Util/BoostPythonHelpers.h"
#include <Cry_Math.h>
#include <Util/GdiUtil.h>

#include <ui_TerrainModifyPanel.h>

static const char* BRUSH_TYPE_FLATTEN = "Flatten";
static const char* BRUSH_TYPE_SMOOTH = "Smooth";
static const char* BRUSH_TYPE_RISE_LOWER = "Rise/Lower";
static const char* BRUSH_TYPE_PICK_HEIGHT = "Pick Height";

/////////////////////////////////////////////////////////////////////////////
// CTerrainModifyPanel dialog

CTerrainModifyPanel::CTerrainModifyPanel(QWidget* parent /* = nullptr */)
    : QWidget(parent)
    , m_ui(new Ui::TerrainModifyPanel)
{
    OnInitDialog();
}

/////////////////////////////////////////////////////////////////////////////

void CTerrainModifyPanel::SetModifyTool(CTerrainModifyTool* tool)
{
    m_tool = tool;
}

//////////////////////////////////////////////////////////////////////////

void CTerrainModifyPanel::OnInitDialog()
{
    m_ui->setupUi(this);

    connect(m_ui->flattenButton, &QAbstractButton::clicked, [=]() { OnBrushTypeCmd(tr(BRUSH_TYPE_FLATTEN)); });
    connect(m_ui->smoothButton, &QAbstractButton::clicked, [=]() { OnBrushTypeCmd(tr(BRUSH_TYPE_SMOOTH)); });
    connect(m_ui->riseLowerButton, &QAbstractButton::clicked, [=]() { OnBrushTypeCmd(tr(BRUSH_TYPE_RISE_LOWER)); });
    connect(m_ui->pickHeightButton, &QAbstractButton::clicked, [=]() { OnBrushTypeCmd(tr(BRUSH_TYPE_PICK_HEIGHT)); });

    auto qSpinBoxValueChangedInt = static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged);
    auto qSpinBoxValueChangedDouble = static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);

    // Sliders are subordinate to spinboxes

    // Brush outside radius
    connect(m_ui->brushOutsideRadiusSlider, &QSlider::valueChanged, [=](int i)
        {
            SyncWidgetValue(m_ui->brushOutsideRadiusSpin, std::pow(10.0, i / 100.0));
        });
    connect(m_ui->brushOutsideRadiusSpin, qSpinBoxValueChangedDouble, [=](double d)
        {
            SyncWidgetValue(m_ui->brushOutsideRadiusSlider, std::round(std::log10(std::max(d, 0.01)) * 100.0));
        });

    connect(m_ui->syncRadiusCheck, &QCheckBox::stateChanged, this, &CTerrainModifyPanel::OnSyncRadius);

    // Brush inner radius
    connect(m_ui->brushInsideRadiusSlider, &QSlider::valueChanged, [=](int i)
        {
            // Since this can go to zero and 10^0 = 1, we special-case zero
            SyncWidgetValue(m_ui->brushInsideRadiusSpin, i == 0 ? 0 : std::pow(10.0, i / 100.0));
        });
    connect(m_ui->brushInsideRadiusSpin, qSpinBoxValueChangedDouble, [=](double d)
        {
            SyncWidgetValue(m_ui->brushInsideRadiusSlider, std::round(std::log10(std::max(d, 0.01)) * 100.0));
        });

    // Brush hardness
    connect(m_ui->brushHardnessSlider, &QSlider::valueChanged, [=](int i)
        {
            SyncWidgetValue(m_ui->brushHardnessSpin, i / 100.0);
        });
    connect(m_ui->brushHardnessSpin, qSpinBoxValueChangedDouble, [=](double d)
        {
            SyncWidgetValue(m_ui->brushHardnessSlider, d * 100);
        });

    // Brush "height"
    connect(m_ui->brushHeightSlider, &QSlider::valueChanged, [=](int i)
        {
            SyncWidgetValue(m_ui->brushHeightSpin, i);
        });
    connect(m_ui->brushHeightSpin, qSpinBoxValueChangedDouble, [=](double d)
        {
            SyncWidgetValue(m_ui->brushHeightSlider, d);
        });

    connect(m_ui->enableNoiseCheck, &QCheckBox::stateChanged, this, &CTerrainModifyPanel::OnBrushNoise);

    // Brush noise scale
    connect(m_ui->noiseScaleSlider, &QSlider::valueChanged, [=](int i)
        {
            SyncWidgetValue(m_ui->noiseScaleSpin, i);
        });
    connect(m_ui->noiseScaleSpin, qSpinBoxValueChangedInt, [=](int i)
        {
            SyncWidgetValue(m_ui->noiseScaleSlider, i);
        });

    // Brush noise frequency
    connect(m_ui->noiseFreqSlider, &QSlider::valueChanged, [=](int i)
        {
            SyncWidgetValue(m_ui->noiseFreqSpin, i);
        });
    connect(m_ui->noiseFreqSpin, qSpinBoxValueChangedInt, [=](int i)
        {
            SyncWidgetValue(m_ui->noiseFreqSlider, i);
        });

    connect(m_ui->repositionObjectsCheck, &QCheckBox::stateChanged, this, &CTerrainModifyPanel::OnRepositionObjects);
    connect(m_ui->repositionVegetationCheck, &QCheckBox::stateChanged, this, &CTerrainModifyPanel::OnRepositionVegetation);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyPanel::OnBrushTypeCmd(const QString& brushType)
{
    if (m_inSyncCallback)
    {
        return;
    }

    m_tool->ClearCtrlPressedState();
    if (brushType == tr(BRUSH_TYPE_FLATTEN))
    {
        GetIEditor()->ExecuteCommand("terrain.set_tool_flatten");
    }
    else if (brushType == tr(BRUSH_TYPE_SMOOTH))
    {
        GetIEditor()->ExecuteCommand("terrain.set_tool_smooth");
    }
    else if (brushType == tr(BRUSH_TYPE_RISE_LOWER))
    {
        GetIEditor()->ExecuteCommand("terrain.set_tool_riselower");
    }
    else if (brushType == tr(BRUSH_TYPE_PICK_HEIGHT))
    {
        m_tool->SetCurBrushType(eBrushPickHeight);
    }

    CTerrainBrush br;
    m_tool->GetCurBrushParams(br);
    m_ui->brushPreview->SetBrush(br);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyPanel::SetBrush(CTerrainBrush* pBrush, bool bSyncRadiuses)
{
    // This simply stops OnUpdateNumbers from doing anything (breaks the feedback loop)
    m_inSyncCallback = true;

    if (pBrush->type != eBrushPickHeight)
    {
        EnableRadius(true);
        EnableHardness(true);
    }

    // Disable height slider because there is no height contribution when noise is enabled
    // Or when there is no range to play with anyways
    bool validHeightRange = !pBrush->bNoise && (!pBrush->heightRange.IsZeroFast());

    m_ui->brushHeightSpin->setRange(pBrush->heightRange.x, pBrush->heightRange.y);
    m_ui->brushHeightSlider->setRange(pBrush->heightRange.x, pBrush->heightRange.y);
    if (pBrush->type == eBrushFlatten)
    {
        m_ui->flattenButton->setChecked(true);
        EnableRadiusInner(true);
        EnableHeight(validHeightRange);
        m_ui->enableNoiseCheck->setEnabled(true);
    }
    else if (pBrush->type == eBrushSmooth)
    {
        m_ui->smoothButton->setChecked(true);
        pBrush->bNoise = false;
        EnableRadiusInner(false);
        EnableHeight(false);
        m_ui->enableNoiseCheck->setEnabled(false);
    }
    else if (pBrush->type == eBrushRiseLower)
    {
        m_ui->riseLowerButton->setChecked(true);
        EnableRadiusInner(true);
        EnableHeight(validHeightRange);
        m_ui->enableNoiseCheck->setEnabled(true);
    }
    else if (pBrush->type == eBrushPickHeight)
    {
        m_ui->pickHeightButton->setChecked(true);
        EnableRadius(false);
        EnableRadiusInner(false);
        EnableHardness(false);
        EnableHeight(validHeightRange);
        m_ui->enableNoiseCheck->setEnabled(false);
    }
    m_ui->brushOutsideRadiusSpin->setValue(pBrush->radius);
    m_ui->brushInsideRadiusSpin->setValue(pBrush->radiusInside);
    m_ui->brushHeightSpin->setValue(pBrush->height);
    m_ui->brushHardnessSpin->setValue(pBrush->hardness);
    m_ui->noiseScaleSpin->setValue(pBrush->noiseScale);
    m_ui->noiseFreqSpin->setValue(pBrush->noiseFreq);

    m_ui->noiseScaleSpin->setEnabled(pBrush->bNoise);
    m_ui->noiseFreqSpin->setEnabled(pBrush->bNoise);
    m_ui->noiseScaleSlider->setEnabled(pBrush->bNoise);
    m_ui->noiseFreqSlider->setEnabled(pBrush->bNoise);

    m_ui->enableNoiseCheck->setChecked(pBrush->bNoise);
    m_ui->repositionObjectsCheck->setChecked(pBrush->bRepositionObjects);
    m_ui->repositionVegetationCheck->setChecked(pBrush->bRepositionVegetation);

    m_ui->syncRadiusCheck->setChecked(bSyncRadiuses);

    m_ui->brushPreview->SetBrush(*pBrush);

    m_inSyncCallback = false;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyPanel::OnUpdateNumbers()
{
    if (m_inSyncCallback)
    {
        return;
    }

    CTerrainBrush br;
    m_tool->GetCurBrushParams(br);
    float prevRadius = br.radius;
    float prevRadiusInside = br.radiusInside;
    br.radius = m_ui->brushOutsideRadiusSpin->value();
    br.radiusInside = m_ui->brushInsideRadiusSpin->value();
    if (br.radius < br.radiusInside)
    {
        if (prevRadiusInside != br.radiusInside) // Check if changing inside radius.
        {
            br.radius = br.radiusInside;
        }
        else
        {
            br.radiusInside = br.radius; // Changing outside radius;
        }
    }
    br.height = m_ui->brushHeightSpin->value();
    br.hardness = m_ui->brushHardnessSpin->value();
    br.noiseFreq = m_ui->noiseFreqSpin->value();
    br.noiseScale = m_ui->noiseScaleSpin->value();
    m_tool->SetCurBrushParams(br);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyPanel::OnBrushNoise()
{
    CTerrainBrush br;
    m_tool->GetCurBrushParams(br);
    br.bNoise = m_ui->enableNoiseCheck->isChecked();
    m_tool->SetCurBrushParams(br);
}


//////////////////////////////////////////////////////////////////////////
void CTerrainModifyPanel::OnSyncRadius()
{
    m_tool->SyncBrushRadiuses(m_ui->syncRadiusCheck->isChecked());
}

void CTerrainModifyPanel::OnRepositionObjects()
{
    CTerrainBrush br;
    m_tool->GetCurBrushParams(br);
    br.bRepositionObjects = m_ui->repositionObjectsCheck->isChecked();
    m_tool->SetCurBrushParams(br);
}

void CTerrainModifyPanel::OnRepositionVegetation()
{
    CTerrainBrush br;
    m_tool->GetCurBrushParams(br);
    br.bRepositionVegetation = m_ui->repositionVegetationCheck->isChecked();
    m_tool->SetCurBrushParams(br);
}


void CTerrainModifyPanel::EnableRadius(bool isEnable)
{
    m_ui->brushOutsideRadiusSlider->setEnabled(isEnable);
    m_ui->brushOutsideRadiusSpin->setEnabled(isEnable);
}

void CTerrainModifyPanel::EnableRadiusInner(bool isEnable)
{
    m_ui->brushInsideRadiusSlider->setEnabled(isEnable);
    m_ui->brushInsideRadiusSpin->setEnabled(isEnable);
}

void CTerrainModifyPanel::EnableHardness(bool isEnable)
{
    m_ui->brushHardnessSlider->setEnabled(isEnable);
    m_ui->brushHardnessSpin->setEnabled(isEnable);
}

void CTerrainModifyPanel::EnableHeight(bool isEnable)
{
    m_ui->brushHeightSlider->setEnabled(isEnable);
    m_ui->brushHeightSpin->setEnabled(isEnable);
}

//////////////////////////////////////////////////////////////////////////
// Brush Preview Picture Box
CBrushPreviewPictureBox::CBrushPreviewPictureBox(QWidget* parent /* = nullptr */)
    : QLabel(parent)
{
    setUpdatesEnabled(true);
}

QSize CBrushPreviewPictureBox::sizeHint() const
{
    return {
               32, 32
    };
}

void CBrushPreviewPictureBox::render(const QSize& bounds)
{
#ifdef KDAB_MAC_PORT
    using namespace Gdiplus;
    int diameter = min(bounds.height(), bounds.width());
    Rect clipRect = Rect(0, 0, diameter, diameter);
    Bitmap buffer(diameter, diameter);
    std::unique_ptr<Graphics> bufferGraphics(Graphics::FromImage(&buffer));
    if (!bufferGraphics)
    {
        // not likely to get here but the API says Graphics::FromImage could return null
        return;
    }

    CheckerboardFillRect(bufferGraphics.get(), clipRect, diameter / 6, Color::DarkSlateGray, Color::LightSlateGray);

    // when it's pick height mode, the radius is 0 and we don't need to provide the brush preview
    if (m_brush.radius > 0.f)
    {
        GraphicsPath path;
        path.AddEllipse(0, 0, diameter, diameter);
        PathGradientBrush pthGrBrush(&path);

        // Linear rolloff makes it too hard to see brush under 30% hardness so using a sin ease.
        BYTE alphaValue = static_cast<BYTE>(255.f * sin(m_brush.hardness * (gf_PI / 2.f)));
        BYTE brightnessValue = 0;
        if (m_brush.heightRange.y - m_brush.heightRange.x > FLT_EPSILON)
        {
            brightnessValue = static_cast<BYTE>((m_brush.height - m_brush.heightRange.x) / (m_brush.heightRange.y - m_brush.heightRange.x) * 255.0f);
        }
        Color presetColors[] = // colors are arranged from edge to center of a circle
        {
            {0, 255, 255, 255},
            {alphaValue, brightnessValue, brightnessValue, brightnessValue},
            {alphaValue, brightnessValue, brightnessValue, brightnessValue}
        };
        REAL interpPositions[] = {0.0f, 1.0f - m_brush.radiusInside / m_brush.radius, 1.0f};
        pthGrBrush.SetInterpolationColors(presetColors, interpPositions, 3);
        bufferGraphics->FillEllipse(&pthGrBrush, 0, 0, diameter, diameter);
    }

    QImage image(diameter, diameter, QImage::Format_ARGB32);

    BitmapData data;
    buffer.LockBits(&clipRect, ImageLockModeRead, PixelFormat32bppARGB, &data);

    for (int y = 0; y < bounds.height(); y++)
    {
        memcpy(image.scanLine(y), static_cast<uint8_t*>(data.Scan0) + y * data.Stride, data.Stride);
    }
    buffer.UnlockBits(&data);

    setPixmap(QPixmap::fromImage(image));
#endif // KDAB_MAC_PORT
}

void CBrushPreviewPictureBox::SetBrush(const CTerrainBrush& brush)
{
    m_brush = brush;
    m_brush.radiusInside = clamp_tpl(m_brush.radiusInside, 0.f, m_brush.radius);
    m_brush.hardness = clamp_tpl(m_brush.hardness, 0.f, 1.f);
    if (m_brush.heightRange.y < m_brush.heightRange.x)
    {
        std::swap(m_brush.heightRange.y, m_brush.heightRange.x);
    }
    m_brush.height = clamp_tpl(m_brush.height, m_brush.heightRange.x, m_brush.heightRange.y);

    render(size());
}

#include <TerrainModifyPanel.moc>
