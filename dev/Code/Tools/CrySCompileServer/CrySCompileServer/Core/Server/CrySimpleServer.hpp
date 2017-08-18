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

#ifndef __CRYSIMPLESERVER__
#define __CRYSIMPLESERVER__

#include <Core/Common.h>
#include <AzCore/std/string/string.h>
#include <string>
#include <vector>

extern bool g_Success;

namespace AZ {
    class JobManager;
}

class CCrySimpleSock;

class SEnviropment
{
public:
    std::string     m_Root;
    std::string     m_Compiler;
    std::string     m_Cache;
    std::string     m_Temp;
    std::string     m_Error;
    std::string     m_Shader;
    std::string     m_Platform;

    std::string   m_FailEMail;
    std::string   m_MailServer;
    uint32_t      m_port;
    uint32_t      m_MailInterval; // seconds since last error to flush error mails

    bool                    m_Caching;
    bool                    m_PrintErrors = 1;
    bool          m_PrintListUpdates;
    bool          m_DedupeErrors;
    bool          m_DumpShaders = 1;
    bool          m_RunAsRoot = false;
    std::string     m_FallbackServer;
    int32_t                 m_FallbackTreshold;
    int32_t       m_MaxConnections;
    std::vector<AZStd::string> m_WhitelistAddresses;

    static SEnviropment&    Instance();
};

class CCrySimpleServer
{
    static volatile AtomicCountType             ms_ExceptionCount;
    CCrySimpleSock*                     m_pServerSocket;
    void            Init();
public:
    CCrySimpleServer(const char* pShaderModel, const char* pDst, const char* pSrc, const char* pEntryFunction);
    CCrySimpleServer();


    static AtomicCountType          GetExceptionCount() { return ms_ExceptionCount; }
    static void                             IncrementExceptionCount();
};

#endif
