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

// Description : This node includes different generic VR functionality


#include "CryLegacy_precompiled.h"

#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <HMDBus.h>
#include <AzCore/Component/TransformBus.h>
#include <IStereoRenderer.h>
#include <VRControllerBus.h>

class VRRecenterPose
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    VRRecenterPose(SActivationInfo* pActInfo)
    {
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inConfig[] =
        {
            InputPortConfig_Void("Activate", _HELP("Resets the tracking origin to the headset's current location, and sets the yaw origin to the current headset yaw value")),
            {0}
        };

        config.sDescription = _HELP("Recenter the HMD's pose");
        config.pInputPorts = inConfig;
        config.pOutputPorts = nullptr;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* actInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        {
            EBUS_EVENT(AZ::VR::HMDDeviceRequestBus, RecenterPose);
        }
        break;
        }
    }
};

class VREnabled
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum OUTPUTS
    {
        VREnabled_True,
        VREnabled_False
    };

public:
    VREnabled(SActivationInfo* actInfo)
    {
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inConfig[] =
        {
            InputPortConfig_Void("Activate", _HELP("Determine if VR rendering is enabled and properly setup")),
            { 0 }
        };

        static const SOutputPortConfig outConfig[] =
        {
            OutputPortConfig<bool>("True", _HELP("VR rendering is enabled and active")),
            OutputPortConfig<bool>("False", _HELP("VR rendering is disabled")),
            { 0 }
        };

        config.sDescription = _HELP("If true, VR rendering is enabled");
        config.pInputPorts = inConfig;
        config.pOutputPorts = outConfig;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* actInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        {
            if (gEnv->pRenderer->GetIStereoRenderer()->IsRenderingToHMD())
            {
                ActivateOutput(actInfo, VREnabled_True, true);
            }
            else
            {
                ActivateOutput(actInfo, VREnabled_False, false);
            }
        }
        break;
        }
    }
};

class VRSetTrackingLevel
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum INPUTS
    {
        TrackingLevel_Activate,
        TrackingLevel_Level
    };

public:
    VRSetTrackingLevel(SActivationInfo* actInfo)
    {
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inConfig[] =
        {
            InputPortConfig_Void("Activate", _HELP("Set the tracking level to the requested value")),
            InputPortConfig<int>("TrackingLevel", 0, _HELP("Level to set the tracking to - 0: Head, 1: Floor")),
            { 0 }
        };


        config.sDescription = _HELP("Set the tracking level");
        config.pInputPorts = inConfig;
        config.pOutputPorts = nullptr;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* actInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        {
            AZ::VR::HMDTrackingLevel level = static_cast<AZ::VR::HMDTrackingLevel>(GetPortInt(actInfo, TrackingLevel_Level));
            EBUS_EVENT(AZ::VR::HMDDeviceRequestBus, SetTrackingLevel, level);
        }
        break;
        }
    }
};

class VRTransformInfo
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum INPUTS
    {
        Enabled = 0,
    };

    enum OUTPUTS
    {
        CamPos,
        CamRot,
        HMDPos,
        HMDRot,
    };

public:
    VRTransformInfo(SActivationInfo* actInfo)
    {
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inConfig[] =
        {
            InputPortConfig<bool>("Enabled", true, _HELP("Enable / disable the node")),
            {0}
        };
        static const SOutputPortConfig outConfig[] =
        {
            OutputPortConfig<AZ::Vector3>("Camera pos", _HELP("The position of the current camera in world coordinates")),
            OutputPortConfig<AZ::Vector3>("Camera rot (PRY)", _HELP("The orientation of the current camera in world coordinates (Pitch,Roll,Yaw) in Degrees")),
            OutputPortConfig<AZ::Vector3>("HMD pos", _HELP("The position of the HMD with respect to the recentered pose of the tracker")),
            OutputPortConfig<AZ::Vector3>("HMD rot (PRY)", _HELP("The orientation of the HMD in world coordinates (Pitch,Roll,Yaw) in Degrees")),
            {0}
        };

        config.sDescription = _HELP("This node provides information about the orientation and position of the camera and HMD");
        config.pInputPorts = inConfig;
        config.pOutputPorts = outConfig;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* actInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            if (actInfo->pGraph != nullptr)
            {
                const bool enabled = GetPortBool(actInfo, Enabled);
                actInfo->pGraph->SetRegularlyUpdated(actInfo->myID, enabled);
            }
        }
        break;

        case eFE_Activate:
        {
            if (IsPortActive(actInfo, Enabled) && actInfo->pGraph != nullptr)
            {
                const bool enabled = GetPortBool(actInfo, Enabled);
                actInfo->pGraph->SetRegularlyUpdated(actInfo->myID, enabled);
            }
        }
        break;

        case eFE_Update:
        {
            // Camera info
            IRenderer* pRenderer = gEnv->pRenderer;
            if (pRenderer)
            {
                const CCamera& rCam = pRenderer->GetCamera();
                const Ang3 angles = RAD2DEG(rCam.GetAngles());

                ActivateOutput(actInfo, CamPos, LYVec3ToAZVec3(rCam.GetPosition()));
                ActivateOutput(actInfo, CamRot, AZ::Vector3(angles.y, angles.z, angles.x));     // camera angles are in YPR and we need PRY
            }

            // HMD info
            if (gEnv->pRenderer->GetIStereoRenderer()->IsRenderingToHMD())
            {
                const AZ::VR::TrackingState* sensorState = nullptr;
                EBUS_EVENT_RESULT(sensorState, AZ::VR::HMDDeviceRequestBus, GetTrackingState);

                if (sensorState && sensorState->CheckStatusFlags(AZ::VR::HMDStatus_IsUsable))
                {
                    const Ang3 hmdAngles(AZQuaternionToLYQuaternion(sensorState->pose.orientation));
                    ActivateOutput(actInfo, HMDRot, AZ::Vector3(RAD2DEG(hmdAngles.x), RAD2DEG(hmdAngles.y), RAD2DEG(hmdAngles.z)));
                    ActivateOutput(actInfo, HMDPos, sensorState->pose.position);
                }
            }
        }
        break;
        }
    }
};

// ------------------------------------------------------------------------------------------------------------
class VRControllerTracking
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum INPUTS
    {
        Enabled = 0,
        Scale
    };

    enum OUTPUTS
    {
        PosWS_L,
        RotWS_L,
        ValidData_L,
        PosWS_R,
        RotWS_R,
        ValidData_R,
    };

public:
    VRControllerTracking(SActivationInfo* actInfo)
    {
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inConfig[] =
        {
            InputPortConfig<bool>("Enabled", true, _HELP("Enable / disable the node")),
            InputPortConfig<float>("Scale", 1.f, _HELP("Scales the controller's movements")),
            {0}
        };
        static const SOutputPortConfig outConfig[] =
        {
            OutputPortConfig<Vec3>("Left Pos", _HELP("The position of the HMD left controller in world space")),
            OutputPortConfig<Vec3>("Left Rot (PRY)", _HELP("The orientation of the HMD left controller in world space (Pitch,Roll,Yaw) in Degrees")),
            OutputPortConfig<bool>("Left data ok", _HELP("The left HMD controller is connected and the data is valid")),
            OutputPortConfig<Vec3>("Right Pos", _HELP("The position of the HMD right controller in world space")),
            OutputPortConfig<Vec3>("Right Rot (PRY)", _HELP("The orientation of the HMD right controller in world space (Pitch,Roll,Yaw) in Degrees")),
            OutputPortConfig<bool>("Right data ok", _HELP("The right HMD controller is connected and the data is valid")),
            {0}
        };

        config.sDescription = _HELP("This node provides information about the orientation and position of the VR controller in world space based on the world transform of the selected entity");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inConfig;
        config.pOutputPorts = outConfig;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* actInfo) override
    {
        switch (event)
        {
            case eFE_Initialize:
            {
                if (actInfo->pGraph != nullptr)
                {
                    const bool enabled = GetPortBool(actInfo, Enabled);
                    actInfo->pGraph->SetRegularlyUpdated(actInfo->myID, enabled);
                }
            }
            break;

            case eFE_Activate:
            {
                if (IsPortActive(actInfo, Enabled) && actInfo->pGraph != nullptr)
                {
                    const bool enabled = GetPortBool(actInfo, Enabled);
                    actInfo->pGraph->SetRegularlyUpdated(actInfo->myID, enabled);
                }
            }
            break;

            case eFE_Update:
            {
                bool bHmdLeftControllerOk = false, bHmdRightControllerOk = false;

                if (gEnv->pRenderer->GetIStereoRenderer()->IsRenderingToHMD())
                { 
                    Matrix34 entityTM;
                    bool entityExists = false;
                    if (IsLegacyEntityId(actInfo->entityId))
                    {
                        // by default use the selected entity as reference
                        const IEntity* entity = actInfo->pEntity;
                        if (entity == nullptr)
                        {
                            // if no entity is passed to the node use the local player
                            if (const IActor* pActor = gEnv->pGame->GetIGameFramework()->GetClientActor())
                            {
                                entity = pActor->GetEntity();
                            }
                        }

                        if (entity != nullptr)
                        {
                            entityExists = true;
                            entityTM = entity->GetWorldTM();
                        }
                    }
                    else
                    {
                        // This is a component entity.
                        entityExists = true;
                        AZ::Transform transform;
                        EBUS_EVENT_ID_RESULT(transform, actInfo->entityId, AZ::TransformBus, GetWorldTM);
                        entityTM = AZTransformToLYTransform(transform);
                    }

                    if (entityExists)
                    {
                        const AZ::VR::TrackingState* sensorState = nullptr;
                        EBUS_EVENT_RESULT(sensorState, AZ::VR::HMDDeviceRequestBus, GetTrackingState);
                        if (sensorState && sensorState->CheckStatusFlags(AZ::VR::HMDStatus_IsUsable))
                        {
                            EBUS_EVENT_RESULT(bHmdLeftControllerOk, AZ::VR::ControllerRequestBus, IsConnected, AZ::VR::ControllerIndex::LeftHand);
                            EBUS_EVENT_RESULT(bHmdRightControllerOk, AZ::VR::ControllerRequestBus, IsConnected, AZ::VR::ControllerIndex::RightHand);

                            Matrix33 m;
                            entityTM.GetRotation33(m);
                            const Quat entityRotWS(m);

                            AZ::VR::TrackingState* leftCtrlState = nullptr;
                            EBUS_EVENT_RESULT(leftCtrlState, AZ::VR::ControllerRequestBus, GetTrackingState, AZ::VR::ControllerIndex::LeftHand);

                            if (leftCtrlState)
                            {
                                const Quat hmdLeftQ = entityRotWS * AZQuaternionToLYQuaternion(leftCtrlState->pose.orientation);
                                const Ang3 hmdLeftCtrlAngles(hmdLeftQ);
                                ActivateOutput(actInfo, RotWS_L, Vec3(RAD2DEG(hmdLeftCtrlAngles)));

                                const float scale = GetPortFloat(actInfo, Scale);

                                const Vec3 hmdPosL = entityTM.GetTranslation() + entityRotWS * (AZVec3ToLYVec3(leftCtrlState->pose.position) * scale);
                                ActivateOutput(actInfo, PosWS_L, hmdPosL);
                            }

                            AZ::VR::TrackingState* rightCtrlState = nullptr;
                            EBUS_EVENT_RESULT(rightCtrlState, AZ::VR::ControllerRequestBus, GetTrackingState, AZ::VR::ControllerIndex::RightHand);

                            if (rightCtrlState)
                            {
                                const Quat hmdRightQ = entityRotWS * AZQuaternionToLYQuaternion(rightCtrlState->pose.orientation);
                                const Ang3 hmdRightCtrlAngles(hmdRightQ);
                                ActivateOutput(actInfo, RotWS_R, Vec3(RAD2DEG(hmdRightCtrlAngles)));

                                const float scale = GetPortFloat(actInfo, Scale);

                                const Vec3 hmdPosR = entityTM.GetTranslation() + entityRotWS * (AZVec3ToLYVec3(rightCtrlState->pose.position) * scale);
                                ActivateOutput(actInfo, PosWS_R, hmdPosR);
                            }
                        }
                    }
                }

                ActivateOutput(actInfo, ValidData_L, bHmdLeftControllerOk);
                ActivateOutput(actInfo, ValidData_R, bHmdRightControllerOk);
            }
            break;
        }
    }
};

class HMDDynamics
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum INPUTS
    {
        Enabled = 0,
    };

    enum OUTPUTS
    {
        LinearVelocity,
        LinearAcceleration,
        AngularVelocity,
        AngularAcceleration,
    };

public:

    HMDDynamics(SActivationInfo* actInfo)
    {
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inConfig[] =
        {
            InputPortConfig<bool>("Enabled", true, _HELP("Enable / disable the node")),
            { 0 }
        };
        static const SOutputPortConfig outConfig[] =
        {
            OutputPortConfig<AZ::Vector3>("Linear Velocity", _HELP("Linear velocity of the HMD in local space")),
            OutputPortConfig<AZ::Vector3>("Linear Acceleration", _HELP("Linear acceleration of the HMD in local space")),
            OutputPortConfig<AZ::Vector3>("Angular Velocity", _HELP("Angular velocity of the HMD in local space")),
            OutputPortConfig<AZ::Vector3>("Angular Acceleration", _HELP("Angular acceleration of the HMD in local space")),
            { 0 }
        };

        config.sDescription = _HELP("This node provides information about the current angular and linear dynamics of the HMD");
        config.pInputPorts = inConfig;
        config.pOutputPorts = outConfig;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* actInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            if (actInfo->pGraph != nullptr)
            {
                const bool enabled = GetPortBool(actInfo, Enabled);
                actInfo->pGraph->SetRegularlyUpdated(actInfo->myID, enabled);
            }
        }
        break;

        case eFE_Activate:
        {
            if (IsPortActive(actInfo, Enabled) && actInfo->pGraph != nullptr)
            {
                const bool enabled = GetPortBool(actInfo, Enabled);
                actInfo->pGraph->SetRegularlyUpdated(actInfo->myID, enabled);
            }
        }
        break;

        case eFE_Update:
        {
            if (gEnv->pRenderer->GetIStereoRenderer()->IsRenderingToHMD())
            {
                const AZ::VR::TrackingState* state = nullptr;
                EBUS_EVENT_RESULT(state, AZ::VR::HMDDeviceRequestBus, GetTrackingState);
                if (state && state->CheckStatusFlags(AZ::VR::HMDStatus_IsUsable))
                {
                    const AZ::VR::DynamicsState* dynamics = &(state->dynamics);
                    ActivateOutput(actInfo, LinearVelocity, dynamics->linearVelocity);
                    ActivateOutput(actInfo, LinearAcceleration, dynamics->linearAcceleration);
                    ActivateOutput(actInfo, AngularVelocity, dynamics->angularVelocity);
                    ActivateOutput(actInfo, AngularAcceleration, dynamics->angularAcceleration);
                }
            }
        }
        break;
        }
    }
};

class VRControllerDynamics
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum INPUTS
    {
        Enabled = 0,
    };

    enum OUTPUTS
    {
        ControllerActive_Left,
        LinearVelocity_Left,
        LinearAcceleration_Left,
        AngularVelocity_Left,
        AngularAcceleration_Left,

        ControllerActive_Right,
        LinearVelocity_Right,
        LinearAcceleration_Right,
        AngularVelocity_Right,
        AngularAcceleration_Right,
    };

public:

    VRControllerDynamics(SActivationInfo* actInfo)
    {
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inConfig[] =
        {
            InputPortConfig<bool>("Enabled", true, _HELP("Enable / disable the node")),
            { 0 }
        };
        static const SOutputPortConfig outConfig[] =
        {
            OutputPortConfig<bool>("Left Controller Active", _HELP("The left controller is active and being tracked")),
            OutputPortConfig<AZ::Vector3>("Left Linear Velocity", _HELP("Linear velocity of the left controller in local space")),
            OutputPortConfig<AZ::Vector3>("Left Linear Acceleration", _HELP("Linear acceleration of the left controller in local space")),
            OutputPortConfig<AZ::Vector3>("Left Angular Velocity", _HELP("Angular velocity of the left controller in local space")),
            OutputPortConfig<AZ::Vector3>("Left Angular Acceleration", _HELP("Angular acceleration of the left controller in local space")),
            OutputPortConfig<bool>("Right Controller Active", _HELP("The right controller is active and being tracked")),
            OutputPortConfig<AZ::Vector3>("Right Linear Velocity", _HELP("Linear velocity of the right controller in local space")),
            OutputPortConfig<AZ::Vector3>("Right Linear Acceleration", _HELP("Linear acceleration of the right controller in local space")),
            OutputPortConfig<AZ::Vector3>("Right Angular Velocity", _HELP("Angular velocity of the right controller in local space")),
            OutputPortConfig<AZ::Vector3>("Right Angular Acceleration", _HELP("Angular acceleration of the right controller in local space")),
            { 0 }
        };

        config.sDescription = _HELP("This node provides information about the current angular and linear dynamics of the VR controllers");
        config.pInputPorts = inConfig;
        config.pOutputPorts = outConfig;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* actInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            if (actInfo->pGraph != nullptr)
            {
                const bool enabled = GetPortBool(actInfo, Enabled);
                actInfo->pGraph->SetRegularlyUpdated(actInfo->myID, enabled);
            }
        }
        break;

        case eFE_Activate:
        {
            if (IsPortActive(actInfo, Enabled) && actInfo->pGraph != nullptr)
            {
                const bool enabled = GetPortBool(actInfo, Enabled);
                actInfo->pGraph->SetRegularlyUpdated(actInfo->myID, enabled);
            }
        }
        break;

        case eFE_Update:
        {
            bool leftActive = false;
            bool rightActive = false;

            if (gEnv->pRenderer->GetIStereoRenderer()->IsRenderingToHMD())
            {
                const AZ::VR::TrackingState* state = nullptr;
                EBUS_EVENT_RESULT(state, AZ::VR::HMDDeviceRequestBus, GetTrackingState);
                if (state && state->CheckStatusFlags(AZ::VR::HMDStatus_IsUsable))
                {
                    EBUS_EVENT_RESULT(leftActive, AZ::VR::ControllerRequestBus, IsConnected, AZ::VR::ControllerIndex::LeftHand);
                    if (leftActive)
                    {
                        AZ::VR::TrackingState* leftTrackingState = nullptr;
                        EBUS_EVENT_RESULT(leftTrackingState, AZ::VR::ControllerRequestBus, GetTrackingState, AZ::VR::ControllerIndex::LeftHand);

                        const AZ::VR::DynamicsState* dynamics = &(leftTrackingState->dynamics);
                        ActivateOutput(actInfo, LinearVelocity_Left, dynamics->linearVelocity);
                        ActivateOutput(actInfo, LinearAcceleration_Left, dynamics->linearAcceleration);
                        ActivateOutput(actInfo, AngularVelocity_Left, dynamics->angularVelocity);
                        ActivateOutput(actInfo, AngularAcceleration_Left, dynamics->angularAcceleration);
                    }

                    EBUS_EVENT_RESULT(rightActive, AZ::VR::ControllerRequestBus, IsConnected, AZ::VR::ControllerIndex::RightHand);
                    if (rightActive)
                    {
                        AZ::VR::TrackingState* rightTrackingState = nullptr;
                        EBUS_EVENT_RESULT(rightTrackingState, AZ::VR::ControllerRequestBus, GetTrackingState, AZ::VR::ControllerIndex::RightHand);

                        const AZ::VR::DynamicsState* dynamics = &(rightTrackingState->dynamics);
                        ActivateOutput(actInfo, LinearVelocity_Right, dynamics->linearVelocity);
                        ActivateOutput(actInfo, LinearAcceleration_Right, dynamics->linearAcceleration);
                        ActivateOutput(actInfo, AngularVelocity_Right, dynamics->angularVelocity);
                        ActivateOutput(actInfo, AngularAcceleration_Right, dynamics->angularAcceleration);
                    }
                }
            }

            ActivateOutput(actInfo, ControllerActive_Left, leftActive);
            ActivateOutput(actInfo, ControllerActive_Right, rightActive);
        }
        break;
        }
    }
};

class VRDeviceInfo
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum OUTPUTS
    {
        HMDName,
        RenderWidth,
        RenderHeight,
        FOV_V,
        FOV_H
    };

public:
    VRDeviceInfo(SActivationInfo* actInfo)
    {
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inConfig[] =
        {
            InputPortConfig_Void("Activate", _HELP("Get info about the currently connected device")),
            { 0 }
        };

        static const SOutputPortConfig outConfig[] =
        {
            OutputPortConfig<string>("Name", _HELP("The name of the active HMD")),
            OutputPortConfig<uint32>("RenderWidth", _HELP("The render width of a single eye (in pixels)")),
            OutputPortConfig<uint32>("RenderHeight", _HELP("The render height of a single eye (in pixels)")),
            OutputPortConfig<float>("VerticalFOV", _HELP("The vertical FOV of the HMD (in degrees)")),
            OutputPortConfig<float>("HorizontalFOV", _HELP("The combined horizontal FOV of both eyes (in degrees)")),
            { 0 }
        };

        config.sDescription = _HELP("Get info about the currently connected device");
        config.pInputPorts = inConfig;
        config.pOutputPorts = outConfig;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* actInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        {
            if (gEnv->pRenderer->GetIStereoRenderer()->IsRenderingToHMD())
            {
                const AZ::VR::HMDDeviceInfo* info = nullptr;
                EBUS_EVENT_RESULT(info, AZ::VR::HMDDeviceRequestBus, GetDeviceInfo);
                if (info)
                {
                    ActivateOutput(actInfo, HMDName, string(info->productName));
                    ActivateOutput(actInfo, RenderWidth, info->renderWidth);
                    ActivateOutput(actInfo, RenderHeight, info->renderHeight);
                    ActivateOutput(actInfo, FOV_V, RAD2DEG(info->fovV));
                    ActivateOutput(actInfo, FOV_H, RAD2DEG(info->fovH));
                }
            }
        }
        break;
        }
    }
};

class VRPlayspace : public CFlowBaseNode<eNCT_Singleton>
{
    enum INPUTS
    {
        Activate = 0,
    };

    enum OUTPUTS
    {
        Corner0,
        Corner1,
        Corner2,
        Corner3,
        Center,
        Dimensions,
        Valid
    };

public:

    VRPlayspace(SActivationInfo* actInfo)
    {
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inConfig[] =
        {
            InputPortConfig<bool>("Activate", true, _HELP("Get information about the HMD's playspace")),
            { 0 }
        };
        static const SOutputPortConfig outConfig[] =
        {
            OutputPortConfig<AZ::Vector3>("Corner0", _HELP("The world-space position of Corner 0")),
            OutputPortConfig<AZ::Vector3>("Corner1", _HELP("The world-space position of Corner 1")),
            OutputPortConfig<AZ::Vector3>("Corner2", _HELP("The world-space position of Corner 2")),
            OutputPortConfig<AZ::Vector3>("Corner3", _HELP("The world-space position of Corner 3")),
            OutputPortConfig<AZ::Vector3>("Center", _HELP("The world-space center of the playspace. Note that the center is on the floor.")),
            OutputPortConfig<AZ::Vector3>("Dimensions", _HELP("The width (x) and height (y) of the playspace in meters.")),
            OutputPortConfig<bool>("IsValid", _HELP("If true, the playspace data is valid and configured correctly.")),
            { 0 }
        };

        config.sDescription = _HELP("This node provides information about HMD's playspace");
        config.pInputPorts = inConfig;
        config.pOutputPorts = outConfig;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* actInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        {
            const AZ::VR::Playspace* playspace = nullptr;
            EBUS_EVENT_RESULT(playspace, AZ::VR::HMDDeviceRequestBus, GetPlayspace);
            
            if (playspace == nullptr)
            {
                ActivateOutput(actInfo, Valid, false);
            }
            else
            {
                ActivateOutput(actInfo, Valid, playspace->isValid);

                if (playspace->isValid)
                {
                    IViewSystem* viewSystem = gEnv->pGame->GetIGameFramework()->GetIViewSystem();
                    IView* mainView = viewSystem->GetActiveView();
                    const SViewParams* viewParams = mainView->GetCurrentParams();

                    AZ::Quaternion rotation = LYQuaternionToAZQuaternion(viewParams->rotation);
                    AZ::Vector3 translation = LYVec3ToAZVec3(viewParams->position);
                    AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(rotation, translation);

                    const AZ::Vector3 corners[] =
                    {
                        transform * playspace->corners[0],
                        transform * playspace->corners[1],
                        transform * playspace->corners[2],
                        transform * playspace->corners[3]
                    };

                    ActivateOutput(actInfo, Corner0, corners[0]);
                    ActivateOutput(actInfo, Corner1, corners[1]);
                    ActivateOutput(actInfo, Corner2, corners[2]);
                    ActivateOutput(actInfo, Corner3, corners[3]);

                    // Camera is centered at the playspace's 0 position.
                    Vec3 center = AZVec3ToLYVec3((corners[2] + corners[0]) * 0.5f);
                    ActivateOutput(actInfo, Center, center);

                    Vec3 dimensions;
                    dimensions.x = fabsf(corners[2].GetX() - corners[0].GetX());
                    dimensions.y = fabsf(corners[2].GetY() - corners[0].GetY());
                    dimensions.z = 0.0f;
                    ActivateOutput(actInfo, Dimensions, dimensions);
                }
            }
        }
        break;
        }
    }
};

REGISTER_FLOW_NODE("VR:RecenterPose", VRRecenterPose);
REGISTER_FLOW_NODE("VR:VREnabled", VREnabled);
REGISTER_FLOW_NODE("VR:SetTrackingLevel", VRSetTrackingLevel);
REGISTER_FLOW_NODE("VR:TransformInfo", VRTransformInfo);
REGISTER_FLOW_NODE("VR:ControllerTracking", VRControllerTracking);
REGISTER_FLOW_NODE("VR:Dynamics:HMD", HMDDynamics);
REGISTER_FLOW_NODE("VR:Dynamics:Controllers", VRControllerDynamics);
REGISTER_FLOW_NODE("VR:DeviceInfo", VRDeviceInfo);
REGISTER_FLOW_NODE("VR:Playspace", VRPlayspace);
