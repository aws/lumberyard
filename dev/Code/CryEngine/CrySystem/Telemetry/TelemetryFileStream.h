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


#ifndef CRYINCLUDE_CRYSYSTEM_TELEMETRY_TELEMETRYFILESTREAM_H
#define CRYINCLUDE_CRYSYSTEM_TELEMETRY_TELEMETRYFILESTREAM_H
#pragma once
#include <AzCore/IO/FileIO.h>


#include "ITelemetryStream.h"

namespace Telemetry
{
    class CFileStream
        : public Telemetry::IStream
    {
    public:

        CFileStream();

        ~CFileStream();

        bool Init(const string& fileName);

        void Shutdown();

        void Release();

        void Write(const uint8* pData, size_t size);

        static const char* ms_defaultDir;
        static const char* ms_defaultExtension;

    private:

        void FormatFileName(string& fileName) const;

        string GetTimeStamp() const;

        AZ::IO::HandleType m_fileHandle;
    };
}

#endif // CRYINCLUDE_CRYSYSTEM_TELEMETRY_TELEMETRYFILESTREAM_H
