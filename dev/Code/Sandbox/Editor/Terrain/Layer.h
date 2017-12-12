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

#ifndef CRYINCLUDE_EDITOR_TERRAIN_LAYER_H
#define CRYINCLUDE_EDITOR_TERRAIN_LAYER_H
#pragma once

#include "LayerWeight.h"

class CSurfaceType;

//! Single texture layer
class CLayer
{
public:
    enum
    {
        e_undefined = 127,
        e_hole,
    };

    // constructor
    CLayer();
    // destructor
    virtual ~CLayer();

    // Name
    QString GetLayerName() const { return m_strLayerName; }
    void SetLayerName(const QString& strName) { m_strLayerName = strName; }
    QString GetLayerPath() const;
    void SetLayerPath(const QString& strPath) { m_strLayerPath = strPath; }
    QString GetSplatMapPath() const { return m_splatMapPath;  }
    void SetSplatMapPath(const QString& splatMapPath) { m_splatMapPath = splatMapPath; }

    //! Get this Layer GUID.
    REFGUID GetGUID() const { return m_guid; }

    // Slope Angle 0=flat..90=max steep
    float GetLayerMinSlopeAngle() { return m_minSlopeAngle; }
    float GetLayerMaxSlopeAngle() { return m_maxSlopeAngle; }
    float GetLayerBrightness() { return m_fLayerBrightness; }
    float GetLayerUseRemeshing() { return m_fUseRemeshing; }
    float GetLayerTiling() { return m_fLayerTiling; }
    float GetLayerSpecularAmount() { return m_fSpecularAmount; }
    float GetLayerSortOrder() { return m_fSortOrder; }
    ColorF GetLayerFilterColor() { return m_cLayerFilterColor; }

    void SetLayerMinSlopeAngle(float min) { m_minSlopeAngle = min; }
    void SetLayerMaxSlopeAngle(float max) { m_maxSlopeAngle = max; }
    void SetLayerBrightness(float fBr) { m_fLayerBrightness = fBr; }
    void SetLayerUseRemeshing(float fUseRemeshing) { m_fUseRemeshing = fUseRemeshing; }
    void SetLayerTiling(float fVal) { m_fLayerTiling = fVal; }
    void SetLayerSpecularAmount(float fVal) { m_fSpecularAmount = fVal; }
    void SetLayerSortOrder(float fVal) { m_fSortOrder = fVal; }
    void SetLayerFilterColor(ColorF colVal) { m_cLayerFilterColor = colVal; }

    // Altitude
    float GetLayerStart() { return m_LayerStart; }
    float GetLayerEnd() { return m_LayerEnd; }
    void SetLayerStart(float iStart) { m_LayerStart = iStart; }
    void SetLayerEnd(float iEnd) { m_LayerEnd = iEnd; }

    // Calculate memory size allocated for this layer
    // Return:
    //   in bytes
    int GetSize() const;

    //////////////////////////////////////////////////////////////////////////
    int GetSectorInfoSurfaceTextureSize() const { return m_sectorInfoSurfaceTextureSize; }

    // Texture
    int GetTextureWidth() { return m_cTextureDimensions.width(); }
    int GetTextureHeight() { return m_cTextureDimensions.height(); }
    QSize GetTextureDimensions() { return m_cTextureDimensions; }
    // filename without path
    QString GetTextureFilename();
    // including path
    QString GetTextureFilenameWithPath() const;
    void DrawLayerTexturePreview(const QRect& rcPos, QPainter* pDC);
    bool LoadTexture(QString strFileName);
    // used to fille with water color
    void FillWithColor(const QColor& col, int width, int height);
    bool LoadTexture(const QString& lpBitmapName, UINT iWidth, UINT iHeight);
    bool LoadTexture(DWORD* pBitmapData, UINT iWidth, UINT iHeight);
    bool ExportTexture(QString strFileName);
    // represents the editor feature
    bool HasTexture() { return ((GetTextureWidth() != 0) && (m_texture.GetData() != NULL)); }

    uint32& GetTexturePixel(int x, int y) { return m_texture.ValueAt(x, y); }


    // Release temporar allocated resources
    void ReleaseTempResources();

    // Serialisation
    void Serialize(CXmlArchive& xmlAr);

    // In use
    bool IsInUse() { return m_bLayerInUse; }
    void SetInUse(bool bNewState) { m_bLayerInUse = bNewState; }

    //////////////////////////////////////////////////////////////////////////
    // True if layer is currently selected.
    bool IsSelected() const { return m_bSelected; }
    //! Mark layer as selected or not.
    void SetSelected(bool bSelected);

    CSurfaceType* GetSurfaceType() const { return m_pSurfaceType; }
    void SetSurfaceType(CSurfaceType* pSurfaceType);
    int GetEngineSurfaceTypeId() const;

    void AssignMaterial(const QString& materialName);

    //! Load texture if it was unloaded.
    void PrecacheTexture();

    //////////////////////////////////////////////////////////////////////////
    //! Set the sector info surface texture size for layer.
    void SetSectorInfoSurfaceTextureSize();

    void GetMemoryUsage(ICrySizer* pSizer);

    //! should always return a valid LayerId
    uint32 GetOrRequestLayerId();

    //! might return 0xffffffff
    uint32 GetCurrentLayerId();

    // CHeightmap::m_LayerIdBitmap
    void SetLayerId(const uint32 dwLayerId);

    QImage& GetTexturePreviewBitmap();
    CImageEx& GetTexturePreviewImage();

private: // -----------------------------------------------------------------------

    QString                 m_strLayerName;         // Name (might not be unique)
    QString                 m_strLayerPath;
    QString                 m_splatMapPath;

    // Layer texture
    QImage                 m_bmpLayerTexPrev;
    QString                 m_strLayerTexPath;
    QSize                       m_cTextureDimensions;

public: // test
    CImageEx                    m_texture;
    CImageEx          m_previewImage;

public:

    ColorF                  m_cLayerFilterColor;        //
    float                       m_fLayerBrightness;         // used together with m_cLayerFilterColor
    float                       m_fUseRemeshing;
    float                       m_fLayerTiling;
    float                       m_fSpecularAmount;
    float                       m_fSortOrder;

protected:

    GUID m_guid;

private:
    bool LoadTextureFromPath();

    // Layer parameters
    float                       m_LayerStart;
    float                       m_LayerEnd;

    float                       m_minSlopeAngle;        // min slope 0=flat..90=max steep
    float                       m_maxSlopeAngle;        // max slope 0=flat..90=max steep

    int                         m_sectorInfoSurfaceTextureSize;               //

    bool                        m_bLayerInUse;                  // Should this layer be used during terrain generation ?

    bool                        m_bSelected;                        // True if layer is selected as current.
    int                         m_iSurfaceTypeId;               // -1 if not assigned yet,
    uint32                  m_dwLayerId;                        // used in CHeightmap::m_LayerIdBitmap, usually in the range 0..0xff, 0xffffffff means not set

    static UINT         m_iInstanceCount;               // Internal instance count, used for Uniq name assignment.

    _smart_ptr<CSurfaceType> m_pSurfaceType;

    //////////////////////////////////////////////////////////////////////////
    // Layer Sectors.
    //////////////////////////////////////////////////////////////////////////
    int                         m_numSectors;
};

#endif // CRYINCLUDE_EDITOR_TERRAIN_LAYER_H
