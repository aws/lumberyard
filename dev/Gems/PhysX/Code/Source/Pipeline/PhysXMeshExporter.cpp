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

#include <Pipeline/PhysXMeshExporter.h>
#include <Pipeline/PhysXMeshAsset.h>
#include <Pipeline/PhysXMeshGroup.h>

#include <AzCore/IO/FileIO.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>
#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

#include <PxPhysicsAPI.h>

// A utility macro helping set/clear bits in a single line
#define SET_BITS(flags, condition, bits) flags = (condition) ? ((flags) | (bits)) : ((flags) & ~(bits))

using namespace AZ::SceneAPI;

namespace PhysX
{
    namespace Pipeline
    {
        namespace SceneContainers = AZ::SceneAPI::Containers;
        namespace SceneEvents = AZ::SceneAPI::Events;
        namespace SceneUtil = AZ::SceneAPI::Utilities;

        static physx::PxDefaultAllocator pxDefaultAllocatorCallback;

        // Error code conversion functions
        static AZStd::string convexCookingResultToString(physx::PxConvexMeshCookingResult::Enum convexCookingResultCode)
        {
            static const AZStd::string resultToString[] = { "eSUCCESS", "eZERO_AREA_TEST_FAILED", "ePOLYGONS_LIMIT_REACHED", "eFAILURE" };
            if (AZ_ARRAY_SIZE(resultToString) > convexCookingResultCode)
            {
                return resultToString[convexCookingResultCode];
            }
            else
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Unknown convex cooking result code: %i", convexCookingResultCode);
                return "";
            }
        }

        static AZStd::string trimeshCookingResultToString(physx::PxTriangleMeshCookingResult::Enum triangleCookingResultCode)
        {
            static const AZStd::string resultToString[] = { "eSUCCESS", "eLARGE_TRIANGLE", "eFAILURE" };
            if (AZ_ARRAY_SIZE(resultToString) > triangleCookingResultCode)
            {
                return resultToString[triangleCookingResultCode];
            }
            else
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Unknown trimesh cooking result code: %i", triangleCookingResultCode);
                return "";
            }
        }

        /**
        * Implementation of the PhysX error callback interface directing errors to ErrorWindow output.
        */
        static class PxExportErrorCallback
            : public physx::PxErrorCallback
        {
        public:
            virtual void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line)
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "PxErrorCode %i: %s (line %i in %s)", code, message, line, file);
            }
        } pxDefaultErrorCallback;

        PhysXMeshExporter::PhysXMeshExporter()
        {
            BindToCall(&PhysXMeshExporter::ProcessContext);
        }

        void PhysXMeshExporter::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<PhysXMeshExporter, AZ::SceneAPI::SceneCore::ExportingComponent>()->Version(1);
            }
        }


        AZ::SceneAPI::Events::ProcessingResult PhysXMeshExporter::ExportMeshObject(AZ::SceneAPI::Events::ExportEventContext& context, const AZStd::shared_ptr<const AZ::SceneAPI::DataTypes::IMeshData>& meshToExport, const AZStd::string& nodePath, const Pipeline::PhysXMeshGroup& pxMeshGroup) const
        {
            SceneEvents::ProcessingResult result;

            auto vertexCount = meshToExport->GetVertexCount();

            AZ::Vector3* verts = new AZ::Vector3[vertexCount];
            for (unsigned int i = 0; i < vertexCount; ++i)
            {
                verts[i] = meshToExport->GetPosition(i);
            }

            auto faceCount = meshToExport->GetFaceCount();
            AZ::SceneAPI::DataTypes::IMeshData::Face* faces = new AZ::SceneAPI::DataTypes::IMeshData::Face[faceCount];

            for (unsigned int i = 0; i < faceCount; ++i)
            {
                faces[i] = meshToExport->GetFaceInfo(i);
            }

            auto pxFoundation = PxCreateFoundation(PX_FOUNDATION_VERSION, pxDefaultAllocatorCallback, pxDefaultErrorCallback);
            if (pxFoundation)
            {
                bool shouldExportAsConvex = pxMeshGroup.GetExportAsConvex();
                bool cookingSuccessful = false;
                AZStd::string cookingResultErrorCodeString;

                physx::PxCookingParams pxCookingParams = physx::PxCookingParams(physx::PxTolerancesScale());

                pxCookingParams.buildGPUData = pxMeshGroup.GetBuildGPUData();
                pxCookingParams.midphaseDesc.setToDefault(physx::PxMeshMidPhase::eBVH34); // Always set to 3.4 since 3.3 is being deprecated (despite being default)

                if (shouldExportAsConvex)
                {
                    if (pxMeshGroup.GetCheckZeroAreaTriangles())
                    {
                        pxCookingParams.areaTestEpsilon = pxMeshGroup.GetAreaTestEpsilon();
                    }

                    pxCookingParams.planeTolerance = pxMeshGroup.GetPlaneTolerance();
                    pxCookingParams.gaussMapLimit = pxMeshGroup.GetGaussMapLimit();
                }
                else
                {
                    pxCookingParams.midphaseDesc.mBVH34Desc.numTrisPerLeaf = pxMeshGroup.GetNumTrisPerLeaf();
                    pxCookingParams.meshWeldTolerance = pxMeshGroup.GetMeshWeldTolerance();
                    pxCookingParams.buildTriangleAdjacencies = pxMeshGroup.GetBuildTriangleAdjacencies();
                    pxCookingParams.suppressTriangleMeshRemapTable = pxMeshGroup.GetSuppressTriangleMeshRemapTable();

                    if (pxMeshGroup.GetWeldVertices())
                    {
                        pxCookingParams.meshPreprocessParams |= physx::PxMeshPreprocessingFlag::eWELD_VERTICES;
                    }

                    if (pxMeshGroup.GetDisableCleanMesh())
                    {
                        pxCookingParams.meshPreprocessParams |= physx::PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH;
                    }

                    if (pxMeshGroup.GetForce32BitIndices())
                    {
                        pxCookingParams.meshPreprocessParams |= physx::PxMeshPreprocessingFlag::eFORCE_32BIT_INDICES;
                    }
                }

                auto pxCooking = PxCreateCooking(PX_PHYSICS_VERSION, *pxFoundation, pxCookingParams);
                if (pxCooking)
                {
                    physx::PxDefaultMemoryOutputStream memoryStream;

                    if (shouldExportAsConvex)
                    {
                        physx::PxConvexMeshDesc convexDesc;
                        convexDesc.points.count = vertexCount;
                        convexDesc.points.stride = sizeof(AZ::Vector3);
                        convexDesc.points.data = verts;
                        convexDesc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX;

                        SET_BITS(convexDesc.flags, pxMeshGroup.GetUse16bitIndices(), physx::PxConvexFlag::e16_BIT_INDICES);
                        SET_BITS(convexDesc.flags, pxMeshGroup.GetCheckZeroAreaTriangles(), physx::PxConvexFlag::eCHECK_ZERO_AREA_TRIANGLES);
                        SET_BITS(convexDesc.flags, pxMeshGroup.GetQuantizeInput(), physx::PxConvexFlag::eQUANTIZE_INPUT);
                        SET_BITS(convexDesc.flags, pxMeshGroup.GetUsePlaneShifting(), physx::PxConvexFlag::ePLANE_SHIFTING);
                        SET_BITS(convexDesc.flags, pxMeshGroup.GetBuildGPUData(), physx::PxConvexFlag::eGPU_COMPATIBLE);
                        SET_BITS(convexDesc.flags, pxMeshGroup.GetShiftVertices(), physx::PxConvexFlag::eSHIFT_VERTICES);

                        physx::PxConvexMeshCookingResult::Enum convexCookingResultCode = physx::PxConvexMeshCookingResult::eSUCCESS;
                        cookingSuccessful = pxCooking->cookConvexMesh(convexDesc, memoryStream, &convexCookingResultCode) && ValidateCookedConvexMesh(memoryStream.getData(), memoryStream.getSize());
                        cookingResultErrorCodeString = convexCookingResultToString(convexCookingResultCode);
                    }
                    else
                    {
                        physx::PxTriangleMeshDesc meshDesc;
                        meshDesc.points.count = vertexCount;
                        meshDesc.points.stride = sizeof(AZ::Vector3);
                        meshDesc.points.data = verts;

                        meshDesc.triangles.count = faceCount;
                        meshDesc.triangles.stride = sizeof(AZ::SceneAPI::DataTypes::IMeshData::Face);
                        meshDesc.triangles.data = faces;

                        physx::PxTriangleMeshCookingResult::Enum trimeshCookingResultCode = physx::PxTriangleMeshCookingResult::eSUCCESS;
                        cookingSuccessful = pxCooking->cookTriangleMesh(meshDesc, memoryStream, &trimeshCookingResultCode) && ValidateCookedTriangleMesh(memoryStream.getData(), memoryStream.getSize());
                        cookingResultErrorCodeString = trimeshCookingResultToString(trimeshCookingResultCode);
                    }

                    if (cookingSuccessful)
                    {
                        auto groupName = pxMeshGroup.GetName();

                        const AZ::SceneAPI::Containers::Scene& scene = context.GetScene();
                        const AZ::SceneAPI::Containers::SceneGraph& graph = scene.GetGraph();
                        auto nodeIndex = graph.Find(nodePath);
                        auto nodeName = AZStd::string(graph.GetNodeName(nodeIndex).GetName());
                        AZStd::string assetName = groupName + "_" + nodeName;

                        AZStd::string filename = SceneUtil::FileUtilities::CreateOutputFileName(assetName, context.GetOutputDirectory(), PhysXMeshAsset::m_assetFileExtention);

                        bool canStartWritingToFile = !filename.empty() && SceneUtil::FileUtilities::EnsureTargetFolderExists(filename);

                        if (canStartWritingToFile)
                        {
                            result = SceneEvents::ProcessingResult::Success;

                            WriteToFile(memoryStream.getData(), memoryStream.getSize(), filename, pxMeshGroup);

                            AZStd::string productUuidString;
                            productUuidString += pxMeshGroup.GetId().ToString<AZStd::string>();
                            productUuidString += nodePath;
                            AZ::Uuid productUuid = AZ::Uuid::CreateData(productUuidString.data(), productUuidString.size() * sizeof(productUuidString[0]));

                            context.GetProductList().AddProduct(AZStd::move(filename), productUuid, AZ::AzTypeInfo<PhysXMeshAsset>::Uuid());
                        }
                        else
                        {
                            AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Unable to create a file for a PhysX mesh asset.");
                            result = SceneEvents::ProcessingResult::Failure;
                        }
                    }
                    else
                    {
                        AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Cooking Mesh failed: %s", cookingResultErrorCodeString);
                        result = SceneEvents::ProcessingResult::Failure;
                    }

                    pxCooking->release();
                }
                else
                {
                    AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Creating PxCreateCooking failed!");
                    result = SceneEvents::ProcessingResult::Failure;
                }

                pxFoundation->release();
            }
            else
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Creating PxCreateFoundation failed!");
                result = SceneEvents::ProcessingResult::Failure;
            }

            return result;
        }

        SceneEvents::ProcessingResult PhysXMeshExporter::ProcessContext(SceneEvents::ExportEventContext& context) const
        {
            AZ_TraceContext("Exporter", "PhysX");

            SceneEvents::ProcessingResultCombiner result;

            const AZ::SceneAPI::Containers::Scene& scene = context.GetScene();
            const AZ::SceneAPI::Containers::SceneGraph& graph = scene.GetGraph();

            const SceneContainers::SceneManifest& manifest = context.GetScene().GetManifest();

            auto valueStorage = manifest.GetValueStorage();
            auto view = SceneContainers::MakeExactFilterView<PhysXMeshGroup>(valueStorage);

            for (const PhysXMeshGroup& pxMeshGroup : view)
            {
                auto groupName = pxMeshGroup.GetName();

                AZ_TraceContext("Group Name", groupName);

                const auto& sceneNodeSelectionList = pxMeshGroup.GetSceneNodeSelectionList();

                auto selectedNodeCount = sceneNodeSelectionList.GetSelectedNodeCount();

                for (size_t i = 0; i < selectedNodeCount; i++)
                {
                    const auto& selectedNodePath = sceneNodeSelectionList.GetSelectedNode(i);
                    auto nodeIndex = graph.Find(selectedNodePath);
                    auto it = graph.ConvertToStorageIterator(nodeIndex);
                    auto mesh = azrtti_cast<const AZ::SceneAPI::DataTypes::IMeshData*>(*it);

                    if (mesh)
                    {
                        result += ExportMeshObject(context, mesh, selectedNodePath, pxMeshGroup);
                    }
                }
            }

            return result.GetResult();
        }

        void PhysXMeshExporter::WriteToFile(const void* assetData, AZ::u32 assetDataSize, const AZStd::string& filename, const Pipeline::PhysXMeshGroup& pxMeshGroup) const
        {
            const AZ::u32 assetVersion = 1;
            const AZ::u8 isConvexMesh = pxMeshGroup.GetExportAsConvex() ? 1 : 0;

            AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
            if (AZ::IO::FileIOBase::GetInstance()->Open(filename.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary, fileHandle))
            {
                // Write the header
                AZ::IO::FileIOBase::GetInstance()->Write(fileHandle, &assetVersion, sizeof(assetVersion));
                AZ::IO::FileIOBase::GetInstance()->Write(fileHandle, &isConvexMesh, sizeof(isConvexMesh));
                AZ::IO::FileIOBase::GetInstance()->Write(fileHandle, &assetDataSize, sizeof(assetDataSize));
                AZ::IO::FileIOBase::GetInstance()->Write(fileHandle, assetData, assetDataSize);

                AZ::IO::FileIOBase::GetInstance()->Close(fileHandle);
            }
        }

        bool PhysXMeshExporter::ValidateCookedTriangleMesh(void* assetData, AZ::u32 assetDataSize) const
        {
            auto pxPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, PxGetFoundation(), physx::PxTolerancesScale(), false);

            physx::PxDefaultMemoryInputData inpStream(static_cast<physx::PxU8*>(assetData), assetDataSize);
            physx::PxTriangleMesh* triangleMesh = pxPhysics->createTriangleMesh(inpStream);

            pxPhysics->release();

            return triangleMesh != nullptr;
        }

        bool PhysXMeshExporter::ValidateCookedConvexMesh(void* assetData, AZ::u32 assetDataSize) const
        {
            auto pxPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, PxGetFoundation(), physx::PxTolerancesScale(), false);

            physx::PxDefaultMemoryInputData inpStream(static_cast<physx::PxU8*>(assetData), assetDataSize);
            physx::PxConvexMesh* convexMesh = pxPhysics->createConvexMesh(inpStream);

            pxPhysics->release();

            return convexMesh != nullptr;
        }
    }
}
