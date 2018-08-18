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

#ifndef CRYINCLUDE_CRYAISYSTEM_VALUEHISTORY_H
#define CRYINCLUDE_CRYAISYSTEM_VALUEHISTORY_H
#pragma once

// Simple class to track a history of float values. Assumes positive values.
template <typename T>
class CValueHistory
{
public:
    CValueHistory(unsigned s, float interval)
        : sampleInterval(interval)
        , head(0)
        , size(0)
        , t(0)
        , v(0) { data.resize(s); }

    inline void Reset()
    {
        size = 0;
        head = 0;
        v = 0;
        t = 0;
    }

    inline void Sample(T nv)
    {
        data[head] = nv;
        head = (head + 1) % data.size();
        if (size < data.size())
        {
            size++;
        }
    }

    inline void Sample(T nv, float dt)
    {
        t += dt;
        v = max(v, nv);

        int iter = 0;
        while (t > sampleInterval && iter < 5)
        {
            data[head] = v;
            head = (head + 1) % data.size();
            if (size < data.size())
            {
                size++;
            }
            ++iter;
            t -= sampleInterval;
        }
        if (iter == 5)
        {
            t = 0;
        }
        v = 0;
    }

    inline unsigned GetSampleCount() const
    {
        return size;
    }

    inline unsigned GetMaxSampleCount() const
    {
        return data.size();
    }

    inline T GetSampleInterval() const
    {
        return sampleInterval;
    }

    inline T GetSample(unsigned i) const
    {
        const unsigned n = data.size();
        return data[(head + (n - 1 - i)) % n];
    }

    inline T GetMaxSampleValue() const
    {
        T maxVal = 0;
        const unsigned n = data.size();
        for (unsigned i = 0; i < size; ++i)
        {
            maxVal = max(maxVal, data[(head + (n - 1 - i)) % n]);
        }
        return maxVal;
    };

private:
    std::vector<T> data;
    unsigned head, size;
    T v;
    float t;
    const float sampleInterval;
};

#endif // CRYINCLUDE_CRYAISYSTEM_VALUEHISTORY_H
