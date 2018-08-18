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

// Description : Telemetry file stream.


#include "StdAfx.h"
#include "TelemetryFileStream.h"

namespace Telemetry
{
    const string CFileStream::ms_defaultDir("@log@/Telemetry/"), CFileStream::ms_defaultExtension("tlm");

    CFileStream::CFileStream()
        : m_fileHandle(AZ::IO::InvalidHandle)
    {
    }

    CFileStream::~CFileStream()
    {
        Shutdown();
    }

    bool CFileStream::Init(const string& fileName)
    {
        string  formattedFileName = fileName;

        FormatFileName(formattedFileName);

        m_fileHandle = gEnv->pCryPak->FOpen(formattedFileName.c_str(), "wb");

        return m_fileHandle != AZ::IO::InvalidHandle;
    }

    void CFileStream::Shutdown()
    {
        if (m_fileHandle != AZ::IO::InvalidHandle)
        {
            gEnv->pCryPak->FClose(m_fileHandle);

            m_fileHandle = AZ::IO::InvalidHandle;
        }
    }

    void CFileStream::Release()
    {
        delete this;
    }

    void CFileStream::Write(const uint8* pData, size_t size)
    {
        CRY_ASSERT(pData);

        CRY_ASSERT(size);

        if (m_fileHandle != AZ::IO::InvalidHandle)
        {
            gEnv->pCryPak->FWrite(pData, size, m_fileHandle);

            gEnv->pCryPak->FFlush(m_fileHandle);
        }
    }

    void CFileStream::FormatFileName(string& fileName) const
    {
        size_t  pos = fileName.find("\\/");

        if (pos == string::npos)
        {
            gEnv->pCryPak->MakeDir(ms_defaultDir.c_str());

            fileName.insert(0, ms_defaultDir);
        }

        pos = fileName.find_last_of(".");

        fileName.insert(pos, GetTimeStamp());

        if (pos == string::npos)
        {
            fileName.append(".");

            fileName.append(ms_defaultExtension);
        }
    }

    string CFileStream::GetTimeStamp() const
    {
        time_t  currentTime;

        time(&currentTime);

        char    timeStamp[256];
#ifdef AZ_COMPILER_MSVC
        tm lt;
        localtime_s(&lt, &currentTime);
        strftime(timeStamp, 20, "_%y-%m-%d_%H-%M-%S", &lt);
#else
        auto lt = localtime(&currentTime);
        strftime(timeStamp, 20, "_%y-%m-%d_%H-%M-%S", lt);
#endif

        return string(timeStamp);
    }
}