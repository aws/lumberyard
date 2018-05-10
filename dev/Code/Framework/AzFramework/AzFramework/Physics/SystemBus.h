
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Component/EntityId.h>

#include <AzFramework/Physics/Base.h>

namespace AZ
{
    class Vector3;
}

namespace Physics
{
    class World;
    class WorldBody;
    class RigidBody;
    class Ghost;
    class Character;
    class CollisionFilter;
    class WorldSettings;
    class RigidBodySettings;
    class GhostSettings;
    class CharacterSettings;
    class ActionSettings;
    class Action;
    class ActionSettings;
    struct RayCastRequest;
    struct RayCastResult;
    struct ShapeCastRequest;
    struct ShapeCastResult;

    /**
      * Represents a debug vertex (position & color)
      */
    struct DebugDrawVertex
    {
        AZ::Vector3     m_position;
        AZ::Color       m_color;

        DebugDrawVertex(const AZ::Vector3& v, const AZ::Color& c)
            : m_position(v)
            , m_color(c)
        {}

        static AZ::Color GetGhostColor()        { return AZ::Color(1.0f, 0.7f, 0.0f, 1.0f); }
        static AZ::Color GetRigidBodyColor()    { return AZ::Color(0.0f, 1.0f, 0.0f, 1.0f); }
        static AZ::Color GetSleepingBodyColor() { return AZ::Color(0.5f, 0.5f, 0.5f, 1.0f); }
        static AZ::Color GetCharacterColor()    { return AZ::Color(0.0f, 1.0f, 1.0f, 1.0f); }
        static AZ::Color GetRayColor()          { return AZ::Color(0.8f, 0.4f, 0.2f, 1.0f); }
        static AZ::Color GetRed()               { return AZ::Color(1.0f, 0.0f, 0.0f, 1.0f); }
        static AZ::Color GetGreen()             { return AZ::Color(0.0f, 1.0f, 0.0f, 1.0f); }
        static AZ::Color GetBlue()              { return AZ::Color(0.0f, 0.0f, 1.0f, 1.0f); }
        static AZ::Color GetWhite()             { return AZ::Color(1.0f, 1.0f, 1.0f, 1.0f); }
    };

    /**
     * Settings structure provided to DebugDrawPhysics to drive debug drawing behavior.
     */
    struct DebugDrawSettings
    {
        using DebugDrawLineCallback = AZStd::function<void(const DebugDrawVertex& from, const DebugDrawVertex& to, const Ptr<WorldBody>& body, float thickness, void* udata)>;
        using DebugDrawTriangleCallback = AZStd::function<void(const DebugDrawVertex& a, const DebugDrawVertex& b, const DebugDrawVertex& c, const Ptr<WorldBody>& body, void* udata)>;
        using DebugDrawTriangleBatchCallback = AZStd::function<void(const DebugDrawVertex* verts, AZ::u32 numVerts, const AZ::u32* indices, AZ::u32 numIndices, const Ptr<WorldBody>& body, void* udata)>;

        DebugDrawLineCallback           m_drawLineCB;                               ///< Required user callback for line drawing.
        DebugDrawTriangleBatchCallback  m_drawTriBatchCB;                           ///< User callback for triangle batch drawing. Required if \ref m_isWireframe is false.
        bool                            m_isWireframe = false;                      ///< Specifies whether or not physics shapes should be draw as wireframe (lines only) or solid triangles.
        AZ::u32                         m_objectLayers = static_cast<AZ::u32>(~0);  ///< Mask specifying which \ref AzFramework::Physics::StandardObjectLayers should be drawn.
        AZ::Vector3                     m_cameraPos = AZ::Vector3::CreateZero();    ///< Camera position, for limiting objects based on \ref m_drawDistance.
        float                           m_drawDistance = 500.f;                     ///< Distance from \ref m_cameraPos within which objects will be drawn.
        bool                            m_drawBodyTransforms = false;               ///< If enabled, draws transform axes for each body.
        void*                           m_udata = nullptr;                          ///< Platform specific and/or gem specific optional user data pointer.

        void DrawLine(const DebugDrawVertex& from, const DebugDrawVertex& to, const Ptr<WorldBody>& body, float thickness = 1.0f) { m_drawLineCB(from, to, body, thickness, m_udata); }
        void DrawTriangleBatch(const DebugDrawVertex* verts, AZ::u32 numVerts, const AZ::u32* indices, AZ::u32 numIndices, const Ptr<WorldBody>& body) { m_drawTriBatchCB(verts, numVerts, indices, numIndices, body, m_udata); }
    };

    /**
      * Physics system global requests.
      */
    class SystemRequests
        : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        // singleton pattern
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        ////////////////////////////////////////////////////////////////////////

        virtual ~SystemRequests() = default;

        /**
         * Create/register a world.
         */
        virtual Ptr<World> CreateWorldByName(const char* worldName, const Ptr<WorldSettings>& settings) = 0;
        virtual Ptr<World> CreateWorldById(AZ::u32 worldId, const Ptr<WorldSettings>& settings) = 0;

        /**
         * Find a world.
         */
        virtual Ptr<World> FindWorldByName(const char* worldName) = 0;
        virtual Ptr<World> FindWorldById(AZ::u32 worldId) = 0;

        /**
         * \brief Returns the Default world managed by this System component
         * \return Smart pointer to the Default physics world
         */
        virtual Ptr<World> GetDefaultWorld() = 0;

        /**
         * Destroy a world.
         */
        virtual bool DestroyWorldByName(const char* worldName) = 0;
        virtual bool DestroyWorldById(AZ::u32 worldId) = 0;

        virtual Ptr<RigidBody> CreateRigidBody(const Ptr<RigidBodySettings>& settings) = 0;
        virtual Ptr<Ghost> CreateGhost(const Ptr<GhostSettings>& settings) = 0;
        virtual Ptr<Character> CreateCharacter(const Ptr<CharacterSettings>& settings) = 0;
        virtual Ptr<Action> CreateBuoyancyAction(const ActionSettings& settings) = 0;
    };

    typedef AZ::EBus<SystemRequests> SystemRequestBus;

    /**
     * Physics system global debug requests.
     */
    class SystemDebugRequests
        : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        // singleton pattern
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        ////////////////////////////////////////////////////////////////////////

        /**
          * Draw physics system state.
          * \param settings see \ref DebugDrawSettings
          */
        virtual void DebugDrawPhysics(const DebugDrawSettings& settings) { (void)settings; }

        /**
          * Exports an entity's physics body(ies) to the specified filename, if supported by the physics backend.
          */
        virtual void ExportEntityPhysics(const AZStd::vector<AZ::EntityId>& ids, const AZStd::string& filename) { (void)ids; (void)filename; }
    };

    typedef AZ::EBus<SystemDebugRequests> SystemDebugRequestBus;

    /**
     * 
     */
    class SystemNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~SystemNotifications() {}

        virtual void OnPrePhysicsUpdate() {};
        virtual void OnPostPhysicsUpdate() {};
    };

    typedef AZ::EBus<SystemNotifications> SystemNotificationBus;
    
} // namespace Physics
