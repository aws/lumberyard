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
#include "PythonTerrainFuncs.h"
#include "IEditor.h"
#include "GameEngine.h"
#include "ViewManager.h"
#include "Util/BoostPythonHelpers.h"
#include "Terrain/Heightmap.h"

#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <QFileDialog>

namespace PyTerrainHeightmap
{
    float GetMaxHeight()
    {
        IEditor* editor = GetIEditor();

        if (editor)
        {
            CHeightmap* heightMap = editor->GetHeightmap();

            if (heightMap)
            {
                return heightMap->GetMaxHeight();
            }
        }

        return 0.0f;
    }

    void SetMaxHeight(float maxHeight)
    {
        IEditor* editor = GetIEditor();

        if (editor)
        {
            CHeightmap* heightMap = editor->GetHeightmap();

            if (heightMap)
            {
                heightMap->SetMaxHeight(maxHeight);
            }
        }
    }

    void ImportHeightmap(const char* fileName, bool resize)
    {
        IEditor* editor = GetIEditor();

        if (editor)
        {
            CHeightmap* heightMap = editor->GetHeightmap();

            if (heightMap)
            {
                heightMap->ImportHeightmap(fileName, resize ? CHeightmap::HeightmapImportTechnique::Resize : CHeightmap::HeightmapImportTechnique::Clip);
                heightMap->RefreshTerrain();
            }
        }
    }

    void ExportHeightmap(const char* fileName)
    {
        IEditor* editor = GetIEditor();

        if (editor)
        {
            CHeightmap* heightMap = editor->GetHeightmap();

            if (heightMap)
            {
                heightMap->ExportHeightmap(fileName);
            }
        }
    }

    float GetElevation(float x, float y)
    {
        return GetIEditor() ? GetIEditor()->GetTerrainElevation(x, y) : 0.0f;
    }

    void Flatten(float percentage)
    {
        if (percentage > 0.0f && percentage <= 100.0f)
        {
            IEditor* editor = GetIEditor();

            if (editor)
            {
                CHeightmap* heightMap = editor->GetHeightmap();

                if (heightMap)
                {
                    heightMap->Flatten(1.0f - (percentage / 100.f));
                    heightMap->RefreshTerrain();
                }
            }
        }
    }

    void Smooth()
    {
        IEditor* editor = GetIEditor();

        if (editor)
        {
            CHeightmap* heightMap = editor->GetHeightmap();

            if (heightMap)
            {
                heightMap->Smooth();
                heightMap->RefreshTerrain();
            }
        }
    }

    void SmoothSlope()
    {
        IEditor* editor = GetIEditor();

        if (editor)
        {
            CHeightmap* heightMap = editor->GetHeightmap();

            if (heightMap)
            {
                heightMap->SmoothSlope();
                heightMap->RefreshTerrain();
            }
        }
    }

    void Erase()
    {
        IEditor* editor = GetIEditor();

        if (editor)
        {
            CHeightmap* heightMap = editor->GetHeightmap();

            if (heightMap)
            {
                heightMap->Clear(false);
                heightMap->RefreshTerrain();
            }
        }
    }

    void Resize(int resolution, int unitSize)
    {
        IEditor* editor = GetIEditor();

        if (editor)
        {
            CHeightmap* heightMap = editor->GetHeightmap();

            if (heightMap)
            {
                constexpr bool cleanOld = false;
                heightMap->Resize(resolution, resolution, unitSize, cleanOld);
                heightMap->RefreshTerrain();
            }
        }
    }

    void MakeIsle()
    {
        IEditor* editor = GetIEditor();

        if (editor)
        {
            CHeightmap* heightMap = editor->GetHeightmap();

            if (heightMap)
            {
                heightMap->MakeIsle();
                heightMap->RefreshTerrain();
            }
        }
    }

    void Normalize()
    {
        IEditor* editor = GetIEditor();

        if (editor)
        {
            CHeightmap* heightMap = editor->GetHeightmap();

            if (heightMap)
            {
                heightMap->Normalize();
                heightMap->RefreshTerrain();
            }
        }
    }

    void Invert()
    {
        IEditor* editor = GetIEditor();

        if (editor)
        {
            CHeightmap* heightMap = editor->GetHeightmap();

            if (heightMap)
            {
                heightMap->Invert();
                heightMap->RefreshTerrain();
            }
        }
    }

    void Fetch()
    {
        IEditor* editor = GetIEditor();

        if (editor)
        {
            CHeightmap* heightMap = editor->GetHeightmap();

            if (heightMap)
            {
                heightMap->Fetch();
                heightMap->RefreshTerrain();
            }
        }
    }

    void Hold()
    {
        IEditor* editor = GetIEditor();

        if (editor)
        {
            CHeightmap* heightMap = editor->GetHeightmap();

            if (heightMap)
            {
                heightMap->Hold();
            }
        }
    }
}

void AzToolsFramework::TerrainPythonFuncsHandler::Reflect(AZ::ReflectContext* context)
{
    if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
    {
        // this will put these methods into the 'azlmbr.legacy.terrain' module
        auto addTerrainMethod = [](AZ::BehaviorContext::GlobalMethodBuilder methodBuilder)
        {
            methodBuilder->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                         ->Attribute(AZ::Script::Attributes::Category, "Legacy/Editor")
                         ->Attribute(AZ::Script::Attributes::Module, "legacy.terrain");
        };
        addTerrainMethod(behaviorContext->Method("get_max_height", PyTerrainHeightmap::GetMaxHeight, nullptr, "Gets the max height of the terrain."));
        addTerrainMethod(behaviorContext->Method("set_max_height", PyTerrainHeightmap::SetMaxHeight, nullptr, "Sets the max height of the terrain."));
        addTerrainMethod(behaviorContext->Method("import_heightmap", PyTerrainHeightmap::ImportHeightmap, nullptr, "Import a heightmap into the terrain.  Use resize=True to scale the input if it's a different size, resize=False to clip it."));
        addTerrainMethod(behaviorContext->Method("export_heightmap", PyTerrainHeightmap::ExportHeightmap, nullptr, "Export a heightmap from the terrain."));
        addTerrainMethod(behaviorContext->Method("get_elevation", PyTerrainHeightmap::GetElevation, nullptr, "Get the elevation for an XY point on the terrain."));
        addTerrainMethod(behaviorContext->Method("flatten", PyTerrainHeightmap::Flatten, nullptr, "Globally flattens the terrain."));
        addTerrainMethod(behaviorContext->Method("smooth", PyTerrainHeightmap::Smooth, nullptr, "Globally applies a 3x3 smooth to the terrain."));
        addTerrainMethod(behaviorContext->Method("slope_smooth", PyTerrainHeightmap::SmoothSlope, nullptr, "Globally applies a 3x3 smooth to slopes of the terrain."));
        addTerrainMethod(behaviorContext->Method("erase", PyTerrainHeightmap::Erase, nullptr, "Erases/clears the terrain."));
        addTerrainMethod(behaviorContext->Method("resize", PyTerrainHeightmap::Resize, nullptr, "Resizes the terrain."));
        addTerrainMethod(behaviorContext->Method("make_isle", PyTerrainHeightmap::MakeIsle, nullptr, "Convert any terrain into an isle."));
        addTerrainMethod(behaviorContext->Method("normalize", PyTerrainHeightmap::Normalize, nullptr, "Normalize the heightmap to a 0 - max height range."));
        addTerrainMethod(behaviorContext->Method("invert", PyTerrainHeightmap::Invert, nullptr, "Inverts the heightmap."));
        addTerrainMethod(behaviorContext->Method("fetch", PyTerrainHeightmap::Fetch, nullptr, "Reads a backup copy of the heightmap."));
        addTerrainMethod(behaviorContext->Method("hold", PyTerrainHeightmap::Hold, nullptr, "Saves a backup copy of the heightmap."));
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
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTerrainHeightmap::Flatten, terrain, flatten,
    "Globally flattens the terrain.",
    "void terrain.flatten(float percentage)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTerrainHeightmap::Smooth, terrain, smooth,
    "Globally applies a 3x3 smooth to the terrain.",
    "void terrain.smooth()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTerrainHeightmap::SmoothSlope, terrain, slope_smooth,
    "Globally applies a 3x3 smooth to slopes of the terrain.",
    "void terrain.slope_smooth()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTerrainHeightmap::Erase, terrain, erase,
    "Erases/clears the terrain.",
    "void terrain.erase()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTerrainHeightmap::Resize, terrain, resize,
    "Resizes the terrain.",
    "void terrain.resize(int resolution, int unit_size)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTerrainHeightmap::MakeIsle, terrain, make_isle,
    "Convert any terrain into an isle.",
    "void terrain.make_isle()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTerrainHeightmap::Normalize, terrain, normalize,
    "Normalize the heightmap to a 0 - max height range.",
    "void terrain.normalize()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTerrainHeightmap::Invert, terrain, invert,
    "Inverts the heightmap.",
    "void terrain.invert()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTerrainHeightmap::Fetch, terrain, fetch,
    "Reads a backup copy of the heightmap.",
    "void terrain.fetch()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTerrainHeightmap::Hold, terrain, hold,
    "Saves a backup copy of the heightmap.",
    "void terrain.hold()");
