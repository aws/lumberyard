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

namespace Serialization {
    class IArchive;
}

namespace SourceAsset
{
    using std::vector;
    using Serialization::IArchive;

    struct Node
    {
        string name;
        int mesh;
        vector<int> children;

        Node()
            : mesh(-1)
        {
        }

        void Serialize(IArchive& ar)
        {
            ar(name, "name", "^");
            ar(mesh, "mesh", "Mesh");
            ar(children, "children", "Children");
        }
    };
    typedef vector<Node> Nodes;

    struct Mesh
    {
        string name;

        void Serialize(IArchive& ar)
        {
            ar(name, "name", "^");
        }
    };
    typedef vector<Mesh> Meshes;

    struct Scene
    {
        Nodes nodes;
        Meshes meshes;

        void Serialize(IArchive& ar)
        {
            ar(nodes, "nodes", "Nodes");
            ar(meshes, "meshes", "Meshes");
        }
    };
}
