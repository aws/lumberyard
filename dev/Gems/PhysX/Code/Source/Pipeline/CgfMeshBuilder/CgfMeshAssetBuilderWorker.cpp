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

#include <PhysX_precompiled.h>

#include <Source/Pipeline/CgfMeshBuilder/CgfMeshAssetBuilderWorker.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Physics/Material.h>
#include <PhysX/MeshAsset.h>
#include <Source/Utils.h>
#include <Source/Pipeline/MeshAssetHandler.h>
#include <Source/Pipeline/MeshGroup.h>

#include <QUuid>

// Guid needs to be included after QUuid to enable QUuid <-> GUID conversions
#include <AzCore/Math/Guid.h>
#include <AzCore/Math/Uuid.h>

// All the cry headers needed for parsing CGF file, as well as several cpp files, which is hacky, 
// but not as hacky as any alternative feasible in reasonable time
// CGF Parser is deeply intertwined with CryEngine and it's impossible to use it without 
// dragging multiple modules with it
#include <platform_impl.h>
#include <CryHeaders.h>
#include <CryHeaders_info.h>
#include <Common_TypeInfo.h>
#include <IIndexedMesh_info.h>
#include <CGFContent_info.h>
#undef FUNCTION_PROFILER_3DENGINE
#undef LOADING_TIME_PROFILE_SECTION
#undef ENABLE_TYPE_INFO_NAMES
#include <../../../../Code/CryEngine/Cry3DEngine/CGF/ChunkFile.cpp>
#include <../../../../Code/CryEngine/Cry3DEngine/CGF/ChunkFileReaders.cpp>
#include <../../../../Code/CryEngine/Cry3DEngine/CGF/ChunkFileWriters.cpp>
#include <../../../../Code/CryEngine/Cry3DEngine/MeshCompiler/VertexCacheOptimizer.cpp>
#include <../../../../Code/CryEngine/Cry3DEngine/CGF/CGFLoader.cpp>

void RCLog(const char* szFormat, ...){}
void RCLogWarning(const char* szFormat, ...){}
void RCLogError(const char* szFormat, ...){}
void RCLogContext(const char* szMessage){}

#define CGF_PHYSX_COMPILER
#include <RC/ResourceCompilerPC/PhysWorld.h>
#include <../../../../Code/CryEngine/CryPhysics/geoman.h>

// use the cook function from meshexporter.cpp
namespace PhysX
{
    namespace Pipeline
    {
        bool CookPhysxTriangleMesh(
            const AZStd::vector<Vec3>& vertices,
            const AZStd::vector<AZ::u32>& indices,
            const AZStd::vector<AZ::u16>& faceMaterials,
            AZStd::vector<AZ::u8>* output,
            const MeshGroup& pxMeshGroup
        );
    }
}

namespace PhysX
{
    namespace Pipeline
    {
        class CgfReaderErrorsListener
            : public ILoaderCGFListener
        {
        public:
            virtual void Warning(const char* format)
            {
                AZ_TracePrintf(AssetBuilderSDK::WarningWindow, "%s", format);
                m_bLoadingWarnings = true;
            }
            virtual void Error(const char* format)
            {
                AZ_Error(AssetBuilderSDK::ErrorWindow, false, "%s", format);
                m_bLoadingErrors = true;
            }

        public:
            bool m_bLoadingErrors = false;
            bool m_bLoadingWarnings = false;
        };

        static const char* physxGemAssetJobKey = "PhysX mesh";

        void CgfMeshAssetBuilderWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse & response)
        {
            if (m_isShuttingDown)
            {
                response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
                return;
            }

            AZStd::string ext;
            AzFramework::StringFunc::Path::GetExtension(request.m_sourceFile.c_str(), ext, false);

            if (AzFramework::StringFunc::Equal(ext.c_str(), "cgf"))
            {
                for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
                {
                    AssetBuilderSDK::JobDescriptor descriptor;
                    descriptor.m_jobKey = physxGemAssetJobKey;
                    descriptor.SetPlatformIdentifier(platformInfo.m_identifier.c_str());

                    response.m_createJobOutputs.push_back(descriptor);
                }

                response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
                return;
            }
        }

        static void ConvertCryPhysToPhysx(CNodeCGF* pNode, AZStd::vector<Vec3>* vertices, AZStd::vector<AZ::u32>* indices, AZStd::vector<AZ::u16>* faceMaterials, AZStd::vector<Physics::MaterialConfiguration>* materials)
        {
            CPhysWorldLoader physLoader;
            IPhysicalWorld* pPhysWorld = physLoader.GetWorldPtr();
            AZ_Assert(pPhysWorld, "CGF to PhysX cooking: No CryPhysics world");
            IGeomManager* geomManager = pPhysWorld->GetGeomManager();
            AZ_Assert(geomManager, "CGF to PhysX cooking: No IGeomManager");

            for (int nPhysGeomType = 0; nPhysGeomType < 4; nPhysGeomType++)
            {
                DynArray<char>& physData = pNode->physicalGeomData[nPhysGeomType];
                if (physData.empty())
                {
                    continue;
                }

                phys_geometry* pPhysGeom = 0;
                CMemStream stmIn(&physData.front(), physData.size(), true);
                if (pNode->pMesh)
                {
                    CMesh& mesh = *pNode->pMesh;
                    AZ_Assert(mesh.m_pPositionsF16 == 0, "CGF to PhysX cooking: The mesh has 16-bit positions");
                    pPhysGeom = geomManager->LoadPhysGeometry(stmIn, mesh.m_pPositions, mesh.m_pIndices, nullptr);
                }
                else
                {
                    pPhysGeom = geomManager->LoadPhysGeometry(stmIn, 0, 0, 0);
                }

                int geomType = pPhysGeom->pGeom->GetType();
                if (geomType == GEOM_TRIMESH)
                {
                    const mesh_data* geomMeshData = static_cast<const mesh_data*>(pPhysGeom->pGeom->GetData());
                    int vertsNum = geomMeshData->nVertices;
                    int facesNum = geomMeshData->nTris;

                    // cryphsics has a full set of all the vertices in the mesh, and an index buffer that references a small number of these.
                    // this is a bug in cryphsics.  Workaround: Use only the referenced verts.
                    AZStd::vector<AZ::u32> remap(vertsNum, 0);
                    int filteredVertexNum = 0;
                    for (int i = 0; i < facesNum*3; ++i)
                    {
                        index_t idx = geomMeshData->pIndices[i];
                        if (remap[idx] == 0)
                        {
                            remap[idx] = 1;
                            filteredVertexNum++;
                        }
                    }

                    // compact the vertex buffer and write the remap index.
                    // Doing this as 2 steps keeps them in the same order as the original data
                    AZStd::vector<Vec3> filteredVertices(filteredVertexNum);
                    int filteredVertexCurr = 0;
                    for (int i = 0; i < vertsNum; ++i)
                    {
                        if (remap[i] != 0)
                        {
                            filteredVertices[filteredVertexCurr] = geomMeshData->pVertices[i];
                            remap[i] = filteredVertexCurr;
                            filteredVertexCurr++;
                        }
                    }
                    AZ_Assert(filteredVertexCurr == filteredVertexNum, "CGF to PhysX cooking: Error while remapping verticies");

                    // append the remapped vertices
                    AZ::u32 vertOffset = vertices->size();
                    vertices->resize(vertices->size() + filteredVertexNum);
                    for (int i = 0; i < filteredVertexNum; ++i)
                    {
                        (*vertices)[vertOffset + i] = filteredVertices[i];
                    }

                    // append the remapped indices
                    AZ::u32 indexOffset = indices->size();
                    indices->resize(indices->size() + facesNum*3);
                    for (int i = 0; i < facesNum * 3; ++i)
                    {
                        index_t idx = geomMeshData->pIndices[i];
                        AZ::u32 remapIdx = remap[idx];
                        (*indices)[indexOffset + i] = vertOffset + remapIdx;
                    }

                    // update material library
                    AZ::u16 materialIndex = static_cast<AZ::u16>(materials->size());
                    Physics::MaterialConfiguration materialEntry;
                    if (pNode->pMaterial != nullptr)
                    {
                        // @KB TODO: additional material data
                        materialEntry.m_surfaceType = pNode->pMaterial->name;
                        //materialEntry.SetDynamicFriction(0.f);
                        //materialEntry.SetRestitution(0.f);
                    }
                    materials->push_back(materialEntry);

                    // append material info for each triangle
                    // I'm unsure from the data if these materials need to be iterated from the geomMeshData, and multiple materials inserted into the library.  This may need updating if that is the case.
                    faceMaterials->resize(faceMaterials->size() + facesNum, materialIndex); // One entry per triangle
                }
                else if (geomType == GEOM_BOX)
                {
                    // create mesh geometry from box primitive.  It would be better to export this as a primitive type instead.

                    const primitives::box* boxData = static_cast<const primitives::box*>(pPhysGeom->pGeom->GetData());

                    Vec3 boxVerts[8] = {
                        Vec3(-boxData->size.x, -boxData->size.y, +boxData->size.z),
                        Vec3(+boxData->size.x, -boxData->size.y, +boxData->size.z),
                        Vec3(+boxData->size.x, +boxData->size.y, +boxData->size.z),
                        Vec3(-boxData->size.x, +boxData->size.y, +boxData->size.z),
                        Vec3(-boxData->size.x, -boxData->size.y, -boxData->size.z),
                        Vec3(+boxData->size.x, -boxData->size.y, -boxData->size.z),
                        Vec3(+boxData->size.x, +boxData->size.y, -boxData->size.z),
                        Vec3(-boxData->size.x, +boxData->size.y, -boxData->size.z)
                    };
                    AZ::u32 boxIndices[36] = {
                        1, 0, 4,   1, 4, 5,
                        2, 1, 5,   2, 5, 6,
                        3, 2, 6,   3, 6, 7,
                        0, 3, 7,   0, 7, 4,
                        0, 1, 3,   3, 1, 2,     // top
                        5, 4, 7,   5, 7, 6      // bottom
                    };

                    // apply orientation
                    if (boxData->bOriented)
                    {
                        for (int i = 0; i < 8; ++i)
                        {
                            boxVerts[i] = boxVerts[i] * boxData->Basis;
                        }
                    }

                    // append the remapped vertices
                    AZ::u32 vertOffset = vertices->size();
                    vertices->resize(vertices->size() + 8);
                    for (int i = 0; i < 8; ++i)
                    {
                        (*vertices)[vertOffset + i] = boxData->center + boxVerts[i];
                    }

                    // append the remapped indices
                    AZ::u32 indexOffset = indices->size();
                    indices->resize(indices->size() + 36);
                    for (int i = 0; i < 36; ++i)
                    {
                        (*indices)[indexOffset + i] = vertOffset + boxIndices[i];
                    }

                    // update material library
                    AZ::u16 materialIndex = static_cast<AZ::u16>(materials->size());
                    Physics::MaterialConfiguration materialEntry;
                    if (pNode->pMaterial != nullptr)
                    {
                        // @KB TODO: additional material data
                        materialEntry.m_surfaceType = pNode->pMaterial->name;
                        //materialEntry.SetDynamicFriction(0.f);
                        //materialEntry.SetRestitution(0.f);
                    }
                    materials->push_back(materialEntry);

                    // append material info for each triangle
                    // I'm unsure from the data if these materials need to be iterated from the geomMeshData, and multiple materials inserted into the library.  This may need updating if that is the case.
                    faceMaterials->resize(faceMaterials->size() + 12, materialIndex); // One entry per triangle
                }
                else
                {
                    AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Discovered physicalized node of other then GEOM_TRIMESH type, skipping.\n");
                }
            }
        }

        static bool ConvertCGFToPhysX(const char* sourcePath, AZStd::vector<uint8_t>* cookedMeshData, AZStd::vector<Physics::MaterialConfiguration>* materials)
        {
            CChunkFile chunkFile;
            CLoaderCGF cgfLoader;
            CgfReaderErrorsListener listener;

            std::unique_ptr<CContentCGF> pCGF;
            pCGF.reset(cgfLoader.LoadCGF(sourcePath, chunkFile, &listener));

            if (pCGF == nullptr || listener.m_bLoadingErrors)
            {
                AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Physics Cook: Failed to load geometry file %s - %s\n", sourcePath, cgfLoader.GetLastError());
                return false;
            }

            if (pCGF->GetConsoleFormat())
            {
                AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Physics Cook: Cannot cook geometry file %s because it's in console format.\n", sourcePath);
                return false;
            }

            AZStd::vector<Vec3> vertices; // don't use AZ::Vector3, it includes padding. Vec3 does not have padding.
            AZStd::vector<vtx_idx> indices;
            AZStd::vector<AZ::u16> faceMaterials;

            for (int32_t i = 0; i < pCGF->GetNodeCount(); ++i)
            {
                CNodeCGF* pNode = pCGF->GetNode(i);
                if (pNode)
                {
                    CMesh& mesh = *pNode->pMesh;
                    ConvertCryPhysToPhysx(pNode, &vertices, &indices, &faceMaterials, materials);
                }
            }

            if (vertices.size() == 0)
            {
                return true;    // no data found, skip generating th eobject
            }

            MeshGroup meshGroup;
            // cryphysics is duplicating vertices, using PhysX welding to remove duplicates
            meshGroup.SetMeshWeldTolerance(0.001f);
            meshGroup.SetWeldVertices(true);

            bool success = CookPhysxTriangleMesh(vertices, indices, faceMaterials, cookedMeshData, meshGroup);
            return success;
        }

        void CgfMeshAssetBuilderWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
        {
            AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);
            
            // Check if we are cancelled or shutting down before doing intensive processing on this source file
            if (jobCancelListener.IsCancelled())
            {
                AZ_TracePrintf(AssetBuilderSDK::WarningWindow, "Cancel was requested for job %s.\n", request.m_fullPath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                return;
            }


            if (request.m_jobDescription.m_jobKey == physxGemAssetJobKey)
            {
                AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Starting PhysX Gem mesh conversion.\n");
                CreatePhysXGemMeshAsset(request, response);
            }
        }

        void CgfMeshAssetBuilderWorker::CreatePhysXGemMeshAsset(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
        {
            // Check if there's already existing .pxmesh asset next to cgf. 
            AZStd::string pxMeshPath = request.m_fullPath;
            AzFramework::StringFunc::Path::ReplaceExtension(pxMeshPath, MeshAssetHandler::s_assetFileExtension);

            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            if (fileIO->Exists(pxMeshPath.c_str()))
            {
                AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "PhysX mesh has been already created for this cgf file (presumably by Legacy Convertor).\n");
                // Skip the processing if there's already existing pxmesh.
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
                return;
            }

            AZStd::vector<uint8_t> cookedMeshData;
            AZStd::vector<Physics::MaterialConfiguration> materials; // material data is ignored by .pxmesh files, but material indices are still stored in the cooked data
            if (!ConvertCGFToPhysX(request.m_fullPath.c_str(), &cookedMeshData, &materials))
            {
                AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Physics Cook: Cannot cook geometry file %s because it's in console format.\n", request.m_fullPath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }

            if (cookedMeshData.size() == 0)
            {
                AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "No physicalized nodes discovered, ending the job.\n");
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
                return;
            }

            AZStd::string fileName;
            AzFramework::StringFunc::Path::GetFullFileName(request.m_fullPath.c_str(), fileName);
            AzFramework::StringFunc::Path::StripExtension(fileName);

            AZStd::string destPath;
            AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), fileName.c_str(), MeshAssetHandler::s_assetFileExtension, destPath, true);

            MeshAssetCookedData assetFileContent;
            assetFileContent.m_isConvexMesh = false;
            assetFileContent.m_cookedPxMeshData = cookedMeshData;

            Utils::WriteCookedMeshToFile(destPath, assetFileContent);

            AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Writing cooked mesh to %s finished!\n", destPath.c_str());

            AzFramework::StringFunc::Path::GetFullFileName(destPath.c_str(), fileName);

            AssetBuilderSDK::JobProduct jobProduct(fileName);
            jobProduct.m_productAssetType = AZ::AzTypeInfo<MeshAsset>::Uuid();
            jobProduct.m_productSubID = AssetBuilderSDK::ConstructSubID(1, 0, 0);

            response.m_outputProducts.push_back(jobProduct);
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Job finished, returning success.!\n");
        }


        void CgfMeshAssetBuilderWorker::ShutDown()
        {
            m_isShuttingDown = true;
        }
    }
}