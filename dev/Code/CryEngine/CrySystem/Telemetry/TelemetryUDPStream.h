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

// Description : Telemetry UDP stream.


#ifndef CRYINCLUDE_CRYSYSTEM_TELEMETRY_TELEMETRYUDPSTREAM_H
#define CRYINCLUDE_CRYSYSTEM_TELEMETRY_TELEMETRYUDPSTREAM_H
#pragma once


#ifdef WIN32
#include <winsock2.h>
#endif //WIN32

#include "ITelemetryStream.h"

namespace Telemetry
{
    class CUDPStream
        : public Telemetry::IStream
    {
    public:

        CUDPStream();

        ~CUDPStream();

        bool Init(const string& serverIP);

        void Shutdown();

        void Release();

        void Write(const uint8* pData, size_t size);

    private:

        bool m_initialized;

#ifdef WIN32
        void PrintLastWSAError() const;

        SOCKET          m_outputSocket;

        sockaddr_in m_serverAddress;
#endif //WIN32
    };
}

#endif // CRYINCLUDE_CRYSYSTEM_TELEMETRY_TELEMETRYUDPSTREAM_H
