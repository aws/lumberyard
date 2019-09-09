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

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/Pose.h>

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional.h>


namespace EMotionFX
{
    class ActorInstance;

    class EMFX_API SpringSolver
    {
    public:
        struct EMFX_API Spring
        {
            float   m_restLength = 1.0f;        /**< The rest length of the spring. */
            size_t  m_particleA = ~0;           /**< The first particle index. */
            size_t  m_particleB = ~0;           /**< The second particle index (the parent). */
            bool    m_isSupportSpring = false;  /**< Is this spring a support spring? (default=false). */
            bool    m_allowStretch = false;     /**< Allow this spring to be stretched or compressed? (default=false). */

            Spring() = default;
            Spring(size_t particleA, size_t particleB, float restLength, bool isSupportSpring, bool allowStretch)
                : m_restLength(restLength)
                , m_particleA(particleA)
                , m_particleB(particleB)
                , m_isSupportSpring(isSupportSpring)
                , m_allowStretch(allowStretch) {}
        };

        struct EMFX_API Particle
        {
            AZ::Vector3  m_pos;                   /**< The current (desired) particle position, in world space. */
            AZ::Vector3  m_oldPos;                /**< The previous position of the particle. */
            AZ::Vector3  m_force;                 /**< The internal force, which contains the gravity and other pulling and pushing forces. */
            AZ::Vector3  m_externalForce;         /**< A user defined external force, which is added on top of the internal force. Can be used to simulate wind etc. */
            float        m_coneAngleLimit;        /**< The conic angular limit, in degrees. This defaults on 180, which allows any rotation. This is the angle it allows to deviate from the original input pose (from the animation or bind pose). */
            float        m_invMass;               /**< The inverse mass, which equals 1.0f / mass. This is the inverse weight of the bone. Default value is 1.0. */
            float        m_stiffness;             /**< The stiffness of the bone. A value of 0.0 means no stiffness at all. A value of say 50 would make it move back into its original pose. Making things look bouncy. Default value is 0.0. */
            float        m_damping;               /**< The damping value, which defaults on 0.001f. A value of 0 would mean the bone would oscilate forever. Higher values make it come to rest sooner. */
            float        m_gravityFactor;         /**< The factor with which the gravity force will be multiplied with. Default is 1.0. A value of 2.0 means two times the amount of gravity being applied to this particle. */
            float        m_friction;              /**< The friction factor, between 0 and 1, which happens when a collision happens with a surface. The default is 0.0f. */
            AZ::u32      m_jointIndex;            /**< The joint index inside the Actor object, which is represented by this particle. */
            bool         m_fixed;                 /**< This is set to false on default, which means it is a movable particle. When set to true, the particle will stick to the input position of the joint. This would make it stick to the body. */
            uint8        m_groupId;               /**< Some optional particle group ID (default=0). You can use it to identify specific groups of particles/simulated nodes. You could for example group all hair particles together, and change the settings only for those particles. */

            Particle();
            ~Particle() = default;
        };

        class EMFX_API CollisionObject
        {
            friend class SpringSolver;
        public:
            enum CollisionType
            {
                Sphere  = 0,        /**< A sphere, which is described by a center position (m_start) and a radius. */
                Capsule = 1         /**< A capsule, which is described by a start (m_start) and end (m_end) position, and a diameter (m_radius). */
            };

            CollisionObject();
            ~CollisionObject() = default;

            void SetupSphere(const AZ::Vector3& center, float radius);
            void SetupCapsule(const AZ::Vector3& startPos, const AZ::Vector3& endPos, float diameter);

            AZ_INLINE void LinkToJoint(AZ::u32 jointIndex)               { m_jointIndex = jointIndex; }
            AZ_INLINE void Unlink()                                      { m_jointIndex = ~0; }
            AZ_INLINE void SetStart(const AZ::Vector3& start)            { m_start = start; }
            AZ_INLINE void SetEnd(const AZ::Vector3& end)                { m_end = end; }
            AZ_INLINE void SetRadius(float radiusOrDiameter)             { m_radius = radiusOrDiameter; }
            AZ_INLINE void SetType(CollisionType newType)                { m_type = newType; }
            AZ_INLINE const AZ::Vector3& GetStart() const                { return m_start; }
            AZ_INLINE const AZ::Vector3& GetEnd() const                  { return m_end; }
            AZ_INLINE const AZ::Vector3& GetGlobalStart() const          { return m_globalStart; }
            AZ_INLINE const AZ::Vector3& GetGlobalEnd() const            { return m_globalEnd; }
            AZ_INLINE float GetRadius() const                            { return m_radius; }
            AZ_INLINE AZ::u32 GetParentJoint() const                     { return m_jointIndex; }
            AZ_INLINE bool GetHasParentJoint() const                     { return (m_jointIndex != ~0); }
            AZ_INLINE CollisionType GetType() const                      { return m_type; }

        private:
            AZ::Vector3     m_globalStart;               /**< The world space start position, or the world space center in case of a sphere. */
            AZ::Vector3     m_globalEnd;                 /**< The world space end position. This is ignored in case of a sphere. */
            AZ::Vector3     m_start;                     /**< The start of the primitive. In case of a sphere the center, in case of a capsule the start of the capsule. */
            AZ::Vector3     m_end;                       /**< The end position of the primitive. In case of a sphere this is ignored. */
            float           m_radius;                    /**< The radius or thickness. */
            AZ::u32         m_jointIndex;                /**< The joint index to attach to, or ~0 for non-attached. */
            CollisionType   m_type;                      /**< The collision primitive type (a sphere, or capsule, etc). */
            bool            m_firstUpdate;               /**< Is this the first update? */
        };

        using ParticleAdjustFunction = AZStd::function<void(Particle&)>;

        SpringSolver();
        ~SpringSolver();

        void Clear();
        void SetActorInstance(ActorInstance* actorInstance);
        void Update(Pose& pose, float timePassedInSeconds, float weight);
        void DebugRender(const AZ::Color& color);
        void Scale(float scaleFactor);
        void Log();

        void AdjustParticles(const ParticleAdjustFunction& func);

        AZ_INLINE Particle& GetParticle(size_t index)             { return m_particles[index]; }
        AZ_INLINE size_t GetNumParticles() const                  { return m_particles.size(); }
        AZ_INLINE Spring& GetSpring(AZ::u32 index)                { return m_springs[index]; }
        AZ_INLINE size_t GetNumSprings() const                    { return m_springs.size(); }

        void SetGravity(const AZ::Vector3& gravity);
        const AZ::Vector3& GetGravity() const;
        void SetFixedTimeStep(double timeStep);
        double GetFixedTimeStep() const;
        void SetNumIterations(size_t numIterations);
        size_t GetNumIterations() const;

        void StartNewChain();
        Particle* AddJoint(AZ::u32 jointIndex, bool isFixed, bool allowStretch, const Particle* copySettingsFrom = nullptr);
        Particle* AddJoint(const char* nodeName, bool isFixed, bool allowStretch, const Particle* copySettingsFrom = nullptr);

        bool AddChain(AZ::u32 startJointIndex, AZ::u32 endJointIndex, bool startJointIsFixed = true, bool allowStretch = false, const Particle* copySettingsFrom = nullptr);
        bool AddChain(const char* startJointName, const char* endJointName, bool startJointIsFixed = true, bool allowStretch = false, const Particle* copySettingsFrom = nullptr);
        bool AddSupportSpring(AZ::u32 nodeA, AZ::u32 nodeB, float restLength = -1.0f);
        bool AddSupportSpring(const char* nodeNameA, const char* nodeNameB, float restLength = -1.0f);

        bool RemoveJoint(AZ::u32 jointIndex);
        bool RemoveJoint(const char* nodeName);
        bool RemoveSupportSpring(AZ::u32 jointIndexA, AZ::u32 jointIndexB);
        bool RemoveSupportSpring(const char* nodeNameA, const char* nodeNameB);

        size_t FindParticle(AZ::u32 jointIndex) const;
        Particle* FindParticle(const char* nodeName);

        void AddCollisionSphere(AZ::u32 jointIndex, const AZ::Vector3& center, float radius);
        void AddCollisionCapsule(AZ::u32 jointIndex, const AZ::Vector3& start, const AZ::Vector3& end, float diameter);

        AZ_INLINE void RemoveCollisionObject(size_t index)                  { m_collisionObjects.erase(m_collisionObjects.begin() + index); }
        AZ_INLINE void RemoveAllCollisionObjects()                          { m_collisionObjects.clear(); }
        AZ_INLINE CollisionObject& GetCollisionObject(AZ::u32 index)        { return m_collisionObjects[index]; }
        AZ_INLINE size_t GetNumCollisionObjects() const                     { return m_collisionObjects.size(); }
        AZ_INLINE bool GetCollisionEnabled() const                          { return m_collisionDetection; }
        AZ_INLINE void SetCollisionEnabled(bool enabled)                    { m_collisionDetection = enabled; }

    private:
        struct EMFX_API ParticleState
        {
            AZ::Vector3 m_pos;
            AZ::Vector3 m_oldPos;
        };

        struct EMFX_API State
        {
            AZStd::vector<ParticleState> m_particles;

            void Clear()
            {
                m_particles.clear();
            }
        };

        void Integrate(float timeDelta);
        void CalcForces(const Pose& pose);
        void SatisfyConstraints(const Pose& pose, size_t numIterations);
        void Simulate(float deltaTime, const Pose& pose);
        void UpdateJointTransforms(Pose& pose, float weight);
        size_t AddParticle(AZ::u32 jointIndex, const Particle* copySettingsFrom = nullptr);
        bool CollideWithSphere(AZ::Vector3& pos, const AZ::Vector3& center, float radius);
        bool CollideWithCapsule(AZ::Vector3& pos, const AZ::Vector3& lineStart, const AZ::Vector3& lineEnd, float radius);
        void UpdateCollisionObjects(const Pose& pose);
        void PrepareCollisionObjects(const Pose& pose);
        bool PerformCollision(AZ::Vector3& inOutPos);
        void PerformConeLimit(const Spring& spring, Particle& particleA, Particle& particleB, const AZ::Vector3& inputDir);
        void UpdateFixedParticles(const Pose& pose);
        void UpdateCurrentState();
        void InterpolateState(float alpha);

        State                           m_currentState;         /**< The current physics state. */
        State                           m_prevState;            /**< The previous physics state. */
        AZStd::vector<Spring>           m_springs;              /**< The collection of springs in the system. */
        AZStd::vector<Particle>         m_particles;            /**< The particles, which are connected by springs. */
        AZStd::vector<CollisionObject>  m_collisionObjects;     /**< The collection of collision objects. */
        AZ::Vector3                     m_gravity;              /**< The gravity force vector, which is (0.0f, 0.0f, -9.81f) on default. */
        ActorInstance*                  m_actorInstance;        /**< The actor instance we work on. */
        size_t                          m_numIterations;        /**< The number of iterations to run the constraint solving routines. The default is 1. */
        size_t                          m_parentParticle;       /**< The parent particle of the one you add next. Set ot ~0 when there is no parent particle yet. */
        double                          m_fixedTimeStep;        /**< The fixed time step value, to update the simulation with. The default is (1.0 / 60.0), which updates the simulation at 60 fps. */
        double                          m_lastTimeDelta;        /**< The previous time delta. */
        double                          m_timeAccumulation;     /**< The accumulated time of the updates, used for fixed timestep processing. */
        bool                            m_collisionDetection;   /**< Perform collision detection? Default is true. */
    };
}
