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
#include <BlastHelpers.h>

#include <GA/GA_Detail.h>
#include <GA/GA_Primitive.h>
#include <GU/GU_Detail.h>
#include <OP/OP_Director.h>
#include <OP/OP_Network.h>
#include <SOP/SOP_Node.h>
#include <UT/UT_StringHolder.h>

#include <NvBlast.h>
#include <PxVec3.h>

#include <algorithm>
#include <cctype>
#include <unordered_map>

namespace BlastHelpers
{
    bool createChunkInfos(GU_Detail* detail, OP_Context& context, const std::string& rootName, ChunkInfos& outChunkInfos, std::vector<std::string>& errors)
    {
        if (detail == nullptr)
        {
            errors.emplace_back("Null detail node from parent SOP node");
            return false;
        }

        // We rely upon chunks having the naming format:
        // <parent name>_<child name>
        // This should guarantee that when sorted we see parent nodes before child nodes
        // We also require the root to be named "root" to avoid confusing with other non parented groups

        // Map to store names of chunks to their indices for parent index lookup
        std::unordered_map<std::string, int> parentNamesToIndices;

        GA_PrimitiveGroup* primitiveGroup = nullptr;
        GA_FOR_ALL_PRIMGROUPS_SORTED(detail, primitiveGroup)
        {
            if (primitiveGroup == nullptr)
            {
                errors.emplace_back("Null group found");
                return false;
            }

            std::string name = primitiveGroup->getName().toStdString();
            std::string loweredRootName = rootName;
            
            // convert to lower case for ease of comparisons
            std::transform(name.begin(), name.end(), name.begin(), std::tolower);
            std::transform(loweredRootName.begin(), loweredRootName.end(), loweredRootName.begin(), std::tolower);
            const auto separatorIndex = name.find_last_of('_');
            int parentIndex = -1;

            if (separatorIndex == std::string::npos)
            {
                if (name != loweredRootName)
                {
                    // skip other miscellaneous groups like inside & outside
                    continue;
                }
            }
            else
            {
                // Find the parent node from our map
                std::string parentName = name.substr(0, separatorIndex);
                auto parentIt = parentNamesToIndices.find(parentName);
                if (parentIt != parentNamesToIndices.end())
                {
                    parentIndex = parentIt->second;

                    if (parentIndex < 0 || parentIndex >= outChunkInfos.chunkDescs.size())
                    {
                        std::string errorMessage = "Invalid parent index for chunk ";
                        errorMessage += name;
                        errors.emplace_back(std::move(errorMessage));
                        return false;
                    }

                    // Clear the support flag, as we currently make all leaf nodes supports.
                    outChunkInfos.chunkDescs[parentIndex].flags = NvBlastChunkDesc::Flags::NoFlags;
                }
                else
                {
                    // This node is not the root and we couldn't find a parent so the data must be malformatted
                    std::string errorMessage = "Unable to find parent for node ";
                    errorMessage += name;
                    errors.emplace_back(std::move(errorMessage));
                    return false;
                }
            }

            // Add to each of our output arrays
            size_t index = outChunkInfos.chunkDescs.size();
            outChunkInfos.chunkDescs.emplace_back();
            auto& chunkDesc = outChunkInfos.chunkDescs.back();
            outChunkInfos.chunkTriangles.emplace_back();
            auto& chunkTriangles = outChunkInfos.chunkTriangles.back();
            outChunkInfos.chunkNames.emplace_back(name);
            outChunkInfos.chunkIsStatic.emplace_back(false);

            // check if static is in our name, but not our parent's - case sensitive
            const auto staticIndex = name.find("static", separatorIndex);
            if (staticIndex != std::string::npos)
            {
                outChunkInfos.chunkIsStatic.back() = true;
            }

            auto insertResult = parentNamesToIndices.emplace(name, static_cast<int>(index));
            if (!insertResult.second)
            {
                std::string errorMessage = "Found two nodes with the same name: ";
                errorMessage += name;
                errors.emplace_back(std::move(errorMessage));
                return false;
            }

            chunkDesc.parentChunkIndex = static_cast<uint32_t>(parentIndex);
            chunkDesc.flags = NvBlastChunkDesc::Flags::SupportFlag;

            // Convert prim group to triangle mesh
            const int maxVertices = 3;
            const bool flipEdges = true;
            const bool avoidDegeneracy = false;
            detail->convex(maxVertices, primitiveGroup, nullptr, flipEdges, avoidDegeneracy);

            const GA_Primitive* primitive;
            physx::PxVec3 chunkCentroid(0, 0, 0);
            GA_FOR_ALL_GROUP_PRIMITIVES(detail, primitiveGroup, primitive)
            {
                if (primitive == nullptr)
                {
                    errors.emplace_back("Null primitive found");
                    return false;
                }

                GA_Size vertexCount = primitive->getVertexCount();
                if (vertexCount != 3)
                {
                    std::string errorMessage = "Triangulation failed on chunk: ";
                    errorMessage += name;
                    errors.emplace_back(std::move(errorMessage));
                    return false;
                }

                auto convertToLYCoords = [](const UT_Vector3& pos) -> physx::PxVec3
                    {
                        return physx::PxVec3(-pos.x(), pos.z(), pos.y());
                    };
                const physx::PxVec3 posA = convertToLYCoords(primitive->getPos3(0));
                const physx::PxVec3 posB = convertToLYCoords(primitive->getPos3(1));
                const physx::PxVec3 posC = convertToLYCoords(primitive->getPos3(2));

                chunkCentroid += posA + posB + posC;

                chunkTriangles.emplace_back();
                Nv::Blast::Triangle& triangle = chunkTriangles.back();

                auto convertToNvcVec3 = [](const physx::PxVec3& pos)
                    {
                        return NvcVec3{ pos.x, pos.y, pos.z };
                    };
                triangle.a.p = convertToNvcVec3(posA);
                triangle.b.p = convertToNvcVec3(posB);
                triangle.c.p = convertToNvcVec3(posC);
            }

            if (chunkTriangles.empty())
            {
                std::string errorMessage = "Found chunk with no geometry: ";
                errorMessage += name;
                errors.emplace_back(std::move(errorMessage));
                return false;
            }

            chunkCentroid /= chunkTriangles.size() * 3;
            chunkDesc.centroid[0] = chunkCentroid.x;
            chunkDesc.centroid[1] = chunkCentroid.y;
            chunkDesc.centroid[2] = chunkCentroid.z;
        }

        if (outChunkInfos.chunkDescs.empty())
        {
            errors.emplace_back("No chunks were found in the input");
            return false;
        }

        return true;
    }
}
