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

#include "CryLegacy_precompiled.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include "I3DEngine.h"
#include "ITerrain.h"
#include "ITimeOfDay.h"

class CFlowNode_EnvLighting
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_EnvLighting(SActivationInfo* pActInfo)
    {
    }

    enum EInputPorts
    {
        eIP_TimeOfDay = 0,
    };

    enum EOutputPorts
    {
        eOP_SunDirection = 0,
    };

    void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig<float>("TimeOfDay"),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<Vec3>("SunDirection"),
            {0}
        };
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_OBSOLETE);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
            if (IsPortActive(pActInfo, eIP_TimeOfDay))
            {
                Update(pActInfo);
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

private:

    void Update(SActivationInfo* pActInfo)
    {
        float timeOfDay = GetPortFloat(pActInfo, eIP_TimeOfDay);
        float longitude = timeOfDay / 24.0f * gf_PI * 2.0f;
        float latitude = -45.0f * gf_PI / 180.0f;
        Vec3 sunDir(sinf(longitude) * cosf(latitude), sinf(longitude) * sinf(latitude), cosf(longitude));

        CRY_ASSERT(!"Direct call of I3DEngine::SetSunDir() is not supported anymore. Use Time of day featue instead.");
        //      gEnv->p3DEngine->SetSunDir(sunDir);
        ActivateOutput(pActInfo, eOP_SunDirection, sunDir);
    }
};

class CFlowNode_EnvMoonDirection
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_EnvMoonDirection(SActivationInfo* pActInfo)
    {
    }

    enum EInputPorts
    {
        eIP_Get = 0,
        eIP_Set,
        eIP_Latitude,
        eIP_Longitude,
        eIP_ForceUpdate,
    };

    enum EOutputPorts
    {
        eOP_Latitude = 0,
        eOP_Longitude,
    };

    void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void("Get", _HELP("Get the current Latitude/Longitude")),
            InputPortConfig_Void("Set", _HELP("Set the Latitude/Longitude")),
            InputPortConfig<float>("Latitude",  0.0f, _HELP("Latitude to be set")),
            InputPortConfig<float>("Longitude", 35.0f, _HELP("Longitude to be set")),
            InputPortConfig<bool>("ForceUpdate", false, _HELP("Force Immediate Update of Sky if true. USE ONLY IN SPECIAL CASES, is heavy on performance!")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<float>("Latitude", _HELP("Current Latitude")),
            OutputPortConfig<float>("Longitude", _HELP("Current Longitude")),
            {0}
        };
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.sDescription = _HELP("Set the 3DEngine's Moon Direction");
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
            if (IsPortActive(pActInfo, eIP_Get))
            {
                Vec3 v;
                gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKY_MOONROTATION, v);
                ActivateOutput(pActInfo, eOP_Latitude, v.x);
                ActivateOutput(pActInfo, eOP_Longitude, v.y);
            }
            if (IsPortActive(pActInfo, eIP_Set))
            {
                Vec3 v(ZERO);
                v.x = GetPortFloat(pActInfo, eIP_Latitude);
                v.y = GetPortFloat(pActInfo, eIP_Longitude);
                gEnv->p3DEngine->SetGlobalParameter(E3DPARAM_SKY_MOONROTATION, v);
                ActivateOutput(pActInfo, eOP_Latitude, v.x);
                ActivateOutput(pActInfo, eOP_Longitude, v.y);
                bool forceUpdate = GetPortBool(pActInfo, eIP_ForceUpdate);
                ITimeOfDay* pTOD = gEnv->p3DEngine->GetTimeOfDay();
                if (forceUpdate && pTOD)
                {
                    pTOD->Update(true, true);
                }
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};


class CFlowNode_EnvSun
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_EnvSun(SActivationInfo* pActInfo)
    {
    }

    enum EInputPorts
    {
        eIP_Get = 0,
        eIP_Set,
        eIP_Latitude,
        eIP_Longitude,
        eIP_ForceUpdate,
    };

    enum EOutputPorts
    {
        eOP_Latitude = 0,
        eOP_Longitude,
    };

    void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void("Get", _HELP("Get the current Latitude/Longitude")),
            InputPortConfig_Void("Set", _HELP("Set the Latitude/Longitude")),
            InputPortConfig<float>("Latitude",  0.0f, _HELP("Latitude to be set")),
            InputPortConfig<float>("Longitude", 35.0f, _HELP("Longitude to be set")),
            InputPortConfig<bool>("ForceUpdate", false, _HELP("Force Immediate Update of Sky if true. USE ONLY IN SPECIAL CASES, is heavy on performance!")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<float>("Latitude", _HELP("Current Latitude")),
            OutputPortConfig<float>("Longitude", _HELP("Current Longitude")),
            {0}
        };
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.sDescription = _HELP("Set the 3DEngine's Sun Data");
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
            if (IsPortActive(pActInfo, eIP_Get))
            {
                ITimeOfDay* pTOD = gEnv->p3DEngine->GetTimeOfDay();
                if (pTOD)
                {
                    ActivateOutput(pActInfo, eOP_Latitude, pTOD->GetSunLatitude());
                    ActivateOutput(pActInfo, eOP_Longitude, pTOD->GetSunLongitude());
                }
            }
            if (IsPortActive(pActInfo, eIP_Set))
            {
                ITimeOfDay* pTOD = gEnv->p3DEngine->GetTimeOfDay();
                if (pTOD)
                {
                    float latitude = GetPortFloat(pActInfo, eIP_Latitude);
                    float longitude = GetPortFloat(pActInfo, eIP_Longitude);

                    pTOD->SetSunPos(longitude, latitude);
                    bool forceUpdate = GetPortBool(pActInfo, eIP_ForceUpdate);
                    pTOD->Update(true, forceUpdate);

                    ActivateOutput(pActInfo, eOP_Latitude, latitude);
                    ActivateOutput(pActInfo, eOP_Longitude, longitude);
                }
            }
            break;
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};


//////////////////////////////////////////////////////////////////////////
class CFlowNode_TimeOfDayTransitionTrigger
    : public CFlowBaseNode<eNCT_Instanced>
{
    typedef uint32 InternalID; // InternalID is used to control that only 1 of those nodes is active at any time

    float               m_startTOD;
    float               m_startSunLatitude;
    float               m_startSunLongitude;
    float               m_durationLeft;
    float               m_TODTimeToUpdate;  // used when update is not every frame
    float               m_sunTimeToUpdate;  // used when update is not every frame
    InternalID  m_ID;
    bool                m_blending;
    bool                m_paused;

    static InternalID m_IDCounter;
    static InternalID m_activeID;

public:
    enum EInputs
    {
        IN_TOD = 0,
        IN_DURATION,
        IN_SUN_LATITUDE,
        IN_SUN_LONGITUDE,
        IN_SUN_POSITION_UPDATE_INTERVAL,
        IN_TOD_FORCE_UPDATE_INTERVAL,
        IN_START,
        IN_PAUSE
    };
    enum EOutputs
    {
        OUT_DONE = 0,
    };

    CFlowNode_TimeOfDayTransitionTrigger(SActivationInfo* pActInfo)
        : m_startTOD(0.0f)
        , m_startSunLatitude(0.0f)
        , m_startSunLongitude(0.0f)
        , m_durationLeft(0.0f)
        , m_TODTimeToUpdate(0.0f)
        , m_sunTimeToUpdate(0.0f)
        , m_ID(m_IDCounter++)
        , m_blending(false)
        , m_paused(false)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowNode_TimeOfDayTransitionTrigger(pActInfo);
    }

    virtual void Serialize(SActivationInfo*, TSerialize ser)
    {
        ser.Value("blending", m_blending);
        ser.Value("IDCounter", m_IDCounter); // those statics will get serialized more than once (once for each node), but does not matter much
        ser.Value("activeID", m_activeID);
        if (m_blending)
        {
            ser.Value("paused", m_paused);
            ser.Value("startTOD", m_startTOD);
            ser.Value("startSunLatitude", m_startSunLatitude);
            ser.Value("startSunLongitude", m_startSunLongitude);
            ser.Value("durationLeft", m_durationLeft);
            ser.Value("TODTimeToUpdate", m_TODTimeToUpdate);
            ser.Value("sunTimeToUpdate", m_sunTimeToUpdate);
        }
    }


    void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig<float>("Time", -1.0f, _HELP("Blend from the current time in the level to this new specified time. (24 hours value. -1 = no TOD blending)")),
            InputPortConfig<float>("Duration",  0.f, _HELP("Blend duration in seconds")),
            InputPortConfig<float>("SunLatitude", -1.f, _HELP("Blend from the current sun latitude in the level to this new specified latitude (value in degrees. -1=no latitude blending)")),
            InputPortConfig<float>("SunLongitude", -1.f, _HELP("blend from the current sun longitude in the level to this new specified longitude (value in degrees. -1=no longitude blending)")),
            InputPortConfig<float>("SunPositionUpdateInterval", 1.f,  _HELP("Amount of time in seconds between every update for the repositioning of the sun (0 second means the sun position is constantly updated during the transition) ")),
            InputPortConfig<float>("ForceUpdateInterval", 1.0f, _HELP("amount of time in seconds between every update of the time of day (0 second means the time of day is constantly updated during the transition, not recommended for low spec) ")),
            InputPortConfig_Void("Start", _HELP("Starts the transition")),
            InputPortConfig_Void("Pause", _HELP("Pause/Unpause the transition")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig_Void("Done", _HELP("triggers when transition is finished")),
            {0}
        };

        config.sDescription = _HELP("Time of Day and sun position transitions.");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            m_blending = false;
            m_paused = false;
            break;
        }

        case eFE_Activate:
        {
            ITimeOfDay* pTOD = gEnv->p3DEngine->GetTimeOfDay();
            if (!pTOD)
            {
                break;
            }

            if (IsPortActive(pActInfo, IN_START))
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
                m_durationLeft = GetPortFloat(pActInfo, IN_DURATION);
                m_sunTimeToUpdate = GetPortFloat(pActInfo, IN_SUN_POSITION_UPDATE_INTERVAL);
                m_TODTimeToUpdate = GetPortFloat(pActInfo, IN_TOD_FORCE_UPDATE_INTERVAL);
                m_startSunLongitude = pTOD->GetSunLongitude();
                m_startSunLatitude = pTOD->GetSunLatitude();
                m_startTOD = pTOD->GetTime();
                m_blending = true;
                m_paused = false;
                m_activeID = m_ID;
            }
            if (IsPortActive(pActInfo, IN_PAUSE))
            {
                if (m_blending)
                {
                    m_paused = !m_paused;
                    pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, !m_paused);
                }
            }
            break;
        }

        case eFE_Update:
        {
            if (m_activeID != m_ID)
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
                m_blending = false;
                break;
            }

            ITimeOfDay* pTOD = gEnv->p3DEngine->GetTimeOfDay();
            if (!pTOD)
            {
                break;
            }

            m_durationLeft -= gEnv->pTimer->GetFrameTime();
            bool forceUpdate = false;
            if (m_durationLeft <= 0)
            {
                m_durationLeft = 0;
                m_blending = false;
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
                ActivateOutput(pActInfo, OUT_DONE, true);
                forceUpdate = true;
                m_activeID = 0xffffffff;
            }
            float totalDuration = GetPortFloat(pActInfo, IN_DURATION);
            float blendPos = totalDuration == 0 ? 1.f : 1.f - (m_durationLeft / totalDuration);

            bool needUpdate = UpdateTOD(blendPos, pActInfo, pTOD, forceUpdate);
            needUpdate |= UpdateSun(blendPos, pActInfo, pTOD, forceUpdate);

            if (needUpdate)
            {
                pTOD->Update(true, true);
            }
            break;
        }
        }
    }


    bool UpdateTOD(float blendPos, SActivationInfo* pActInfo, ITimeOfDay* pTOD, bool forceUpdate)
    {
        bool needUpdate = forceUpdate;
        float endTOD = GetPortFloat(pActInfo, IN_TOD);
        if (endTOD < 0)
        {
            return needUpdate;
        }

        m_TODTimeToUpdate -= gEnv->pTimer->GetFrameTime();
        if (m_TODTimeToUpdate <= 0 || forceUpdate)
        {
            m_TODTimeToUpdate = GetPortFloat(pActInfo, IN_TOD_FORCE_UPDATE_INTERVAL);
            float currTime = m_startTOD + (endTOD - m_startTOD) * blendPos;
            pTOD->SetTime(currTime, false);
            needUpdate = true;
        }
        return needUpdate;
    }


    bool UpdateSun(float blendPos, SActivationInfo* pActInfo, ITimeOfDay* pTOD, bool forceUpdate)
    {
        bool needUpdate = forceUpdate;
        m_sunTimeToUpdate -= gEnv->pTimer->GetFrameTime();
        if (m_sunTimeToUpdate > 0 && !forceUpdate)
        {
            return needUpdate;
        }

        m_sunTimeToUpdate = GetPortFloat(pActInfo, IN_SUN_POSITION_UPDATE_INTERVAL);

        float endLatitude = GetPortFloat(pActInfo, IN_SUN_LATITUDE);
        float currLatitude = pTOD->GetSunLatitude();
        if (endLatitude >= 0)
        {
            currLatitude = m_startSunLatitude + (endLatitude - m_startSunLatitude) * blendPos;
            needUpdate = true;
        }

        float endLongitude = GetPortFloat(pActInfo, IN_SUN_LONGITUDE);
        float currLongitude = pTOD->GetSunLongitude();
        if (endLongitude >= 0)
        {
            currLongitude = m_startSunLongitude + (endLongitude - m_startSunLongitude) * blendPos;
            needUpdate = true;
        }
        if (needUpdate)
        {
            pTOD->SetSunPos(currLongitude, currLatitude);
        }
        return needUpdate;
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

CFlowNode_TimeOfDayTransitionTrigger::InternalID CFlowNode_TimeOfDayTransitionTrigger::m_IDCounter = 0;
CFlowNode_TimeOfDayTransitionTrigger::InternalID CFlowNode_TimeOfDayTransitionTrigger::m_activeID = 0xffffffff;

class CFlowNode_EnvWind
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_EnvWind(SActivationInfo* pActInfo)
    {
    }

    enum EInputPorts
    {
        eIP_Get = 0,
    };

    enum EOutputPorts
    {
        eOP_WindVector,
    };

    void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void("Get", _HELP("Get the Current Environment Wind Vector")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<Vec3>("WindVector", _HELP("Current Environment Wind Vector")),
            {0}
        };
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.sDescription = _HELP("Get the Environment's Wind Data");
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
            if (IsPortActive(pActInfo, eIP_Get))
            {
                ActivateOutput(pActInfo, eOP_WindVector, gEnv->p3DEngine->GetGlobalWind(false));
            }
            break;
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_SetOceanMat
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_SetOceanMat(SActivationInfo* pActInfo)
    {
    }

    enum EInputPorts
    {
        eIP_Set = 0,
        eIP_Material,
    };

    enum EOutputPorts
    {
        eOP_Success = 0,
        eOP_Failed,
    };

    void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void("Set", _HELP("Sets the material on the ocean")),
            InputPortConfig<string>("Material", _HELP("Material of choice to be set"), 0, _UICONFIG("dt=mat")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig_Void("Success", _HELP("Triggered when successfully finished")),
            OutputPortConfig_Void("Failed", _HELP("Triggered when something went wrong")),
            {0}
        };
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.sDescription = _HELP("Set the ocean material");
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
            if (IsPortActive(pActInfo, eIP_Set))
            {
                string mat = GetPortString(pActInfo, eIP_Material);
                _smart_ptr<IMaterial> pMat = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(mat.c_str(), false);
                if (pMat)
                {
                    gEnv->p3DEngine->GetITerrain()->ChangeOceanMaterial(pMat);
                    ActivateOutput(pActInfo, eOP_Success, 0);
                }
                else
                {
                    ActivateOutput(pActInfo, eOP_Failed, 0);
                }
            }
            break;
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

class CFlowNode_SkyMaterialSwitch
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_SkyMaterialSwitch(SActivationInfo* pActInfo){};
    virtual void GetMemoryUsage(ICrySizer* s) const;
    void GetConfiguration(SFlowNodeConfig& config);
    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo);
};

void CFlowNode_SkyMaterialSwitch::GetMemoryUsage(ICrySizer* s) const
{
    s->Add(*this);
}

void CFlowNode_SkyMaterialSwitch::GetConfiguration(SFlowNodeConfig& config)
{
    static const SInputPortConfig in_config[] = {
        InputPortConfig<string> ("mat_Material", _HELP("Skybox material name")),
        InputPortConfig<bool>("Start", false, _HELP("Trigger to start the loading")),
        InputPortConfig<float>("Angle", 1.f, _HELP("Sky box rotation")),
        InputPortConfig<float>("Stretching", 1.f, _HELP("Sky box stretching")),
        {0}
    };

    config.pInputPorts = in_config;
    config.sDescription = _HELP("Node for sky box switching");
    config.SetCategory(EFLN_ADVANCED);
}

void CFlowNode_SkyMaterialSwitch::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
{
    switch (event)
    {
    case eFE_Activate:
        if (IsPortActive(pActInfo, 1))
        {
            gEnv->p3DEngine->SetGlobalParameter(E3DPARAM_SKY_SKYBOX_ANGLE, GetPortFloat(pActInfo, 2));
            gEnv->p3DEngine->SetGlobalParameter(E3DPARAM_SKY_SKYBOX_STRETCHING, GetPortFloat(pActInfo, 3));

            string mat = GetPortString(pActInfo, 0);

            _smart_ptr<IMaterial> pSkyMtl = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(mat, false, false, IMaterialManager::ELoadingFlagsPreviewMode);

            if (pSkyMtl != NULL)
            {
                gEnv->p3DEngine->SetSkyMaterial(pSkyMtl);
                gEnv->pLog->Log("Sky box switched");
            }
            else
            {
                gEnv->pLog->LogError("Error switching sky box: can't find material");
            }
        }
        break;
    }
}

REGISTER_FLOW_NODE("Environment:Lighting", CFlowNode_EnvLighting);
REGISTER_FLOW_NODE("Environment:MoonDirection", CFlowNode_EnvMoonDirection);
REGISTER_FLOW_NODE("Environment:Sun", CFlowNode_EnvSun);
REGISTER_FLOW_NODE("Time:TimeOfDayTransitionTrigger", CFlowNode_TimeOfDayTransitionTrigger)
REGISTER_FLOW_NODE("Environment:Wind", CFlowNode_EnvWind)
REGISTER_FLOW_NODE("Environment:SetOceanMaterial", CFlowNode_SetOceanMat)
REGISTER_FLOW_NODE("Environment:SkyMaterialSwitch", CFlowNode_SkyMaterialSwitch);
