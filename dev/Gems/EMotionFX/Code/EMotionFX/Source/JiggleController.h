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

// include required headers
#include "EMotionFXConfig.h"
#include "GlobalSpaceController.h"

#include <MCore/Source/Vector.h>
#include <MCore/Source/Array.h>
#include <MCore/Source/Matrix4.h>


namespace EMotionFX
{
    /**
     * The jiggle bone global space controller.
     * Jiggle bones are basically simple physically simulated bones.
     * They can be used to automatically animate bones inside hair, boobs and clothing items.
     * When using this controller all you need to do is call the StartNewChain(), followed by a list of AddNode(...) method calls for every bone you wish to simulate.
     * For every new chain you wish to simulate, you first call StartNewChain.
     * For your convenience we also added a method named AddChain(...) which allows you to specify a start and end node. All nodes from the start node till the end node
     * will then be added to the simulation.
     * Please keep in mind that it is VERY important to do this in a way where you add the bones up in the hierarchy (closer to the root node)
     * before you call an AddNode on their child nodes. If you do not do this, this could lead to incorrect behavior.
     * Basic collision detection is also supported by this controller. Collision detection is enabled on default. The more collision objects you add, the slower it gets.
     */
    class EMFX_API JiggleController
        : public GlobalSpaceController
    {
    public:
        // the unique ID of this controller
        enum
        {
            TYPE_ID = 0x01056001
        };                              // a unique 32 bit ID number that identifies this JiggleController class

        /**
         * A spring, which connects two particles together.
         * Each spring has a given rest length, which is the original distance between the two particles.
         * When the particles are further away, they will be pulled together. When they are too close together they
         * are being pushed away out of eachother.
         */
        struct EMFX_API Spring
        {
            float   mRestLength;        /**< The rest length of the spring. */
            uint32  mParticleA;         /**< The first particle index. */
            uint32  mParticleB;         /**< The second particle index (the parent). */
            bool    mIsSupportSpring;   /**< Is this spring a support spring? (default=false). */
            bool    mAllowStretch;      /**< Allow this spring to be stretched or compressed? (default=false). */

            Spring()
                : mRestLength(1.0f)
                , mParticleA(MCORE_INVALIDINDEX32)
                , mParticleB(MCORE_INVALIDINDEX32)
                , mIsSupportSpring(false)
                , mAllowStretch(false) {}
            Spring(uint32 particleA, uint32 particleB, float restLength, bool isSupportSpring, bool allowStretch)
                : mRestLength(restLength)
                , mParticleA(particleA)
                , mParticleB(particleB)
                , mIsSupportSpring(isSupportSpring)
                , mAllowStretch(allowStretch) {}
        };

        /**
         * A particle in the mass spring system.
         * Every node/bone basically has one particle, which simulates that node.
         * Particle settings can be adjusted on the fly. You can use the JiggleController::GetParticle(...) method for this.
         */
        class EMFX_API Particle
        {
            friend class JiggleController;
        public:
            MCore::Vector3  mPos;                       /**< The current (desired) particle position, in global space. */
            MCore::Vector3  mOldPos;                    /**< The previous position of the particle. */
            MCore::Vector3  mForce;                     /**< The internal force, which contains the gravity and other pulling and pushing forces. */
            MCore::Vector3  mExternalForce;             /**< A user defined external force, which is added on top of the internal force. Can be used to simulate wind etc. */
            MCore::Vector3  mInterpolateStartPos;       /**< The start interpolation position, used for interpolation towards the target over time when processing multiple time steps. */

            float           mConeAngleLimit;        /**< The conic angular limit, in degrees. This defaults on 180, which allows any rotation. This is the angle it allows to deviate from the original input pose (from the animation or bind pose). */
            float           mInvMass;               /**< The inverse mass, which equals 1.0f / mass. This is the inverse weight of the bone. Default value is 1.0. */
            float           mStiffness;             /**< The stiffness of the bone. A value of 0.0 means no stiffness at all. A value of say 50 would make it move back into its original pose. Making things look bouncy. Default value is 0.0. */
            float           mDamping;               /**< The damping value, which defaults on 0.001f. A value of 0 would mean the bone would oscilate forever. Higher values make it come to rest sooner. */
            float           mGravityFactor;         /**< The factor with which the gravity force will be multiplied with. Default is 1.0. A value of 2.0 means two times the amount of gravity being applied to this particle. */
            float           mFriction;              /**< The friction factor, between 0 and 1, which happens when a collision happens with a surface. The default is 0.0f. */
            uint32          mNodeIndex;             /**< The node index inside the Actor object, which is represented by this particle. */
            bool            mFixed;                 /**< This is set to false on default, which means it is a movable particle. When set to true, the particle will stick to the input position of the node. This would make it stick to the body. */
            uint8           mGroupID;               /**< Some optional particle group ID (default=0). You can use it to identify specific groups of particles/simulated nodes. You could for example group all hair particles together, and change the settings only for those particles. */

            // possible future settings
            //float mYawLimitMin;
            //float mYawLimitMax;
            //float mPitchLimitMin;
            //float mPitchLimitMax;
            //uint8 mYawAxis;       // 0/1/2
            //uint8 mPitchAxis;     // 0/1/2

            /**
             * The constructor which inits all values on their defaults.
             */
            Particle();
            ~Particle() {}
        };

        /**
         * A collision object.
         * Collision objects can be added with methods like AddCollisionSphere and AddCollisionCapsule.
         * They prevent the bones from penetrating the meshes.
         */
        class EMFX_API CollisionObject
        {
            friend class EMotionFX::JiggleController;
        public:
            /**
             * The collision object type.
             * This can be something like a sphere or capsule, etc.
             */
            enum ECollisionType
            {
                COLTYPE_SPHERE      = 0,        /**< A sphere, which is described by a center position (mStart) and a radius. */
                COLTYPE_CAPSULE     = 1         /**< A capsule, which is described by a start (mStart) and end (mEnd) position, and a diameter (radius). */
            };

            /**
             * The constructor.
             * This creates a sphere at (0.0f, 0.0f, 0.0f) with a radius of 1, and not linked to any node.
             */
            CollisionObject();

            /**
             * The destructor.
             */
            ~CollisionObject() {}

            /**
             * Setup this collision object as a sphere.
             * @param center The center of the sphere. This is in local space of the node you link this collision object to. If you do not link it to any node, it is in global space.
             * @param radius The radius of the sphere.
             * @see LinkToNode
             */
            void SetupSphere(const MCore::Vector3& center, float radius);

            /**
             * Setup this collision object as a capsule.
             * The capsule is defined by a thickness and start and end position.
             * Please keep in mind that the actual capsule goes beyond the start and end position, for "diameter" number of units.
             * The hemisphere parts at the start and end of the capsule will start from the given positions you specify to this methods parameters.
             * @param startPos The start position of the center line inside of the capsule. This is in local space of the node you link this collision object to. If you do not link it to any node, it is in global space.
             * @param endPos The end position of the center line inside of the capsule. This is in local space of the node you link this collision object to. If you do not link it to any node, it is in global space.
             * @param diameter The thickness of the capsule. This is how many units to the left and right (from top view) the capsule will go from the center line. Basically a value of 5 means the total thickness is 10 units.
             * @see LinkToNode
             */
            void SetupCapsule(const MCore::Vector3& startPos, const MCore::Vector3& endPos, float diameter);

            /**
             * Link or attach the collision object to a given node.
             * This will make the collision object move and rotate with the node it is attached to.
             * @param nodeIndex The node index of the node to which to link or attach this node to.
             */
            MCORE_INLINE void LinkToNode(uint32 nodeIndex)                  { mNodeIndex = nodeIndex; }

            /**
             * Unlink or detach the collision object from the node it might be attached to.
             * This sets its node index to a value of MCORE_INVALIDINDEX32, which identifies the object isn't linked to any  node.
             */
            MCORE_INLINE void Unlink()                                      { mNodeIndex = MCORE_INVALIDINDEX32; }

            /**
             * Adjust the start position of the collision object.
             * In case of a sphere this is the center of the sphere.
             * If you link this collision object to a node, the position is in space of the node it is attached to.
             * @param start The start position of the collision object, or in case of a sphere, the center.
             */
            MCORE_INLINE void SetStart(const MCore::Vector3& start)         { mStart = start; }

            /**
             * Adjust the end position of the collision object.
             * In case of a sphere this value is ignored.
             * If you link this collision object to a node, the position is in space of the node it is attached to.
             * @param end The end position of the collision object.
             */
            MCORE_INLINE void SetEnd(const MCore::Vector3& end)             { mEnd = end; }

            /**
             * Set the radius, diameter or thickness of the collision object.
             * @param radiusOrDiameter The radius, diameter or thickness value of the collision object.
             */
            MCORE_INLINE void SetRadius(float radiusOrDiameter)             { mRadius = radiusOrDiameter; }

            /**
             * Set the collision object type.
             * This determines internally what collision routines to use.
             * On default the collision object type is a sphere (COLTYPE_SPHERE).
             * @param newType The collision object type.
             */
            MCORE_INLINE void SetType(ECollisionType newType)               { mType = newType; }

            /**
             * Get the start position of the collision object.
             * In case of a sphere this is the center of the sphere.
             * Also in case this object is linked to a given node, it is the position in local space of the node.
             * @result The start position of the collision object.
             */
            MCORE_INLINE const MCore::Vector3& GetStart() const             { return mStart; }

            /**
             * Get the end position of the collision object.
             * In case of a sphere this value is ignored.
             * Also in case this object is linked to a given node, it is the position in local space of the node.
             * @result The end position of the collision object.
             */
            MCORE_INLINE const MCore::Vector3& GetEnd() const               { return mEnd; }

            /**
             * Get the global space start position of this collision object.
             * In case of a sphere this value is the center of the sphere, in global space.
             * In case of a capsule this is the start position of the capsule, in global space, unlike the value returned by GetStart(), which is in local space of the node it is attached to.
             * @result The global space start position of this collision object.
             */
            MCORE_INLINE const MCore::Vector3& GetGlobalStart() const       { return mGlobalStart; }

            /**
             * Get the global space end position of this collision object.
             * In case of a sphere this value is unused.
             * In case of a capsule this is the end position of the capsule, in global space, unlike the value returned by GetEnd(), which is in local space of the node it is attached to.
             * @result The global space end position of this collision object.
             */
            MCORE_INLINE const MCore::Vector3& GetGlobalEnd() const         { return mGlobalEnd; }

            /**
             * Get the radius or diameter of the collision object.
             * In case of a capsule this is half of the thickness of the capsule.
             * @result The radius or thickness or diameter of the collision object, depending on the collision object type.
             */
            MCORE_INLINE float GetRadius() const                            { return mRadius; }

            /**
             * Get the parent node index of this collision object.
             * When the parent node is MCORE_INVALIDINDEX32 it means it isn't linked to any node.
             * You can also use the HasParentNode() method to easily check this.
             * @result The index of the node where this collision object is linked to, or MCORE_INVALIDINDEX32 when it isn't linked to any node.
             */
            MCORE_INLINE uint32 GetParentNode() const                       { return mNodeIndex; }

            /**
             * Check if this collision object is linked to a node or not.
             * If it is linked to a node, it is also positioned relative to that node.
             * @result Returns true when the collision object is linked to a node. False is returned when it not linked to any node.
             */
            MCORE_INLINE bool GetHasParentNode() const                      { return (mNodeIndex != MCORE_INVALIDINDEX32); }

            /**
             * Get the type of collision object.
             * This will effect which collision detection check will be performed internally.
             * @result The collision object type.
             */
            MCORE_INLINE ECollisionType GetType() const                     { return mType; }

        private:
            MCore::Vector3  mGlobalStart;               /**< The global space start position, or the global space center in case of a sphere. */
            MCore::Vector3  mGlobalEnd;                 /**< The global space end position. This is ignored in case of a sphere. */
            MCore::Vector3  mStart;                     /**< The start of the primitive. In case of a sphere the center, in case of a capsule the start of the capsule. */
            MCore::Vector3  mEnd;                       /**< The end position of the primitive. In case of a sphere this is ignored. */
            MCore::Vector3  mInterpolationStart;        /**< The start of the primitive. In case of a sphere the center, in case of a capsule the start of the capsule. */
            MCore::Vector3  mInterpolationEnd;          /**< The end position of the primitive. In case of a sphere this is ignored. */
            float           mRadius;                    /**< The radius or thickness. */
            uint32          mNodeIndex;                 /**< The node index to attach to, or MCORE_INVALIDINDEX32 for non-attached. */
            ECollisionType  mType;                      /**< The collision primitive type (a sphere, or capsule, etc). */
            bool            mFirstUpdate;               /**< Is this the first update? */
        };

        /**
         * The creation method.
         * After creating the controller, use the StartNewchain(), followed by a set of AddNode method calls to add nodes to the simulation.
         * Please keep in mind that you need to first add parents and then their children, and not the other way around.
         * After that, you can call the Activate method, which will start enabling the controller.
         * @param actorInstance The actor instance to apply this controller on.
         */
        static JiggleController* Create(ActorInstance* actorInstance);

        /**
         * Get the unique type ID of this controller.
         * This will return the value of JiggleController::TYPE_ID.
         * @result The unique type ID of the controller.
         */
        uint32 GetType() const override;

        /**
         * Get the unique type string ID of the controller.
         * For this controller this will return "JiggleController", which is the class name.
         * @result The type string, identifying the type of the controller in form of a string.
         */
        const char* GetTypeString() const override;

        /**
         * Adjust the gravity force vector.
         * On default this is Vector3(0.0f, -9.81f, 0.0f).
         * To make gravity appear more strongly, multiply that vector with a given factor.
         * @param gravity The gravity force vector.
         */
        void SetGravity(const MCore::Vector3& gravity);

        /**
         * Get the gravity force vector.
         * On default this is Vector3(0.0f, -9.81f, 0.0f).
         * @result The gravity force vector.
         */
        const MCore::Vector3& GetGravity() const;

        /**
         * Get a given particle.
         * You can use this method to modify particle properties on the fly.
         * @param index The particle number, which must be in range of [0..GetNumParticles()-1].
         * @result A reference to the particle. You can modify the particle properties with this.
         * @see FindParticle
         */
        MCORE_INLINE Particle& GetParticle(uint32 index)            { return mParticles[index]; }

        /**
         * Get the number of particles in the system.
         * @result The number of particles.
         */
        MCORE_INLINE uint32 GetNumParticles() const                 { return mParticles.GetLength(); }

        /**
         * Get a given spring.
         * @param index The spring number, which must be in range of [0..GetNumSprings()-1].
         * @result A reference to the spring.
         */
        MCORE_INLINE Spring& GetSpring(uint32 index)                { return mSprings[index]; }

        /**
         * Get the number of springs.
         * @result The number of springs inside the system.
         */
        MCORE_INLINE uint32 GetNumSprings() const                   { return mSprings.GetLength(); }

        /**
         * Scale the particle and spring data. This modifies the particle stiffness and weight.
         * The spring rest lenghts are also adjusted by the scale factor.
         * Keep in mind that you cannot continuously call this function, as it multiplies the values with the provided scale factor.
         * Also keep in mind to call this after you initialized the Jiggle Controller. So after you add all the chains etc.
         * You can use this function when you scale your ActorInstance, by making it for example 10 times as small.
         * @param scaleFactor The scale factor.
         */
        void Scale(float scaleFactor);

        /**
         * Set the fixed timestep value, in seconds.
         * On default this value is set to (1.0f / 60.0f), which means an update rate of 60 fps.
         * Smaller timeSteps can be more stable and accurate, but in trade for performance.
         * This value may NEVER be zero.
         * @param timeStep The new fixed time step, in seconds.
         */
        void SetFixedTimeStep(float timeStep);

        /**
         * Get the fixed time step value, which is in seconds.
         * On default this value is set to (1.0f / 60.0f), which means an update rate of 60 fps.
         * Smaller timeSteps can be more stable and accurate, but in trade for performance.
         * @result The fixed time step value, in seconds.
         */
        float GetFixedTimeStep() const;

        /**
         * Set the number of iterations to use when trying to satisfy the spring constraints.
         * The default value for this property is 1.
         * Higher values can increase stability, but also effect performance.
         * The value will automatically be clamped between 1 and 50 for security reasons.
         * @param numIterations The number of update iterations. This value may NOT be zero.
         */
        void SetNumIterations(uint32 numIterations);

        /**
         * Get the number of constraint solving iterations.
         * The default value for this property is 1.
         * Higher values can increase stability, but also effect performance.
         * The value will automatically be clamped between 1 and 50 for security reasons.
         * @result The number of constraint solving iterations to be performed.
         */
        uint32 GetNumIterations() const;

        /**
         * Activate the controller.
         * Without calling this method the controller will never be activated and will never be processed.
         * @param fadeInTime The time, in seconds, to smoothly blend in the controller.
         */
        void Activate(float fadeInTime = 0.3f) override;

        /**
         * Mark the start of a new chain when adding nodes.
         * You have to call this every time before you start adding a new chain of nodes.
         * If you do not do this, it will think the last node of the last chain is connected to the next chain.
         */
        void StartNewChain();

        /**
         * Add a node (and spring) to the system.
         * It is extremely important that you add the springs in the right order.
         * First add the parent nodes, then the child nodes. So go from the top of the hierarchy to the bottom.
         * If you do not do this, you can see unexpected and incorrect behavior on some of the nodes.
         * A spring is formed between the node you specify to this function and its parent node.
         * Generally the first thing you do is add a fixed (pinned) node, followed by a set of non-fixed ones.
         * Also please remember to call StartNewChain() before starting adding nodes that form a new chain in the system.
         * @param nodeIndex The index of the node to add to the simulation.
         * @param isFixed The first node in the chain is most likely always to be fixed, so set it to true in that case. For all springs lower in the chain, set it to false.
         *                Setting it to true attaches it to the character. Setting it to false means it can freely move around. This is why the first node in the chain
         *                in most of the cases gets a 'true' as value, while all the nodes attached to that root node generally get a false.
         *                Another name for this is often 'pinned'. Set it to true to pin the particle to the body of the character, or false when it can move away from it.
         *                This setting will overwrite the isFixed setting from the copySettingsFrom particle that can be specified.
         * @param allowStretch Set to true when you allow the springs between the parent node and this node to stretch and compress. Set to false otherwise, which should keep the distance between them at fixed length.
         * @param copySettingsFrom A pointer to a particle object from which to copy the settings (except the isFixed and node index). When set to nullptr the new added particle will receive default settings.
         * @result A pointer to the particle that represents the added node. You can use it to apply further property changes, such as setting up angular rotation limits. nullptr is returned when the node index is invalid.
         * @see StartNewChain
         */
        JiggleController::Particle* AddNode(uint32 nodeIndex, bool isFixed, bool allowStretch, const JiggleController::Particle* copySettingsFrom = nullptr);

        /**
         * Add a node (and spring) to the system.
         * This works the same as the other AddNode method that takes a node index, except that this one uses a node name (non case-sensitive).
         * It is extremely important that you add the springs in the right order.
         * First add the parent nodes, then the child nodes. So go from the top of the hierarchy to the bottom.
         * If you do not do this, you can see unexpected and incorrect behavior on some of the nodes.
         * A spring is formed between the node you specify to this function and its parent node.
         * Generally the first thing you do is add a fixed (pinned) node, followed by a set of non-fixed ones.
         * Also please remember to call StartNewChain() before starting adding nodes that form a new chain in the system.
         * @param nodeName The name of the node to add to the simulation. If this node doesn't exist nullptr is returned. The node name is non case sensitive.
         * @param isFixed The first node in the chain is most likely always to be fixed, so set it to true in that case. For all springs lower in the chain, set it to false.
         *                Setting it to true attaches it to the character. Setting it to false means it can freely move around. This is why the first node in the chain
         *                in most of the cases gets a 'true' as value, while all the nodes attached to that root node generally get a false.
         *                Another name for this is often 'pinned'. Set it to true to pin the particle to the body of the character, or false when it can move away from it.
         *                This setting will overwrite the isFixed setting from the copySettingsFrom particle that can be specified.
         * @param allowStretch Set to true when you allow the springs between the parent node and this node to stretch and compress. Set to false otherwise, which should keep the distance between them at fixed length.
         * @param copySettingsFrom A pointer to a particle object from which to copy the settings (except the isFixed and node index). When set to nullptr the new added particle will receive default settings.
         * @result A pointer to the particle that represents the added node. You can use it to apply further property changes, such as setting up angular rotation limits. nullptr is returned when the node index is invalid.
         * @see StartNewChain
         */
        JiggleController::Particle* AddNode(const char* nodeName, bool isFixed, bool allowStretch, const JiggleController::Particle* copySettingsFrom = nullptr);

        /**
         * Add a chain to the controller.
         * This adds all nodes, starting from the start node, all the way down to the end node. Both the start and end nodes are included as well.
         * A pre-condition is that there is a valid path between the start and end node. The start node can be the upper arm, while the end node is for example the hand.
         * So the start node must be closer to the root node of the character, hierarchy wise.
         * The start node cannot be the same as the end node either, as we need at least two nodes to be added to form a chain.
         * If somehow the chain is invalid (if there is no valid path between the nodes, or you didn't specify two different nodes) then no chain will be added and false is returned.
         * On success this function will return true.
         * All nodes (particles) that will be added to the controller will copy over settings from an existing particle object that you can optionally specify.
         * @param startNodeIndex The index of the start node, for example the upper arm.
         * @param endNodeIndex The index of the end node, for example the hand.
         * @param startNodeIsFixed When set to true the start node will be marked as fixed (unmovable). This will make it stick to the body of the character.
         * @param allowStretch Set to true when you allow the springs in this chain to stretch and compress. Set to false otherwise, which should keep them at fixed length.
         * @param copySettingsFrom An optional pointer to a particle from which to copy the properties from (stiffness, damping, mass, etc.). Those settings will be applied to all nodes added.
         *                         If you set this parameter to nullptr, the particles will receive default values as defined by the JiggleController::Particle constructor.
         * @result Returns true when the chain was added, or false when the chain is not a valid chain.
         */
        bool AddChain(uint32 startNodeIndex, uint32 endNodeIndex, bool startNodeIsFixed = true, bool allowStretch = false, const JiggleController::Particle* copySettingsFrom = nullptr);

        /**
         * Add a chain to the controller.
         * This adds all nodes, starting from the start node, all the way down to the end node. Both the start and end nodes are included as well.
         * A pre-condition is that there is a valid path between the start and end node. The start node can be the upper arm, while the end node is for example the hand.
         * So the start node must be closer to the root node of the character, hierarchy wise.
         * The start node cannot be the same as the end node either, as we need at least two nodes to be added to form a chain.
         * If somehow the chain is invalid (if there is no valid path between the nodes, or you didn't specify two different nodes) then no chain will be added and false is returned.
         * On success this function will return true.
         * All nodes (particles) that will be added to the controller will copy over settings from an existing particle object that you can optionally specify.
         * @param startNodeName The name of the start node, for example the upper arm. This is not case sensitive.
         * @param endNodeName The name of the end node, for example the hand. This is not case sensitive.
         * @param startNodeIsFixed When set to true the start node will be marked as fixed (unmovable). This will make it stick to the body of the character.
         * @param allowStretch Set to true when you allow the springs in this chain to stretch and compress. Set to false otherwise, which should keep them at fixed length.
         * @param copySettingsFrom An optional pointer to a particle from which to copy the properties from (stiffness, damping, mass, etc.). Those settings will be applied to all nodes added.
         *                         If you set this parameter to nullptr, the particles will receive default values as defined by the JiggleController::Particle constructor.
         * @result Returns true when the chain was added, or false when the chain is not a valid chain.
         */
        bool AddChain(const char* startNodeName, const char* endNodeName, bool startNodeIsFixed = true, bool allowStretch = false, const JiggleController::Particle* copySettingsFrom = nullptr);

        /**
         * Add a support spring to the system.
         * Support springs can be springs that are linked between nodes of different chains.
         * Think of a grid that has two vertical chains of bones inside it. In order to make horizontal links
         * between those chains we must add support springs. They can try to keep the shape of the surface more intact.
         * Support springs can be made between any node.
         * @param nodeA The index of the first node.
         * @param nodeB The index of the second node.
         * @param restLength The rest length of the spring. When set to a negative value, this is automatically calculated from the hierarchy.
         * @result Returns true when successful, or false when one of the nodes cannot be found.
         */
        bool AddSupportSpring(uint32 nodeA, uint32 nodeB, float restLength = -1.0f);

        /**
         * Add a support spring to the system.
         * Support springs can be springs that are linked between nodes of different chains.
         * Think of a grid that has two vertical chains of bones inside it. In order to make horizontal links
         * between those chains we must add support springs. They can try to keep the shape of the surface more intact.
         * Support springs can be made between any node.
         * @param nodeNameA The name of the first node. This is not case sensitive.
         * @param nodeNameB The name of the second node. This is not case sensitive.
         * @param restLength The rest length of the spring. When set to a negative value, this is automatically calculated from the hierarchy.
         * @result Returns true when successful, or false when one of the nodes cannot be found.
         */
        bool AddSupportSpring(const char* nodeNameA, const char* nodeNameB, float restLength = -1.0f);

        /**
         * Remove a given node from the simulation.
         * This automatically removes the particle internally that represents this node, and also automatically removes all springs that use this node.
         * Values returned by GetNumParticles() and GetNumSprings() can change after execution of this method.
         * Also keep in mind that you shouldn't store any pointers to particles or springs, as their memory locations can change after adding new nodes and chains and support springs
         * or after removing them.
         * @param nodeIndex The index of the node to remove from the simulation.
         * @result Returns true in case removal was succesfully completed, or false when the node cannot be found.
         */
        bool RemoveNode(uint32 nodeIndex);

        /**
         * Remove a given node from the simulation, by using a node name.
         * This automatically removes the particle internally that represents this node, and also automatically removes all springs that use this node.
         * Values returned by GetNumParticles() and GetNumSprings() can change after execution of this method.
         * Also keep in mind that you shouldn't store any pointers to particles or springs, as their memory locations can change after adding new nodes and chains and support springs
         * or after removing them.
         * @param nodeName The name of the node to remove from the simulation. This is not case sensitive.
         * @result Returns true in case removal was succesfully completed, or false when the node cannot be found.
         */
        bool RemoveNode(const char* nodeName);

        /**
         * Remove a given spring from the simulation.
         * This does not remove the particles/nodes from the simulation.
         * The values returned by GetNumSprings() can change after execution of this method.
         * Also keep in mind that you shouldn't store any pointers to particles or springs, as their memory locations can change after adding new nodes and chains and support springs
         * or after removing them.
         * You can specify two node indices, which are used by the spring. It doesn't matter if the first node you specify is actually the second node in the spring.
         * This function automatically handles that.
         * @param nodeIndexA The index of the first node used by the spring.
         * @param nodeIndexB The index of the second node used by the spring.
         * @result Returns true in case removal was succesfully completed, or false when no springs/particles were found that use the specified nodes.
         */
        bool RemoveSupportSpring(uint32 nodeIndexA, uint32 nodeIndexB);

        /**
         * Remove a given spring from the simulation, using two node names.
         * This does not remove the particles/nodes from the simulation.
         * The values returned by GetNumSprings() can change after execution of this method.
         * Also keep in mind that you shouldn't store any pointers to particles or springs, as their memory locations can change after adding new nodes and chains and support springs
         * or after removing them.
         * You can specify two node indices, which are used by the spring. It doesn't matter if the first node you specify is actually the second node in the spring.
         * This function automatically handles that.
         * @param nodeNameA The name of the first node used by the spring.
         * @param nodeNameB The name of the second node used by the spring.
         * @result Returns true in case removal was succesfully completed, or false when no springs/particles were found that use the specified nodes.
         */
        bool RemoveSupportSpring(const char* nodeNameA, const char* nodeNameB);

        /**
         * Update the controller.
         * This is automatically called by EMotion FX. Do not manually call this when not needed.
         * @param outPose The input and output global space matrix buffer.
         * @param timePassedInSeconds The amount of seconds passed since the last update.
         */
        void Update(GlobalPose* outPose, float timePassedInSeconds) override;

        /**
         * Clone the controller.
         * @param targetActorInstance The actor instance where the clone will be added to.
         * @result A pointer to an exact copy of this controller.
         */
        GlobalSpaceController* Clone(ActorInstance* targetActorInstance) override;

        /**
         * Find a given particle which represents a given node in the actor.
         * @param nodeIndex The node index to search for.
         * @result The particle index, in range of [0..GetNumParticles()-1] which uses the specified node index, or MCORE_INVALIDINDEX32 when no particle uses the specified node.
         */
        uint32 FindParticle(uint32 nodeIndex) const;

        /**
         * Find a given particle that uses a given node name.
         * @param nodeName The node name to search for. The name is not case sensitive.
         * @result A pointer to the particle that represents the node with the given name, or nullptr when no such particle cannot be found.
         */
        JiggleController::Particle* FindParticle(const char* nodeName);

        //------

        /**
         * Add a collision sphere to the controller.
         * Collision will prevent chains from going inside those objects.
         * @param nodeIndex The node to attach this sphere to. Please note that the position of the object will become relative to the node you attach it to.
         *                  You can use MCORE_INVALIDINDEX32 if you do not want to attach the sphere to any node.
         * @param center The sphere center, relative to the node it is attached to. This can also mean that the up axis in global space is not really the up axis anymore, depending
         *               on the transformation of the node you attach to. If you specified MCORE_INVALIDINDEX32 as nodeIndex it is in global space instead of local space.
         * @param radius The radius of the sphere, in units.
         */
        void AddCollisionSphere(uint32 nodeIndex, const MCore::Vector3& center, float radius);

        /**
         * Add a collision capsule to the controller.
         * Collision will prevent chains from going inside those objects.
         * @param nodeIndex The node to attach this capsule to. Please note that the start and end position of the object will become relative to the node you attach it to.
         *                  You can use MCORE_INVALIDINDEX32 if you do not want to attach the capsule to any node.
         * @param start The start position of the capsule. Please note that the capsule goes beyond the start and end point, for "diameter" units. However the hemisphere parts of the capsule will start from this point.
         * @param end The end position of the capsule. Please note that the capsule goes beyond the start and end point, for "diameter" units. However the hemisphere parts of the capsule will start from this point.
         * @param diameter The diameter of the capsule. Actually this would be half of it. If you specify 5 here, it would be 5 units on all sides from the center. So 10 in total.
         */
        void AddCollisionCapsule(uint32 nodeIndex, const MCore::Vector3& start, const MCore::Vector3& end, float diameter);

        /**
         * Remove a collision object.
         * @param index The collision object to remove, which must be in range of [0..GetNumCollisionObjects()-1].
         */
        MCORE_INLINE void RemoveCollisionObject(uint32 index)           { mCollisionObjects.Remove(index); }

        /**
         * Remove all collision objects.
         */
        MCORE_INLINE void RemoveAllCollisionObjects()                   { mCollisionObjects.Clear(); }

        /**
         * Get a given collision object.
         * You can use this to modify its parameters.
         * Please keep in mind that its not always safe to store a reference to the collision object.
         * This is because when you add new objects, the actual memory locations can change, so please always re-get the collision objects rather than storing some
         * reference or pointer to them.
         * @param index The collision object number to get, which must be in range of [0..GetNumCollisionObjects()-1].
         * @result A reference to the collision object.
         */
        MCORE_INLINE CollisionObject& GetCollisionObject(uint32 index)  { return mCollisionObjects[index]; }

        /**
         * Get the number of collision objects.
         * On default there are no collision objects yet, after creation of the controller.
         * @result The number of collision objects.
         */
        MCORE_INLINE uint32 GetNumCollisionObjects() const              { return mCollisionObjects.GetLength(); }

        /**
         * Check whether collision detection is enabled for this controller or not.
         * On default collision detection is enabled.
         * @result Returns true when collision detection is enabled, and false if it isn't.
         */
        MCORE_INLINE bool GetCollisionEnabled() const                   { return mCollisionDetection; }

        /**
         * Enable or disable collision detection for this controller.
         * On default collision detection is enabled, after creation of the controller.
         * @param enabled Set this to true when you want to enable collision detection, or set it to false when you wish to disable it.
         */
        MCORE_INLINE void SetCollisionEnabled(bool enabled)             { mCollisionDetection = enabled; }

    private:
        MCore::Array<Spring>            mSprings;               /**< The collection of springs in the system. */
        MCore::Array<Particle>          mParticles;             /**< The particles, which are connected by springs. */
        MCore::Array<CollisionObject>   mCollisionObjects;      /**< The array of collision objects. */
        MCore::Vector3                  mGravity;               /**< The gravity force vector, which is (0.0f, -9.81f, 0.0f) on default. */
        float                           mForceBlendTime;        /**< The force blend in time, in seconds. The default is 1. */
        float                           mCurForceBlendTime;     /**< The current force blend time. This will go from 0 up to the mForceBlendTime. */
        float                           mForceBlendFactor;      /**< The force blend factor, which is a value between 0..1. It is used to prevent the spring system to go crazy in the first few frames after activation of the controller. */
        float                           mFixedTimeStep;         /**< The fixed time step value, to update the simulation with. The default is (1.0 / 60.0), which updates the simulation at 60 fps. */
        float                           mLastTimeDelta;         /**< The previous time delta. */
        float                           mTimeAccumulation;      /**< The accumulated time of the updates, used for fixed timestep processing. */
        uint32                          mNumIterations;         /**< The number of iterations to run the constraint solving routines. The default is 1. */
        uint32                          mParentParticle;        /**< The parent particle of the one you add next. Set ot MCORE_INVALIDINDEX32 when there is no parent particle yet. */
        bool                            mCollisionDetection;    /**< Perform collision detection? Default is true. */
        bool                            mStableUpdates;         /**< Use stable updates by doing pure fixed time step updates? (fixed time step must be smaller than actual frame time to make this work the best). */

        /**
         * The constructor.
         * After creating the controller, use the StartNewchain(), followed by a set of AddNode method calls to add nodes to the simulation.
         * Please keep in mind that you need to first add parents and then their children, and not the other way around.
         * After that, you can call the Activate method, which will start enabling the controller.
         * @param actorInstance The actor instance to apply this controller on.
         */
        JiggleController(ActorInstance* actorInstance);

        /**
         * The destructor.
         */
        ~JiggleController();

        /**
         * Perform the physics integration on the particles.
         * @param timeDelta The time step to use during integration.
         */
        void Integrate(float timeDelta);

        /**
         * Calculate the forces that will be applied to the particles.
         * This updates the JiggleController::Particle::mForce values.
         * @param globalMatrices The global space matrices used to get the fixed particle positions from.
         * @param interpolationFraction The linear interpolation fraction in range of [0..1] used internally for interpolating some values during multiple fixed time step updates.
         */
        void CalcForces(const MCore::Matrix* globalMatrices, float interpolationFraction);

        /**
         * Try to satisfy the spring constraints.
         * This will pull particles together, or push them away from eachother and it also solves rotational limits.
         * @param globalMatrices The global space matrices used to get the fixed particle positions from.
         * @param interpolationFraction The linear interpolation fraction in range of [0..1] used internally for interpolating some values during multiple fixed time step updates.
         */
        void SatisfyConstraints(const MCore::Matrix* globalMatrices, float interpolationFraction);

        /**
         * Perform a full simulation step.
         * This calculates forces, performs integration and tries to satisfy the constraints.
         * @param deltaTime The time step to use during integration.
         * @param globalMatrices The global space matrices.
         * @param interpolationFraction The linear interpolation fraction in range of [0..1] used internally for interpolating some values during multiple fixed time step updates.
         */
        void Simulate(float deltaTime, const MCore::Matrix* globalMatrices, float interpolationFraction);

        /**
         * Update the global space matrices based on the particles and springs as they currently are.
         * @param globalMatrices The input and output global space matrices.
         */
        void UpdateGlobalMatrices(MCore::Matrix* globalMatrices);

        /**
         * Add a particle to the system.
         * @param nodeIndex The node that this particle represents.
         * @param copySettingsFrom The particle to copy the settings from. When set to nullptr it will use its default values.
         * @result The particle index of the newly created particle.
         */
        uint32 AddParticle(uint32 nodeIndex, const JiggleController::Particle* copySettingsFrom = nullptr);

        /**
         * Collide a given position with a sphere.
         * @param pos The input and output position. This position will be modified when a collision occurs.
         * @param center The center of the sphere.
         * @param radius The radius of the sphere.
         * @result Returns true when a collision occurred, which means the input position is also modified and pushed outside of the sphere. False is returned when there is no collision.
         */
        bool CollideWithSphere(MCore::Vector3& pos, const MCore::Vector3& center, float radius);

        /**
         * Collide a given position with a capsule.
         * @param pos The input and output position. The position will be modified when a collision occurs.
         * @param lineStart The start of the center line of the capsule.
         * @param lineEnd The end of the center line of the capsule.
         * @param radius The radius or diameter of the capsule. In a top view this would be how many units to the left and right the capsule would be. So half of its thickness.
         * @result Returns true when a collision occurred, which means the input position is also modified and pushed outside of the capsule. False is returned when there is no collision.
         */
        bool CollideWithCapsule(MCore::Vector3& pos, const MCore::Vector3& lineStart, const MCore::Vector3& lineEnd, float radius);

        /**
         * Update all collision objects.
         * This will calculate and update the global space start and end positions of the collision objects.
         * When a collision object is attached to a node its positions are relative to the node it is attached to.
         * The actual global space positions are calculated and updated inside this method.
         * @param globalMatrices The global space matrix buffer, which holds the transformations for all nodes.
         * @param interpolationFraction The linear interpolation fraction in range of [0..1] used internally for interpolating some values during multiple fixed time step updates.
         */
        void UpdateCollisionObjects(const MCore::Matrix* globalMatrices, float interpolationFraction);

        /*
         * Prepare the collision objects for interpolation.
         * @param globalMatrices The current input global space matrices of the actor instance.
         */
        void PrepareCollisionObjects(const MCore::Matrix* globalMatrices);

        /**
         * Perform collision detection on a given position.
         * This will perform collision detection with all collision objects.
         * @param inOutPos The position that will be checked and modified in case a collision occurs.
         * @result Returns true when a collision occurred, or false when not.
         */
        bool PerformCollision(MCore::Vector3& inOutPos);
    };
}   // namespace EMotionFX
