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

#include "StdAfx.h"
#include "MacroTextureExporter.h"
#include "RGBLayer.h"
#include "Util/PakFile.h"
#include <ITerrain.h>

#include <qfile.h>
#include <qdir.h>

class MacroTextureExporter::Context
{
public:
    Context(
        const Settings& settings,
        const uint32 tileSizeInBytes,
        const uint64 tileFileOffset,
        const uint32 textureCountMax,
        std::vector<int16>&& indexBlock)
        : m_Settings(settings)
        , m_TileSizeInBytes(tileSizeInBytes)
        , m_TileFileOffset(tileFileOffset)
        , m_TextureCountMax(textureCountMax)
        , m_IndexBlock(std::move(indexBlock))
        , m_CurrentIndex(0)
    {
    }

    bool ExportTiles(const char* ctcFilename, const CCryMemFile& header);

private:
    void ExportTiles(
        CImageEx& tile,
        const int16 parentIndex,
        const float left,
        const float bottom,
        const float size,
        const uint32 recursionDepth);

    void AllocateTile(CImageEx& tile) const;

    void ExportLeafTile(CImageEx& tile, float left, float bottom, float size);

    static void DownSampleWithBordersPreserved(const CImageEx inputQuadrants[4], CImageEx& output);

    const Settings& m_Settings;

    const uint32 m_TileSizeInBytes;

    const uint64 m_TileFileOffset;

    const uint32 m_TextureCountMax;

    const std::vector<int16> m_IndexBlock;

    AZStd::vector<AZ::u8> m_FileMemory;

    uint32 m_CurrentIndex;

    uint32 m_TextureCount;

    struct ImageQuad
    {
        CImageEx Quadrant[4];
    };

    ImageQuad m_ImageStack[32];
};

MacroTextureExporter::MacroTextureExporter(CRGBLayer* texture)
{
    m_Settings.Texture = texture;
    m_Settings.MaximumTotalSize = std::numeric_limits<uint32>::max();
    m_Settings.ProgressCallback = nullptr;
}

void MacroTextureExporter::BuildIndexBlock(
    std::vector<int16>& outIndexBlock,
    uint32& outTextureId,
    const float left,
    const float bottom,
    const float size,
    const uint32 recursionDepth)
{
    bool bDescent = true;

    if (recursionDepth)
    {
        uint32 resMax = m_Settings.Texture->CalcMaxLocalResolution(left, bottom, left + size, bottom + size);

        resMax = (uint32)(resMax * size);

        if (resMax < TileResolution)
        {
            bDescent = false;
        }

        if ((TileResolution << recursionDepth) > m_Settings.MaximumTotalSize)
        {
            bDescent = false;
        }
    }

    if (bDescent)
    {
        outIndexBlock.push_back(outTextureId++);

        float childSize = size * 0.5f;
        for (uint32 x = 0; x < 2; ++x)
        {
            for (uint32 y = 0; y < 2; ++y)
            {
                const float childLeft = left + x * childSize;
                const float childBottom = bottom + y * childSize;
                BuildIndexBlock(outIndexBlock, outTextureId, childLeft, childBottom, childSize, recursionDepth + 1);
            }
        }
    }
    else
    {
        outIndexBlock.push_back(-1);
    }
}

void MacroTextureExporter::Context::DownSampleWithBordersPreserved(const CImageEx inputQuadrants[4], CImageEx& output)
{
    uint32 outputWidth = output.GetWidth();
    uint32 outputHeight = output.GetHeight();

    uint32 inputQuadrantWidth = inputQuadrants[0].GetWidth();
    uint32 inputQuadrantHeight = inputQuadrants[0].GetHeight();

    assert(inputQuadrants[0].GetData() != output.GetData());
    assert(inputQuadrants[1].GetData() != output.GetData());
    assert(inputQuadrants[2].GetData() != output.GetData());
    assert(inputQuadrants[3].GetData() != output.GetData());

    for (uint32 outputY = 0; outputY < outputHeight; ++outputY)
    {
        for (uint32 outputX = 0; outputX < outputWidth; ++outputX)
        {
            // Map output coordinates to input map (2x downsample).
            uint32 inputX = (outputX * 2);
            uint32 inputY = (outputY * 2);

            // Remap input coordinates to a quadrant and compute the index
            uint32 quadrantX = inputX / inputQuadrantWidth;
            uint32 quadrantY = inputY / inputQuadrantHeight;
            uint32 quadrantIndex = quadrantY * 2 + quadrantX;

            // Rebase coordinates to the local quadrant space
            inputX %= inputQuadrantWidth;
            inputY %= inputQuadrantHeight;

            if (outputX == 0)
            {
                if (outputY == 0)
                {
                    // left top corner
                    output.ValueAt(outputX, outputY) = inputQuadrants[quadrantIndex].ValueAt(inputX, inputY);
                }
                else if (outputY == outputHeight - 1)
                {
                    // left bottom corner
                    output.ValueAt(outputX, outputY) = inputQuadrants[quadrantIndex].ValueAt(inputX, inputY + 1);
                }
                else
                {
                    // left side
                    output.ValueAt(outputX, outputY) = ColorB::ComputeAvgCol_Fast(inputQuadrants[quadrantIndex].ValueAt(inputX, inputY), inputQuadrants[quadrantIndex].ValueAt(inputX, inputY + 1));
                }
            }
            else if (outputX == outputWidth - 1)
            {
                if (outputY == 0)
                {
                    // right top corner
                    output.ValueAt(outputX, outputY) = inputQuadrants[quadrantIndex].ValueAt(inputX + 1, inputY);
                }
                else if (outputY == outputHeight - 1)
                {
                    // right bottom corner
                    output.ValueAt(outputX, outputY) = inputQuadrants[quadrantIndex].ValueAt(inputX + 1, inputY + 1);
                }
                else
                {
                    // right side
                    output.ValueAt(outputX, outputY) = ColorB::ComputeAvgCol_Fast(inputQuadrants[quadrantIndex].ValueAt(inputX + 1, inputY), inputQuadrants[quadrantIndex].ValueAt(inputX + 1, inputY + 1));
                }
            }
            else
            {
                if (outputY == 0)
                {
                    output.ValueAt(outputX, outputY) = ColorB::ComputeAvgCol_Fast(inputQuadrants[quadrantIndex].ValueAt(inputX, inputY), inputQuadrants[quadrantIndex].ValueAt(inputX + 1, inputY));
                }
                else if (outputY == outputHeight - 1)
                {
                    output.ValueAt(outputX, outputY) = ColorB::ComputeAvgCol_Fast(inputQuadrants[quadrantIndex].ValueAt(inputX, inputY + 1), inputQuadrants[quadrantIndex].ValueAt(inputX + 1, inputY + 1));
                }
                else
                {
                    // inner
                    uint32 dwC[2];

                    dwC[0] = ColorB::ComputeAvgCol_Fast(inputQuadrants[quadrantIndex].ValueAt(inputX, inputY), inputQuadrants[quadrantIndex].ValueAt(inputX + 1, inputY));
                    dwC[1] = ColorB::ComputeAvgCol_Fast(inputQuadrants[quadrantIndex].ValueAt(inputX, inputY + 1), inputQuadrants[quadrantIndex].ValueAt(inputX + 1, inputY + 1));

                    output.ValueAt(outputX, outputY) = ColorB::ComputeAvgCol_Fast(dwC[0], dwC[1]);
                }
            }
        }
    }
}

namespace
{
    class ScopedFileMap
    {
    public:
        ScopedFileMap(QFile& file, uint64 totalSize)
            : m_File(file)
            , m_MappedMemory(nullptr)
        {
            if (!m_File.isOpen())
            {
                m_File.open(QIODevice::WriteOnly | QIODevice::ReadOnly);
            }

            m_MappedMemory = m_File.map(0, totalSize);
        }

        ~ScopedFileMap()
        {
            if (m_MappedMemory)
            {
                m_File.unmap(m_MappedMemory);
                m_MappedMemory = nullptr;
            }
            m_File.close();
        }

        bool bIsOpen() const
        {
            return m_File.isOpen() && m_MappedMemory != nullptr;
        }

        inline uchar* GetMemory()
        {
            return m_MappedMemory;
        }

    private:
        QFile& m_File;
        uchar* m_MappedMemory;
    };
}

bool MacroTextureExporter::Export(const char* ctcFilename)
{
    IEditor* editor = GetIEditor();
    IRenderer* renderer = editor->GetSystem()->GetIRenderer();

    editor->SetStatusText("Exporting terrain texture (Saving)...");
    CLogFile::WriteLine("Generating terrain texture...");

    ETEX_Format textureFormat = eTF_B8G8R8A8;
    uint32 tileSizeInBytes = renderer->GetTextureFormatDataSize(TileResolution, TileResolution, 1, 1, textureFormat);

    CCryMemFile ctcFile;

    // Common Header
    {
        SCommonFileHeader header;
        ZeroStruct(header);

        cry_strcpy(header.signature, "CRY");
        header.file_type = 0;
        header.flags = 0;
        header.version = FILEVERSION_TERRAIN_TEXTURE_FILE;
        ctcFile.Write(&header, sizeof(header));
    }

    // Texture Sub-Header
    {
        STerrainTextureFileHeader subHeader;
        subHeader.LayerCount = 1;
        subHeader.Flags = 0;
        subHeader.ColorMultiplier_deprecated = 1.0f;
        ctcFile.Write(&subHeader, sizeof(subHeader));
    }

    // Layer Header
    {
        STerrainTextureLayerFileHeader layerHeader;
        layerHeader.eTexFormat = textureFormat;
        layerHeader.SectorSizeInPixels = TileResolution;
        layerHeader.SectorSizeInBytes = tileSizeInBytes;
        ctcFile.Write(&layerHeader, sizeof(STerrainTextureLayerFileHeader));
    }

    uint32 textureCountMax;
    std::vector<int16> indexBlock;

    {
        uint32 textureId = 0;
        BuildIndexBlock(indexBlock, textureId, 0.0f, 0.0f, 1.0f, 0);
        textureCountMax = textureId;

        uint16 size = (uint16)indexBlock.size();
        ctcFile.Write(&size, sizeof(uint16));
        ctcFile.Write(&indexBlock[0], sizeof(uint16) * size);
    }

    const uint64 tileFileOffset = ctcFile.GetPosition();
    const double FileSizeMB = (double)(tileSizeInBytes * textureCountMax) / (1024.0 * 1024.0);

    CryLog("Generation stats: %d tiles (base: %dx%d) = %.2f MB", textureCountMax, TileResolution, TileResolution, FileSizeMB);

    Context context(m_Settings, tileSizeInBytes, tileFileOffset, textureCountMax, std::move(indexBlock));
    return context.ExportTiles(ctcFilename, ctcFile);
}

void MacroTextureExporter::Context::AllocateTile(CImageEx& tile) const
{
    if (!tile.IsValid())
    {
        tile.Allocate(TileResolution, TileResolution);
    }
}

void MacroTextureExporter::Context::ExportLeafTile(CImageEx& tile, float left, float bottom, float size)
{
    AllocateTile(tile);

    m_Settings.Texture->GetSubImageStretched(left, bottom, left + size, bottom + size, tile, true);

#if TERRAIN_USE_CIE_COLORSPACE
    // convert RGB colour into format that has less compression artifacts for brightness variations
    {
        uint32* pMem = &tile.ValueAt(0, 0);

        for (uint32 y = 0; y < tile.GetHeight(); ++y)
        {
            for (uint32 x = 0; x < tile.GetWidth(); ++x)
            {
                uint32 r, g, b;

                {
                    float fR = GetRValue(*pMem) * (1.0f / 255.0f);
                    float fG = GetGValue(*pMem) * (1.0f / 255.0f);
                    float fB = GetBValue(*pMem) * (1.0f / 255.0f);

                    ColorF cCol = ColorF(fR, fG, fB);

                    // Convert to linear space
                    cCol.srgb2rgb();


                    cCol = cCol.RGB2mCIE();


                    // Convert to gamma 2.2 space
                    cCol.rgb2srgb();

                    r = (uint32)(cCol.r * 255.0f + 0.5f);
                    g = (uint32)(cCol.g * 255.0f + 0.5f);
                    b = (uint32)(cCol.b * 255.0f + 0.5f);
                }

                *pMem = RGB(r, g, b);
                pMem++;
            }
        }
    }
#endif
}

bool MacroTextureExporter::Context::ExportTiles(const char* ctcFilename, const CCryMemFile& header)
{
    IEditor& editor = *GetIEditor();
    ICryPak& cryPak = *gEnv->pCryPak;

    char adjustedCoverPath[ICryPak::g_nMaxPath];
    cryPak.AdjustFileName(ctcFilename, adjustedCoverPath, AZ_ARRAY_SIZE(adjustedCoverPath), ICryPak::FLAGS_NEVER_IN_PAK | ICryPak::FLAGS_NO_LOWCASE);

    if (!cryPak.MakeDir(adjustedCoverPath))
    {
        Error("Error generating macro texture: Failed to create destination directory");
        return false;
    }

    if (!CFileUtil::OverwriteFile(adjustedCoverPath))
    {
        Error(QStringLiteral("Cannot overwrite file %1").arg(adjustedCoverPath).toUtf8().data());
        return false;
    }

    const uint64 totalFileSize = m_TileFileOffset + m_TileSizeInBytes * m_TextureCountMax;

    m_FileMemory.resize(totalFileSize);
    memcpy(m_FileMemory.data(), header.GetMemPtr(), header.GetLength());

    editor.SetStatusText(QObject::tr("Export surface texture..."));

    {
        CImageEx& rootTile = m_ImageStack[0].Quadrant[0];
        const int16 RootIndex = m_IndexBlock[m_CurrentIndex++];
        const float RootLeft = 0.0f;
        const float RootBottom = 0.0f;
        const float RootSize = 1.0f;
        ExportTiles(rootTile, RootIndex, RootLeft, RootBottom, RootSize, 0);
    }

    char resolvedCoverPath[AZ_MAX_PATH_LEN];
    AZ::IO::FileIOBase::GetInstance()->ResolvePath(adjustedCoverPath, resolvedCoverPath, AZ_MAX_PATH_LEN);
    QFile outputFile = {
        QString{ resolvedCoverPath }
    };

    // macOS and Linux need to open the file, otherwise resize fails for non-existing files.
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::ReadOnly))
    {
        Error("Error generating macro texture: Failed to create destination file %s", resolvedCoverPath);
        return false;
    }


    if (!outputFile.resize(totalFileSize))
    {
        Error("Error generating macro texture: Failed to resize destination file %s", resolvedCoverPath);
        return false;
    }

    {
        ScopedFileMap scopedFile(outputFile, totalFileSize);
        if (!scopedFile.bIsOpen())
        {
            Error("Error generating macro texture: Failed to open destination file %s", resolvedCoverPath);
            return false;
        }

        memcpy(scopedFile.GetMemory(), m_FileMemory.data(), m_FileMemory.size());
    }

    m_FileMemory.clear();
    editor.SetStatusText("Ready");
    return true;
}

void MacroTextureExporter::Context::ExportTiles(
    CImageEx& tile,
    const int16 parentIndex,
    const float left,
    const float bottom,
    const float size,
    const uint32 dwRecursionDepth)
{
    bool bLeafNode = true;
    AZ_Assert(m_CurrentIndex < m_IndexBlock.size(), "Exceeded index block array bounds.");

    //
    // This is a leaf node if the next four indices are sentinels
    //
    if (parentIndex != -1)
    {
        for (uint32 i = 0; i < 4; ++i)
        {
            bool bHasChild = m_IndexBlock[m_CurrentIndex + i] >= 0;
            if (bHasChild)
            {
                bLeafNode = false;
                break;
            }
        }
    }

    //
    // Leaf nodes are processed from the texture data itself.
    //
    if (bLeafNode)
    {
        ExportLeafTile(tile, left, bottom, size);

        if (parentIndex != -1)
        {
            m_CurrentIndex += 4;
        }
    }

    //
    // Interior nodes downsample from the four higher resolution child textures.
    //
    else
    {
        ImageQuad& imageQuad = m_ImageStack[dwRecursionDepth + 1];

        const float childSize = size / 2.0f;

        for (uint32 x = 0; x < 2; ++x)
        {
            for (uint32 y = 0; y < 2; ++y)
            {
                const uint32 quadrant   = x + y * 2;
                const float childLeft   = left   + x * childSize;
                const float childBottom = bottom + y * childSize;
                const int16 childIndex  = m_IndexBlock[m_CurrentIndex++];
                CImageEx& childTile = imageQuad.Quadrant[quadrant];

                ExportTiles(childTile, childIndex, childLeft, childBottom, childSize, dwRecursionDepth + 1);
                AZ_Assert(childTile.IsValid(), "Exported tile not valid.");
            }
        }

        //
        // Attaching the image here tears down the images as we traverse back up the tree.
        //
        CImageEx InputQuarters[4];
        for (uint32 quadrant = 0; quadrant < 4; ++quadrant)
        {
            InputQuarters[quadrant].Attach(imageQuad.Quadrant[quadrant]);
        }

        AllocateTile(tile);
        DownSampleWithBordersPreserved(InputQuarters, tile);
    }

    if (parentIndex != -1)
    {
        uint64 fileOffset = m_TileFileOffset + m_TileSizeInBytes * parentIndex;

        memcpy(m_FileMemory.data() + fileOffset, tile.GetData(), tile.GetSize());

        if (m_Settings.ProgressCallback)
        {
            m_Settings.ProgressCallback(100 * m_TextureCount / m_TextureCountMax);
        }

        ++m_TextureCount;
    }
}
