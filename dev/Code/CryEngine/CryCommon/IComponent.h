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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYCOMMON_ICOMPONENT_H
#define CRYINCLUDE_CRYCOMMON_ICOMPONENT_H
#pragma once

struct SEntityEvent;
struct IEntity;
class IComponent;


#include <CryFlags.h>
#include <Cry_Math.h>
#include <BoostHelpers.h>
#include <IMaterial.h>
#define DECLARE_COMPONENT_POINTERS(name)                \
    typedef std::shared_ptr<name> name##Ptr;            \
    typedef std::shared_ptr<const name> name##ConstPtr; \
    typedef std::weak_ptr<name> name##WeakPtr;          \
    typedef std::weak_ptr<const name> name##ConstWeakPtr;

typedef unsigned int EntityId;
DECLARE_COMPONENT_POINTERS(IComponent)

class ComponentType;
struct SEntityEvent;
struct SEntityUpdateContext;
struct SEntitySpawnParams;

//! Central location to specify serialization order of components.
namespace EntityEventPriority
{
    enum
    {
        User = 1,
        EntityNode,
        Rope,
        Trigger,
        Substitution,
        FlowGraph,
        Camera,
        BoidObject,
        Boids,
        Area,
        AI,
        Audio,
        Script,
        Physics,
        Render,
        GameObjectExtension
    };
}

struct IComponentEventDistributer
{
    enum
    {
        EVENT_ALL = 0xffffffff
    };

    virtual void RegisterEvent(const EntityId entityID, IComponentPtr pComponent, const int eventID, const bool bEnable) = 0;
    virtual ~IComponentEventDistributer() {}
};

//! Base interface for all entity components.
//! A component is responsible for some particular facet of entity behavior.
//! <p>
//! Components within an entity are identified based on their ComponentType.
//! The system currently supports one component of a given type per entity.
//! <p>
//! A ComponentType is defined using the DECLARE_COMPONENT_TYPE macro.
//! If a component implements an interface, and is identified based on that
//! interface, then the ComponentType should be declared on the interface.
//! For example, CComponentRender implements IComponentRender. Many different
//! engine modules interact with this component through the IComponentRender
//! interface. Therefore, the ComponentType is declared on IComponentRender
//! and all instances of CComponentRender use IComponentRender's ComponentType.
//! <p>
//! Components are created through an IComponentFactory<T>.
//! Each component of a given type is created through the same factory.
//! Each concrete component type must declare a global factory in a CPP file.
//! The factories are gathered into a central registry during program startup.
//! For a standard factory, use the DECLARE_DEFAULT_COMPONENT_FACTORY macro.
class IComponent
    : public std::enable_shared_from_this<IComponent>
{
public:
    typedef int ComponentEventPriority;
    typedef CCryFlags<uint> ComponentFlags;

    struct SComponentInitializer
    {
        SComponentInitializer(IEntity* pEntity)
            : m_pEntity(pEntity) {}
        IEntity* m_pEntity;
    };

    IComponent()
        : m_pComponentEventDistributer(nullptr)
        , m_componentEntityId(0) {}
    virtual ~IComponent() {}

    //! The DECLARE_COMPONENT_TYPE macro will implement this function for subclasses of IComponent.
    virtual const ComponentType& GetComponentType() const = 0;

    // <interfuscator:shuffle>
    // Description:
    //    By overriding this function component will be able to handle events sent from the host Entity.
    // Arguments:
    //    event - Event structure, contains event id and parameters.
    virtual void ProcessEvent(SEntityEvent& event) { }
    virtual ComponentEventPriority GetEventPriority(const int eventID) const { return EntityEventPriority::User; }

    //! Called immediately after a component is registered with an entity.
    //! Initialize() is called after the constructor and before InitComponent().
    //! <p>
    //! There is no guarantee that other components on the entity have been
    //! created at this point. It is acceptable for a component's Initialize()
    //! to cause the creation/registration/initialization of another
    //! component that it must communicate with.
    //!
    //! \param init Contains necessary initialization data, including a
    //!             reference to the entity that this component is
    //!             registered with. If a custom component requires more
    //!             information, create a subclass of SComponentInitializer
    //!             containing the extra data and pass that in to Initialize().
    virtual void Initialize(const SComponentInitializer& init) {}

    //! Called as the entity initializes. At this stage, all components
    //! have been created and had their Initialize() called.
    virtual bool InitComponent(IEntity* pEntity, SEntitySpawnParams& params) { return true; }

    //! Called as the entity is reloaded.
    virtual void Reload(IEntity* pEntity, SEntitySpawnParams& params) {}

    //! Called as the entity is being destroyed.
    //! At this point, all other components are valid.
    //! No memory should be deleted here!
    virtual void Done() {}

    // Adds up the amount of memory used by the component.
    virtual void GetMemoryUsage(ICrySizer* pSizer) const { }

    // Description:
    //    Update will be called every time the host Entity is updated.
    // Arguments:
    //    ctx - Update context of the host entity, provides all the information needed to update the entity.
    virtual void UpdateComponent(SEntityUpdateContext& ctx) {}

    // </interfuscator:shuffle>

    ComponentFlags& GetFlags() { return m_flags; }
    const ComponentFlags& GetFlags() const { return m_flags; }

    void SetDistributer(IComponentEventDistributer* piEntityEventDistributer, const EntityId entityID)
    {
        m_pComponentEventDistributer = piEntityEventDistributer;
        m_componentEntityId = entityID;
    }

protected:

    void RegisterEvent(const int eventID)
    {
        CRY_ASSERT(m_pComponentEventDistributer);
        if (m_pComponentEventDistributer)
        {
            m_pComponentEventDistributer->RegisterEvent(m_componentEntityId, shared_from_this(), eventID, true);
        }
    }

private:
    IComponentEventDistributer* m_pComponentEventDistributer;
    EntityId m_componentEntityId;
    ComponentFlags m_flags;
};

//////////////////////////////////////////////////////////////////////////
// Description:
//    Flags the can be set on each of the entity object slots.
//////////////////////////////////////////////////////////////////////////
enum EEntitySlotFlags
{
    ENTITY_SLOT_RENDER = 0x0001,  // Draw this slot.
    ENTITY_SLOT_RENDER_NEAREST = 0x0002,  // Draw this slot as nearest. [Rendered in camera space]
    ENTITY_SLOT_RENDER_WITH_CUSTOM_CAMERA = 0x0004,  // Draw this slot using custom camera passed as a Public ShaderParameter to the entity.
    ENTITY_SLOT_IGNORE_PHYSICS = 0x0008,  // This slot will ignore physics events sent to it.
    ENTITY_SLOT_BREAK_AS_ENTITY = 0x0010,
    ENTITY_SLOT_RENDER_AFTER_POSTPROCESSING = 0x0020,
    ENTITY_SLOT_BREAK_AS_ENTITY_MP = 0x0040, // In MP this an entity that shouldn't fade or participate in network breakage
    ENTITY_SLOT_RENDER_NEAREST_NO_ATTACH_CAMERA = 0x0080, // Had to add this flag for backward compatibility in CryAction view system
};

// Description of the contents of the entity slot.
struct SEntitySlotInfo
{
    // Slot flags.
    int nFlags;
    // Index of parent slot, (-1 if no parent)
    int nParentSlot;
    // Hide mask used by breakable object to indicate what index of the CStatObj sub-object is hidden.
    uint64 nSubObjHideMask;
    // Slot local transformation matrix.
    const Matrix34* pLocalTM;
    // Slot world transformation matrix.
    const Matrix34* pWorldTM;
    // Objects that can binded to the slot.
    EntityId                   entityId;
    struct IStatObj*           pStatObj;
    struct ICharacterInstance*   pCharacter;
    struct IParticleEmitter*   pParticleEmitter;
    struct ILightSource*      pLight;
    struct IRenderNode*      pChildRenderNode;
#if defined(USE_GEOM_CACHES)
    struct IGeomCacheRenderNode* pGeomCacheRenderNode;
#endif
    // Custom Material used for the slot.
    _smart_ptr<IMaterial> pMaterial;
};

struct IShaderParamCallback;
DECLARE_SMART_POINTERS(IShaderParamCallback);

#endif // CRYINCLUDE_CRYCOMMON_ICOMPONENT_H
