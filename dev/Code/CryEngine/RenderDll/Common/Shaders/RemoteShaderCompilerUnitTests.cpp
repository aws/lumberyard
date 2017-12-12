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
#include <AzTest/AzTest.h>

#include "RemoteCompiler.h"

namespace NRemoteCompiler
{
    class ShaderSrvUnitTestAccessor
    {
    public:
        ShaderSrvUnitTestAccessor(CShaderSrv& srv)
            : m_srv(srv) { }

        bool EncapsulateRequestInEngineConnectionProtocol(std::vector<uint8>& rCompileData) const
        {
            return m_srv.EncapsulateRequestInEngineConnectionProtocol(rCompileData);
        }
        EServerError SendRequestViaEngineConnection(std::vector<uint8>&  rCompileData) const
        {
            return m_srv.SendRequestViaEngineConnection(rCompileData);
        }

    private:
        CShaderSrv& m_srv;
    };

    // tests the "engine-connection" based shader compiler capability.
    // note that it tests it in isolation, it does not test the connection layer
    // and it does not test existing code present before remote proxying was implemented.
    INTEG_TEST(RemoteShaderCompilerUnitTests, TestRemoteShaderCompilerInterface)
    {
        CShaderSrv& srv = CShaderSrv::Instance();
        ShaderSrvUnitTestAccessor srv_(srv);

        srv.EnableUnitTestingMode(true);

        unsigned short oldPort = gRenDev->CV_r_ShaderCompilerPort;
        gRenDev->CV_r_ShaderCompilerPort = 12345;
        string oldString = gRenDev->CV_r_ShaderCompilerServer->GetString();
        const char* testList = "10.20.30.40";
        gRenDev->CV_r_ShaderCompilerServer->Set(testList);

        // okay, lets give it a go

        std::vector<uint8> testVector;

        std::string testString("_-=!-");

        EXPECT_TRUE(!srv_.EncapsulateRequestInEngineConnectionProtocol(testVector)); // empty vector error condition
        testVector.insert(testVector.begin(), testString.begin(), testString.end());

        EXPECT_TRUE(srv_.EncapsulateRequestInEngineConnectionProtocol(testVector));

        // now test the actual contents to make sure they were packed and follow the protocol.  Unpack it as if you are a client.

        // the payload must be
        // [original payload][null][serverlist][null][port][serverlistlength])
        EXPECT_TRUE(testVector.size() > testString.size() + (sizeof(short) + sizeof(unsigned int) + 2)); // +2 for the 2 nulls

        // find out the length of the serverlist:
        unsigned char* data_end = reinterpret_cast<unsigned char*>(testVector.data()) + testVector.size();
        unsigned int* serverListSizePtr = reinterpret_cast<unsigned int*>(data_end - sizeof(unsigned int));
        unsigned short* serverPortPtr = reinterpret_cast<unsigned short*>(data_end - sizeof(unsigned int) - sizeof(unsigned short));

        // its also expected to be in little endian, and so are the unit tests.

        unsigned int serverListSizeValue = *serverListSizePtr;
        unsigned short serverPortValue = *serverPortPtr;
        EXPECT_TRUE(serverListSizeValue < 4096); // a server list will never be this big - if it is, it means endian swapping was not done right
        EXPECT_TRUE(serverPortValue == 12345);
        char* end_of_serverList = reinterpret_cast<char*>(serverPortPtr) - 1;
        char* beginning_of_serverList = end_of_serverList - serverListSizeValue;
        EXPECT_TRUE(*end_of_serverList == 0);
        EXPECT_TRUE(serverListSizeValue == strlen(testList));

        EXPECT_TRUE(memcmp(beginning_of_serverList, testList, strlen(testList)) == 0);
        EXPECT_TRUE(reinterpret_cast<ptrdiff_t>(end_of_serverList) > reinterpret_cast<ptrdiff_t>(testVector.data())); // make sure its within range!
        // now read the payload
        // there is a null after the server list but before the payload.
        size_t inner_payload_size = static_cast<size_t>(beginning_of_serverList - reinterpret_cast<char*>(testVector.data())) - 1; // -1 for the null

        EXPECT_TRUE(inner_payload_size == 5);
        EXPECT_TRUE(memcmp(testVector.data(), testString.data(), 5) == 0);

        // packing test done!
        testVector.clear();

        // test for empty data - recvfailed expected
        testString = "empty";
        testVector.assign(testString.begin(), testString.end());
        EXPECT_TRUE(srv_.SendRequestViaEngineConnection(testVector) == EServerError::ESRecvFailed);

        // test for incomplete data - recvfailed expected
        testString = "incomplete";
        testVector.assign(testString.begin(), testString.end());
        EXPECT_TRUE(srv_.SendRequestViaEngineConnection(testVector) == EServerError::ESRecvFailed);

        // test for corrupt data - recvfailed expecged
        testString = "corrupt";
        testVector.assign(testString.begin(), testString.end());
        EXPECT_TRUE(srv_.SendRequestViaEngineConnection(testVector) == EServerError::ESRecvFailed);

        // test for an actual compile error - decompressed compile erro rexpected to be attached.
        testString = "compile_failure";
        testVector.assign(testString.begin(), testString.end());
        EXPECT_TRUE(srv_.SendRequestViaEngineConnection(testVector) == EServerError::ESCompileError);
        // validate hte compile erorr decompressed successfully
        const char* expected_decode = "decompressed_plaintext";
        EXPECT_TRUE(testVector.size() == strlen(expected_decode));
        EXPECT_TRUE(memcmp(testVector.data(), expected_decode, strlen(expected_decode)) == 0);

        testString = "success";
        testVector.assign(testString.begin(), testString.end());
        EXPECT_TRUE(srv_.SendRequestViaEngineConnection(testVector) == EServerError::ESOK);
        // validate that the result decompressed successfully - its expected to contain "decompressed_plaintext"
        EXPECT_TRUE(testVector.size() == strlen(expected_decode));
        EXPECT_TRUE(memcmp(testVector.data(), expected_decode, strlen(expected_decode)) == 0);

        gRenDev->CV_r_ShaderCompilerPort = oldPort;
        gRenDev->CV_r_ShaderCompilerServer->Set(oldString);
        srv.EnableUnitTestingMode(false);
    }
}
