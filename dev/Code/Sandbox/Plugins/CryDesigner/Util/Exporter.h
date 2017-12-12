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

#pragma once

#include "Core/Polygon.h"

class CBrushObject;
class AreaSolidObject;
class ClipVolumeObject;
class CPakFile;
class CCryMemFile;

namespace CD
{
    struct SExportedBrushGeom
    {
        enum EFlags
        {
            SUPPORT_LIGHTMAP = 0x01,
            NO_PHYSICS = 0x02,
        };
        int size; // Size of this sructure.
        char filename[128];
        int flags; //! @see EFlags
        Vec3 m_minBBox;
        Vec3 m_maxBBox;
    };

    struct SExportedBrushMaterial
    {
        int size;
        char material[64];
    };

    class Exporter
    {
    public:
        void ExportBrushes(const QString& path, CPakFile& pakFile);

    private:
        bool ExportAreaSolid(const QString& path, AreaSolidObject* pAreaSolid, CPakFile& pakFile) const;
        bool ExportClipVolume(const QString& path, ClipVolumeObject* pClipVolume, CPakFile& pakFile);
        void ExportStatObj(const QString& path, IStatObj* pStatObj, CBaseObject* pObj, int renderFlag, const QString& sGeomFileName, CPakFile& pakFile);

        struct SAreaSolidStatistic
        {
            int numOfClosedPolygons;
            int numOfOpenPolygons;
            int totalSize;
        };

        static void ComputeAreaSolidMemoryStatistic(AreaSolidObject* pAreaSolid, SAreaSolidStatistic& outStatistic, std::vector<CD::PolygonPtr>& optimizedPolygons);

        //////////////////////////////////////////////////////////////////////////
        std::map<CMaterial*, int> m_mtlMap;
        std::vector<SExportedBrushGeom> m_geoms;
        std::vector<SExportedBrushMaterial> m_materials;
    };
};