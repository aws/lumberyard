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

#include <AzCore/Math/Vector3.h>

#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/WorldBody.h>


namespace Physics
{
    class Character;

    /// Represents a single contact point in a character contact manifold.
    struct CharacterContact
    {
        AZ::Vector3 m_point = AZ::Vector3::CreateZero(); ///< World-space location of the contact.
        AZ::Vector3 m_normal = AZ::Vector3::CreateZero(); ///< World-space surface normal at point of contact.
        AZ::Vector3 m_velocity = AZ::Vector3::CreateZero(); ///< World-space velocity at point of contact.
        AZStd::shared_ptr<WorldBody> m_otherBody; ///< Pointer to the other body, if available.
        float m_planeDistance = 0.f; ///< Penetration depth of this contact
        bool m_isMaxSlopeBlocker = false; ///< Contact is due to a surface exceeding the max slope of the character.
        bool m_ignoreContact = false; ///< Set to true to tell the character solver to ignore the contact.
    };

    /// Represents a collision manifold for a character, providing access to individual contacts.
    struct CharacterManifold
    {
        AZStd::vector<CharacterContact> m_contacts;
    };

    /// Structure describing a character-to-object interaction.
    struct CharacterObjectInteraction
    {
        CharacterObjectInteraction(const AZStd::shared_ptr<WorldBody>& body, const AZ::Vector3& point, const AZ::Vector3& normal, float impulseMagnitude)
            : m_body(body)
            , m_point(point)
            , m_normal(normal)
            , m_impulseMagnitude(impulseMagnitude)
            , m_ignoreCollision(false)
        {
        }

        AZStd::shared_ptr<WorldBody> m_body; ///< Other body the character has interacted with.
        const AZ::Vector3&  m_point; ///< World-space location of the contact point.
        const AZ::Vector3&  m_normal; ///< World-space normal at the contact point.
        float m_impulseMagnitude; ///< Magnitude of the contact impulse .
        bool m_ignoreCollision; ///< Set to true to tell the character solver to ignore the contact
    };

    /// Structure describing a character-to-character interaction.
    struct CharacterCharacterInteraction
    {
        CharacterCharacterInteraction(const AZStd::shared_ptr<Character>& character)
            : m_character(character)
            , m_velocityScale(0.f)
            , m_ignoreCollision(false)
        {
        }

        AZStd::shared_ptr<Character> m_character; ///< The other character that the character to which the event handler is associated is interacting with.
        float m_velocityScale;
        bool m_ignoreCollision; ///< Set to true to tell the character solver to ignore the contact.
    };

    /// Base object for handling character based collision events.
    class CharacterEventHandler
    {
        friend class Character;

    public:
        /// The character has interacted with a physical object.
        /// \param eventInfo information about the interaction.
        virtual void ObjectInteraction(CharacterObjectInteraction& eventInfo) { (void)eventInfo; }

        /// The character has interacted with another character.
        /// \param eventInfo information about the interaction.
        virtual void CharacterInteraction(CharacterCharacterInteraction& eventInfo) { (void)eventInfo; }
    };

    /// Types to enumerate when a character is "standing" on something or not.
    enum class CharacterSupportLevel
    {
        Supported, ///< The character is supposed by a surface.
        Unsupported, ///< The character is not supported by a surface.
        Sliding, ///< The character is sliding on a steep slope.
    };

    /// Info regarding a character and the surface supporting it.
    class CharacterSupportInfo
    {
    public:

        CharacterSupportLevel m_supportLevel; ///< The character's support level.
        AZ::Vector3 m_normal; ///< The normal of the surface supporting the character (if supported).
        AZ::Vector3 m_velocity; ///< The velocity of the character at the point of contact with supporting surface (if supported).
        float m_distance; ///< The distance to nearest surface.
        AZStd::shared_ptr<WorldBody> m_mainCollidingBody; ///< The colliding body currently supporting the character (if supported).
    };

    class CharacterColliderNodeConfiguration
    {
    public:
        AZ_RTTI(CharacterColliderNodeConfiguration, "{C16F3301-0979-400C-B734-692D83755C39}");
        AZ_CLASS_ALLOCATOR_DECL

        virtual ~CharacterColliderNodeConfiguration() = default;

        static void Reflect(AZ::ReflectContext* context);

        AZStd::string m_name;
        ShapeConfigurationList m_shapes;
    };

    class CharacterColliderConfiguration
    {
    public:
        AZ_RTTI(CharacterColliderConfiguration, "{4DFF1434-DF5B-4ED5-BE0F-D3E66F9B331A}");
        AZ_CLASS_ALLOCATOR_DECL

        virtual ~CharacterColliderConfiguration() = default;

        CharacterColliderNodeConfiguration* FindNodeConfigByName(const AZStd::string& nodeName) const;
        AZ::Outcome<size_t> FindNodeConfigIndexByName(const AZStd::string& nodeName) const;

        void RemoveNodeConfigByName(const AZStd::string& nodeName);

        static void Reflect(AZ::ReflectContext* context);

        AZStd::vector<CharacterColliderNodeConfiguration> m_nodes;
    };

    /// Information required to create the basic physics representation of a character.
    class CharacterConfiguration
        : public WorldBodyConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(CharacterConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(CharacterConfiguration, "{58D5A6CA-113B-4AC3-8D53-239DB0C4E240}");

        virtual ~CharacterConfiguration() = default;

        static void Reflect(AZ::ReflectContext* context);

        CollisionGroups::Id m_collisionGroupId; ///< Which layers does this character collide with.
        CollisionLayer m_collisionLayer; ///< Which collision layer is this character on.
        MaterialSelection m_materialSelection; ///< Material selected from library for the body associated with the character.
        AZ::Vector3 m_upDirection = AZ::Vector3::CreateAxisZ(); ///< Up direction for character orientation and step behavior.
        float m_maximumSlopeAngle = 30.0f; ///< The maximum slope on which the character can move, in degrees.
        float m_stepHeight = 0.5f; ///< Affects what size steps the character can climb.
        float m_minimumMovementDistance = 0.001f; ///< To avoid jittering, the controller will not attempt to move distances below this.
        bool m_directControl = true; ///< If false, the character will move purely kinematically under external control.
                                     ///< Can be used, for example, to replicate the movement of a character in a cosmetic world.
    };

    /// Basic implementation of common character-style needs as a WorldBody. Is not a full-functional ship-ready
    /// all-purpose character controller implementation. This class just abstracts some common functionality amongst
    /// typical characters, and is take-it-or-leave it style; useful as a starting point or reference.
    class Character
        : public WorldBody
    {
    public:
        AZ_CLASS_ALLOCATOR(Character, AZ::SystemAllocator, 0);
        AZ_RTTI(Character, "{962E37A1-3401-4672-B896-0A6157CFAC97}", WorldBody);

        ~Character() override = default;

        virtual AZ::Vector3 GetBasePosition() const = 0;
        virtual void SetBasePosition(const AZ::Vector3& position) = 0;
        virtual void SetRotation(const AZ::Quaternion& rotation) = 0;
        virtual AZ::Vector3 GetCenterPosition() const = 0;
        virtual float GetStepHeight() const = 0;
        virtual void SetStepHeight(float stepHeight) = 0;
        virtual AZ::Vector3 GetUpDirection() const = 0;
        virtual void SetUpDirection(const AZ::Vector3& upDirection) = 0;
        virtual float GetSlopeLimitDegrees() const = 0;
        virtual void SetSlopeLimitDegrees(float slopeLimitDegrees) = 0;
        virtual AZ::Vector3 GetVelocity() const = 0;

        /// Tries to move the character by the requested position change relative to its current position.
        /// Obstacles may prevent the actual movement from exactly matching the requested movement.
        /// @param deltaPosition The change in position relative to the current position.
        /// @param deltaTime Elapsed time.
        /// @return The new base (foot) position.
        virtual AZ::Vector3 TryRelativeMove(const AZ::Vector3& deltaPosition, float deltaTime) = 0;

        virtual void CheckSupport(const AZ::Vector3& direction, float distance, const CharacterSupportInfo& supportInfo) = 0;

        virtual void RegisterEventHandler(const AZStd::shared_ptr<CharacterEventHandler>& /*handler*/) {};
        virtual void UnregisterEventHandler(const AZStd::shared_ptr<CharacterEventHandler>& /*handler*/) {};

        virtual void AttachShape(AZStd::shared_ptr<Physics::Shape> shape) = 0;
    };
} // namespace Physics
