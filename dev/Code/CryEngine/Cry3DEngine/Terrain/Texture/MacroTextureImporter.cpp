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

#include <ITerrain.h>
#include <Terrain/Bus/LegacyTerrainBus.h>

#include "MacroTextureImporter.h"


namespace MacroTextureImporter
{

MacroTexture::UniquePtr Import(const char* filepath, uint32 maxElementCountPerPool)
{

    LegacyTerrain::MacroTextureConfiguration configuration;
    bool success = ReadMacroTextureFile(filepath, configuration);

    if (!success)
    {
        return nullptr;
    }

    // This is a compiler enforced safety net to avoid crashing the game creating an unreasonable amount of textures.
    const uint32 MaxSupportedCapacity = 4096; // @TODO: This probably shouldn't be a static value and instead be based off of the system capabilities.

    AZ_Assert(maxElementCountPerPool > 0,
        "Attempting to configure texture pool with no capacity");

    AZ_Assert(maxElementCountPerPool <= MaxSupportedCapacity,
        "Attempting to configure texture pool with more than the supported amount of elements. Please increase MaxSupportedCapacity if this is intentional.");

    configuration.maxElementCountPerPool = clamp_tpl(maxElementCountPerPool, 1u, MaxSupportedCapacity);

    // Pass the information in the configuration on to MacroTexture

    return AZStd::unique_ptr<MacroTexture>(new MacroTexture(configuration));

}

bool ReadMacroTextureFile(const char* filepath, LegacyTerrain::MacroTextureConfiguration& configuration)
{
    configuration.filePath = filepath;

    ICryPak& cryPak = *gEnv->pCryPak;

    ScopedFileHandle scopedHandle(filepath, "rbx");
    if (!scopedHandle.IsValid())
    {
        Cry3DEngineBase::FileWarning(0, filepath, "Error opening terrain texture file: file not found (you might need to regenerate the surface texture)");
        return false;
    }
    bool bSwapEndian = false;

    // Common file header

    SCommonFileHeader commonHeader;
    {
        if (!cryPak.FRead(&commonHeader, 1, scopedHandle, false))
        {
            AZ_Warning("LegacyTerrain", false, "Error opening terrain texture file: %s. Header not found (file is broken)", filepath);
            return false;
        }

        configuration.endian = (commonHeader.flags & SERIALIZATION_FLAG_BIG_ENDIAN) ? eBigEndian : eLittleEndian;
        bSwapEndian = configuration.endian != GetPlatformEndian();
        SwapEndian(commonHeader, bSwapEndian);

        if (strcmp(commonHeader.signature, "CRY"))
        {
            AZ_Warning("LegacyTerrain", false, "Error opening terrain texture file: %s. Invalid signature", filepath);
            return false;
        }

        if (commonHeader.version != FILEVERSION_TERRAIN_TEXTURE_FILE)
        {
            AZ_Warning("LegacyTerrain", false, "Error opening terrain texture file: %s. Version incompatible (you might need to regenerate the surface texture)", filepath);
            return false;
        }
    }

    // Texture File Header

    STerrainTextureFileHeader header;
    {
        cryPak.FRead(&header, 1, scopedHandle, bSwapEndian);
        configuration.colorMultiplier_deprecated = header.ColorMultiplier_deprecated;
    }

    // Layer File Headers

    STerrainTextureLayerFileHeader layerHeader;
    {
        cryPak.FRead(&layerHeader, 1, scopedHandle, bSwapEndian);
        if (layerHeader.eTexFormat != 0xFF)
        {
            AZ_Printf("LegacyTerrain", "MacroTexture Tiles: Format: %s, Pixel Dimensions: (%dx%d), Bytes: %d",
                LegacyTerrainBase::GetRenderer()->GetTextureFormatName(layerHeader.eTexFormat),
                layerHeader.SectorSizeInPixels,
                layerHeader.SectorSizeInPixels,
                layerHeader.SectorSizeInBytes);
        }
        else
        {
            AZ_Warning("LegacyTerrain", false, "MacroTexture Layer: %s. FAILED TO LOAD. Please regenerate the terrain texture.", filepath);
        }

        configuration.texureFormat = layerHeader.eTexFormat;

        // The sector index descriptors can vary in size based on the number of layers. Compute the total descriptor size here.
        {
            configuration.totalSectorDataSize = layerHeader.SectorSizeInBytes;
            configuration.tileSizeInPixels = layerHeader.SectorSizeInPixels;

            // Additional layers in header.
            for (uint32 i = 1; i < header.LayerCount; ++i)
            {
                STerrainTextureLayerFileHeader legacyLayerHeader;
                cryPak.FRead(&legacyLayerHeader, 1, scopedHandle, bSwapEndian);
                configuration.totalSectorDataSize += legacyLayerHeader.SectorSizeInBytes;
            }
        }
    }

    // Index Block

    uint32 indexBlockSize = 0;
    {
        uint16 size;
        cryPak.FRead(&size, 1, scopedHandle, bSwapEndian);

        configuration.indexBlocks.resize(size);
        indexBlockSize = size * sizeof(uint16) + sizeof(uint16);
        cryPak.FRead(configuration.indexBlocks.data(), size, scopedHandle, bSwapEndian);

        AZ_Printf("LegacyTerrain", "Texture Index Count: %d", size);
    }

    configuration.sectorStartDataOffset =
        sizeof(SCommonFileHeader) +
        sizeof(STerrainTextureFileHeader) +
        header.LayerCount * sizeof(STerrainTextureLayerFileHeader) +
        indexBlockSize;

    return true;
}

} // end MacroTextureImporter
