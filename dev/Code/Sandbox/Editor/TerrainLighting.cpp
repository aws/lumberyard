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

// Description : implementation file


#include "StdAfx.h"
#include "TerrainLighting.h"
#include "CryEditDoc.h"
#include "WaitProgress.h"
#include "GameEngine.h"
#include "Mission.h"
#include "QtUtilWin.h"
#include "QtViewPaneManager.h"
#include "ITimeOfDay.h"
#include <AzQtComponents/Components/Widgets/Slider.h>

#ifdef LY_TERRAIN_EDITOR
#include "Terrain/TerrainTexGen.h"
#endif // #ifdef LY_TERRAIN_EDITOR

#if defined(Q_OS_WIN)
#include <QtWinExtras/QtWin>
#endif
#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QTime>
#include <QTimer>

#include <ui_TerrainLighting.h>

#define LIGHTING_PREVIEW_RESOLUTION 256
#define SLIDER_SCALE 100.0f

int MOON_SUN_TRANS_WIDTH = 400;
int MOON_SUN_TRANS_HEIGHT = 15;


// put the linear itnerpolation between a and b by s into the res color
static void lerpFloat(QColor& res, const QColor& ca, const QColor& cb, float s)
{
    res.setRedF(ca.redF() + s * (cb.redF() - ca.redF()));
    res.setGreenF(ca.greenF() + s * (cb.greenF() - ca.greenF()));
    res.setBlueF(ca.blueF() + s * (cb.blueF() - ca.blueF()));
    res.setAlphaF(ca.alphaF() + s * (cb.alphaF() - ca.alphaF()));
}

static QString convertTimeToTimeOfDayString(float time)
{
    int nHour = floor(time);
    int nMins = roundf((time - floor(time)) * 60.0f);

    QTime t(nHour, nMins);
    return t.isValid() ? t.toString("HH:mm") : QString("24:00");
}

static bool convertTimeOfDayStringToTime(const QString& tod, float& out)
{
    QTime time = QTime::fromString(tod, "HH:mm");
    if (!time.isValid())
    {
        return false;
    }

    float hour = time.hour();
    float min = time.minute() / 60.0f;
    out = hour + min;
    return true;
}

static bool convertTransitionTimeStringToMinutes(const QString& timeString, int& out)
{
    QTime time = QTime::fromString(timeString, "HH:mm");
    if (!time.isValid())
    {
        return false;
    }

    int hours = (time.hour() > 12) ? time.hour() - 12 : time.hour();
    int mins = time.minute();
    out = (hours * 60 + mins);
    return true;
}

static bool convertMinutesToTransitionTimeString(int minutesTotal, bool onlyPM, QString& out)
{
    int hoursInt = (onlyPM) ? (minutesTotal / 60)  + 12 : minutesTotal / 60;
    int minsInt = minutesTotal % 60;

    QTime time(hoursInt, minsInt);

    if (!time.isValid())
    {
        return false;
    }

    out = time.toString("HH:mm");
    return true;
}

CTerrainLighting::CTerrainLighting(QWidget* pParent)
    : QWidget(pParent)
    , ui(new Ui::TerrainLighting)
{
    ui->setupUi(this);

    //  m_bTerrainShadows = FALSE;
    //  m_sldSkyBrightening=0;

    ////////////////////////////////////////////////////////////////////////
    // Initialize the dialog with the settings from the document
    ////////////////////////////////////////////////////////////////////////
#ifdef LY_TERRAIN_EDITOR
    m_pTexGen = nullptr;
    InitTerrainTexGen();
#endif // #ifdef LY_TERRAIN_EDITOR

    m_originalLightSettings = *GetIEditor()->GetDocument()->GetLighting();
    GetIEditor()->RegisterNotifyListener(this);

    //CLogFile::WriteLine("Loading lighting dialog..."); // TODO: portibng log ?
    ui->TOP_LEVEL_WIDGET->setMinimumWidth(900);
    ui->TOP_LEVEL_WIDGET->setMinimumHeight(470);

    ui->SUN_DIRECTION->setMinimum(0);
    ui->SUN_DIRECTION->setMaximum(360);
    ui->SUN_DIRECTION->setValue(240);

    ui->SUN_MAP_LONGITUDE->setMinimum(0);
    ui->SUN_MAP_LONGITUDE->setMaximum(180);
    ui->SUN_MAP_LONGITUDE->setValue(90);

    ui->DAWN_SLIDER->setMinimum(0);
    ui->DAWN_SLIDER->setMaximum(12 * 60);

    ui->DAWN_DUR_SLIDER->setMinimum(0);
    ui->DAWN_DUR_SLIDER->setMaximum(60);

    // Maximum for dusk is 23:59
    ui->DUSK_SLIDER->setMinimum(0);
    ui->DUSK_SLIDER->setMaximum((12 * 60) - 1);

    ui->DUSK_DUR_SLIDER->setMinimum(0);
    ui->DUSK_DUR_SLIDER->setMaximum(60);

    // Maximum time is 23:59 converted to a float scale
    static const float maxTime = ((24 * 60) - 1) / 60.0f;
    ui->LIGHTING_TIME_OF_DAY->setMinimum(0);
    ui->LIGHTING_TIME_OF_DAY->setMaximum(maxTime * SLIDER_SCALE);

    ui->DAWN_DUR_INFO->setMinimum(0);
    ui->DAWN_DUR_INFO->setMaximum(60);
    ui->DAWN_DUR_INFO->setSingleStep(1);

    ui->DUSK_DUR_INFO->setMinimum(0);
    ui->DUSK_DUR_INFO->setMaximum(60);
    ui->DUSK_DUR_INFO->setSingleStep(1);

    ui->LIGHTING_SUNDIR_EDIT->setMinimum(0);
    ui->LIGHTING_SUNDIR_EDIT->setMaximum(360);
    ui->LIGHTING_SUNDIR_EDIT->setSingleStep(1);

    ui->LIGHTING_POLE_EDIT->setMinimum(0);
    ui->LIGHTING_POLE_EDIT->setMaximum(180);
    ui->LIGHTING_POLE_EDIT->setSingleStep(1);

    QRegExpValidator* timeValidator = new QRegExpValidator(QRegExp(QStringLiteral("^(?:\\d|[01]\\d|2[0-3]):[0-5]\\d$")), this);

    ui->TIME_OF_DAY_EDIT->setValidator(timeValidator);
    ui->DAWN_INFO->setValidator(timeValidator);
    ui->DUSK_INFO->setValidator(timeValidator);

    ui->SKY_QUALITY->setMinimum(0);
    ui->SKY_QUALITY->setMaximum(10);

    ui->FORCE_SKY_UPDATE->setChecked(gSettings.bForceSkyUpdate);

    m_moonSunTransitionPreview = QPixmap(ui->MOONSUNSHADOWTRANSITIONGPBOX->width(), MOON_SUN_TRANS_HEIGHT);
    ui->MOON_SUN_TRANSITION_PREVIEW_LABEL->setMinimumHeight(MOON_SUN_TRANS_HEIGHT);
    m_moonSunTransitionPreview.fill(QColor(Qt::magenta));
    ui->MOON_SUN_TRANSITION_PREVIEW_LABEL->setPixmap(m_moonSunTransitionPreview);

    m_lightmap.Allocate(LIGHTING_PREVIEW_RESOLUTION, LIGHTING_PREVIEW_RESOLUTION); // initialization of m_lightmap resolution with a power of two

    m_sunPathPreview = QPixmap(LIGHTING_PREVIEW_RESOLUTION, LIGHTING_PREVIEW_RESOLUTION);
    ui->SUN_PATH_PREVIEW_LABEL->setMinimumSize(QSize(LIGHTING_PREVIEW_RESOLUTION, LIGHTING_PREVIEW_RESOLUTION));
    ui->SUN_PATH_PREVIEW_LABEL->setAspectRatioMode(Qt::KeepAspectRatio); // own function that will keep it square
    m_sunPathPreview.fill(QColor(Qt::blue));
    ui->SUN_PATH_PREVIEW_LABEL->setPixmap(m_sunPathPreview);

    // Synchronize the controls with the values from the document, need to be done before connecting all the ui at dialog launch
    UpdateControls();

    connect(ui->LIGHTING_TYPE_DYNAMIC_SUN, &QRadioButton::clicked, this, &CTerrainLighting::OnDynamicSun);
    connect(ui->LIGHTING_TYPE_PRECISE, &QRadioButton::clicked, this, &CTerrainLighting::OnPrecise);

    connect(ui->LIGHTING_TIME_OF_DAY, &AzQtComponents::SliderInt::valueChanged, this, &CTerrainLighting::OnLightingTimeOfDaySlider);
    connect(ui->TIME_OF_DAY_EDIT, &QLineEdit::editingFinished, this, &CTerrainLighting::UpdateScrollBarsFromEdits);

    connect(ui->SUN_DIRECTION, &AzQtComponents::SliderInt::valueChanged, this, &CTerrainLighting::OnSunDirectionSlider);
    connect(ui->LIGHTING_SUNDIR_EDIT, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &CTerrainLighting::UpdateScrollBarsFromEdits);

    connect(ui->SUN_MAP_LONGITUDE, &AzQtComponents::SliderInt::valueChanged, this, &CTerrainLighting::OnSunMapLongitudeSlider);
    connect(ui->LIGHTING_POLE_EDIT, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &CTerrainLighting::UpdateScrollBarsFromEdits);

    connect(ui->DAWN_SLIDER, &AzQtComponents::SliderInt::valueChanged, this, &CTerrainLighting::OnDawnTimeSlider);
    connect(ui->DAWN_INFO, &QLineEdit::editingFinished, this, &CTerrainLighting::UpdateScrollBarsFromEdits);

    connect(ui->DAWN_DUR_SLIDER, &AzQtComponents::SliderInt::valueChanged, this, &CTerrainLighting::OnDawnDurationSlider);
    connect(ui->DAWN_DUR_INFO, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &CTerrainLighting::UpdateScrollBarsFromEdits);

    connect(ui->DUSK_SLIDER, &AzQtComponents::SliderInt::valueChanged, this, &CTerrainLighting::OnDuskTimeSlider);
    connect(ui->DUSK_INFO, &QLineEdit::editingFinished, this, &CTerrainLighting::UpdateScrollBarsFromEdits);

    connect(ui->DUSK_DUR_SLIDER, &AzQtComponents::SliderInt::valueChanged, this, &CTerrainLighting::OnDuskDurationSlider);
    connect(ui->DUSK_DUR_INFO, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &CTerrainLighting::UpdateScrollBarsFromEdits);

    connect(ui->SKY_QUALITY, &AzQtComponents::SliderInt::valueChanged, this, &CTerrainLighting::OnSkyQualitySlider);
    connect(ui->TERRAIN_OBJECT_OCCLUSION, &QCheckBox::stateChanged, this, &CTerrainLighting::OnApplyILSS);

    connect(ui->FORCE_SKY_UPDATE, &QCheckBox::stateChanged, this, &CTerrainLighting::OnForceSkyUpdate);

    connect(ui->LIGHTING_IMPORT_BTN, &QPushButton::clicked, this, &CTerrainLighting::OnFileImport);
    connect(ui->LIGHTING_EXPORT_BTN, &QPushButton::clicked, this, &CTerrainLighting::OnFileExport);
}


CTerrainLighting::~CTerrainLighting()
{
    GetIEditor()->UnregisterNotifyListener(this);
#ifdef LY_TERRAIN_EDITOR
    delete m_pTexGen;
#endif // #ifdef LY_TERRAIN_EDITOR
}
const GUID& CTerrainLighting::GetClassID()
{
    static const GUID guid = {
        0x8e845df0, 0x7000, 0x4aae, { 0xae, 0xc8, 0x57, 0x88, 0xde, 0x4c, 0x6, 0xa3 }
    };
    return guid;
}

void CTerrainLighting::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions options;
    options.paneRect = QRect(5, 110, 750, 360);
    options.canHaveMultipleInstances = true;
    options.isDeletable = false;
    options.sendViewPaneNameBackToAmazonAnalyticsServers = true;

    AzToolsFramework::RegisterViewPane<CTerrainLighting>(LIGHTING_TOOL_WINDOW_NAME, LyViewPane::CategoryOther, options);
}

void CTerrainLighting::InitTerrainTexGen()
{
#ifdef LY_TERRAIN_EDITOR
    if (m_pTexGen)
    {
        delete m_pTexGen;
    }

    m_pTexGen = new CTerrainTexGen;
    m_pTexGen->Init(LIGHTING_PREVIEW_RESOLUTION, true);
#endif // #ifdef LY_TERRAIN_EDITOR
}

void CTerrainLighting::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    // Important! Currently, the code in m_pTexGen->GenerateSurfaceTexture, called by GenerateLightmap below, called by Refresh()
    // uses a CWaitProgress object internally, to update the progress bar that it puts along the bottom of the Editor's MainWindow.
    // CWaitProgress pumps the windows message queue, which pumps the Qt message queue. We're in a showEvent,
    // but the lightmap generation forces the window to be polished/shown before this event completes. That normally wouldn't be a problem,
    // except that because the message queues are pumped during that process, the widget is shown during the lightmap generation
    // in a terrible state (attached to the main window but obscured by the view port and not interactive).
    // The proper fix is to use QTimers or something else when generating the lightmap, so that we don't need to directly
    // pump the message queue. For now, this resolves the problem.
    QTimer::singleShot(0, [this]()
    {
        Refresh();

        // make sure we repaint too
        repaint();
    });
}

void CTerrainLighting::GenerateLightmap(bool drawOntoDialog)
{
#ifdef LY_TERRAIN_EDITOR
    // Don't try to generate lightmaps if we're refreshing the dialog in-between loading levels.
    if (m_pTexGen == nullptr)
    {
        return;
    }

    // Calculate the lighting.
    m_pTexGen->InvalidateLighting();

    int flags = ETTG_LIGHTING | ETTG_QUIET | ETTG_INVALIDATE_LAYERS | ETTG_BAKELIGHTING;

    // no texture needed
    flags |= ETTG_NOTEXTURE;
    flags |= ETTG_SHOW_WATER;

    m_pTexGen->GenerateSurfaceTexture(flags, m_lightmap);
#endif // #ifdef LY_TERRAIN_EDITOR

    //put m_lightmap into m_bmpLightmap
    m_bmpLightmap = QImage(reinterpret_cast<uchar*>(m_lightmap.GetData()), m_lightmap.GetWidth(), m_lightmap.GetHeight(), QImage::Format_RGBX8888);
    m_sunPathPreview = QPixmap::fromImage(m_bmpLightmap);

    //  Draw Sun direction.
    QPainter painter(&m_sunPathPreview);
    painter.setPen(Qt::red);
    QPoint center = m_sunPathPreview.rect().center();

    // fSunRot is negative to match how it's used in TerrainLightGen.cpp.  We could change both to be positive,
    // but that would break any existing sun direction data that's been saved. 
    float fSunRot = -(gf_PI2 * (ui->SUN_DIRECTION->value())) / 360.0f;
    float fLongitude = gf_halfPI - gf_PI * ui->SUN_MAP_LONGITUDE->value() / 180.0f;

    uint32 dwI = 0;
    const float numSunDirectionArrows = 64.0f;

    for (double fAngle = 0; fAngle <= gf_PI2; fAngle += gf_PI2 / numSunDirectionArrows, ++dwI)
    {
        const float fR = 120.0f;

        Matrix33 a, b, c, m;

        a.SetRotationZ(fAngle);
        b.SetRotationX(fLongitude);
        c.SetRotationY(fSunRot);

        m = a * b * c;

        Vec3 vLocalSunDir = Vec3(0, fR, 0) * m;
        Vec3 vLocalSunDirLeft = Vec3(-fR * 0.05f, fR, fR * 0.05f) * m;
        Vec3 vLocalSunDirRight = Vec3(-fR * 0.05f, fR, -fR * 0.05f) * m;

        QPoint point = center + QPoint((int)vLocalSunDir.x, (int)vLocalSunDir.z);
        QPoint pointLeft = center + QPoint((int)vLocalSunDirLeft.x, (int)vLocalSunDirLeft.z);
        QPoint pointRight = center + QPoint((int)vLocalSunDirRight.x, (int)vLocalSunDirRight.z);

        if (vLocalSunDir.y > -0.1f)     // beginning or below surface
        {
            painter.drawLine(pointLeft, point);
            painter.drawLine(point, pointRight);
        }
    }

    // Update the preview
    if (drawOntoDialog)
    {
        ui->SUN_PATH_PREVIEW_LABEL->setPixmap(m_sunPathPreview);
    }

    GetIEditor()->GetGameEngine()->ReloadEnvironment();
    GetIEditor()->UpdateViews(eRedrawViewports);
}

void CTerrainLighting::GenerateMoonSunTransition(bool drawOntoDialog)
{
    int dawnTime = ui->DAWN_SLIDER->value();
    int duskTime = ui->DUSK_SLIDER->value();
    int dawnDuration = ui->DAWN_DUR_SLIDER->value();
    int duskDuration = ui->DUSK_DUR_SLIDER->value();

    MOON_SUN_TRANS_WIDTH = ui->MOON_SUN_TRANSITION_PREVIEW_LABEL->width();
    MOON_SUN_TRANS_HEIGHT = ui->MOON_SUN_TRANSITION_PREVIEW_LABEL->height();

    m_moonSunTransitionPreview = QPixmap(ui->MOON_SUN_TRANSITION_PREVIEW_LABEL->width(), ui->MOON_SUN_TRANSITION_PREVIEW_LABEL->height());
    QPainter painter(&m_moonSunTransitionPreview);

    // render sun/moon transition as overlay into DIB
    for (int x = 0; x < MOON_SUN_TRANS_WIDTH; ++x)
    {
        // render gradient
        {
            QColor sun;
            sun.setRgbF(1, 0.9f, 0.4f, 1);

            QColor moon;
            moon.setRgbF(0.4f, 0.4f, 1, 1);

            QColor black;
            black.setRgbF(0, 0, 0, 1);

            QColor col;

            float f(2.0f * x / (float)MOON_SUN_TRANS_WIDTH);
            if (f >= 1.0f)
            {
                f -= 1.0f;

                float t((float)duskTime / (float)(12 * 60));
                float d((float)duskDuration / (float)(12 * 60));

                float m(t + d * 0.5f);
                float s(t - d * 0.5f);

                if (f < s)
                {
                    col = sun;
                }
                else if (f >= m)
                {
                    col = moon;
                }
                else
                {
                    assert(s < m);
                    float b(0.5f * (s + m));
                    if (f < b)
                    {
                        lerpFloat(col, sun, black, (f - s) / (b - s));
                    }
                    else
                    {
                        lerpFloat(col, black, moon, (f - b) / (m - b));
                    }
                }
            }
            else
            {
                float t((float)dawnTime / (float)(12 * 60));
                float d((float)dawnDuration / (float)(12 * 60));

                float s(t + d * 0.5f);
                float m(t - d * 0.5f);

                if (f < m)
                {
                    col = moon;
                }
                else if (f >= s)
                {
                    col = sun;
                }
                else
                {
                    assert(s > m);
                    float b(0.5f * (s + m));
                    if (f < b)
                    {
                        lerpFloat(col, moon, black, (f - m) / (b - m));
                    }
                    else
                    {
                        lerpFloat(col, black, sun, (f - b) / (s - b));
                    }
                }
            }

            QColor _col(col);
            painter.setPen(_col);
            painter.drawLine(x, 0, x, MOON_SUN_TRANS_HEIGHT);
        }
    }

    // drawing red marks to enlight label steps
    painter.setPen(Qt::red);
    for (int i = 0; i < 6; ++i)
    {
        int x = MOON_SUN_TRANS_WIDTH / 4 * i;
        painter.drawLine(x, 0, x, MOON_SUN_TRANS_HEIGHT + 3);
    }

    // finally setting the pixmap into the label
    ui->MOON_SUN_TRANSITION_PREVIEW_LABEL->setPixmap(m_moonSunTransitionPreview);

    GetIEditor()->GetGameEngine()->ReloadEnvironment();
    GetIEditor()->UpdateViews(eRedrawViewports);
}

void CTerrainLighting::OnHSlidersScroll()
{
    // Prevent lighting setting values from being reset on accident while
    // the level is loading
    if (!GetIEditor()->GetDocument()->IsDocumentReady())
    {
        return;
    }

    ////////////////////////////////////////////////////////////////////////
    // Update the document with the values from the sliders
    ////////////////////////////////////////////////////////////////////////
    LightingSettings* ls = GetIEditor()->GetDocument()->GetLighting();

    ls->iSunRotation = ui->SUN_DIRECTION->value();
    ls->iILApplySS = ui->TERRAIN_OBJECT_OCCLUSION->isChecked();
    ls->iHemiSamplQuality = ui->SKY_QUALITY->value();
    ls->iLongitude = ui->SUN_MAP_LONGITUDE->value();
    //  ls->iSkyBrightening = m_sldSkyBrightening; // was commented on original code

    ls->iDawnTime = ui->DAWN_SLIDER->value();
    ls->iDawnDuration = ui->DAWN_DUR_SLIDER->value();
    ls->iDuskTime = ui->DUSK_SLIDER->value();
    ls->iDuskDuration = ui->DUSK_DUR_SLIDER->value();

    float fHour = ui->LIGHTING_TIME_OF_DAY->value() / SLIDER_SCALE;
    QString timeString = convertTimeToTimeOfDayString(ui->LIGHTING_TIME_OF_DAY->value() / SLIDER_SCALE);
    ui->TIME_OF_DAY_EDIT->setText(timeString);
    SetTime(fHour, ui->FORCE_SKY_UPDATE->isChecked());

    // We modified the document
    GetIEditor()->SetModifiedFlag();
    GetIEditor()->SetModifiedModule(eModifiedTerrain);

    UpdateMoonSunTransitionLabels();
}

void CTerrainLighting::OnApplyILSS()
{
    ////////////////////////////////////////////////////////////////////////
    // Synchronize value in the document
    ////////////////////////////////////////////////////////////////////////

    GetIEditor()->GetDocument()->GetLighting()->iILApplySS = ui->TERRAIN_OBJECT_OCCLUSION->isChecked();

    // We modified the document
    GetIEditor()->SetModifiedFlag();
    GetIEditor()->SetModifiedModule(eModifiedTerrain);
}

void CTerrainLighting::OnFileImport()
{
    ////////////////////////////////////////////////////////////////////////
    // Import the lighting settings
    ////////////////////////////////////////////////////////////////////////
    QString fileName = QFileDialog::getOpenFileName(this,
            tr("Open Light Settings"), GetIEditor()->GetLevelFolder(), tr("Light Settings Files (*.lgt)"));

    if (fileName.isEmpty())
    {
        return;
    }

    CLogFile::WriteLine("Importing light settings...");

    XmlNodeRef node = XmlHelpers::LoadXmlFromFile(fileName.toStdString().c_str());
    GetIEditor()->GetDocument()->GetLighting()->Serialize(node, true);

    // We modified the document
    GetIEditor()->SetModifiedFlag();
    GetIEditor()->SetModifiedModule(eModifiedTerrain);

    // Update the controls with the settings from the document
    UpdateControls();

    // Update the preview
    Refresh();
}

void CTerrainLighting::OnFileExport()
{
    //////////////////////////////////////////////////////////////////////////
    //// Export the lighting settings
    //////////////////////////////////////////////////////////////////////////
    QString fileName = QFileDialog::getSaveFileName(this,
            tr("Export light settigns"),
            GetIEditor()->GetLevelFolder(),
            tr("Light Settings Files (*.lgt)"));

    if (fileName.isEmpty())
    {
        return;
    }

    CLogFile::WriteLine("Exporting light settings...");

    // Write the light settings into the archive
    XmlNodeRef node = XmlHelpers::CreateXmlNode("LightSettings");
    GetIEditor()->GetDocument()->GetLighting()->Serialize(node, false);
    XmlHelpers::SaveXmlNode(GetIEditor()->GetFileUtil(), node, fileName.toStdString().c_str());
}

void CTerrainLighting::UpdateControls()
{
    ////////////////////////////////////////////////////////////////////////
    // Update the controls with the settings from the document
    ////////////////////////////////////////////////////////////////////////

    LightingSettings* ls = GetIEditor()->GetDocument()->GetLighting();

    ui->SUN_DIRECTION->setValue(ls->iSunRotation);
    //  m_bTerrainShadows = ls->bTerrainShadows;
    ui->TERRAIN_OBJECT_OCCLUSION->setChecked(ls->iILApplySS);
    //  m_sldSkyBrightening = ls->iSkyBrightening;
    ui->SUN_MAP_LONGITUDE->setValue(ls->iLongitude);
    ui->SKY_QUALITY->setValue(ls->iHemiSamplQuality);

    // Time Of Day slider
    float timeFloat = GetTime();
    int sliderTime = (int)clamp_tpl(timeFloat * SLIDER_SCALE, 0.0f, 24 * SLIDER_SCALE);
    const QString lineEditTime = convertTimeToTimeOfDayString(timeFloat);

    ui->LIGHTING_TIME_OF_DAY->setValue(sliderTime);
    ui->TIME_OF_DAY_EDIT->setText(lineEditTime);

    ui->DAWN_SLIDER->setValue(clamp_tpl(ls->iDawnTime, 0, 12 * 60));
    ui->DAWN_DUR_SLIDER->setValue(clamp_tpl(ls->iDawnDuration, 0, 60));

    ui->DUSK_SLIDER->setValue(clamp_tpl(ls->iDuskTime, 0, 12 * 60));
    ui->DUSK_DUR_SLIDER->setValue(clamp_tpl(ls->iDuskDuration, 0, 60));

    switch (ls->eAlgo)
    {
    case ePrecise:
        ui->LIGHTING_TYPE_PRECISE->setChecked(true);
        break;

    case eDynamicSun:
        ui->LIGHTING_TYPE_DYNAMIC_SUN->setChecked(true);
        break;

    default:
        Q_ASSERT(false);
    }

    UpdateMoonSunTransitionLabels();
}

void CTerrainLighting::OnDynamicSun()
{
    GetIEditor()->GetDocument()->GetLighting()->eAlgo = eDynamicSun;
    GetIEditor()->SetModifiedFlag();
    GetIEditor()->SetModifiedModule(eModifiedTerrain);
    UpdateControls();
}

void CTerrainLighting::OnPrecise()
{
    GetIEditor()->GetDocument()->GetLighting()->eAlgo = ePrecise;
    GetIEditor()->SetModifiedFlag();
    GetIEditor()->SetModifiedModule(eModifiedTerrain);
    UpdateControls();
}


void CTerrainLighting::OnLightingTimeOfDaySlider()
{
    OnHSlidersScroll();
    if (ui->FORCE_SKY_UPDATE->isChecked())
    {
        GenerateMoonSunTransition();
    }
}

void CTerrainLighting::OnDawnDurationSlider()
{
    OnHSlidersScroll();
    GenerateMoonSunTransition();
}

void CTerrainLighting::OnDuskTimeSlider()
{
    OnHSlidersScroll();
    GenerateMoonSunTransition();
}

void CTerrainLighting::OnDawnTimeSlider()
{
    OnHSlidersScroll();
    GenerateMoonSunTransition();
}

void CTerrainLighting::OnDuskDurationSlider()
{
    OnHSlidersScroll();
    GenerateMoonSunTransition();
}

void  CTerrainLighting::OnSunDirectionSlider()
{
    OnHSlidersScroll();
    GenerateLightmap();
}

void CTerrainLighting::OnSunMapLongitudeSlider()
{
    OnHSlidersScroll();
    GenerateLightmap();
}

void CTerrainLighting::OnSkyQualitySlider()
{
    OnHSlidersScroll();
    GenerateLightmap();
}

void CTerrainLighting::UpdateMoonSunTransitionLabels()
{
    QString out;
    if (convertMinutesToTransitionTimeString(ui->DAWN_SLIDER->value(), false, out))
    {
        ui->DAWN_INFO->setText(out);
    }
    if (convertMinutesToTransitionTimeString(ui->DUSK_SLIDER->value(), true, out))
    {
        ui->DUSK_INFO->setText(out);
    }

    // Extract our values here because the setValue functions below trigger the callback CTerrainLighting::UpdateScrollBarsFromEdits.
    // This causes the sliders to update from their corresponding control, so at initialization time it improperly zeros out 3 of these controls.
    const int duskDurSlider = ui->DUSK_DUR_SLIDER->value();
    const int dawnDurSlider = ui->DAWN_DUR_SLIDER->value();
    const int sunMapLongitude = ui->SUN_MAP_LONGITUDE->value();
    const int sunDirection = ui->SUN_DIRECTION->value();

    ui->DUSK_DUR_INFO->setValue(duskDurSlider);
    ui->DAWN_DUR_INFO->setValue(dawnDurSlider);

    ui->LIGHTING_POLE_EDIT->setValue(sunMapLongitude);
    ui->LIGHTING_SUNDIR_EDIT->setValue(sunDirection);
}

float CTerrainLighting::GetTime() const
{
    float fTime = 0;
    if (GetIEditor()->GetDocument()->GetCurrentMission())
    {
        fTime = GetIEditor()->GetDocument()->GetCurrentMission()->GetTime();
    }
    return fTime;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainLighting::SetTime(const float fHour, const bool bforceSkyUpate)
{
    ITimeOfDay* pTimeOfDay = gEnv->p3DEngine->GetTimeOfDay();
    pTimeOfDay->SetTime(fHour, bforceSkyUpate);

    if (GetIEditor()->GetDocument()->GetCurrentMission())
    {
        GetIEditor()->GetDocument()->GetCurrentMission()->SetTime(fHour);
    }

    GetIEditor()->Notify(eNotify_OnTimeOfDayChange);
}

void CTerrainLighting::UpdateScrollBarsFromEdits()
{
    // Prevent lighting setting values from being reset on accident while
    // the level is loading
    if (!GetIEditor()->GetDocument()->IsDocumentReady())
    {
        return;
    }

    float fTime = 0.0f;
    if (!convertTimeOfDayStringToTime(ui->TIME_OF_DAY_EDIT->text(), fTime))
    {
        return;
    }

    SetTime(fTime, ui->FORCE_SKY_UPDATE->isChecked());

    // dawn and dusk slider from 0 to 12*60 minutes, display on edit as hh:mm (pm style for dusk, am for dawn)
    int dawnTimeMinutes = 0.0f;
    if (convertTransitionTimeStringToMinutes(ui->DAWN_INFO->text(), dawnTimeMinutes))
    {
        ui->DAWN_SLIDER->setValue(dawnTimeMinutes);
    }

    int duskTimeMinutes = 0.0f;
    if (convertTransitionTimeStringToMinutes(ui->DUSK_INFO->text(), duskTimeMinutes))
    {
        ui->DUSK_SLIDER->setValue(duskTimeMinutes);
    }

    ui->DUSK_DUR_SLIDER->setValue(ui->DUSK_DUR_INFO->value());
    ui->DAWN_DUR_SLIDER->setValue(ui->DAWN_DUR_INFO->value());

    ui->SUN_DIRECTION->setValue(ui->LIGHTING_SUNDIR_EDIT->value());
    ui->SUN_MAP_LONGITUDE->setValue(ui->LIGHTING_POLE_EDIT->value());

    LightingSettings* ls = GetIEditor()->GetDocument()->GetLighting();
    ls->iDawnTime = ui->DAWN_SLIDER->value();
    ls->iDuskTime = ui->DUSK_SLIDER->value();
    ls->iDawnDuration = ui->DAWN_DUR_SLIDER->value();
    ls->iDuskDuration = ui->DUSK_DUR_SLIDER->value();
    ls->iLongitude = ui->SUN_MAP_LONGITUDE->value();
    ls->iSunRotation = ui->SUN_DIRECTION->value();

    UpdateControls();
    UpdateDocument();
}

void CTerrainLighting::UpdateDocument()
{
    // Update the preview
    if (ui->FORCE_SKY_UPDATE->isChecked())
    {
        GenerateLightmap();
    }
    else
    {
        GenerateMoonSunTransition();
    }

    UpdateMoonSunTransitionLabels();

    // We modified the document
    GetIEditor()->SetModifiedFlag();
    GetIEditor()->SetModifiedModule(eModifiedTerrain);
}

void CTerrainLighting::OnForceSkyUpdate()
{
    bool checked = ui->FORCE_SKY_UPDATE->isChecked();
    ITimeOfDay* pTimeOfDay = gEnv->p3DEngine->GetTimeOfDay();
    pTimeOfDay->SetTime(GetTime(), checked);
    gSettings.bForceSkyUpdate = checked;
}


void CTerrainLighting::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnEndNewScene:
    case eNotify_OnEndSceneOpen:
    {
        InitTerrainTexGen();
        UpdateControls();

        // Update the preview
        Refresh();
    }

    break;
    case eNotify_OnCloseScene:
    case eNotify_OnBeginSWNewScene:
    {
#ifdef LY_TERRAIN_EDITOR
        if (m_pTexGen)
        {
            delete m_pTexGen;
            m_pTexGen = nullptr;
        }
#endif // #ifdef LY_TERRAIN_EDITOR

        m_sunPathPreview = QPixmap(LIGHTING_PREVIEW_RESOLUTION, LIGHTING_PREVIEW_RESOLUTION);
        m_sunPathPreview.fill(QColor(Qt::blue));
        ui->SUN_PATH_PREVIEW_LABEL->setPixmap(m_sunPathPreview);
    }
    break;
    }
    ;
}

void CTerrainLighting::Refresh()
{
    GenerateLightmap();
    GenerateMoonSunTransition();
}

#include <TerrainLighting.moc>
