
#pragma once

#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/containers/unordered_set.h>

#include <AzFramework/Physics/Base.h>
#include <AzFramework/Physics/CollisionFilter.h>
#include <AzFramework/Physics/WorldBody.h>
#include <AzFramework/Physics/ShapeConfiguration.h>

namespace Physics
{
    class World;
    class WorldBody;
    class Ghost;

    /**
     * 
     */
    enum class GhostType : AZ::u8
    {
        Aabb,
        Shape,
    };

    /**
     * 
     */
    class GhostEnterEventInfo
    {
    public:
        Ptr<Ghost>      m_ghost;
        Ptr<WorldBody>  m_body;

        /// Contact data is only relevant for Shape ghosts, as AABB ghosts are only guaranteed to report basic overlap information.
        struct
        {
            AZ::Vector3     m_point;
            AZ::Vector3     m_normal;
        } m_contact;
    };

    /**
     * 
     */
    class GhostExitEventInfo
    {
    public:
        Ptr<Ghost>      m_ghost;
        Ptr<WorldBody>  m_body;
    };

    /**
     * 
     */
    class GhostEventHandler : public ReferenceBase
    {
    public:

        friend class Ghost;

        AZ_CLASS_ALLOCATOR(GhostEventHandler, AZ::SystemAllocator, 0);

        virtual ~GhostEventHandler() = default;

        virtual void OnEnterGhost(const Ptr<Ghost>& ghost, const GhostEnterEventInfo& info) = 0;
        virtual void OnExitGhost(const Ptr<Ghost>& ghost, const GhostExitEventInfo& info) = 0;
    };

    /**
     * 
     */
    class GhostSettings : public WorldBodySettings
    {
    public:

        AZ_CLASS_ALLOCATOR(GhostSettings, AZ::SystemAllocator, 0);
        AZ_RTTI(GhostSettings, "{C935A52E-F4BA-4EA1-8642-7790DA38BEAB}", WorldBodySettings);

        static Ptr<GhostSettings> Create()
        {
            return aznew GhostSettings();
        }

        GhostType                   m_ghostType     = GhostType::Aabb;          ///< Type of ghost (Aabb or shape-based).
        Ptr<ShapeConfiguration>     m_ghostShape    = nullptr;                  ///< Ghost shape (required for Shape-type ghosts).
        AZ::Aabb                    m_ghostAabb     = AZ::Aabb::CreateNull();   ///< Ghost aabb (reuqired for Aabb-type ghosts).

    private:

        GhostSettings()
        {}
    };

    /**
     * 
     */
    class Ghost : public WorldBody
    {
    public:

        AZ_CLASS_ALLOCATOR(Ghost, AZ::SystemAllocator, 0);
        AZ_RTTI(Ghost, "{948E4ED4-AF1F-490A-872B-F6826C3D5B9E}", WorldBody);

        friend class AZStd::intrusive_ptr<Ghost>;

    public:

        virtual Ptr<NativeShape> GetNativeShape() { return Ptr<NativeShape>(); }

        virtual void SetAabb(const AZ::Aabb& aabb) = 0;

        virtual void RegisterEventHandler(const Ptr<GhostEventHandler>& handler)
        {
            AZ_Assert(m_eventHandlers.end() == AZStd::find(m_eventHandlers.begin(), m_eventHandlers.end(), handler), "Event handler is already registered.");

            m_eventHandlers.push_back(handler.get());
        }

        virtual void UnregisterEventHandler(const Ptr<GhostEventHandler>& handler)
        {
            auto iter = AZStd::find(m_eventHandlers.begin(), m_eventHandlers.end(), handler);
            if (iter != m_eventHandlers.end())
            {
                m_eventHandlers.erase(iter);
            }
        }

        const AZStd::vector<Ptr<GhostEventHandler>>& GetGhostEventHandlers() const
        {
            return m_eventHandlers;
        }

    protected:

        Ghost()
            : WorldBody(GhostSettings::Create())
        {
        }

        explicit Ghost(const Ptr<GhostSettings>& settings)
            : WorldBody(settings)
        {
            if (settings && settings->m_ghostShape)
            {
                m_shapeHierarchyMaterialMap = settings->m_ghostShape->m_shapeHierarchyMaterialMap;
            }
        }

        ~Ghost() override
        {
        }

        AZStd::vector<Ptr<GhostEventHandler>> m_eventHandlers;
    };
    
} // namespace Physics
