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

#ifndef CRYINCLUDE_CRYSCRIPTSYSTEM_LUAREMOTEDEBUG_SERIALIZATIONHELPER_H
#define CRYINCLUDE_CRYSCRIPTSYSTEM_LUAREMOTEDEBUG_SERIALIZATIONHELPER_H
#pragma once


//-----------------------------------------------------------------------------
class CSerializationHelper
{
public:
    //-----------------------------------------------------------------------------
    CSerializationHelper(int size)
        : m_pos(0)
        , m_size(size)
        , m_bBufferOverflow(false)
        , m_bWriting(true)
    {
        m_pBuffer = new char[size];
    }

    CSerializationHelper(char* buffer, int size)
        : m_pos(0)
        , m_size(size)
        , m_bBufferOverflow(false)
        , m_bWriting(false)
    {
        m_pBuffer = buffer;
    }

    //-----------------------------------------------------------------------------
    ~CSerializationHelper()
    {
        if (m_bWriting)
        {
            delete [] m_pBuffer;
        }
    }

    //-----------------------------------------------------------------------------
    char* GetBuffer() { return m_pBuffer; }
    int GetUsedSize() const { return m_pos; }
    bool Overflow() const { return m_bBufferOverflow; }
    bool IsWriting() const { return m_bWriting; }
    void Clear() { m_pos = 0; m_bBufferOverflow = false; }

    void ExpandBuffer(int spaceRequired)
    {
        while (spaceRequired > m_size)
        {
            m_size *= 2;
        }
        char* pNewBuffer = new char[m_size];
        memcpy(pNewBuffer, m_pBuffer, m_pos);
        SAFE_DELETE_ARRAY(m_pBuffer);
        m_pBuffer = pNewBuffer;
    }

    //-----------------------------------------------------------------------------
    template <class T>
    void Write(const T& constData)
    {
        CRY_ASSERT(m_bWriting);

        T data = constData;

        if (eBigEndian)
        {
            SwapEndian(data, eBigEndian);       //swap to Big Endian
        }

        if (m_pos + (int)sizeof(T) > m_size)
        {
            ExpandBuffer(m_pos + (int)sizeof(T));
        }

        memcpy(m_pBuffer + m_pos, &data, sizeof(T));
        m_pos += sizeof(T);
    }

    //-----------------------------------------------------------------------------
    void WriteString(const char* pString)
    {
        CRY_ASSERT(m_bWriting);

        int length = strlen(pString);
        // Write the length of the string followed by the string itself
        Write(length);
        if (m_pos + length + 1 > m_size)
        {
            ExpandBuffer(m_pos + length + 1);
        }
        memcpy(m_pBuffer + m_pos, pString, length);
        m_pBuffer[m_pos + length] = '\0';
        m_pos += length + 1;
    }

    //-----------------------------------------------------------------------------
    void WriteBuffer(const char* pData, int length)
    {
        if (m_pos + length > m_size)
        {
            ExpandBuffer(m_pos + length);
        }
        memcpy(m_pBuffer + m_pos, pData, length);
        m_pos += length;
    }

    //-----------------------------------------------------------------------------
    template <class T>
    void Read(T& data)
    {
        CRY_ASSERT(!m_bWriting);

        if (m_pos + (int)sizeof(T) <= m_size)
        {
            memcpy(&data, m_pBuffer + m_pos, sizeof(T));
            m_pos += sizeof(T);
            if (eBigEndian)
            {
                SwapEndian(data, eBigEndian);       //swap to Big Endian
            }
        }
        else
        {
            m_bBufferOverflow = true;
            CRY_ASSERT_MESSAGE(false, "Buffer size is not large enough");
        }
    }

    //-----------------------------------------------------------------------------
    const char* ReadString()
    {
        CRY_ASSERT(!m_bWriting);

        const char* pResult = NULL;
        int length = 0;
        // Read the length of the string followed by the string itself
        Read(length);
        if (m_pos + length <= m_size)
        {
            pResult = m_pBuffer + m_pos;
            m_pos += length + 1;
        }
        else
        {
            m_bBufferOverflow = true;
            CRY_ASSERT_MESSAGE(false, "Buffer size is not large enough");
        }
        return pResult;
    }

private:
    //-----------------------------------------------------------------------------
    char* m_pBuffer;
    int m_pos;
    int m_size;
    bool m_bBufferOverflow;
    bool m_bWriting;
};

#endif // CRYINCLUDE_CRYSCRIPTSYSTEM_LUAREMOTEDEBUG_SERIALIZATIONHELPER_H
