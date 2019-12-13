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
#include "PythonTerrainLayerFuncs.h"
#include "IEditor.h"
#include "GameEngine.h"
#include "ViewManager.h"
#include "Util/BoostPythonHelpers.h"
#include "Terrain/Heightmap.h"
#include "BitFiddling.h"

#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <QFileDialog>

// NOTE:  The RGB Layers are rotated 90 degrees from what the UI shows or what you would expect.
// The X tile count and index maps along the Y world axis, and the Y tile count and index maps
// along the X world axis.
// The naming and behavior of the Python bindings provide consistency with the C++ API, even though
// it's non-intuitive.
//
// Some example world to index mappings:
//     (0, 0) in the world maps to index (0, 0).
//     (N, 0) in the world maps to index (0, M).
//     (0, N) in the world maps to index (M, 0)

// See also:  CTerrainGrid::SectorToWorld / CTerrainGrid::WorldToSector

namespace PyTerrainLayers
{
    static CRGBLayer* GetRGBLayer()
    {
        if (GetIEditor() && GetIEditor()->GetHeightmap())
        {
            return GetIEditor()->GetHeightmap()->GetRGBLayer();
        }

        return nullptr;
    }

    // NOTE:  See explanation of tile indices and counts at top of this file.
    uint32 GetTileCountX()
    {
        CRGBLayer *rgbLayer = GetRGBLayer();

        if (rgbLayer)
        {
            return rgbLayer->GetTileCountX();
        }

        return 0;
    }

    // NOTE:  See explanation of tile indices and counts at top of this file.
    uint32 GetTileCountY()
    {
        CRGBLayer *rgbLayer = GetRGBLayer();

        if (rgbLayer)
        {
            return rgbLayer->GetTileCountY();
        }

        return 0;
    }

    // NOTE:  See explanation of tile indices and counts at top of this file.
    uint32 GetTileResolution(uint32 xIndex, uint32 yIndex)
    {
        CRGBLayer *rgbLayer = GetRGBLayer();

        if ((xIndex >= GetTileCountX()) || (yIndex >= GetTileCountY()))
        {
            AZ_Error("Terrain", false, "Invalid Tile Index.  Requested tile (%d, %d), which is outside the bounds of (%d, %d)", 
                      xIndex, yIndex, GetTileCountX(), GetTileCountY());
            return 0;
        }

        if (rgbLayer)
        {
            return rgbLayer->GetTileResolution(xIndex, yIndex);
        }

        return 0;
    }

    // NOTE:  See explanation of tile indices and counts at top of this file.
    bool SetTileResolution(uint32 xIndex, uint32 yIndex, uint32 resolution)
    {
        bool result = false;
        CRGBLayer *rgbLayer = GetRGBLayer();

        if ((xIndex >= GetTileCountX()) || (yIndex >= GetTileCountY()))
        {
            AZ_Error("Terrain", false, "Invalid Tile Index.  Requested tile (%d, %d), which is outside the bounds of (%d, %d)",
                xIndex, yIndex, GetTileCountX(), GetTileCountY());
            return false;
        }

        if ((resolution < 64) || (resolution > 2048) || !IsPowerOfTwo(resolution))
        {
            AZ_Error("Terrain", false, "SetTileResolution requires the resolution to be a power of two between 64-2048.  Requested resolution: %d", resolution);
            return false;
        }

        if (rgbLayer)
        {
            result = rgbLayer->ChangeTileResolution(xIndex, yIndex, resolution);
            if (!result)
            {
                AZ_Error("Terrain", false, "ChangeTileResolution() failed, possibly due to memory usage.");
                return false;
            }

            // Tell the terrain system to update
            GetIEditor()->GetHeightmap()->RefreshTextureTile(xIndex, yIndex);
            GetIEditor()->GetHeightmap()->RefreshTerrain();
        }

        return result;
    }

    // NOTE:  See explanation of tile indices and counts at top of this file.
    bool SetTileCount(uint32 xTiles, uint32 yTiles)
    {
        // Validation: we require the same number of tiles in each direction.
        if (xTiles != yTiles)
        {
            AZ_Error("Terrain", false, "SetTileCount requires the x and y tile counts to be the same.  Requested counts: (%d, %d)", xTiles, yTiles);
            return false;
        }

        // Validation: make sure the number of tiles in each direction is exactly a power of two.
        if ((xTiles == 0) || !IsPowerOfTwo(xTiles))
        {
            AZ_Error("Terrain", false, "SetTileCount requires the x and y tile counts to be non-zero powers of two.  Requested counts: (%d, %d)", xTiles, yTiles);
            return false;
        }

        bool result = false;
        CRGBLayer *rgbLayer = GetRGBLayer();

        if (rgbLayer)
        {
            uint32 curXTiles = rgbLayer->GetTileCountX();
            uint32 curYTiles = rgbLayer->GetTileCountY();

            // Validation:  make sure we're growing the number of tiles, as we don't currently support the ability to shrink.
            if ((xTiles < curXTiles) || (yTiles < curYTiles))
            {
                AZ_Error("Terrain", false, "SetTileCount doesn't support reducing the current tile counts.  Current counts = (%d, %d),  Requested counts = (%d, %d)",
                    curXTiles, curYTiles, xTiles, yTiles);
                return false;
            }

            while (rgbLayer->GetTileCountX() < xTiles)
            {
                result = rgbLayer->RefineTiles();
                if (!result)
                {
                    AZ_Error("Terrain", false, "RefineTiles() failed, possibly due to memory usage or locked files.");
                    return false;
                }
            }
        }

        return result;
    }

    // NOTE:  See explanation of tile indices and counts at top of this file.
    // Returns a color packed as ABGR.
    AZ::u32 GetColorAt(double xPercent, double yPercent)
    {
        if (CRGBLayer* rgbLayer = GetRGBLayer())
        {
            return rgbLayer->GetValueAt(xPercent, yPercent);
        }
        else
        {
            return 0;
        }
    }

    // NOTE:  See explanation of tile indices and counts at top of this file.
    bool ImportMegaterrain(const char* fileName, AZ::u32 left, AZ::u32 top, AZ::u32 right, AZ::u32 bottom)
    {
        if (fileName)
        {
            if (CRGBLayer* rgbLayer = GetRGBLayer())
            {
                if (rgbLayer->TryImportTileRect(fileName, left, top, right, bottom))
                {
                    CHeightmap* heightMap = GetIEditor()->GetHeightmap();
                    AZ::u32 dwTileCountX = rgbLayer->GetTileCountX();
                    AZ::u32 dwTileCountY = rgbLayer->GetTileCountY();

                    QPoint topLeft(left * heightMap->GetWidth() / dwTileCountX, top * heightMap->GetHeight() / dwTileCountY);
                    QPoint topRight(right * heightMap->GetWidth() / dwTileCountX, bottom * heightMap->GetHeight() / dwTileCountY);
                    QRect rect(topLeft, topRight);

                    heightMap->UpdateLayerTexture(rect);
                    heightMap->RefreshTerrain();

                    return true;
                }
            }
        }

        AZ_Error("Terrain", false, "ImportMegaterrain() failed, Check file path and tile indices.");
        return false;
    }

    // NOTE:  See explanation of tile indices and counts at top of this file.
    void ExportMegaterrain(const char* fileName, AZ::u32 left, AZ::u32 top, AZ::u32 right, AZ::u32 bottom)
    {
        if (fileName)
        {
            if (CRGBLayer* rgbLayer = GetRGBLayer())
            {
                rgbLayer->ExportTileRect(fileName, left, top, right, bottom);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////

void AzToolsFramework::TerrainLayerPythonFuncsHandler::Reflect(AZ::ReflectContext* context)
{
    if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
    {
        // this will put these methods into the 'azlmbr.legacy.terrain_layer' module
        auto addTerrainLayer = [](AZ::BehaviorContext::GlobalMethodBuilder methodBuilder)
        {
            methodBuilder->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Legacy/Editor")
                ->Attribute(AZ::Script::Attributes::Module, "legacy.terrain_layer");
        };
        addTerrainLayer(behaviorContext->Method("get_tile_count_x", PyTerrainLayers::GetTileCountX, nullptr, "Gets the number of terrain layer tiles in the X direction of the RGB layer, which is the Y world direction."));
        addTerrainLayer(behaviorContext->Method("get_tile_count_y", PyTerrainLayers::GetTileCountY, nullptr, "Gets the number of terrain layer tiles in the Y direction or the RGB layer, which is the X world direction."));
        addTerrainLayer(behaviorContext->Method("get_tile_resolution", PyTerrainLayers::GetTileResolution, nullptr, "Gets the input resolution of the requested terrain layer tile.\n" \
                                                                                                                    "Note: the tile indices are rotated 90 degrees from the world axis, so the X index is along the Y world axis and vice versa."));
        addTerrainLayer(behaviorContext->Method("set_tile_resolution", PyTerrainLayers::SetTileResolution, nullptr, "Sets the input resolution of the requested terrain layer tile.\n" \
                                                                                                                    "Note: the tile indices are rotated 90 degrees from the world axis, so the X index is along the Y world axis and vice versa."));
        addTerrainLayer(behaviorContext->Method("set_tile_count", PyTerrainLayers::SetTileCount, nullptr, "Sets the number of terrain layer tiles."));
        addTerrainLayer(behaviorContext->Method("get_color_at", PyTerrainLayers::GetColorAt, nullptr, "Returns the pixel color from the megaterrain texture given a normalized texture coordinate."));
        addTerrainLayer(behaviorContext->Method("import_megaterrain", PyTerrainLayers::ImportMegaterrain, nullptr, "Imports and applies a megaterrain texture."));
        addTerrainLayer(behaviorContext->Method("export_megaterrain", PyTerrainLayers::ExportMegaterrain, nullptr, "Exports the active megaterrain texture."));
    }
}

///////////////////////////////////////////////////////////////////

REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTerrainLayers::GetTileCountX, terrain, get_tile_count_x,
    "Gets the number of terrain layer tiles in the X direction of the RGB layer, which is the Y world direction.",
    "int terrain.get_tile_count_x()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTerrainLayers::GetTileCountY, terrain, get_tile_count_y,
    "Gets the number of terrain layer tiles in the Y direction or the RGB layer, which is the X world direction.",
    "int terrain.get_tile_count_y()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTerrainLayers::GetTileResolution, terrain, get_tile_resolution,
    "Gets the input resolution of the requested terrain layer tile.\n" \
    "Note: the tile indices are rotated 90 degrees from the world axis, so the X index is along the Y world axis and vice versa.",
    "int terrain.get_tile_resolution(int tile_index_x, int tile_index_y)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTerrainLayers::SetTileResolution, terrain, set_tile_resolution,
    "Sets the input resolution of the requested terrain layer tile.\n" \
    "Note: the tile indices are rotated 90 degrees from the world axis, so the X index is along the Y world axis and vice versa.",
    "bool terrain.set_tile_resolution(int tile_index_x, int tile_index_y, int resolution)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTerrainLayers::SetTileCount, terrain, set_tile_count,
    "Sets the number of terrain layer tiles.",
    "bool terrain.set_tile_count(int num_tiles_x, int num_tiles_y)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTerrainLayers::GetColorAt, terrain, get_color_at,
    "Returns the pixel color from the megaterrain texture given a normalized texture coordinate.",
    "int terrain.get_color_at(float xPercent, float yPercent)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTerrainLayers::ImportMegaterrain, terrain, import_megaterrain,
    "Imports and applies a megaterrain texture.",
    "bool terrain.import_megaterrain(string file_name, int left, int top, int right, int bottom)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTerrainLayers::ExportMegaterrain, terrain, export_megaterrain,
    "Exports the active megaterrain texture.",
    "void terrain.export_megaterrain(string file_name, int left, int top, int right, int bottom)");
