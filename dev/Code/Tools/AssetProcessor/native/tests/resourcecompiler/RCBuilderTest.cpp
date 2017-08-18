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

#include "RCBuilderTest.h"
#include "../BaseAssetProcessorTest.h"
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/AllocationRecords.h>

#include "../../resourcecompiler/RCBuilder.h"
#include "../../utilities/assetUtils.h"
#include "../../unittests/UnitTestRunner.h"

void Foo(int*, int* b)
{ 
}

TEST_F(RCBuilderTest, CreateBuilderDesc_CreateBuilder_Valid)
{
    AssetBuilderSDK::AssetBuilderPattern    pattern;
    pattern.m_pattern = "*.foo";
    AZStd::vector<AssetBuilderSDK::AssetBuilderPattern> builderPatterns;
    builderPatterns.push_back(pattern);

    TestInternalRecognizerBasedBuilder  test(new MockRCCompiler());

    AssetBuilderSDK::AssetBuilderDesc result = test.CreateBuilderDesc(this->GetBuilderID(), builderPatterns);

    ASSERT_EQ(this->GetBuilderName(), result.m_name);
    ASSERT_EQ(this->GetBuilderUUID(), result.m_busId);
    ASSERT_TRUE(result.m_patterns.size() == 1);
    ASSERT_EQ(result.m_patterns[0].m_pattern, pattern.m_pattern);
}

TEST_F(RCBuilderTest, Shutdown_NormalShutdown_Requested)
{
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);

    test.ShutDown();

    ASSERT_EQ(mockRC->m_request_quit, 1);
}


TEST_F(RCBuilderTest, Initialize_StandardInitialization_Fail)
{
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);

    MockRecognizerConfiguration     configuration;

    mockRC->SetResultInitialize(false);
    bool initialization_result = test.Initialize(configuration);
    ASSERT_FALSE(initialization_result);
}


TEST_F(RCBuilderTest, Initialize_StandardInitializationWithDuplicateAndInvalidRecognizers_Valid)
{
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);
    UnitTestUtils::AssertAbsorber       assertAbsorber;
    MockRecognizerConfiguration         configuration;

    // 3 Asset recognizers, 1 duplicate & 1 without platform should result in only 1 InternalAssetRecognizer

    // Good spec
    AssetRecognizer     good;
    good.m_name = "Good";
    good.m_patternMatcher = AssetUtilities::FilePatternMatcher("*.foo", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
    AssetPlatformSpec   good_spec;
    good_spec.m_extraRCParams = "/i";
    good.m_platformSpecs["pc"] = good_spec;

    // No Platform spec
    AssetRecognizer     no_platform;
    no_platform.m_name = "No Platform";
    no_platform.m_patternMatcher = AssetUtilities::FilePatternMatcher("*.ccc", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);

    // Duplicate
    AssetRecognizer     duplicate(good.m_name, good.m_testLockSource, good.m_priority, good.m_isCritical, good.m_supportsCreateJobs, good.m_patternMatcher, good.m_version, good.m_productAssetType);
    duplicate.m_platformSpecs["pc"] = good_spec;

    configuration.m_recognizerContainer["good"] = good;
    configuration.m_recognizerContainer["no_platform"] = no_platform;
    configuration.m_recognizerContainer["duplicate"] = duplicate;

    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    ASSERT_EQ(mockRC->m_initialize, 1);

    InternalRecognizerPointerContainer  good_recognizers;
    bool good_recognizers_found = test.GetMatchingRecognizers(AssetBuilderSDK::Platform::Platform_PC, "test.foo", good_recognizers);
    ASSERT_TRUE(good_recognizers_found); // Should find at least 1
    ASSERT_EQ(good_recognizers.size(), 1);  // 1, not 2 since the duplicates should be removed
    ASSERT_EQ(good_recognizers.at(0)->m_name, good.m_name); // Match the same recognizer


    InternalRecognizerPointerContainer  bad_recognizers;
    bool no_recognizers_found = !test.GetMatchingRecognizers(AssetBuilderSDK::Platform::Platform_PC, "test.ccc", good_recognizers);
    ASSERT_TRUE(no_recognizers_found);
    ASSERT_EQ(bad_recognizers.size(), 0);  // 1, not 2 since the duplicates should be removed

    ASSERT_EQ(assertAbsorber.m_numWarningsAbsorbed, 1); // this should be the "duplicate builder" warning.
    ASSERT_EQ(assertAbsorber.m_numErrorsAbsorbed, 0);
    ASSERT_EQ(assertAbsorber.m_numAssertsAbsorbed, 0);
}


TEST_F(RCBuilderTest, CreateJobs_CreateSingleJobStandard_Valid)
{
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);

    MockRecognizerConfiguration         configuration;

    AssetRecognizer     good;
    good.m_name = "Good";
    good.m_patternMatcher = AssetUtilities::FilePatternMatcher("*.foo", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
    AssetPlatformSpec   good_spec;
    good_spec.m_extraRCParams = "/i";
    good.m_platformSpecs["pc"] = good_spec;
    configuration.m_recognizerContainer["good"] = good;


    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    AssetBuilderSDK::CreateJobsRequest  request;
    AssetBuilderSDK::CreateJobsResponse response;

    request.m_watchFolder = "c:\temp";
    request.m_sourceFile = "test.foo";
    request.m_platformFlags = AssetBuilderSDK::Platform::Platform_PC;
    AssetProcessor::BUILDER_ID_RC.GetUuid(request.m_builderid);

    test.CreateJobs(request, response);

    ASSERT_EQ(response.m_result, AssetBuilderSDK::CreateJobsResultCode::Success);
    ASSERT_EQ(response.m_createJobOutputs.size(), 1);

    AssetBuilderSDK::JobDescriptor descriptor = response.m_createJobOutputs.at(0);
    ASSERT_EQ(descriptor.m_platform, AssetBuilderSDK::Platform::Platform_PC);
    ASSERT_FALSE(descriptor.m_critical);
}

TEST_F(RCBuilderTest, CreateJobs_CreateMultiplesJobStandard_Valid)
{
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);

    MockRecognizerConfiguration         configuration;

    AssetRecognizer         standard_AR_RC;
    const AZStd::string     job_key_rc = "RCjob";
    {
        standard_AR_RC.m_name = "RCjob";
        standard_AR_RC.m_patternMatcher = AssetUtilities::FilePatternMatcher("*.foo", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
        AssetPlatformSpec   good_rc_spec;
        good_rc_spec.m_extraRCParams = "/i";
        standard_AR_RC.m_platformSpecs["pc"] = good_rc_spec;
    }
    configuration.m_recognizerContainer["rc_foo"] = standard_AR_RC;

    AssetRecognizer         standard_AR_Copy;
    const AZStd::string     job_key_copy = "Copyjob";
    {
        standard_AR_Copy.m_name = QString(job_key_copy.c_str());
        standard_AR_Copy.m_patternMatcher = AssetUtilities::FilePatternMatcher("*.foo", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
        AssetPlatformSpec   good_copy_spec;
        good_copy_spec.m_extraRCParams = "copy";
        standard_AR_Copy.m_platformSpecs["pc"] = good_copy_spec;
    }
    configuration.m_recognizerContainer["copy_foo"] = standard_AR_Copy;


    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    // Request is for the copy builder
    {
        AssetBuilderSDK::CreateJobsRequest  request_copy;
        AssetBuilderSDK::CreateJobsResponse response_copy;

        request_copy.m_watchFolder = "c:\temp";
        request_copy.m_sourceFile = "test.foo";
        request_copy.m_platformFlags = AssetBuilderSDK::Platform::Platform_PC;
        AssetProcessor::BUILDER_ID_COPY.GetUuid(request_copy.m_builderid);
        test.CreateJobs(request_copy, response_copy);

        ASSERT_EQ(response_copy.m_result, AssetBuilderSDK::CreateJobsResultCode::Success);
        ASSERT_EQ(response_copy.m_createJobOutputs.size(), 1);

        AssetBuilderSDK::JobDescriptor descriptor = response_copy.m_createJobOutputs.at(0);
        ASSERT_EQ(descriptor.m_platform, AssetBuilderSDK::Platform::Platform_PC);
        ASSERT_EQ(descriptor.m_jobKey.compare(job_key_copy), 0);
        ASSERT_TRUE(descriptor.m_critical);
    }

    // Request is for the rc builder
    {
        AssetBuilderSDK::CreateJobsRequest  request_rc;
        AssetBuilderSDK::CreateJobsResponse response_rc;

        request_rc.m_watchFolder = "c:\temp";
        request_rc.m_sourceFile = "test.foo";
        request_rc.m_platformFlags = AssetBuilderSDK::Platform::Platform_PC;
        AssetProcessor::BUILDER_ID_RC.GetUuid(request_rc.m_builderid);
        test.CreateJobs(request_rc, response_rc);

        ASSERT_EQ(response_rc.m_result, AssetBuilderSDK::CreateJobsResultCode::Success);
        ASSERT_EQ(response_rc.m_createJobOutputs.size(), 1);

        AssetBuilderSDK::JobDescriptor descriptor = response_rc.m_createJobOutputs.at(0);
        ASSERT_EQ(descriptor.m_platform, AssetBuilderSDK::Platform::Platform_PC);
        ASSERT_EQ(descriptor.m_jobKey.compare(job_key_rc), 0);
        ASSERT_FALSE(descriptor.m_critical);
    }

}

TEST_F(RCBuilderTest, CreateJobs_CreateSingleJobCopy_Valid)
{
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);

    MockRecognizerConfiguration     configuration;

    AssetRecognizer     copy;
    copy.m_name = "Copy";
    copy.m_patternMatcher = AssetUtilities::FilePatternMatcher("*.copy", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
    AssetPlatformSpec   copy_spec;
    copy_spec.m_extraRCParams = "copy";
    copy.m_platformSpecs["pc"] = copy_spec;
    configuration.m_recognizerContainer["copy"] = copy;



    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    AssetBuilderSDK::CreateJobsRequest  request;
    AssetBuilderSDK::CreateJobsResponse response;

    request.m_watchFolder = "c:\temp";
    request.m_sourceFile = "test.copy";
    request.m_platformFlags = AssetBuilderSDK::Platform::Platform_PC;
    AssetProcessor::BUILDER_ID_COPY.GetUuid(request.m_builderid);

    test.CreateJobs(request, response);

    ASSERT_EQ(response.m_result, AssetBuilderSDK::CreateJobsResultCode::Success);
    ASSERT_EQ(response.m_createJobOutputs.size(), 1);

    AssetBuilderSDK::JobDescriptor descriptor = response.m_createJobOutputs.at(0);
    ASSERT_EQ(descriptor.m_platform, AssetBuilderSDK::Platform::Platform_PC);
    ASSERT_TRUE(descriptor.m_critical);
}

TEST_F(RCBuilderTest, CreateJobs_CreateSingleJobStandardSkip_Valid)
{
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);

    MockRecognizerConfiguration     configuration;

    {
        AssetRecognizer     skip;
        skip.m_name = "Skip";
        skip.m_patternMatcher = AssetUtilities::FilePatternMatcher("*.skip", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
        AssetPlatformSpec   skip_spec;
        skip_spec.m_extraRCParams = "skip";
        skip.m_platformSpecs["pc"] = skip_spec;
        configuration.m_recognizerContainer["skip"] = skip;
    }


    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    AssetBuilderSDK::CreateJobsRequest  request;
    AssetBuilderSDK::CreateJobsResponse response;

    request.m_watchFolder = "c:\temp";
    request.m_sourceFile = "test.skip";
    request.m_platformFlags = AssetBuilderSDK::Platform::Platform_PC;
    request.m_builderid = this->GetBuilderUUID();

    test.CreateJobs(request, response);

    ASSERT_EQ(response.m_result, AssetBuilderSDK::CreateJobsResultCode::Success);
    ASSERT_EQ(response.m_createJobOutputs.size(), 0);
}


TEST_F(RCBuilderTest, CreateJobs_CreateSingleJobStandard_Failed)
{
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);

    MockRecognizerConfiguration     configuration;

    AssetRecognizer     good;
    good.m_name = "Good";
    good.m_patternMatcher = AssetUtilities::FilePatternMatcher("*.foo", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
    AssetPlatformSpec   good_spec;
    good_spec.m_extraRCParams = "/i";
    good.m_platformSpecs["pc"] = good_spec;
    configuration.m_recognizerContainer["good"] = good;


    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    AssetBuilderSDK::CreateJobsRequest  request;
    AssetBuilderSDK::CreateJobsResponse response;

    request.m_watchFolder = "c:\temp";
    request.m_sourceFile = "test.ccc";
    request.m_platformFlags = AssetBuilderSDK::Platform::Platform_PC;
    request.m_builderid = this->GetBuilderUUID();

    test.CreateJobs(request, response);

    ASSERT_EQ(response.m_result, AssetBuilderSDK::CreateJobsResultCode::Failed);
    ASSERT_EQ(response.m_createJobOutputs.size(), 0);
}

TEST_F(RCBuilderTest, CreateJobs_CreateSingleJobStandard_ShuttingDown)
{
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);

    MockRecognizerConfiguration     configuration;

    AssetRecognizer     good;
    good.m_name = "Good";
    good.m_patternMatcher = AssetUtilities::FilePatternMatcher("*.foo", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
    AssetPlatformSpec   good_spec;
    good_spec.m_extraRCParams = "/i";
    good.m_platformSpecs["pc"] = good_spec;
    configuration.m_recognizerContainer["good"] = good;


    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    test.ShutDown();

    AssetBuilderSDK::CreateJobsRequest  request;
    AssetBuilderSDK::CreateJobsResponse response;

    request.m_watchFolder = "c:\temp";
    request.m_sourceFile = "test.ccc";
    request.m_platformFlags = AssetBuilderSDK::Platform::Platform_PC;
    request.m_builderid = this->GetBuilderUUID();

    test.CreateJobs(request, response);

    ASSERT_EQ(response.m_result, AssetBuilderSDK::CreateJobsResultCode::ShuttingDown);
    ASSERT_EQ(response.m_createJobOutputs.size(), 0);
}

TEST_F(RCBuilderTest, CreateJobs_CreateSingleJobBadJobRequest1_Failed)
{
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);

    MockRecognizerConfiguration     configuration;

    AssetRecognizer     good;
    good.m_name = "Good";
    good.m_patternMatcher = AssetUtilities::FilePatternMatcher("*.foo", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
    AssetPlatformSpec   good_spec;
    good_spec.m_extraRCParams = "/i";
    good.m_platformSpecs["pc"] = good_spec;
    configuration.m_recognizerContainer["good"] = good;


    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    AssetBuilderSDK::CreateJobsRequest  request;
    AssetBuilderSDK::CreateJobsResponse response;

    request.m_sourceFile = "test.ccc";
    request.m_platformFlags = AssetBuilderSDK::Platform::Platform_PC;
    request.m_builderid = this->GetBuilderUUID();

    test.CreateJobs(request, response);

    ASSERT_EQ(response.m_result, AssetBuilderSDK::CreateJobsResultCode::Failed);
    ASSERT_EQ(response.m_createJobOutputs.size(), 0);
}

TEST_F(RCBuilderTest, ProcessLegacyRCJob_ProcessStandardSingleJob_Failed)
{
    AZ::Uuid                            assetTypeUUid = AZ::Uuid::CreateRandom();
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);

    MockRecognizerConfiguration         configuration;

    // Create the test job
    AssetBuilderSDK::ProcessJobRequest request = CreateTestJobRequest("file.c", false, AssetBuilderSDK::Platform::Platform_PC, 1);

    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    // Case 1: execution failed
    mockRC->SetResultResultExecute(NativeLegacyRCCompiler::Result(1, true, ""));
    mockRC->SetResultExecute(false);
    AssetBuilderSDK::ProcessJobResponse responseCrashed;
    AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);
    test.TestProcessLegacyRCJob(request, "/i", assetTypeUUid, jobCancelListener, responseCrashed);
    ASSERT_EQ(responseCrashed.m_resultCode, AssetBuilderSDK::ProcessJobResultCode::ProcessJobResult_Crashed);

    // case 2: result code from execution non-zero
    mockRC->SetResultExecute(true);
    mockRC->SetResultResultExecute(NativeLegacyRCCompiler::Result(1, false, ""));
    AssetBuilderSDK::ProcessJobResponse responseFailed;
    test.TestProcessLegacyRCJob(request, "/i", assetTypeUUid, jobCancelListener, responseFailed);
    ASSERT_EQ(responseFailed.m_resultCode, AssetBuilderSDK::ProcessJobResultCode::ProcessJobResult_Failed);
}


TEST_F(RCBuilderTest, ProcessLegacyRCJob_ProcessStandardSingleJob_Valid)
{
    AZ::Uuid                            assetTypeUUid = AZ::Uuid::CreateRandom();
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);

    MockRecognizerConfiguration         configuration;

    // Create the test job
    AssetBuilderSDK::ProcessJobRequest request = CreateTestJobRequest("file.c", false, AssetBuilderSDK::Platform::Platform_PC);


    test.AddTestFileInfo("c:\\temp\\file.a").AddTestFileInfo("c:\\temp\\file.b");


    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    mockRC->SetResultResultExecute(NativeLegacyRCCompiler::Result(0, false, "c:\\temp"));
    mockRC->SetResultExecute(true);
    AssetBuilderSDK::ProcessJobResponse response;
    AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);
    test.TestProcessLegacyRCJob(request, "/i", assetTypeUUid, jobCancelListener, response);
    ASSERT_EQ(response.m_resultCode, AssetBuilderSDK::ProcessJobResultCode::ProcessJobResult_Success);

    ASSERT_EQ(response.m_outputProducts.size(), 2); // file.c->(file.a, file.b)
}


TEST_F(RCBuilderTest, ProcessLegacyRCJob_ProcessCopySingleJob_Valid)
{
    AZStd::string                       name = "test";
    AZ::Uuid                            builderUuid = AZ::Uuid::CreateRandom();
    AZ::Uuid                            assetTypeUUid = AZ::Uuid::CreateRandom();
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);

    MockRecognizerConfiguration         configuration;

    // Create the test job
    AssetBuilderSDK::ProcessJobRequest request = CreateTestJobRequest("file.c", false, AssetBuilderSDK::Platform::Platform_PC);


    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    mockRC->SetResultResultExecute(NativeLegacyRCCompiler::Result(0, false, "c:\\temp"));
    mockRC->SetResultExecute(true);
    AssetBuilderSDK::ProcessJobResponse response;
    AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);
    test.TestProcessCopyJob(request, assetTypeUUid, jobCancelListener, response);
    ASSERT_EQ(response.m_resultCode, AssetBuilderSDK::ProcessJobResultCode::ProcessJobResult_Success);

    ASSERT_EQ(response.m_outputProducts.size(), 1); // file.c->(file.a, file.b)
    AssetBuilderSDK::JobProduct resultJobProd = response.m_outputProducts.at(0);
    ASSERT_EQ(resultJobProd.m_productAssetType, assetTypeUUid);
    ASSERT_EQ(resultJobProd.m_productFileName, request.m_fullPath);
}


TEST_F(RCBuilderTest, MatchTempFileToSkip_SkipRCFiles_true)
{
    const char* rc_skip_fileNames[] = {
        "rc_createdfiles.txt",
        "rc_log.log",
        "rc_log_warnings.log",
        "rc_log_errors.log"
    };

    for (const char* filename : rc_skip_fileNames)
    {
        ASSERT_TRUE(AssetProcessor::InternalRecognizerBasedBuilder::MatchTempFileToSkip(filename));
    }
}

TEST_F(RCBuilderTest, MatchTempFileToSkip_SkipRCFiles_false)
{
    const char* rc_not_skip_fileNames[] = {
        "foo.log",
        "bar.txt"
    };

    for (const char* filename : rc_not_skip_fileNames)
    {
        ASSERT_FALSE(AssetProcessor::InternalRecognizerBasedBuilder::MatchTempFileToSkip(filename));
    }
}



TEST_F(RCBuilderTest, ProcessJob_ProcessStandardRCSingleJob_Valid)
{
    AZ::Uuid                            assetTypeUUid = AZ::Uuid::CreateRandom();
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);
    MockRecognizerConfiguration         configuration;


    // Create a dummy test recognizer
    AZ::u32 recID = test.AddTestRecognizer(this->GetBuilderID(),QString("/i"), AssetBuilderSDK::Platform::Platform_PC);

    // Create the test job
    AssetBuilderSDK::ProcessJobRequest request = CreateTestJobRequest("test.tif", false, AssetBuilderSDK::Platform::Platform_PC);
    request.m_jobDescription.m_jobParameters[recID] = "/i";


    test.AddTestFileInfo("c:\\temp\\file.a").AddTestFileInfo("c:\\temp\\file.b");

    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    mockRC->SetResultResultExecute(NativeLegacyRCCompiler::Result(0, false, "c:\\temp"));
    mockRC->SetResultExecute(true);
    AssetBuilderSDK::ProcessJobResponse response;
    test.TestProcessJob(request, response);
    ASSERT_EQ(response.m_resultCode, AssetBuilderSDK::ProcessJobResultCode::ProcessJobResult_Success);
    ASSERT_EQ(response.m_outputProducts.size(), 2); // file.c->(file.a, file.b)
}


TEST_F(RCBuilderTest, ProcessJob_ProcessStandardRCSingleJob_Failed)
{
    AZ::Uuid                            assetTypeUUid = AZ::Uuid::CreateRandom();
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);
    MockRecognizerConfiguration         configuration;


    // Create a dummy test recognizer
    AZ::u32 recID = test.AddTestRecognizer(this->GetBuilderID(), QString("/i"), AssetBuilderSDK::Platform::Platform_PC);

    // Create the test job
    AssetBuilderSDK::ProcessJobRequest request = CreateTestJobRequest("test.tif", false, AssetBuilderSDK::Platform::Platform_PC);
    request.m_jobDescription.m_jobParameters[recID] = "/i";

    test.AddTestFileInfo("c:\\temp\\file.a").AddTestFileInfo("c:\\temp\\file.b");

    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    mockRC->SetResultResultExecute(NativeLegacyRCCompiler::Result(1, false, "c:\\temp"));
    mockRC->SetResultExecute(true);
    AssetBuilderSDK::ProcessJobResponse response;
    test.TestProcessJob(request, response);
    ASSERT_EQ(response.m_resultCode, AssetBuilderSDK::ProcessJobResultCode::ProcessJobResult_Failed);
}


TEST_F(RCBuilderTest, ProcessJob_ProcessStandardCopySingleJob_Valid)
{
    AZ::Uuid                            assetTypeUUid = AZ::Uuid::CreateRandom();
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);
    MockRecognizerConfiguration         configuration;


    // Create a dummy test recognizer
    AZ::u32 recID = test.AddTestRecognizer(this->GetBuilderID(), QString("copy"), AssetBuilderSDK::Platform::Platform_PC);

    // Create the test job
    AssetBuilderSDK::ProcessJobRequest request = CreateTestJobRequest("test.tif", true, AssetBuilderSDK::Platform::Platform_PC);
    request.m_jobDescription.m_jobParameters[recID] = "copy";

    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    mockRC->SetResultExecute(true);
    AssetBuilderSDK::ProcessJobResponse response;
    test.TestProcessJob(request, response);
    ASSERT_EQ(response.m_resultCode, AssetBuilderSDK::ProcessJobResultCode::ProcessJobResult_Success);
    ASSERT_EQ(response.m_outputProducts.size(), 1); // test.
    ASSERT_TRUE(response.m_outputProducts[0].m_productFileName.find("test.tif") != AZStd::string::npos);
}

TEST_F(RCBuilderTest, ProcessJob_ProcessStandardSkippedSingleJob_Invalid)
{
    AZ::Uuid                            assetTypeUUid = AZ::Uuid::CreateRandom();
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);
    MockRecognizerConfiguration         configuration;


    // Create a dummy test recognizer
    AZ::u32 recID = test.AddTestRecognizer(this->GetBuilderID(), QString("skip"), AssetBuilderSDK::Platform::Platform_PC);

    // Create the test job
    AssetBuilderSDK::ProcessJobRequest request = CreateTestJobRequest("test.tif", true, AssetBuilderSDK::Platform::Platform_PC);
    request.m_jobDescription.m_jobParameters[recID] = "copy";

    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    mockRC->SetResultExecute(true);
    AssetBuilderSDK::ProcessJobResponse response;
    test.TestProcessJob(request, response);
    ASSERT_EQ(response.m_resultCode, AssetBuilderSDK::ProcessJobResultCode::ProcessJobResult_Failed);
}

