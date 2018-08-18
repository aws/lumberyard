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
#ifndef CRYINCLUDE_CRYACTION_FEATURETESTS_FEATURETESTS_H
#define CRYINCLUDE_CRYACTION_FEATURETESTS_FEATURETESTS_H
#pragma once

namespace FeatureTests
{
    struct IFeatureTest
    {
        enum class State
        {
            Created,
            Initialized,
            Ready,
            Running,
            Finished
        };
        struct TestResults
        {
            TestResults()
                : Succeeded(false){}
            TestResults(const TestResults& other)
            {
                Succeeded = other.Succeeded;
                ResultDescription = std::move(other.ResultDescription);
                AdditionalDetails = std::move(other.AdditionalDetails);
            }
            TestResults(TestResults&& other)
            {
                Succeeded = other.Succeeded;
                ResultDescription = std::move(other.ResultDescription);
                AdditionalDetails = std::move(other.AdditionalDetails);
            }
            ~TestResults(){};
            TestResults& operator=(TestResults&& other)
            {
                Succeeded = other.Succeeded;
                ResultDescription = std::move(other.ResultDescription);
                AdditionalDetails = std::move(other.AdditionalDetails);
                return *this;
            }

            bool Succeeded;
            std::string ResultDescription;
            std::string AdditionalDetails;
        };
        IFeatureTest(){}
        virtual ~IFeatureTest(){}

        virtual const char* GetName() const = 0;
        virtual const char* GetDescription() const = 0;
        virtual float GetMaxRunningTime() const = 0;
        virtual State GetState() const = 0;

        virtual void StartTest() = 0;
        virtual void Update(float deltaTime) = 0;
        virtual void UpdateCamera() = 0;
        virtual TestResults GetResults() const = 0;
    private:
        IFeatureTest(const IFeatureTest&);
        IFeatureTest& operator=(IFeatureTest&);
    };

    struct IFeatureTestsManager
    {
        IFeatureTestsManager(){}
        virtual ~IFeatureTestsManager(){}
    private:
        IFeatureTestsManager(const IFeatureTestsManager&);
        IFeatureTestsManager& operator=(IFeatureTestsManager&);
    };

    typedef AZStd::shared_ptr < IFeatureTestsManager > FeatureTestHandle;

    //When you destroy the returned handle, your test will be un-registered.
    //Its best to just store the handle in your feature test as a member variable, because
    //the handle must be destroyed before the passed in test is destroyed. We retain a ptr
    //to your test, but don't keep it alive.
    FeatureTestHandle RegisterFeatureTest(IFeatureTest* test);
    //Needs to be called once per frame. From your main game loop.
    void Update();
    //Call this late, so Feature tests can override any other camera control
    void UpdateCamera();
    IFeatureTest* GetCurrentlyRunningTest();
    const char* GetTestGroupName();
    bool ShouldCreateTimeStampedFolder();
    std::string CreateTimeStampedFileName(const char* originalFileName);

    void CreateManager();
    void DestroyManager();
}

#endif //CRYINCLUDE_CRYACTION_FEATURETESTS_FEATURETESTS_H
