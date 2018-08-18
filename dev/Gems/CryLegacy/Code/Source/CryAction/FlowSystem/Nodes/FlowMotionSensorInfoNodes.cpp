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
#include "IInput.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class CFlowMotionSensorNode
    : public CFlowBaseNode<eNCT_Instanced>
    , public IMotionSensorEventListener
{
protected:
    enum Inputs
    {
        Input_Enable = 0,
        Input_Disable
    };

    enum Outputs
    {
        Output_MotionSensorData = 0,
        Output_IsSensorDataAvailable
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    CFlowMotionSensorNode(SActivationInfo* activationInfo, EMotionSensorFlags sensorFlags)
        : CFlowBaseNode()
        , m_actInfo(*activationInfo)
        , m_sensorFlags(sensorFlags)
        , m_enabled(false)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ~CFlowMotionSensorNode() override
    {
        Disable();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void Serialize(SActivationInfo* activationInfo, TSerialize ser) override
    {
        bool enabled = m_enabled;
        ser.Value("enabled", enabled);
        if (ser.IsReading())
        {
            enabled ? Enable() : Disable();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Enable", _HELP("Enable motion sensor input")),
            InputPortConfig_Void("Disable", _HELP("Disable motion sensor input")),
            { 0 }
        };

        static SOutputPortConfig outputs[] =
        {
            OutputPortConfig<Vec3>("SensorData", ""),
            OutputPortConfig<bool>("IsSensorDataAvailable", _HELP("Outputs true or false when this node is activated")),
            { 0 }
        };
        outputs[0].description = GetSensorDataDescription();

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = GetNodeDescription();
        config.SetCategory(EFLN_APPROVED);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    virtual const char* GetNodeDescription() const = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    virtual const char* GetSensorDataDescription() const = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        m_actInfo = *activationInfo;
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(activationInfo, Input_Disable))
            {
                Disable();
            }
            else if (IsPortActive(activationInfo, Input_Enable))
            {
                Enable();
                ActivateOutput(&m_actInfo, Output_IsSensorDataAvailable, IsSensorDataAvailable());
            }
        }
        break;
        case eFE_Uninitialize:
        {
            Disable();
        }
        break;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline IInput* GetIInput()
    {
        ISystem* system = GetISystem();
        return system ? system->GetIInput() : nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline bool IsSensorDataAvailable()
    {
        const IInput* input = GetIInput();
        return input ? input->IsMotionSensorDataAvailable(m_sensorFlags) : false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void Enable()
    {
        if (!m_enabled)
        {
            m_enabled = true;
            if (IInput* input = GetIInput())
            {
                input->AddMotionSensorEventListener(this, m_sensorFlags);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void Disable()
    {
        if (m_enabled)
        {
            m_enabled = false;
            if (IInput* input = GetIInput())
            {
                input->RemoveMotionSensorEventListener(this, m_sensorFlags);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void OnMotionSensorEvent(const SMotionSensorEvent& event) override
    {
        if (!m_enabled)
        {
            return;
        }

        if (!(event.updatedFlags & m_sensorFlags))
        {
            return;
        }

        switch (m_sensorFlags)
        {
        case eMSF_AccelerationRaw:
        {
            ActivateOutput(&m_actInfo, Output_MotionSensorData, event.sensorData.accelerationRaw);
        }
        break;
        case eMSF_AccelerationUser:
        {
            ActivateOutput(&m_actInfo, Output_MotionSensorData, event.sensorData.accelerationUser);
        }
        break;
        case eMSF_AccelerationGravity:
        {
            ActivateOutput(&m_actInfo, Output_MotionSensorData, event.sensorData.accelerationGravity);
        }
        break;
        case eMSF_RotationRateRaw:
        {
            ActivateOutput(&m_actInfo, Output_MotionSensorData, RAD2DEG(event.sensorData.rotationRateRaw));
        }
        break;
        case eMSF_RotationRateUnbiased:
        {
            ActivateOutput(&m_actInfo, Output_MotionSensorData, RAD2DEG(event.sensorData.rotationRateUnbiased));
        }
        break;
        case eMSF_MagneticFieldRaw:
        {
            ActivateOutput(&m_actInfo, Output_MotionSensorData, event.sensorData.magneticFieldRaw);
        }
        break;
        case eMSF_MagneticFieldUnbiased:
        {
            ActivateOutput(&m_actInfo, Output_MotionSensorData, event.sensorData.magneticFieldUnbiased);
        }
        break;
        case eMSF_MagneticNorth:
        {
            ActivateOutput(&m_actInfo, Output_MotionSensorData, event.sensorData.magneticNorth);
        }
        break;
        case eMSF_Orientation:
        {
            ActivateOutput(&m_actInfo, Output_MotionSensorData, Vec3(RAD2DEG(Ang3(event.sensorData.orientation))));
        }
        break;
        case eMSF_OrientationDelta:
        {
            ActivateOutput(&m_actInfo, Output_MotionSensorData, Vec3(RAD2DEG(Ang3(event.sensorData.orientationDelta))));
        }
        break;
        }
    }

private:
    SActivationInfo m_actInfo;
    EMotionSensorFlags m_sensorFlags;
    bool m_enabled;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class CFlowAccelerationRawNode
    : public CFlowMotionSensorNode
{
public:
    CFlowAccelerationRawNode(SActivationInfo* i)
        : CFlowMotionSensorNode(i, eMSF_AccelerationRaw) {}
    ~CFlowAccelerationRawNode() override {}
    IFlowNodePtr Clone(SActivationInfo* i) override { return new CFlowAccelerationRawNode(i); }
    void GetMemoryUsage(ICrySizer* sizer) const override { sizer->Add(*this); }
    const char* GetNodeDescription() const override { return _HELP("Outputs raw acceleration as measured by the acceleromter,\
                                                                    which includes acceleration due to gravity and user interaction"); }
    const char* GetSensorDataDescription() const override { return _HELP("Raw acceleration measured in g-forces (1G ~= 9.8 m/s^2)"); }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class CFlowAccelerationUserNode
    : public CFlowMotionSensorNode
{
public:
    CFlowAccelerationUserNode(SActivationInfo* i)
        : CFlowMotionSensorNode(i, eMSF_AccelerationUser) {}
    ~CFlowAccelerationUserNode() override {}
    IFlowNodePtr Clone(SActivationInfo* i) override { return new CFlowAccelerationUserNode(i); }
    void GetMemoryUsage(ICrySizer* sizer) const override { sizer->Add(*this); }
    const char* GetNodeDescription() const override { return _HELP("Outputs user generated acceleration that is filtered from the raw acceleration"); }
    const char* GetSensorDataDescription() const override { return _HELP("User generated acceleration measured in g-forces (1G ~= 9.8 m/s^2)"); }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class CFlowAccelerationGravityNode
    : public CFlowMotionSensorNode
{
public:
    CFlowAccelerationGravityNode(SActivationInfo* i)
        : CFlowMotionSensorNode(i, eMSF_AccelerationGravity) {}
    ~CFlowAccelerationGravityNode() override {}
    IFlowNodePtr Clone(SActivationInfo* i) override { return new CFlowAccelerationGravityNode(i); }
    void GetMemoryUsage(ICrySizer* sizer) const override { sizer->Add(*this); }
    const char* GetNodeDescription() const override { return _HELP("Outputs gravity generated acceleration that is filtered from the raw acceleration"); }
    const char* GetSensorDataDescription() const override { return _HELP("Gravity generated acceleration measured in g-forces (1G ~= 9.8 m/s^2)"); }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class CFlowRotationRateRawNode
    : public CFlowMotionSensorNode
{
public:
    CFlowRotationRateRawNode(SActivationInfo* i)
        : CFlowMotionSensorNode(i, eMSF_RotationRateRaw) {}
    ~CFlowRotationRateRawNode() override {}
    IFlowNodePtr Clone(SActivationInfo* i) override { return new CFlowRotationRateRawNode(i); }
    void GetMemoryUsage(ICrySizer* sizer) const override { sizer->Add(*this); }
    const char* GetNodeDescription() const override { return _HELP("Outputs raw rotation rate as measured by the gyroscope"); }
    const char* GetSensorDataDescription() const override { return _HELP("Raw rotation rate measured in degrees per second (deg/s)"); }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class CFlowRotationRateUnbiased
    : public CFlowMotionSensorNode
{
public:
    CFlowRotationRateUnbiased(SActivationInfo* i)
        : CFlowMotionSensorNode(i, eMSF_RotationRateUnbiased) {}
    ~CFlowRotationRateUnbiased() override {}
    IFlowNodePtr Clone(SActivationInfo* i) override { return new CFlowRotationRateUnbiased(i); }
    void GetMemoryUsage(ICrySizer* sizer) const override { sizer->Add(*this); }
    const char* GetNodeDescription() const override { return _HELP("Outputs rotation rate that has been processed to remove gyroscope bias"); }
    const char* GetSensorDataDescription() const override { return _HELP("Unbiased rotation rate measured in degrees per second (deg/s)"); }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class CFlowMagneticFieldRawNode
    : public CFlowMotionSensorNode
{
public:
    CFlowMagneticFieldRawNode(SActivationInfo* i)
        : CFlowMotionSensorNode(i, eMSF_MagneticFieldRaw) {}
    ~CFlowMagneticFieldRawNode() override {}
    IFlowNodePtr Clone(SActivationInfo* i) override { return new CFlowMagneticFieldRawNode(i); }
    void GetMemoryUsage(ICrySizer* sizer) const override { sizer->Add(*this); }
    const char* GetNodeDescription() const override { return _HELP("Outputs raw magnetic field data as measured by the magnetometer, which includes device bias"); }
    const char* GetSensorDataDescription() const override { return _HELP("Raw magnetic field measured in microteslas (uT)"); }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class CFlowMagneticFieldUnbiasedNode
    : public CFlowMotionSensorNode
{
public:
    CFlowMagneticFieldUnbiasedNode(SActivationInfo* i)
        : CFlowMotionSensorNode(i, eMSF_MagneticFieldUnbiased) {}
    ~CFlowMagneticFieldUnbiasedNode() override {}
    IFlowNodePtr Clone(SActivationInfo* i) override { return new CFlowMagneticFieldUnbiasedNode(i); }
    void GetMemoryUsage(ICrySizer* sizer) const override { sizer->Add(*this); }
    const char* GetNodeDescription() const override { return _HELP("Outputs magnetic field that has been processed to remove device bias (Earth's magnetic field plus surrounding fields, may take some seconds to begin outputting values)"); }
    const char* GetSensorDataDescription() const override { return _HELP("Unbiased magnetic field measured in microteslas (uT)"); }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class CFlowMagneticNorthNode
    : public CFlowMotionSensorNode
{
public:
    CFlowMagneticNorthNode(SActivationInfo* i)
        : CFlowMotionSensorNode(i, eMSF_MagneticNorth) {}
    ~CFlowMagneticNorthNode() override {}
    IFlowNodePtr Clone(SActivationInfo* i) override { return new CFlowMagneticNorthNode(i); }
    void GetMemoryUsage(ICrySizer* sizer) const override { sizer->Add(*this); }
    const char* GetNodeDescription() const override { return _HELP("Outputs a vector pointing to magnetic north (may take some seconds while the device calibrates to begin outputting values)"); }
    const char* GetSensorDataDescription() const override { return _HELP("Scene world space vector pointing to the real world magnetic north"); }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class CFlowOrientationNode
    : public CFlowMotionSensorNode
{
public:
    CFlowOrientationNode(SActivationInfo* i)
        : CFlowMotionSensorNode(i, eMSF_Orientation) {}
    ~CFlowOrientationNode() override {}
    IFlowNodePtr Clone(SActivationInfo* i) override { return new CFlowOrientationNode(i); }
    void GetMemoryUsage(ICrySizer* sizer) const override { sizer->Add(*this); }
    const char* GetNodeDescription() const override { return _HELP("Outputs the orientation (or attitude) of the device with respect to an arbitrary but constant frame of reference"); }
    const char* GetSensorDataDescription() const override { return _HELP("Orientation (or attitude) measured in Euler angles (degrees)"); }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class CFlowOrientationDeltaNode
    : public CFlowMotionSensorNode
{
public:
    CFlowOrientationDeltaNode(SActivationInfo* i)
        : CFlowMotionSensorNode(i, eMSF_OrientationDelta) {}
    ~CFlowOrientationDeltaNode() override {}
    IFlowNodePtr Clone(SActivationInfo* i) override { return new CFlowOrientationDeltaNode(i); }
    void GetMemoryUsage(ICrySizer* sizer) const override { sizer->Add(*this); }
    const char* GetNodeDescription() const override { return _HELP("Outputs the change in orientation (or attitude) of the device since the last measurement"); }
    const char* GetSensorDataDescription() const override { return _HELP("Orientation (or attitude) delta measured in Euler angles (degrees)"); }
};

REGISTER_FLOW_NODE("Input:MotionSensor:AccelerationRaw", CFlowAccelerationRawNode);
REGISTER_FLOW_NODE("Input:MotionSensor:AccelerationUser", CFlowAccelerationUserNode);
REGISTER_FLOW_NODE("Input:MotionSensor:AccelerationGravity", CFlowAccelerationGravityNode);
REGISTER_FLOW_NODE("Input:MotionSensor:RotationRateRaw", CFlowRotationRateRawNode);
REGISTER_FLOW_NODE("Input:MotionSensor:RotationRateUnbiased", CFlowRotationRateUnbiased);
REGISTER_FLOW_NODE("Input:MotionSensor:MagneticFieldRaw", CFlowMagneticFieldRawNode);
REGISTER_FLOW_NODE("Input:MotionSensor:MagneticFieldUnbiased", CFlowMagneticFieldUnbiasedNode);
REGISTER_FLOW_NODE("Input:MotionSensor:MagneticNorth", CFlowMagneticNorthNode);
REGISTER_FLOW_NODE("Input:MotionSensor:Orientation", CFlowOrientationNode);
REGISTER_FLOW_NODE("Input:MotionSensor:OrientationDelta", CFlowOrientationDeltaNode);
