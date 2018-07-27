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

// Description : Mouse Info Flownode


#include "CryLegacy_precompiled.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include "IInput.h"
#include <LyShine/Bus/UiCursorBus.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CFlowMouseCoordNode
    : public CFlowBaseNode<eNCT_Instanced>
    , public IInputEventListener
{
private:
    SActivationInfo m_actInfo;
    bool m_bOutputWorldCoords;
    bool m_bOutputScreenCoords;
    bool m_bOutputDeltaCoords;
    bool m_enabled;

    enum InputPorts
    {
        Enable = 0,
        Disable,
        World,
        Screen,
        DeltaScreen,
    };

    enum OutputPorts
    {
        OutWorld = 0,
        ScreenX,
        ScreenY,
        DeltaScreenX,
        DeltaScreenY,
    };


public:

    ~CFlowMouseCoordNode()
    {
        if (GetISystem() && GetISystem()->GetIInput())
        {
            GetISystem()->GetIInput()->RemoveEventListener(this);
        }
    }

    CFlowMouseCoordNode(SActivationInfo* pActInfo)
        : m_actInfo(*pActInfo)
        , m_bOutputWorldCoords(false)
        , m_bOutputScreenCoords(false)
        , m_bOutputDeltaCoords(false)
        , m_enabled(false)
    {
    }

    void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
    {
        ser.Value("enabled", m_enabled);

        // register listeners if this node was saved as enabled
        if (ser.IsReading() && m_enabled)
        {
            if (GetISystem() && GetISystem()->GetIInput())
            {
                GetISystem()->GetIInput()->AddEventListener(this);
            }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Enable", _HELP("Enable")),
            InputPortConfig_Void("Disable", _HELP("Disable")),
            InputPortConfig<bool>("World", false, _HELP("Enable output of Mouse Position in World Space")),
            InputPortConfig<bool>("Screen", false, _HELP("Enable output of Mouse Position in Screen Space")),
            InputPortConfig<bool>("Delta", false, _HELP("Enable the output of Mouse Screen Delta")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig<Vec3>("WorldPosition", _HELP("Unprojected mouse position (World Space)"), _HELP("World")),
            OutputPortConfig<int>("ScreenX", _HELP("X coordinate of the Mouse position on screen")),
            OutputPortConfig<int>("ScreenY", _HELP("Y coordinate of the Mouse position on screen")),
            OutputPortConfig<int>("DeltaScreenX", _HELP("Delta of X coordinate of the Mouse position on screen")),
            OutputPortConfig<int>("DeltaScreenY", _HELP("Delta of Y coordinate of the Mouse position on screen")),
            {0}
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Outputs a variety of mouse position information when Enabled");
        config.SetCategory(EFLN_APPROVED);
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowMouseCoordNode(pActInfo);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        m_actInfo = *pActInfo;
        switch (event)
        {
        case eFE_Initialize:
        {
            m_bOutputScreenCoords = GetPortBool(pActInfo, InputPorts::Screen);
            m_bOutputWorldCoords = GetPortBool(pActInfo, InputPorts::World);
            m_bOutputDeltaCoords = GetPortBool(pActInfo, InputPorts::DeltaScreen);
            break;
        }
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Enable))
            {
                if (GetISystem() && GetISystem()->GetIInput())
                {
                    GetISystem()->GetIInput()->AddEventListener(this);
                }

                m_bOutputScreenCoords = GetPortBool(pActInfo, InputPorts::Screen);
                m_bOutputWorldCoords = GetPortBool(pActInfo, InputPorts::World);
                m_bOutputDeltaCoords = GetPortBool(pActInfo, InputPorts::DeltaScreen);
                m_enabled = true;
            }

            if (IsPortActive(pActInfo, InputPorts::Disable))
            {
                if (GetISystem() && GetISystem()->GetIInput())
                {
                    GetISystem()->GetIInput()->RemoveEventListener(this);
                }

                m_enabled = false;
            }

            if (IsPortActive(pActInfo, InputPorts::World))
            {
                m_bOutputWorldCoords = GetPortBool(pActInfo, InputPorts::World);
            }

            if (IsPortActive(pActInfo, InputPorts::Screen))
            {
                m_bOutputScreenCoords = GetPortBool(pActInfo, InputPorts::Screen);
            }

            if (IsPortActive(pActInfo, InputPorts::DeltaScreen))
            {
                m_bOutputDeltaCoords = GetPortBool(pActInfo, InputPorts::DeltaScreen);
            }

            break;
        }
        }
    }

    void OnMousePositionEvent(const SInputEvent& event)
    {
        if (m_bOutputScreenCoords)
        {
            ActivateOutput(&m_actInfo, OutputPorts::ScreenX, event.screenPosition.x);
            ActivateOutput(&m_actInfo, OutputPorts::ScreenY, event.screenPosition.y);
        }

        if (m_bOutputWorldCoords)
        {
            Vec3 vPos;
            float mouseX, mouseY;
            mouseX = event.screenPosition.x;
            mouseY = gEnv->pRenderer->GetHeight() - event.screenPosition.y;
            gEnv->pRenderer->UnProjectFromScreen(mouseX, mouseY, 1, &vPos.x, &vPos.y, &vPos.z);

            ActivateOutput(&m_actInfo, OutputPorts::OutWorld, vPos);
        }
    }

    bool OnInputEvent(const SInputEvent& event) override
    {
        if (event.keyId == eKI_MousePosition)
        {
            OnMousePositionEvent(event);
            return false;
        }

        if (m_bOutputDeltaCoords == false)
        {
            return false;
        }

        switch (event.keyId)
        {
        case eKI_MouseX:
        {
            ActivateOutput(&m_actInfo, OutputPorts::DeltaScreenX, event.value);
            break;
        }
        case eKI_MouseY:
        {
            ActivateOutput(&m_actInfo, OutputPorts::DeltaScreenY, event.value);
            break;
        }
        }

        return false;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CFlowMouseButtonNode
    : public CFlowBaseNode<eNCT_Instanced>
    , public IInputEventListener
{
private:
    SActivationInfo m_actInfo;
    bool m_bOutputMouseButton;
    bool m_bOutputMouseWheel;
    bool m_enabled;

    enum InputPorts
    {
        Enable = 0,
        Disable,
        OutputMouseButtons,
        OutputMouseWheel,
    };

    enum OutputPorts
    {
        MousePressed = 0,
        MouseReleased,
        MouseWheel
    };

public:

    CFlowMouseButtonNode(SActivationInfo* pActInfo)
    {
        m_actInfo = *pActInfo;
        m_bOutputMouseButton = false;
        m_bOutputMouseWheel = false;
        m_enabled = false;
    }

    ~CFlowMouseButtonNode()
    {
        if (GetISystem() && GetISystem()->GetIInput())
        {
            GetISystem()->GetIInput()->RemoveEventListener(this);
        }
    }

    void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
    {
        ser.Value("enabled", m_enabled);

        // register listeners if this node was saved as enabled
        if (ser.IsReading() && m_enabled)
        {
            if (GetISystem() && GetISystem()->GetIInput())
            {
                GetISystem()->GetIInput()->AddEventListener(this);
            }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Enable", _HELP("Enable")),
            InputPortConfig_Void("Disable", _HELP("Disable")),
            InputPortConfig<bool>("MouseButton", false, _HELP("Output mouse button information")),
            InputPortConfig<bool>("MouseWheel", false, _HELP("Output mouse wheel information")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig<int>("MousePressed", _HELP("Integer value representing the button that was pressed")),
            OutputPortConfig<int>("MouseReleased", _HELP("Integer value representing the button that was released")),
            OutputPortConfig<float>("MouseWheel", _HELP("Positive value when wheel is moved up, negative value when wheel is moved down")),
            {0}
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Outputs Mouse button information");
        config.SetCategory(EFLN_APPROVED);
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowMouseButtonNode(pActInfo);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        m_actInfo = *pActInfo;
        switch (event)
        {
        case eFE_Initialize:
        {
            m_bOutputMouseButton = GetPortBool(pActInfo, InputPorts::OutputMouseButtons);
            m_bOutputMouseWheel = GetPortBool(pActInfo, InputPorts::OutputMouseWheel);
            break;
        }
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Enable))
            {
                if (GetISystem() && GetISystem()->GetIInput())
                {
                    GetISystem()->GetIInput()->AddEventListener(this);
                }

                m_bOutputMouseButton = GetPortBool(pActInfo, InputPorts::OutputMouseButtons);
                m_bOutputMouseWheel = GetPortBool(pActInfo, InputPorts::OutputMouseWheel);
                m_enabled = true;
            }

            if (IsPortActive(pActInfo, InputPorts::Disable))
            {
                if (GetISystem() && GetISystem()->GetIInput())
                {
                    GetISystem()->GetIInput()->RemoveEventListener(this);
                }

                m_enabled = false;
            }

            if (IsPortActive(pActInfo, InputPorts::OutputMouseButtons))
            {
                m_bOutputMouseButton = GetPortBool(pActInfo, InputPorts::OutputMouseButtons);
            }

            if (IsPortActive(pActInfo, InputPorts::OutputMouseWheel))
            {
                m_bOutputMouseWheel = GetPortBool(pActInfo, InputPorts::OutputMouseWheel);
            }

            break;
        }
        }
    }

    bool OnInputEvent(const SInputEvent& event) override
    {
        switch (event.keyId)
        {
        case eKI_Mouse1:
        case eKI_Mouse2:
        case eKI_Mouse3:
        case eKI_Mouse4:
        case eKI_Mouse5:
        case eKI_Mouse6:
        case eKI_Mouse7:
        case eKI_Mouse8:
        {
            if (m_bOutputMouseButton)
            {
                if (event.state == eIS_Pressed)
                {
                    ActivateOutput(&m_actInfo, OutputPorts::MousePressed, (event.keyId - KI_MOUSE_BASE) + 1);     // mouse1 should be 1, etc
                }
                if (event.state == eIS_Released)
                {
                    ActivateOutput(&m_actInfo, OutputPorts::MouseReleased, (event.keyId - KI_MOUSE_BASE) + 1);     // mouse1 should be 1, etc
                }
            }
            break;
        }
        case eKI_MouseWheelDown:
        case eKI_MouseWheelUp:
        {
            // pressed: started scrolling, down: repeating event while scrolling at a speed, released: finished (delta 0)
            // for every pressed event there is at least 1 down event, and we only want to forward the down event to the user (shows the speed and length of a scroll)
            if (m_bOutputMouseWheel && event.state == eIS_Down)
            {
                ActivateOutput(&m_actInfo, OutputPorts::MouseWheel, event.value);
            }
            break;
        }
        }

        return false;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CFlowMouseRayCast
    : public CFlowBaseNode<eNCT_Instanced>
    , public IInputEventListener
    , public IGameFrameworkListener
{
private:
    SActivationInfo m_actInfo;
    float m_mouseX, m_mouseY;
    entity_query_flags m_rayTypeFilter;
    bool m_enabled;
    bool m_ignoreBackfaces;

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
        HitPos = 0,
        HitNormal,
        OutEntityId,
        NoHit,
    };

public:

    CFlowMouseRayCast(SActivationInfo* pActInfo)
        : m_actInfo(*pActInfo)
        , m_mouseX(0)
        , m_mouseY(0)
        , m_rayTypeFilter(ent_all)
        , m_enabled(false)
        , m_ignoreBackfaces(false)
    {
    }

    ~CFlowMouseRayCast()
    {
        if (GetISystem() && GetISystem()->GetIInput())
        {
            GetISystem()->GetIInput()->RemoveEventListener(this);
        }

        if (gEnv->pGame && gEnv->pGame->GetIGameFramework())
        {
            gEnv->pGame->GetIGameFramework()->UnregisterListener(this);
        }
    }

    void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
    {
        ser.Value("enabled", m_enabled);

        // register listeners if this node was saved as enabled
        if (ser.IsReading() && m_enabled)
        {
            if (GetISystem() && GetISystem()->GetIInput())
            {
                GetISystem()->GetIInput()->AddEventListener(this);
            }

            if (gEnv->pGame && gEnv->pGame->GetIGameFramework())
            {
                gEnv->pGame->GetIGameFramework()->RegisterListener(this, "MouseRayInfoNode", FRAMEWORKLISTENERPRIORITY_DEFAULT);
            }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Enable", _HELP("Enable")),
            InputPortConfig_Void("Disable", _HELP("Disable")),
            InputPortConfig<int>("RayType", 0, _HELP("Raycast filter (All=0, Terrain=1, Rigid=2, Static=3, Water=4, Living=5)"), "All", _UICONFIG("enum_int: All=0, Terrain=1, Rigid=2, Static=3, Water=4, Living=5")),
            InputPortConfig<int>("EntitiesToIgnore", _HELP("Container with Entities to ignore during raycast")),
            InputPortConfig<bool>("IgnoreBackfaces", _HELP("Ignore back faces of geometry during raycast")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig<Vec3>("HitPos", _HELP("Co-ordinates of the first position that was hit by the raycast in 3D space")),
            OutputPortConfig<Vec3>("HitNormal", _HELP("Normal of the first position that was hit by the raycast in 3D space")),
            OutputPortConfig<FlowEntityId>("EntityId", _HELP("Id of the Entity that got hit by raycast")),
            OutputPortConfig_Void("NoHit", _HELP("Activated every frame when enabled and no item is hit")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Mouse Raycast information");
        config.SetCategory(EFLN_APPROVED);
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowMouseRayCast(pActInfo);
    }

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

    void StoreIgnoreBackfaces()
    {
        m_ignoreBackfaces = GetPortBool(&m_actInfo, InputPorts::IgnoreBackfaces);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        m_actInfo = *pActInfo;
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
            if (IsPortActive(pActInfo, InputPorts::Enable))
            {
                if (GetISystem() && GetISystem()->GetIInput())
                {
                    GetISystem()->GetIInput()->AddEventListener(this);
                }

                if (gEnv->pGame && gEnv->pGame->GetIGameFramework())
                {
                    gEnv->pGame->GetIGameFramework()->RegisterListener(this, "MouseRayInfoNode", FRAMEWORKLISTENERPRIORITY_DEFAULT);
                }

                StoreRayType();

                m_enabled = true;
            }

            if (IsPortActive(pActInfo, InputPorts::Disable))
            {
                if (GetISystem() && GetISystem()->GetIInput())
                {
                    GetISystem()->GetIInput()->RemoveEventListener(this);
                }

                if (gEnv->pGame && gEnv->pGame->GetIGameFramework())
                {
                    gEnv->pGame->GetIGameFramework()->UnregisterListener(this);
                }

                m_enabled = false;
            }

            if (IsPortActive(pActInfo, InputPorts::RayType))
            {
                StoreRayType();
            }

            if (IsPortActive(pActInfo, InputPorts::IgnoreBackfaces))
            {
                StoreIgnoreBackfaces();
            }

            break;
        }
        }
    }

    bool OnInputEvent(const SInputEvent& event) override
    {
        if (event.keyId == eKI_MousePosition)
        {
            m_mouseX = event.screenPosition.x;
            m_mouseY = event.screenPosition.y;
        }
        return false;
    }

    // IGameFrameworkListener
    void OnSaveGame(ISaveGame* pSaveGame) override
    {
    }

    void OnLoadGame(ILoadGame* pLoadGame) override
    {
    }

    void OnLevelEnd(const char* nextLevel) override
    {
    }

    void OnActionEvent(const SActionEvent& event) override
    {
    }

    // Since the modelviewmatrix is updated in the update, and flowgraph is updated in the preupdate, we need this postupdate
    void OnPostUpdate(float fDeltaTime) override
    {
        float invMouseY = static_cast<float>(gEnv->pRenderer->GetHeight()) - m_mouseY;

        Vec3 vPos0(0, 0, 0);
        gEnv->pRenderer->UnProjectFromScreen(m_mouseX, invMouseY, 0, &vPos0.x, &vPos0.y, &vPos0.z);

        Vec3 vPos1(0, 0, 0);
        gEnv->pRenderer->UnProjectFromScreen(m_mouseX, invMouseY, 1, &vPos1.x, &vPos1.y, &vPos1.z);

        Vec3 vDir = vPos1 - vPos0;
        vDir.Normalize();

        ray_hit hit;
        const unsigned int flags = rwi_stop_at_pierceable | rwi_colltype_any | (m_ignoreBackfaces ? rwi_ignore_back_faces : 0);

        IPhysicalEntity** pSkipEnts = NULL;
        int numSkipped = 0;

        int containerId = GetPortInt(&m_actInfo, InputPorts::EntitiesToIgnore);
        if (containerId != 0)
        {
            if (IFlowSystemContainerPtr container = gEnv->pFlowSystem->GetContainer(containerId))
            {
                numSkipped = container->GetItemCount();
                pSkipEnts = new IPhysicalEntity*[numSkipped];
                for (int i = 0; i < numSkipped; i++)
                {
                    FlowEntityId id;
                    container->GetItem(i).GetValueWithConversion(id);
                    pSkipEnts[i] = gEnv->pEntitySystem->GetEntity(id)->GetPhysics();
                }
            }
        }

        if (gEnv->pPhysicalWorld && gEnv->pPhysicalWorld->RayWorldIntersection(vPos0, vDir *  gEnv->p3DEngine->GetMaxViewDistance(), m_rayTypeFilter, flags, &hit, 1, pSkipEnts, numSkipped))
        {
            ActivateOutput(&m_actInfo, OutputPorts::HitPos, hit.pt);
            ActivateOutput(&m_actInfo, OutputPorts::HitNormal, hit.n);

            IEntity* pEntity = nullptr;
            if (hit.pCollider)
            {
                pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(hit.pCollider);
            }
            ActivateOutput(&m_actInfo, OutputPorts::OutEntityId, pEntity ? pEntity->GetId() : 0);
        }
        else
        {
            ActivateOutput(&m_actInfo, OutputPorts::NoHit, true);    // void type
            ActivateOutput(&m_actInfo, OutputPorts::OutEntityId, 0);    // no hit means no entity
        }

        if (pSkipEnts)
        {
            delete [] pSkipEnts;
        }
    }
    // ~IGameFrameworkListener
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CFlowMouseCursor
    : public CFlowBaseNode<eNCT_Instanced>
{
private:

    bool m_isShowing;

    enum InputPorts
    {
        Show = 0,
        Hide,
    };

    enum OutputPorts
    {
        Done = 0,
    };

public:

    CFlowMouseCursor(SActivationInfo* pActInfo)
        : m_isShowing(false)
    {
    }

    void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
    {
        ser.Value("isShowing", m_isShowing);

        if (ser.IsReading())
        {
            if (m_isShowing)
            {
                UiCursorBus::Broadcast(&UiCursorInterface::IncrementVisibleCounter);
            }
        }
    }

    ~CFlowMouseCursor()
    {
        // make sure to put the cursor at the correct counter before deleting this
        if (m_isShowing)
        {
            UiCursorBus::Broadcast(&UiCursorInterface::DecrementVisibleCounter);
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Show", _HELP("Show mouse cursor")),
            InputPortConfig_Void("Hide", _HELP("Hide mouse cursor")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("Done", _HELP("Done")),
            {0}
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Shows or Hides the mouse cursor");
        config.SetCategory(EFLN_APPROVED);
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowMouseCursor(pActInfo);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            // hide cursor from start (default behavior)
            if (m_isShowing)
            {
                UiCursorBus::Broadcast(&UiCursorInterface::DecrementVisibleCounter);
                m_isShowing = false;
            }
            break;
        }
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Show))
            {
                if (!m_isShowing)
                {
                    UiCursorBus::Broadcast(&UiCursorInterface::IncrementVisibleCounter);
                    m_isShowing = true;
                }

                ActivateOutput(pActInfo, OutputPorts::Done, 1);
            }

            if (IsPortActive(pActInfo, InputPorts::Hide))
            {
                if (m_isShowing)
                {
                    UiCursorBus::Broadcast(&UiCursorInterface::DecrementVisibleCounter);
                    m_isShowing = false;
                }

                ActivateOutput(pActInfo, OutputPorts::Done, 1);
            }

            break;
        }
        }
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CFlowMouseEntitiesInBox
    : public CFlowBaseNode<eNCT_Instanced>
    , public IGameFrameworkListener
{
private:
    int m_screenX, m_screenY, m_screenX2, m_screenY2;
    SActivationInfo m_actInfo;
    bool m_get;

    enum InputPorts
    {
        Get = 0,
        ContainerId,
        ScreenX,
        ScreenY,
        ScreenX2,
        ScreenY2,
    };

    enum OutputPorts
    {
        Done = 0,
    };


public:

    CFlowMouseEntitiesInBox(SActivationInfo* pActInfo)
        : m_screenX(0)
        , m_screenX2(0)
        , m_screenY(0)
        , m_screenY2(0)
        , m_get(false)
    {
    }

    ~CFlowMouseEntitiesInBox()
    {
        if (gEnv->pGame && gEnv->pGame->GetIGameFramework())
        {
            gEnv->pGame->GetIGameFramework()->UnregisterListener(this);
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Get", _HELP("Show mouse cursor")),
            InputPortConfig<int>("ContainerId", _HELP("The container to store the entitiy Ids in, user is responsible for creating/deleting this")),
            InputPortConfig<int>("ScreenX", _HELP("Hide the mouse cursor")),
            InputPortConfig<int>("ScreenY", _HELP("Hide the mouse cursor")),
            InputPortConfig<int>("ScreenX2", _HELP("Hide the mouse cursor")),
            InputPortConfig<int>("ScreenY2", _HELP("Hide the mouse cursor")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("Done", _HELP("Done")),
            {0}
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Node to show or hide the mouse cursor");
        config.SetCategory(EFLN_APPROVED);
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowMouseEntitiesInBox(pActInfo);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Update:
        {
            break;
        }
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Get))
            {
                gEnv->pGame->GetIGameFramework()->RegisterListener(this, "FlowMouseEntitiesInBox", FRAMEWORKLISTENERPRIORITY_DEFAULT);

                m_get = true;
                m_actInfo = *pActInfo;
                m_screenX = GetPortInt(pActInfo, InputPorts::ScreenX);
                m_screenY = GetPortInt(pActInfo, InputPorts::ScreenY);
                m_screenX2 = GetPortInt(pActInfo, InputPorts::ScreenX2);
                m_screenY2 = GetPortInt(pActInfo, InputPorts::ScreenY2);
            }
            break;
        }
        }
    }

    // IGameFrameworkListener
    void OnSaveGame(ISaveGame* pSaveGame) override
    {
    }

    void OnLoadGame(ILoadGame* pLoadGame) override
    {
    }

    void OnLevelEnd(const char* nextLevel) override
    {
    }

    void OnActionEvent(const SActionEvent& event) override
    {
    }

    void OnPostUpdate(float fDeltaTime) override
    {
        // Get it once, then unregister to prevent unnescessary updates
        if (!m_get)
        {
            gEnv->pGame->GetIGameFramework()->UnregisterListener(this);
            return;
        }
        m_get = false;

        // Grab the container, and make sure its valid (user has to create it for us and pass the valid ID)
        IFlowSystemContainerPtr pContainer = gEnv->pFlowSystem->GetContainer(GetPortInt(&m_actInfo, InputPorts::ContainerId));

        if (!pContainer)
        {
            return;
        }

        IEntityItPtr pIt = gEnv->pEntitySystem->GetEntityIterator();

        while (!pIt->IsEnd())
        {
            if (IEntity* pEntity = pIt->Next())
            {
                //skip Local player
                if (IPhysicalEntity* physEnt = pEntity->GetPhysics())
                {
                    IActor* pClientActor = gEnv->pGame->GetIGameFramework()->GetClientActor();

                    if (!pClientActor)
                    {
                        return;
                    }

                    //skip the client actor entity
                    if (physEnt == pClientActor->GetEntity()->GetPhysics())
                    {
                        continue;
                    }

                    AABB worldBounds;
                    pEntity->GetWorldBounds(worldBounds);

                    //skip further calculations if the entity is not visible at all...
                    if (gEnv->pSystem->GetViewCamera().IsAABBVisible_F(worldBounds) == CULL_EXCLUSION)
                    {
                        continue;
                    }

                    Vec3 wpos = pEntity->GetWorldPos();
                    Quat rot = pEntity->GetWorldRotation();
                    AABB localBounds;

                    pEntity->GetLocalBounds(localBounds);

                    //get min and max values of the entity bounding box (local positions)
                    Vec3 points[2];
                    points[0] = wpos + rot * localBounds.min;
                    points[1] = wpos + rot * localBounds.max;

                    Vec3 pointsProjected[2];

                    //project the bounding box min max values to screen positions
                    for (int i = 0; i < 2; ++i)
                    {
                        gEnv->pRenderer->ProjectToScreen(points[i].x, points[i].y, points[i].z, &pointsProjected[i].x, &pointsProjected[i].y, &pointsProjected[i].z);
                        const float fWidth = (float)gEnv->pRenderer->GetWidth();
                        const float fHeight = (float)gEnv->pRenderer->GetHeight();

                        //scale projected values to the actual screen resolution
                        pointsProjected[i].x *= 0.01f * fWidth;
                        pointsProjected[i].y *= 0.01f * fHeight;
                    }

                    //check if the projected bounding box min max values are fully or partly inside the screen selection
                    if ((m_screenX <= pointsProjected[0].x && pointsProjected[0].x <= m_screenX2) ||
                        (m_screenX >= pointsProjected[0].x && m_screenX2 <= pointsProjected[1].x) ||
                        (m_screenX <= pointsProjected[1].x && m_screenX2 >= pointsProjected[1].x) ||
                        (m_screenX <= pointsProjected[0].x && m_screenX2 >= pointsProjected[1].x))
                    {
                        if ((m_screenY <= pointsProjected[0].y && m_screenY2 >= pointsProjected[0].y) ||
                            (m_screenY <= pointsProjected[1].y && m_screenY2 >= pointsProjected[0].y) ||
                            (m_screenY <= pointsProjected[1].y && m_screenY2 >= pointsProjected[1].y))
                        {
                            // Add entity to container
                            pContainer->AddItem(TFlowInputData(FlowEntityId(pEntity->GetId())));
                        }
                    }
                }
            }
        }
    }
    // ~IGameFrameworkListener
};

class CFlowMouseSetPosNode
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    CFlowMouseSetPosNode(SActivationInfo* actInfo)
    {
    }

private:
    enum InputNodes
    {
        IN_SOCKET,
        MOUSE_POSITION
    };
    enum OutputNodes
    {
        OUT_SOCKET
    };

    //IFlowNode implementation
    virtual void GetConfiguration(SFlowNodeConfig& config) override;
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override;
    virtual void GetMemoryUsage(ICrySizer* s) const override;
    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowMouseSetPosNode(pActInfo);
    }
};

void CFlowMouseSetPosNode::GetConfiguration(SFlowNodeConfig& config)
{
    static const SInputPortConfig inputs[] = {
        InputPortConfig_AnyType("In", _HELP("Will activate node"), _HELP("In")),
        InputPortConfig<Vec3>("Coords", _HELP("Screen-space location to set hardware mouse"), _HELP("Coords")),
        { 0 }
    };
    static const SOutputPortConfig outputs[] = {
        OutputPortConfig_Void("out", _HELP("Will trigger after mouse position has been set"), _HELP("out")),
        { 0 }
    };


    config.sDescription = _HELP("When triggered, this node will set the hardware mouse to the specified coordinates");
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.SetCategory(EFLN_DEBUG); //meant for use with feature tests
}

void CFlowMouseSetPosNode::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
{
    switch (event)
    {
    case eFE_Initialize:
        pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
        break;
    case eFE_Activate:
        auto mouseCoord = GetPortVec3(pActInfo, InputNodes::MOUSE_POSITION);

        if (IsPortActive(pActInfo, InputNodes::IN_SOCKET))
        {
            if (mouseCoord.x >= 0 && mouseCoord.y >= 0)
            {
                const float normalizedX = static_cast<float>(mouseCoord.x) / static_cast<float>(gEnv->pRenderer->GetWidth());
                const float normalizedY = static_cast<float>(mouseCoord.y) / static_cast<float>(gEnv->pRenderer->GetHeight());
                const AZ::Vector2 systemCursorPositionNormalized(normalizedX, normalizedY);
                AzFramework::InputSystemCursorRequestBus::Event(AzFramework::InputDeviceMouse::Id,
                                                                &AzFramework::InputSystemCursorRequests::SetSystemCursorPositionNormalized,
                                                                systemCursorPositionNormalized);
            }
            ActivateOutput(pActInfo, OUT_SOCKET, true);
        }
        break;
    }
}

void CFlowMouseSetPosNode::GetMemoryUsage(ICrySizer* s) const
{
    s->Add(*this);
}


REGISTER_FLOW_NODE("Input:MouseCoords", CFlowMouseCoordNode);
REGISTER_FLOW_NODE("Input:MouseSetPos", CFlowMouseSetPosNode);
REGISTER_FLOW_NODE("Input:MouseButtonInfo", CFlowMouseButtonNode);
REGISTER_FLOW_NODE("Input:MouseRayCast", CFlowMouseRayCast);
REGISTER_FLOW_NODE("Input:MouseCursor", CFlowMouseCursor);
REGISTER_FLOW_NODE("Input:MouseEntitiesInBox", CFlowMouseEntitiesInBox);
