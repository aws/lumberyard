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
#include "Layer.h"

#include "TerrainGrid.h"
#include "CryEditDoc.h"                     // will be removed
#include "CrySizer.h"                           // ICrySizer
#include "SurfaceType.h"                    // CSurfaceType

#include "Terrain/TerrainManager.h"

#include "MaterialUtils.h" // for Canonicalizing legacy asset IDs

#include <QPainter>

//! Size of the texture preview
#define LAYER_TEX_PREVIEW_CX 128
//! Size of the texture preview
#define LAYER_TEX_PREVIEW_CY 128

#define MAX_TEXTURE_SIZE (1024 * 1024 * 2)

#define DEFAULT_MASK_RESOLUTION 4096

// Static member variables
UINT CLayer::m_iInstanceCount = 0;

#if !defined(_RELEASE)
    // Texture names to be used for error / process loading indications
    static const char* szReplaceMe = "EngineAssets/TextureMsg/ReplaceMe.tif";
#else
    // Some of the textures here will direct to the regular DefaultSolids_diff to prevent eye catching
    // bug textures in release mode
    static const char* szReplaceMe = "EngineAssets/TextureMsg/ReplaceMeRelease.tif";
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLayer::CLayer()
    : m_cLayerFilterColor(1, 1, 1)
    , m_fLayerBrightness(0.5f)
    , m_fLayerTiling(1)
    , m_fSpecularAmount(0)
    , m_fSortOrder(0)
    , m_fUseRemeshing(0)
{
    ////////////////////////////////////////////////////////////////////////
    // Initialize member variables
    ////////////////////////////////////////////////////////////////////////
    // One more instance
    m_iInstanceCount++;

    m_dwLayerId = e_undefined;        // not set yet

    // Create a layer name based on the instance count
    m_strLayerName = QObject::tr("Layer %1").arg(m_iInstanceCount);

    // Init member variables
    m_LayerStart = 0;
    m_LayerEnd = 1024;
    m_strLayerTexPath = "";

    m_minSlopeAngle = 0;
    m_maxSlopeAngle = 90;

    m_guid = QUuid::createUuid();

    // Create the bitmap
    m_bmpLayerTexPrev = QImage(LAYER_TEX_PREVIEW_CX, LAYER_TEX_PREVIEW_CX, QImage::Format_RGBA8888);

    // Layer is used
    m_bLayerInUse = true;
    m_bSelected = false;

    m_numSectors = 0;

    SetSectorInfoSurfaceTextureSize();
}

uint32 CLayer::GetCurrentLayerId()
{
    return m_dwLayerId;
}

uint32 CLayer::GetOrRequestLayerId()
{
    if (m_dwLayerId == e_undefined)      // not set yet
    {
        bool bFree[e_undefined];

        for (uint32 id = 0; id < e_undefined; id++)
        {
            bFree[id] = true;
        }

        GetIEditor()->GetTerrainManager()->MarkUsedLayerIds(bFree);
        GetIEditor()->GetHeightmap()->MarkUsedLayerIds(bFree);

        for (uint32 id = 0; id < e_undefined; id++)
        {
            if (bFree[id])
            {
                m_dwLayerId = id;
                CryLog("GetOrRequestLayerId() '%s' m_dwLayerId=%d", qPrintable(GetLayerName()), m_dwLayerId);
                GetIEditor()->GetDocument()->SetModifiedFlag(TRUE);
                GetIEditor()->GetDocument()->SetModifiedModules(eModifiedTerrain);
                break;
            }
        }
    }

    assert(m_dwLayerId < e_undefined);

    return m_dwLayerId;
}


CLayer::~CLayer()
{
    CCryEditDoc* doc = GetIEditor()->GetDocument();

    SetSurfaceType(NULL);


    m_iInstanceCount--;
}

QString CLayer::GetTextureFilename()
{
    //Note that this function is used by the UI for display purposes, so its nice to show the source image name (TIF or whatever) instead of asset ID:
    return QFileInfo(Path::GamePathToFullPath(m_strLayerTexPath)).fileName();
}

QString CLayer::GetTextureFilenameWithPath() const
{
    // This function is used to open a file browser, so return a source name, not an assetID
    return Path::GamePathToFullPath(m_strLayerTexPath);
}


//////////////////////////////////////////////////////////////////////////
void CLayer::DrawLayerTexturePreview(const QRect& rcPos, QPainter* pDC)
{
    assert(pDC);
    QBrush brshGray;

    if (!m_bmpLayerTexPrev.isNull())
    {
        pDC->setRenderHint(QPainter::SmoothPixmapTransform);
        pDC->drawImage(rcPos, m_bmpLayerTexPrev);
    }
    else
    {
        brshGray = QPalette().brush(QPalette::Button);
        pDC->fillRect(rcPos, brshGray);
    }
}

void CLayer::Serialize(CXmlArchive& xmlAr)
{
    CCryEditDoc* doc = GetIEditor()->GetDocument();

    if (xmlAr.bLoading)
    {
        XmlNodeRef layer = xmlAr.root;

        layer->getAttr("Name", m_strLayerName);

        //for compatibility purpose we check existing of "GUID" attribute, if not GUID generated in constructor will be used
        if (layer->haveAttr("GUID"))
        {
            layer->getAttr("GUID", m_guid);
        }

        // Texture
        layer->getAttr("Texture", m_strLayerTexPath);

        // convert legacy data.  The call to MaterialUtils will eliminate invalid prefixes such as /engine/ and also elimiante the exetension.
        // it is an in-place operation that always shortens, we we can re-use the buffer:
        auto layerTexPathBuffer = m_strLayerTexPath.toUtf8();
        MaterialUtils::UnifyMaterialName(layerTexPathBuffer.data());
        size_t newLength = strlen(layerTexPathBuffer.data()); // since the operation was in place, we must find the null and shorten it manually:
        m_strLayerTexPath.truncate(newLength); // truncate to that length.
        m_strLayerTexPath.append(".dds"); // dds is the only true extension for image assets is dds.  m_strLayerTexPath is now a true Asset ID

        layer->getAttr("TextureWidth", m_cTextureDimensions.rwidth());
        layer->getAttr("TextureHeight", m_cTextureDimensions.rheight());

        if (layer->haveAttr("SplatMapPath"))
        {
            layer->getAttr("SplatMapPath", m_splatMapPath);
        }
        else if (layer->haveAttr("SplatMaskPath"))
        {
            layer->getAttr("SplatMaskPath", m_splatMapPath);
        }

        // Parameters (Altitude, Slope...)
        layer->getAttr("AltStart", m_LayerStart);
        layer->getAttr("AltEnd", m_LayerEnd);
        layer->getAttr("MinSlopeAngle", m_minSlopeAngle);
        layer->getAttr("MaxSlopeAngle", m_maxSlopeAngle);

        // In use flag
        layer->getAttr("InUse", m_bLayerInUse);


        if (m_dwLayerId > e_undefined)
        {
            CLogFile::WriteLine("ERROR: LayerId is out of range - value was clamped");
            m_dwLayerId = e_undefined;
        }

        {
            QString sSurfaceType;

            layer->getAttr("SurfaceType", sSurfaceType);

            auto terrainManager = GetIEditor()->GetTerrainManager();
            int iSurfaceType = terrainManager->FindSurfaceType(sSurfaceType);
            if (iSurfaceType >= 0)
            {
                SetSurfaceType(terrainManager->GetSurfaceTypePtr(iSurfaceType));
            }
            else if (sSurfaceType != "")
            {
                AssignMaterial(sSurfaceType);
            }
        }

        if (!layer->getAttr("LayerId", m_dwLayerId))
        {
            if (m_pSurfaceType)
            {
                m_dwLayerId = m_pSurfaceType->GetSurfaceTypeID();
            }

            char str[256];
            sprintf_s(str, "LayerId missing - old level format - generate value from detail layer %d", m_dwLayerId);
            CLogFile::WriteLine(str);
        }

        {
            Vec3 vFilterColor(1, 1, 1);

            layer->getAttr("FilterColor", vFilterColor);

            m_cLayerFilterColor = vFilterColor;
        }

        {
            m_fLayerBrightness = 1;
            layer->getAttr("LayerBrightness", m_fLayerBrightness);
        }

        {
            m_fUseRemeshing = 0;
            layer->getAttr("UseRemeshing", m_fUseRemeshing);
        }

        {
            m_fLayerTiling = 1;
            layer->getAttr("LayerTiling", m_fLayerTiling);
        }

        {
            m_fSpecularAmount = 0;
            layer->getAttr("SpecularAmount", m_fSpecularAmount);
        }

        {
            m_fSortOrder = 0;
            layer->getAttr("SortOrder", m_fSortOrder);
        }

        void* pData;
        int nSize;
        if (xmlAr.pNamedData && xmlAr.pNamedData->GetDataBlock(QString("Layer_") + m_strLayerName, pData, nSize))
        {
            // Load it
            if (!LoadTexture((DWORD*)pData, m_cTextureDimensions.width(), m_cTextureDimensions.height()))
            {
                Warning("Failed to load texture for layer %s", m_strLayerName.toUtf8().data());
            }
        }
        else if (xmlAr.pNamedData)
        {
            // Try loading texture from external file,
            if (!m_strLayerTexPath.isEmpty() && !LoadTexture(m_strLayerTexPath))
            {
                GetISystem()->Warning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, VALIDATOR_FLAG_FILE, m_strLayerTexPath.toUtf8().data(), "Failed to load texture for layer %s", m_strLayerName.toUtf8().data());
            }
        }
    }
    else
    {
        XmlNodeRef layer = xmlAr.root;

        // Name
        layer->setAttr("Name", m_strLayerName.toUtf8().data());

        //GUID
        layer->setAttr("GUID", m_guid);

        // Texture
        layer->setAttr("Texture", m_strLayerTexPath.toUtf8().data());
        layer->setAttr("TextureWidth", m_cTextureDimensions.width());
        layer->setAttr("TextureHeight", m_cTextureDimensions.height());

        if (!m_splatMapPath.isEmpty())
        {
            layer->setAttr("SplatMapPath", m_splatMapPath.toUtf8().data());
        }

        // Parameters (Altitude, Slope...)
        layer->setAttr("AltStart", m_LayerStart);
        layer->setAttr("AltEnd", m_LayerEnd);
        layer->setAttr("MinSlopeAngle", m_minSlopeAngle);
        layer->setAttr("MaxSlopeAngle", m_maxSlopeAngle);

        // In use flag
        layer->setAttr("InUse", m_bLayerInUse);

        // Auto mask or explicit mask.
        layer->setAttr("LayerId", m_dwLayerId);

        {
            QString sSurfaceType = "";

            CSurfaceType* pSurfaceType = m_pSurfaceType;

            if (pSurfaceType)
            {
                sSurfaceType = pSurfaceType->GetName();
            }

            layer->setAttr("SurfaceType", sSurfaceType.toUtf8().data());
        }

        {
            Vec3 vFilterColor = Vec3(m_cLayerFilterColor.r, m_cLayerFilterColor.g, m_cLayerFilterColor.b);

            layer->setAttr("FilterColor", vFilterColor);
        }

        {
            layer->setAttr("LayerBrightness", m_fLayerBrightness);
        }

        {
            layer->setAttr("UseRemeshing", m_fUseRemeshing);
        }

        {
            layer->setAttr("LayerTiling", m_fLayerTiling);
        }

        {
            layer->setAttr("SpecularAmount", m_fSpecularAmount);
        }

        {
            layer->setAttr("SortOrder", m_fSortOrder);
        }

        int layerTexureSize = m_cTextureDimensions.width() * m_cTextureDimensions.height() * sizeof(DWORD);
    }
}

void CLayer::FillWithColor(const QColor& col, int width, int height)
{
    m_cTextureDimensions = QSize(width, height);

    // Allocate new memory to hold the bitmap data
    m_cTextureDimensions = QSize(width, height);
    m_texture.Allocate(width, height);
    uint32* pTex = m_texture.GetData();
    for (int i = 0; i < width * height; i++)
    {
        *pTex++ = RGB(col.red(), col.green(), col.blue());
    }
}

bool CLayer::LoadTexture(const QString& lpBitmapName, UINT iWidth, UINT iHeight)
{
    ////////////////////////////////////////////////////////////////////////
    // Load a layer texture out of a ressource
    ////////////////////////////////////////////////////////////////////////

    QImage bmpLoad(lpBitmapName);

    assert(iWidth != 0);
    assert(iHeight != 0);

    if (bmpLoad.isNull())
    {
        assert(!bmpLoad.isNull());
        return false;
    }

    // Save the bitmap's dimensions
    m_cTextureDimensions = QSize(iWidth, iHeight);

    // Free old tetxure data

    // Allocate new memory to hold the bitmap data
    m_cTextureDimensions = QSize(iWidth, iHeight);
    m_texture.Allocate(iWidth, iHeight);

    // Retrieve the bits from the bitmap
    memcpy(m_texture.GetData(), bmpLoad.bits(), 128 * 128 * sizeof(DWORD));
    // no alpha-channel wanted
    std::transform(m_texture.GetData(), m_texture.GetData() + 128 * 128, m_texture.GetData(), [](unsigned int i) { return 0x00ffffff & i; });

    return true;
}

inline bool IsPower2(int n)
{
    for (int i = 0; i < 30; i++)
    {
        if (n == (1 << i))
        {
            return true;
        }
    }
    return false;
}

bool CLayer::LoadTexture(QString strFileName)
{
    CLogFile::FormatLine("Loading layer texture (%s)...", strFileName.toUtf8().data());

    // Save the filename
    m_strLayerTexPath = Path::FullPathToGamePath(strFileName);
    if (m_strLayerTexPath.isEmpty())
    {
        m_strLayerTexPath = strFileName;
    }

    return LoadTextureFromPath();
}

bool CLayer::LoadTextureFromPath()
{
    bool bQualityLoss;
    bool bError = false;

    // Try to load the source asset first, if its available.  It doesn't have to be:
    QString sourcePath = Path::GamePathToFullPath(m_strLayerTexPath);
    if (!CFileUtil::FileExists(sourcePath) || !CImageUtil::LoadImage(sourcePath, m_texture, &bQualityLoss))
    {
        // we failed to load the source asset itself, try the output compiled asset instead:
        if (!CImageUtil::LoadImage(m_strLayerTexPath, m_texture, &bQualityLoss))
        {
            CLogFile::FormatLine("Error loading layer texture (%s)...", m_strLayerTexPath.toUtf8().data());
            bError = true;
        }
    }

    if (!bError && (!IsPower2(m_texture.GetWidth()) || !IsPower2(m_texture.GetHeight())))
    {
        Warning("Selected Layer Texture must have power of 2 size.");
        bError = true;
    }

    if (bError && !CImageUtil::LoadImage(szReplaceMe, m_texture, &bQualityLoss))
    {
        m_texture.Allocate(4, 4);
        m_texture.Fill(0xff);
    }

    // Store the size
    m_cTextureDimensions = QSize(m_texture.GetWidth(), m_texture.GetHeight());

    QImage bmpLoad(reinterpret_cast<uchar*>(m_texture.GetData()), m_texture.GetWidth(), m_texture.GetHeight(), QImage::Format_RGBA8888);

    // Copy it to the preview bitmap
    QPainter painter(&m_bmpLayerTexPrev);
    painter.drawImage(m_bmpLayerTexPrev.rect(), bmpLoad);

    CImageEx filteredImage;
    filteredImage.Allocate(m_texture.GetWidth(), m_texture.GetHeight());
    for (int y = 0; y < m_texture.GetHeight(); y++)
    {
        for (int x = 0; x < m_texture.GetWidth(); x++)
        {
            uint32 val = m_texture.ValueAt(x, y);

            ColorB& colorB = *(ColorB*)&val;

            ColorF colorF(colorB.r / 255.f, colorB.g / 255.f, colorB.b / 255.f);
            colorB = colorF;

            filteredImage.ValueAt(x, y) = val;
        }
    }

    m_previewImage.Allocate(LAYER_TEX_PREVIEW_CX, LAYER_TEX_PREVIEW_CY);
    CImageUtil::ScaleToFit(filteredImage, m_previewImage);

    return !bError;
}

bool CLayer::LoadTexture(DWORD* pBitmapData, UINT iWidth, UINT iHeight)
{
    ////////////////////////////////////////////////////////////////////////
    // Load a texture from an array into the layer
    ////////////////////////////////////////////////////////////////////////

    if (iWidth == 0 || iHeight == 0)
    {
        return false;
    }

    // Allocate new memory to hold the bitmap data
    m_cTextureDimensions = QSize(iWidth, iHeight);
    m_texture.Allocate(iWidth, iHeight);

    // Copy the image data into the layer
    memcpy(m_texture.GetData(), pBitmapData, m_texture.GetSize());

    ////////////////////////////////////////////////////////////////////////
    // Generate the texture preview
    ////////////////////////////////////////////////////////////////////////


    // Create the matching bitmap
    QImage bmpLoad(reinterpret_cast<uchar*>(pBitmapData), iWidth, iHeight, QImage::Format_RGBA8888);
    if (bmpLoad.isNull())
    {
        assert(0);
        return false;
    }

    // Copy it to the preview bitmap
    QPainter painter(&m_bmpLayerTexPrev);
    painter.drawImage(m_bmpLayerTexPrev.rect(), bmpLoad);

    return true;
}


bool CLayer::ExportTexture(QString strFileName)
{
    ////////////////////////////////////////////////////////////////////////
    // Save the texture data of this layer into a BMP file
    ////////////////////////////////////////////////////////////////////////


    CLogFile::WriteLine("Exporting texture from layer data...");

    PrecacheTexture();

    if (!m_texture.IsValid())
    {
        return false;
    }

    // Write the bitmap
    return CImageUtil::SaveImage(strFileName, m_texture);
}

void CLayer::ReleaseTempResources()
{
    // If texture image is too big, release it.
    if (m_texture.GetSize() > MAX_TEXTURE_SIZE)
    {
        m_texture.Release();
    }
}

void CLayer::PrecacheTexture()
{
    if (m_texture.IsValid())
    {
        return;
    }

    if (!LoadTextureFromPath())
    {
        Error("Error caching layer texture (%s)...", m_strLayerTexPath.toUtf8().data());
        m_cTextureDimensions = QSize(4, 4);
        m_texture.Allocate(m_cTextureDimensions.width(), m_cTextureDimensions.height());
        m_texture.Fill(0xff);
        return;
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayer::SetSectorInfoSurfaceTextureSize()
{
    CHeightmap* pHeightmap = GetIEditor()->GetHeightmap();
    SSectorInfo si;
    pHeightmap->GetSectorsInfo(si);
    m_numSectors = si.numSectors;

    m_sectorInfoSurfaceTextureSize = si.surfaceTextureSize;
}


//////////////////////////////////////////////////////////////////////////
void CLayer::SetLayerId(const uint32 dwLayerId)
{
    assert(dwLayerId < e_undefined);

    m_dwLayerId = dwLayerId < e_undefined ? dwLayerId : e_undefined;
}

//////////////////////////////////////////////////////////////////////////
int CLayer::GetSize() const
{
    int size = sizeof(*this);
    size += m_texture.GetSize();
    return size;
}

//////////////////////////////////////////////////////////////////////////
QImage& CLayer::GetTexturePreviewBitmap()
{
    return m_bmpLayerTexPrev;
}

//////////////////////////////////////////////////////////////////////////
CImageEx& CLayer::GetTexturePreviewImage()
{
    return m_previewImage;
}

void CLayer::GetMemoryUsage(ICrySizer* pSizer)
{
    pSizer->Add(*this);
}

//////////////////////////////////////////////////////////////////////////
void CLayer::AssignMaterial(const QString& materialName)
{
    bool bFound = false;
    CCryEditDoc* doc = GetIEditor()->GetDocument();
    auto terrainManager = GetIEditor()->GetTerrainManager();
    for (int i = 0; i < terrainManager->GetSurfaceTypeCount(); i++)
    {
        CSurfaceType* pSrfType = terrainManager->GetSurfaceTypePtr(i);
        if (QString::compare(pSrfType->GetMaterial(), materialName, Qt::CaseInsensitive) == 0)
        {
            SetSurfaceType(pSrfType);
            bFound = true;
            break;
        }
    }

    if (!bFound)
    {
        // If this is the last reference of this particular surface type, then we
        // simply change its data without fearing that anything else will change.
        if (m_pSurfaceType && m_pSurfaceType->GetLayerReferenceCount() == 1)
        {
            m_pSurfaceType->SetMaterial(materialName);
            m_pSurfaceType->SetName(materialName);
        }
        else if (terrainManager->GetSurfaceTypeCount() < MAX_SURFACE_TYPE_ID_COUNT)
        {
            // Create a new surface type.
            CSurfaceType* pSrfType = new CSurfaceType;

            SetSurfaceType(pSrfType);

            pSrfType->SetMaterial(materialName);
            pSrfType->SetName(materialName);
            terrainManager->AddSurfaceType(pSrfType);

            pSrfType->AssignUnusedSurfaceTypeID();
        }
        else
        {
            Warning("Maximum of %d different detail textures are supported.", MAX_SURFACE_TYPE_ID_COUNT);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayer::SetSurfaceType(CSurfaceType* pSurfaceType)
{
    if (pSurfaceType != m_pSurfaceType)
    {
        // Unreference previous surface type.
        _smart_ptr<CSurfaceType> pPrev = m_pSurfaceType;
        if (m_pSurfaceType)
        {
            m_pSurfaceType->RemoveLayerReference();
        }
        m_pSurfaceType = pSurfaceType;
        if (m_pSurfaceType)
        {
            m_pSurfaceType->AddLayerReference();
        }

        if (pPrev && pPrev->GetLayerReferenceCount() < 1)
        {
            // Old unreferenced surface type must be deleted.
            GetIEditor()->GetTerrainManager()->RemoveSurfaceType((CSurfaceType*)pPrev);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
int CLayer::GetEngineSurfaceTypeId() const
{
    if (m_pSurfaceType)
    {
        return m_pSurfaceType->GetSurfaceTypeID();
    }
    return e_undefined;
}

void CLayer::SetSelected(bool bSelected)
{
    m_bSelected = bSelected;
}

QString CLayer::GetLayerPath() const
{
    CSurfaceType* pSurfaceType = GetSurfaceType();

    if (pSurfaceType)
    {
        QString sName = pSurfaceType->GetName();
        QString sNameLower = sName.toLower();
        QString sTag = "materials/terrain/";
        int nInd = sNameLower.indexOf(sTag);

        if (nInd == 0)
        {
            sName = sName.mid(sTag.length());
        }

        nInd = sName.lastIndexOf('/');

        if (nInd != -1)
        {
            return sName.mid(0, nInd);
        }
    }

    return "";
}
