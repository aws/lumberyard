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
#include "CryLegacy_precompiled.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include "IInput.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
/// CFlowTouchBaseNode class
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
/// Base class for all touch event flowgraph nodes
////////////////////////////////////////////////////////////////////////////////////////////////////
class CFlowTouchBaseNode
    : public CFlowBaseNode<eNCT_Instanced>
    , public IInputEventListener
{
public:
    //! Makes sure the object is removed from its listener handles
    ~CFlowTouchBaseNode() override
    {
        UnregisterListeners();
    }


protected:
    //! Constructs the base touch event handler flowgraph node
    //! \param activationInfo    The basic information needed for initializing a flowgraph node, including but not
    //!                         limited to IEntity links, input/output connections, etc.
    CFlowTouchBaseNode(SActivationInfo* activationInfo)
        : CFlowBaseNode()
        , m_actInfo(*activationInfo)
        , m_enabled(false)
    {
    }

    //! Wrapper for adding the flow node to it's listener handlers
    virtual void RegisterListeners()
    {
        if (GetISystem() && GetISystem()->GetIInput())
        {
            GetISystem()->GetIInput()->AddEventListener(this);
        }
    }

    //! Wrapper for removing the flow node from it's listener handlers
    virtual void UnregisterListeners()
    {
        if (GetISystem() && GetISystem()->GetIInput())
        {
            GetISystem()->GetIInput()->RemoveEventListener(this);
        }
    }

    SActivationInfo m_actInfo; //!< A copy of the activation info used for construction
    bool m_enabled; //!< Flag indicating if the flow node should be active
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowTouchEventNode class
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
//! Flowgraph node specifically for sending out the touch events for finger id zero
class CFlowTouchEventNode
    : public CFlowTouchBaseNode
{
    enum InputPorts
    {
        Input_Enable = 0,
        Input_Disable,
        Input_TouchId,
        Input_ScreenCoords,
        Input_WorldCoords,
    };

    enum OutputPorts
    {
        Output_TouchDown = 0,
        Output_TouchUp,
        Output_ScreenCoordX,
        Output_ScreenCoordY,
        Output_WorldCoords,
    };

public:

    //! Constructor for a basic touch event node
    //! \param activationInfo   The basic information needed for initializing a flowgraph node, including but not
    //!                         limited to IEntity links, input/output connections, etc.
    CFlowTouchEventNode(SActivationInfo* activationInfo)
        : CFlowTouchBaseNode(activationInfo)
        , m_touchIdFilter(0)
        , m_shouldOutputScreenCoords(false)
        , m_shouldOutputWorldCoords(false)
    {
    }

    //! Mainly removes the touch event listener from the input system
    ~CFlowTouchEventNode() override
    {
    }


    // IFlowNode

    //! Handles read/write serialization of the flowgraph node
    //! \param activationInfo   Basic information about the flowgraph node, including IEntity, input, output, etc. connections
    //! \param ser              The I/O serializer object used for reading/writing
    void Serialize(SActivationInfo* activationInfo, TSerialize ser) override
    {
        ser.Value("enabled", m_enabled);
        ser.Value("touchIdFilter", m_touchIdFilter);
        ser.Value("shouldOutputScreenCoords", m_shouldOutputScreenCoords);
        ser.Value("shouldOutputWorldCoords", m_shouldOutputWorldCoords);

        // register listeners if this node was saved as enabled
        if (ser.IsReading() && m_enabled)
        {
            RegisterListeners();
        }
    }

    //! Puts the objects used in this module into the sizer interface
    //! \param sizer    Sizer object to track memory allocations
    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    //! Main accessor for getting the inputs/outputs and flags for the flowgraph node
    //! \param config   Ref to a flow node config struct for the node to fill out with its inputs/outputs
    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Enable", _HELP("Enable")),
            InputPortConfig_Void("Disable", _HELP("Disable")),
            InputPortConfig<int>("TouchId", 0, _HELP("The desired touch (finger) id for which events will be sent")),
            InputPortConfig<bool>("ScreenCoords", true, _HELP("Output screen space coordinates")),
            InputPortConfig<bool>("WorldCoords", false, _HELP("Output world space coordinates")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_AnyType("TouchDown", _HELP("Activation indicates a touch down happened from finger id zero")),
            OutputPortConfig_AnyType("TouchUp", _HELP("Activation indicates a touch up happened from finger id zero")),
            OutputPortConfig<int>("ScreenCoordX", _HELP("Screen space X pixel coord of the touch")),
            OutputPortConfig<int>("ScreenCoordY", _HELP("Screen space Y pixel coord of the touch")),
            OutputPortConfig<Vec3>("WorldCoords", _HELP("Unprojected touch pos in world coords")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Outputs touch events from the specified id (finger)");
        config.SetCategory(EFLN_APPROVED);
    }

    //! Clones the flowgraph node.  Clone is called instead of creating a new node if the node type is set to eNCT_Cloned
    //! \param activationInfo       The activation info from the flowgraph node that is being cloned from
    //! \return                     The newly cloned flowgraph node
    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowTouchEventNode(activationInfo);
    }

    //! Event handler specifically for flowgraph events
    //! \param event            The flowgraph event ID
    //! \param activationInfo   The node's updated activation data
    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        m_actInfo = *activationInfo;
        switch (event)
        {
        case eFE_Initialize:
        {
            m_touchIdFilter = GetPortInt(activationInfo, InputPorts::Input_TouchId);
            m_shouldOutputScreenCoords = GetPortBool(activationInfo, InputPorts::Input_ScreenCoords);
            m_shouldOutputWorldCoords = GetPortBool(activationInfo, InputPorts::Input_WorldCoords);
        }
        break;
        case eFE_Activate:
        {
            if (IsPortActive(activationInfo, InputPorts::Input_Enable))
            {
                m_touchIdFilter = GetPortInt(activationInfo, InputPorts::Input_TouchId);
                m_shouldOutputScreenCoords = GetPortBool(activationInfo, InputPorts::Input_ScreenCoords);
                m_shouldOutputWorldCoords = GetPortBool(activationInfo, InputPorts::Input_WorldCoords);

                RegisterListeners();
                m_enabled = true;
            }

            if (IsPortActive(activationInfo, InputPorts::Input_Disable))
            {
                UnregisterListeners();
                m_enabled = false;
            }

            if (IsPortActive(activationInfo, InputPorts::Input_TouchId))
            {
                m_touchIdFilter = GetPortInt(activationInfo, InputPorts::Input_TouchId);
            }

            if (IsPortActive(activationInfo, InputPorts::Input_ScreenCoords))
            {
                m_shouldOutputScreenCoords = GetPortBool(activationInfo, InputPorts::Input_ScreenCoords);
            }

            if (IsPortActive(activationInfo, InputPorts::Input_WorldCoords))
            {
                m_shouldOutputWorldCoords = GetPortBool(activationInfo, InputPorts::Input_WorldCoords);
            }
        }
        break;
        }
    }

    // ~IFlowNode


    // IInputEventListener

    //! Input event handler
    //! \param event        Information about the input event
    bool OnInputEvent(const SInputEvent& event) override
    {
        if (!m_enabled || event.deviceType != eIDT_TouchScreen)
        {
            return false;
        }

        // skip the event if its not the touch id we want
        const int touchId = event.keyId - eKI_Touch0;
        if (touchId != m_touchIdFilter)
        {
            return false;
        }

        // process the event
        bool sendCoords = false;
        switch (event.state)
        {
        case eIS_Pressed:
        {
            ActivateOutput(&m_actInfo, OutputPorts::Output_TouchDown, event.value);
            sendCoords = true;
        }
        break;
        case eIS_Released:
        {
            ActivateOutput(&m_actInfo, OutputPorts::Output_TouchUp, event.value);
            sendCoords = true;
        }
        break;
        case eIS_Down:
        {
            sendCoords = true;
        }
        break;
        }

        if (!sendCoords)
        {
            return false;
        }

        // send the world coordinates, if specified
        if (m_shouldOutputWorldCoords)
        {
            Vec3 worldPos;
            gEnv->pRenderer->UnProjectFromScreen(event.screenPosition.x, gEnv->pRenderer->GetHeight() - event.screenPosition.y, 1, &worldPos.x, &worldPos.y, &worldPos.z);
            ActivateOutput(&m_actInfo, OutputPorts::Output_WorldCoords, worldPos);
        }

        // send the screen coordinates, if specified
        if (m_shouldOutputScreenCoords)
        {
            ActivateOutput(&m_actInfo, OutputPorts::Output_ScreenCoordX, event.screenPosition.x);
            ActivateOutput(&m_actInfo, OutputPorts::Output_ScreenCoordY, event.screenPosition.y);
        }

        return false;
    }

    // ~IInputEventListener


private:
    int m_touchIdFilter; //!< Request which touch (finger) index's coordinates to be output (defaults to 0)
    bool m_shouldOutputScreenCoords; //!< Indicating if the 2D screen coordinates should be broadcast
    bool m_shouldOutputWorldCoords; //!< Indicating if the 3D world coordinates should be broadcast
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowTouchRayCast class
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
/// Flowgraph node for sending off touch up and touch down events
//////////////////////////////////////////////////////////////////////////
class CFlowTouchRayCast
    : public CFlowTouchBaseNode
    , public IGameFrameworkListener
{
    enum InputPorts
    {
        Enable = 0,
        Disable,
        RayType,
        EntitiesToIgnore,
        IgnoreBackfaces
    };

    enum OutputPorts
    {
        HitPosition = 0,
        HitNormal,
        OutEntityId,
        NoHit,
    };

public:

    //! Constructor for a touch raycast node
    //! \param activationInfo    The basic information needed for initializing a flowgraph node, including but not
    //!                         limited to IEntity links, input/output connections, etc.
    CFlowTouchRayCast(SActivationInfo* activationInfo)
        : CFlowTouchBaseNode(activationInfo)
        , m_rayTypeFilter()
        , m_touchPos(0.0f, 0.0f)
        , m_handleUpdate(false)
        , m_ignoreBackfaces(false)
    {
    }

    //! Mainly removes the touch event listener from the input system
    ~CFlowTouchRayCast() override
    {
        UnregisterListeners();
    }


    // IFlowNode

    //! Handles read/write serialization of the flowgraph node
    //! \param activationInfo   Basic information about the flowgraph node, including IEntity, input, output, etc. connections
    //! \param ser              The I/O serializer object used for reading/writing
    void Serialize(SActivationInfo* activationInfo, TSerialize ser) override
    {
        ser.Value("enabled", m_enabled);

        int rayTypeFilter = static_cast<int>(m_rayTypeFilter);
        ser.Value("rayTypeFilter", rayTypeFilter);
        m_rayTypeFilter = static_cast<entity_query_flags>(rayTypeFilter);

        // register listeners if this node was saved as enabled
        if (ser.IsReading() && m_enabled)
        {
            RegisterListeners();
        }
    }

    //! Puts the objects used in this module into the sizer interface
    //! \param sizer    Sizer object to track memory allocations
    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    //! Main accessor for getting the inputs/outputs and flags for the flowgraph node
    //! \param config   Ref to a flow node config struct for the node to fill out with its inputs/outputs
    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] = {
            InputPortConfig_Void("Enable", _HELP("Enable")),
            InputPortConfig_Void("Disable", _HELP("Disable")),
            InputPortConfig<int>("RayType", 0, _HELP("Raycast filter (All=0, Terrain=1, Rigid=2, Static=3, Water=4, Living=5)"), "All", _UICONFIG("enum_int: All=0, Terrain=1, Rigid=2, Static=3, Water=4, Living=5")),
            InputPortConfig<int>("EntitiesToIgnore", _HELP("Container with Entities to ignore during raycast")),
            InputPortConfig<bool>("IgnoreBackfaces", _HELP("Ignore back faces of geometry during raycast")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig<Vec3>("HitPos", _HELP("The 3D position of where the raycast hit")),
            OutputPortConfig<Vec3>("HitNormal", _HELP("The 3D normal vector of what object was hit")),
            OutputPortConfig<FlowEntityId>("EntityId", _HELP("Entity that got hit by raycast; 0 if no hit or hit object is not an Entity")),
            OutputPortConfig_Void("NoHit", _HELP("Activated every frame when enabled and no item is hit")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Generates a ray cast for every frame finger id zero active");
        config.SetCategory(EFLN_APPROVED);
    }

    //! Clones the flowgraph node.  Clone is called instead of creating a new node if the node type is set to eNCT_Cloned
    //! \param activationInfo       The activation info from the flowgraph node that is being cloned from
    //! \return                     The newly cloned flowgraph node
    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowTouchRayCast(activationInfo);
    }

    //! Event handler specifically for flowgraph events
    //! \param event            The flowgraph event ID
    //! \param activationInfo   The node's updated activation data
    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        m_actInfo = *activationInfo;
        switch (event)
        {
        case eFE_Initialize:
        {
            StoreRayType();
            StoreIgnoreBackfaces();
            break;
        }
        case eFE_Activate:
        {
            if (IsPortActive(activationInfo, InputPorts::Enable))
            {
                StoreRayType();
                RegisterListeners();
                m_enabled = true;
            }

            if (IsPortActive(activationInfo, InputPorts::Disable))
            {
                UnregisterListeners();
                m_enabled = false;
            }

            if (IsPortActive(activationInfo, InputPorts::RayType))
            {
                StoreRayType();
            }

            if (IsPortActive(activationInfo, InputPorts::IgnoreBackfaces))
            {
                StoreIgnoreBackfaces();
            }
            break;
        }
        }
    }

    // ~IFlowNode


    // IInputEventListener

    //! Input event handler
    //! \param event        Information about the input event
    bool OnInputEvent(const SInputEvent& event) override
    {
        if (!m_enabled || event.deviceType != eIDT_TouchScreen)
        {
            return false;
        }

        // process the event
        switch (event.keyId)
        {
        case eKI_Touch0:
        {
            m_touchPos = event.screenPosition;
            m_handleUpdate = true;
        }
        break;
        }

        return false;
    }

    // ~IInputEventListener


    // IGameFrameworkListener

    void OnSaveGame(ISaveGame* saveGame) override
    {
    }

    void OnLoadGame(ILoadGame* loadGame) override
    {
    }

    void OnLevelEnd(const char* nextLevel) override
    {
    }

    void OnActionEvent(const SActionEvent& event) override
    {
    }

    //! We need the post update because the model-view matrix is updated in the normal update and the ProcessEvent update occurs in the pre-update
    //! \param deltaTime    The time it took to get here from the last frame in milliseconds
    void OnPostUpdate(float deltaTime) override
    {
        // update the raycast state
        if (!m_handleUpdate)
        {
            return;
        }
        else
        {
            m_handleUpdate = false;
        }

        // construct the ray from the touch position
        float invTouchY = static_cast<float>(gEnv->pRenderer->GetHeight()) - m_touchPos.y;

        Vec3 origin(0, 0, 0);
        gEnv->pRenderer->UnProjectFromScreen(m_touchPos.x, invTouchY, 0, &origin.x, &origin.y, &origin.z);

        Vec3 forwardPoint(0, 0, 0);
        gEnv->pRenderer->UnProjectFromScreen(m_touchPos.x, invTouchY, 1, &forwardPoint.x, &forwardPoint.y, &forwardPoint.z);

        Vec3 direction = forwardPoint - origin;
        direction.Normalize();

        // construct the list of entities to filter out of the ray cast
        IPhysicalEntity** entitesToSkip = NULL;
        int numSkipped = 0;

        int containerId = GetPortInt(&m_actInfo, InputPorts::EntitiesToIgnore);
        if (containerId != 0)
        {
            if (IFlowSystemContainerPtr container = gEnv->pFlowSystem->GetContainer(containerId))
            {
                numSkipped = container->GetItemCount();
                entitesToSkip = new IPhysicalEntity*[numSkipped];
                for (int i = 0; i < numSkipped; ++i)
                {
                    EntityId id;
                    container->GetItem(i).GetValueWithConversion<EntityId>(id);
                    entitesToSkip[i] = gEnv->pEntitySystem->GetEntity(id)->GetPhysics();
                }
            }
        }

        // process the ray cast
        ray_hit hit;
        const unsigned int flags = rwi_stop_at_pierceable | rwi_colltype_any | (m_ignoreBackfaces ? rwi_ignore_back_faces : 0);

        if (gEnv->pPhysicalWorld && gEnv->pPhysicalWorld->RayWorldIntersection(origin, direction * gEnv->p3DEngine->GetMaxViewDistance(), m_rayTypeFilter, flags, &hit, 1, entitesToSkip, numSkipped))
        {
            ActivateOutput(&m_actInfo, OutputPorts::HitPosition, hit.pt);
            ActivateOutput(&m_actInfo, OutputPorts::HitNormal, hit.n);

            if (hit.pCollider)
            {
                auto entity = gEnv->pEntitySystem->GetEntityFromPhysics(hit.pCollider);
                ActivateOutput(&m_actInfo, OutputPorts::OutEntityId, entity ? entity->GetId() : 0);
            }
        }
        else
        {
            ActivateOutput(&m_actInfo, OutputPorts::NoHit, true);     // void type
            ActivateOutput(&m_actInfo, OutputPorts::OutEntityId, 0);    // no hit, no entity
        }

        // clean up the heap allocations
        if (entitesToSkip)
        {
            delete[] entitesToSkip;
        }
    }

    // ~IGameFrameworkListener


protected:
    //!< Helper for getting the node name, which is used for registering with different listeners
    const char* GetListenerName()
    {
        return "CFlowTouchRayCast";
    }

    //! Wrapper for adding the flow node to it's listener handlers
    void RegisterListeners() override
    {
        CFlowTouchBaseNode::RegisterListeners();

        if (gEnv->pGame && gEnv->pGame->GetIGameFramework())
        {
            gEnv->pGame->GetIGameFramework()->RegisterListener(this, GetListenerName(), FRAMEWORKLISTENERPRIORITY_DEFAULT);
        }
    }

    //! Wrapper for removing the flow node from it's listener handlers
    void UnregisterListeners() override
    {
        CFlowTouchBaseNode::UnregisterListeners();

        if (gEnv->pGame && gEnv->pGame->GetIGameFramework())
        {
            gEnv->pGame->GetIGameFramework()->UnregisterListener(this);
        }
    }


private:
    //! Wrapper for getting the ray cast hit type
    void StoreRayType()
    {
        int ray_type = GetPortInt(&m_actInfo, InputPorts::RayType);
        switch (ray_type)
        {
        case 0:
        {
            m_rayTypeFilter = ent_all;
            break;
        }
        case 1:
        {
            m_rayTypeFilter = ent_terrain;
            break;
        }
        case 2:
        {
            m_rayTypeFilter = ent_rigid;
            break;
        }
        case 3:
        {
            m_rayTypeFilter = ent_static;
            break;
        }
        case 4:
        {
            m_rayTypeFilter = ent_water;
            break;
        }
        case 5:
        {
            m_rayTypeFilter = ent_living;
            break;
        }
        }
    }

    //! Wrapper for getting the ignore-backfaces flag
    void StoreIgnoreBackfaces()
    {
        m_ignoreBackfaces = GetPortBool(&m_actInfo, InputPorts::IgnoreBackfaces);
    }

    entity_query_flags m_rayTypeFilter; //!< What type entities should be allowed to collide with the ray cast
    Vec2 m_touchPos; //!< The stored screen space location of the touch
    bool m_handleUpdate; //!< Indicates if the next "update" event during this frame should be handled
    bool m_ignoreBackfaces; //!< Flag indicating if backfacing geometry should be ignored during raycasting
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowMultiTouchEventNode class
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
//! Flowgraph node for sending multi-touch event information
class CFlowMultiTouchEventNode
    : public CFlowTouchBaseNode
{
    enum InputPorts
    {
        Input_Enable = 0,
        Input_Disable,
    };

    enum OutputPorts
    {
        Output_TouchDown = 0,
        Output_TouchUp,
    };

public:

    //! Constructor for a multi-touch event node
    //! \param activationInfo    The basic information needed for initializing a flowgraph node, including but not
    //!                         limited to IEntity links, input/output connections, etc.
    CFlowMultiTouchEventNode(SActivationInfo* activationInfo)
        : CFlowTouchBaseNode(activationInfo)
    {
    }

    //! Mainly removes the touch event listener from the input system
    ~CFlowMultiTouchEventNode() override
    {
    }


    // IFlowNode

    //! Handles read/write serialization of the flowgraph node
    //! \param activationInfo   Basic information about the flowgraph node, including IEntity, input, output, etc. connections
    //! \param ser              The I/O serializer object used for reading/writing
    void Serialize(SActivationInfo* activationInfo, TSerialize ser) override
    {
        ser.Value("enabled", m_enabled);

        // register listeners if this node was saved as enabled
        if (ser.IsReading() && m_enabled)
        {
            RegisterListeners();
        }
    }

    //! Puts the objects used in this module into the sizer interface
    //! \param sizer    Sizer object to track memory allocations
    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    //! Main accessor for getting the inputs/outputs and flags for the flowgraph node
    //! \param config   Ref to a flow node config struct for the node to fill out with its inputs/outputs
    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Enable", _HELP("Enable touch info")),
            InputPortConfig_Void("Disable", _HELP("Disable touch info")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig<int>("TouchDown", _HELP("Represents the touch finger id that was pressed")),
            OutputPortConfig<int>("TouchUp", _HELP("Represents the touch finger id that was released")),

            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Returns touch location information.");
        config.SetCategory(EFLN_APPROVED);
    }

    //! Clones the flowgraph node.  Clone is called instead of creating a new node if the node type is set to eNCT_Cloned
    //! \param activationInfo       The activation info from the flowgraph node that is being cloned from
    //! \return                     The newly cloned flowgraph node
    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowMultiTouchEventNode(activationInfo);
    }

    //! Event handler specifically for flowgraph events
    //! \param event            The flowgraph event ID
    //! \param activationInfo   The node's updated activation data
    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        m_actInfo = *activationInfo;
        switch (event)
        {
        case eFE_Initialize:
        case eFE_Activate:
        {
            if (IsPortActive(activationInfo, InputPorts::Input_Enable))
            {
                RegisterListeners();
                m_enabled = true;
            }

            if (IsPortActive(activationInfo, InputPorts::Input_Disable))
            {
                UnregisterListeners();
                m_enabled = false;
            }
        }
        }
    }

    // ~IFlowNode


    // IInputEventListener

    //! Input event handler
    //! \param event        Information about the input event
    bool OnInputEvent(const SInputEvent& event) override
    {
        // we don't care about the event if the node is not active
        if (!m_enabled || event.deviceType != eIDT_TouchScreen)
        {
            return false;
        }

        // process the event
        const int fingerId = event.keyId - eKI_Touch0;
        switch (event.state)
        {
        case eIS_Pressed:
        {
            ActivateOutput(&m_actInfo, Output_TouchDown, fingerId);
        }
        break;
        case eIS_Released:
        {
            ActivateOutput(&m_actInfo, Output_TouchUp, fingerId);
        }
        break;
        }

        return false;
    }

    // ~IInputEventListener
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowMultiTouchCoordsNode class
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
//! Flowgraph node specifically for sending out the touch events for finger id zero
class CFlowMultiTouchCoordsNode
    : public CFlowTouchBaseNode
{
    enum InputPorts
    {
        Input_Enable = 0,
        Input_Disable,
        Input_TouchId,
    };

    enum OutputPorts
    {
        Output_ScreenCoordX,
        Output_ScreenCoordY,
    };

public:

    //! Constructor for a basic touch event node
    //! \param activationInfo    The basic information needed for initializing a flowgraph node, including but not
    //!                         limited to IEntity links, input/output connections, etc.
    CFlowMultiTouchCoordsNode(SActivationInfo* activationInfo)
        : CFlowTouchBaseNode(activationInfo)
        , m_touchIdFilter(0)
    {
    }

    //! Mainly removes the touch event listener from the input system
    ~CFlowMultiTouchCoordsNode() override
    {
    }


    // IFlowNode

    //! Handles read/write serialization of the flowgraph node
    //! \param activationInfo   Basic information about the flowgraph node, including IEntity, input, output, etc. connections
    //! \param ser              The I/O serializer object used for reading/writing
    void Serialize(SActivationInfo* activationInfo, TSerialize ser) override
    {
        ser.Value("enabled", m_enabled);
        ser.Value("touchIdFilter", m_touchIdFilter);

        // register listeners if this node was saved as enabled
        if (ser.IsReading() && m_enabled)
        {
            RegisterListeners();
        }
    }

    //! Puts the objects used in this module into the sizer interface
    //! \param sizer    Sizer object to track memory allocations
    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    //! Main accessor for getting the inputs/outputs and flags for the flowgraph node
    //! \param config   Ref to a flow node config struct for the node to fill out with its inputs/outputs
    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Enable", _HELP("Enable")),
            InputPortConfig_Void("Disable", _HELP("Disable")),
            InputPortConfig<int>("TouchId", 0, _HELP("The desired touch (finger) id for which coords will be sent")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig<int>("ScreenCoordX", _HELP("Screen space X pixel coord of the touch")),
            OutputPortConfig<int>("ScreenCoordY", _HELP("Screen space Y pixel coord of the touch")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Outputs touch events from the specified id (finger)");
        config.SetCategory(EFLN_APPROVED);
    }

    //! Clones the flowgraph node.  Clone is called instead of creating a new node if the node type is set to eNCT_Cloned
    //! \param activationInfo       The activation info from the flowgraph node that is being cloned from
    //! \return                     The newly cloned flowgraph node
    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowTouchEventNode(activationInfo);
    }

    //! Event handler specifically for flowgraph events
    //! \param event            The flowgraph event ID
    //! \param activationInfo   The node's updated activation data
    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        m_actInfo = *activationInfo;
        switch (event)
        {
        case eFE_Initialize:
        {
            m_touchIdFilter = GetPortInt(activationInfo, InputPorts::Input_TouchId);
        }
        break;
        case eFE_Activate:
        {
            if (IsPortActive(activationInfo, InputPorts::Input_Enable))
            {
                m_touchIdFilter = GetPortInt(activationInfo, InputPorts::Input_TouchId);

                RegisterListeners();
                m_enabled = true;
            }

            if (IsPortActive(activationInfo, InputPorts::Input_Disable))
            {
                UnregisterListeners();
                m_enabled = false;
            }

            if (IsPortActive(activationInfo, InputPorts::Input_TouchId))
            {
                m_touchIdFilter = GetPortInt(activationInfo, InputPorts::Input_TouchId);
            }
        }
        break;
        }
    }

    // ~IFlowNode


    // IInputEventListener

    //! Input event handler
    //! \param event        Information about the input event
    bool OnInputEvent(const SInputEvent& event) override
    {
        if (!m_enabled || event.deviceType != eIDT_TouchScreen)
        {
            return false;
        }

        // skip the event if its not the touch id we want
        const int touchId = event.keyId - eKI_Touch0;
        if (touchId != m_touchIdFilter)
        {
            return false;
        }

        // send the screen coordinates,
        ActivateOutput(&m_actInfo, OutputPorts::Output_ScreenCoordX, event.screenPosition.x);
        ActivateOutput(&m_actInfo, OutputPorts::Output_ScreenCoordY, event.screenPosition.y);

        return false;
    }

    // ~IInputEventListener


private:
    int m_touchIdFilter; //!< Request which touch (finger) index's coordinates to be output (defaults to 0)
};

REGISTER_FLOW_NODE("Input:Touch:TouchEvent", CFlowTouchEventNode);
REGISTER_FLOW_NODE("Input:Touch:TouchRayCast", CFlowTouchRayCast);
REGISTER_FLOW_NODE("Input:Touch:MultiTouchEvent", CFlowMultiTouchEventNode);
REGISTER_FLOW_NODE("Input:Touch:MultiTouchCoords", CFlowMultiTouchCoordsNode);
