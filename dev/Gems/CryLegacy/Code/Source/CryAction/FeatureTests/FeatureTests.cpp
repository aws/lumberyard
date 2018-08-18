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
#include "FeatureTests.h"
#include "CryActionCVars.h"
#include "Algorithm.h"

namespace FeatureTests
{
    class FeatureTestsManager& GetManagerInstance();

    class FeatureTestsManager
        : public IFeatureTestsManager
    {
    public:
        FeatureTestHandle RegisterFeatureTest(IFeatureTest* test);
        void UnRegisterFeatureTest(IFeatureTest* test);
        void Update();
        void UpdateCamera();
        IFeatureTest* GetCurrentlyRunningTest();
        std::string CreateTimeStampedFileName(const char* originalFileName);
        const char* GetTestGroupName() const { return m_testGroupName.c_str(); }
        bool ShouldCreateTimeStampedFolder() const { return m_createTimestampedFolder; }

        const static float c_fixedTimeStep;

        enum State
        {
            Disabled,
            StartNextTest,
            WaitingForReady,
            Running,
            Finished,
            DontRun, //Tests have not been requested to run.
            NumStates
        };

        struct TestResults
        {
            TestResults(){}
            TestResults(TestResults&& other)
            {
                m_timeToRun = other.m_timeToRun;
                m_testResults = std::move(other.m_testResults);
            }
            TestResults& operator=(TestResults&& other)
            {
                m_timeToRun = other.m_timeToRun;
                m_testResults = std::move(other.m_testResults);
                return *this;
            }

            float m_timeToRun;
            IFeatureTest::TestResults m_testResults;
        };
        FeatureTestsManager();
        virtual ~FeatureTestsManager(){}

        void StateDisabled();
        void StateStartNextTest();
        void StateWaitingForReady();
        void StateRunning();
        void StateFinished();

        void CreateJUnitResultsFile();
        bool SaveFiles(const XmlNodeRef& xmlToSave);

        bool ReadTestConfiguration(const string& featureTest);

        typedef std::unordered_map<IFeatureTest*, TestResults> FeatureTestType;
        FeatureTestType m_featureTests;
        State m_currentState;
        IFeatureTest* m_currentTest;
        AZStd::array<AZStd::function<void()>, State::NumStates> m_update;

        float m_currentTestRunningTime;
        int64 m_timeStart;
        char m_timeStamp[64];
        char m_timestampedFolder[ICryPak::g_nMaxPath];
        float m_timeWaiting;

        string m_mapName;
        bool m_createTimestampedFolder;
        float m_maxInitTime;
        string m_testGroupName;

        FeatureTestsManager(const FeatureTestsManager&) = delete;
        FeatureTestsManager& operator=(FeatureTestsManager&) = delete;
    };

    const float FeatureTestsManager::c_fixedTimeStep = 1.0f / 60.0f;

    FeatureTestsManager::FeatureTestsManager()
        : m_currentState(Disabled)
        , m_createTimestampedFolder(false)
        , m_currentTest(nullptr)
        , m_currentTestRunningTime(0)
        , m_timeStart(0)
        , m_timeWaiting(0)
        , m_maxInitTime(1.0f)
    {
        m_update[State::Disabled] = [&]() { StateDisabled(); };
        m_update[State::StartNextTest] = [&]() { StateStartNextTest(); };
        m_update[State::WaitingForReady] = [&]() { StateWaitingForReady(); };
        m_update[State::Running] = [&]() { StateRunning(); };
        m_update[State::Finished] = [&]() { StateFinished(); };
        m_update[State::DontRun] = []() { };
    }

    FeatureTestHandle FeatureTestsManager::RegisterFeatureTest(IFeatureTest* test)
    {
        m_featureTests.insert(std::make_pair(test, TestResults()));
        return FeatureTestHandle(this, [test, this](FeatureTestsManager*) { this->UnRegisterFeatureTest(test); });
    }

    void FeatureTestsManager::UnRegisterFeatureTest(IFeatureTest* test)
    {
        m_featureTests.erase(test);
    }

    void FeatureTestsManager::Update()
    {
        m_update[m_currentState]();
    }

    void FeatureTestsManager::UpdateCamera()
    {
        if (m_currentState == State::Running && m_currentTest)
        {
            m_currentTest->UpdateCamera();
        }
    }

    IFeatureTest* FeatureTestsManager::GetCurrentlyRunningTest()
    {
        return m_currentTest;
    }

    std::string FeatureTestsManager::CreateTimeStampedFileName(const char* originalFileName)
    {
        std::string result = m_timestampedFolder;
        auto filename = PathUtil::GetFileName(string(originalFileName));
        result += "/";
        result += filename;
        result += ".";
        result += PathUtil::GetExt(originalFileName);
        return result;
    }

    void FeatureTestsManager::StateDisabled()
    {
        CRY_ASSERT(m_currentState == State::Disabled);
        CRY_ASSERT(CCryActionCVars::Get().ft_runFeatureTest);
        string testToRun = CCryActionCVars::Get().ft_runFeatureTest->GetString();
        if (testToRun.empty()) //a test run has not been requested
        {
            m_currentState = State::DontRun;
            return;
        }

        if (!ReadTestConfiguration(testToRun))
        {
            m_currentState = State::DontRun;
            return;
        }

        gEnv->pTimer->EnableFixedTimeMode(true, c_fixedTimeStep);

        //Unfortunately this doesn't return an error if it fails. If it does fail to find/load the map, there
        //won't be any feature tests registered and it shouldn't cause any troubles.
        gEnv->pConsole->ExecuteString(string().Format("map %s", m_mapName.c_str()));

        IActor* actor = gEnv->pGame->GetIGameFramework()->GetClientActor();
        if (actor)
        {
            actor->GetEntity()->Hide(true);
        }
        //Disable all input
        gEnv->pInput->EnableEventPosting(false);

        m_timeStart = CryGetTicks();

        //JUnit wants timestamps in ISO 8601 format. ex "2011-10-08T07:07:09Z"
        time_t now;
        time(&now);
#if defined (AZ_COMPILER_MSVC)
        tm gmTime;
        gmtime_s(&gmTime, &now);
        strftime(m_timeStamp, sizeof m_timeStamp, "%Y-%m-%dT%H:%M:%SZ", &gmTime);
#else
        auto gmTime = gmtime(&now);
        strftime(m_timeStamp, sizeof m_timeStamp, "%Y-%m-%dT%H:%M:%SZ", gmTime);
#endif

        char timestampedFolderTime[ICryPak::g_nMaxPath];
#if defined (AZ_COMPILER_MSVC)
        tm lTime;
        localtime_s(&lTime, &now);
        strftime(timestampedFolderTime, sizeof(timestampedFolderTime), "%Y-%m-%d/%I-%M%p", &lTime);
#else
        auto lTime = localtime(&now);
        strftime(timestampedFolderTime, sizeof(timestampedFolderTime), "%Y-%m-%d/%I-%M%p", lTime);
#endif
        string timestampedFolderRoot;
        timestampedFolderRoot.Format("@LOG@/Reports/%s/%s/%s", m_mapName.c_str(),
            m_testGroupName.c_str(), timestampedFolderTime);
        gEnv->pCryPak->AdjustFileName(timestampedFolderRoot, m_timestampedFolder, AZ_ARRAY_SIZE(m_timestampedFolder), ICryPak::FLAGS_FOR_WRITING);
        if (ShouldCreateTimeStampedFolder())
        {
            gEnv->pCryPak->MakeDir(m_timestampedFolder);
        }

        m_currentState = State::StartNextTest;
    }

    void FeatureTestsManager::StateStartNextTest()
    {
        CRY_ASSERT(m_currentState == State::StartNextTest);
        if (gEnv->pSystem->GetSystemGlobalState() != ESYSTEM_GLOBAL_STATE_RUNNING)
        {
            return;
        }
        auto nextTest = std17::find_if(m_featureTests, [](const FeatureTestType::value_type& test) { return test.first->GetState() == IFeatureTest::State::Ready; });
        if (nextTest == end(m_featureTests))
        {
            if (std17::any_of(m_featureTests, [](const FeatureTestType::value_type& test) { return test.first->GetState() == IFeatureTest::State::Initialized; }))
            {
                m_timeWaiting = 0;
                m_currentState = State::WaitingForReady;
            }
            else
            {
                m_currentState = State::Finished;
            }
        }
        else
        {
            m_currentTest = nextTest->first;
            m_currentTest->StartTest();
            m_currentTestRunningTime = 0;
            m_currentState = State::Running;
            gEnv->p3DEngine->ResetPostEffects();
            CryLogAlways("=============================== Starting Feature Test %s ===============================", m_currentTest->GetName());
        }
    }

    void FeatureTestsManager::StateWaitingForReady()
    {
        CRY_ASSERT(m_currentState == State::WaitingForReady);
        if (std17::any_of(m_featureTests, [](const FeatureTestType::value_type& test) { return test.first->GetState() == IFeatureTest::State::Ready; }))
        {
            m_currentState = State::StartNextTest;
            return;
        }
        m_timeWaiting += gEnv->pTimer->GetFrameTime();
        if (m_timeWaiting > m_maxInitTime)
        {
            for (auto& test : m_featureTests)
            {
                if (test.first->GetState() == IFeatureTest::State::Initialized)
                {
                    test.second.m_testResults.Succeeded = false;
                    test.second.m_testResults.ResultDescription = "Test was never run. Test never entered the Ready state.";
                }
            }
            m_currentState = State::Finished;
        }
    }

    void FeatureTestsManager::StateRunning()
    {
        CRY_ASSERT(m_currentTest);
        CRY_ASSERT(m_currentState == State::Running);

        auto frameTime = gEnv->pTimer->GetFrameTime();
        m_currentTestRunningTime += frameTime;
        m_currentTest->Update(frameTime);
        if (m_currentTest->GetState() == IFeatureTest::State::Finished)
        {
            m_featureTests[m_currentTest].m_timeToRun = m_currentTestRunningTime;
            m_featureTests[m_currentTest].m_testResults = m_currentTest->GetResults();
            m_currentState = State::StartNextTest;

            if (!m_featureTests[m_currentTest].m_testResults.Succeeded)
            {
                CryWarning(EValidatorModule::VALIDATOR_MODULE_FEATURETESTS, EValidatorSeverity::VALIDATOR_ERROR, "Feature Test Failed, see junit report for more information");
            }
            CryLogAlways("=============================== Finished Feature Test %s ===============================", m_currentTest->GetName());
        }
        //Time out the test. The test will get left in the running state probably, and it could cause problems for the next test if it does anything.
        if (m_currentTestRunningTime > m_currentTest->GetMaxRunningTime())
        {
            m_featureTests[m_currentTest].m_timeToRun = m_currentTestRunningTime;
            m_featureTests[m_currentTest].m_testResults.Succeeded = false;
            m_featureTests[m_currentTest].m_testResults.ResultDescription = "Test was canceled because it timed out. This could cause other tests to fail.";
            m_currentState = State::StartNextTest;
        }
    }

    void FeatureTestsManager::StateFinished()
    {
        CRY_ASSERT(m_currentState == State::Finished);

        CreateJUnitResultsFile();
        gEnv->pSystem->Quit();
    }

    void FeatureTestsManager::CreateJUnitResultsFile()
    {
        int totalTime = static_cast<int>((CryGetTicks() - m_timeStart) / CryGetTicksPerSec());

        XmlNodeRef testSuites = GetISystem()->CreateXmlNode("testsuites");

        XmlNodeRef testSuite = GetISystem()->CreateXmlNode("testsuite");
        string testName;
        testName.Format("%s - %s", m_mapName.c_str(), m_testGroupName.c_str());
        testSuite->setAttr("name", testName.c_str());
        testSuite->setAttr("tests", m_featureTests.size());
        testSuite->setAttr("failures", std17::count_if(m_featureTests, [](const FeatureTestType::value_type& test) { return !test.second.m_testResults.Succeeded; }));
        testSuite->setAttr("errors", 0);
        testSuite->setAttr("time", totalTime);
        testSuite->setAttr("timestamp", m_timeStamp);
        testSuites->addChild(testSuite);

        for (const auto& test : m_featureTests)
        {
            XmlNodeRef testCase = GetISystem()->CreateXmlNode("testCase");
            testCase->setAttr("name", test.first->GetName());
            testCase->setAttr("description", test.first->GetDescription());
            testCase->setAttr("time", test.second.m_timeToRun);
            if (!test.second.m_testResults.Succeeded)
            {
                XmlNodeRef failure = GetISystem()->CreateXmlNode("failure");
                failure->setAttr("message", test.second.m_testResults.ResultDescription.c_str());
                failure->setContent(test.second.m_testResults.AdditionalDetails.c_str());
                testCase->addChild(failure);
            }

            testSuite->addChild(testCase);
        }

        SaveFiles(testSuites);
    }

    bool FeatureTestsManager::SaveFiles(const XmlNodeRef& xmlToSave)
    {
        string junitFilename;
        string gameLogFilename;

        junitFilename.Format("@LOG@/Reports/%s/%s/report.xml", m_mapName.c_str(), m_testGroupName.c_str());
        gameLogFilename.Format("@LOG@/Reports/%s/%s/gamelog.log", m_mapName.c_str(), m_testGroupName.c_str());

        XmlString xmlStr = xmlToSave->getXML();

        CDebugAllowFileAccess allowFileAccess;
        AZ::IO::HandleType file = gEnv->pCryPak->FOpen(junitFilename.c_str(), "wt");
        allowFileAccess.End();
        if (!file)
        {
            return false;
        }

        const char* sxml = (const char*)xmlStr;
        char xmlHeader[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
        gEnv->pCryPak->FWrite(xmlHeader, strlen(xmlHeader), file);
        gEnv->pCryPak->FWrite(sxml, xmlStr.length(), file);
        gEnv->pCryPak->FFlush(file);
        gEnv->pCryPak->FClose(file);

        //make a copy of the game.log file and the report xml
        string rootSrcPath;
        rootSrcPath.Format("@root@/%s", gEnv->pLog->GetFileName());
        char srcLogPath[ICryPak::g_nMaxPath];
        char dstLogPath[ICryPak::g_nMaxPath];
        gEnv->pCryPak->AdjustFileName(rootSrcPath.c_str(), srcLogPath, AZ_ARRAY_SIZE(srcLogPath), ICryPak::FLAGS_FOR_WRITING);
        gEnv->pCryPak->AdjustFileName(gameLogFilename, dstLogPath, AZ_ARRAY_SIZE(dstLogPath), ICryPak::FLAGS_FOR_WRITING);
        bool result = gEnv->pCryPak->CopyFileOnDisk(srcLogPath, dstLogPath, false);

        if (m_createTimestampedFolder)
        {
            auto dstPath = CreateTimeStampedFileName(dstLogPath);
            gEnv->pCryPak->CopyFileOnDisk(dstLogPath, dstPath.c_str(), false);

            char srcReportPath[ICryPak::g_nMaxPath];
            gEnv->pCryPak->AdjustFileName(junitFilename.c_str(), srcReportPath, AZ_ARRAY_SIZE(srcReportPath), ICryPak::FLAGS_FOR_WRITING);
            dstPath = CreateTimeStampedFileName(junitFilename.c_str());
            gEnv->pCryPak->CopyFileOnDisk(srcReportPath, dstPath.c_str(), false);
        }
        return true;
    }

    bool FeatureTestsManager::ReadTestConfiguration(const string& featureTest)
    {
        string testFullPath;
        testFullPath.Format("FeatureTests/FeatureTests/%s.xml", PathUtil::GetFileName(featureTest).c_str());

        auto testConfig = GetISystem()->LoadXmlFromFile(testFullPath);
        if (!testConfig)
        {
            CryWarning(EValidatorModule::VALIDATOR_MODULE_FEATURETESTS, EValidatorSeverity::VALIDATOR_ERROR, "Could not load feature test file %s", testFullPath.c_str());
            return false;
        }
        auto mapNode = testConfig->findChild("map");
        if (!mapNode)
        {
            CryWarning(EValidatorModule::VALIDATOR_MODULE_FEATURETESTS, EValidatorSeverity::VALIDATOR_ERROR, "Could not find a map to load in feature test file %s", testFullPath.c_str());
            return false;
        }
        XmlString mapName;
        mapNode->getAttr("value", mapName);
        m_mapName = std::move(mapName);

        m_testGroupName = "";
        auto testGroupNode = testConfig->findChild("testGroupName");
        if (testGroupNode)
        {
            XmlString testGroupName;
            testGroupNode->getAttr("value", testGroupName);
            m_testGroupName = std::move(testGroupName);
        }

        m_createTimestampedFolder = false;
        auto timeStampNode = testConfig->findChild("createTimestampedFolder");
        if (timeStampNode)
        {
            timeStampNode->getAttr("value", m_createTimestampedFolder);
        }

        m_maxInitTime = 1.0f;
        auto maxInitNode = testConfig->findChild("maxInitTime");
        if (maxInitNode)
        {
            maxInitNode->getAttr("value", m_maxInitTime);
        }

        //r_GraphicsQuality gets handled outside of cvar group, to make sure it gets applied first.
        auto sysSpecNode = testConfig->findChild("r_GraphicsQuality");
        if (sysSpecNode)
        {
            int sysSpec = 1;
            if (sysSpecNode->getAttr("value", sysSpec))
            {
                auto cvar = gEnv->pConsole->GetCVar("r_GraphicsQuality");
                if (cvar)
                {
                    cvar->Set(sysSpec);
                }
            }
        }

        auto cvarsNode = testConfig->findChild("cvars");
        for (auto childNode : cvarsNode)
        {
            auto cvar = gEnv->pConsole->GetCVar(childNode->getTag());
            if (cvar)
            {
                XmlString cvarValue;
                if (childNode->getAttr("value", cvarValue))
                {
                    cvar->Set(cvarValue.c_str());
                }
            }
        }
        return true;
    }

    FeatureTestHandle RegisterFeatureTest(IFeatureTest* test)
    {
        return GetManagerInstance().RegisterFeatureTest(test);
    }

    void Update()
    {
        return GetManagerInstance().Update();
    }

    void UpdateCamera()
    {
        return GetManagerInstance().UpdateCamera();
    }

    IFeatureTest* GetCurrentlyRunningTest()
    {
        return GetManagerInstance().GetCurrentlyRunningTest();
    }

    const char* GetTestGroupName()
    {
        return GetManagerInstance().GetTestGroupName();
    }

    bool ShouldCreateTimeStampedFolder()
    {
        return GetManagerInstance().ShouldCreateTimeStampedFolder();
    }

    std::string CreateTimeStampedFileName(const char* originalFileName)
    {
        return GetManagerInstance().CreateTimeStampedFileName(originalFileName);
    }

    FeatureTestsManager* s_instance = nullptr;

    void CreateManager()
    {
        AZ_Assert(!s_instance, "Feature tests manager has already been created.");
        s_instance = new FeatureTestsManager();
    }

    void DestroyManager()
    {
        delete s_instance;
        s_instance = nullptr;
    }

    FeatureTestsManager& GetManagerInstance()
    {
        AZ_Assert(s_instance, "Feature tests manager has not been created.");
        return *s_instance;
    }
} // namespace FeatureTests
