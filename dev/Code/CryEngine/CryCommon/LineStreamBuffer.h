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

#ifndef CRYINCLUDE_CRYCOMMON_LINESTREAMBUFFER_H
#define CRYINCLUDE_CRYCOMMON_LINESTREAMBUFFER_H
#pragma once


class LineStreamBuffer
{
public:
    template <typename T>
    LineStreamBuffer(T* object, void (T::* method)(const char* line))
        : m_charCount(0)
        , m_bTruncated(false)
    {
        m_target = new Target<T>(object, method);
    }

    ~LineStreamBuffer()
    {
        Flush();
        delete m_target;
    }

    void HandleText(const char* text, int length)
    {
        const char* pos = text;
        while (pos - text < length)
        {
            const char* start = pos;

            while (pos - text < length && *pos != '\n' && *pos != '\r')
            {
                ++pos;
            }

            size_t n = pos - start;
            if (m_charCount + n > kMaxCharCount)
            {
                n = kMaxCharCount - m_charCount;
                m_bTruncated = true;
            }
            memcpy(&m_buffer[m_charCount], start, n);
            m_charCount += n;

            if (pos - text < length)
            {
                Flush();
                while (pos - text < length && (*pos == '\n' || *pos == '\r'))
                {
                    ++pos;
                }
            }
        }
    }

    void Flush()
    {
        if (m_charCount > 0)
        {
            m_buffer[m_charCount] = 0;
            m_target->Call(m_buffer);
            m_charCount = 0;
        }
    }

    bool IsTruncated() const
    {
        return m_bTruncated;
    }

private:
    struct ITarget
    {
        virtual ~ITarget() {}
        virtual void Call(const char* line) = 0;
    };
    template <typename T>
    struct Target
        : public ITarget
    {
    public:
        Target(T* object, void (T::* method)(const char* line))
            : object(object)
            , method(method) {}
        virtual void Call(const char* line)
        {
            (object->*method)(line);
        }
    private:
        T* object;
        void (T::* method)(const char* line);
    };

    ITarget* m_target;
    size_t m_charCount;
    static const size_t kMaxCharCount = 2047;
    char m_buffer[kMaxCharCount + 1];
    bool m_bTruncated;
};

#endif // CRYINCLUDE_CRYCOMMON_LINESTREAMBUFFER_H
