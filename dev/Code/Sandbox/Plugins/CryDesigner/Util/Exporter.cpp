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
#include "Exporter.h"

#include "Objects/BrushObject.h"
#include "Core/BrushHelper.h"
#include "Objects/AreaSolidObject.h"
#include "Objects/ClipVolumeObject.h"
#include "Objects/DesignerObject.h"
#include "Objects/Group.h"
#include "Material/Material.h"
#include "CryArray.h"
#include "Util/Pakfile.h"
#include "IChunkFile.h"
#include "Core/ModelCompiler.h"
#include "Core/Model.h"
#include "Core/PolygonDecomposer.h"

#include <I3DEngine.h>

#define BRUSH_SUB_FOLDER "Brush"
#define BRUSH_FILE "brush.lst"
#define BRUSH_LIST_FILE "brushlist.txt"

using namespace CD;

struct brush_sort_predicate
{
    bool operator() (const std::pair<QString, CBaseObject*>& left, const std::pair<QString, CBaseObject*>& right)
    {
        return left.first < right.first;
    }
};

void Exporter::ExportBrushes(const QString& path, CPakFile& pakFile)
{
    CLogFile::WriteLine("Exporting Brushes...");

    pakFile.RemoveDir(Path::Make(path, BRUSH_SUB_FOLDER).toUtf8().data());

    QString filename = Path::Make(path, BRUSH_FILE);
    QString brushListFilename = Path::Make(path, BRUSH_LIST_FILE);
    CCryMemFile file;

    DynArray<CBaseObject*> objects;
    GetIEditor()->GetObjectManager()->GetObjects(objects);

    typedef std::vector<std::pair<QString, CBaseObject*> > SortedObjects;
    SortedObjects sortedObjects;
    for (int i = 0; i < objects.size(); i++)
    {
        if (objects[i] == NULL)
        {
            continue;
        }
        if (!IsDesignerRelated(objects[i]))
        {
            continue;
        }

        QString gameFileName;
        if (GenerateGameFilename(objects[i], gameFileName) == false)
        {
            continue;
        }

        if (!CD::CheckIfHasValidModel(objects[i]))
        {
            continue;
        }

        sortedObjects.push_back(std::make_pair(gameFileName, objects[i]));
    }

    std::sort(sortedObjects.begin(), sortedObjects.end(), brush_sort_predicate());

    for (SortedObjects::const_iterator it = sortedObjects.begin(); it != sortedObjects.end(); ++it)
    {
        CBaseObject* pObject = it->second;

        if (pObject->GetType() != OBJTYPE_SOLID && pObject->GetType() != OBJTYPE_VOLUMESOLID)
        {
            continue;
        }

        if (qobject_cast<AreaSolidObject*>(pObject))
        {
            AreaSolidObject* obj = (AreaSolidObject*)pObject;
            if (!ExportAreaSolid(path, obj, pakFile))
            {
                assert(0);
            }
        }
        else if (qobject_cast<ClipVolumeObject*>(pObject))
        {
            ClipVolumeObject* pClipVolume = static_cast<ClipVolumeObject*>(pObject);
            if (!ExportClipVolume(path, pClipVolume, pakFile))
            {
                assert(0);
            }
        }
        else if (qobject_cast<DesignerObject*>(pObject))
        {
            _smart_ptr<IStatObj> pStatObj = NULL;
            if (GetIStatObj(pObject, &pStatObj) == false)
            {
                continue;
            }
            QString gameFileName;
            if (GenerateGameFilename(pObject, gameFileName) == false)
            {
                continue;
            }
            int nRenderFlag(0);
            if (GetRenderFlag(pObject, nRenderFlag) == false)
            {
                continue;
            }
            ExportStatObj(path, pStatObj, pObject, nRenderFlag, gameFileName, pakFile);
            UpdateStatObj(pObject);
        }
    }

    pakFile.RemoveFile(filename.toUtf8().constData());

    {
        CCryMemFile brushListFile;
        string tempStr;
        for (int i = 0; i < m_geoms.size(); i++)
        {
            tempStr = m_geoms[i].filename;
            tempStr += "\r\n";
            brushListFile.Write(tempStr.c_str(), tempStr.size());
        }
        pakFile.UpdateFile(brushListFilename.toLocal8Bit(), brushListFile);
    }

    CLogFile::WriteString("Done.");
}

void Exporter::ExportStatObj(const QString& path, IStatObj* pStatObj, CBaseObject* pObj, int renderFlag, const QString& sGeomFileName, CPakFile& pakFile)
{
    if (pStatObj == NULL || pObj == NULL)
    {
        return;
    }

    QString sRealGeomFileName = sGeomFileName;
    sRealGeomFileName.replace("%level%", Path::ToUnixPath(Path::RemoveBackslash(path)));

    QString sInternalGeomFileName = sGeomFileName;

    IChunkFile* pChunkFile = NULL;
    if (pStatObj->SaveToCGF(sRealGeomFileName.toLocal8Bit(), &pChunkFile))
    {
        void* pMemFile = NULL;
        int nFileSize = 0;
        pChunkFile->WriteToMemoryBuffer(&pMemFile, &nFileSize);
        pakFile.UpdateFile(sRealGeomFileName.toLocal8Bit(), pMemFile, nFileSize, true, ICryArchive::LEVEL_FASTER);
        pChunkFile->Release();
    }

    SExportedBrushGeom geom;
    ZeroStruct(geom);
    geom.size = sizeof(geom);
    cry_strcpy(geom.filename, sInternalGeomFileName.toLocal8Bit());
    geom.flags = 0;
    geom.m_minBBox = pStatObj->GetBoxMin();
    geom.m_maxBBox = pStatObj->GetBoxMax();
    m_geoms.push_back(geom);
    int geomIndex = m_geoms.size() - 1;

    int mtlIndex = -1;
    CMaterial* pMaterial = pObj->GetMaterial();
    if (pMaterial)
    {
        mtlIndex = stl::find_in_map(m_mtlMap, pMaterial, -1);
        if (mtlIndex < 0)
        {
            SExportedBrushMaterial mtl;
            mtl.size = sizeof(mtl);
            memset(mtl.material, 0, sizeof(mtl.material));
            cry_strcpy(mtl.material, pMaterial->GetFullName().toUtf8().data());
            m_materials.push_back(mtl);
            mtlIndex = m_materials.size() - 1;
            m_mtlMap[pMaterial] = mtlIndex;
        }
    }

    SExportedBrushGeom* pBrushGeom = &m_geoms[geomIndex];
    if (pBrushGeom && pStatObj)
    {
        if (pStatObj->GetPhysGeom() == NULL && pStatObj->GetPhysGeom(PHYS_GEOM_TYPE_NO_COLLIDE) == NULL)
        {
            pBrushGeom->flags |= SExportedBrushGeom::NO_PHYSICS;
        }
    }
}

bool AppendDataToMemoryblock(CMemoryBlock& memoryBlock, int& memoryOffset, void* data, int datasize)
{
    if (memoryBlock.GetBuffer() == NULL)
    {
        return false;
    }
    int nextOffset = memoryOffset + datasize;
    if (nextOffset > memoryBlock.GetSize())
    {
        return false;
    }
    memcpy(((char*)memoryBlock.GetBuffer()) + memoryOffset, data, datasize);
    memoryOffset = nextOffset;
    return true;
}

bool ExportPolygonForAreaSolid(CMemoryBlock& memoryBlock, int& offset, const std::vector<SVertex>& vertices, const std::vector<SMeshFace>& meshFaces)
{
    int numberOfFaces(meshFaces.size());
    for (int k = 0; k < numberOfFaces; ++k)
    {
        int numberOfVertices(3);

        if (!AppendDataToMemoryblock(memoryBlock, offset, &numberOfVertices, sizeof(numberOfVertices)))
        {
            return false;
        }

        for (int a = 0; a < numberOfVertices; ++a)
        {
            Vec3 vPos = ToVec3(vertices[meshFaces[k].v[a]].pos);
            if (!AppendDataToMemoryblock(memoryBlock, offset, &vPos, sizeof(Vec3)))
            {
                return false;
            }
        }
    }
    return true;
}

bool Exporter::ExportAreaSolid(const QString& path, AreaSolidObject* pAreaSolid, CPakFile& pakFile) const
{
    if (pAreaSolid == NULL)
    {
        return false;
    }
    CD::ModelCompiler* pCompiler(pAreaSolid->GetCompiler());
    if (pCompiler == NULL)
    {
        return false;
    }
    Model* pModel(pAreaSolid->GetModel());
    if (pModel == NULL)
    {
        return false;
    }

    std::vector<PolygonPtr> optimizedPolygons;
    pModel->GetPolygonList(optimizedPolygons);

    int iPolygonSize = optimizedPolygons.size();
    if (iPolygonSize <= 0)
    {
        return false;
    }

    SAreaSolidStatistic statisticForAreaSolid;
    ComputeAreaSolidMemoryStatistic(pAreaSolid, statisticForAreaSolid, optimizedPolygons);

    CMemoryBlock memoryBlock;
    memoryBlock.Allocate(statisticForAreaSolid.totalSize);

    int offset = 0;
    if (!AppendDataToMemoryblock(memoryBlock, offset, &statisticForAreaSolid.numOfClosedPolygons, sizeof(statisticForAreaSolid.numOfClosedPolygons)))
    {
        return false;
    }
    if (!AppendDataToMemoryblock(memoryBlock, offset, &statisticForAreaSolid.numOfOpenPolygons, sizeof(statisticForAreaSolid.numOfOpenPolygons)))
    {
        return false;
    }

    for (int i = 0; i < iPolygonSize; ++i)
    {
        CD::FlexibleMesh mesh;
        PolygonDecomposer decomposer;
        decomposer.TriangulatePolygon(optimizedPolygons[i], mesh);
        if (!ExportPolygonForAreaSolid(memoryBlock, offset, mesh.vertexList, mesh.faceList))
        {
            return false;
        }
    }

    for (int i = 0; i < iPolygonSize; ++i)
    {
        PolygonPtr pPolygon(optimizedPolygons[i]);
        std::vector<PolygonPtr> innerPolygons;
        pPolygon->GetSeparatedPolygons(innerPolygons, eSP_InnerHull);
        for (int k = 0, iSize(innerPolygons.size()); k < iSize; ++k)
        {
            innerPolygons[k]->ReverseEdges();
            if (pModel->QueryEquivalentPolygon(innerPolygons[k]))
            {
                continue;
            }
            FlexibleMesh mesh;
            PolygonDecomposer decomposer;
            decomposer.TriangulatePolygon(innerPolygons[k], mesh);
            if (!ExportPolygonForAreaSolid(memoryBlock, offset, mesh.vertexList, mesh.faceList))
            {
                return false;
            }
        }
    }

    assert(statisticForAreaSolid.totalSize == offset);

    QString sRealGeomFileName;
    pAreaSolid->GenerateGameFilename(sRealGeomFileName);
    sRealGeomFileName.replace("%level%", Path::ToUnixPath(Path::RemoveBackslash(path)));

    pakFile.UpdateFile(sRealGeomFileName.toLocal8Bit(), memoryBlock, true, ICryArchive::LEVEL_FASTER);
    return true;
}

bool Exporter::ExportClipVolume(const QString& path, ClipVolumeObject* pClipVolume, CPakFile& pakFile)
{
    if (!pClipVolume)
    {
        return false;
    }

    if (CD::ModelCompiler* pCompiler = pClipVolume->GetCompiler())
    {
        _smart_ptr<IStatObj> pStatObj;
        if (pCompiler->GetIStatObj(&pStatObj))
        {
            UpdateStatObjWithoutBackFaces(pClipVolume);

            QString sRealGeomFileName = pClipVolume->GenerateGameFilename();
            sRealGeomFileName.replace("%level%", Path::ToUnixPath(Path::RemoveBackslash(path)));

            IChunkFile* pChunkFile = NULL;
            if (pStatObj->SaveToCGF(sRealGeomFileName.toLocal8Bit(), &pChunkFile))
            {
                pClipVolume->ExportBspTree(pChunkFile);

                void* pMemFile = NULL;
                int nFileSize = 0;
                pChunkFile->WriteToMemoryBuffer(&pMemFile, &nFileSize);
                pakFile.UpdateFile(sRealGeomFileName.toLocal8Bit(), pMemFile, nFileSize, true, ICryArchive::LEVEL_FASTER);
                pChunkFile->Release();
            }

            UpdateStatObj(pClipVolume);

            return true;
        }
    }

    return false;
}

int ComputeFaceSize(const std::vector<SMeshFace>& faceList)
{
    int totalSize(0);
    for (int k = 0, iFaceSize(faceList.size()); k < iFaceSize; ++k)
    {
        totalSize += sizeof(int);
        totalSize += 3 * sizeof(Vec3);
    }
    return totalSize;
}

void Exporter::ComputeAreaSolidMemoryStatistic(AreaSolidObject* pAreaSolid, SAreaSolidStatistic& outStatistic, std::vector<PolygonPtr>& optimizedPolygons)
{
    if (pAreaSolid == NULL)
    {
        return;
    }
    CD::ModelCompiler* pCompiler(pAreaSolid->GetCompiler());
    if (pCompiler == NULL)
    {
        return;
    }
    Model* pModel(pAreaSolid->GetModel());
    if (pModel == NULL)
    {
        return;
    }
    int iPolygonSize = optimizedPolygons.size();

    memset(&outStatistic, 0, sizeof(outStatistic));

    outStatistic.totalSize += sizeof(outStatistic.numOfClosedPolygons);
    outStatistic.totalSize += sizeof(outStatistic.numOfOpenPolygons);

    for (int i = 0; i < iPolygonSize; ++i)
    {
        CD::FlexibleMesh mesh;
        PolygonDecomposer decomposer;
        decomposer.TriangulatePolygon(optimizedPolygons[i], mesh);

        outStatistic.numOfClosedPolygons += mesh.faceList.size();
        outStatistic.totalSize += ComputeFaceSize(mesh.faceList);
    }

    for (int i = 0; i < iPolygonSize; ++i)
    {
        PolygonPtr pPolygon(optimizedPolygons[i]);
        std::vector<PolygonPtr> innerPolygons;
        pPolygon->GetSeparatedPolygons(innerPolygons, eSP_InnerHull);
        for (int k = 0, iSize(innerPolygons.size()); k < iSize; ++k)
        {
            if (pModel->QueryEquivalentPolygon(innerPolygons[k]))
            {
                continue;
            }
            innerPolygons[k]->ReverseEdges();

            CD::FlexibleMesh mesh;
            PolygonDecomposer decomposer;
            decomposer.TriangulatePolygon(innerPolygons[k], mesh);

            outStatistic.numOfOpenPolygons += mesh.faceList.size();
            outStatistic.totalSize += ComputeFaceSize(mesh.faceList);
        }
    }
}