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

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <ProjectDefines.h>
#include <AzCore/Math/Vector2.h>
#include <foundation/PxVec4.h>

namespace nv
{
    namespace cloth
    {
        class Factory;
        class Solver;
        class Fabric;
        class Cloth;
    }
}

namespace NvCloth
{
    using MeshNodeList = AZStd::vector<AZStd::string>;

    using SimParticleType = physx::PxVec4;
    using SimIndexType = vtx_idx;
    using SimUVType = AZ::Vector2;

    // Defines deleters for NvCloth types to destroy them appropriately,
    // allowing to handle them with unique pointers.
    struct NvClothTypesDeleter
    {
        void operator()(nv::cloth::Factory* factory) const;
        void operator()(nv::cloth::Solver* solver) const;
        void operator()(nv::cloth::Fabric* fabric) const;
        void operator()(nv::cloth::Cloth* cloth) const;
    };

    using FactoryUniquePtr = AZStd::unique_ptr<nv::cloth::Factory, NvClothTypesDeleter>;
    using SolverUniquePtr = AZStd::unique_ptr<nv::cloth::Solver, NvClothTypesDeleter>;
    using FabricUniquePtr = AZStd::unique_ptr<nv::cloth::Fabric, NvClothTypesDeleter>;
    using ClothUniquePtr = AZStd::unique_ptr<nv::cloth::Cloth, NvClothTypesDeleter>;
} // namespace NvCloth
