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
#include "Noise.h"
#include "Layer.h"
#include "CryEditDoc.h"
#include "VegetationMap.h"
#include "TerrainGrid.h"
#include "Util/DynamicArray2D.h"
#include "GameEngine.h"
#include "OceanConstants.h"
#include <Cry3DEngine/Environment/OceanEnvironmentBus.h>

#include "Terrain/TerrainManager.h"
#include "SurfaceType.h"
#include "Util/ImagePainter.h"

#include "Util/ImageASC.h"
#include "Util/ImageBT.h"
#include "Util/ImageTIF.h"

#include <I3DEngine.h>
#include <ITerrain.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Casting/lossy_cast.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

#include "QtUtil.h"

#include <QMessageBox>

#ifndef VERIFY
#define VERIFY(EXPRESSION) { auto e = EXPRESSION; assert(e); }
#endif


//! Size of terrain sector in units, sector size (in meters) depends from heightmap unit size - it gives more efficient heightmap triangulation
#define SECTOR_SIZE_IN_UNITS 32

//! Default Max Height value.
#define HEIGHTMAP_MAX_HEIGHT 150

//! Size of noise array.
#define NOISE_ARRAY_SIZE 512
#define NOISE_RANGE 255.0f

//! Filename used when Holding/Fetching heightmap.
#define HEIGHTMAP_HOLD_FETCH_FILE "Heightmap.hld"

#define OVVERIDE_LAYER_SURFACETYPE_FROM 128

const int kInitHeight = 32;

namespace
{
    inline void ClampHeight(float& h, float maxHeight)
    {
        h = min(maxHeight, max(0.0f, h));
    }

    inline void ClampToAverage(t_hmap* pValue, float fAverage, float maxHeight)
    {
        //////////////////////////////////////////////////////////////////////
        // Used during slope removal to clamp height values into a normalized
        // range
        //////////////////////////////////////////////////////////////////////

        float fClampedVal;

        // Does the height value differ heavily from the average value ?
        if (fabs(*pValue - fAverage) > fAverage * 0.001f)
        {
            // Negative / Positive ?
            if (*pValue < fAverage)
            {
                fClampedVal = fAverage - (fAverage * 0.001f);
            }
            else
            {
                fClampedVal = fAverage + (fAverage * 0.001f);
            }

            // Renormalize it
            ClampHeight(fClampedVal, maxHeight);

            *pValue = fClampedVal;
        }
    }

    inline void Smooth3x3(std::vector<t_hmap>& heightmap, int width, int x, int y, float maxHeight)
    {
        int iCurPos = (y * width) + x;
        // Get the average value for this area
        float fAverage = (
                heightmap[iCurPos] + heightmap[iCurPos + 1] + heightmap[iCurPos + width] +
                heightmap[iCurPos + width + 1] + heightmap[iCurPos - 1] + heightmap[iCurPos - width] +
                heightmap[iCurPos - width - 1] + heightmap[iCurPos - width + 1] + heightmap[iCurPos + width - 1])
            * 0.11111111111f;

        // Clamp the surrounding values to the given level
        ClampToAverage(&heightmap[iCurPos], fAverage, maxHeight);
        ClampToAverage(&heightmap[iCurPos + 1], fAverage, maxHeight);
        ClampToAverage(&heightmap[iCurPos + width], fAverage, maxHeight);
        ClampToAverage(&heightmap[iCurPos + width + 1], fAverage, maxHeight);
        ClampToAverage(&heightmap[iCurPos - 1], fAverage, maxHeight);
        ClampToAverage(&heightmap[iCurPos - width], fAverage, maxHeight);
        ClampToAverage(&heightmap[iCurPos - width - 1], fAverage, maxHeight);
        ClampToAverage(&heightmap[iCurPos - width + 1], fAverage, maxHeight);
        ClampToAverage(&heightmap[iCurPos + width - 1], fAverage, maxHeight);
    }
}

//////////////////////////////////////////////////////////////////////////
//! Undo object for heightmap modifications.
class CUndoHeightmapModify
    : public IUndoObject
{
public:
    CUndoHeightmapModify(int x1, int y1, int width, int height, CHeightmap* heightmap)
    {
        m_.Attach(heightmap->GetData(), heightmap->GetWidth(), heightmap->GetHeight());
        // Store heightmap block.
        m_rc = QRect(x1, y1, width, height);
        m_rc &= QRect(0, 0, m_.GetWidth(), m_.GetHeight());
        m_.GetSubImage(m_rc.left(), m_rc.top(), m_rc.width(), m_rc.height(), m_undo);
    }
protected:
    virtual void Release() { delete this; };
    virtual int GetSize() { return sizeof(*this) + m_undo.GetSize() + m_redo.GetSize(); };
    virtual QString GetDescription() { return "Heightmap Modify"; };

    virtual void Undo(bool bUndo)
    {
        if (bUndo)
        {
            // Store for redo.
            m_.GetSubImage(m_rc.left(), m_rc.top(), m_rc.width(), m_rc.height(), m_redo);
        }
        // Restore image.
        m_.SetSubImage(m_rc.left(), m_rc.top(), m_undo);

        int w = m_rc.width();
        if (w < m_rc.height())
        {
            w = m_rc.height();
        }

        GetIEditor()->GetHeightmap()->UpdateEngineTerrain(m_rc.left(), m_rc.top(), w, w, true, false);

        if (bUndo)
        {
            AABB box;
            box.min = GetIEditor()->GetHeightmap()->HmapToWorld(m_rc.topLeft());
            box.max = GetIEditor()->GetHeightmap()->HmapToWorld(QPoint(m_rc.left() + w, m_rc.top() + w));
            box.min.z -= 10000;
            box.max.z += 10000;
            GetIEditor()->UpdateViews(eUpdateHeightmap, &box);
        }
    }
    virtual void Redo()
    {
        if (m_redo.IsValid())
        {
            // Restore image.
            m_.SetSubImage(m_rc.left(), m_rc.top(), m_redo);
            GetIEditor()->GetHeightmap()->UpdateEngineTerrain(m_rc.left(), m_rc.top(), m_rc.width(), m_rc.height(), true, false);

            {
                AABB box;
                box.min = GetIEditor()->GetHeightmap()->HmapToWorld(m_rc.topLeft());
                box.max = GetIEditor()->GetHeightmap()->HmapToWorld(m_rc.bottomRight() + QPoint(1, 1));
                box.min.z -= 10000;
                box.max.z += 10000;
                GetIEditor()->UpdateViews(eUpdateHeightmap, &box);
            }
        }
    }

private:
    QRect m_rc;
    TImage<float> m_undo;
    TImage<float> m_redo;

    TImage<float> m_;       // memory data is shared
};

//////////////////////////////////////////////////////////////////////////
//! Undo object for heightmap modifications.
class CUndoHeightmapInfo
    : public IUndoObject
{
public:
    CUndoHeightmapInfo(int x1, int y1, int width, int height, CHeightmap* heightmap)
    {
        m_Weightmap.Attach(heightmap->m_Weightmap.GetData(), heightmap->GetWidth(), heightmap->GetHeight());
        // Store heightmap block.
        m_Weightmap.GetSubImage(x1, y1, width, height, m_undo);
        m_rc = QRect(x1, y1, width, height);
    }
protected:
    virtual void Release() { delete this; };
    virtual int GetSize()   { return sizeof(*this) + m_undo.GetSize() + m_redo.GetSize(); };
    virtual QString GetDescription() { return "Heightmap Hole"; };

    virtual void Undo(bool bUndo)
    {
        if (bUndo)
        {
            // Store for redo.
            m_Weightmap.GetSubImage(m_rc.left(), m_rc.top(), m_rc.width(), m_rc.height(), m_redo);
        }
        // Restore image.
        m_Weightmap.SetSubImage(m_rc.left(), m_rc.top(), m_undo);
        GetIEditor()->GetHeightmap()->UpdateEngineHole(m_rc.left(), m_rc.top(), m_rc.width(), m_rc.height());
    }
    virtual void Redo()
    {
        if (m_redo.IsValid())
        {
            // Restore image.
            m_Weightmap.SetSubImage(m_rc.left(), m_rc.top(), m_redo);
            GetIEditor()->GetHeightmap()->UpdateEngineHole(m_rc.left(), m_rc.top(), m_rc.width(), m_rc.height());
        }
    }

private:
    QRect m_rc;
    Weightmap m_undo;
    Weightmap m_redo;
    Weightmap m_Weightmap;
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHeightmap::CHeightmap()
    : m_fOceanLevel(AZ::OceanConstants::s_DefaultHeight)
    , m_fMaxHeight(HEIGHTMAP_MAX_HEIGHT)
    , m_iWidth(0)
    , m_iHeight(0)
    , m_textureSize(DEFAULT_HEIGHTMAP_SIZE)
    , m_numSectors(0)
    , m_unitSize(2)
    , m_TerrainBGRTexture("TerrainTexture.pak")
    , m_terrainGrid(new CTerrainGrid)
    , m_updateModSectors(false)
    , m_standaloneMode(false)
    , m_useTerrain(true)
{
    // is the feature toggle enabled for the ocean component feature enabled when the project includes the Water gem
    if (OceanToggle::IsActive())
    {
        m_fOceanLevel = AZ::OceanConstants::s_HeightUnknown;
    }
}

CHeightmap::CHeightmap(const CHeightmap& h)
    : m_fOceanLevel(h.GetOceanLevel())
    , m_fMaxHeight(h.m_fMaxHeight)
    , m_pHeightmap(h.m_pHeightmap)
    , m_Weightmap() // no copy ctor for this
    , m_iWidth(h.m_iWidth)
    , m_iHeight(h.m_iHeight)
    , m_textureSize(h.m_textureSize)
    , m_numSectors(h.m_numSectors)
    , m_unitSize(h.m_unitSize)
    , m_terrainGrid(new CTerrainGrid) // no copy ctor for this
    , m_TerrainBGRTexture("TerrainTexture.pak") // no copy ctor for this
    , m_modSectors(h.m_modSectors)
    , m_updateModSectors(h.m_updateModSectors)
    , m_standaloneMode(true) // always stand alone if copied
{
}

CHeightmap::~CHeightmap()
{
    // Reset the heightmap
    CleanUp();
}

//////////////////////////////////////////////////////////////////////////
void CHeightmap::CleanUp()
{
    // This could be a lot of memory, so we use swap to free it.
    // (Note: clear + shrink_to_fit is not actually guaranteed to release memory)
    std::vector<t_hmap> temp;
    m_pHeightmap.swap(temp);

    m_TerrainBGRTexture.FreeData();

    m_iWidth = 0;
    m_iHeight = 0;
}

//////////////////////////////////////////////////////////////////////////
void CHeightmap::SetUseTerrain(bool useTerrain)
{
    m_useTerrain = useTerrain;
}

//////////////////////////////////////////////////////////////////////////
bool CHeightmap::GetUseTerrain()
{
    return m_useTerrain;
}

//////////////////////////////////////////////////////////////////////////
void CHeightmap::Resize(int iWidth, int iHeight, int unitSize, bool bCleanOld, bool bForceKeepVegetation /*=false*/)
{
    if (iWidth <= 0 || iHeight <= 0)
    {
        return;
    }

    if (m_standaloneMode)
    {
        // TODO: Maybe allow layers and terrain grid to be updated in standalone mode ..
        // TODO: Non-destructive resize
        m_pHeightmap.clear();
        m_pHeightmap.resize(iWidth * iHeight);
        m_iWidth = iWidth;
        m_iHeight = iHeight;
        return;
    }

    int prevWidth, prevHeight, prevUnitSize;
    prevWidth = m_iWidth;
    prevHeight = m_iHeight;
    prevUnitSize = m_unitSize;

    std::vector<t_hmap> prevHeightmap;
    Weightmap prevWeightmap;

    if (bCleanOld)
    {
        // Free old heightmap
        CleanUp();
    }
    else
    {
        if (!m_pHeightmap.empty())
        {
            // Remember old state.
            prevHeightmap.swap(m_pHeightmap);
        }
        if (m_Weightmap.IsValid())
        {
            prevWeightmap.Allocate(m_iWidth, m_iHeight);
            memcpy(prevWeightmap.GetData(), m_Weightmap.GetData(), prevWeightmap.GetSize());
        }
    }

    // Save width and height
    m_iWidth = iWidth;
    m_iHeight = iHeight;
    m_unitSize = unitSize;

    int sectorSize = m_unitSize * SECTOR_SIZE_IN_UNITS;
    m_numSectors = (m_iWidth * m_unitSize) / sectorSize;

    bool            boChangedTerrainDimensions(prevWidth != m_iWidth || prevHeight != m_iHeight || prevUnitSize != m_unitSize);
    ITerrain* pTerrain = GetIEditor()->Get3DEngine()->GetITerrain();

    uint32 dwTerrainSizeInMeters = m_iWidth * unitSize;
    uint32 dwTerrainTextureResolution = dwTerrainSizeInMeters / 2;          // terrain texture resolution scaled with heightmap - bad: hard coded factor 2m=1texel
    dwTerrainTextureResolution = min(dwTerrainTextureResolution, (uint32)16384);

    uint32 dwTileResolution = 512;          // to avoid having too many tiles we try to get big ones

    // As dwTileCount must be at least 1, dwTerrainTextureResolution must be at most equals to
    // dwTerrainTextureResolution.
    if (dwTerrainTextureResolution <= dwTileResolution)
    {
        dwTileResolution = dwTerrainTextureResolution;
    }

    uint32 dwTileCount = dwTerrainTextureResolution / dwTileResolution;

    m_TerrainBGRTexture.AllocateTiles(dwTileCount, dwTileCount, dwTileResolution);

    // Allocate new data (reuses capacity if the new map is smaller)
    m_pHeightmap.resize(iWidth * iHeight);

    CryLog("allocating editor height map (%dx%d)*4", iWidth, iHeight);

    Verify();

    m_Weightmap.Allocate(iWidth, iHeight);

    if (prevWidth < m_iWidth || prevHeight < m_iHeight || prevUnitSize < m_unitSize)
    {
        Clear();
    }

    if (bCleanOld)
    {
        // Set to zero
        Clear();
    }
    else
    {
        // Copy from previous data.
        if (!prevHeightmap.empty() && prevWeightmap.IsValid())
        {
            QWaitCursor wait;
            if (prevUnitSize == m_unitSize)
            {
                CopyFrom(&prevHeightmap[0], prevWeightmap.GetData(), prevWidth);
            }
            else
            {
                CopyFromInterpolate(&prevHeightmap[0], prevWeightmap.GetData(), prevWidth, prevUnitSize);
            }
        }
    }

    m_terrainGrid->InitSectorGrid(m_numSectors);
    m_terrainGrid->SetResolution(m_textureSize);


    // This must run only when creating a new terrain.
    if (!boChangedTerrainDimensions || !pTerrain)
    {
        CVegetationMap* pVegetationMap = GetIEditor()->GetVegetationMap();
        if (pVegetationMap)
        {
            pVegetationMap->Allocate(m_unitSize * max(m_iWidth, m_iHeight), bForceKeepVegetation | !bCleanOld);
        }
    }

    if (!bCleanOld)
    {
        int numLayers = GetIEditor()->GetTerrainManager()->GetLayerCount();
        for (int i = 0; i < numLayers; i++)
        {
            CLayer* pLayer = GetIEditor()->GetTerrainManager()->GetLayer(i);
            pLayer->SetSectorInfoSurfaceTextureSize();
        }
    }

    if (boChangedTerrainDimensions)
    {
        if (pTerrain)
        {
            static bool bNoReentrant = false;
            if (!bNoReentrant)
            {
                bNoReentrant = true;
                // This will reload the level with the new terrain size.
                GetIEditor()->GetDocument()->Hold("_tmpResize");
                GetIEditor()->GetDocument()->Fetch("_tmpResize", false, true);
                bNoReentrant = false;
            }
        }
    }

    NotifyModified();
}


//////////////////////////////////////////////////////////////////////////
void CHeightmap::PaintLayerId(const float fpx, const float fpy, const SEditorPaintBrush& brush, const uint32 dwLayerId)
{
    if (m_standaloneMode)
    {
        // TODO: Implement for standalone mode (pass vox as a parameter!)
        return;
    }

    assert(dwLayerId <= CLayer::e_hole);
    uint8 ucLayerInfoData = dwLayerId & (CLayer::e_hole | CLayer::e_undefined);

    SEditorPaintBrush cpyBrush = brush;

    cpyBrush.bBlended = true;
    cpyBrush.color = ucLayerInfoData;

    CImagePainter painter;

    painter.PaintBrush(fpx, fpy, m_Weightmap, cpyBrush);
}


//////////////////////////////////////////////////////////////////////////
void CHeightmap::EraseLayerID(uint8 id, uint8 replacementId)
{
    for (uint32 y = 0; y < m_iHeight; ++y)
    {
        for (uint32 x = 0; x < m_iWidth; ++x)
        {
            LayerWeight& weight = m_Weightmap.ValueAt(x, y);
            weight.EraseLayer(id, replacementId);
        }
    }
    UpdateEngineTerrain(false, true);
}


//////////////////////////////////////////////////////////////////////////
void CHeightmap::MarkUsedLayerIds(bool bFree[CLayer::e_undefined]) const
{
    for (uint32 dwY = 0; dwY < m_iHeight; ++dwY)
    {
        for (uint32 dwX = 0; dwX < m_iWidth; ++dwX)
        {
            LayerWeight weight = GetLayerWeightAt(dwX, dwY);

            for (int i = 0; i < weight.WeightCount; ++i)
            {
                uint8 id = weight.GetLayerId(i);
                if (id < CLayer::e_undefined)
                {
                    bFree[id] = false;
                }
            }
        }
    }
}


//////////////////////////////////////////////////////////////////////////
QPoint CHeightmap::WorldToHmap(const Vec3& wp) const
{
    //swap x/y.
    return QPoint(wp.y / m_unitSize, wp.x / m_unitSize);
}

//////////////////////////////////////////////////////////////////////////
Vec3 CHeightmap::HmapToWorld(const QPoint& hpos) const
{
    return Vec3(hpos.y() * m_unitSize, hpos.x() * m_unitSize, 0);
}

//////////////////////////////////////////////////////////////////////////
QRect CHeightmap::WorldBoundsToRect(const AABB& worldBounds) const
{
    QPoint p1 = WorldToHmap(worldBounds.min);
    QPoint p2 = WorldToHmap(worldBounds.max);
    if (p1.x() > p2.x())
    {
        std::swap(p1.rx(), p2.rx());
    }
    if (p1.y() > p2.y())
    {
        std::swap(p1.ry(), p2.ry());
    }
    QRect rc(p1, p2);
    return rc.intersected(QRect(0, 0, m_iWidth, m_iHeight));
}


void CHeightmap::Clear(bool bClearLayerBitmap)
{
    if (m_iWidth && m_iHeight)
    {
        InitHeight(kInitHeight);

        if (bClearLayerBitmap)
        {
            for (int y = 0; y < m_Weightmap.GetHeight(); ++y)
            {
                for (int x = 0; x < m_Weightmap.GetWidth(); ++x)
                {
                    m_Weightmap.ValueAt(x, y) = LayerWeight(0);
                }
            }
        }
    }
}


void CHeightmap::InitHeight(float fHeight)
{
    if (fHeight > m_fMaxHeight)
    {
        fHeight = m_fMaxHeight;
    }

    for (int i = 0; i < m_iWidth; ++i)
    {
        for (int j = 0; j < m_iHeight; ++j)
        {
            SetXY(i, j, fHeight);
        }
    }
}


void CHeightmap::SetMaxHeight(float fMaxHeight, bool scaleHeightmap)
{
    float prevHeight = m_fMaxHeight;

    m_fMaxHeight = fMaxHeight;
    if (m_fMaxHeight < 1.0f)
    {
        m_fMaxHeight = 1.0f;
    }

    float scaleFactor = scaleHeightmap ? ((m_fMaxHeight - 1.0f) / prevHeight) : 1.0f;

    // Scale heightmap to the max height.
    int nSize = GetWidth() * GetHeight();
    for (int i = 0; i < nSize; i++)
    {
        m_pHeightmap[i] *= scaleFactor;

        if (m_pHeightmap[i] > m_fMaxHeight)
        {
            m_pHeightmap[i] = m_fMaxHeight;
        }
    }
}

void CHeightmap::LoadASC(const QString& fileName)
{
    CFloatImage tmpImage;

    if (!CImageASC().Load(fileName, tmpImage))
    {
        QMessageBox::critical(AzToolsFramework::GetActiveWindow(), QString(), QObject::tr("Load image failed."));
        return;
    }

    ProcessLoadedImage(fileName, tmpImage, false, ImageRotationDegrees::Rotate270);
}

void CHeightmap::SaveASC(const QString& fileName)
{
    std::shared_ptr<CFloatImage> image = GetHeightmapFloatImage(false, ImageRotationDegrees::Rotate90);

    CImageASC().Save(fileName, *image);
}

void CHeightmap::LoadBT(const QString& fileName)
{
    CFloatImage tmpImage;

    if (!CImageBT().Load(fileName, tmpImage))
    {
        QMessageBox::critical(AzToolsFramework::GetActiveWindow(), QString(), QObject::tr("Load image failed."));
        return;
    }

    ProcessLoadedImage(fileName, tmpImage, false, ImageRotationDegrees::Rotate0);
}

void CHeightmap::SaveBT(const QString& fileName)
{
    std::shared_ptr<CFloatImage> image = GetHeightmapFloatImage(false, ImageRotationDegrees::Rotate0);

    CImageBT().Save(fileName, *image);
}

void CHeightmap::LoadTIF(const QString& fileName)
{
    CFloatImage tmpImage;

    if (!CImageTIF().Load(fileName, tmpImage))
    {
        QMessageBox::critical(AzToolsFramework::GetActiveWindow(), QString(), QObject::tr("Load image failed."));
        return;
    }

    ProcessLoadedImage(fileName, tmpImage, false, ImageRotationDegrees::Rotate270);
}

void CHeightmap::SaveTIF(const QString& fileName)
{
    std::shared_ptr<CFloatImage> image = GetHeightmapFloatImage(true, ImageRotationDegrees::Rotate90);

    CImageTIF().SaveRAW(fileName, image->GetData(), image->GetWidth(), image->GetHeight(), sizeof(float), 1, true, NULL);
}

void CHeightmap::LoadImage(const QString& fileName)
{
    // Load either 8-bit or 16-bit images (BMP, TIF, PGM, ASC) into the heightmap.

    CImageEx tmpImage;
    CFloatImage floatImage;

    if (!CImageUtil::LoadImage(fileName, tmpImage))
    {
        QMessageBox::critical(AzToolsFramework::GetActiveWindow(), QString(), QObject::tr("Load image failed."));
        return;
    }

    if (!tmpImage.ConvertToFloatImage(floatImage))
    {
        QMessageBox::critical(AzToolsFramework::GetActiveWindow(), QString(), QObject::tr("Load image failed."));
        return;
    }

    ProcessLoadedImage(fileName, floatImage, false, ImageRotationDegrees::Rotate270);
}

bool CHeightmap::ProcessLoadedImage(const QString& fileName, const CFloatImage& tmpImage, bool atWorldScale, ImageRotationDegrees rotationAmount)
{
    CFloatImage image;
    CFloatImage hmap;

    bool resizedImage = false;

    // 270-degree rotation is necessary to match what is displayed in image editing tools
    image.RotateOrt(tmpImage, rotationAmount);

    if (image.GetWidth() != m_iWidth || image.GetHeight() != m_iHeight)
    {
        // If our width / height doesn't match, find out if the user would rather clip the rectangle or scale it.
        QString str = QObject::tr("Image dimensions do not match dimensions of heightmap.\nImage size is %1x%2,\nHeightmap size is %3x%4.\nWould you like to clip the image, resize it, or cancel?")
                .arg(image.GetWidth()).arg(image.GetHeight()).arg(m_iWidth).arg(m_iHeight);

        QMessageBox userPrompt(AzToolsFramework::GetActiveWindow());
        userPrompt.setWindowTitle(QObject::tr("Warning"));
        userPrompt.setText(str);

        QAbstractButton* clipButton = (QAbstractButton*)userPrompt.addButton(QObject::tr("Clip"), QMessageBox::YesRole);
        QAbstractButton* resizeButton = (QAbstractButton*)userPrompt.addButton(QObject::tr("Resize"), QMessageBox::YesRole);
        QAbstractButton* cancelButton = (QAbstractButton*)userPrompt.addButton(QObject::tr("Cancel"), QMessageBox::RejectRole);

        userPrompt.exec();

        // If clip, just use the values as-is.  If the image boundary is larger, extra pixels will be dropped.  If the heightmap boundary is larger,
        // values outside the image bounds will be set to 0.
        if (userPrompt.clickedButton() == clipButton)
        {
            hmap.Attach(image);
        }
        // If resize, we'll stretch or shrink the image to fit.  Note that this will cause stairstep artifacts.
        else if (userPrompt.clickedButton() == resizeButton)
        {
            hmap.Allocate(m_iWidth, m_iHeight);
            hmap.ScaleToFit(image);

            resizedImage = true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        hmap.Attach(image);
    }

    // Scale from 0 - 1 to 0 - maxHeight.
    // Note that the scaling is based on max possible values, not max values in the heightmap.
    float heightScale = atWorldScale ? 1.0f : m_fMaxHeight;

    // Either grab 8-bit or 16-bit values from the image and scale based on the max possible value.
    for (int32 y = 0, imageHeight = hmap.GetHeight(); y < m_iHeight; y++)
    {
        for (int32 x = 0, imageWidth = hmap.GetWidth(); x < m_iWidth; x++)
        {
            if ((x < imageWidth) && (y < imageHeight))
            {
                float srcHeight = hmap.ValueAt(x, y);
                float scaledHeight = clamp_tpl(srcHeight * heightScale, 0.0f, m_fMaxHeight);

                SetXY(x, y, scaledHeight);
            }
            else
            {
                // Clear any height values outside what we're importing.
                SetXY(x, y, 0.0f);
            }
        }
    }

    NotifyModified();

    CLogFile::FormatLine("Heightmap loaded from file: %s.  Input size %d x %d %s into heightmap of size %d x %d.",
        fileName.toUtf8().data(),
        image.GetWidth(), image.GetHeight(),
        (resizedImage ? "resized" : "copied"),
        m_iWidth, m_iHeight);
    return true;
}

void CHeightmap::SaveImage(LPCSTR pszFileName) const
{
    std::shared_ptr<CImageEx> image = GetHeightmapImageEx();

    CImageEx newImage;
    newImage.RotateOrt(*image, ImageRotationDegrees::Rotate90);

    // Save the heightmap into the bitmap
    CImageUtil::SaveImage(pszFileName, newImage);
}




void CHeightmap::SaveImage16Bit(const QString& fileName)
{
    CImageEx image;
    image.Allocate(m_iWidth, m_iHeight);

    float fPrecisionScale = GetShortPrecisionScale();
    for (int j = 0; j < m_iHeight; j++)
    {
        for (int i = 0; i < m_iWidth; i++)
        {
            unsigned int h = ftoi(GetXY(i, j) * fPrecisionScale + 0.5f);
            if (h > 0xFFFF)
            {
                h = 0xFFFF;
            }
            image.ValueAt(i, j) = ftoi(h);
        }
    }

    CImageEx newImage;
    newImage.RotateOrt(image, ImageRotationDegrees::Rotate90);
    CImageUtil::SaveImage(fileName, newImage);
}

//! Save heightmap in RAW format.
void CHeightmap::SaveRAW(const QString& rawFile)
{
    FILE* file = fopen(rawFile.toUtf8().data(), "wb");
    if (!file)
    {
        QMessageBox::warning(AzToolsFramework::GetActiveWindow(), QObject::tr("Warning"), QObject::tr("Error saving file %1").arg(rawFile));
        return;
    }

    CWordImage image;
    image.Allocate(m_iWidth, m_iHeight);

    float fPrecisionScale = GetShortPrecisionScale();

    for (int j = 0; j < m_iHeight; j++)
    {
        for (int i = 0; i < m_iWidth; i++)
        {
            unsigned int h = ftoi(GetXY(i, j) * fPrecisionScale + 0.5f);
            if (h > 0xFFFF)
            {
                h = 0xFFFF;
            }
            image.ValueAt(i, j) = h;
        }
    }

    CWordImage newImage;
    newImage.RotateOrt(image, ImageRotationDegrees::Rotate90);

    fwrite(newImage.GetData(), newImage.GetSize(), 1, file);

    fclose(file);
}

//! Load heightmap from RAW format.
void    CHeightmap::LoadRAW(const QString& rawFile)
{
    FILE* file = fopen(rawFile.toUtf8().data(), "rb");
    if (!file)
    {
        QMessageBox::warning(AzToolsFramework::GetActiveWindow(), QObject::tr("Warning"), QObject::tr("Error loading file %1").arg(rawFile));
        return;
    }
    fseek(file, 0, SEEK_END);
    int fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (fileSize != m_iWidth * m_iHeight * 2)
    {
        QMessageBox::warning(AzToolsFramework::GetActiveWindow(), QObject::tr("Warning"), QObject::tr("Bad RAW file, RAW file must be %1x%2 16bit image").arg(m_iWidth).arg(m_iHeight));
        fclose(file);
        return;
    }

    CFloatImage tmpImage;
    tmpImage.Allocate(m_iWidth, m_iHeight);
    float* pixels = tmpImage.GetData();

    for (int j = 0; j < m_iHeight; j++)
    {
        for (int i = 0; i < m_iWidth; i++)
        {
            uint16 pixel;
            fread(&pixel, sizeof(uint16), 1, file);
            *pixels++ = static_cast<float>(pixel) / static_cast<float>(std::numeric_limits<uint16>::max());
        }
    }

    fclose(file);

    ProcessLoadedImage(rawFile, tmpImage, false, ImageRotationDegrees::Rotate270);
}

void CHeightmap::Noise()
{
    ////////////////////////////////////////////////////////////////////////
    // Add noise to the heightmap
    ////////////////////////////////////////////////////////////////////////
    InitNoise();
    assert(m_pNoise);

    Verify();

    const float fInfluence = 10.0f;

    // Calculate the way we have to swap the noise. We do this to avoid
    // singularities when a noise array is aplied more than once
    srand(clock());
    UINT iNoiseSwapMode = rand() % 5;

    // Calculate a noise offset for the same reasons
    UINT iNoiseOffsetX = rand() % NOISE_ARRAY_SIZE;
    UINT iNoiseOffsetY = rand() % NOISE_ARRAY_SIZE;

    UINT iNoiseX, iNoiseY;

    for (uint64 j = 1; j < m_iHeight - 1; j++)
    {
        // Precalculate for better speed
        uint64 iCurPos = j * m_iWidth + 1;

        for (uint64 i = 1; i < m_iWidth - 1; i++)
        {
            // Next pixel
            iCurPos++;

            // Skip amnything below the ocean water level
            if (m_pHeightmap[iCurPos] > 0.0f && m_pHeightmap[iCurPos] >= GetOceanLevel())
            {
                // Swap the noise
                switch (iNoiseSwapMode)
                {
                case 0:
                    iNoiseX = i;
                    iNoiseY = j;
                    break;
                case 1:
                    iNoiseX = j;
                    iNoiseY = i;
                    break;
                case 2:
                    iNoiseX = m_iWidth - i;
                    iNoiseY = j;
                    break;
                case 3:
                    iNoiseX = i;
                    iNoiseY = m_iHeight - j;
                    break;
                case 4:
                    iNoiseX = m_iWidth - i;
                    iNoiseY = m_iHeight - j;
                    break;
                }

                // Add the random noise offset
                iNoiseX += iNoiseOffsetX;
                iNoiseY += iNoiseOffsetY;

                float fInfluenceValue = GetNoise(iNoiseX, iNoiseY) / NOISE_RANGE * fInfluence - fInfluence / 2;

                // Add the signed noise
                m_pHeightmap[iCurPos] = __min(m_fMaxHeight, __max(GetOceanLevel(), m_pHeightmap[iCurPos] + fInfluenceValue));
            }
        }
    }

    NotifyModified();
}

//////////////////////////////////////////////////////////////////////////
void CHeightmap::Smooth(CFloatImage& hmap, const QRect& rect) const
{
    int w = hmap.GetWidth();
    int h = hmap.GetHeight();

    int x1 = max((int)rect.left() + 2, 1);
    int y1 = max((int)rect.top() + 2, 1);
    int x2 = min((int)rect.right() - 2, w - 1);
    int y2 = min((int)rect.bottom() - 2, h - 1);

    t_hmap* pData = hmap.GetData();

    int i, j, pos;
    // Smooth it
    for (j = y1; j < y2; j++)
    {
        pos = j * w;
        for (i = x1; i < x2; i++)
        {
            pData[i + pos] =
                (pData[i + pos] +
                 pData[i + 1 + pos] +
                 pData[i - 1 + pos] +
                 pData[i + pos + w] +
                 pData[i + pos - w] +
                 pData[(i - 1) + pos - w] +
                 pData[(i + 1) + pos - w] +
                 pData[(i - 1) + pos + w] +
                 pData[(i + 1) + pos + w])
                * (1.0f / 9.0f);
        }
    }
}

void CHeightmap::Smooth()
{
    ////////////////////////////////////////////////////////////////////////
    // Smooth the heightmap
    ////////////////////////////////////////////////////////////////////////
    if (!IsAllocated())
    {
        return;
    }

    unsigned int i, j;

    Verify();

    // Smooth it
    for (i = 1; i < m_iWidth - 1; i++)
    {
        for (j = 1; j < m_iHeight - 1; j++)
        {
            m_pHeightmap[i + j * m_iWidth] =
                (m_pHeightmap[i + j * m_iWidth] +
                 m_pHeightmap[(i + 1) + j * m_iWidth] +
                 m_pHeightmap[i + (j + 1) * m_iWidth] +
                 m_pHeightmap[(i + 1) + (j + 1) * m_iWidth] +
                 m_pHeightmap[(i - 1) + j * m_iWidth] +
                 m_pHeightmap[i + (j - 1) * m_iWidth] +
                 m_pHeightmap[(i - 1) + (j - 1) * m_iWidth] +
                 m_pHeightmap[(i + 1) + (j - 1) * m_iWidth] +
                 m_pHeightmap[(i - 1) + (j + 1) * m_iWidth])
                / 9.0f;
        }
    }

    NotifyModified();
}

void CHeightmap::Invert()
{
    ////////////////////////////////////////////////////////////////////////
    // Invert the heightmap
    ////////////////////////////////////////////////////////////////////////

    unsigned int i;

    Verify();

    for (i = 0; i < m_iWidth * m_iHeight; i++)
    {
        m_pHeightmap[i] = m_fMaxHeight - m_pHeightmap[i];
        if (m_pHeightmap[i] > m_fMaxHeight)
        {
            m_pHeightmap[i] = m_fMaxHeight;
        }
        if (m_pHeightmap[i] < 0)
        {
            m_pHeightmap[i] = 0;
        }
    }

    NotifyModified();
}

void CHeightmap::Normalize()
{
    ////////////////////////////////////////////////////////////////////////
    // Normalize the heightmap to a 0 - m_fMaxHeight range
    ////////////////////////////////////////////////////////////////////////

    unsigned int i, j;
    float fLowestPoint = 512000.0f, fHighestPoint = -512000.0f;
    float fValueRange;
    float fHeight;

    Verify();

    // Find the value range
    for (i = 0; i < m_iWidth; i++)
    {
        for (j = 0; j < m_iHeight; j++)
        {
            fLowestPoint = __min(fLowestPoint, GetXY(i, j));
            fHighestPoint = __max(fHighestPoint, GetXY(i, j));
        }
    }

    // Storing the value range in this way saves us a division and a multiplication
    fValueRange = (1.0f / (fHighestPoint - (float)fLowestPoint)) * m_fMaxHeight;

    // Normalize the heightmap
    for (i = 0; i < m_iWidth; i++)
    {
        for (j = 0; j < m_iHeight; j++)
        {
            fHeight = GetXY(i, j);
            fHeight -= fLowestPoint;
            fHeight *= fValueRange;
            SetXY(i, j, fHeight);
        }
    }

    NotifyModified();
}

bool CHeightmap::GetDataEx(t_hmap* pData, UINT iDestWidth, bool bSmooth, bool bNoise) const
{
    long iXSrcFl, iXSrcCe, iYSrcFl, iYSrcCe;
    float fXSrc, fYSrc;
    float fHeight[4];
    float fHeightWeight[4];
    float fHeightBottom;
    float fHeightTop;
    UINT dwHeightmapWidth = GetWidth();
    t_hmap* pDataStart = pData;

    const bool bProgress = iDestWidth > 1024;

    // Only log significant allocations. This also prevents us from cluttering the
    // log file during the lightmap preview generation
    if (bProgress)
    {
        CLogFile::FormatLine("Retrieving heightmap data (Width: %i)...", iDestWidth);
    }

    std::unique_ptr<CWaitProgress> wait;
    if (!m_standaloneMode)
    {
        wait = std::make_unique<CWaitProgress>("Scaling Heightmap", bProgress);
    }

    // Loop trough each field of the new image and interpolate the value
    // from the source heightmap
    for (UINT j = 0; j < iDestWidth; ++j)
    {
        if (bProgress && !m_standaloneMode)
        {
            if (!wait->Step(j * 100 / iDestWidth))
            {
                return false;
            }
        }

        // Calculate the average source array position
        fYSrc = ((float)j / (float)iDestWidth) * dwHeightmapWidth;
        assert(fYSrc >= 0.0f && fYSrc <= dwHeightmapWidth);

        // Precalculate floor and ceiling values. Use fast asm integer floor and
        // fast asm float / integer conversion
        iYSrcFl = ifloor(fYSrc);
        iYSrcCe = iYSrcFl + 1;

        // Clamp the ceiling coordinates to a save range
        if (iYSrcCe >= (int)dwHeightmapWidth)
        {
            iYSrcCe = dwHeightmapWidth - 1;
        }

        // Distribution between top and bottom height values
        fHeightWeight[3] = fYSrc - (float)iYSrcFl;
        fHeightWeight[2] = 1.0f - fHeightWeight[3];

        for (UINT i = 0; i < iDestWidth; ++i)
        {
            // Calculate the average source array position
            fXSrc = ((float)i / (float)iDestWidth) * dwHeightmapWidth;
            assert(fXSrc >= 0.0f && fXSrc <= dwHeightmapWidth);

            // Precalculate floor and ceiling values. Use fast asm integer floor and
            // fast asm float / integer conversion
            iXSrcFl = ifloor(fXSrc);
            iXSrcCe = iXSrcFl + 1;

            // Distribution between left and right height values
            fHeightWeight[1] = fXSrc - (float)iXSrcFl;
            fHeightWeight[0] = 1.0f - fHeightWeight[1];

            if (iXSrcCe >= (int)dwHeightmapWidth)
            {
                iXSrcCe = dwHeightmapWidth - 1;
            }

            // Get the four nearest height values
            fHeight[0] = (float)m_pHeightmap[iXSrcFl + iYSrcFl * dwHeightmapWidth];
            fHeight[1] = (float)m_pHeightmap[iXSrcCe + iYSrcFl * dwHeightmapWidth];
            fHeight[2] = (float)m_pHeightmap[iXSrcFl + iYSrcCe * dwHeightmapWidth];
            fHeight[3] = (float)m_pHeightmap[iXSrcCe + iYSrcCe * dwHeightmapWidth];

            // Interpolate between the four nearest height values

            // Get the height for the given X position trough interpolation between
            // the left and the right height
            fHeightBottom = (fHeight[0] * fHeightWeight[0] + fHeight[1] * fHeightWeight[1]);
            fHeightTop = (fHeight[2] * fHeightWeight[0] + fHeight[3] * fHeightWeight[1]);

            // Set the new value in the destination heightmap
            *pData++ = static_cast<t_hmap>(fHeightBottom * fHeightWeight[2] + fHeightTop * fHeightWeight[3]);
        }
    }

    if (bNoise)
    {
        InitNoise();

        pData = pDataStart;
        // Smooth it
        for (int i = 1; i < iDestWidth - 1; i++)
        {
            for (int j = 1; j < iDestWidth - 1; j++)
            {
                *pData++ += cry_random(0.0f, 1.0f / 16.0f);
            }
        }
    }

    if (bSmooth)
    {
        CFloatImage img;
        img.Attach(pDataStart, iDestWidth, iDestWidth);
        Smooth(img, QRect(0, 0, iDestWidth, iDestWidth));
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CHeightmap::GetData(const QRect& srcRect, const int resolution, const QPoint& vTexOffset, CFloatImage& hmap, bool bSmooth, bool bNoise)
{
    if (m_pHeightmap.empty())
    {
        return false;
    }

    int iXSrcFl, iXSrcCe, iYSrcFl, iYSrcCe;
    float fXSrc, fYSrc;
    float fHeight[4];
    float fHeightWeight[4];
    float fHeightBottom;
    float fHeightTop;
    UINT dwHeightmapWidth = GetWidth();

    int width = hmap.GetWidth();

    // clip within source
    int x1 = max((int)srcRect.left(), 0);
    int y1 = max((int)srcRect.top(), 0);
    int x2 = min((int)srcRect.right() + 1, resolution);
    int y2 = min((int)srcRect.bottom() + 1, resolution);

    // clip within dest hmap
    x1 = max(x1, (int)vTexOffset.x());
    y1 = max(y1, (int)vTexOffset.y());
    x2 = min(x2, hmap.GetWidth() + (int)vTexOffset.x());
    y2 = min(y2, hmap.GetHeight() + (int)vTexOffset.y());

    float fScaleX = m_iWidth / (float)hmap.GetWidth();
    float fScaleY = m_iHeight / (float)hmap.GetHeight();

    t_hmap* pDataStart = hmap.GetData();

    int trgW = x2 - x1;

    bool bProgress = trgW > 1024;
    CWaitProgress wait("Scaling Heightmap", bProgress);

    // Loop trough each field of the new image and interpolate the value
    // from the source heightmap
    for (int j = y1; j < y2; ++j)
    {
        if (bProgress)
        {
            if (!wait.Step((j - y1) * 100 / (y2 - y1)))
            {
                return false;
            }
        }

        t_hmap* pData = &pDataStart[(j - vTexOffset.y()) * width + x1 - vTexOffset.x()];

        // Calculate the average source array position
        fYSrc = j * fScaleY;
        assert(fYSrc >= 0.0f && fYSrc <= dwHeightmapWidth);

        // Precalculate floor and ceiling values. Use fast asm integer floor and
        // fast asm float / integer conversion
        iYSrcFl = ifloor(fYSrc);
        iYSrcCe = iYSrcFl + 1;

        // Clamp the ceiling coordinates to a save range
        if (iYSrcCe >= (int)dwHeightmapWidth)
        {
            iYSrcCe = dwHeightmapWidth - 1;
        }


        // Distribution between top and bottom height values
        fHeightWeight[3] = fYSrc - (float)iYSrcFl;
        fHeightWeight[2] = 1.0f - fHeightWeight[3];

        for (int i = x1; i < x2; ++i)
        {
            // Calculate the average source array position
            fXSrc = i * fScaleX;
            assert(fXSrc >= 0.0f && fXSrc <= dwHeightmapWidth);

            // Precalculate floor and ceiling values. Use fast asm integer floor and
            // fast asm float / integer conversion
            iXSrcFl = ifloor(fXSrc);
            iXSrcCe = iXSrcFl + 1;

            if (iXSrcCe >= (int)dwHeightmapWidth)
            {
                iXSrcCe = dwHeightmapWidth - 1;
            }

            // Distribution between left and right height values
            fHeightWeight[1] = fXSrc - (float)iXSrcFl;
            fHeightWeight[0] = 1.0f - fHeightWeight[1];

            // Get the four nearest height values
            fHeight[0] = (float)m_pHeightmap[iXSrcFl + iYSrcFl * dwHeightmapWidth];
            fHeight[1] = (float)m_pHeightmap[iXSrcCe + iYSrcFl * dwHeightmapWidth];
            fHeight[2] = (float)m_pHeightmap[iXSrcFl + iYSrcCe * dwHeightmapWidth];
            fHeight[3] = (float)m_pHeightmap[iXSrcCe + iYSrcCe * dwHeightmapWidth];

            // Interpolate between the four nearest height values

            // Get the height for the given X position trough interpolation between
            // the left and the right height
            fHeightBottom = (fHeight[0] * fHeightWeight[0] + fHeight[1] * fHeightWeight[1]);
            fHeightTop = (fHeight[2] * fHeightWeight[0] + fHeight[3] * fHeightWeight[1]);

            // Set the new value in the destination heightmap
            *pData++ = (t_hmap)(fHeightBottom * fHeightWeight[2] + fHeightTop * fHeightWeight[3]);
        }
    }


    // Only if requested resolution, higher then current resolution.
    if (resolution > m_iWidth)
    {
        if (bNoise)
        {
            InitNoise();

            int ye = hmap.GetHeight();
            int xe = hmap.GetWidth();

            t_hmap* pData = pDataStart;

            // add noise
            for (int y = 0; y < ye; y++)
            {
                for (int x = 0; x < xe; x++)
                {
                    *pData++ += cry_random(0.0f, 1.0f / 16.0f);
                }
            }
        }
    }

    if (bSmooth)
    {
        Smooth(hmap, srcRect.translated(-vTexOffset));
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CHeightmap::Reset(int resolution, int unitSize)
{
    ClearModSectors();

    bool bHasOceanFeature = false;
    AZ::OceanFeatureToggleBus::BroadcastResult(bHasOceanFeature, &AZ::OceanFeatureToggleBus::Events::OceanComponentEnabled);
    if (bHasOceanFeature)
    {
        // by default, there is no ocean
        SetOceanLevel(AZ::OceanConstants::s_HeightUnknown);
    }
    else
    {
        // Legacy: Default ocean level.
        SetOceanLevel(AZ::OceanConstants::s_DefaultHeight);
    }

    Resize(resolution, resolution, unitSize);
    SetMaxHeight(resolution);
}

//////////////////////////////////////////////////////////////////////////
bool CHeightmap::GetPreviewBitmap(DWORD* pBitmapData, int width, bool bSmooth, bool bNoise, QRect* pUpdateRect, bool bShowOcean, bool bUseScaledRange)
{
    if (m_pHeightmap.empty())
    {
        return false;
    }

    QRect bounds(0, 0, width, width);

    CFloatImage hmap;
    hmap.Allocate(width, width);
    t_hmap* pHeightmap = hmap.GetData();

    // If we're using an unscaled greyscale range, we can get away with just updating the updateRect area of the bitmap if one
    // was provided.  However, with a scaling greyscale range, the scale can change on every update so we need to update the
    // entire bitmap to retain consistency, not just the part that changed height values.
    if (pUpdateRect && (!bUseScaledRange))
    {
        QRect destUpdateRect = *pUpdateRect;
        float fScale = (float)width / m_iWidth;
        // resizing the whole rect - use moveTopLeft, not setTopLeft
        // since setTopLeft would resize it
        destUpdateRect.moveTopLeft(destUpdateRect.topLeft() * fScale);
        destUpdateRect.setSize(destUpdateRect.size() * fScale);

        if (!GetData(destUpdateRect, width, QPoint(0, 0), hmap, bSmooth, bNoise))
        {
            return false;
        }

        bounds = bounds.intersected(destUpdateRect);
    }
    else
    {
        if (!GetDataEx(pHeightmap, width, bSmooth, bNoise))
        {
            return false;
        }
    }

    float minHeight = 0.0f;
    float maxHeight = 0.0f;

    // Grab the min and max height values from the heightmap data we got back.  We need to do this because depending on the
    // bSmooth, bNoise, and scaling parameters, the values we're checking here might not match the "real" heightmap data.
    if (bUseScaledRange)
    {
        minHeight = std::numeric_limits<float>::max();
        maxHeight = std::numeric_limits<float>::min();

        for (int iY = bounds.top(); iY < bounds.bottom() + 1; iY++)
        {
            for (int iX = bounds.left(); iX < bounds.right() + 1; iX++)
            {
                minHeight = AZStd::min(minHeight, pHeightmap[iX + iY * width]);
                maxHeight = AZStd::max(maxHeight, pHeightmap[iX + iY * width]);
            }
        }
    }
    else
    {
        maxHeight = m_fMaxHeight;
    }

    // Make sure we have at least an arbitrarily small non-zero height difference so we can safely divide.
    float maxHeightDifference = AZStd::max(0.00001f, (maxHeight - minHeight));

    // Pleasant blue color for the ocean.
    const QColor oceanColor(0x00, 0x99, 0xFF);

    // Alpha blend the ocean color in by 25%
    const float oceanBlendFactor = 0.25f;

    QColor finalColor;

    // Fill the preview with the heightmap image
    for (int iY = bounds.top(); iY < bounds.bottom(); iY++)
    {
        for (int iX = bounds.left(); iX < bounds.right(); iX++)
        {
            float height = pHeightmap[iX + iY * width];

            // Scale our height range to (0, 1) across the minHeight to maxHeight range.
            float greyVal = (height - minHeight) / maxHeightDifference;

            if (bShowOcean && (height <= GetOceanLevel()))
            {
                // If we want to see ocean, blend a blue tint to any height data that's underwater.
                finalColor.setRgbF(
                    (oceanColor.redF()   * oceanBlendFactor) + (greyVal * (1.0f - oceanBlendFactor)),
                    (oceanColor.greenF() * oceanBlendFactor) + (greyVal * (1.0f - oceanBlendFactor)),
                    (oceanColor.blueF()  * oceanBlendFactor) + (greyVal * (1.0f - oceanBlendFactor))
                );
            }
            else
            {
                finalColor.setRgbF(greyVal, greyVal, greyVal);
            }

            // Use a texel from the tiled water texture when it is below the ocean level
            pBitmapData[iX + iY * width] = RGB(finalColor.red(), finalColor.green(), finalColor.blue());
        }
    }

    return true;
}

void CHeightmap::GenerateTerrain(const SNoiseParams& noiseParam)
{
    ////////////////////////////////////////////////////////////////////////
    // Generate a new terrain with the parameters stored in sParam
    ////////////////////////////////////////////////////////////////////////

    CDynamicArray2D cHeightmap(GetWidth(), GetHeight());
    CNoise cNoise;
    float fYScale = 255.0f;
    SNoiseParams sParam = noiseParam;

    assert(sParam.iWidth == m_iWidth && sParam.iHeight == m_iHeight);

    //////////////////////////////////////////////////////////////////////
    // Generate the noise array
    //////////////////////////////////////////////////////////////////////

    if (!m_standaloneMode)
    {
        qApp->setOverrideCursor(Qt::WaitCursor);
    }

    CLogFile::WriteLine("Noise...");

    // Set the random value
    srand(sParam.iRandom);

    // Process layers
    for (unsigned int i = 0; i < sParam.iPasses; i++)
    {
        // Apply the fractal noise function to the array
        cNoise.FracSynthPass(&cHeightmap, sParam.fFrequency, fYScale,
            sParam.iWidth, sParam.iHeight, FALSE);

        // Modify noise generation parameters
        sParam.fFrequency *= sParam.fFrequencyStep;
        if (sParam.fFrequency > 16000.f)
        {
            sParam.fFrequency = 16000.f;
        }
        fYScale *= sParam.fFade;
    }

    //////////////////////////////////////////////////////////////////////
    // Store the generated terrain in the heightmap
    //////////////////////////////////////////////////////////////////////

    for (unsigned int j = 0; j < m_iHeight; j++)
    {
        for (unsigned int i = 0; i < m_iWidth; i++)
        {
            SetXY(i, j, MAX(MIN(cHeightmap.m_Array[i][j], 512000.0f), -512000.0f));
        }
    }

    //////////////////////////////////////////////////////////////////////
    // Perform some modifications on the heightmap
    //////////////////////////////////////////////////////////////////////

    // Smooth the heightmap and normalize it
    for (unsigned int i = 0; i < sParam.iSmoothness; i++)
    {
        Smooth();
    }

    Normalize();
    MakeIsle();

    //////////////////////////////////////////////////////////////////////
    // Finished
    //////////////////////////////////////////////////////////////////////

    NotifyModified();

    if (!m_standaloneMode)
    {
        qApp->restoreOverrideCursor();
    }
}


float CHeightmap::CalcHeightScale() const
{
    return 1.0f / (float)(GetWidth() * GetUnitSize());
}

void CHeightmap::SmoothSlope()
{
    //////////////////////////////////////////////////////////////////////
    // Remove areas with high slope from the heightmap
    //////////////////////////////////////////////////////////////////////

    if (!IsAllocated())
    {
        return;
    }


    CLogFile::WriteLine("Smoothing the slope of the heightmap...");

    // Remove the high slope areas (horizontal)
    for (int ypos = 1; ypos < m_iHeight - 1; ypos++)
    {
        for (int xpos = 1; xpos < m_iWidth - 1; ++xpos)
        {
            Smooth3x3(m_pHeightmap, m_iWidth, xpos, ypos, m_fMaxHeight);
        }
    }

    // Remove the high slope areas (vertical)
    for (int xpos = 1; xpos < m_iWidth - 1; xpos++)
    {
        for (int ypos = 1; ypos < m_iHeight - 1; ypos++)
        {
            Smooth3x3(m_pHeightmap, m_iWidth, xpos, ypos, m_fMaxHeight);
        }
    }

    NotifyModified();
}

void CHeightmap::MakeIsle()
{
    //////////////////////////////////////////////////////////////////////
    // Convert any terrain into an isle
    //////////////////////////////////////////////////////////////////////

    int i, j;

    auto pHeightmapData = m_pHeightmap.begin();
    float fDeltaX, fDeltaY;
    float fDistance;
    float fCurHeight, fFade;

    CLogFile::WriteLine("Modifying heightmap to an isle...");

    // Calculate the length of the diagonal through the heightmap
    float fMaxDistance = sqrtf((GetWidth() / 2) * (GetWidth() / 2) + (GetHeight() / 2) * (GetHeight() / 2));

    for (j = 0; j < m_iHeight; j++)
    {
        // Calculate the distance delta
        fDeltaY = (float)abs((int)(j - m_iHeight / 2));

        for (i = 0; i < m_iWidth; i++)
        {
            // Calculate the distance delta
            fDeltaX = (float)abs((int)(i - m_iWidth / 2));

            // Calculate the distance
            fDistance = (float)sqrt(fDeltaX * fDeltaX + fDeltaY * fDeltaY);

            // Calculate the fade-off
            float fCosX = sinf((float)i / (float)m_iWidth * 3.1416f);
            float fCosY = sinf((float)j / (float)m_iHeight * 3.1416f);
            fFade = fCosX * fCosY;
            fFade = 1.0 - ((1.0f - fFade) * (1.0f - fFade));

            // Modify the value
            fCurHeight = *pHeightmapData;
            fCurHeight *= fFade;

            // Clamp
            ClampHeight(fCurHeight, m_fMaxHeight);

            // Write the value back and advance
            *pHeightmapData++ = fCurHeight;
        }
    }

    NotifyModified();
}

void CHeightmap::Flatten(float fFactor)
{
    ////////////////////////////////////////////////////////////////////////
    // Increase the number of flat areas on the heightmap (TODO: Fix !)
    ////////////////////////////////////////////////////////////////////////

    auto pHeightmapData = m_pHeightmap.begin();
    auto pHeightmapDataEnd = m_pHeightmap.end();
    float fRes;

    CLogFile::WriteLine("Flattening heightmap...");

    // Perform the conversion
    while (pHeightmapData != pHeightmapDataEnd)
    {
        // Get the exponential value for this height value
        fRes = ExpCurve(*pHeightmapData, 128.0f, 0.985f);

        // Is this part of the landscape a potential flat area ?
        // Only modify parts of the landscape that are above the ocean level
        if (fRes < 100 && *pHeightmapData > GetOceanLevel())
        {
            // Yes, apply the factor to it
            *pHeightmapData = *pHeightmapData * fFactor;

            // When we use a factor greater than 0.5, we don't want to drop below
            // the ocean level
            *pHeightmapData++ = __max(GetOceanLevel(), *pHeightmapData);
        }
        else
        {
            // No, use the exponential function to make smooth transitions
            *pHeightmapData++ = fRes;
        }
    }

    NotifyModified();
}

float CHeightmap::ExpCurve(float v, float fCover, float fSharpness)
{
    //////////////////////////////////////////////////////////////////////
    // Exponential function
    //////////////////////////////////////////////////////////////////////

    float c;

    c = v - fCover;

    if (c < 0)
    {
        c = 0;
    }

    return m_fMaxHeight - (float)((pow(fSharpness, c)) * m_fMaxHeight);
}

void CHeightmap::LowerRange(float fFactor)
{
    //////////////////////////////////////////////////////////////////////
    // Lower the value range of the heightmap, effectively making it
    // more flat
    //////////////////////////////////////////////////////////////////////

    unsigned int i;
    float fOceanHeight = GetOceanLevel();

    CLogFile::WriteLine("Lowering range...");

    // Lower the range, make sure we don't put anything below the ocean level
    for (i = 0; i < m_iWidth * m_iHeight; i++)
    {
        m_pHeightmap[i] = ((m_pHeightmap[i] - fOceanHeight) * fFactor) + fOceanHeight;
    }

    NotifyModified();
}

void CHeightmap::Randomize()
{
    ////////////////////////////////////////////////////////////////////////
    // Add a small amount of random noise
    ////////////////////////////////////////////////////////////////////////

    unsigned int i;

    CLogFile::WriteLine("Lowering range...");

    // Add the noise
    for (i = 0; i < m_iWidth * m_iHeight; i++)
    {
        m_pHeightmap[i] += cry_random(-4.0f, 4.0f);
    }

    // Normalize because we might have valid the valid range
    Normalize();

    NotifyModified();
}

void CHeightmap::DrawSpot(unsigned long iX, unsigned long iY,
    uint8 iWidth, float fAddVal,
    float fSetToHeight, bool bAddNoise)
{
    ////////////////////////////////////////////////////////////////////////
    // Draw an attenuated spot on the map
    ////////////////////////////////////////////////////////////////////////
    long i, j;
    long iPosX, iPosY, iIndex;
    float fMaxDist, fAttenuation, fJSquared;
    float fCurHeight;

    InitNoise();
    assert(m_pNoise);

    // Calculate the maximum distance
    fMaxDist = sqrtf((float)((iWidth / 2) * (iWidth / 2) + (iWidth / 2) * (iWidth / 2)));

    RecordUndo(iX - iWidth, iY - iWidth, iWidth * 2, iWidth * 2);

    for (j = (long)-iWidth; j < iWidth; j++)
    {
        // Precalculate
        iPosY = iY + j;
        fJSquared = (float)(j * j);

        for (i = (long)-iWidth; i < iWidth; i++)
        {
            // Calculate the position
            iPosX = iX + i;

            // Skip invalid locations
            if (iPosX < 0 || iPosY < 0 ||
                iPosX > (long) m_iWidth - 1 || iPosY > (long) m_iHeight - 1)
            {
                continue;
            }

            // Calculate the array index
            iIndex = iPosX + iPosY * m_iWidth;

            // Calculate attenuation factor
            fAttenuation = 1.0f - __min(1.0f, sqrtf((float)(i * i + fJSquared)) / fMaxDist);

            // Which drawing mode are we in ?
            if (fSetToHeight >= 0.0f)
            {
                // Set to height mode, modify the location towards the specified height
                fCurHeight = m_pHeightmap[iIndex];
                m_pHeightmap[iIndex] *= 4.0f;
                m_pHeightmap[iIndex] += (1.0f - fAttenuation) * fCurHeight + fAttenuation * fSetToHeight;
                m_pHeightmap[iIndex] /= 5.0f;
            }
            else if (bAddNoise)
            {
                // Noise brush
                if (fAddVal > 0.0f)
                {
                    m_pHeightmap[iIndex] += fAddVal / 100 *
                        (fabs(GetNoise(iPosX, iPosY)))
                        * fAttenuation;
                }
                else
                {
                    m_pHeightmap[iIndex] += fAddVal / 100 *
                        (-fabs(GetNoise(iPosX, iPosY)))
                        * fAttenuation;
                }
            }
            else
            {
                // No, modify the location with a normal brush
                m_pHeightmap[iIndex] += fAddVal * fAttenuation;
            }

            // Clamp
            ClampHeight(m_pHeightmap[iIndex], m_fMaxHeight);
        }
    }

    NotifyModified(iX - iWidth, iY - iWidth, iX + iWidth, iY + iWidth);
}

void CHeightmap::DrawSpot2(int iX, int iY, int radius, float insideRadius, float fHeight, float fHardness, bool bAddNoise, float noiseFreq, float noiseScale)
{
    ////////////////////////////////////////////////////////////////////////
    // Draw an attenuated spot on the map
    ////////////////////////////////////////////////////////////////////////
    int i, j;
    int iPosX, iPosY, iIndex;
    float fMaxDist, fAttenuation, fYSquared;
    float fCurHeight;

    if (bAddNoise)
    {
        InitNoise();
    }

    RecordUndo(iX - radius, iY - radius, radius * 2, radius * 2);

    // Calculate the maximum distance
    fMaxDist = radius;

    for (j = (long)-radius; j < radius; j++)
    {
        // Precalculate
        iPosY = iY + j;
        fYSquared = (float)(j * j);

        for (i = (long)-radius; i < radius; i++)
        {
            // Calculate the position
            iPosX = iX + i;

            // Skip invalid locations
            if (iPosX < 0 || iPosY < 0 || iPosX > m_iWidth - 1 || iPosY > m_iHeight - 1)
            {
                continue;
            }

            // Only circle.
            float dist = sqrtf(fYSquared + i * i);
            if (dist > fMaxDist)
            {
                continue;
            }

            // Calculate the array index
            iIndex = iPosX + iPosY * m_iWidth;

            // Calculate attenuation factor
            if (dist <= insideRadius)
            {
                fAttenuation = 1.0f;
            }
            else
            {
                fAttenuation = 1.0f - __min(1.0f, (dist - insideRadius) / fMaxDist);
            }

            // Set to height mode, modify the location towards the specified height
            fCurHeight = m_pHeightmap[iIndex];
            float dh = fHeight - fCurHeight;

            float h = fCurHeight + (fAttenuation) * dh * fHardness;


            if (bAddNoise)
            {
                float noise = GetNoise(ftoi(iPosX * noiseFreq), ftoi(iPosY * noiseFreq));
                // No height contribution when using noise, but hardness contributes to noiseScale
                h = fCurHeight + (float)(noise) * fAttenuation * noiseScale * fHardness;
            }

            // Clamp
            ClampHeight(h, m_fMaxHeight);
            m_pHeightmap[iIndex] = h;
        }
    }

    // We modified the heightmap.
    NotifyModified(iX - radius, iY - radius, iX + radius, iY + radius);
}

void CHeightmap::RiseLowerSpot(int iX, int iY, int radius, float insideRadius, float fHeight, float fHardness, bool bAddNoise, float noiseFreq, float noiseScale)
{
    ////////////////////////////////////////////////////////////////////////
    // Draw an attenuated spot on the map
    ////////////////////////////////////////////////////////////////////////
    int i, j;
    int iPosX, iPosY, iIndex;
    float fMaxDist, fAttenuation, fYSquared;
    float fCurHeight;

    if (bAddNoise)
    {
        InitNoise();
    }

    RecordUndo(iX - radius, iY - radius, radius * 2, radius * 2);

    // Calculate the maximum distance
    fMaxDist = radius;

    for (j = (long)-radius; j < radius; j++)
    {
        // Precalculate
        iPosY = iY + j;
        fYSquared = (float)(j * j);

        for (i = (long)-radius; i < radius; i++)
        {
            // Calculate the position
            iPosX = iX + i;

            // Skip invalid locations
            if (iPosX < 0 || iPosY < 0 || iPosX > m_iWidth - 1 || iPosY > m_iHeight - 1)
            {
                continue;
            }

            // Only circle.
            float dist = sqrtf(fYSquared + i * i);
            if (dist > fMaxDist)
            {
                continue;
            }

            // Calculate the array index
            iIndex = iPosX + iPosY * m_iWidth;

            // Calculate attenuation factor
            if (dist <= insideRadius)
            {
                fAttenuation = 1.0f;
            }
            else
            {
                fAttenuation = 1.0f - __min(1.0f, (dist - insideRadius) / fMaxDist);
            }

            // Set to height mode, modify the location towards the specified height
            fCurHeight = m_pHeightmap[iIndex];
            float dh = fHeight;

            float h = fCurHeight + (fAttenuation) * dh * fHardness;

            if (bAddNoise)
            {
                float noise = GetNoise(ftoi(iPosX * noiseFreq), ftoi(iPosY * noiseFreq));
                // No height contribution when using noise, but hardness contributes to noiseScale
                h = fCurHeight + (float)(noise) * fAttenuation * noiseScale * fHardness;
            }

            // Clamp
            ClampHeight(h, m_fMaxHeight);
            m_pHeightmap[iIndex] = h;
        }
    }

    // We modified the heightmap.
    NotifyModified(iX - radius, iY - radius, iX + radius, iY + radius);
}

void CHeightmap::SmoothSpot(int iX, int iY, int radius, float fHeight, float fHardness)
{
    ////////////////////////////////////////////////////////////////////////
    // Draw an attenuated spot on the map
    ////////////////////////////////////////////////////////////////////////
    int i, j;
    int iPosX, iPosY;
    float fMaxDist, fYSquared;

    RecordUndo(iX - radius, iY - radius, radius * 2, radius * 2);

    // Calculate the maximum distance
    fMaxDist = radius;

    for (j = (long)-radius; j < radius; j++)
    {
        // Precalculate
        iPosY = iY + j;
        fYSquared = (float)(j * j);

        // Skip invalid locations
        if (iPosY < 1 || iPosY > m_iHeight - 2)
        {
            continue;
        }

        for (i = (long)-radius; i < radius; i++)
        {
            // Calculate the position
            iPosX = iX + i;

            // Skip invalid locations
            if (iPosX < 1 || iPosX > m_iWidth - 2)
            {
                continue;
            }

            // Only circle.
            float dist = sqrtf(fYSquared + i * i);
            if (dist > fMaxDist)
            {
                continue;
            }

            int pos = iPosX + iPosY * m_iWidth;
            float h;
            h = (m_pHeightmap[pos] +
                 m_pHeightmap[pos + 1] +
                 m_pHeightmap[pos - 1] +
                 m_pHeightmap[pos + m_iWidth] +
                 m_pHeightmap[pos - m_iWidth] +
                 m_pHeightmap[pos + 1 + m_iWidth] +
                 m_pHeightmap[pos + 1 - m_iWidth] +
                 m_pHeightmap[pos - 1 + m_iWidth] +
                 m_pHeightmap[pos - 1 - m_iWidth])
                / 9.0f;

            float currH = m_pHeightmap[pos];
            m_pHeightmap[pos] = currH + (h - currH) * fHardness;
        }
    }

    // We modified the heightmap.
    NotifyModified(iX - radius, iY - radius, iX + radius, iY + radius);
}

void CHeightmap::Hold()
{
    if (m_standaloneMode)
    {
        // No "Hold"/"Fetch" in Standalone mode -- use the copy ctor to copy the original!
        return;
    }

    ////////////////////////////////////////////////////////////////////////
    // Save a backup copy of the heightmap
    ////////////////////////////////////////////////////////////////////////

    if (!IsAllocated())
    {
        return;
    }

    FILE* hFile = nullptr;

    CLogFile::WriteLine("Saving temporary copy of the heightmap");

    qApp->setOverrideCursor(Qt::WaitCursor);

    // Open the hold / fetch file
    hFile = fopen(HEIGHTMAP_HOLD_FETCH_FILE, "wb");
    assert(hFile);
    if (hFile)
    {
        // Write the dimensions
        VERIFY(fwrite(&m_iWidth, sizeof(m_iWidth), 1, hFile));
        VERIFY(fwrite(&m_iHeight, sizeof(m_iHeight), 1, hFile));

        // Write the data
        VERIFY(fwrite(m_pHeightmap.data(), sizeof(t_hmap), m_iWidth * m_iHeight, hFile));

        //! Write the info.
        VERIFY(fwrite(m_Weightmap.GetData(), sizeof(LayerWeight), m_Weightmap.GetSize(), hFile));

        fclose(hFile);
    }

    qApp->restoreOverrideCursor();
}

void CHeightmap::Fetch()
{
    if (m_standaloneMode)
    {
        // No "Hold"/"Fetch" in Standalone mode -- use the copy ctor to copy the original!
        return;
    }

    ////////////////////////////////////////////////////////////////////////
    // Read a backup copy of the heightmap
    ////////////////////////////////////////////////////////////////////////

    CLogFile::WriteLine("Loading temporary copy of the heightmap");

    qApp->setOverrideCursor(Qt::WaitCursor);

    if (!Read(HEIGHTMAP_HOLD_FETCH_FILE))
    {
        qApp->restoreOverrideCursor();
        QMessageBox::critical(AzToolsFramework::GetActiveWindow(), QString(), QObject::tr("You need to use 'Hold' before 'Fetch' !"));
        return;
    }

    qApp->restoreOverrideCursor();
    NotifyModified();
}

bool CHeightmap::Read(QString strFileName)
{
    ////////////////////////////////////////////////////////////////////////
    // Load a heightmap from a file
    ////////////////////////////////////////////////////////////////////////

    FILE* hFile = nullptr;
    uint64 iWidth, iHeight;

    if (strFileName.isEmpty())
    {
        return false;
    }

    // Open the hold / fetch file
    hFile = fopen(strFileName.toUtf8().data(), "rb");

    if (!hFile)
    {
        return false;
    }

    // Read the dimensions
    VERIFY(fread(&iWidth, sizeof(iWidth), 1, hFile));
    VERIFY(fread(&iHeight, sizeof(iHeight), 1, hFile));

    // Resize the heightmap
    Resize(iWidth, iHeight, m_unitSize, true, true);

    // Load the data
    VERIFY(fread(m_pHeightmap.data(), sizeof(t_hmap), m_iWidth * m_iHeight, hFile));

    //! Write the info.
    m_Weightmap.Allocate(m_iWidth, m_iHeight);
    VERIFY(fread(m_Weightmap.GetData(), sizeof(LayerWeight), m_Weightmap.GetSize(), hFile));

    fclose(hFile);

    return true;
}

void CHeightmap::InitNoise() const
{
    ////////////////////////////////////////////////////////////////////////
    // Initialize the noise array
    ////////////////////////////////////////////////////////////////////////
    if (m_pNoise)
    {
        return;
    }


    CNoise cNoise;
    static bool bFirstQuery = true;
    float fFrequency = 6.0f;
    float fFrequencyStep = 2.0f;
    float fYScale = 1.0f;
    float fFade = 0.46f;
    float fLowestPoint = 256000.0f, fHighestPoint = -256000.0f;
    float fValueRange;
    unsigned int i, j;

    // Allocate a new array class to
    m_pNoise = std::make_unique<CDynamicArray2D>(NOISE_ARRAY_SIZE, NOISE_ARRAY_SIZE);

    // Process layers
    for (i = 0; i < 8; i++)
    {
        // Apply the fractal noise function to the array
        cNoise.FracSynthPass(m_pNoise.get(), fFrequency, fYScale, NOISE_ARRAY_SIZE, NOISE_ARRAY_SIZE, TRUE);

        // Modify noise generation parameters
        fFrequency *= fFrequencyStep;
        fYScale *= fFade;
    }

    // Find the value range
    for (i = 0; i < NOISE_ARRAY_SIZE; i++)
    {
        for (j = 0; j < NOISE_ARRAY_SIZE; j++)
        {
            fLowestPoint = __min(fLowestPoint, m_pNoise->m_Array[i][j]);
            fHighestPoint = __max(fHighestPoint, m_pNoise->m_Array[i][j]);
        }
    }

    // Storing the value range in this way saves us a division and a multiplication
    fValueRange = NOISE_RANGE / (fHighestPoint - fLowestPoint);

    // Normalize the heightmap
    for (i = 0; i < NOISE_ARRAY_SIZE; i++)
    {
        for (j = 0; j < NOISE_ARRAY_SIZE; j++)
        {
            m_pNoise->m_Array[i][j] -= fLowestPoint;
            m_pNoise->m_Array[i][j] *= fValueRange;
        }
    }
}

void CHeightmap::UpdateEngineTerrain(bool bOnlyElevation, bool boUpdateReloadSurfacertypes)
{
    if (m_standaloneMode)
    {
        // TODO: Implement for standalone mode if needed
        return;
    }

    LOADING_TIME_PROFILE_SECTION(gEnv->pSystem);

    if (boUpdateReloadSurfacertypes)
    {
        // ReloadSurfaceTypes calls into _this_ function if bUpdateHeightmap is true. Pass false to prevent a 2nd unnecessary call to UpdateEngineTerrain
        GetIEditor()->GetTerrainManager()->ReloadSurfaceTypes(true /*bUpdateEngineTerrain*/, false /*bUpdateHeightmap*/);
    }

    UpdateEngineTerrain(0, 0, m_iWidth, m_iHeight, true, !bOnlyElevation);
}

//////////////////////////////////////////////////////////////////////////
void CHeightmap::UpdateEngineTerrain(int left, int bottom, int areaSize, int _height, bool bElevation, bool bInfoBits)
{
    if (m_standaloneMode)
    {
        // TODO: Implement for standalone mode if needed
        return;
    }

    LOADING_TIME_PROFILE_SECTION(gEnv->pSystem);

    I3DEngine* p3DEngine = GetIEditor()->Get3DEngine();

    const int nHeightMapUnitSize = p3DEngine->GetHeightMapUnitSize();

    // update terrain by square blocks aligned to terrain sector size

    int nSecSize = p3DEngine->GetTerrainSectorSize() / p3DEngine->GetHeightMapUnitSize();
    if (nSecSize == 0)
    {
        return;
    }

    if (areaSize <= 0)
    {
        return;
    }

    const int HeightmapSize = static_cast<int>(m_iWidth);

    const int originalInputAreaSize = areaSize;
    const int originalInputX1 = left;
    const int originalInputY1 = bottom;

    // [0, M] input maps to [1, N] sectors. Append one sector at the end.
    const int SectorSpan = (areaSize + nSecSize - 1) / nSecSize;
    areaSize = SectorSpan * nSecSize + nSecSize;
    areaSize = CLAMP(areaSize, 0, HeightmapSize);

    left = (left / nSecSize) * nSecSize;
    bottom = (bottom / nSecSize) * nSecSize;
    left = CLAMP(left, 0, (HeightmapSize - areaSize));
    bottom = CLAMP(bottom, 0, (HeightmapSize - areaSize));

    //
    // Some important notes:
    //
    // 1) The editor-side weightmap is in LayerId's, which are a single level of indirection away from the engine ids.
    //    Additionally, editor weight data encodes holes slightly differently. Editor data masks holes in but retains the
    //    weight information. The engine data just has a simple hole value in the first ID slot.
    //
    // 2) This format conversion requires allocating a temporary map covering the update area. This is why we can't just pass
    //    in the weightmap directly.
    //
    // 3) There is a SINGLE UNIT BORDER on the weightmap! This is because every tile of terrain shares one unit at the edge.
    //
    const int WeightmapSize = areaSize + 1;

    TImage<ITerrain::SurfaceWeight> image;
    image.Allocate(WeightmapSize, WeightmapSize);
    ITerrain::SurfaceWeight* surfaceWeights = image.GetData();

    {
        uint8 LayerIdToDetailId[256];
        CTerrainManager& terrainManager = *GetIEditor()->GetTerrainManager();
        for (uint32 dwI = 0; dwI < CLayer::e_hole; ++dwI)
        {
            LayerIdToDetailId[dwI] = terrainManager.GetDetailIdLayerFromLayerId(dwI);
        }

        const int right = left + WeightmapSize;
        const int top = bottom + WeightmapSize;

        for (int y = bottom; y < top; y++)
        {
            int clamped_y = std::min(y, HeightmapSize - 1);

            for (int x = left; x < right; x++)
            {
                int clamped_x = std::min(x, HeightmapSize - 1);

                int localIndex = (clamped_y - bottom) * WeightmapSize + (clamped_x - left);
                ITerrain::SurfaceWeight& surfaceWeight = surfaceWeights[localIndex];

                const LayerWeight& layerWeight = m_Weightmap.ValueAt(clamped_x, clamped_y);
                if (layerWeight.Ids[0] & CLayer::e_hole)
                {
                    surfaceWeight = ITerrain::SurfaceWeight();
                    surfaceWeight.Ids[0] = surfaceWeight.Hole;
                    surfaceWeight.Weights[0] = 255;
                }
                else
                {
                    surfaceWeight = layerWeight;
                    for (int i = 0; i < surfaceWeight.WeightCount; ++i)
                    {
                        surfaceWeight.Ids[i] = LayerIdToDetailId[layerWeight.Ids[i] & CLayer::e_undefined];
                    }
                }
            }
        }
    }

    if (bElevation || bInfoBits)
    {
        // TODO (bethelz): Take note that we swap bottom / left. The editor height data is transposed. Fix this.
        p3DEngine->GetITerrain()->SetTerrainElevation(bottom, left, areaSize, m_pHeightmap.data(), WeightmapSize, surfaceWeights);
    }

    const Vec2 worldModPosition(
        originalInputY1* nHeightMapUnitSize + originalInputAreaSize* nHeightMapUnitSize / 2,
        originalInputX1* nHeightMapUnitSize + originalInputAreaSize* nHeightMapUnitSize / 2);
    const float areaRadius = originalInputAreaSize * nHeightMapUnitSize / 2;

    GetIEditor()->GetGameEngine()->OnTerrainModified(worldModPosition, areaRadius, (originalInputAreaSize == m_iWidth));
}


void CHeightmap::Serialize(CXmlArchive& xmlAr)
{
    if (m_standaloneMode)
    {
        // TODO: Implement for standalone mode if needed
        return;
    }

    if (xmlAr.bLoading)
    {
        // Loading
        XmlNodeRef heightmap = xmlAr.root;

        if (_stricmp(heightmap->getTag(), "Heightmap"))
        {
            heightmap = xmlAr.root->findChild("Heightmap"); // load old version
            if (!heightmap)
            {
                return;
            }
        }

        uint32  nWidth(m_iWidth);
        uint32  nHeight(m_iHeight);

        // To remain compatible.
        if (heightmap->getAttr("Width", nWidth))
        {
            m_iWidth = nWidth;
        }

        // To remain compatible.
        if (heightmap->getAttr("Height", nHeight))
        {
            m_iHeight = nHeight;
        }

        if (OceanToggle::IsActive())
        {
            m_fOceanLevel = OceanRequest::GetOceanLevel();
        }
        else
        {
            heightmap->getAttr("WaterLevel", m_fOceanLevel);
        }
        heightmap->getAttr("UnitSize", m_unitSize);
        heightmap->getAttr("MaxHeight", m_fMaxHeight);

        int textureSize;
        if (heightmap->getAttr("TextureSize", textureSize))
        {
            SetSurfaceTextureSize(textureSize, textureSize);
        }

        void* pData;
        int size1, size2;

        ClearModSectors();

        if (xmlAr.pNamedData->GetDataBlock("HeightmapModSectors", pData, size1))
        {
            int nSize = size1 / (sizeof(int) * 2);
            int* data = (int*)pData;
            for (int i = 0; i < nSize; i++)
            {
                AddModSector(data[i * 2], data[i * 2 + 1]);
            }
            m_updateModSectors = true;
        }

        // Allocate new memory
        Resize(m_iWidth, m_iHeight, m_unitSize);

        // Load heightmap data.
        if (xmlAr.pNamedData->GetDataBlock("HeightmapDataW", pData, size1))
        {
            const int dataSize = m_iWidth * m_iHeight * sizeof(uint16);
            if (size1 != dataSize)
            {
                CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "ERROR: Unexpected size of HeightmapDataW: %d (expected %d)", size1, dataSize);
            }
            else
            {
                float fInvPrecision = 1.0f / GetShortPrecisionScale();
                uint16* pSrc = (uint16*)pData;
                for (int i = 0; i < m_iWidth * m_iHeight; i++)
                {
                    m_pHeightmap[i] = (float)pSrc[i] * fInvPrecision;
                }
            }
        }

        if (xmlAr.pNamedData->GetDataBlock("WeightmapData", pData, size2))
        {
            const int dataSize = m_Weightmap.GetSize();
            // new version
            if (size2 != dataSize)
            {
                CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "ERROR: Unexpected size of Weightmap: %d (expected %d)", size2, dataSize);
            }
            else
            {
                memcpy(m_Weightmap.GetData(), pData, dataSize);
            }
        }

        // Upgrade old format.
        else if (xmlAr.pNamedData->GetDataBlock("HeightmapLayerIdBitmap_ver2", pData, size2))
        {
            CByteImage layerIdBitmap;
            layerIdBitmap.Allocate(m_iWidth, m_iHeight);

            const int dataSize = layerIdBitmap.GetSize();
            // new version
            if (size2 != dataSize)
            {
                CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "ERROR: Unexpected size of HeightmapLayerIdBitmap_ver2: %d (expected %d)", size2, dataSize);
            }
            else
            {
                memcpy(layerIdBitmap.GetData(), pData, dataSize);

                for (int y = 0; y < m_iHeight; ++y)
                {
                    for (int x = 0; x < m_iWidth; ++x)
                    {
                        m_Weightmap.ValueAt(x, y) = LayerWeight(layerIdBitmap.ValueAt(x, y));
                    }
                }
            }
        }
    }
    else
    {
        // Storing
        xmlAr.root = XmlHelpers::CreateXmlNode("Heightmap");
        XmlNodeRef heightmap = xmlAr.root;

        heightmap->setAttr("Width", (uint32)m_iWidth);
        heightmap->setAttr("Height", (uint32)m_iHeight);
        heightmap->setAttr("WaterLevel", GetOceanLevel());
        heightmap->setAttr("UnitSize", m_unitSize);
        heightmap->setAttr("TextureSize", m_textureSize);
        heightmap->setAttr("MaxHeight", m_fMaxHeight);

        if (m_modSectors.size())
        {
            int* data = new int[m_modSectors.size() * 2];
            // Switching mod sectors to set for efficiency
            int i = 0;
            for (std::set<std::pair<int, int> >::iterator it = m_modSectors.begin(); it != m_modSectors.end(); ++it)
            {
                data[i * 2] = std::get<0>(*it);
                data[i * 2 + 1] = std::get<1>(*it);
                ++i;
            }
            xmlAr.pNamedData->AddDataBlock("HeightmapModSectors", data, sizeof(int) * m_modSectors.size() * 2);
            delete[] data;
        }

        // Save heightmap data as words.
        {
            CWordImage hdata;
            hdata.Allocate(m_iWidth, m_iHeight);
            uint16* pTrg = hdata.GetData();
            float fPrecisionScale = GetShortPrecisionScale();
            for (int i = 0; i < m_iWidth * m_iHeight; i++)
            {
                float val = m_pHeightmap[i];

                int h = ftoi(val * fPrecisionScale + 0.5f);
                h = clamp_tpl(h, 0, 0xFFFF);
                pTrg[i] = h;
            }
            xmlAr.pNamedData->AddDataBlock("HeightmapDataW", hdata.GetData(), hdata.GetSize());
        }

        xmlAr.pNamedData->AddDataBlock("WeightmapData", m_Weightmap.GetData(), m_Weightmap.GetSize());
    }
}

void CHeightmap::SerializeTerrain(CXmlArchive& xmlAr)
{
    if (m_standaloneMode)
    {
        // TODO: Implement for standalone mode if needed
        return;
    }

    LOADING_TIME_PROFILE_SECTION(gEnv->pSystem);

    if (xmlAr.bLoading)
    { // Loading
        void* pData = nullptr;
        int nSize = 0;

        if (xmlAr.pNamedData->GetDataBlock("TerrainCompiledData", pData, nSize))
        {
            STerrainChunkHeader* pHeader = (STerrainChunkHeader*)pData;
            if ((pHeader->nVersion == TERRAIN_CHUNK_VERSION) && (pHeader->TerrainInfo.nSectorSize_InMeters == pHeader->TerrainInfo.nUnitSize_InMeters * SECTOR_SIZE_IN_UNITS))
            {
                SSectorInfo si;
                GetSectorsInfo(si);

                ITerrain* pTerrain = GetIEditor()->Get3DEngine()->CreateTerrain(pHeader->TerrainInfo);
                // check if size of terrain in compile data match
                if (pHeader->TerrainInfo.nUnitSize_InMeters)
                {
                    if (!pTerrain->SetCompiledData((uint8*)pData, nSize, nullptr, nullptr))
                    {
                        GetIEditor()->Get3DEngine()->DeleteTerrain();
                    }
                }
            }
        }
    }
    else
    {
        if (ITerrain* pTerrain = GetIEditor()->Get3DEngine()->GetITerrain())
        {
            int nSize = pTerrain->GetCompiledDataSize();
            if (nSize > 0)
            { // Storing
                uint8* pData = new uint8[nSize];
                GetIEditor()->Get3DEngine()->GetITerrain()->GetCompiledData(pData, nSize, nullptr, nullptr, nullptr, GetPlatformEndian());
                xmlAr.pNamedData->AddDataBlock("TerrainCompiledData", pData, nSize, true);
                delete[] pData;
            }
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CHeightmap::SetOceanLevel(float oceanLevel)
{
    if (OceanToggle::IsActive())
    {
        // the Water gem is enabled, so there is no reason to be in this method!
        CLogFile::WriteLine("Deprecated: Please use the Water Gem's Infinite Component.");
        return;
    }

    m_fOceanLevel = oceanLevel;

    if (!m_standaloneMode)
    {
        I3DEngine* i3d = GetIEditor()->GetSystem()->GetI3DEngine();
        if (i3d && i3d->GetITerrain())
        {
            i3d->GetITerrain()->SetOceanWaterLevel(oceanLevel);
        }
    }

    NotifyModified();
}


float CHeightmap::GetOceanLevel() const
{
    return OceanToggle::IsActive() ? OceanRequest::GetOceanLevel() : m_fOceanLevel;
}

void CHeightmap::SetHoleAt(const int x, const int y, const bool bHole)
{
    if (bHole)
    {
        m_Weightmap.ValueAt(x, y).Ids[0] |= CLayer::e_hole;
    }
    else
    {
        m_Weightmap.ValueAt(x, y).Ids[0] &= CLayer::e_undefined;
    }
}

//////////////////////////////////////////////////////////////////////////
// Make hole.
void CHeightmap::MakeHole(int x1, int y1, int width, int height, bool bMake)
{
    RecordUndo(x1, y1, width + 1, height + 1, true);

    I3DEngine* engine = GetIEditor()->Get3DEngine();
    int x2 = x1 + width;
    int y2 = y1 + height;
    for (int x = x1; x <= x2; x++)
    {
        for (int y = y1; y <= y2; y++)
        {
            SetHoleAt(x, y, bMake);
        }
    }

    UpdateEngineTerrain(x1, y1, x2 - x1, y2 - y1, false, true);
}

//////////////////////////////////////////////////////////////////////////
bool CHeightmap::IsHoleAt(const int x, const int y) const
{
    return m_Weightmap.ValueAt(x, y).Ids[0] & CLayer::e_hole;
}


//////////////////////////////////////////////////////////////////////////
void CHeightmap::SetLayerWeightAt(const int x, const int y, const LayerWeight& weight)
{
    m_Weightmap.ValueAt(x, y) = weight;
}

//////////////////////////////////////////////////////////////////////////
void CHeightmap::SetLayerWeights(const AZStd::vector<uint8>& layerIds, const CImageEx* splatMaps, size_t splatMapCount)
{
    AZStd::vector<uint8> layerWeights(splatMapCount);
    for (uint32 y = 0; y < m_iHeight; ++y)
    {
        for (uint32 x = 0; x < m_iWidth; ++x)
        {
            // For each point that we need to fill in, consult all the layers and sample the relevant weight from each mask
            for (size_t layer = 0; layer < splatMapCount; ++layer)
            {
                // Adjust the splat width to account for the '+1' flag in WorldMachine
                auto splatWidth = splatMaps[layer].GetWidth();
                if ((splatWidth & 1) != 0)
                {
                    --splatWidth;
                }
                auto splatHeight = splatMaps[layer].GetHeight();
                if ((splatHeight & 1) != 0)
                {
                    --splatHeight;
                }

                // Set up the scale factor for the lookup. Power of two isn't really relevant here since we are doing float operations
                float widthSpread = aznumeric_caster(splatWidth);
                widthSpread /= m_iWidth;
                float heightSpread = aznumeric_caster(splatHeight);
                heightSpread /= m_iHeight;

                // Now point sample the scaled location in the layer's mask
                int splatX = aznumeric_caster(x * widthSpread);
                int splatY = aznumeric_caster(y * heightSpread);
                layerWeights[layer] = azlossy_caster(splatMaps[layer].ValueAt(splatX, splatY));
            }

            // Now we can build a normalized weight and place it in the heightmap
            LayerWeight weight(layerIds, layerWeights);
            SetLayerWeightAt(x, y, weight);
        }
    }
    UpdateEngineTerrain(false, true);
}

//////////////////////////////////////////////////////////////////////////
LayerWeight CHeightmap::GetLayerWeightAt(const int x, const int y) const
{
    return m_Weightmap.ValueAt(x, y);
}

//////////////////////////////////////////////////////////////////////////
float CHeightmap::GetZInterpolated(const float x, const float y)
{
    if (x <= 0 || y <= 0 || x >= m_iWidth - 1 || y >= m_iHeight - 1)
    {
        return 0;
    }

    int nX = fastftol_positive(x);
    int nY = fastftol_positive(y);

    float dx1 = x - nX;
    float dy1 = y - nY;

    float dDownLandZ0 =
        (1.f - dx1) * (m_pHeightmap[nX + nY * m_iWidth]) +
        (dx1) * (m_pHeightmap[(nX + 1) + nY * m_iWidth]);

    float dDownLandZ1 =
        (1.f - dx1) * (m_pHeightmap[nX + (nY + 1) * m_iWidth]) +
        (dx1) * (m_pHeightmap[(nX + 1) + (nY + 1) * m_iWidth]);

    float dDownLandZ = (1 - dy1) * dDownLandZ0 + (dy1) * dDownLandZ1;

    if (dDownLandZ < 0)
    {
        dDownLandZ = 0;
    }

    return dDownLandZ;
}

//////////////////////////////////////////////////////////////////////////
float CHeightmap::GetAccurateSlope(const float x, const float y)
{
    if (m_standaloneMode)
    {
        // TODO: Implement for standalone mode if needed
        return 0;
    }

    uint32 iHeightmapWidth = GetWidth();

    if (x <= 0 || y <= 0 || x >= m_iWidth - 1 || y >= m_iHeight - 1)
    {
        return 0;
    }

    // Calculate the slope for this point

    float arrEvelvations[8];
    int nId = 0;

    float d = 0.7f;
    arrEvelvations[nId++] = GetZInterpolated(x + 1, y);
    arrEvelvations[nId++] = GetZInterpolated(x - 1, y);
    arrEvelvations[nId++] = GetZInterpolated(x, y + 1);
    arrEvelvations[nId++] = GetZInterpolated(x, y - 1);
    arrEvelvations[nId++] = GetZInterpolated(x + d, y + d);
    arrEvelvations[nId++] = GetZInterpolated(x - d, y - d);
    arrEvelvations[nId++] = GetZInterpolated(x + d, y - d);
    arrEvelvations[nId++] = GetZInterpolated(x - d, y + d);

    float fMin = arrEvelvations[0];
    float fMax = arrEvelvations[0];

    for (int i = 0; i < 8; i++)
    {
        if (arrEvelvations[i] > fMax)
        {
            fMax = arrEvelvations[i];
        }
        if (arrEvelvations[i] < fMin)
        {
            fMin = arrEvelvations[i];
        }
    }

    // Compensate the smaller slope for bigger height fields
    return (fMax - fMin) * 0.5f / GetUnitSize();
}

//////////////////////////////////////////////////////////////////////////
void CHeightmap::UpdateEngineHole(int x1, int y1, int width, int height)
{
    UpdateEngineTerrain(x1, y1, width, height, false, true);
}


//////////////////////////////////////////////////////////////////////////
void CHeightmap::GetSectorsInfo(SSectorInfo& si)
{
    ZeroStruct(si);
    si.unitSize = m_unitSize;
    si.sectorSize = m_unitSize * SECTOR_SIZE_IN_UNITS;
    si.numSectors = m_numSectors;
    if (si.numSectors > 0)
    {
        si.sectorTexSize = m_textureSize / si.numSectors;
    }
    si.surfaceTextureSize = m_textureSize;
}

//////////////////////////////////////////////////////////////////////////
void CHeightmap::SetSurfaceTextureSize(int width, int height)
{
    assert(width == height);

    if (width != 0)
    {
        m_textureSize = width;
    }
    m_terrainGrid->SetResolution(m_textureSize);
}

//////////////////////////////////////////////////////////////////////////
int CHeightmap::LogLayerSizes()
{
    if (m_standaloneMode)
    {
        // TODO: Implement for standalone mode if needed
        return 0;
    }

    int totalSize = 0;
    CCryEditDoc* doc = GetIEditor()->GetDocument();
    int numLayers = GetIEditor()->GetTerrainManager()->GetLayerCount();
    for (int i = 0; i < numLayers; i++)
    {
        CLayer* pLayer = GetIEditor()->GetTerrainManager()->GetLayer(i);
        int layerSize = pLayer->GetSize();
        totalSize += layerSize;
        CLogFile::FormatLine("Layer %s: %dM", pLayer->GetLayerName().toUtf8().data(), layerSize / (1024 * 1024));
    }
    CLogFile::FormatLine("Total Layers Size: %dM", totalSize / (1024 * 1024));
    return totalSize;
}

//////////////////////////////////////////////////////////////////////////
void CHeightmap::ExportBlock(const QRect& inrect, CXmlArchive& xmlAr, bool bIsExportVegetation, std::set<int>* pLayerIds, std::set<int>* pSurfaceIds)
{
    if (m_standaloneMode)
    {
        // TODO: Implement for standalone mode if needed
        return;
    }

    // Storing
    CLogFile::WriteLine("Exporting Heightmap settings...");

    XmlNodeRef heightmap = xmlAr.root->newChild("Heightmap");

    QRect subRc(inrect);

    heightmap->setAttr("Width", (uint32)m_iWidth);
    heightmap->setAttr("Height", (uint32)m_iHeight);

    // Save rectangle dimensions to root of terrain block.
    xmlAr.root->setAttr("X1", subRc.left());
    xmlAr.root->setAttr("Y1", subRc.top());
    xmlAr.root->setAttr("X2", subRc.right() + 1);
    xmlAr.root->setAttr("Y2", subRc.bottom() + 1);

    // Rectangle.
    heightmap->setAttr("X1", subRc.left());
    heightmap->setAttr("Y1", subRc.top());
    heightmap->setAttr("X2", subRc.right() + 1);
    heightmap->setAttr("Y2", subRc.bottom() + 1);

    heightmap->setAttr("UnitSize", m_unitSize);

    CFloatImage hmap;
    CFloatImage hmapSubImage;
    hmap.Attach(m_pHeightmap.data(), m_iWidth, m_iHeight);
    hmapSubImage.Allocate(subRc.width(), subRc.height());

    hmap.GetSubImage(subRc.left(), subRc.top(), subRc.width(), subRc.height(), hmapSubImage);

    Weightmap weightmap;
    weightmap.Allocate(subRc.width(), subRc.height());

    m_Weightmap.GetSubImage(subRc.left(), subRc.top(), subRc.width(), subRc.height(), weightmap);

    // Save heightmap.
    xmlAr.pNamedData->AddDataBlock("HeightmapData", hmapSubImage.GetData(), hmapSubImage.GetSize());
    xmlAr.pNamedData->AddDataBlock("HeightmapLayerIdBitmap", weightmap.GetData(), weightmap.GetSize());


    //return;
    if (bIsExportVegetation)
    {
        Vec3 p1 = HmapToWorld(subRc.topLeft());
        Vec3 p2 = HmapToWorld(subRc.bottomRight() + QPoint(1, 1));
        if (GetIEditor()->GetVegetationMap())
        {
            QRect worldRC(QPoint(p1.x, p1.y), QPoint(p2.x, p2.y) - QPoint(1, 1));
            GetIEditor()->GetVegetationMap()->ExportBlock(worldRC, xmlAr);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
QPoint CHeightmap::ImportBlock(CXmlArchive& xmlAr, const QPoint& newPos, bool bUseNewPos, float heightOffset, bool bOnlyVegetation, ImageRotationDegrees rotation)
{
    if (m_standaloneMode)
    {
        // TODO: Implement for standalone mode if needed
        return QPoint(0, 0);
    }

    CLogFile::WriteLine("Importing Heightmap settings...");

    XmlNodeRef heightmap = xmlAr.root->findChild("Heightmap");
    if (!heightmap)
    {
        return QPoint(0, 0);
    }

    uint32 width, height;

    heightmap->getAttr("Width", width);
    heightmap->getAttr("Height", height);

    QPoint offset(0, 0);

    if (width != m_iWidth || height != m_iHeight)
    {
        QMessageBox::warning(AzToolsFramework::GetActiveWindow(), QObject::tr("Warning"), QObject::tr("Terrain Block dimensions differ from current terrain."));
        return QPoint(0, 0);
    }

    QRect subRc;
    int srcWidth, srcHeight;
    if (rotation == ImageRotationDegrees::Rotate90 || rotation == ImageRotationDegrees::Rotate270)
    {
        int left, top, right, bottom;
        heightmap->getAttr("Y1", left);
        heightmap->getAttr("X1", top);
        heightmap->getAttr("Y2", right);
        heightmap->getAttr("X2", bottom);
        subRc = QRect(QPoint(left, top), QPoint(right, bottom) - QPoint(1, 1));
        srcWidth = subRc.width();
        srcHeight = subRc.height();
    }
    else
    {
        int left, top, right, bottom;
        heightmap->getAttr("X1", left);
        heightmap->getAttr("Y1", top);
        heightmap->getAttr("X2", right);
        heightmap->getAttr("Y2", bottom);
        subRc = QRect(QPoint(left, top), QPoint(right, bottom) - QPoint(1, 1));
        srcWidth = subRc.width();
        srcHeight = subRc.height();
    }

    if (bUseNewPos)
    {
        offset = QPoint(newPos.x() - subRc.left(), newPos.y() - subRc.top());
        subRc.translate(offset);
    }

    if (!bOnlyVegetation)
    {
        void* pData;
        int size;

        // Load heightmap data.
        if (xmlAr.pNamedData->GetDataBlock("HeightmapData", pData, size))
        {
            // Backward compatibility for float heightmap data.
            CFloatImage hmap;
            CFloatImage hmapSubImage;
            hmap.Attach(m_pHeightmap.data(), m_iWidth, m_iHeight);
            hmapSubImage.Attach((float*)pData, srcWidth, srcHeight);

            if (rotation != ImageRotationDegrees::Rotate0)
            {
                CFloatImage hmapSubImageRot;
                hmapSubImageRot.RotateOrt(hmapSubImage, rotation);
                hmap.SetSubImage(subRc.left(), subRc.top(), hmapSubImageRot, heightOffset, m_fMaxHeight);
            }
            else
            {
                hmap.SetSubImage(subRc.left(), subRc.top(), hmapSubImage, heightOffset, m_fMaxHeight);
            }
        }

        if (xmlAr.pNamedData->GetDataBlock("HeightmapLayerIdBitmap", pData, size))
        {
            Weightmap weightmap;
            weightmap.Attach((LayerWeight*)pData, srcWidth, srcHeight);

            if (rotation != ImageRotationDegrees::Rotate0)
            {
                Weightmap weightmapRot;
                weightmapRot.RotateOrt(weightmap, rotation);
                m_Weightmap.SetSubImage(subRc.left(), subRc.top(), weightmapRot);
            }
            else
            {
                m_Weightmap.SetSubImage(subRc.left(), subRc.top(), weightmap);
            }
        }

        // After heightmap serialization, update terrain in 3D Engine.
        int wid = subRc.width() + 2;
        if (wid < subRc.height() + 2)
        {
            wid = subRc.height() + 2;
        }
        UpdateEngineTerrain(subRc.left() - 1, subRc.top() - 1, wid, wid, true, true);
    }

    if (GetIEditor()->GetVegetationMap())
    {
        Vec3 ofs = HmapToWorld(offset);
        GetIEditor()->GetVegetationMap()->ImportBlock(xmlAr, QPoint(ofs.x, ofs.y));
    }

    return offset;
}

//////////////////////////////////////////////////////////////////////////
void CHeightmap::CopyFrom(t_hmap* prevHeightmap, LayerWeight* prevWeightmap, int prevSize)
{
    int x, y;
    int res = min(prevSize, (int)m_iWidth);
    for (y = 0; y < res; y++)
    {
        for (x = 0; x < res; x++)
        {
            SetXY(x, y, prevHeightmap[x + y * prevSize]);
            m_Weightmap.ValueAt(x, y) = prevWeightmap[x + y * prevSize];
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CHeightmap::CopyFromInterpolate(t_hmap* prevHeightmap, LayerWeight* prevWeightmap, int resolution, int prevUnitSize)
{
    float fKof = float(prevUnitSize) / m_unitSize;
    if (fKof > 1)
    {
        int iWidth = m_iWidth;
        int iHeight = m_iHeight;
        float ratio = m_iWidth / (fKof * resolution);
        if (ratio > 1)
        {
            iWidth = iHeight = resolution * prevUnitSize;
        }

        int kof = (int)fKof;
        for (int y = 0, y2 = 0; y < iHeight; y += kof, y2++)
        {
            for (int x = 0, x2 = 0; x < iWidth; x += kof, x2++)
            {
                for (int y1 = 0; y1 < kof; y1++)
                {
                    for (int x1 = 0; x1 < kof; x1++)
                    {
                        if (x + x1 < iWidth && y + y1 < iHeight && x2 < resolution && y2 < resolution)
                        {
                            float kofx = (float)x1 / kof;
                            float kofy = (float)y1 / kof;

                            int x3 = x2 + 1;
                            int y3 = y2 + 1;
                            if (x3 >= resolution)
                            {
                                x3 = x2;
                            }
                            if (y3 >= resolution)
                            {
                                y3 = y2;
                            }

                            SetXY(x + x1, y + y1,
                                (1.0f - kofy) * ((1.0f - kofx) * prevHeightmap[x2 + y2 * resolution] + kofx * prevHeightmap[x3 + y2 * resolution]) +
                                kofy * ((1.0f - kofx) * prevHeightmap[x2 + y3 * resolution] + kofx * prevHeightmap[x3 + y3 * resolution])
                                );

                            m_Weightmap.ValueAt(x + x1, y + y1) = prevWeightmap[x2 + y2 * resolution];
                        }
                    }
                }
                LayerWeight val = prevWeightmap[x2 + y2 * resolution];

                if (y2 < resolution - 1)
                {
                    LayerWeight val1 = prevWeightmap[x2 + (y2 + 1) * resolution];
                    if (val1.PrimaryId() > val.PrimaryId())
                    {
                        m_Weightmap.ValueAt(x, y + kof - 1) = val1;
                    }
                }

                if (x2 < resolution - 1)
                {
                    LayerWeight val1 = prevWeightmap[x2 + 1 + y2 * resolution];
                    if (val1.PrimaryId() > val.PrimaryId())
                    {
                        m_Weightmap.ValueAt(x + kof - 1, y) = val1;
                    }
                }

                if (x2 < resolution - 1 && y2 < resolution - 1)
                {
                    // choose max occured value or max value between several max occured.
                    LayerWeight valu[4];
                    int bal[4];
                    valu[0] = val;
                    valu[1] = prevWeightmap[x2 + 1 + y2 * resolution];
                    valu[2] = prevWeightmap[x2 + (y2 + 1) * resolution];
                    valu[3] = prevWeightmap[x2 + 1 + (y2 + 1) * resolution];

                    int max = 0;

                    int k = 0;
                    for (k = 0; k < 4; k++)
                    {
                        bal[k] = 1000 + valu[k].PrimaryId();
                        if (bal[k] > max)
                        {
                            val = valu[k];
                            max = bal[k];
                        }
                        if (k > 0)
                        {
                            for (int kj = 0; kj < k; kj++)
                            {
                                if (valu[kj].PrimaryId() == valu[k].PrimaryId())
                                {
                                    valu[k] = LayerWeight(0);
                                    bal[kj] += bal[k];
                                    bal[k] = 0;
                                    if (bal[kj] > max)
                                    {
                                        val = valu[kj];
                                        max = bal[kj];
                                    }
                                    break;
                                }
                            }
                        }
                    }

                    m_Weightmap.ValueAt(x + kof - 1, y + kof - 1) = val;
                }
            }
        }
    }
    else if (0.1f < fKof && fKof < 1.0f)
    {
        int kof = int(1.0f / fKof + 0.5f);
        for (int y = 0; y < m_iHeight; y++)
        {
            for (int x = 0; x < m_iWidth; x++)
            {
                int x2 = x * kof;
                int y2 = y * kof;
                if (x2 < resolution && y2 < resolution)
                {
                    SetXY(x, y, prevHeightmap[x2 + y2 * resolution]);
                    m_Weightmap.ValueAt(x, y) = prevWeightmap[x2 + y2 * resolution];
                }
            }
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CHeightmap::InitSectorGrid()
{
    SSectorInfo si;
    GetSectorsInfo(si);
    GetTerrainGrid()->InitSectorGrid(si.numSectors);
}



void CHeightmap::GetMemoryUsage(ICrySizer* pSizer)
{
    pSizer->Add(*this);

    if (m_pHeightmap.capacity() > 0)
    {
        SIZER_COMPONENT_NAME(pSizer, "Heightmap 2D Array");
        pSizer->Add(m_pHeightmap.data(), m_pHeightmap.capacity());
    }

    if (m_pNoise)
    {
        m_pNoise->GetMemoryUsage(pSizer);
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "LayerId 2D Array");
        pSizer->Add((char*)m_Weightmap.GetData(), m_Weightmap.GetSize());
    }

    if (m_terrainGrid)
    {
        m_terrainGrid->GetMemoryUsage(pSizer);
    }
}


t_hmap CHeightmap::GetSafeXY(const uint32 dwX, const uint32 dwY) const
{
    if (dwX <= 0 || dwY <= 0 || dwX >= (uint32)m_iWidth || dwY >= (uint32)m_iHeight)
    {
        return 0;
    }

    return m_pHeightmap[dwX + dwY * m_iWidth];
}

void CHeightmap::RecordUndo(int x1, int y1, int width, int height, bool bInfo)
{
    if (!m_standaloneMode && GetIEditor()->IsUndoRecording())
    {
        if (bInfo)
        {
            GetIEditor()->RecordUndo(new CUndoHeightmapInfo(x1, y1, width, height, this));
        }
        else
        {
            GetIEditor()->RecordUndo(new CUndoHeightmapModify(x1, y1, width, height, this));
        }
    }
}

void CHeightmap::UpdateLayerTexture(const QRect& rect)
{
    I3DEngine* p3DEngine = GetIEditor()->Get3DEngine();

    float x1 = HmapToWorld(rect.topLeft()).x;
    float y1 = HmapToWorld(rect.topLeft()).y;
    float x2 = HmapToWorld((rect.bottomRight() + QPoint(1, 1))).x;
    float y2 = HmapToWorld((rect.bottomRight() + QPoint(1, 1))).y;

    int iTexSectorSize = GetIEditor()->Get3DEngine()->GetTerrainTextureNodeSizeMeters();
    if (iTexSectorSize == 0)
    {
        return;
    }

    int iTerrainSize = GetIEditor()->Get3DEngine()->GetTerrainSize();
    int iTexSectorsNum = iTerrainSize / iTexSectorSize;

    uint32 dwFullResolution = m_TerrainBGRTexture.CalcMaxLocalResolution((float)rect.left() / GetWidth(), (float)rect.top() / GetHeight(), (float)(rect.right() + 1) / GetWidth(), (float)(rect.bottom() + 1) / GetHeight());
    uint32 dwNeededResolution = dwFullResolution / iTexSectorsNum;

    int iSectMinX = (int)floor(y1 / iTexSectorSize);
    int iSectMinY = (int)floor(x1 / iTexSectorSize);
    int iSectMaxX = (int)floor(y2 / iTexSectorSize);
    int iSectMaxY = (int)floor(x2 / iTexSectorSize);

    CWaitProgress progress("Updating Terrain Layers");

    int nTotalSectors = (iSectMaxX - iSectMinX + 1) * (iSectMaxY - iSectMinY + 1);
    int nCurrSectorNum = 0;
    for (int iSectX = iSectMinX; iSectX <= iSectMaxX; iSectX++)
    {
        for (int iSectY = iSectMinY; iSectY <= iSectMaxY; iSectY++)
        {
            progress.Step((100 * nCurrSectorNum) / nTotalSectors);
            nCurrSectorNum++;
            int iLocalOutMinX = y1 * dwFullResolution / iTerrainSize - iSectX * dwNeededResolution;
            int iLocalOutMinY = x1 * dwFullResolution / iTerrainSize - iSectY * dwNeededResolution;
            int iLocalOutMaxX = y2 * dwFullResolution / iTerrainSize - iSectX * dwNeededResolution;
            int iLocalOutMaxY = x2 * dwFullResolution / iTerrainSize - iSectY * dwNeededResolution;

            if (iLocalOutMinX < 0)
            {
                iLocalOutMinX = 0;
            }
            if (iLocalOutMinY < 0)
            {
                iLocalOutMinY = 0;
            }
            if (iLocalOutMaxX > dwNeededResolution)
            {
                iLocalOutMaxX = dwNeededResolution;
            }
            if (iLocalOutMaxY > dwNeededResolution)
            {
                iLocalOutMaxY = dwNeededResolution;
            }

            if (iLocalOutMinX != iLocalOutMaxX && iLocalOutMinY != iLocalOutMaxY)
            {
                bool bFullRefreshRequired = GetTerrainGrid()->GetSector(QPoint(iSectX, iSectY))->textureId == 0;
                bool bRecreated;
                int texId = GetTerrainGrid()->LockSectorTexture(QPoint(iSectX, iSectY), dwFullResolution / iTexSectorsNum, bRecreated);
                if (bFullRefreshRequired || bRecreated)
                {
                    iLocalOutMinX = 0;
                    iLocalOutMinY = 0;
                    iLocalOutMaxX = dwNeededResolution;
                    iLocalOutMaxY = dwNeededResolution;
                }

                CImageEx imageBGR;
                imageBGR.Allocate(iLocalOutMaxX - iLocalOutMinX, iLocalOutMaxY - iLocalOutMinY);

                m_TerrainBGRTexture.GetSubImageStretched(
                    (iSectX + (float)iLocalOutMinX / dwNeededResolution) / iTexSectorsNum,
                    (iSectY + (float)iLocalOutMinY / dwNeededResolution) / iTexSectorsNum,
                    (iSectX + (float)iLocalOutMaxX / dwNeededResolution) / iTexSectorsNum,
                    (iSectY + (float)iLocalOutMaxY / dwNeededResolution) / iTexSectorsNum,
                    imageBGR);

                {
                    uint32 dwWidth = imageBGR.GetWidth();
                    uint32 dwHeight = imageBGR.GetHeight();

                    uint32* pMemBGR = &imageBGR.ValueAt(0, 0);

#if !TERRAIN_USE_CIE_COLORSPACE
                    // common case with no multiplier just requires alpha channel fixup
                    for (uint32 dwY = 0; dwY < dwHeight; ++dwY)
                    {
                        for (uint32 dwX = 0; dwX < dwWidth; ++dwX)
                        {
                            *pMemBGR++ |= 0xff000000; // shader requires alpha channel = 1
                        }
                    }
#else // convert RGB colour into format that has less compression artefacts for brightness variations
                    for (uint32 dwY = 0; dwY < dwHeight; ++dwY)
                    {
                        for (uint32 dwX = 0; dwX < dwWidth; ++dwX)
                        {
                            float fR = GetBValue(*pMemBGR) * (1.0f / 255.0f); // Reading BGR, so R == B
                            float fG = GetGValue(*pMemBGR) * (1.0f / 255.0f);
                            float fB = GetRValue(*pMemBGR) * (1.0f / 255.0f); // Reading BGR, so B == R

                            ColorF cCol = ColorF(fR, fG, fB);

                            // Convert to linear space
                            cCol.srgb2rgb();

                            cCol.Clamp();

                            cCol = cCol.RGB2mCIE();

                            // Convert to gamma 2.2 space
                            cCol.rgb2srgb();

                            uint32 dwR = (uint32)(cCol.r * 255.0f + 0.5f);
                            uint32 dwG = (uint32)(cCol.g * 255.0f + 0.5f);
                            uint32 dwB = (uint32)(cCol.b * 255.0f + 0.5f);

                            *pMemBGR++ = 0xff000000 | RGB(dwB, dwG, dwR);        // shader requires alpha channel = 1, Storing in BGR.
                        }
                    }
#endif
                }

                GetIEditor()->GetRenderer()->UpdateTextureInVideoMemory(texId, (unsigned char*)imageBGR.GetData(),
                    iLocalOutMinX, iLocalOutMinY, imageBGR.GetWidth(), imageBGR.GetHeight(), eTF_B8G8R8A8);
                AddModSector(iSectX, iSectY);
            }
        }
    }
}

/////////////////////////////////////////////////////////
void CHeightmap::GetWeightmapBlock(int x, int y, int width, int height, Weightmap& map)
{
    if (m_Weightmap.IsValid())
    {
        m_Weightmap.GetSubImage(x, y, width, height, map);
    }
}

/////////////////////////////////////////////////////////
void CHeightmap::SetWeightmapBlock(int x, int y, const Weightmap& map)
{
    if (m_Weightmap.IsValid())
    {
        m_Weightmap.SetSubImage(x, y, map);
    }
}


//////////////////////////////////////////////////////////////////////////
void CHeightmap::UpdateSectorTexture(const QPoint& texsector,
    const float fGlobalMinX, const float fGlobalMinY, const float fGlobalMaxX, const float fGlobalMaxY)
{
    I3DEngine* p3DEngine = GetIEditor()->Get3DEngine();

    int texSectorSize = p3DEngine->GetTerrainTextureNodeSizeMeters();
    if (texSectorSize == 0)
    {
        return;
    }

    int texSectorCount = p3DEngine->GetTerrainSize() / texSectorSize;

    if (texSectorCount == 0 || texsector.x() >= texSectorCount || texsector.y() >= texSectorCount)
    {
        return;
    }

    float fInvSectorCnt = 1.0f / (float)texSectorCount;
    float fMinX = texsector.x() * fInvSectorCnt;
    float fMinY = texsector.y() * fInvSectorCnt;

    uint32 dwFullResolution = m_TerrainBGRTexture.CalcMaxLocalResolution(fMinX, fMinY, fMinX + fInvSectorCnt, fMinY + fInvSectorCnt);

    if (dwFullResolution)
    {
        uint32 dwNeededResolution = dwFullResolution / texSectorCount;

        CTerrainSector* st = m_terrainGrid->GetSector(texsector);

        bool bFullRefreshRequired = true;

        int iLocalOutMinX = 0, iLocalOutMinY = 0, iLocalOutMaxX = dwNeededResolution, iLocalOutMaxY = dwNeededResolution;       // in pixel

        bool bRecreated;

        int texId = m_terrainGrid->LockSectorTexture(texsector, dwNeededResolution, bRecreated);

        if (bRecreated)
        {
            bFullRefreshRequired = true;
        }

        if (!bFullRefreshRequired)
        {
            iLocalOutMinX = floor((fGlobalMinX - fMinX) * dwFullResolution);
            iLocalOutMinY = floor((fGlobalMinY - fMinY) * dwFullResolution);
            iLocalOutMaxX = ceil((fGlobalMaxX - fMinX) * dwFullResolution);
            iLocalOutMaxY = ceil((fGlobalMaxY - fMinY) * dwFullResolution);

            iLocalOutMinX = CLAMP(iLocalOutMinX, 0, dwNeededResolution);
            iLocalOutMinY = CLAMP(iLocalOutMinY, 0, dwNeededResolution);
            iLocalOutMaxX = CLAMP(iLocalOutMaxX, 0, dwNeededResolution);
            iLocalOutMaxY = CLAMP(iLocalOutMaxY, 0, dwNeededResolution);
        }

        if (iLocalOutMaxX != iLocalOutMinX && iLocalOutMaxY != iLocalOutMinY)
        {
            CImageEx imageBGR;

            imageBGR.Allocate(iLocalOutMaxX - iLocalOutMinX, iLocalOutMaxY - iLocalOutMinY);

            m_TerrainBGRTexture.GetSubImageStretched(
                fMinX + fInvSectorCnt / dwNeededResolution * iLocalOutMinX,
                fMinY + fInvSectorCnt / dwNeededResolution * iLocalOutMinY,
                fMinX + fInvSectorCnt / dwNeededResolution * iLocalOutMaxX,
                fMinY + fInvSectorCnt / dwNeededResolution * iLocalOutMaxY,
                imageBGR);

            // convert RGB colour into format that has less compression artifacts for brightness variations
#if TERRAIN_USE_CIE_COLORSPACE
            {
                uint32 dwWidth = imageBGR.GetWidth();
                uint32 dwHeight = imageBGR.GetHeight();

                uint32* pMemBGR = &imageBGR.ValueAt(0, 0);
                if (!pMemBGR)
                {
                    CryLog("Can't get surface terrain texture. May be it was not generated.");
                    return;
                }

                for (uint32 dwY = 0; dwY < dwHeight; ++dwY)
                {
                    for (uint32 dwX = 0; dwX < dwWidth; ++dwX)
                    {
                        float fR = GetBValue(*pMemBGR) * (1.0f / 255.0f); // Reading BGR, so R == B
                        float fG = GetGValue(*pMemBGR) * (1.0f / 255.0f);
                        float fB = GetRValue(*pMemBGR) * (1.0f / 255.0f); // Reading BGR, so B == R

                        ColorF cCol = ColorF(fR, fG, fB);

                        // Convert to linear space
                        cCol.srgb2rgb();

                        cCol.Clamp();

                        cCol = cCol.RGB2mCIE();

                        // Convert to gamma 2.2 space
                        cCol.rgb2srgb();

                        uint32 dwR = (uint32)(cCol.r * 255.0f + 0.5f);
                        uint32 dwG = (uint32)(cCol.g * 255.0f + 0.5f);
                        uint32 dwB = (uint32)(cCol.b * 255.0f + 0.5f);

                        *pMemBGR++ = 0xff000000 | RGB(dwB, dwG, dwR);        // shader requires alpha channel = 1, Storing in BGR.
                    }
                }
            }
#endif

            GetIEditor()->GetRenderer()->UpdateTextureInVideoMemory(texId, (unsigned char*)imageBGR.GetData(), iLocalOutMinX, iLocalOutMinY, iLocalOutMaxX - iLocalOutMinX, iLocalOutMaxY - iLocalOutMinY, eTF_B8G8R8A8);
            AddModSector(texsector.x(), texsector.y());
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CHeightmap::InitTerrain()
{
    InitSectorGrid();

    // construct terrain in 3dengine if was not loaded during SerializeTerrain call
    SSectorInfo si;
    GetSectorsInfo(si);

    STerrainInfo TerrainInfo;
    TerrainInfo.nHeightMapSize_InUnits = si.sectorSize * si.numSectors / si.unitSize;
    TerrainInfo.nUnitSize_InMeters = si.unitSize;
    TerrainInfo.nSectorSize_InMeters = si.sectorSize;
    TerrainInfo.nSectorsTableSize_InSectors = si.numSectors;
    TerrainInfo.fHeightmapZRatio = 0.0f;
    TerrainInfo.fOceanWaterLevel = GetOceanLevel();

    bool bCreateTerrain = false;
    ITerrain* pTerrain = gEnv->p3DEngine->GetITerrain();
    if (pTerrain)
    {
        if ((gEnv->p3DEngine->GetTerrainSize() != (si.sectorSize * si.numSectors)) ||
            (gEnv->p3DEngine->GetHeightMapUnitSize() != si.unitSize))
        {
            bCreateTerrain = true;
        }
    }
    else
    {
        bCreateTerrain = true;
    }
    if (bCreateTerrain)
    {
        pTerrain = gEnv->p3DEngine->CreateTerrain(TerrainInfo);

        // pass heightmap data to the 3dengine
        UpdateEngineTerrain(false);
    }
}


/////////////////////////////////////////////////////////
void CHeightmap::AddModSector(int x, int y)
{
    // Switching mod sectors to set for insert efficiency
    m_modSectors.insert(std::make_pair(x, y));
}

/////////////////////////////////////////////////////////
void CHeightmap::ClearModSectors()
{
    m_modSectors.clear();
}

/////////////////////////////////////////////////////////
void CHeightmap::UnlockSectorsTexture(const QRect& rc)
{
    if (m_modSectors.size())
    {
        //  Switching mod sectors to set for insert efficiency
        //  This would probably be more efficient doing a set::find on each sector
        for (std::set<std::pair<int, int> >::iterator it = m_modSectors.begin(); it != m_modSectors.end(); )
        {
            QPoint pointSector(std::get<0>(*it), std::get<1>(*it));
            if (rc.contains(pointSector))
            {
                GetTerrainGrid()->UnlockSectorTexture(pointSector);
                it = m_modSectors.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
}

/////////////////////////////////////////////////////////
void CHeightmap::UpdateModSectors()
{
    if (!m_updateModSectors)
    {
        return;
    }

    I3DEngine* p3DEngine = GetIEditor()->Get3DEngine();
    int iTerrainSize = p3DEngine->GetTerrainSize();
    if (!iTerrainSize)
    {
        assert(false && "[CHeightmap::UpdateModSectors] Zero sized terrain.");     // maybe we should visualized this to the user
        return;
    }

    if (m_modSectors.size())
    {
        // Switching mod sectors to set for efficiency
        for (std::set<std::pair<int, int> >::iterator it = m_modSectors.begin(); it != m_modSectors.end(); ++it)
        {
            UpdateSectorTexture(QPoint(std::get<0>(*it), std::get<1>(*it)), 0, 0, 1, 1);
        }
    }
    m_updateModSectors = false;
}

/////////////////////////////////////////////////////////
bool CHeightmap::IsAllocated()
{
    return !m_pHeightmap.empty();
}

int CHeightmap::GetNoiseSize() const
{
    return NOISE_ARRAY_SIZE;
}

float CHeightmap::GetNoise(int x, int y) const
{
    // wraps negative indices such that [-ve, 0] is equivalent to [0, MAX] instead of [MAX, 0]
    if (x < 0)
    {
        x = NOISE_ARRAY_SIZE - x;
    }
    if (y < 0)
    {
        y = NOISE_ARRAY_SIZE - y;
    }
    return m_pNoise->m_Array[x % NOISE_ARRAY_SIZE][y % NOISE_ARRAY_SIZE];
}

std::shared_ptr<CImageEx> CHeightmap::GetHeightmapImageEx() const
{
    const int width = static_cast<int>(GetWidth());
    const int height = static_cast<int>(GetHeight());
    const float fPrecisionScale = GetBytePrecisionScale();
    std::shared_ptr<CImageEx> image = std::make_shared<CImageEx>();
    image->Allocate(width, height);
    uint32* pImageData = image->GetData();
    for (int i = 0; i < width * height; ++i)
    {
        // Get a normalized grayscale value from the heightmap
        const uint8 iColor = (uint8)__min((m_pHeightmap[i] * fPrecisionScale + 0.5f), 255.0f);
        // Create an ARGB grayscale value
        pImageData[i] = (0xFF << 24) | (iColor << 16) | (iColor << 8) | iColor;
    }
    return image;
}

std::shared_ptr<CFloatImage> CHeightmap::GetHeightmapFloatImage(bool scaleValues, ImageRotationDegrees rotationAmount) const
{
    const int width = static_cast<int>(GetWidth());
    const int height = static_cast<int>(GetHeight());

    CFloatImage image;
    image.Allocate(m_iWidth, m_iHeight);

    float scale = scaleValues ? m_fMaxHeight : 1.0f;

    for (int j = 0; j < m_iHeight; j++)
    {
        for (int i = 0; i < m_iWidth; i++)
        {
            image.ValueAt(i, j) = GetXY(i, j) / scale;
        }
    }

    std::shared_ptr<CFloatImage> rotatedImage = std::make_shared<CFloatImage>();
    rotatedImage->RotateOrt(image, rotationAmount);

    return rotatedImage;
}

void CHeightmap::NotifyModified(int x, int y, int width, int height)
{
    // TODO: Make this a generic delegate (not hardcoded to the global Editor instance...)
    if (!m_standaloneMode)
    {
        GetIEditor()->GetTerrainManager()->SetModified(x, y, width, height);
    }
}
