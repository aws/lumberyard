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
#include "TerrainDialog.h"

#include "Terrain/GenerationParam.h"
#include "Terrain/Noise.h"

#include "TerrainLighting.h"

#include "TerrainModifyTool.h"
#include "TerrainModifyPanel.h"
#include "TerrainTextureExport.h"
#include "TopRendererWnd.h"
#include "VegetationMap.h"

#include "Terrain/Heightmap.h"
#include "Terrain/TerrainManager.h"

#include "EditMode/ObjectMode.h"

#include "GameEngine.h"

#include "CryEditDoc.h"

#include <QDesktopServices>
#include <QMessageBox>
#include <QInputDialog>
#include "QtUtilWin.h"
#include "QtViewPaneManager.h"
#include "Util/AutoDirectoryRestoreFileDialog.h"
#include "NewTerrainDialog.h"

#include <AzCore/Casting/numeric_cast.h>

#include <ui_TerrainDialog.h>

#include <Cry3DEngine/Environment/OceanEnvironmentBus.h>
#include <AzToolsFramework/Physics/EditorTerrainComponentBus.h>

#include "QtUtil.h"

static const int TOOLBAR_ICON_SIZE = 32;

namespace
{
    class HeightmapPreviewGenerator
        : public NoiseGenerator::PreviewDelegate
    {
    public:
        HeightmapPreviewGenerator(const CHeightmap& original)
            : m_heightmap(original)
        {
        }

        void SetNoise(const SNoiseParams& noise) override
        {
            SNoiseParams localParams = noise;
            localParams.iWidth = m_heightmap.GetWidth();
            localParams.iHeight = m_heightmap.GetHeight();
            m_heightmap.GenerateTerrain(localParams);
        }

        QImage GetImage() override
        {
            auto image = m_heightmap.GetHeightmapImageEx();
            return QImage(reinterpret_cast<uchar*>(image->GetData()), image->GetWidth(), image->GetHeight(), QImage::Format_RGB32).rgbSwapped();
        }

    private:

        CHeightmap m_heightmap;
    };
}

/////////////////////////////////////////////////////////////////////////////
// CTerrainDialog dialog
CTerrainDialog::CTerrainDialog(QWidget* parent)
    : QMainWindow(parent)
    , m_ui(new Ui::TerrainDialog)
{
    m_ui->setupUi(this);

    // We don't have valid recent terrain generation parameters yet
    m_sLastParam = new SNoiseParams;
    m_sLastParam->bValid = false;

    m_pHeightmap = GetIEditor()->GetHeightmap();

    GetIEditor()->RegisterNotifyListener(this);
    LmbrCentral::WaterNotificationBus::Handler::BusConnect();

    OnInitDialog();
}

CTerrainDialog::~CTerrainDialog()
{
    LmbrCentral::WaterNotificationBus::Handler::BusDisconnect();
    GetIEditor()->UnregisterNotifyListener(this);
    GetIEditor()->SetEditTool(nullptr);
    delete m_sLastParam;
}

const GUID& CTerrainDialog::GetClassID()
{
    // {CB15B296-6829-459d-BA26-A5C0BFE0B8F5}
    static const GUID guid = {
        0xcb15b296, 0x6829, 0x459d, { 0xba, 0x26, 0xa5, 0xc0, 0xbf, 0xe0, 0xb8, 0xf5 }
    };
    return guid;
}

void CTerrainDialog::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions options;
    options.paneRect = QRect(100, 100, 1000, 800);
    options.canHaveMultipleInstances = true;
    options.sendViewPaneNameBackToAmazonAnalyticsServers = true;

    if (!GetIEditor()->IsNewViewportInteractionModelEnabled())
    {
        AzToolsFramework::RegisterViewPane<CTerrainDialog>(
            LyViewPane::TerrainEditor, LyViewPane::CategoryTools, options);
    }
}


/////////////////////////////////////////////////////////////////////////////
// CTerrainDialog message handlers

void CTerrainDialog::OnInitDialog()
{
    m_ui->actionResize_Terrain->setEnabled(GetIEditor()->GetDocument()->IsDocumentReady());

    connect(m_ui->actionImport_Heightmap, &QAction::triggered, this, &CTerrainDialog::OnTerrainLoad);
    connect(m_ui->actionExport_Heightmap, &QAction::triggered, this, &CTerrainDialog::OnExportHeightmap);
    connect(m_ui->actionShow_Large_Preview, &QAction::triggered, this, &CTerrainDialog::OnHeightmapShowLargePreview);
    connect(m_ui->actionMake_Isle, &QAction::triggered, this, &CTerrainDialog::OnModifyMakeisle);

    // is the feature toggle enabled for the ocean component feature enabled when the project includes the Water gem
    bool bHasOceanFeature = false;
    AZ::OceanFeatureToggleBus::BroadcastResult(bHasOceanFeature, &AZ::OceanFeatureToggleBus::Events::OceanComponentEnabled);
    if (bHasOceanFeature)
    {
        m_ui->actionRemove_Ocean->setVisible(false);
        m_ui->actionSet_Ocean_Height->setVisible(false);
    }
    else
    {
        connect(m_ui->actionRemove_Ocean, &QAction::triggered, this, &CTerrainDialog::OnModifyRemoveOcean);
        connect(m_ui->actionSet_Ocean_Height, &QAction::triggered, this, &CTerrainDialog::OnSetOceanLevel);
    }

    connect(m_ui->actionSet_Terrain_Max_Height, &QAction::triggered, this, &CTerrainDialog::OnSetMaxHeight);
    connect(m_ui->actionSet_Unit_Size, &QAction::triggered, this, &CTerrainDialog::OnSetUnitSize);
    connect(m_ui->actionFlatten, &QAction::triggered, this, &CTerrainDialog::OnModifyFlatten);
    connect(m_ui->actionSmooth, &QAction::triggered, this, &CTerrainDialog::OnModifySmooth);
    connect(m_ui->actionSmooth_Slope, &QAction::triggered, this, &CTerrainDialog::OnModifySmoothSlope);
    connect(m_ui->actionNormalize, &QAction::triggered, this, &CTerrainDialog::OnModifyNormalize);
    connect(m_ui->actionReduce_Range_Light, &QAction::triggered, this, &CTerrainDialog::OnModifyReduceRangeLight);
    connect(m_ui->actionReduce_Range_Heavy, &QAction::triggered, this, &CTerrainDialog::OnModifyReduceRange);
    connect(m_ui->actionErase_Terrain, &QAction::triggered, this, &CTerrainDialog::OnTerrainErase);
    connect(m_ui->actionResize_Terrain, &QAction::triggered, this, &CTerrainDialog::OnTerrainResize);
    connect(m_ui->actionInvert_Heightmap, &QAction::triggered, this, &CTerrainDialog::OnTerrainInvert);
    connect(m_ui->actionGenerate_Terrain, &QAction::triggered, this, &CTerrainDialog::OnTerrainGenerate);
    connect(m_ui->actionSun_Trajectory_Tool, &QAction::triggered, this, &CTerrainDialog::OnTerrainLight);
    connect(m_ui->actionExport_Import_Megaterrain_Texture, &QAction::triggered, this, &CTerrainDialog::OnTerrainTextureImportExport);
    connect(m_ui->actionShow_Terrain_Texture_Layers, &QAction::triggered, this, &CTerrainDialog::OnTerrainSurface);
    connect(m_ui->actionShow_Water, &QAction::triggered, this, &CTerrainDialog::OnOptionsShowWater);
    connect(m_ui->actionShow_Map_Objects, &QAction::triggered, this, &CTerrainDialog::OnOptionsShowMapObjects);
    connect(m_ui->actionShow_Grid, &QAction::triggered, this, &CTerrainDialog::OnOptionsShowGrid);
    connect(m_ui->actionAuto_Scale_Grey_Range, &QAction::triggered, this, &CTerrainDialog::OnOptionsAutoScaleGreyRange);
    connect(m_ui->actionHold, &QAction::triggered, this, &CTerrainDialog::OnHold);
    connect(m_ui->actionFetch, &QAction::triggered, this, &CTerrainDialog::OnFetch);
    connect(m_ui->actionBrush_One, &QAction::triggered, this, &CTerrainDialog::OnBrush1);
    connect(m_ui->actionBrush_Two, &QAction::triggered, this, &CTerrainDialog::OnBrush2);
    connect(m_ui->actionBrush_Three, &QAction::triggered, this, &CTerrainDialog::OnBrush3);
    connect(m_ui->actionBrush_Four, &QAction::triggered, this, &CTerrainDialog::OnBrush4);
    connect(m_ui->actionBrush_Five, &QAction::triggered, this, &CTerrainDialog::OnBrush5);
    connect(m_ui->actionFine_Opacity, &QAction::triggered, this, &CTerrainDialog::OnLowOpacity);
    connect(m_ui->actionMedium_Opacity, &QAction::triggered, this, &CTerrainDialog::OnMediumOpacity);
    connect(m_ui->actionCoarse_Opacity, &QAction::triggered, this, &CTerrainDialog::OnHighOpacity);

    CreateModifyTool();

    m_ui->viewport->m_bShowHeightmap = true;
    m_ui->viewport->SetShowWater(true);
    m_ui->viewport->SetShowViewMarker(false);
    m_ui->viewport->SetAutoScaleGreyRange(true);

    m_ui->actionShow_Map_Objects->setChecked(m_ui->viewport->m_bShowStatObjects);
    m_ui->actionShow_Water->setChecked(m_ui->viewport->GetShowWater());
    m_ui->actionShow_Grid->setChecked(m_ui->viewport->GetShowGrid());
    m_ui->actionAuto_Scale_Grey_Range->setChecked(m_ui->viewport->GetAutoScaleGreyRange());

    //////////////////////////////////////////////////////////////////////////
    m_terrainDimensions = new QLabel(this);
    m_ui->statusBar->addPermanentWidget(m_terrainDimensions);

    UpdateTerrainDimensions();
}

void CTerrainDialog::UpdateTerrainDimensions()
{
    m_terrainDimensions->setText(QString("Heightmap %1x%2").arg(m_pHeightmap->GetWidth()).arg(m_pHeightmap->GetHeight()));
}

//////////////////////////////////////////////////////////////////////////
void CTerrainDialog::CreateModifyTool()
{
    m_pTerrainTool = new CTerrainModifyTool;
    m_ui->terrainModifyPanel->SetModifyTool(m_pTerrainTool);
    m_pTerrainTool->SetExternalUIPanel(m_ui->terrainModifyPanel);

    m_ui->viewport->SetEditTool(m_pTerrainTool, true);
    GetIEditor()->SetEditTool(m_pTerrainTool);
}

void CTerrainDialog::OnTerrainLoad()
{
    ////////////////////////////////////////////////////////////////////////
    // Load a heightmap from a file
    ////////////////////////////////////////////////////////////////////////

    char szFilters[] = "All Image Files (*.bt *.asc *.tif *.pgm *.raw *.r16 *.bmp *.png);;32-bit BT files (*.bt);;32-bit ARCGrid ASCII files (*.asc);;32-bit TIFF Files (*.tif);;16-bit PGM Files (*.pgm);;16-bit RAW Files (*.raw);;16-bit RAW Files (*.r16);;8-bit Bitmap Files (*.bmp);;8-bit PNG Files (*.png);;All files (*)";
    CAutoDirectoryRestoreFileDialog dlg(QFileDialog::AcceptOpen, QFileDialog::ExistingFile, {}, Path::GetEditingGameDataFolder().c_str(), szFilters, {}, {}, this);

    if (dlg.exec())
    {
        QString fileName = dlg.selectedFiles().constFirst();
        QWaitCursor wait;
        m_pHeightmap->ImportHeightmap(fileName);

        InvalidateTerrain();

        m_ui->viewport->InitHeightmapAlignment();
        InvalidateViewport();
    }
}

void CTerrainDialog::OnTerrainErase()
{
    ////////////////////////////////////////////////////////////////////////
    // Erase the heightmap
    ////////////////////////////////////////////////////////////////////////

    auto result = QMessageBox::question(this, tr("Erase Heightmap?"), tr("Really erase the heightmap?"));
    if (QMessageBox::Yes == result)
    {
        m_pHeightmap->Clear(false);

        InvalidateTerrain();
    }
}

void CTerrainDialog::closeEvent(QCloseEvent* ev)
{
    if (m_processing)
    {
        QMessageBox::information(this, "Terrain - Processing", "The terrain editor is still processing the last operation.  Please wait until complete before closing.", QMessageBox::StandardButton::Ok);
        ev->ignore();
    }
    else
    {
        ev->accept();
    }
}

void CTerrainDialog::OnTerrainResize()
{
    // Resizing can be a lengthy operation, so make sure we prevent the Terrain Dialog from closing
    // while it's occurring.  (It can get closed because the terrain resize includes an export that
    // updates a progress bar, which processes UI events.  These events can include closing this
    // dialog)
    m_processing = true;

    CCryEditApp::instance()->OnTerrainResizeterrain();
    UpdateTerrainDimensions();

    m_processing = false;
}

void CTerrainDialog::OnTerrainInvert()
{
    ////////////////////////////////////////////////////////////////////////
    // Invert the heightmap
    ////////////////////////////////////////////////////////////////////////

    if (!GetIEditor()->FlushUndo(true))
    {
        return;
    }

    QWaitCursor wait;

    m_pHeightmap->Invert();

    InvalidateTerrain();
}

void CTerrainDialog::OnTerrainGenerate()
{
    if (!m_pHeightmap || !m_pHeightmap->GetWidth() || !m_pHeightmap->GetHeight())
    {
        // Can't generate a terrain without a valid heightmap
        return;
    }

    SNoiseParams sParam;
    CGenerationParam cDialog;

    if (GetLastParam()->bValid)
    {
        // Use last parameters
        cDialog.LoadParam(*GetLastParam());
    }
    else
    {
        // Set default parameters for the dialog
        SNoiseParams sDefaultParam;
        sDefaultParam.iPasses = 8;  // Detail (Passes)
        sDefaultParam.fFrequency = 7.0f;  // Feature Size
        sDefaultParam.fFrequencyStep = 2.0f;
        sDefaultParam.fFade = 0.46f;  // Bumpiness
        sDefaultParam.iRandom = 1;  // Variation
        sDefaultParam.iSharpness = 0.999f;
        sDefaultParam.iSmoothness = 0;

        cDialog.LoadParam(sDefaultParam);
    }

    HeightmapPreviewGenerator previewDelegate(*m_pHeightmap);
    cDialog.SetPreviewDelegate(&previewDelegate);

    // Show the generation parameter dialog
    if (cDialog.exec() == QDialog::Rejected)
    {
        return;
    }

    ////////////////////////////////////////////////////////////////////////
    // Generate a terrain
    ////////////////////////////////////////////////////////////////////////
    GetIEditor()->FlushUndo();

    CLogFile::WriteLine("Generating new terrain...");

    // Fill the parameter structure for the terrain generation
    cDialog.FillParam(sParam);
    sParam.iWidth = m_pHeightmap->GetWidth();
    sParam.iHeight = m_pHeightmap->GetHeight();
    sParam.bBlueSky = false;

    // Save the parameters
    ZeroStruct(*m_sLastParam);

    QWaitCursor wait;
    // Generate
    m_pHeightmap->GenerateTerrain(sParam);

    InvalidateTerrain();
}

void CTerrainDialog::OnExportHeightmap()
{
    char szFilters[] = "32-bit VTP BT (*.bt);;32-bit ARCGrid ASCII (*.asc);;32-bit TIF (*.tif);;16-bit PGM (*.pgm);;16-bit RAW (*.raw);;16-bit RAW (*.r16);;8-bit Bitmap (*.bmp);;8-bit PNG Files (*.png)";
    CAutoDirectoryRestoreFileDialog dlg(QFileDialog::AcceptSave, QFileDialog::AnyFile, "bt", Path::GetEditingGameDataFolder().c_str(), szFilters, {}, {}, this);
    if (dlg.exec())
    {
        QWaitCursor wait;
        CLogFile::WriteLine("Exporting heightmap...");

        QString fileName = dlg.selectedFiles().first();
        m_pHeightmap->ExportHeightmap(fileName);
    }
}

void CTerrainDialog::OnModifyMakeisle()
{
    ////////////////////////////////////////////////////////////////////////
    // Convert the heightmap to an island
    ////////////////////////////////////////////////////////////////////////
    if (!GetIEditor()->FlushUndo(true))
    {
        return;
    }

    QWaitCursor wait;

    // Call the make isle function of the heightmap class
    m_pHeightmap->MakeIsle();

    InvalidateTerrain();
}

void CTerrainDialog::Flatten(float flattenPercent)
{
    ////////////////////////////////////////////////////////////////////////
    // Increase the number of flat areas on the heightmap
    ////////////////////////////////////////////////////////////////////////

    QWaitCursor wait;

    // Heightmap expects a scalar factor to be applied
    float flattenFactor = 1.0f - (flattenPercent / 100.f);

    // Call the flatten function of the heightmap class
    m_pHeightmap->Flatten(flattenFactor);

    InvalidateTerrain();
}

void CTerrainDialog::OnModifyFlatten()
{
    // Create dialog to allow user to enter percentage to apply flattening by.
    bool ok = false;
    int fractionalDigitCount = 2;
    float value = aznumeric_caster(QInputDialog::getDouble(this, tr("Flatten Terrain"), tr("Percent to Flatten"), 0.0, 0.0, 100.0, fractionalDigitCount, &ok));

    if (ok)
    {
        if (!GetIEditor()->FlushUndo(true))
        {
            return;
        }

        Flatten(value);
    }
}

void CTerrainDialog::OnModifyRemoveOcean()
{
    //////////////////////////////////////////////////////////////////////
    // Remove ocean from the heightmap
    //////////////////////////////////////////////////////////////////////
    if (!GetIEditor()->FlushUndo(true))
    {
        return;
    }

    CLogFile::WriteLine("Removing ocean from heightmap...");

    QWaitCursor wait;

    // Using a ocean level <=0, if we reload the environment, it will
    // cause in method CTerrain::InitTerrainWater the SAFE_DELETE(m_pOcean);
    m_pHeightmap->SetOceanLevel(WATER_LEVEL_UNKNOWN);

    // Changed the InvalidateTerrain to include a environment reload.
    InvalidateTerrain();
}

void CTerrainDialog::OnModifySmoothSlope()
{
    //////////////////////////////////////////////////////////////////////
    // Remove areas with high slope from the heightmap
    //////////////////////////////////////////////////////////////////////
    if (!GetIEditor()->FlushUndo(true))
    {
        return;
    }

    QWaitCursor wait;

    // Call the smooth slope function of the heightmap class
    m_pHeightmap->SmoothSlope();

    InvalidateTerrain();
}

void CTerrainDialog::OnModifySmooth()
{
    //////////////////////////////////////////////////////////////////////
    // Smooth the heightmap
    //////////////////////////////////////////////////////////////////////

    if (!GetIEditor()->FlushUndo(true))
    {
        return;
    }

    QWaitCursor wait;

    m_pHeightmap->Smooth();
    InvalidateTerrain();
}

void CTerrainDialog::OnModifyNormalize()
{
    ////////////////////////////////////////////////////////////////////////
    // Normalize the heightmap
    ////////////////////////////////////////////////////////////////////////
    if (!GetIEditor()->FlushUndo(true))
    {
        return;
    }

    QWaitCursor wait;
    m_pHeightmap->Normalize();

    InvalidateTerrain();
}

void CTerrainDialog::OnModifyReduceRange()
{
    ////////////////////////////////////////////////////////////////////////
    // Reduce the value range of the heightmap (Heavy)
    ////////////////////////////////////////////////////////////////////////
    if (!GetIEditor()->FlushUndo(true))
    {
        return;
    }

    QWaitCursor wait;
    m_pHeightmap->LowerRange(0.8f);

    InvalidateTerrain();
}

void CTerrainDialog::OnModifyReduceRangeLight()
{
    ////////////////////////////////////////////////////////////////////////
    // Reduce the value range of the heightmap (Light)
    ////////////////////////////////////////////////////////////////////////
    if (!GetIEditor()->FlushUndo(true))
    {
        return;
    }

    QWaitCursor wait;
    m_pHeightmap->LowerRange(0.95f);

    InvalidateTerrain();
}

void CTerrainDialog::OnHeightmapShowLargePreview()
{
    ////////////////////////////////////////////////////////////////////////
    // Show a full-size version of the heightmap
    ////////////////////////////////////////////////////////////////////////

    setCursor(Qt::WaitCursor);

    CLogFile::WriteLine("Exporting heightmap...");

    UINT iWidth = m_pHeightmap->GetWidth();
    UINT iHeight = m_pHeightmap->GetHeight();

    CImageEx image;
    image.Allocate(iHeight, iWidth);   // swap x with y
    // Allocate memory to export the heightmap
    DWORD* pImageData = (DWORD*)image.GetData();

    bool bOk = false;

    // Get the full bitmap, no additional smoothing or noise, no water texture.
    bOk = m_pHeightmap->GetPreviewBitmap(pImageData, iWidth, false, false, NULL, false, m_ui->viewport->GetAutoScaleGreyRange());

    QString tempDirectory = Path::AddBackslash(gSettings.strStandardTempDirectory);
    QString imageName = "HeightmapPreview.bmp";

    if (bOk)
    {
        // Need to rotate the image 90 degrees so it displays externally in the same orientation as in the engine.
        CImageEx rotatedImage;
        rotatedImage.RotateOrt(image, ImageRotationDegrees::Rotate90);

        // Save the heightmap into the bitmap
        CFileUtil::CreateDirectory(tempDirectory.toUtf8().data());
        bOk = CImageUtil::SaveImage(tempDirectory + imageName, rotatedImage);
    }

    setCursor(Qt::ArrowCursor);

    if (bOk)
    {
        // Show the heightmap
        QDesktopServices::openUrl(QUrl::fromLocalFile(tempDirectory + imageName));
    }
}

void CTerrainDialog::OnTerrainLight()
{
    ////////////////////////////////////////////////////////////////////////
    // Show the terrain lighting tool
    ////////////////////////////////////////////////////////////////////////
    GetIEditor()->OpenView(LIGHTING_TOOL_WINDOW_NAME);
}

void CTerrainDialog::OnTerrainTextureImportExport()
{
    ////////////////////////////////////////////////////////////////////////
    // Show the terrain texture import/export tool
    ////////////////////////////////////////////////////////////////////////
    GetIEditor()->SetEditTool(nullptr);

    CTerrainTextureExport cDialog;
    cDialog.exec();
}

void CTerrainDialog::OnTerrainSurface()
{
    ////////////////////////////////////////////////////////////////////////
    // Show the terrain texture dialog
    ////////////////////////////////////////////////////////////////////////

    GetIEditor()->OpenView(LyViewPane::TerrainTextureLayers);
}

void CTerrainDialog::OnHold()
{
    // Hold the current heightmap state
    m_pHeightmap->Hold();
}

void CTerrainDialog::OnFetch()
{
    // Did we modify the heightmap ?
    if (GetIEditor()->IsModified())
    {
        // Ask first
        auto result = QMessageBox::question(this, tr("Restore State"), tr("Do you really want to restore the previous heightmap state ?"));

        // Abort
        if (QMessageBox::No == result)
        {
            return;
        }
    }

    // Restore the old heightmap state
    m_pHeightmap->Fetch();

    // We modified the document
    GetIEditor()->SetModifiedFlag();
    GetIEditor()->SetModifiedModule(eModifiedTerrain);

    InvalidateTerrain();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainDialog::OnOptionsShowMapObjects()
{
    m_ui->viewport->m_bShowStatObjects = !m_ui->viewport->m_bShowStatObjects;
    m_ui->actionShow_Map_Objects->setChecked(m_ui->viewport->m_bShowStatObjects);
    // Update the draw window
    InvalidateViewport();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainDialog::OnOptionsShowWater()
{
    m_ui->viewport->SetShowWater(!m_ui->viewport->GetShowWater());
    m_ui->actionShow_Water->setChecked(m_ui->viewport->GetShowWater());
    // Update the draw window
    InvalidateViewport();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainDialog::OnOptionsShowGrid()
{
    m_ui->viewport->SetShowGrid(!m_ui->viewport->GetShowGrid());
    m_ui->actionShow_Grid->setChecked(m_ui->viewport->GetShowGrid());
    // Update the draw window
    InvalidateViewport();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainDialog::OnOptionsAutoScaleGreyRange()
{
    m_ui->viewport->SetAutoScaleGreyRange(!m_ui->viewport->GetAutoScaleGreyRange());
    m_ui->actionAuto_Scale_Grey_Range->setChecked(m_ui->viewport->GetAutoScaleGreyRange());
    // Update the draw window
    InvalidateViewport();
}



/////////////////////////////////////////////////////////////////////////////
// Brushes

void CTerrainDialog::OnBrush1()
{
    if (m_pTerrainTool)
    {
        CTerrainBrush br;
        m_pTerrainTool->GetCurBrushParams(br);
        br.radiusInside = br.radius = 2;
        m_pTerrainTool->SetCurBrushParams(br);
    }
}

void CTerrainDialog::OnBrush2()
{
    if (m_pTerrainTool)
    {
        CTerrainBrush br;
        m_pTerrainTool->GetCurBrushParams(br);
        br.radiusInside = br.radius = 10;
        m_pTerrainTool->SetCurBrushParams(br);
    }
}

void CTerrainDialog::OnBrush3()
{
    if (m_pTerrainTool)
    {
        CTerrainBrush br;
        m_pTerrainTool->GetCurBrushParams(br);
        br.radiusInside = br.radius = 25;
        m_pTerrainTool->SetCurBrushParams(br);
    }
}

void CTerrainDialog::OnBrush4()
{
    if (m_pTerrainTool)
    {
        CTerrainBrush br;
        m_pTerrainTool->GetCurBrushParams(br);
        br.radiusInside = br.radius = 50;
        m_pTerrainTool->SetCurBrushParams(br);
    }
}

void CTerrainDialog::OnBrush5()
{
    if (m_pTerrainTool)
    {
        CTerrainBrush br;
        m_pTerrainTool->GetCurBrushParams(br);
        br.radiusInside = br.radius = 100;
        m_pTerrainTool->SetCurBrushParams(br);
    }
}

void CTerrainDialog::OnLowOpacity()
{
    if (m_pTerrainTool)
    {
        CTerrainBrush br;
        m_pTerrainTool->GetCurBrushParams(br);
        br.hardness = 0.2f;
        m_pTerrainTool->SetCurBrushParams(br);
    }
}

void CTerrainDialog::OnMediumOpacity()
{
    if (m_pTerrainTool)
    {
        CTerrainBrush br;
        m_pTerrainTool->GetCurBrushParams(br);
        br.hardness = 0.5f;
        m_pTerrainTool->SetCurBrushParams(br);
    }
}

void CTerrainDialog::OnHighOpacity()
{
    if (m_pTerrainTool)
    {
        CTerrainBrush br;
        m_pTerrainTool->GetCurBrushParams(br);
        br.hardness = 1.0;
        m_pTerrainTool->SetCurBrushParams(br);
    }
}


void CTerrainDialog::OnOptionsEditTerrainCurve()
{
}

void CTerrainDialog::OnSetOceanLevel()
{
    ////////////////////////////////////////////////////////////////////////
    // Let the user change the current ocean level
    ////////////////////////////////////////////////////////////////////////
    // Get the ocean level from the document and set it as default into
    // the dialog
    float fPreviousOceanLevel = GetIEditor()->GetHeightmap()->GetOceanLevel();
    bool ok = false;
    int fractionalDigitCount = 2;
    float oceanLevel = aznumeric_caster(QInputDialog::getDouble(this, tr("Set Ocean Height"), tr("Ocean height"), fPreviousOceanLevel, AZ::OceanConstants::s_HeightMin, AZ::OceanConstants::s_HeightMax, fractionalDigitCount, &ok));

    // Show the dialog
    if (ok)
    {
        // Save the new ocean level in the document
        GetIEditor()->GetHeightmap()->SetOceanLevel(oceanLevel);
        InvalidateTerrain();
        GetIEditor()->Notify(eNotify_OnTerrainRebuild);
    }
}

void CTerrainDialog::OnSetMaxHeight()
{
    // Set up dialog box to update Max Height
    float fValue = GetIEditor()->GetHeightmap()->GetMaxHeight();
    bool ok = false;
    int fractionalDigitCount = 2;
    fValue = aznumeric_caster(QInputDialog::getDouble(this, tr("Set Max Terrain Height"), tr("Maximum terrain height"), fValue, 1.0, 65536.0, fractionalDigitCount, &ok));

    if (ok)
    {
        // Set the new max height, but don't rescale the terrain.
        GetIEditor()->GetHeightmap()->SetMaxHeight(fValue, false);

        InvalidateTerrain();
    }
}

void CTerrainDialog::OnSetUnitSize()
{
    CHeightmap* heightmap = GetIEditor()->GetHeightmap();
    if (!heightmap)
    {
        return;
    }

    // Don't go further if a level hasn't been loaded, since the heightmap
    // resolution will be 0, causing a divide by 0
    uint64 terrainResolution = heightmap->GetWidth();
    if (terrainResolution <= 0)
    {
        return;
    }

    // Calculate valid unit sizes for the current terrain resolution
    int currentUnitSize = heightmap->GetUnitSize();
    int maxUnitSize = IntegerLog2(Ui::MAXIMUM_TERRAIN_RESOLUTION / terrainResolution);
    int units = Ui::START_TERRAIN_UNITS;
    QStringList unitSizes;
    int currentIndex = 0;
    for (int i = 0; i <= maxUnitSize; ++i)
    {
        // Find the index of our current unit size so we can make it selected
        // by default
        if (currentUnitSize == units)
        {
            currentIndex = i;
        }

        // Add this unit size to our list of available unit sizes
        unitSizes.append(QString::number(units));
        units *= 2;
    }

    bool ok = false;
    QString newUnitSize = QInputDialog::getItem(this, tr("Set Unit Size"), tr("Unit size (meters/texel)"), unitSizes, currentIndex, false, &ok);
    if (ok)
    {
        heightmap->Resize(terrainResolution, terrainResolution, newUnitSize.toInt(), false);
        InvalidateTerrain();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainDialog::InvalidateTerrain()
{
    GetIEditor()->SetModifiedFlag();
    GetIEditor()->SetModifiedModule(eModifiedTerrain);
    GetIEditor()->GetHeightmap()->UpdateEngineTerrain(true);

    if (m_pTerrainTool)
    {
        CTerrainBrush br;
        m_pTerrainTool->GetCurBrushParams(br);

        if (br.bRepositionVegetation && GetIEditor()->GetVegetationMap())
        {
            GetIEditor()->GetVegetationMap()->PlaceObjectsOnTerrain();
        }
        // Make sure objects preserve height.
        if (br.bRepositionObjects)
        {
            AABB box;
            box.min = -Vec3(100000, 100000, 100000);
            box.max = Vec3(100000, 100000, 100000);
            GetIEditor()->GetObjectManager()->SendEvent(EVENT_KEEP_HEIGHT, box);
        }
    }

    GetIEditor()->GetGameEngine()->ReloadEnvironment();

    InvalidateViewport();
    UpdateTerrainDimensions();
    Physics::EditorTerrainComponentRequestsBus::Broadcast(&Physics::EditorTerrainComponentRequests::UpdateHeightFieldAsset);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainDialog::InvalidateViewport()
{
    m_ui->viewport->update();
    GetIEditor()->UpdateViews(eUpdateHeightmap);
}


//////////////////////////////////////////////////////////////////////////
void CTerrainDialog::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnEndNewScene:
    case eNotify_OnEndSceneOpen:
    case eNotify_OnTerrainRebuild:
        m_ui->actionResize_Terrain->setEnabled(true);
        m_ui->viewport->InitHeightmapAlignment();
        InvalidateViewport();
        UpdateTerrainDimensions();
        break;
    case eNotify_OnEndLoad:
        m_ui->actionResize_Terrain->setEnabled(true);
        break;

        // Keep commented code. Customers can ask this functionality.
        /*
        case eNotify_OnEditToolChange:
            {
                // if edit tool is closed then set terrain modify tool back
                CEditTool* pMainTool = GetIEditor()->GetEditTool();
                if (m_pTerainTool && (!pMainTool || pMainTool->IsKindOf(RUNTIME_CLASS(CObjectMode))))
                {
                    GetIEditor()->SetEditTool(m_pTerainTool);
                }
            }
            break;
        */
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainDialog::OceanHeightChanged(float /*height*/)
{
    // If our ocean height has changed, and we're currently displaying water in our preview, 
    // then we need to refresh the preview.
    if (m_ui->viewport->GetShowWater())
    {
        InvalidateViewport();
    }
}


//////////////////////////////////////////////////////////////////////////
//void CTerrainDialog::OnCustomize()
//{
//    CMFCUtils::ShowShortcutsCustomizeDlg(GetCommandBars(), IDR_TERRAIN, "TerrainEditor");
//}
//
////////////////////////////////////////////////////////////////////////////
//void CTerrainDialog::OnExportShortcuts()
//{
//    CMFCUtils::ExportShortcuts(GetCommandBars()->GetShortcutManager());
//}
//
////////////////////////////////////////////////////////////////////////////
//void CTerrainDialog::OnImportShortcuts()
//{
//    CMFCUtils::ImportShortcuts(GetCommandBars()->GetShortcutManager(), "TerrainEditor");
//}

#include <TerrainDialog.moc>
