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
#include <IImageHandler.h>
#include "ScopeGuard.h"
#include "FeatureTests.h"
#include <AzCore/IO/SystemFile.h>

/** A feature test node for taking screenshots and comparing them against a golden image. For simple cases the succeeded and failed outputs
could be just tied directly to the succeeded and failed inputs of a feature test node. For a more complex test this node can be triggered more
than once. Each time it is triggered it will generate a seperate screenshot with an incrementing number in the file name. i.e.
testLevel_test1_0.tif then testLevel_test1_1.tif. For this case you will need to route your flowgraph accordingly to make sure each one passes
before succeeding the feature test node. For comparison to the golden image a PSNR is specified which allows for a fuzzy compare.
*/
class ScreenshotCompareNode
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    ScreenshotCompareNode(SActivationInfo* actInfo)
        : c_screenshotNameTemplate("@LOG@/FeatureTestScreenShots/%s/%s/%s_%d.tif")
        , c_diffImageNameTemplate("@LOG@/FeatureTestScreenShots/%s/%s/%s_%d_diff.tif")
        , c_goldenImageNameTemplate("@ASSETS@/FeatureTests/GoldenImages/%s/%s/%s_%d.tif")
        , m_currentShotNumber(0)
    {}

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new ScreenshotCompareNode(pActInfo);
    }

private:
    //parameters are meant to be level name, ft_outputPathModifier (can be null), test name, screenshot number
    const char* c_screenshotNameTemplate;
    const char* c_diffImageNameTemplate;
    const char* c_goldenImageNameTemplate;
    enum InputNodes
    {
        TRIGGER,
        RESET,
        PSNR
    };
    enum OutputNodes
    {
        SUCCEEDED,
        FAILED
    };

    //IFlowNode implementation
    virtual void GetConfiguration(SFlowNodeConfig& config) override;
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override;
    virtual void GetMemoryUsage(ICrySizer* s) const override;

    int m_currentShotNumber;
};

void ScreenshotCompareNode::GetConfiguration(SFlowNodeConfig& config)
{
    static const SInputPortConfig inputs[] = {
        InputPortConfig_AnyType("Trigger", _HELP("When Triggered, will take a screenshot"), _HELP("Trigger")),
        InputPortConfig_AnyType("Reset", _HELP("Will reset current screenshot number back to 0."), _HELP("Reset")),
        InputPortConfig<float>("PSNR", 56.0f, _HELP("Picture signal to noise ratio used during comparison with golden image to determine success/failure."), _HELP("PSNR")),
        {0}
    };

    static const SOutputPortConfig outputs[] = {
        OutputPortConfig_Void("Succeeded", _HELP("Will trigger after each taken screenshot if the captured image matches the golden image"), _HELP("Succeeded")),
        OutputPortConfig_AnyType("Failed", _HELP("Will trigger after each taken screenshot if the captured image does not match the golden image. Will also send a string on the port "
                "with the reason for the failure(useful when connecting directly to Failed input of a Feature Test Node"), _HELP("Failed")),
        {0}
    };

    config.sDescription = _HELP("When triggered, this node will take a screenshot and compare it against a golden image. Can be triggered multiple times. Each"
            "screenshot file name will end with the current screenshot number starting at 0 and incrementing for each shot taken. Trigger reset to start back at 0.");
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.SetCategory(EFLN_DEBUG); //meant for use with feature tests
}

void ScreenshotCompareNode::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
{
    switch (event)
    {
    case eFE_Initialize:
        pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
        break;
    case eFE_Activate:
        if (IsPortActive(pActInfo, InputNodes::TRIGGER))
        {
            auto levelName = gEnv->pGame->GetIGameFramework()->GetLevelName();
            levelName = PathUtil::GetFile(levelName);

            auto testName = std::string("Unknown");
            auto currentTest = FeatureTests::GetCurrentlyRunningTest();
            if (currentTest)
            {
                testName = currentTest->GetName();
            }
            CryFixedStringT<CRYFILE_MAX_PATH> screenshotFileName, goldenImageFileName, diffImageFileName;

            screenshotFileName.Format(c_screenshotNameTemplate, levelName, FeatureTests::GetTestGroupName(), testName.c_str(), m_currentShotNumber);
            goldenImageFileName.Format(c_goldenImageNameTemplate, levelName, FeatureTests::GetTestGroupName(), testName.c_str(), m_currentShotNumber);
            diffImageFileName.Format(c_diffImageNameTemplate, levelName, FeatureTests::GetTestGroupName(), testName.c_str(), m_currentShotNumber);
            ++m_currentShotNumber;

            // We currently use TIFFOpen for libtiff, which requires absolute pathes.
            // https://issues.labcollab.net/browse/LMBR-5860: The libtiff implementation needs to change to use the TIFFClientOpen function instead so it can go through the new asset pipeline.
            char resolveBufferScreenshot[CRYFILE_MAX_PATH], resolveBufferGolden[CRYFILE_MAX_PATH], resolveBufferDiff[CRYFILE_MAX_PATH];
            gEnv->pFileIO->ResolvePath(screenshotFileName.c_str(), resolveBufferScreenshot, CRYFILE_MAX_PATH);
            gEnv->pFileIO->ResolvePath(goldenImageFileName.c_str(), resolveBufferGolden, CRYFILE_MAX_PATH);
            gEnv->pFileIO->ResolvePath(diffImageFileName.c_str(), resolveBufferDiff, CRYFILE_MAX_PATH);

            CryFixedStringT<256> failureString;
            auto onExit = std17::scope_guard([&]()
                    {
                        if (failureString.empty())
                        {
                            ActivateOutput(pActInfo, OutputNodes::SUCCEEDED, true);
                        }
                        else
                        {
                            ActivateOutput(pActInfo, OutputNodes::FAILED, string(failureString));
                        }
                    });
            if (!gEnv->pRenderer->ScreenShot(resolveBufferScreenshot))
            {
                failureString.Format("The renderer failed to take a screenshot");
                return;
            }

            auto imageHandler = gEnv->pSystem->GetImageHandler();
            CRY_ASSERT(imageHandler);

            auto testImage = imageHandler->LoadImage(resolveBufferScreenshot);
            if (!testImage)
            {
                failureString.Format("Failed to load the test screenshot %s", resolveBufferScreenshot);
                return;
            }
            auto goldenImage = imageHandler->LoadImage(resolveBufferGolden);
            if (!goldenImage)
            {
                failureString.Format("Failed to load the golden image %s", resolveBufferGolden);
                return;
            }

            auto diffImage = imageHandler->CreateDiffImage(testImage.get(), goldenImage.get());
            if (!diffImage)
            {
                failureString.Format("Failed to create difference image");
                return;
            }

            if (!imageHandler->SaveImage(diffImage.get(), resolveBufferDiff))
            {
                failureString.Format("Failed to save difference image");
                return;
            }

            {
                auto psnr = imageHandler->CalculatePSNR(diffImage.get());
                CryFixedStringT<256> psnrString;
                if (psnr == std::numeric_limits<float>::max())
                {
                    psnrString.Format("There was no difference between test screenshot and golden image. PSNR is infinite.");
                }
                else
                {
                    psnrString.Format("PSNR between test screenshot and golden image was %0.1f.", psnr);
                }

                if (psnr < GetPortFloat(pActInfo, InputNodes::PSNR))
                {
                    failureString.Format("%s Expected PSNR greater than %.1f", psnrString.c_str(), GetPortFloat(pActInfo, InputNodes::PSNR));
                }
                else
                {
                    CryLogAlways(psnrString.c_str());
                }
            }

            //optionaly make a copy to the timestamped folder
            if (FeatureTests::ShouldCreateTimeStampedFolder())
            {
                auto fullPath = FeatureTests::CreateTimeStampedFileName(screenshotFileName.c_str());
                char fullSrcPath[ICryPak::g_nMaxPath];
                gEnv->pCryPak->AdjustFileName(screenshotFileName.c_str(), fullSrcPath, AZ_ARRAY_SIZE(fullSrcPath), ICryPak::FLAGS_FOR_WRITING);
                gEnv->pCryPak->CopyFileOnDisk(fullSrcPath, fullPath.c_str(), false);
            }
        }
        else if (IsPortActive(pActInfo, InputNodes::RESET))
        {
            m_currentShotNumber = 0;
        }
        break;
    }
}

void ScreenshotCompareNode::GetMemoryUsage(ICrySizer* s) const
{
    s->Add(*this);
}

REGISTER_FLOW_NODE("FeatureTest:ScreenshotCompare", ScreenshotCompareNode);
