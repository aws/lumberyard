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
#pragma once

#include <string>
#include <vector>

#include <NvBlastExtAuthoringTypes.h>

class GU_Detail;
class OP_Context;

struct NvBlastChunkDesc;

namespace BlastHelpers
{
    struct ChunkInfos
    {
        std::vector<std::vector<Nv::Blast::Triangle>> chunkTriangles;
        std::vector<NvBlastChunkDesc> chunkDescs;
        std::vector<std::string> chunkNames;
        std::vector<bool> chunkIsStatic;
    };

    bool createChunkInfos(GU_Detail* detail, OP_Context& context, const std::string& rootName, ChunkInfos& outChunkInfos, std::vector<std::string>& errors);
}
