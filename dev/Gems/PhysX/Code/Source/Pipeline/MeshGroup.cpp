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

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneData/Rules/MaterialRule.h>

#include <Source/Pipeline/MeshGroup.h>

#include <PxPhysicsAPI.h>

namespace PhysX
{
    namespace Pipeline
    {
        AZ_CLASS_ALLOCATOR_IMPL(MeshGroup, AZ::SystemAllocator, 0)

        MeshGroup::MeshGroup()
            : m_id(AZ::Uuid::CreateRandom())
        {
            physx::PxCookingParams defaultCookingParams = physx::PxCookingParams(physx::PxTolerancesScale());
            physx::PxConvexMeshDesc defaultconvexDesc;

            m_areaTestEpsilon = defaultCookingParams.areaTestEpsilon;

            m_planeTolerance = defaultCookingParams.planeTolerance;
            m_use16bitIndices = (static_cast<AZ::u32>(defaultconvexDesc.flags) & physx::PxConvexFlag::e16_BIT_INDICES) != 0;
            m_checkZeroAreaTriangles = (static_cast<AZ::u32>(defaultconvexDesc.flags) & physx::PxConvexFlag::eCHECK_ZERO_AREA_TRIANGLES) != 0;
            m_quantizeInput = (static_cast<AZ::u32>(defaultconvexDesc.flags) & physx::PxConvexFlag::eQUANTIZE_INPUT) != 0;
            m_usePlaneShifting = (static_cast<AZ::u32>(defaultconvexDesc.flags) & physx::PxConvexFlag::ePLANE_SHIFTING) != 0;
            m_shiftVertices = (static_cast<AZ::u32>(defaultconvexDesc.flags) & physx::PxConvexFlag::eSHIFT_VERTICES) != 0;
            m_gaussMapLimit = defaultCookingParams.gaussMapLimit;

            m_weldVertices = (static_cast<AZ::u32>(defaultCookingParams.meshPreprocessParams) & physx::PxMeshPreprocessingFlag::eWELD_VERTICES) != 0;
            m_disableCleanMesh = (static_cast<AZ::u32>(defaultCookingParams.meshPreprocessParams) & physx::PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH) != 0;
            m_force32BitIndices = (static_cast<AZ::u32>(defaultCookingParams.meshPreprocessParams) & physx::PxMeshPreprocessingFlag::eFORCE_32BIT_INDICES) != 0;
            m_suppressTriangleMeshRemapTable = defaultCookingParams.suppressTriangleMeshRemapTable;
            m_buildTriangleAdjacencies = defaultCookingParams.buildTriangleAdjacencies;
            m_meshWeldTolerance = defaultCookingParams.meshWeldTolerance;

            defaultCookingParams.midphaseDesc.setToDefault(physx::PxMeshMidPhase::eBVH34);
            m_numTrisPerLeaf = defaultCookingParams.midphaseDesc.mBVH34Desc.numTrisPerLeaf;
        }

        const AZStd::string& MeshGroup::GetName() const
        {
            return m_name;
        }

        void MeshGroup::SetName(const AZStd::string& name)
        {
            m_name = name;
        }

        void MeshGroup::SetName(AZStd::string&& name)
        {
            m_name = AZStd::move(name);
        }

        const AZ::Uuid& MeshGroup::GetId() const
        {
            return m_id;
        }

        void MeshGroup::OverrideId(const AZ::Uuid& id)
        {
            m_id = id;
        }

        AZ::SceneAPI::Containers::RuleContainer& MeshGroup::GetRuleContainer()
        {
            return m_rules;
        }

        const AZ::SceneAPI::Containers::RuleContainer& MeshGroup::GetRuleContainerConst() const
        {
            return m_rules;
        }

        void MeshGroup::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

            // Check if the context is serialized and has PhysXMeshGroup class data.
            if (serializeContext)
            {
                serializeContext->Class<MeshGroup, AZ::SceneAPI::DataTypes::ISceneNodeGroup>()->Version(1, &MeshGroup::VersionConverter)
                    ->Field("name", &MeshGroup::m_name)
                    ->Field("export as convex", &MeshGroup::m_exportAsConvex)

                // Convex params
                    ->Field("AreaTestEpsilon", &MeshGroup::m_areaTestEpsilon)
                    ->Field("PlaneTolerance", &MeshGroup::m_planeTolerance)
                    ->Field("Use16bitIndices", &MeshGroup::m_use16bitIndices)
                    ->Field("CheckZeroAreaTriangles", &MeshGroup::m_checkZeroAreaTriangles)
                    ->Field("QuantizeInput", &MeshGroup::m_quantizeInput)
                    ->Field("UsePlaneShifting", &MeshGroup::m_usePlaneShifting)
                    ->Field("ShiftVertices", &MeshGroup::m_shiftVertices)
                    ->Field("GaussMapLimit", &MeshGroup::m_gaussMapLimit)

                // trimesh params
                    ->Field("WeldVertices", &MeshGroup::m_weldVertices)
                    ->Field("DisableCleanMesh", &MeshGroup::m_disableCleanMesh)
                    ->Field("Force32BitIndices", &MeshGroup::m_force32BitIndices)
                    ->Field("SuppressTriangleMeshRemapTable", &MeshGroup::m_suppressTriangleMeshRemapTable)
                    ->Field("BuildTriangleAdjacencies", &MeshGroup::m_buildTriangleAdjacencies)
                    ->Field("MeshWeldTolerance", &MeshGroup::m_meshWeldTolerance)
                    ->Field("NumTrisPerLeaf", &MeshGroup::m_numTrisPerLeaf)

                    ->Field("BuildGPUData", &MeshGroup::m_buildGPUData)

                    ->Field("NodeSelectionList", &MeshGroup::m_nodeSelectionList)

                    ->Field("id", &MeshGroup::m_id)
                    ->Field("rules", &MeshGroup::m_rules);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<MeshGroup>("PhysX Mesh group", "Configure PhysX mesh data exporting.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute("AutoExpand", true)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")

                        ->DataElement(AZ_CRC("ManifestName", 0x5215b349), &MeshGroup::m_name, "Name PhysX Mesh",
                        "<span>Name for the group. This name will also be used as a part of the name for the generated file.</span>")

                        ->DataElement(AZ_CRC("ExportAsConvex", 0x18b516d9), &MeshGroup::m_exportAsConvex, "Export Mesh As Convex",
                        "<span>This will say the cooking process to build the mesh as a convex. This will make it available for dynamic objects.</span>")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)

                    // Convex params
                        ->DataElement(AZ_CRC("AreaTestEpsilon", 0x3c6f6877), &MeshGroup::m_areaTestEpsilon, "Area Test Epsilon",
                        "<span>If the area of a triangle of the hull is below this value, the triangle will be rejected. This test is done only if Check Zero Area Triangles is used.</span>")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &MeshGroup::GetExportAsConvex)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->DataElement(AZ_CRC("PlaneTolerance", 0xa8640bac), &MeshGroup::m_planeTolerance, "Plane Tolerance",
                        "<span>The value is used during hull construction. When a new point is about to be added to the hull it gets dropped when the point is closer to the hull than the planeTolerance. "
                        "The Plane Tolerance is increased according to the hull size. If 0.0f is set all points are accepted when the convex hull is created. This may lead to edge cases where the "
                        "new points may be merged into an existing polygon and the polygons plane equation might slightly change therefore. This might lead to failures during polygon merging phase "
                        "in the hull computation. It is recommended to use the default value, however if it is required that all points needs to be accepted or huge thin convexes are created, it "
                        "might be required to lower the default value.</span>")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &MeshGroup::GetExportAsConvex)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->DataElement(AZ_CRC("Use16bitIndices", 0xb81adbfa), &MeshGroup::m_use16bitIndices, "Use 16-bit Indices",
                        "<span>Denotes the use of 16-bit vertex indices in Convex triangles or polygons. Otherwise, 32-bit indices are assumed.</span>")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &MeshGroup::GetExportAsConvex)
                        ->DataElement(AZ_CRC("CheckZeroAreaTriangles", 0xa8b649c4), &MeshGroup::m_checkZeroAreaTriangles, "Check Zero Area Triangles",
                        "<span>Checks and removes almost zero-area triangles during convex hull computation. The rejected area size is specified in Area Test Epsilon.</span>")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &MeshGroup::GetExportAsConvex)
                        ->DataElement(AZ_CRC("QuantizeInput", 0xe64b9553), &MeshGroup::m_quantizeInput, "Quantize Input", "<span>Quantizes the input vertices using the k-means clustering.</span>")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &MeshGroup::GetExportAsConvex)
                        ->DataElement(AZ_CRC("UsePlaneShifting", 0xa10bad2e), &MeshGroup::m_usePlaneShifting, "Use Plane Shifting",
                        "<span>Enables plane shifting vertex limit algorithm. Plane shifting is an alternative algorithm for the case when the computed hull has more vertices than the specified vertex limit."
                        " The default algorithm computes the full hull, and an OBB around the input vertices. This OBB is then sliced with the hull planes until the vertex limit is reached."
                        " The default algorithm requires the vertex limit to be set to at least 8, and typically produces results that are much better quality than are produced by plane shifting."
                        " When plane shifting is enabled, the hull computation stops when vertex limit is reached. The hull planes are then shifted to contain all input vertices, and the new plane "
                        "intersection points are then used to generate the final hull with the given vertex limit. Plane shifting may produce sharp edges to vertices very far away from the input cloud,"
                        " and does not guarantee that all input vertices are inside the resulting hull. However, it can be used with a vertex limit as low as 4.</span>")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &MeshGroup::GetExportAsConvex)
                        ->DataElement(AZ_CRC("ShiftVertices", 0x580b6169), &MeshGroup::m_shiftVertices, "Shift Vertices",
                        "<span>Convex hull input vertices are shifted to be around origin to provide better computation stability. It is recommended to provide input vertices around the origin, "
                        "otherwise use this flag to improve numerical stability.</span>")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &MeshGroup::GetExportAsConvex)
                        ->DataElement(AZ_CRC("GaussMapLimit", 0x409f655e), &MeshGroup::m_gaussMapLimit, "Gauss Map Limit",
                        "<span>Vertex limit beyond which additional acceleration structures are computed for each convex mesh. Increase that limit to reduce memory usage. "
                        "Computing the extra structures all the time does not guarantee optimal performance. There is a per-platform break - "
                        "even point below which the extra structures actually hurt performance.</span>")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &MeshGroup::GetExportAsConvex)

                    // trimesh params
                        ->DataElement(AZ_CRC("WeldVertices", 0xe4e0c33c), &MeshGroup::m_weldVertices, "Weld Vertices", "<span>When set, mesh welding is performed. Clean mesh must be enabled.</span>")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &MeshGroup::GetExportAsTriMesh)
                        ->DataElement(AZ_CRC("DisableCleanMesh", 0xc720ef8e), &MeshGroup::m_disableCleanMesh, "Disable Clean Mesh",
                        "<span>When set, mesh cleaning is disabled. This makes cooking faster. When clean mesh is not performed, mesh welding is also not performed.</span>")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &MeshGroup::GetExportAsTriMesh)
                        ->DataElement(AZ_CRC("Force32BitIndices", 0x640dfd70), &MeshGroup::m_force32BitIndices, "Force 32-Bit Indices",
                        "<span>When set, 32-bit indices will always be created regardless of triangle count.</span>")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &MeshGroup::GetExportAsTriMesh)
                        ->DataElement(AZ_CRC("SuppressTriangleMeshRemapTable", 0x8b818a60), &MeshGroup::m_suppressTriangleMeshRemapTable, "Suppress Triangle Mesh Remap Table",
                        "<span>When true, the face remap table is not created. This saves a significant amount of memory, but the SDK will not be able to provide the remap information "
                        "for internal mesh triangles returned by collisions, sweeps or raycasts hits.</span>")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &MeshGroup::GetExportAsTriMesh)
                        ->DataElement(AZ_CRC("BuildTriangleAdjacencies", 0xbb5a9b49), &MeshGroup::m_buildTriangleAdjacencies, "Build Triangle Adjacencies",
                        "<span>When true, the triangle adjacency information is created. You can get the adjacency triangles for a given triangle from getTriangle.</span>")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &MeshGroup::GetExportAsTriMesh)
                        ->DataElement(AZ_CRC("MeshWeldTolerance", 0x37df452d), &MeshGroup::m_meshWeldTolerance, "Mesh Weld Tolerance",
                        "<span>Mesh weld tolerance. If mesh welding is enabled, this controls the distance at which vertices are welded. If mesh welding is not enabled, this value defines "
                        "the acceptance distance for mesh validation. Provided no two vertices are within this distance, the mesh is considered to be clean. If not, a warning will be emitted. "
                        "Having a clean, welded mesh is required to achieve the best possible performance. The default vertex welding uses a snap-to-grid approach."
                        " This approach effectively truncates each vertex to integer values using Mesh Weld Tolerance. Once these snapped vertices are produced, "
                        "all vertices that snap to a given vertex on the grid are remapped to reference a single vertex. Following this, all triangles indices are remapped to reference "
                        "this subset of clean vertices. It should be noted that the vertices that we do not alter the position of the vertices; the snap-to-grid is only performed to identify"
                        " nearby vertices. The mesh validation approach also uses the same snap-to-grid approach to identify nearby vertices. If more than one vertex snaps to a given "
                        "grid coordinate, we ensure that the distance between the vertices is at least Mesh Weld Tolerance. If this is not the case, a warning is emitted.</span>")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &MeshGroup::GetExportAsTriMesh)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->DataElement(AZ_CRC("NumTrisPerLeaf", 0x391bf6d1), &MeshGroup::m_numTrisPerLeaf, "Number of Triangles Per Leaf",
                        "<span>Mesh cooking hint for max triangles per leaf limit. Fewer triangles per leaf produces larger meshes with better runtime performance and worse cooking performance."
                        " More triangles per leaf results in faster cooking speed and smaller mesh sizes, but with worse runtime performance.</span>")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &MeshGroup::GetExportAsTriMesh)
                        ->Attribute(AZ::Edit::Attributes::Min, 4)
                        ->Attribute(AZ::Edit::Attributes::Max, 15)

                    // Both convex and trimesh
                        ->DataElement(AZ_CRC("BuildGPUData", 0x0b7b0568), &MeshGroup::m_buildGPUData, "Build GPU Data",
                        "<span>When true, additional information required for GPU-accelerated rigid body simulation is created. This can increase memory usage and cooking times for convex meshes "
                        "and triangle meshes. Convex hulls are created with respect to GPU simulation limitations. Vertex limit is set to 64 and vertex limit per face is internally set to 32.</span>")

                        ->DataElement(AZ::Edit::UIHandlers::Default, &MeshGroup::m_nodeSelectionList, "Select meshes", "<span>Select the meshes to be included in the mesh group.</span>")
                        ->Attribute("FilterName", "meshes")
                        ->Attribute("FilterType", AZ::SceneAPI::DataTypes::IMeshData::TYPEINFO_Uuid())
                        ->DataElement(AZ::Edit::UIHandlers::Default, &MeshGroup::m_rules, "", "Add or remove rules to fine-tune the export process.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20));
                }
            }
        }

        AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& MeshGroup::GetSceneNodeSelectionList()
        {
            return m_nodeSelectionList;
        }

        const AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& MeshGroup::GetSceneNodeSelectionList() const
        {
            return m_nodeSelectionList;
        }

        bool MeshGroup::GetExportAsConvex() const
        {
            return m_exportAsConvex;
        }

        bool MeshGroup::GetExportAsTriMesh() const
        {
            return !m_exportAsConvex;
        }

        float MeshGroup::GetAreaTestEpsilon() const
        {
            return m_areaTestEpsilon;
        }

        float MeshGroup::GetPlaneTolerance() const
        {
            return m_planeTolerance;
        }

        bool MeshGroup::GetUse16bitIndices() const
        {
            return m_use16bitIndices;
        }

        bool MeshGroup::GetCheckZeroAreaTriangles() const
        {
            return m_checkZeroAreaTriangles;
        }

        bool MeshGroup::GetQuantizeInput() const
        {
            return m_quantizeInput;
        }

        bool MeshGroup::GetUsePlaneShifting() const
        {
            return m_usePlaneShifting;
        }

        bool MeshGroup::GetShiftVertices() const
        {
            return m_shiftVertices;
        }

        AZ::u32 MeshGroup::GetGaussMapLimit() const
        {
            return m_gaussMapLimit;
        }

        bool MeshGroup::GetWeldVertices() const
        {
            return m_weldVertices;
        }

        void MeshGroup::SetWeldVertices(bool weldVertices)
        {
            m_weldVertices = true;
        }

        bool MeshGroup::GetDisableCleanMesh() const
        {
            return m_disableCleanMesh;
        }

        bool MeshGroup::GetForce32BitIndices() const
        {
            return m_force32BitIndices;
        }

        bool MeshGroup::GetSuppressTriangleMeshRemapTable() const
        {
            return m_suppressTriangleMeshRemapTable;
        }

        bool MeshGroup::GetBuildTriangleAdjacencies() const
        {
            return m_buildTriangleAdjacencies;
        }

        float MeshGroup::GetMeshWeldTolerance() const
        {
            return m_meshWeldTolerance;
        }

        void MeshGroup::SetMeshWeldTolerance(float weldTolerance)
        {
            m_meshWeldTolerance = weldTolerance;
        }

        AZ::u32 MeshGroup::GetNumTrisPerLeaf() const
        {
            return m_numTrisPerLeaf;
        }

        bool MeshGroup::GetBuildGPUData() const
        {
            return m_buildGPUData;
        }

        bool MeshGroup::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // Remove the material rule
            if (classElement.GetVersion() < 1)
            {
                int ruleContainerNodeIndex = classElement.FindElement(AZ_CRC("rules", 0x899a993c));
                if (ruleContainerNodeIndex >= 0)
                {
                    AZ::SerializeContext::DataElementNode& ruleContainerNode = classElement.GetSubElement(ruleContainerNodeIndex);
                    AZ::SceneAPI::Containers::RuleContainer ruleContainer;
                    if (ruleContainerNode.GetData<AZ::SceneAPI::Containers::RuleContainer>(ruleContainer))
                    {
                        auto materialRule = ruleContainer.FindFirstByType<AZ::SceneAPI::SceneData::MaterialRule>();
                        ruleContainer.RemoveRule(materialRule);
                    }

                    ruleContainerNode.SetData<AZ::SceneAPI::Containers::RuleContainer>(context, ruleContainer);
                }
            }

            return true;
        }
    }
}