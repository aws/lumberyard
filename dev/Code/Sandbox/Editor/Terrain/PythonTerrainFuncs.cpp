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
#include "IEditor.h"
#include "GameEngine.h"
#include "ViewManager.h"
#include "Util/BoostPythonHelpers.h"
#include "Terrain/Heightmap.h"

#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

#include <QFileDialog>

namespace PyTerrainHeightmap
{
    float GetMaxHeight()
    {
        float maxHeight = 0.0f;
        if (GetIEditor())
        {
            CHeightmap *heightmap = GetIEditor()->GetHeightmap();
            maxHeight = heightmap ? heightmap->GetMaxHeight() : 0.0f;
        }
        return maxHeight;
    }

    void SetMaxHeight(float maxHeight)
    {
        if (GetIEditor())
        {
            CHeightmap *heightmap = GetIEditor()->GetHeightmap();
            if (heightmap)
            {
                heightmap->SetMaxHeight(maxHeight);
            }
        }
    }

    void ImportHeightmap(const char *fileName, bool resize)
    {
        if (GetIEditor())
        {
            CHeightmap *heightmap = GetIEditor()->GetHeightmap();
            if (heightmap)
            {
                heightmap->ImportHeightmap(fileName, resize ? CHeightmap::HeightmapImportTechnique::Resize : CHeightmap::HeightmapImportTechnique::Clip);
                heightmap->RefreshTerrain();
            }
        }
    }

    void ExportHeightmap(const char *fileName)
    {
        if (GetIEditor())
        {
            CHeightmap *heightmap = GetIEditor()->GetHeightmap();
            if (heightmap)
            {
                heightmap->ExportHeightmap(fileName);
            }
        }
    }

    float GetElevation(float x, float y)
    {
        return GetIEditor() ? GetIEditor()->GetTerrainElevation(x, y) : 0.0f;
    }
}

REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTerrainHeightmap::GetMaxHeight, terrain, get_max_height,
    "Gets the max height of the terrain.",
    "float terrain.get_max_height()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTerrainHeightmap::SetMaxHeight, terrain, set_max_height,
    "Sets the max height of the terrain.",
    "void terrain.set_max_height(float maxHeight)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTerrainHeightmap::ImportHeightmap, terrain, import_heightmap,
    "Import a heightmap into the terrain.  Use resize=True to scale the input if it's a different size, resize=False to clip it.",
    "void terrain.import_heightmap(string filename, bool resize)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTerrainHeightmap::ExportHeightmap, terrain, export_heightmap,
    "Export a heightmap from the terrain.",
    "void terrain.export_heightmap(string filename)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTerrainHeightmap::GetElevation, terrain, get_elevation,
    "Get the elevation for an XY point on the terrain.",
    "void terrain.get_elevation(float x, float y)");
