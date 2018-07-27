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
#include "StdAfx.h"
#include <IFlowSystem.h>
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <IImageHandler.h>
#include "ScopeGuard.h"
#include "FeatureTests.h"
#include <AzCore/IO/SystemFile.h>
#include "CryActionTraits.h"

/** A feature test node for taking screenshots base on the camera view in flow graph.
*/
class ScreenshotNode
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    ScreenshotNode(SActivationInfo* actInfo)
        : c_screenshotNameTemplate("@LOG@/ScreenShots/%s/%s/%s.%s")
        , m_currentShotNumber(0)
    {}

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new ScreenshotNode(pActInfo);
    }

private:
    //parameters are meant to be name of screenshot.
    const char* c_screenshotNameTemplate;
    enum InputPorts
    {
        TRIGGER,
        Name
    };
    enum OutputPorts
    {
        SUCCEEDED,
        FAILED
    };
    
    //IFlowNode implementation
    virtual void GetConfiguration(SFlowNodeConfig& config) override;
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override;
    virtual void GetMemoryUsage(ICrySizer* s) const override;

    SActivationInfo m_actInfo;
    int m_currentShotNumber;
};

void ScreenshotNode::GetConfiguration(SFlowNodeConfig& config)
{
    static const SInputPortConfig inputs[] = {
        InputPortConfig_AnyType("Trigger", _HELP("When Triggered, will take a screenshot")),
        InputPortConfig<string>("Name", _HELP("Name of the screenshot. Should be short, unique and descriptive.")),
        { 0 }
    };

    static const SOutputPortConfig outputs[] = {
        OutputPortConfig_AnyType("Succeeded", _HELP("Succeeded take a screenshot")),
        OutputPortConfig_AnyType("Failed", _HELP("Fail to take a screeenshot")),
        { 0 }
    };

    config.sDescription = _HELP("When Triggered,this node will take a screenshot base on current camera view");
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.SetCategory(EFLN_DEBUG); //meant for use with feature tests
}

void ScreenshotNode::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
{
    switch (event)
    {
    case eFE_Initialize:
        pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
        m_actInfo = *pActInfo;
        break;
    case eFE_Activate:
        if (IsPortActive(pActInfo, InputPorts::TRIGGER))
        {
            auto levelName = gEnv->pGame->GetIGameFramework()->GetLevelName();
            levelName = PathUtil::GetFile(levelName);

            auto testName = std::string("Unknown");

            if (m_actInfo.pInputPorts)
            {
                testName = GetPortString(const_cast<SActivationInfo*>(&m_actInfo), InputPorts::Name).c_str(); //GetPortString isn't const correct 
            }

            CryFixedStringT<CRYFILE_MAX_PATH> screenshotFileName;

            screenshotFileName.Format(c_screenshotNameTemplate, levelName, FeatureTests::GetTestGroupName(), testName.c_str(), AZ_LEGACY_CRYACTION_TRAIT_SCREENSHOT_EXTENSION);

            CryFixedStringT<256> failureString;

            if (!gEnv->pRenderer->ScreenShot(screenshotFileName))
            {
                failureString.Format("The renderer failed to take a screenshot");
                ActivateOutput(pActInfo, OutputPorts::FAILED, string(failureString));
                return;
            }
            else
            {
                ActivateOutput(pActInfo, OutputPorts::SUCCEEDED, true);
            }
        }
        break;
    }
}
void ScreenshotNode::GetMemoryUsage(ICrySizer* s) const
{
    s->Add(*this);
}

REGISTER_FLOW_NODE("FeatureTest:Screenshot", ScreenshotNode);
