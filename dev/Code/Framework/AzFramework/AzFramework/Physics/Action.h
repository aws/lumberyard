
#pragma once

#include <AzCore/Math/Plane.h>

#include <AzFramework/Physics/Base.h>
#include <AzFramework/Physics/WorldBody.h>

namespace Physics
{
    class World;
    class WorldBody;

    /**
     * Types of physics actions.
     */
    enum class ActionType
    {
        User,       ///< Action behavior is fully expected to be implemented by the user.

        /// Common standard action types that should be provided by the backing physics implementation.
        Buoyancy,   ///< Water volumes with buoyancy.
    };

    /**
     * Default parameter class for user actions.
     */
    class ActionParameters
    {
    public:
        virtual ~ActionParameters() {}

        virtual ActionType GetActionType() { return ActionType::User; }
    };

    /**
     * Parameter class for buoyancy action.
     */
    class ActionParametersBuoyancy : public ActionParameters
    {
    public:
        ActionType GetActionType() override { return ActionType::Buoyancy; }

        float                           m_waterDensity          = 100.f;    ///< Density of water volume.
        float                           m_linearDamping         = 0.5f;     ///< Linear drag to apply to flowing bodies.
        float                           m_angularDamping        = 3.f;      ///< Angular drag to apply to flowing bodies.

        /// Callback for retrieving water plane and flow information at a given world position (required).
        typedef AZStd::function<AZ::Plane(const AZ::Vector3& /*position*/, AZ::Vector3* /*oFlow*/)> WaterSurfaceCallbackFunction;

        WaterSurfaceCallbackFunction    m_waterSurfaceCallback;
    };

    /**
     * Action creation settings structure.
     */
    class ActionSettings
    {
    public:

        ActionSettings(ActionParameters* parameters)
            : m_parameters(parameters)
        {}

        ActionParameters*               m_parameters;   /// Action-specific parameters. \ref ActionParameters
        AZStd::vector<Ptr<WorldBody>>   m_bodies;       ///< List of bodies to be initially assigned to the action (optional).
    };

    /**
     * Represents a physics action.
     * Physics actions can be registered with the world to receive notification prior to each simulation update.
     * Actions are a generic mechanism for providing any sort of physical effect on a group of bodies.
     * Typically ghosts and other mechanisms are used to gather bodies and subscribe them to a given action.
     * Some examples include: buoyancy, wind, area effects, etc.
     */
    class Action : public ReferenceBase
    {
    public:

        friend class World;
        friend class AZStd::intrusive_ptr<Action>;

        AZ_CLASS_ALLOCATOR(Action, AZ::SystemAllocator, 0);
        AZ_RTTI(Action, "{D66592AD-21BF-414D-8097-426DC081C201}", ReferenceBase);

        /// Main simulation update for the action.
        /// \param deltaTime Time delta since previous physics simulation update.
        virtual void Apply(float deltaTime) = 0;

        /// Add a body to the action.
        /// \param body The body to add to the action.
        virtual void AddBody(const Ptr<WorldBody>& body)
        {
            AZ_Error("Physics", m_bodies.end() == AZStd::find(m_bodies.begin(), m_bodies.end(), body), "Body is already registered with this action.");
            m_bodies.push_back(body);
        }

        /// Remove a body from the action.
        /// \param body The body to remove to the action.
        virtual void RemoveBody(const Ptr<WorldBody>& body)
        {
            auto iter = AZStd::find(m_bodies.begin(), m_bodies.end(), body);
            if (iter != m_bodies.end())
            {
                m_bodies.erase(iter);
            }
        }

        /// Set the list of bodies assigned to the action.
        /// \param bodies List of bodies to add.
        virtual void SetBodies(const AZStd::vector<Ptr<WorldBody>>& bodies)
        {
            m_bodies = bodies;
        }

        /// Retrieve the list of bodies assigned to the action.
        /// \return List of bodies.
        const AZStd::vector<Ptr<WorldBody>>& GetBodies() const
        {
            return m_bodies;
        }

        /// User callback when action is added to the world.
        virtual void OnAddedToWorld(const Ptr<World>& world) { (void)world; }

        /// User callback when action is removed from the world.
        virtual void OnRemovedFromWorld(const Ptr<World>& world) { (void)world; }

    protected:

        Action()
            : m_world(nullptr)
        {
        }

        explicit Action(const ActionSettings& settings)
            : m_world(nullptr)
        {
            SetBodies(settings.m_bodies);
        }

        ~Action() override
        {}

        AZStd::vector<Ptr<WorldBody>>   m_bodies;   ///< The list of bodies assigned to the action.
        World*                          m_world;    ///< The world to which this action is assigned.
    };

} // namespace Physics
