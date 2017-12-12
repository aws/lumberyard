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

#include "stdafx.h"
#include <AzToolsFramework/Process/ProcessCommunicator.h>

namespace AzToolsFramework
{
    AZ::u32 ProcessCommunicator::BlockUntilErrorAvailable(AZStd::string& readBuffer)
    {
        // block until errors can actually be read
        ReadError(readBuffer.data(), static_cast<AZ::u32>(0));
        // at this point errors can be read, return the peek amount
        return PeekError();
    }

    AZ::u32 ProcessCommunicator::BlockUntilOutputAvailable(AZStd::string& readBuffer)
    {
        // block until output can actually be read
        ReadOutput(readBuffer.data(), static_cast<AZ::u32>(0));
        // at this point output can be read, return the peek amount
        return PeekOutput();
    }

    AZ::u32 ProcessCommunicatorForChildProcess::BlockUntilInputAvailable(AZStd::string& readBuffer)
    {
        ReadInput(readBuffer.data(), 0);
        return PeekInput();
    }

    StdInOutProcessCommunicator::StdInOutProcessCommunicator()
        : m_stdInWrite(new CommunicatorHandleImpl())
        , m_stdOutRead(new CommunicatorHandleImpl())
        , m_stdErrRead(new CommunicatorHandleImpl())
    {
    }

    StdInOutProcessCommunicator::~StdInOutProcessCommunicator()
    {
        CloseAllHandles();
    }

    bool StdInOutProcessCommunicator::IsValid()
    {
        return m_initialized && (m_stdInWrite->IsValid() || m_stdOutRead->IsValid() || m_stdErrRead->IsValid());
    }

    AZ::u32 StdInOutProcessCommunicator::ReadError(void* readBuffer, AZ::u32 bufferSize)
    {
        AZ_Assert(m_stdErrRead->IsValid(), "Error read handle is invalid, unable to read error stream");
        return ReadDataFromHandle(m_stdErrRead, readBuffer, bufferSize);
    }

    AZ::u32 StdInOutProcessCommunicator::PeekError()
    {
        AZ_Assert(m_stdErrRead->IsValid(), "Error read handle is invalid, unable to read error stream");
        return PeekHandle(m_stdErrRead);
    }

    AZ::u32 StdInOutProcessCommunicator::ReadOutput(void* readBuffer, AZ::u32 bufferSize)
    {
        AZ_Assert(m_stdOutRead->IsValid(), "Output read handle is invalid, unable to read output stream");
        return ReadDataFromHandle(m_stdOutRead, readBuffer, bufferSize);
    }

    AZ::u32 StdInOutProcessCommunicator::PeekOutput()
    {
        AZ_Assert(m_stdOutRead->IsValid(), "Output read handle is invalid, unable to read output stream");
        return PeekHandle(m_stdOutRead);
    }

    AZ::u32 StdInOutProcessCommunicator::WriteInput(const void* writeBuffer, AZ::u32 bytesToWrite)
    {
        AZ_Assert(m_stdInWrite->IsValid(), "Input write handle is invalid, unable to write input stream");
        return WriteDataToHandle(m_stdInWrite, writeBuffer, bytesToWrite);
    }

    void StdInOutProcessCommunicator::CloseAllHandles()
    {
        m_stdInWrite->Close();
        m_stdOutRead->Close();
        m_stdErrRead->Close();
        m_initialized = false;
    }

    StdInOutProcessCommunicatorForChildProcess::StdInOutProcessCommunicatorForChildProcess()
        : m_stdInRead(new CommunicatorHandleImpl())
        , m_stdOutWrite(new CommunicatorHandleImpl())
        , m_stdErrWrite(new CommunicatorHandleImpl())
    {
    }

    StdInOutProcessCommunicatorForChildProcess::~StdInOutProcessCommunicatorForChildProcess()
    {
        CloseAllHandles();
    }

    bool StdInOutProcessCommunicatorForChildProcess::IsValid()
    {
        return m_initialized && (m_stdInRead->IsValid() || m_stdOutWrite->IsValid() || m_stdErrWrite->IsValid());
    }

    AZ::u32 StdInOutProcessCommunicatorForChildProcess::WriteError(const void* writeBuffer, AZ::u32 bytesToWrite)
    {
        return WriteDataToHandle(m_stdErrWrite, writeBuffer, bytesToWrite);
    }

    AZ::u32 StdInOutProcessCommunicatorForChildProcess::WriteOutput(const void* writeBuffer, AZ::u32 bytesToWrite)
    {
        return WriteDataToHandle(m_stdOutWrite, writeBuffer, bytesToWrite);
    }

    AZ::u32 StdInOutProcessCommunicatorForChildProcess::PeekInput()
    {
        return PeekHandle(m_stdInRead);
    }

    AZ::u32 StdInOutProcessCommunicatorForChildProcess::ReadInput(void* buffer, AZ::u32 bufferSize)
    {
        return ReadDataFromHandle(m_stdInRead, buffer, bufferSize);
    }

    void StdInOutProcessCommunicatorForChildProcess::CloseAllHandles()
    {
        m_stdInRead->Close();
        m_stdOutWrite->Close();
        m_stdErrWrite->Close();
        m_initialized = false;
    }
} // namespace AzToolsFramework
