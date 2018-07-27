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

template <typename T>
struct Limits
{
    static const char* min_name;
    static const char* max_name;
    static const T min_val;
    static const T max_val;
};

template <typename T>
struct LimitsColor
{
    static const char* min_name;
    static const char* max_name;
    static const T min_val;
    static const T max_val;
};

template<typename T>
const char* Limits<T>::min_name = "StartValue";
template<typename T>
const char* Limits<T>::max_name = "EndValue";

template<>
const float Limits<float>::min_val = 0.0f;
template<>
const float Limits<float>::max_val = 1.0f;
template<>
const int Limits<int>::min_val = 0;
template<>
const int Limits<int>::max_val = 255;
template<>
const Vec3 Limits<Vec3>::min_val = Vec3(0.0f, 0.0f, 0.0f);
template<>
const Vec3 Limits<Vec3>::max_val = Vec3(1.0f, 1.0f, 1.0f);
template<>
const Vec3 LimitsColor<Vec3>::min_val = Vec3(0.0f, 0.0f, 0.0f);
template<>
const Vec3 LimitsColor<Vec3>::max_val = Vec3(1.0f, 1.0f, 1.0f);
template<>
const char* LimitsColor<Vec3>::min_name = "color_StartValue";
template<>
const char* LimitsColor<Vec3>::max_name = "color_EndValue";

//////////////////////////////////////////////////////////////////////////
template<class T, class L = Limits<T> >
class CFlowInterpolateNode
    : public CFlowBaseNode<eNCT_Instanced>
{
    enum InputPorts
    {
        Start,
        Stop,
        StartValue,
        EndValue,
        Time,
        UpdateFrequency,
    };

    enum OutputPorts
    {
        Value,
        Done,
    };

    typedef CFlowInterpolateNode<T, L> Self;

public:
    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    CFlowInterpolateNode(SActivationInfo* pActInfo)
        : m_startTime(0.0f)
        , m_endTime(0.0f)
        , m_startValue(L::min_val)
        , m_endValue(L::min_val)
        , m_updateFrequency(0.0f)
        , m_lastUpdateTime(0.0f)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new Self(pActInfo);
    }

    void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
    {
        ser.BeginGroup("Local");
        ser.Value("m_startTime", m_startTime);
        ser.Value("m_endTime", m_endTime);
        ser.Value("m_startValue", m_startValue);
        ser.Value("m_endValue", m_endValue);
        ser.Value("m_updateFrequency", m_updateFrequency);
        ser.Value("m_lastUpdateTime", m_lastUpdateTime);
        ser.EndGroup();
        // regular update is handled by FlowGraph serialization
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config [] =
        {
            InputPortConfig_Void("Start", _HELP("Start Interpolation")),
            InputPortConfig_Void("Stop", _HELP("Stop Interpolation")),
            InputPortConfig<T>(L::min_name,  L::min_val,  _HELP("Start value")),
            InputPortConfig<T>(L::max_name,  L::max_val, _HELP("End value")),
            InputPortConfig<float>("Time", 1.0f, _HELP("Time of interpolation in seconds.")),
            InputPortConfig<float>("UpdateFrequency", 0.0f, _HELP("Update frequency in seconds (0 = every frame).")),
            {0}
        };
        static const SOutputPortConfig out_config [] =
        {
            OutputPortConfig<T>("Value", _HELP("Current Value.")),
            OutputPortConfig_Void("Done", _HELP("Triggered when finished.")),
            {0}
        };

        config.sDescription = _HELP("Interpolation nodes are used to linearly interpolate from a [StartValue] to an [EndValue] within a given time frame.");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    bool GetValue(SActivationInfo* pActInfo, int nPort, T& value)
    {
        T* pVal = (pActInfo->pInputPorts[nPort].GetPtr<T>());
        if (pVal)
        {
            value = *pVal;
            return true;
        }
        return false;
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
            break;
        }
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Stop))
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
                ActivateOutput(pActInfo, OutputPorts::Done, true);
            }
            if (IsPortActive(pActInfo, InputPorts::Start))
            {
                m_startTime = gEnv->pTimer->GetFrameStartTime().GetMilliSeconds();
                m_endTime = m_startTime + GetPortFloat(pActInfo, InputPorts::Time) * 1000.0f;
                GetValue(pActInfo, InputPorts::StartValue, m_startValue);
                GetValue(pActInfo, InputPorts::EndValue, m_endValue);
                m_updateFrequency = GetPortFloat(pActInfo, InputPorts::UpdateFrequency) * 1000.0f;
                m_lastUpdateTime = m_startTime;
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
                Interpol(pActInfo, 0.0f);
            }
            break;
        }
        case eFE_Update:
        {
            const float fTime = gEnv->pTimer->GetFrameStartTime().GetMilliSeconds();
            if (fTime >= m_endTime)
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
                ActivateOutput(pActInfo, OutputPorts::Done, true);
                Interpol(pActInfo, 1.0f);
                m_lastUpdateTime = fTime;
            }
            else if ((fTime - m_lastUpdateTime) >= m_updateFrequency)
            {
                const float fDuration = m_endTime - m_startTime;
                float fPosition;
                if (fDuration <= 0.0f)
                {
                    fPosition = 1.0f;
                }
                else
                {
                    fPosition = (fTime - m_startTime) / fDuration;
                    fPosition = clamp_tpl(fPosition, 0.0f, 1.0f);
                }
                Interpol(pActInfo, fPosition);
                m_lastUpdateTime = fTime;
            }
            break;
        }
        }
    }

protected:
    void Interpol(SActivationInfo* pActInfo, const float fPosition)
    {
        T value = ::Lerp(m_startValue, m_endValue, fPosition);
        ActivateOutput(pActInfo, OutputPorts::Value, value);
    }

    float m_startTime;
    float m_endTime;
    T m_startValue;
    T m_endValue;
    float m_updateFrequency, m_lastUpdateTime;
};

//////////////////////////////////////////////////////////////////////////
namespace FlowInterpolateNodes
{
    template<class T>
    bool ValueReached(const T& currVal, const T& toVal, const T& minDistToVal)
    {
        return currVal == toVal;
    }

    template<>
    bool ValueReached(const float& currVal, const float& toVal, const float& minDistToVal)
    {
        return fabsf(currVal - toVal) <= minDistToVal;
    }

    template<>
    bool ValueReached(const Vec3& currVal, const Vec3& toVal, const Vec3& minDistToVal)
    {
        bool bdone = fabsf(currVal.x - toVal.x) <= minDistToVal.x &&
            fabsf(currVal.y - toVal.y) <= minDistToVal.y &&
            fabsf(currVal.z - toVal.z) <= minDistToVal.z;
        return bdone;
    }

    template<class T>
    T CalcMinDistToVal(const T& startVal, const T& toVal)
    {
        return 0;
    }

    template<>
    float CalcMinDistToVal(const float& startVal, const float& toVal)
    {
        const float FACTOR = (1 / 100.f);
        const float MIN_DIST = 0.01f;
        return max(fabsf(toVal - startVal) * FACTOR, MIN_DIST);
    }

    template<>
    Vec3 CalcMinDistToVal(const Vec3& startVal, const Vec3& toVal)
    {
        const float FACTOR = (1 / 100.f);
        const float MIN_DIST = 0.01f;
        Vec3 calc(max(fabsf(toVal.x - startVal.x) * FACTOR, MIN_DIST),
            max(fabsf(toVal.y - startVal.y) * FACTOR, MIN_DIST),
            max(fabsf(toVal.z - startVal.z) * FACTOR, MIN_DIST)
            );
        return calc;
    }
}

//////////////////////////////////////////////////////////////////////////
// Lumberyard Note:
// The smooth int node seems to be broken.  It never reaches the end value,
// and never outputs 'Done'.
// These smooth nodes do slow down at the end but there's a noticable 'snap'
// at the very end.  Seems to be how it's calculating the 'Done' condition.
template<class T, class L = Limits<T> >
class CFlowSmoothNode
    : public CFlowBaseNode<eNCT_Instanced>
{
    enum InputPorts
    {
        Activate,
        InitialValue,
        TargetValue,
        Time,
    };

    enum OutputPorts
    {
        Value,
        Done,
    };

    typedef CFlowSmoothNode<T, L> Self;

public:
    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    CFlowSmoothNode(SActivationInfo* pActInfo)
        : m_val(L::min_val)
        , m_valRate(L::min_val)
        , m_minDistToVal(L::min_val)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new Self(pActInfo);
    }

    void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
    {
        ser.BeginGroup("Local");
        ser.Value("m_val", m_val);
        ser.Value("m_valRate", m_valRate);
        ser.Value("m_minDistToVal", m_minDistToVal);
        ser.EndGroup();
        // regular update is handled by FlowGraph serialization
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config [] =
        {
            InputPortConfig<T>("InitialValue",  L::min_val,  _HELP("Initial value")),
            InputPortConfig<T>("TargetValue", L::min_val, _HELP("Target value")),
            InputPortConfig<float>("Time",  1.0f, _HELP("Smoothing time (seconds)")),
            {0}
        };
        static const SOutputPortConfig out_config [] =
        {
            OutputPortConfig<T>("Value", _HELP("Current Value")),
            OutputPortConfig_Void("Done", _HELP("Triggered when interpolation is finished.")),
            {0}
        };

        config.sDescription = _HELP("Smooth nodes are used to interpolate in a non-linear way (damped spring system) from a [StartValue] to an [EndValue] within a given time frame.\nCalculation will slow as it reaches the end value.\nNote: The interpolation is updated when [TargetValue] port is activated.");
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    bool GetValue(SActivationInfo* pActInfo, int nPort, T& value)
    {
        T* pVal = (pActInfo->pInputPorts[nPort].GetPtr<T>());
        if (pVal)
        {
            value = *pVal;
            return true;
        }
        return false;
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            GetValue(pActInfo, InputPorts::InitialValue, m_val);
            m_valRate = L::min_val;
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
            break;
        }

        case eFE_Activate:
        {
            // always get the target value, it can be updated
            T toValue;
            GetValue(pActInfo, InputPorts::TargetValue, toValue);

            if (IsPortActive(pActInfo, InputPorts::InitialValue))
            {
                GetValue(pActInfo, InputPorts::InitialValue, m_val);
            }

            m_minDistToVal = FlowInterpolateNodes::CalcMinDistToVal(m_val, toValue);

            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
            }

            break;
        }

        case eFE_Update:
        {
            T toValue;
            GetValue(pActInfo, InputPorts::TargetValue, toValue);
            SmoothCD(m_val, m_valRate, gEnv->pTimer->GetFrameTime(), toValue, GetPortFloat(pActInfo, InputPorts::Time));
            if (FlowInterpolateNodes::ValueReached(m_val, toValue, m_minDistToVal))
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
                ActivateOutput(pActInfo, OutputPorts::Done, true);
                m_val = toValue;
                m_valRate = L::min_val;
            }
            ActivateOutput(pActInfo, OutputPorts::Value, m_val);
            break;
        }
        }
    }

    T m_val;
    T m_valRate;
    T m_minDistToVal;  // used to check when the value is so near that it must be considered reached
};

class CFlowSmoothAngleVec3
    : public CFlowBaseNode < eNCT_Instanced >
{
    enum InputPorts
    {
        Activate,
        InitialValue,
        TargetValue,
        Time,
    };

    enum OutputPorts
    {
        Value,
        Done,
    };

    typedef CFlowSmoothAngleVec3 Self;

public:
    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    CFlowSmoothAngleVec3(SActivationInfo* pActInfo)
        : m_val(Limits<Vec3>::min_val)
        , m_valRate(Limits<Vec3>::min_val)
        , m_minDistToVal(Limits<Vec3>::min_val)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new Self(pActInfo);
    }

    void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
    {
        ser.BeginGroup("Local");
        ser.Value("m_val", m_val);
        ser.Value("m_valRate", m_valRate);
        ser.Value("m_minDistToVal", m_minDistToVal);
        ser.EndGroup();
        // regular update is handled by FlowGraph serialization
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<Vec3>("InitialValue", Limits<Vec3>::min_val, _HELP("Initial value")),
            InputPortConfig<Vec3>("TargetValue", Limits<Vec3>::min_val, _HELP("Target value")),
            InputPortConfig<float>("Time", 1.0f, _HELP("Smoothing time (seconds)")),
            { 0 }
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<Vec3>("Value", _HELP("Current Value")),
            OutputPortConfig_Void("Done", _HELP("Triggered when interpolation is finished.")),
            { 0 }
        };

        config.sDescription = _HELP("Smooth nodes are used to interpolate in a non-linear way (damped spring system) from a [StartValue] to an [EndValue] within a given time frame.\nCalculation will slow as it reaches the end value.\nNote: The interpolation is updated when [TargetValue] port is activated.");
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    bool GetValue(SActivationInfo* pActInfo, int nPort, Vec3& value)
    {
        Vec3* pVal = (pActInfo->pInputPorts[nPort].GetPtr<Vec3>());
        if (pVal)
        {
            value = *pVal;
            return true;
        }
        return false;
    }

    Vec3 CalcTargetMod360(Vec3 initial, Vec3 target)
    {
        // Find interval to test that contains the initial value.
        Vec3 Bias;
        Bias.x = target.x < initial.x ? 360.0f : -360.0f;
        Bias.y = target.y < initial.y ? 360.0f : -360.0f;
        Bias.z = target.z < initial.z ? 360.0f : -360.0f;

        // Find shortest path between two intervals.
        {
            Vec3 t1 = target;
            Vec3 t2 = target + Bias;
            target.x = fabs(initial.x - t1.x) < fabs(initial.x - t2.x) ? t1.x : t2.x;
            target.y = fabs(initial.y - t1.y) < fabs(initial.y - t2.y) ? t1.y : t2.y;
            target.z = fabs(initial.z - t1.z) < fabs(initial.z - t2.z) ? t1.z : t2.z;
        }

        return target;
    }

    Vec3 Mod360(Vec3 input)
    {
        input.x = fmod(input.x, 360.0f);
        input.y = fmod(input.y, 360.0f);
        input.z = fmod(input.z, 360.0f);
        return input;
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            GetValue(pActInfo, InputPorts::InitialValue, m_val);
            m_valRate = Limits<Vec3>::min_val;
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
            break;
        }

        case eFE_Activate:
        {
            // always get the target value, it can be updated
            Vec3 toValue;
            GetValue(pActInfo, InputPorts::TargetValue, toValue);

            if (IsPortActive(pActInfo, InputPorts::InitialValue))
            {
                GetValue(pActInfo, InputPorts::InitialValue, m_val);
            }

            // Bias to (0, 360)
            Vec3 initial = m_val + Vec3(180.0f);
            Vec3 target = toValue + Vec3(180.0f);

            target = CalcTargetMod360(initial, target);

            m_minDistToVal = FlowInterpolateNodes::CalcMinDistToVal(initial, target);

            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
            }

            break;
        }

        case eFE_Update:
        {
            Vec3 toValue;
            GetValue(pActInfo, InputPorts::TargetValue, toValue);

            // Bias to (0, 360)
            Vec3 initial = m_val + Vec3(180.0f);
            Vec3 target = toValue + Vec3(180.0f);
            target = CalcTargetMod360(initial, target);

            SmoothCD(initial, m_valRate, gEnv->pTimer->GetFrameTime(), target, GetPortFloat(pActInfo, InputPorts::Time));
            if (FlowInterpolateNodes::ValueReached(initial, target, m_minDistToVal))
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
                ActivateOutput(pActInfo, OutputPorts::Done, true);

                target = Mod360(target) - Vec3(180.0f);

                m_val = target;
                m_valRate = Limits<Vec3>::min_val;
            }
            else
            {
                m_val = Mod360(initial) - Vec3(180.0f);
            }

            ActivateOutput(pActInfo, OutputPorts::Value, m_val);
            break;
        }
        }
    }

    Vec3 m_val;
    Vec3 m_valRate;
    Vec3 m_minDistToVal;  // used to check when the value is so near that it must be considered reached
};


//////////////////////////////////////////////////////////////////////////
/// Interpolate Flow Node Registration
//////////////////////////////////////////////////////////////////////////

typedef CFlowInterpolateNode<int> CIntInterpolateNode;
typedef CFlowInterpolateNode<float> CFloatInterpolateNode;
typedef CFlowInterpolateNode<Vec3> CVec3InterpolateNode;
typedef CFlowInterpolateNode<Vec3, LimitsColor<Vec3> > CColorInterpolateNode;

REGISTER_FLOW_NODE("Interpolate:Int", CIntInterpolateNode);
REGISTER_FLOW_NODE("Interpolate:Float", CFloatInterpolateNode);
REGISTER_FLOW_NODE("Interpolate:Vec3", CVec3InterpolateNode);
REGISTER_FLOW_NODE("Interpolate:Color", CColorInterpolateNode);

typedef CFlowSmoothNode<int> CSmoothIntNode;
typedef CFlowSmoothNode<float> CSmoothFloatNode;
typedef CFlowSmoothNode<Vec3> CSmoothVec3Node;
typedef CFlowSmoothNode<Vec3> CSmoothColorNode;

REGISTER_FLOW_NODE("Interpolate:SmoothInt", CSmoothIntNode);
REGISTER_FLOW_NODE("Interpolate:SmoothFloat", CSmoothFloatNode);
REGISTER_FLOW_NODE("Interpolate:SmoothVec3", CSmoothVec3Node);
REGISTER_FLOW_NODE("Interpolate:SmoothColor", CSmoothColorNode);

REGISTER_FLOW_NODE("Interpolate:SmoothAngleVec3", CFlowSmoothAngleVec3);
