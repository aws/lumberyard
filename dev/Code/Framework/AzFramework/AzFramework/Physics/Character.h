
#pragma once

#include <AzCore/Math/Vector3.h>

#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/WorldBody.h>

namespace Physics
{
    class Character;

    /**
     * Represents a single contact point in a character contact manifold.
     */
    struct CharacterContact
    {
        AZ::Vector3         m_point                 = AZ::Vector3::CreateZero();    ///< World-space location of the contact.
        AZ::Vector3         m_normal                = AZ::Vector3::CreateZero();    ///< World-space surface normal at point of contact.
        AZ::Vector3         m_velocity              = AZ::Vector3::CreateZero();    ///< World-space velocity at point of contact.
        ShapeIdHierarchy    m_shapeIdHierarchyOther;                                ///< Shape Id hierarchy of other object at point of contact.
        ShapeIdHierarchy    m_shapeIdHierarchySelf;                                 ///< Shape Id hierarchy of other object at point of contact.
        Ptr<WorldBody>      m_otherBody;                                            ///< Pointer to the other body, if available.
        float               m_planeDistance         = 0.f;                          ///< Penetration depth of this contact
        bool                m_isMaxSlopeBlocker     = false;                        ///< Contact is due to a surface exceeding the max slope of the character.
        bool                m_ignoreContact         = false;                        ///< Set to true to tell the character solver to ignore the contact.
    };

    /**
     * Represents a collision manifold for a character, providing access to individual contacts.
     */
    struct CharacterManifold
    {
        AZStd::vector<CharacterContact> m_contacts;
    };

    /**
     * Structure describing a character-to-object interaction.
     */
    struct CharacterObjectInteraction
    {
        CharacterObjectInteraction(const Ptr<WorldBody>& body, const AZ::Vector3& point, const AZ::Vector3& normal, float impulseMagnitude)
            : m_body(body)
            , m_point(point)
            , m_normal(normal)
            , m_impulseMagnitude(impulseMagnitude)
            , m_ignoreCollision(false)
        {
        }

        Ptr<WorldBody>      m_body;                 ///< Other body the character has interacted with.
        const AZ::Vector3&  m_point;                ///< World-space location of the contact point.
        const AZ::Vector3&  m_normal;               ///< World-space normal at the contact point.
        float               m_impulseMagnitude;     ///< Magnitude of the contact impulse .
        bool                m_ignoreCollision;      ///< Set to true to tell the character solver to ignore the contact
    };

    /**
     * Structure describing a character-to-character interaction.
     */
    struct CharacterCharacterInteraction
    {
        CharacterCharacterInteraction(const Ptr<Character>& character)
            : m_character(character)
            , m_velocityScale(0.f)
            , m_ignoreCollision(false)
        {
        }

        Ptr<Character>  m_character;                ///< The other character that the character to which the event handler is associated is interacting with.
        float           m_velocityScale;            ///< 
        bool            m_ignoreCollision;          ///< Set to true to tell the character solver to ignore the contact.
    };

    /**
     * Base object for handling character based collision events.
     */
    class CharacterEventHandler : public ReferenceBase
    {
        friend class Character;

    public:

        /// Allows adjustment of collision contact planes prior to passing to the physics solver.
        /// \param manifold character manifold containing contact planes.
        virtual void PreSolve(CharacterManifold& manifold) { (void)manifold; }

        /// The character has interacted with a physical object.
        /// \param eventInfo information about the interaction.
        virtual void ObjectInteraction(CharacterObjectInteraction& eventInfo) { (void)eventInfo; }

        /// The character has interacted with another character.
        /// \param eventInfo information about the interaction.
        virtual void CharacterInteraction(CharacterCharacterInteraction& eventInfo) { (void)eventInfo; }
    };

    /**
     * Types to enumerate when a character is "standing" on something or not.
     */
    enum class CharacterSupportLevel
    {
        Supported,      ///< The character is supposed by a surface.
        Unsupported,    ///< The character is not supported by a surface.
        Sliding,        ///< The character is sliding on a steep slope.
    };

    /**
     * Info regarding a character and the surface supporting it.
     */
    class CharacterSupportInfo
    {
    public:

        CharacterSupportLevel   m_supportLevel;         ///< The character's support level.
        AZ::Vector3             m_normal;               ///< The normal of the surface supporting the character (if supported).
        AZ::Vector3             m_velocity;             ///< The velocity of the character at the point of contact with supporting surface (if supported).
        float                   m_distance;             ///< The distance to nearest surface.
        Ptr<WorldBody>          m_mainCollidingBody;    ///< The colliding body currently supporting the character (if supported).
        ShapeIdHierarchy        m_shapeIdHierarchy;     ///< Shape Id hierarchy for contact with the supportin body.
    };

    /**
     * Basic character configuration settings.
     */
    class CharacterSettings : public WorldBodySettings
    {
    public:

        AZ_CLASS_ALLOCATOR(CharacterSettings, AZ::SystemAllocator, 0);
        AZ_RTTI(CharacterSettings, "{DCDA2D81-2456-490A-BA2D-CEFBA2F9D0E2}", WorldBodySettings);

        static Ptr<CharacterSettings> Create()
        {
            return aznew CharacterSettings();
        }

        AZ::Vector3                 m_upDirection           = AZ::Vector3::CreateAxisZ();
        float                       m_maxSlope              = AZ::Constants::HalfPi;
        Ptr<ShapeConfiguration>     m_characterShape        = nullptr;

    private:

        CharacterSettings()
        {}
    };

    /**
     * Basic implementation of common character-style needs as a WorldBody. Is not a full-functional ship-ready
     * all-purpose character controll implementation. This class just abstracts some common functionality amongst
     * typical characters, and is take-it-or-leave it style; useful as a starting point or reference.
     */
    class Character : public WorldBody
    {
    public:

        AZ_CLASS_ALLOCATOR(Character, AZ::SystemAllocator, 0);
        AZ_RTTI(Character, "{962E37A1-3401-4672-B896-0A6157CFAC97}", WorldBody);

        friend class AZStd::intrusive_ptr<Character>;

    public:

        virtual AZ::Vector3 GetBasePosition() const = 0;
        virtual void SetBasePosition(AZ::Vector3 position) = 0;
        virtual AZ::Vector3 GetCenterPosition() const = 0;

        virtual AZ::Vector3 GetVelocity() const = 0;
        virtual void SetDesiredVelocity(AZ::Vector3 velocity) = 0;
        virtual AZ::Vector3 GetDesiredVelocity() const = 0;

        virtual void CheckSupport(AZ::Vector3 direction, float distance, CharacterSupportInfo& supportInfo) = 0;
        virtual void Integrate(float deltaTime) = 0;

        virtual void RegisterEventHandler(const Ptr<CharacterEventHandler>& handler)
        {
            AZ_Assert(m_eventHandlers.end() == AZStd::find(m_eventHandlers.begin(), m_eventHandlers.end(), handler), "Event handler is already registered.");
            m_eventHandlers.emplace_back(handler);
        }

        virtual void UnregisterEventHandler(const Ptr<CharacterEventHandler>& handler)
        {
            auto iter = AZStd::find(m_eventHandlers.begin(), m_eventHandlers.end(), handler);
            if (iter != m_eventHandlers.end())
            {
                m_eventHandlers.erase(iter);
            }
        }

    protected:

        AZStd::vector<Ptr<CharacterEventHandler>> m_eventHandlers;

        Character()
            : WorldBody(CharacterSettings::Create())
        {
        }

        explicit Character(const Ptr<CharacterSettings>& settings)
            : WorldBody(settings)
        {
            if (settings && settings->m_characterShape)
            {
                m_shapeHierarchyMaterialMap = settings->m_characterShape->m_shapeHierarchyMaterialMap;
            }
        }

        ~Character() override = default;
    };
    
} // namespace Physics
