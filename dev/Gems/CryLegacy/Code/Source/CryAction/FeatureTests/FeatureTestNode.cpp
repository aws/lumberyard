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
#include <IFlowSystem.h>
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <AzCore/std/string/string.h>
#include "FeatureTests.h"

class FeatureTestNode
    : public CFlowBaseNode<eNCT_Instanced>
    , public FeatureTests::IFeatureTest
    , private ILogCallback
{
public:
    FeatureTestNode(SActivationInfo* actInfo)
        : m_state(State::Created)
        , m_triggerStartNextFrame(false)
        , m_featureTestHandle(RegisterFeatureTest(this)) {}
    virtual ~FeatureTestNode(){}

private:
    FeatureTestNode(const FeatureTestNode&);
    FeatureTestNode& operator=(const FeatureTestNode&);

    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new FeatureTestNode(pActInfo);
    }

    enum InputPorts
    {
        Name,
        Description,
        MaxTime,
        Camera,
        Ready,
        Succeeded,
        Failed,
    };

    enum OutputPorts
    {
        Start,
    };

    //ILogCallback implementation
    virtual void OnWrite(const char* sText, IMiniLog::ELogType type) override;
    virtual void OnWriteToConsole(const char* sText, bool bNewLine) override {}
    virtual void OnWriteToFile(const char* sText, bool bNewLine) override {}

    virtual void GetMemoryUsage(ICrySizer* sizer) const override;
    virtual void GetConfiguration(SFlowNodeConfig& config) override;
    virtual void ProcessEvent(EFlowEvent fevent, SActivationInfo* actInfo) override;
    virtual const char* GetName() const override;
    virtual const char* GetDescription() const override;
    virtual float GetMaxRunningTime() const override;
    virtual State GetState() const override;

    virtual void StartTest() override;
    virtual void Update(float deltaTime) override;
    virtual void UpdateCamera() override;
    virtual TestResults GetResults() const override;

    FeatureTests::FeatureTestHandle m_featureTestHandle;
    SActivationInfo m_actInfo;
    State m_state;
    TestResults m_testResults;
    bool m_triggerStartNextFrame;
};

const char* FeatureTestNode::GetName() const
{
    static auto unknown = "Unknown";
    if (m_actInfo.pInputPorts)
    {
        return GetPortString(const_cast<SActivationInfo*>(&m_actInfo), InputPorts::Name).c_str(); //GetPortString isn't const correct
    }
    else
    {
        return unknown;
    }
}

const char* FeatureTestNode::GetDescription() const
{
    static auto noDescription = "";
    if (m_actInfo.pInputPorts)
    {
        return GetPortString(const_cast<SActivationInfo*>(&m_actInfo), InputPorts::Description).c_str(); //GetPortString isn't const correct
    }
    else
    {
        return noDescription;
    }
}

float FeatureTestNode::GetMaxRunningTime() const
{
    if (m_actInfo.pInputPorts)
    {
        return GetPortFloat(const_cast<SActivationInfo*>(&m_actInfo), InputPorts::MaxTime); //GetPortFloat isn't const correct
    }
    else
    {
        return std::numeric_limits<float>::max();
    }
}

FeatureTestNode::State FeatureTestNode::GetState() const
{
    return m_state;
}

void FeatureTestNode::StartTest()
{
    CRY_ASSERT(m_state == State::Ready);
    m_state = State::Running;
    m_triggerStartNextFrame = true;
}

void FeatureTestNode::Update(float deltaTime)
{
    //Introduce a one frame delay between starting a feature test, and triggering the start output.
    //Ensures that the camera has been moved by the time any later flow graph nodes run.
    if (m_triggerStartNextFrame)
    {
        CRY_ASSERT(m_state == State::Running);
        m_triggerStartNextFrame = false;
        ActivateOutput(&m_actInfo, OutputPorts::Start, true);
    }
}

void FeatureTestNode::UpdateCamera()
{
    if (m_state == State::Running)
    {
        auto id = FlowEntityId(GetPortEntityId(&m_actInfo, InputPorts::Camera));
        if (id)
        {
            auto entity = gEnv->pEntitySystem->GetEntity(id);
            //If the user specifies an actual camera use the camera parameters
            AZStd::string name(entity->GetClass()->GetName());
            int is_camera = name.compare("CameraSource");
            CCamera& viewCamera = gEnv->pSystem->GetViewCamera();
            if (is_camera == 0)
            {
                Vec3 position(0, 0, 0);
                Ang3 direction(0, 0, 0);
                position = entity->GetPos();
                direction = entity->GetWorldAngles();
                viewCamera.SetAngles(direction);
                viewCamera.SetPosition(position);
            }
            //Otherwise use the bounding box and some sane offsets to create a camera view.
            else
            {
                Vec3 position(0, 0, 0);
                Vec3 direction(0, 0, 0);
                AABB bounds;
                entity->GetWorldBounds(bounds);
                Vec3 cameraOffset(0.0f, -(2.0f + bounds.GetRadius()), bounds.GetRadius());
                const Vec3 lookAt = bounds.GetCenter();
                position = lookAt + (entity->GetForwardDir().GetNormalizedSafe() * cameraOffset.y) + (Vec3_OneZ * cameraOffset.z);
                direction = (lookAt - position).GetNormalizedSafe();
                const float roll(0.0f);
                viewCamera.SetMatrix(CCamera::CreateOrientationYPR(CCamera::CreateAnglesYPR(direction, roll)));
                viewCamera.SetPosition(position);
            }
        }
    }
}

FeatureTestNode::TestResults FeatureTestNode::GetResults() const
{
    return m_testResults;
}

void FeatureTestNode::GetMemoryUsage(ICrySizer* sizer) const
{
    sizer->Add(*this);
}

void FeatureTestNode::GetConfiguration(SFlowNodeConfig& config)
{
    static const SInputPortConfig inputs[] = {
        InputPortConfig<string>("Name", _HELP("Name of the feature test. Should be short, unique and descriptive. Appended with entity name if test is sequential.")),
        InputPortConfig<string>("Description", _HELP("Description of the feature test. What feature is being tested and how it's being tested.")),
        InputPortConfig<float>("MaxTime", 30.0f, _HELP("How long (in game time) is the test allowed to run before it fails (seconds).")),
        InputPortConfig<FlowEntityId>("Camera", _HELP("Optional entity used to act as the camera for the test. Does not have to be a real camera.")),
        InputPortConfig<bool>("Ready", true, _HELP("Boolean to indicate if all dependencies have been met and this test is ready to run. Mostly used to ensure tests are run in a"
                "specific order. There must always be at least one test ready to run in your flow graph(unless they are all finished of course), or the feature tests will be aborted.")),
        InputPortConfig_Void("Succeeded", _HELP("Trigger to indicated the feature test has passed. Cleanup will then be triggered.")),
        InputPortConfig_AnyType("Failed", _HELP("Trigger to indicated the feature test has failed. Optionally a string may be provided that will be used as the reason for failure")),
        {0}
    };

    static const SOutputPortConfig outputs[] = {
        OutputPortConfig_Void("Start", _HELP("Trigger to start running the feature test.")),
        {0}
    };

    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("Feature Test node that controls automated feature tests");
    config.SetCategory(EFLN_DEBUG);
}

void FeatureTestNode::ProcessEvent(EFlowEvent fevent, SActivationInfo* actInfo)
{
    switch (fevent)
    {
    case eFE_Initialize:
    {
        // Called on level load/reset
        m_state = State::Initialized;
        m_actInfo = *actInfo;

        ActivateOutput(actInfo, OutputPorts::Start, false);
    }
    break;

    case eFE_Activate:
    {
        if (IsPortActive(actInfo, InputPorts::Ready) && m_state == State::Initialized)
        {
            bool readyInput = GetPortBool(actInfo, InputPorts::Ready);
            if (readyInput)
            {
                m_state = State::Ready;
            }
        }
        else if (IsPortActive(actInfo, InputPorts::Succeeded))
        {
            if (m_state == State::Running)
            {
                m_state = State::Finished;
                m_testResults.Succeeded = true;
                m_testResults.ResultDescription = "Feature Test Succeeded!";
                gEnv->pLog->RemoveCallback(this);
            }
        }
        else if (IsPortActive(actInfo, InputPorts::Failed))
        {
            if (m_state == State::Running)
            {
                m_state = State::Finished;
                auto port = GetPortAny(actInfo, InputPorts::Failed);
                m_testResults.Succeeded = false;
                if (port.GetType() == EFlowDataTypes::eFDT_String)
                {
                    m_testResults.ResultDescription = GetPortString(actInfo, InputPorts::Failed).c_str();
                }
                else
                {
                    m_testResults.ResultDescription = "Feature Test Failed.";
                }
                gEnv->pLog->RemoveCallback(this);
            }
        }
    }
    break;
    }
}

void FeatureTestNode::OnWrite(const char* sText, IMiniLog::ELogType type)
{
    if ((m_state == State::Running) && (type == IMiniLog::ELogType::eError || type == IMiniLog::ELogType::eErrorAlways || type == IMiniLog::ELogType::eWarning ||
                                        type == IMiniLog::ELogType::eWarningAlways))
    {
        m_testResults.AdditionalDetails += sText;
        m_testResults.AdditionalDetails += "\n";
    }
}

REGISTER_FLOW_NODE("FeatureTest:FeatureTest", FeatureTestNode);
