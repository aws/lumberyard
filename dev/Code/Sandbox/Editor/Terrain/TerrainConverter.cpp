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

#include <StdAfx.h>
#include "TerrainConverter.h"
#include "ITerrain.h"
#include "Cry3DEngine/Terrain/Texture/MacroTextureImporter.h"
#include <AzCore/Math/MathUtils.h>
#include "RGBLayer.h"
#include "MacroTextureExporter.h"
#include <qmessagebox.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include "GameEngine.h"

namespace TerrainConverter
{

bool ConvertTerrain(const Config& config)
{
    // Check the terrain importer's results to see if an upgrade is needed
    MacroTextureConfiguration macroTextureConfig;

    AZStd::string quadtreeFilePath;
    AzFramework::StringFunc::Path::ConstructFull(GetIEditor()->GetLevelFolder().toUtf8().data(), config.macroTextureQuadtreeFilename.c_str(), quadtreeFilePath);

    // See if the file exists before asking to read it or else the reader will spit a warning.
    {
        ScopedFileHandle scopedHandle(quadtreeFilePath.c_str(), "rbx");
        if (!scopedHandle.IsValid())
        {
            return false;
        }
    }

    // The file exists, so try to read it.
    bool success = GetIEditor()->Get3DEngine()->ReadMacroTextureFile(quadtreeFilePath.c_str(), macroTextureConfig);
    if (!success)
    {
        // failed to read file.
        return false;
    }

    if (!AZ::IsClose(macroTextureConfig.colorMultiplier_deprecated, 1.0f, 0.001f))
    {
        // Warn user about file modifications.
        QMessageBox::information(
            GetIEditor()->GetEditorMainWindow(),
            QObject::tr("Update Terrain Megatexture"),
            QObject::tr(
                "This level's terrain megatexture is incompatible with this version of Lumberyard and needs to be updated. "
                "The \"terraintexture.pak\" and \"terrain\\cover.ctc\" files will be modified to work with this version of Lumberyard."
            ),
            QMessageBox::StandardButton::Ok
        );

        // Color multiplier needs to be applied to all the data.
        return FixTerrainMultiplier(config, macroTextureConfig);
    }
    ;
    return true;

}

bool FixTerrainMultiplier(const Config& config, MacroTextureConfiguration& macroTextureConfig)
{
    // Transform the multiplier into linear space.
    AZ::Color color = AZ::Color(macroTextureConfig.colorMultiplier_deprecated, 0.0f, 0.0f, 1.0f);
    float colorMultiplier = color.LinearToGamma().GetR();

    AZStd::string quadtreeFilePath;
    AZStd::string xmlFilePath;

    AzFramework::StringFunc::Path::ConstructFull(GetIEditor()->GetLevelFolder().toUtf8().data(), config.macroTextureQuadtreeFilename.c_str(), quadtreeFilePath);
    AzFramework::StringFunc::Path::ConstructFull(GetIEditor()->GetLevelDataFolder().toUtf8().data(), config.macroTextureXMLFilename.c_str(), xmlFilePath);

    XmlNodeRef root = XmlHelpers::LoadXmlFromFile(xmlFilePath.c_str());
    if (!root)
    {
        return false;
    }

    CRGBLayer RGBLayer(config.macroTexturePakFilename.c_str());
    RGBLayer.Serialize(root, true);

    // Make sure terrain texture pak file can be written
    if (!CFileUtil::OverwriteFile(RGBLayer.GetFullFileName()))
    {
        Error(QStringLiteral("Cannot overwrite file %1").arg(RGBLayer.GetFullFileName()).toUtf8().data());
        return false;
    }
    // Make sure the quadtree file can be written (do this now to make sure both files can be modified before actually changing them)
    QString qsQuadtreeFilePath = QString(quadtreeFilePath.c_str());
    if (!CFileUtil::OverwriteFile(qsQuadtreeFilePath))
    {
        Error(QStringLiteral("Cannot overwrite file %1").arg(qsQuadtreeFilePath).toUtf8().data());
        return false;
    }

    // Tell RGB layer to multiply all of it's colors by the provided color multiplier. This will fix the data in memory.
    RGBLayer.ApplyColorMultiply(colorMultiplier);

    // Now that the RGBLayer data is fixed, tell it to serialize back out.
    root = XmlNodeRef();
    RGBLayer.Serialize(root, false);

    // Pull the max size from the current macrotexture quadtree - it should stay the same when it's re-exported.
    const uint32 childCount = 4;
    const int32 invalidIndex = -1;
    uint32 tileCount = 0;
    uint32 depth = 0;
    for (int16 index : macroTextureConfig.indexBlocks)
    {
        if (index != invalidIndex)
        {
            if (++tileCount > pow(childCount, depth))
            {
                depth += 1;
                tileCount = 0; // reset the tile count since it's progressed to the next teir of the quad tree.
            }
        }
    }

    const uint32 childEdgeCount = 2;
    uint32 tilesAlongSide = pow(childEdgeCount, depth);
    uint32 maximumSize = macroTextureConfig.tileSizeInPixels * tilesAlongSide;

    MacroTextureExporter exporter(&RGBLayer);
    exporter.SetMaximumTotalSize(maximumSize);

    exporter.Export(quadtreeFilePath.c_str());

    RGBLayer.FreeData();

    return true;
}

} // End namespace TerrainConverter
