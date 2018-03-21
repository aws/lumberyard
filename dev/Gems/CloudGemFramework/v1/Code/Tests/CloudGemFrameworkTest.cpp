#include "CloudGemFramework_precompiled.h"

#include <AzTest/AzTest.h>

#include <AzCore/Jobs/JobManagerBus.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <CloudGemFramework/AwsApiRequestJob.h>
#include <CloudGemFramework/ServiceRequestJob.h>
#include <CloudGemFramework/JsonObjectHandler.h>
#include <CloudGemFramework/JsonWriter.h>
#include <CloudGemFramework/HttpRequestJob.h>

#include <chrono>

#pragma warning(disable: 4355) // <future> includes ppltasks.h which throws a C4355 warning: 'this' used in base member initializer list
#include <aws/lambda/LambdaClient.h>
#include <aws/lambda/model/ListFunctionsRequest.h>
#include <aws/lambda/model/ListFunctionsResult.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#pragma warning(default: 4355)

#include "ServiceApi.h"

using namespace ::testing;

class UnitTestEnvironment : public AZ::Test::ITestEnvironment
{

public:

    void SetupEnvironment() override
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
    }

    void TeardownEnvironment() override
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    }

};

AZ_UNIT_TEST_HOOK(new UnitTestEnvironment());

class AwsIntegrationTestEnvionment : public AZ::Test::ITestEnvironment
{

public:

    void SetupEnvironment() override
    {
        AZStd::string url;
        EBUS_EVENT_RESULT(url, CloudGemFramework::CloudGemFrameworkRequestBus, GetServiceUrl, "TestResourceGroup1");
        if(url.empty())
        {
            puts("");
            puts("---------------------------------------------------------------------------------");
            puts("-- ");
            puts("-- Use Gems\\CloudGemFramework\\AWS\\test\\runtests.cmd to execute these tests.");
            puts("--");
            puts("-- These tests depend on the following conditions to run successfully:");
            puts("-- ");
            puts("-- 1) sys_game_folder=CloudGemSamples must be specified in the bootstrap.cfg.");
            puts("-- ");
            puts("-- 2) Cloud Canvas project, deployment, and resource group stacks must be setup");
            puts("--    as done by Gems\\CloudGemFramework\\AWS\\test\\integration_test.py before it");
            puts("--    executes the test.");
            puts("-- ");
            puts("-- 3) There must be a \"default\" AWS credentials profile with lambda::ListFunctions");
            puts("      permissions.");
            puts("-- ");
            puts("---------------------------------------------------------------------------------");
            puts("");
        }
        ASSERT_FALSE(url.empty());

    }

    void TeardownEnvironment() override
    {
    }

};

AZ_INTEG_TEST_HOOK(new AwsIntegrationTestEnvionment());

template<class T>
struct TestObject
{

    TestObject()
    {
    }

    TestObject(const T& initialValue)
        : value{initialValue}
    {
    }

    T value;

    bool OnJsonKey(const char* key, CloudGemFramework::JsonReader& reader)
    {
        if(strcmp(key, "value") == 0) return reader.Accept(value);
        return reader.Ignore();
    }

    bool WriteJson(CloudGemFramework::JsonWriter& writer) const
    {
        bool ok = true;
        ok = ok && writer.StartObject();
        ok = ok && writer.Write("value", value);
        ok = ok && writer.EndObject();
        return ok;
    }

    bool operator==(const TestObject& other) const
    {
        return value == other.value;
    }

};

AZStd::string TestObjectJson(const char* value)
{
    return AZStd::string::format("{\"value\":%s}", value);
}

template<class ValueType>
void TestJsonReaderSuccess(const ValueType& expectedValue, const char* valueString)
{

    Aws::StringStream stringStream{TestObjectJson(valueString).c_str()};
    CloudGemFramework::JsonInputStream jsonStream{stringStream};

    TestObject<ValueType> object;

    AZStd::string errorMessage;

    bool ok = CloudGemFramework::JsonReader::ReadObject(jsonStream, object, errorMessage);

    ValueType& actualValue = object.value;

    ASSERT_TRUE(ok);
    ASSERT_TRUE(errorMessage.empty());
    ASSERT_EQ(actualValue, expectedValue);

}

template<class ValueType>
void TestJsonReaderFailure(const char* valueString)
{

    Aws::StringStream stringStream{TestObjectJson(valueString).c_str()};
    CloudGemFramework::JsonInputStream jsonStream{stringStream};

    TestObject<ValueType> object;

    AZStd::string errorMessage;

    bool ok = CloudGemFramework::JsonReader::ReadObject(jsonStream, object, errorMessage);

    ASSERT_FALSE(ok);
    ASSERT_FALSE(errorMessage.empty());

    puts(errorMessage.c_str());

}

template<class ValueType>
void TestJsonWriterSuccess(const ValueType& actualValue, const char* valueString)
{

    Aws::StringStream stringStream{};
    CloudGemFramework::JsonOutputStream jsonStream{stringStream};

    TestObject<ValueType> object;
    object.value = actualValue;

    CloudGemFramework::JsonWriter writer{jsonStream};

    bool ok = writer.Write(object);

    Aws::String actualJson = stringStream.str();

    ASSERT_TRUE(ok);

    AZStd::string expectedJson = TestObjectJson(valueString);

    puts(actualJson.c_str());
    puts(expectedJson.c_str());

    ASSERT_EQ(actualJson, expectedJson.c_str());

}

const AZStd::string STRING_VALUE{"s"};
const char* STRING_VALUE_STRING = "s";
const char* STRING_VALUE_JSON = "\"s\"";

const char* CHARPTR_VALUE{"s"};
const char* CHARPTR_VALUE_STRING = "s";
const char* CHARPTR_VALUE_JSON = "\"s\"";

const bool BOOL_VALUE{true};
const char* BOOL_VALUE_STRING = "true";

const int INT_VALUE{-2};
const char* INT_VALUE_STRING = "-2";

const unsigned UINT_VALUE{2};
const char* UINT_VALUE_STRING = "2";

const unsigned UINT_VALUE_MAX{4294967295};
const char* UINT_VALUE_MAX_STRING = "4294967295";

const int64_t INT64_VALUE{INT64_C(-3000000000)}; // < INT_MIN
const char* INT64_VALUE_STRING = "-3000000000";

const uint64_t UINT64_VALUE{UINT64_C(5000000000)}; // > UINT_MAX and < INT64_MAX
const char* UINT64_VALUE_STRING = "5000000000";

const uint64_t UINT64_VALUE_MAX{18446744073709551615U};
const char* UINT64_VALUE_MAX_STRING = "18446744073709551615";

const double DOUBLE_VALUE{1.0};
const char* DOUBLE_VALUE_STRING = "1.0";

using OBJECT_TYPE = TestObject<AZStd::string>;
//const OBJECT_TYPE OBJECT_VALUE{"s"}; // can't create here because allocator isn't initialized yet
const char* OBJECT_VALUE_JSON = "{\"value\":\"s\"}";

using ARRAY_TYPE = AZStd::vector<AZStd::string>;
//const ARRAY_TYPE ARRAY_VALUE{"a", "b", "c"}; // can't create here because allocator isn't initialized yet
const char* ARRAY_VALUE_JSON = "[\"a\",\"b\",\"c\"]";

using ARRAY_OF_ARRAY_TYPE = AZStd::vector<AZStd::vector<AZStd::string>>;
const char* ARRAY_OF_ARRAY_VALUE_JSON = "[[\"a1\",\"b1\",\"c1\"],[\"a2\",\"b2\",\"c2\"]]";

using ARRAY_OF_OBJECT_TYPE = AZStd::vector<TestObject<AZStd::string>>;
const char* ARRAY_OF_OBJECT_VALUE_JSON = "[{\"value\":\"s1\"},{\"value\":\"s2\"}]";

const char* UNESCAPED = "abc !#$%&'()*+,/:;=?@[]123";
const char* ESCAPED = "abc%20%21%23%24%25%26%27%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5D123";

//////////////////////////////////////////////////////////////////////////////
// JsonReader AZStd::string Tests

TEST(JsonReader, StringString)
{
    TestJsonReaderSuccess<AZStd::string>(STRING_VALUE, STRING_VALUE_JSON);
}

TEST(JsonReader, StringInt)
{
    TestJsonReaderFailure<AZStd::string>(INT_VALUE_STRING);
}

//////////////////////////////////////////////////////////////////////////////
// JsonReader bool Tests

TEST(JsonReader, BoolBool)
{
    TestJsonReaderSuccess<bool>(BOOL_VALUE, BOOL_VALUE_STRING);
}

TEST(JsonReader, BoolString)
{
    TestJsonReaderFailure<bool>(STRING_VALUE_JSON);
}

//////////////////////////////////////////////////////////////////////////////
// JsonReader int Tests

TEST(JsonReader, IntInt)
{
    TestJsonReaderSuccess<int>(INT_VALUE, INT_VALUE_STRING);
}

TEST(JsonReader, IntUInt)
{
    TestJsonReaderSuccess<int>(UINT_VALUE, UINT_VALUE_STRING);
}

TEST(JsonReader, IntUIntMax)
{
    TestJsonReaderFailure<int>(UINT_VALUE_MAX_STRING);
}

TEST(JsonReader, IntInt64)
{
    TestJsonReaderFailure<int>(INT64_VALUE_STRING);
}

TEST(JsonReader, IntUInt64)
{
    TestJsonReaderFailure<int>(UINT64_VALUE_STRING);
}

TEST(JsonReader, IntDouble)
{
    TestJsonReaderFailure<int>(DOUBLE_VALUE_STRING);
}

TEST(JsonReader, IntString)
{
    TestJsonReaderFailure<int>(STRING_VALUE_JSON);
}

//////////////////////////////////////////////////////////////////////////////
// JsonReader unsigned Tests

TEST(JsonReader, UIntInt)
{
    TestJsonReaderFailure<unsigned>(INT_VALUE_STRING);
}

TEST(JsonReader, UIntUInt)
{
    TestJsonReaderSuccess<unsigned>(UINT_VALUE, UINT_VALUE_STRING);
}

TEST(JsonReader, UIntInt64)
{
    TestJsonReaderFailure<unsigned>(INT64_VALUE_STRING);
}

TEST(JsonReader, UIntUInt64)
{
    TestJsonReaderFailure<unsigned>(UINT64_VALUE_STRING);
}

TEST(JsonReader, UIntDouble)
{
    TestJsonReaderFailure<unsigned>(DOUBLE_VALUE_STRING);
}

TEST(JsonReader, UIntString)
{
    TestJsonReaderFailure<unsigned>(STRING_VALUE_JSON);
}

//////////////////////////////////////////////////////////////////////////////
// JsonReader int64_t Tests

TEST(JsonReader, Int64Int)
{
    TestJsonReaderSuccess<int64_t>(INT_VALUE, INT_VALUE_STRING);
}

TEST(JsonReader, Int64UInt)
{
    TestJsonReaderSuccess<int64_t>(UINT_VALUE, UINT_VALUE_STRING);
}

TEST(JsonReader, Int64Int64)
{
    TestJsonReaderSuccess<int64_t>(INT64_VALUE, INT64_VALUE_STRING);
}

TEST(JsonReader, Int64UInt64)
{
    TestJsonReaderSuccess<int64_t>(UINT64_VALUE, UINT64_VALUE_STRING);
}

TEST(JsonReader, Int64UInt64Max)
{
    TestJsonReaderFailure<int64_t>(UINT64_VALUE_MAX_STRING);
}

TEST(JsonReader, Int64Double)
{
    TestJsonReaderFailure<int64_t>(DOUBLE_VALUE_STRING);
}

TEST(JsonReader, Int64String)
{
    TestJsonReaderFailure<int64_t>(STRING_VALUE_JSON);
}

//////////////////////////////////////////////////////////////////////////////
// JsonReader uint64_t Tests

TEST(JsonReader, UInt64Int)
{
    TestJsonReaderFailure<uint64_t>(INT_VALUE_STRING);
}

TEST(JsonReader, UInt64UInt)
{
    TestJsonReaderSuccess<uint64_t>(UINT_VALUE, UINT_VALUE_STRING);
}

TEST(JsonReader, UInt64Int64)
{
    TestJsonReaderFailure<uint64_t>(INT64_VALUE_STRING);
}

TEST(JsonReader, UInt64UInt64)
{
    TestJsonReaderSuccess<uint64_t>(UINT64_VALUE, UINT64_VALUE_STRING);
}

TEST(JsonReader, UInt64Double)
{
    TestJsonReaderFailure<uint64_t>(DOUBLE_VALUE_STRING);
}

TEST(JsonReader, UInt64String)
{
    TestJsonReaderFailure<uint64_t>(STRING_VALUE_JSON);
}

//////////////////////////////////////////////////////////////////////////////
// JsonReader double Tests

TEST(JsonReader, DoubleInt)
{
    TestJsonReaderSuccess<double>(INT_VALUE, INT_VALUE_STRING);
}

TEST(JsonReader, DoubleUInt)
{
    TestJsonReaderSuccess<double>(UINT_VALUE, UINT_VALUE_STRING);
}

TEST(JsonReader, DoubleInt64)
{
    TestJsonReaderSuccess<double>(INT64_VALUE, INT64_VALUE_STRING);
}

TEST(JsonReader, DoubleUInt64)
{
    TestJsonReaderSuccess<double>(UINT64_VALUE, UINT64_VALUE_STRING);
}

TEST(JsonReader, DoubleDouble)
{
    TestJsonReaderSuccess<double>(DOUBLE_VALUE, DOUBLE_VALUE_STRING);
}

TEST(JsonReader, DoubleString)
{
    TestJsonReaderFailure<double>(STRING_VALUE_JSON);
}

//////////////////////////////////////////////////////////////////////////////
// JsonReader object Tests

TEST(JsonReader, ObjectObject)
{
    const OBJECT_TYPE OBJECT_VALUE{"s"};
    TestJsonReaderSuccess<OBJECT_TYPE>(OBJECT_VALUE, OBJECT_VALUE_JSON);
}

TEST(JsonReader, ObjectString)
{
    TestJsonReaderFailure<OBJECT_TYPE>(STRING_VALUE_JSON);
}

TEST(JsonReader, ObjectArray)
{
    TestJsonReaderFailure<OBJECT_TYPE>(ARRAY_VALUE_JSON);
}

//////////////////////////////////////////////////////////////////////////////
// JsonReader array Tests

TEST(JsonReader, ArrayObject)
{
    TestJsonReaderFailure<ARRAY_TYPE>(OBJECT_VALUE_JSON);
}

TEST(JsonReader, ArrayString)
{
    TestJsonReaderFailure<ARRAY_TYPE>(STRING_VALUE_JSON);
}

TEST(JsonReader, ArrayArray)
{
    const ARRAY_TYPE ARRAY_VALUE{"a", "b", "c"};
    TestJsonReaderSuccess<ARRAY_TYPE>(ARRAY_VALUE, ARRAY_VALUE_JSON);
}

TEST(JsonReader, ArrayOfArray)
{
    const ARRAY_OF_ARRAY_TYPE ARRAY_OF_ARRAY_VALUE{ {"a1", "b1", "c1"}, { "a2", "b2", "c2" } };
    TestJsonReaderSuccess<ARRAY_OF_ARRAY_TYPE>(ARRAY_OF_ARRAY_VALUE, ARRAY_OF_ARRAY_VALUE_JSON);
}

TEST(JsonReader, ArrayOfObject)
{
    const ARRAY_OF_OBJECT_TYPE ARRAY_OF_OBJECT_VALUE{ { "s1" }, { "s2" } };
    TestJsonReaderSuccess<ARRAY_OF_OBJECT_TYPE>(ARRAY_OF_OBJECT_VALUE, ARRAY_OF_OBJECT_VALUE_JSON);
}

//////////////////////////////////////////////////////////////////////////////
// JsonWriter Tests

TEST(JsonWriter, String)
{
    TestJsonWriterSuccess<AZStd::string>(STRING_VALUE, STRING_VALUE_JSON);
}

TEST(JsonWriter, Bool)
{
    TestJsonWriterSuccess<bool>(BOOL_VALUE, BOOL_VALUE_STRING);
}

TEST(JsonWriter, Int)
{
    TestJsonWriterSuccess<int>(INT_VALUE, INT_VALUE_STRING);
}

TEST(JsonWriter, UInt)
{
    TestJsonWriterSuccess<unsigned>(UINT_VALUE, UINT_VALUE_STRING);
}

TEST(JsonWriter, Int64)
{
    TestJsonWriterSuccess<int64_t>(INT64_VALUE, INT64_VALUE_STRING);
}

TEST(JsonWriter, UInt64)
{
    TestJsonWriterSuccess<uint64_t>(UINT64_VALUE, UINT64_VALUE_STRING);
}

TEST(JsonWriter, Object)
{
    const OBJECT_TYPE OBJECT_VALUE{"s"};
    TestJsonWriterSuccess<OBJECT_TYPE>(OBJECT_VALUE, OBJECT_VALUE_JSON);
}

TEST(JsonWriter, Array)
{
    const ARRAY_TYPE ARRAY_VALUE{"a", "b", "c"};
    TestJsonWriterSuccess<ARRAY_TYPE>(ARRAY_VALUE, ARRAY_VALUE_JSON);
}


///////////////////////////////////////////////////////////////////////////////
// HttpRequestJob Unit Tests

TEST(HttpRequestJob, StringToHttpMethod)
{
    EXPECT_EQ(CloudGemFramework::HttpRequestJob::HttpMethod::HTTP_GET, *CloudGemFramework::HttpRequestJob::StringToHttpMethod("GET"));
    EXPECT_EQ(CloudGemFramework::HttpRequestJob::HttpMethod::HTTP_POST, *CloudGemFramework::HttpRequestJob::StringToHttpMethod("POST"));
    EXPECT_EQ(CloudGemFramework::HttpRequestJob::HttpMethod::HTTP_DELETE, *CloudGemFramework::HttpRequestJob::StringToHttpMethod("DELETE"));
    EXPECT_EQ(CloudGemFramework::HttpRequestJob::HttpMethod::HTTP_PUT, *CloudGemFramework::HttpRequestJob::StringToHttpMethod("PUT"));
    EXPECT_EQ(CloudGemFramework::HttpRequestJob::HttpMethod::HTTP_HEAD, *CloudGemFramework::HttpRequestJob::StringToHttpMethod("HEAD"));
    EXPECT_EQ(CloudGemFramework::HttpRequestJob::HttpMethod::HTTP_PATCH, *CloudGemFramework::HttpRequestJob::StringToHttpMethod("PATCH"));
    EXPECT_FALSE(CloudGemFramework::HttpRequestJob::StringToHttpMethod("Foo"));
}


///////////////////////////////////////////////////////////////////////////////
// RequestBuilder Unit Tests

class RequestBuilderTest
    : public Test
{
public:

    CloudGemFramework::RequestBuilder target;

    void SetUp() override
    {
    }

};

TEST_F(RequestBuilderTest, WriteJsonBodyParameter)
{

    TestObject<AZStd::string> object;
    object.value = STRING_VALUE;

    target.WriteJsonBodyParameter(object);

    auto stream = target.GetBodyContent();
    ASSERT_EQ(stream->str(), TestObjectJson(STRING_VALUE_JSON).c_str());

}

TEST_F(RequestBuilderTest, SetHttpMethod)
{
    target.SetHttpMethod(Aws::Http::HttpMethod::HTTP_PATCH);
    ASSERT_EQ(target.GetHttpMethod(), Aws::Http::HttpMethod::HTTP_PATCH);
}

TEST_F(RequestBuilderTest, SetErrorMessage)
{
    target.SetErrorMessage("test");
    ASSERT_EQ(target.GetErrorMessage(), "test");
}

///////////////////////////////////////////////////////////////////////////////
// RequestBuilder SetPathParameter Unit Tests

class RequestBuilderSetPathParameterTest
    : public RequestBuilderTest
{

public:

    void SetUp() override
    {
        RequestBuilderTest::SetUp();
        target.SetRequestUrl("http://test/{param}/test");
    }

    const char* KEY = "{param}";

    void AssertUrlParam(const char* value)
    {
        ASSERT_EQ(target.GetRequestUrl(), AZStd::string::format("http://test/%s/test", value).c_str());
    }

};

TEST_F(RequestBuilderSetPathParameterTest, String)
{
    target.SetPathParameter(KEY, STRING_VALUE);
    AssertUrlParam(STRING_VALUE_STRING);
}

TEST_F(RequestBuilderSetPathParameterTest, CharPtr)
{
    target.SetPathParameter(KEY, CHARPTR_VALUE);
    AssertUrlParam(CHARPTR_VALUE_STRING);
}

TEST_F(RequestBuilderSetPathParameterTest, Int)
{
    target.SetPathParameter(KEY, INT_VALUE);
    AssertUrlParam(INT_VALUE_STRING);
}

TEST_F(RequestBuilderSetPathParameterTest, Unsigned)
{
    target.SetPathParameter(KEY, UINT_VALUE);
    AssertUrlParam(UINT_VALUE_STRING);
}

TEST_F(RequestBuilderSetPathParameterTest, Int64)
{
    target.SetPathParameter(KEY, INT64_VALUE);
    AssertUrlParam(INT64_VALUE_STRING);
}

TEST_F(RequestBuilderSetPathParameterTest, UInt64)
{
    target.SetPathParameter(KEY, UINT64_VALUE);
    AssertUrlParam(UINT64_VALUE_STRING);
}

TEST_F(RequestBuilderSetPathParameterTest, Escaped)
{
    target.SetPathParameter(KEY, UNESCAPED);
    AssertUrlParam(ESCAPED);
}

///////////////////////////////////////////////////////////////////////////////
// RequestBuilder AddQueryParameter Unit Tests

class RequestBuilderAddQueryParameterTest
    : public RequestBuilderTest
{

public:

    void SetUp() override
    {
        RequestBuilderTest::SetUp();
        target.SetRequestUrl("http://test");
    }

    const char* NAME = "param";

    void AssertUrlParam(const char* value)
    {
        ASSERT_EQ(target.GetRequestUrl(), AZStd::string::format("http://test?param=%s", value).c_str());
    }

};

TEST_F(RequestBuilderAddQueryParameterTest, String)
{
    target.AddQueryParameter(NAME, STRING_VALUE);
    AssertUrlParam(STRING_VALUE_STRING);
}

TEST_F(RequestBuilderAddQueryParameterTest, CharPtr)
{
    target.AddQueryParameter(NAME, CHARPTR_VALUE);
    AssertUrlParam(CHARPTR_VALUE_STRING);
}

TEST_F(RequestBuilderAddQueryParameterTest, Int)
{
    target.AddQueryParameter(NAME, INT_VALUE);
    AssertUrlParam(INT_VALUE_STRING);
}

TEST_F(RequestBuilderAddQueryParameterTest, Unsigned)
{
    target.AddQueryParameter(NAME, UINT_VALUE);
    AssertUrlParam(UINT_VALUE_STRING);
}

TEST_F(RequestBuilderAddQueryParameterTest, Int64)
{
    target.AddQueryParameter(NAME, INT64_VALUE);
    AssertUrlParam(INT64_VALUE_STRING);
}

TEST_F(RequestBuilderAddQueryParameterTest, UInt64)
{
    target.AddQueryParameter(NAME, UINT64_VALUE);
    AssertUrlParam(UINT64_VALUE_STRING);
}

TEST_F(RequestBuilderAddQueryParameterTest, Multiple)
{

    Aws::String expectedUrl = target.GetRequestUrl();
    expectedUrl.append("?p1=s1&p2=s2");

    target.AddQueryParameter("p1", "s1");
    target.AddQueryParameter("p2", "s2");

    ASSERT_EQ(target.GetRequestUrl(), expectedUrl);

}

TEST_F(RequestBuilderAddQueryParameterTest, Escaped)
{
    target.AddQueryParameter(NAME, UNESCAPED);
    AssertUrlParam(ESCAPED);
}

//////////////////////////////////////////////////////////////////////////////
// AwsApiRequestJob Integration Tests

INTEG_TEST(AwsApiRequestJob, OnSuccessCalled)
{

    using LambdaListFunctionsRequestJob = AWS_API_REQUEST_JOB(Lambda, ListFunctions);

    bool onSuccessCalled = false;
    bool onFailureCalled = false;

    LambdaListFunctionsRequestJob::Config config{LambdaListFunctionsRequestJob::GetDefaultConfig()};
    config.credentialsProvider = std::make_shared<Aws::Auth::ProfileConfigFileAWSCredentialsProvider>("default");

    auto job = LambdaListFunctionsRequestJob::Create(
        [&onSuccessCalled](LambdaListFunctionsRequestJob* job)
        {
            onSuccessCalled = true;
        },
        [&onFailureCalled](LambdaListFunctionsRequestJob* job)
        {
            onFailureCalled = true;
        },
        &config
    );

    job->StartAndAssistUntilComplete();
    AZ::TickBus::ExecuteQueuedEvents();

    ASSERT_TRUE(onSuccessCalled);
    ASSERT_FALSE(onFailureCalled);

}

INTEG_TEST(AwsApiRequestJob, OnFailureCalled)
{

    using LambdaListFunctionsRequestJob = AWS_API_REQUEST_JOB(Lambda, ListFunctions);

    bool onSuccessCalled = false;
    bool onFailureCalled = false;

    LambdaListFunctionsRequestJob::Config config{LambdaListFunctionsRequestJob::GetDefaultConfig()};
    config.credentialsProvider = std::make_shared<Aws::Auth::ProfileConfigFileAWSCredentialsProvider>("default");

    auto job = LambdaListFunctionsRequestJob::Create(
        [&onSuccessCalled](LambdaListFunctionsRequestJob* job)
        {
            onSuccessCalled = true;
        },
        [&onFailureCalled](LambdaListFunctionsRequestJob* job)
        {
            onFailureCalled = true;
        },
        &config
    );

    job->request.SetMarker("A BAD MARKER TO CAUSE AN ERROR");
    job->StartAndAssistUntilComplete();
    AZ::TickBus::ExecuteQueuedEvents();

    ASSERT_FALSE(onSuccessCalled);
    ASSERT_TRUE(onFailureCalled);

}

//////////////////////////////////////////////////////////////////////////////
// ServiceRequestJob Integration Tests

const char* TEST_PATH_PARAM{"test-path-param"};
const char* TEST_QUERY_PARAM_SUCCESS{"test-query-param-success"};
const char* TEST_QUERY_PARAM_FAILURE{"test-query-param-failure"};
const char* TEST_BODY_PARAM_DATA{"test-body-param-data"};
const char* TEST_ERROR_TYPE{"TestErrorType"};
const char* TEST_ERROR_MESSAGE{"Client Error: Test error message."};

INTEG_TEST(ServiceRequestJob, OnSuccessCalled)
{

    using namespace CloudGemFrameworkTest::ServiceApi;

    bool onSuccessCalled = false;
    bool onFailureCalled = false;

    PostTestComplexStringRequestJob* job = PostTestComplexStringRequestJob::Create(
        [&onSuccessCalled](PostTestComplexStringRequestJob* job)
        {
            onSuccessCalled = true;

            ASSERT_EQ(job->result.pathparam, TEST_PATH_PARAM);
            ASSERT_EQ(job->result.queryparam, TEST_QUERY_PARAM_SUCCESS);
            ASSERT_EQ(job->result.bodyparam.data, TEST_BODY_PARAM_DATA);
        },
        [&onFailureCalled](PostTestComplexStringRequestJob* job)
        {
            onFailureCalled = true;
        }
    );

    job->parameters.pathparam = TEST_PATH_PARAM;
    job->parameters.queryparam = TEST_QUERY_PARAM_SUCCESS;
    job->parameters.bodyparam.data = TEST_BODY_PARAM_DATA;
    job->GetHttpRequestJob().StartAndAssistUntilComplete();
    AZ::TickBus::ExecuteQueuedEvents();

    ASSERT_TRUE(onSuccessCalled);
    ASSERT_FALSE(onFailureCalled);

}

INTEG_TEST(ServiceRequestJob, OnFailureCalled)
{

    using namespace CloudGemFrameworkTest::ServiceApi;

    bool onSuccessCalled = false;
    bool onFailureCalled = false;

    PostTestComplexStringRequestJob* job = PostTestComplexStringRequestJob::Create(
        [&onSuccessCalled](PostTestComplexStringRequestJob* job)
        {
            onSuccessCalled = true;
        },
        [&onFailureCalled](PostTestComplexStringRequestJob* job)
        {
            onFailureCalled = true;
            ASSERT_EQ(job->error.type, TEST_ERROR_TYPE);
            ASSERT_EQ(job->error.message, TEST_ERROR_MESSAGE);
        }
    );

    job->parameters.pathparam = TEST_PATH_PARAM;
    job->parameters.queryparam = TEST_QUERY_PARAM_FAILURE;
    job->parameters.bodyparam.data = TEST_BODY_PARAM_DATA;
    job->GetHttpRequestJob().StartAndAssistUntilComplete();
    AZ::TickBus::ExecuteQueuedEvents();

    ASSERT_FALSE(onSuccessCalled);
    ASSERT_TRUE(onFailureCalled);

}

INTEG_TEST(HttpRequestJobTest, OnSuccessCalled)
{
    bool success = false;
    bool failure = false;
    auto job = aznew CloudGemFramework::HttpRequestJob(false, CloudGemFramework::HttpRequestJob::GetDefaultConfig());
    job->SetCallbacks(
        [&success](const AZStd::shared_ptr<CloudGemFramework::HttpRequestJob::Response>& response)
        {
            success = true;
            ASSERT_EQ(response->GetResponseCode(), 200);
        },
        [&failure](const AZStd::shared_ptr<CloudGemFramework::HttpRequestJob::Response>&)
        {
            failure = true;
        }
    );
    job->SetUrl("https://www.amazon.com/");
    job->SetMethod("GET");
    job->StartAndAssistUntilComplete();
    ASSERT_TRUE(success);
    ASSERT_FALSE(failure);
    delete job;
}

INTEG_TEST(HttpRequestJobTest, OnFailureCalled)
{
    bool success = false;
    bool failure = false;
    auto job = aznew CloudGemFramework::HttpRequestJob(false, CloudGemFramework::HttpRequestJob::GetDefaultConfig());
    job->SetCallbacks(
        [&success](const AZStd::shared_ptr<CloudGemFramework::HttpRequestJob::Response>&)
        {
            success = true;
        },
        [&failure](const AZStd::shared_ptr<CloudGemFramework::HttpRequestJob::Response>& response)
        {
            failure = true;
            ASSERT_EQ(response->GetResponseCode(), 404);
        }
    );
    job->SetUrl("https://www.amazon.com/asdf");
    job->SetMethod("GET");
    job->StartAndAssistUntilComplete();
    ASSERT_TRUE(failure);
    ASSERT_FALSE(success);
    delete job;
}
