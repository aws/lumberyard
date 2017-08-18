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

#include "Serialization.h"

namespace SourceAsset
{
    struct SkeletonImport
    {
        string nodeName;

        void Serialize(IArchive& ar)
        {
            if (ar.IsEdit() && ar.IsOutput())
            {
                ar(nodeName, "node", "^!");
            }
            ar(nodeName, "nodeName", "Node Name");
        }
    };
    typedef vector<SkeletonImport> SkeletonImports;

    struct MeshImport
    {
        string nodeName;

        bool use8Weights;
        bool fp32Precision;

        MeshImport()
            : use8Weights(false)
            , fp32Precision(false)
        {
        }

        void Serialize(IArchive& ar)
        {
            if (ar.IsEdit() && ar.IsOutput())
            {
                ar(nodeName, "node", "^!");
            }
            ar(nodeName, "nodeName", "Node Name");
            ar(use8Weights, "use8Weights", "Use 8 Joint Weights");
            ar(fp32Precision, "fp32Precision", "High Precision Positions (FP32)");
        }
    };
    typedef vector<MeshImport> MeshImports;

    struct Settings
    {
        MeshImports meshes;
        SkeletonImports skeletons;


        void Serialize(IArchive& ar)
        {
            ar(meshes, "meshes", "+Meshes");
            ar(skeletons, "skeletons", "+Skeletons");
        }

        bool UsedAsMesh(const char* nodeName) const
        {
            for (size_t i = 0; i < meshes.size(); ++i)
            {
                if (meshes[i].nodeName == nodeName)
                {
                    return true;
                }
            }
            return false;
        }

        bool UsedAsSkeleton(const char* nodeName) const
        {
            for (size_t i = 0; i < skeletons.size(); ++i)
            {
                if (skeletons[i].nodeName == nodeName)
                {
                    return true;
                }
            }
            return false;
        }
    };
}
