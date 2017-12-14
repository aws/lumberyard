
#pragma once

#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/std/containers/vector.h>

#include <AzFramework/Physics/Base.h>
#include <AzFramework/Physics/ShapeIdHierarchy.h>

namespace Physics
{        
    /**
        * 
        */
    enum class ShapeType : AZ::u8
    {
        Sphere,
        Box,
        Capsule,
        Cylinder,
        ConvexHull,
        TriangleSoup,

        Scale,
        Transform,
        Compound,

        Native, ///< Native shape configuration if user wishes to bypass generic shape configurations.
    };

    class ShapeConfiguration : public ReferenceBase
    {
    public:

        AZ_CLASS_ALLOCATOR(ShapeConfiguration, AZ::SystemAllocator, 0);

        virtual ~ShapeConfiguration() = default;

        /// Mapping from ShapeIdHiearchy values to physical materials for this shape.
        Ptr<ShapeHierarchyMaterialMap>  m_shapeHierarchyMaterialMap;

        /// Shape data is expected to be oriented around center of mass, which may differ
        /// from visual/render data. This offset provides the ability to offset the externally
        /// reported body position to which entities are synchronized, from the simulated body position.
        /// Note that this only applies at the root shape of a given body. Shape hierarchies should
        /// use TransformShapeConfiguration or CompoundShapeConfiguration for relative offsets.
        AZ::Transform                   m_offsetFromRender = AZ::Transform::CreateIdentity();

        virtual ShapeType GetShapeType() const = 0;
    };

    class SphereShapeConfiguration : public ShapeConfiguration
    {
    public:

        AZ_CLASS_ALLOCATOR(SphereShapeConfiguration, AZ::SystemAllocator, 0);

        static Ptr<SphereShapeConfiguration> Create(float radius = 0.5f)
        {
            return aznew SphereShapeConfiguration(radius);
        }

        float m_radius;

        ShapeType GetShapeType() const override { return ShapeType::Sphere; }

    private:

        SphereShapeConfiguration(float radius) : m_radius(radius) {}
    };

    class BoxShapeConfiguration : public ShapeConfiguration
    {

    public:

        AZ_CLASS_ALLOCATOR(BoxShapeConfiguration, AZ::SystemAllocator, 0);

        static Ptr<BoxShapeConfiguration> Create(const AZ::Vector3& halfExtents = AZ::Vector3(0.5f))
        {
            return aznew BoxShapeConfiguration(halfExtents);
        }

        AZ::Vector3 m_halfExtents;

        ShapeType GetShapeType() const override { return ShapeType::Box; }

    private:
        BoxShapeConfiguration(const AZ::Vector3& halfExtents)
            : m_halfExtents(halfExtents)
        {}
    };

    class CapsuleShapeConfiguration : public ShapeConfiguration
    {
    public:

        AZ_CLASS_ALLOCATOR(CapsuleShapeConfiguration, AZ::SystemAllocator, 0);

        static Ptr<CapsuleShapeConfiguration> Create(AZ::Vector3 pointA = AZ::Vector3(0.0f, 0.5f, 0.0f), AZ::Vector3 pointB = AZ::Vector3(0.0f, -0.5f, 0.0f), float radius = 0.25f)
        {
            return aznew CapsuleShapeConfiguration(pointA, pointB, radius);
        }

        AZ::Vector3 m_pointA;
        AZ::Vector3 m_pointB;
        float m_radius;

        ShapeType GetShapeType() const override { return ShapeType::Capsule; }

    private:
        CapsuleShapeConfiguration(AZ::Vector3 pointA, AZ::Vector3 pointB, float radius)
            : m_pointA(pointA)
            , m_pointB(pointB)
            , m_radius(radius)
        {}
    };

    class CylinderShapeConfiguration : public ShapeConfiguration
    {
    public:

        AZ_CLASS_ALLOCATOR(CylinderShapeConfiguration, AZ::SystemAllocator, 0);

        static Ptr<CylinderShapeConfiguration> Create(AZ::Vector3 pointA = AZ::Vector3(0.0f, 0.5f, 0.0f), AZ::Vector3 pointB = AZ::Vector3(0.0f, -0.5f, 0.0f), float radius = 0.25f)
        {
            return aznew CylinderShapeConfiguration(pointA, pointB, radius);
        }

        AZ::Vector3 m_pointA;
        AZ::Vector3 m_pointB;
        float m_radius;

        ShapeType GetShapeType() const override { return ShapeType::Cylinder; }

    private:
        CylinderShapeConfiguration(AZ::Vector3 pointA, AZ::Vector3 pointB, float radius)
            : m_pointA(pointA)
            , m_pointB(pointB)
            , m_radius(radius)
        {}
    };

    class ConvexHullShapeConfiguration : public ShapeConfiguration
    {
    public:

        AZ_CLASS_ALLOCATOR(ConvexHullShapeConfiguration, AZ::SystemAllocator, 0);

        static Ptr<ConvexHullShapeConfiguration> Create()
        {
            return aznew ConvexHullShapeConfiguration();
        }

        const void* m_vertexData;
        AZ::u32 m_vertexCount;
        AZ::u32 m_vertexStride;

        const void* m_planeData;
        AZ::u32 m_planeCount;
        AZ::u32 m_planeStride;

        const void* m_adjacencyData;
        AZ::u32 m_adjacencyCount;
        AZ::u32 m_adjacencyStride;

        bool m_copyData;        ///< If set, vertex buffer will be copied in the native physics implementation,

        ShapeType GetShapeType() const override { return ShapeType::ConvexHull; }

    private:

        ConvexHullShapeConfiguration()
            : m_vertexData(nullptr)
            , m_vertexCount(0)
            , m_vertexStride(4)
            , m_planeData(nullptr)
            , m_planeCount(0)
            , m_planeStride(4)
            , m_adjacencyData(nullptr)
            , m_adjacencyCount(0)
            , m_adjacencyStride(4)
            , m_copyData(true)
        {}
    };

    class TriangleSoupShapeConfiguration : public ShapeConfiguration
    {
    public:

        AZ_CLASS_ALLOCATOR(TriangleSoupShapeConfiguration, AZ::SystemAllocator, 0);

        static Ptr<TriangleSoupShapeConfiguration> Create()
        {
            return aznew TriangleSoupShapeConfiguration();
        }

        const void* m_vertexData;
        AZ::u32 m_vertexCount;
        AZ::u32 m_vertexStride; ///< Data size of a given vertex, e.g. float * 3 = 12.

        const void* m_indexData;
        AZ::u32 m_indexCount;
        AZ::u32 m_indexStride;  ///< Data size of indices for a given triangle, e.g. AZ::u32 * 3 = 12.

        bool m_copyData;        ///< If set, vertex/index buffers will be copied in the native physics implementation,
                                ///< and don't need to be kept alive by the caller;

        // Need to store material lookup per primitive (triangle).

        ShapeType GetShapeType() const override { return ShapeType::TriangleSoup; }

    private:

        TriangleSoupShapeConfiguration()
            : m_vertexData(nullptr)
            , m_vertexCount(0)
            , m_vertexStride(4)
            , m_indexData(nullptr)
            , m_indexCount(0)
            , m_indexStride(12)
            , m_copyData(true)
        {}
    };

    class ScaleShapeConfiguration : public ShapeConfiguration
    {
    public:

        AZ_CLASS_ALLOCATOR(ScaleShapeConfiguration, AZ::SystemAllocator, 0);

        static Ptr<ScaleShapeConfiguration> Create()
        {
            return aznew ScaleShapeConfiguration();
        }

        Ptr<ShapeConfiguration> m_shapeConfiguration;
        AZ::Vector3 m_scale;

        ShapeType GetShapeType() const override { return ShapeType::Scale; }

    private:

        ScaleShapeConfiguration()
            : m_shapeConfiguration(nullptr)
            , m_scale(AZ::Vector3::CreateOne())
        {}

    };

    class TransformShapeConfiguration : public ShapeConfiguration
    {
    public:

        AZ_CLASS_ALLOCATOR(TransformShapeConfiguration, AZ::SystemAllocator, 0);

        static Ptr<TransformShapeConfiguration> Create()
        {
            return aznew TransformShapeConfiguration();
        }

        Ptr<ShapeConfiguration> m_shapeConfiguration;
        AZ::Transform m_relativeTransform;

        ShapeType GetShapeType() const override { return ShapeType::Transform; }

    private:

        TransformShapeConfiguration()
            : m_shapeConfiguration(nullptr)
            , m_relativeTransform(AZ::Transform::CreateIdentity())
        {}

    };

    class CompoundShapeConfiguration : public ShapeConfiguration
    {
    public:

        AZ_CLASS_ALLOCATOR(CompoundShapeConfiguration, AZ::SystemAllocator, 0);

        static Ptr<CompoundShapeConfiguration> Create()
        {
            return aznew CompoundShapeConfiguration();
        }

        AZStd::vector<Ptr<TransformShapeConfiguration>> m_transformShapes;

        ShapeType GetShapeType() const override { return ShapeType::Compound; }

    private:

        CompoundShapeConfiguration()
        {}
    };

    class NativeShape : public ReferenceBase
    {
    public:

        AZ_CLASS_ALLOCATOR(NativeShape, AZ::SystemAllocator, 0);
        AZ_RTTI(NativeShape, "{1CBFC4B4-03C5-4B57-B23F-60FD02A06F5C}", ReferenceBase);

        NativeShape(void* nativePointer)
            : m_nativePointer(nativePointer)
        {
            AZ_Assert(m_nativePointer, "Native shape wrapper requires a pointer to the native engine-specific shape.");
        }

        void* m_nativePointer = nullptr;
    };

    class NativeShapeConfiguration : public ShapeConfiguration
    {
    public:

        AZ_CLASS_ALLOCATOR(NativeShapeConfiguration, AZ::SystemAllocator, 0);

        static Ptr<NativeShapeConfiguration> Create(const Ptr<NativeShape>& nativeShape = Ptr<NativeShape>())
        {
            return aznew NativeShapeConfiguration(nativeShape);
        }

        Ptr<NativeShape> m_nativeShape;

        ShapeType GetShapeType() const override { return ShapeType::Native; }

    private:

        NativeShapeConfiguration(const Ptr<NativeShape>& nativeShape)
            : m_nativeShape(nativeShape)
        {}
    };

} // namespace Physics
