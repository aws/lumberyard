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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZEBUFFER_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZEBUFFER_H
#pragma once


void inline SaveBuffer(void* pData, int nSize, uchar* pSerialBuffer, int& nSaveBufferPos)
{
    if (pSerialBuffer)
    {
        // set the first 4 bytes of the buffer to the size of the buffer or to 0 if the data isn't available
        *(int*)(pSerialBuffer + nSaveBufferPos) = pData ? nSize : 0;
    }

    nSaveBufferPos += sizeof(int);

    if (pSerialBuffer)
    {
        if (nSize && pData)
        {
            memcpy (pSerialBuffer + nSaveBufferPos, pData, nSize);
        }
    }

    if (pData)
    {
        nSaveBufferPos += nSize;
    }
}

bool inline LoadBuffer(void* pData, uint32 nMaxBytesToLoad, uchar* pSerialBuffer, int& nSaveBufferPos)
{
    int nSize = 0;
    if (nMaxBytesToLoad < 4)
    {
        nSaveBufferPos += 4;
        return false;
    }

    nSize = *(int*)(pSerialBuffer + nSaveBufferPos);
    nSaveBufferPos += 4;

    if ((uint32)nSize > nMaxBytesToLoad)
    {
        return false;
    }

    if (!nSize)
    {
        return true;
    }

    assert(pData);

    if (nSize)
    {
        memcpy (pData, pSerialBuffer + nSaveBufferPos, nSize);
    }

    nSaveBufferPos += nSize;

    return true;
}

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZEBUFFER_H
