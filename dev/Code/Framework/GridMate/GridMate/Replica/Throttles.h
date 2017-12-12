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
#ifndef GM_REPLICA_THROTTLING_H
#define GM_REPLICA_THROTTLING_H

namespace GridMate
{
    //-----------------------------------------------------------------------------
    // BasicThrottle
    // Returns prev == cur
    // Uses operator==
    //-----------------------------------------------------------------------------
    template<typename DataType>
    class BasicThrottle
    {
    public:
        AZ_TYPE_INFO(BasicThrottle, "{394E41BF-D60C-4917-8810-251E2E3D3EAF}", DataType);
        bool WithinThreshold(const DataType& cur) const
        {
            return cur == m_baseline;
        }

        void UpdateBaseline(const DataType& baseline)
        {
            m_baseline = baseline;
        }

    private:
        DataType m_baseline;
    };
    //-----------------------------------------------------------------------------

    //-----------------------------------------------------------------------------
    // EpsilonThrottle
    // Returns (second - first)^2 < e^2;
    //-----------------------------------------------------------------------------
    template<typename T>
    class EpsilonThrottle
    {
        T m_epsilon2;

    public:
        EpsilonThrottle()
            : m_epsilon2(0) { }

        bool WithinThreshold(const T& newValue) const
        {
            T diff = m_baseline - newValue;
            return diff * diff < m_epsilon2;
        }

        void SetBaseline(const T& baseline)
        {
            m_baseline = baseline;
        }

        void UpdateBaseline(const T& baseline)
        {
            m_baseline = baseline;
        }

        void SetThreshold(const T& e)
        {
            m_epsilon2 = e * e;
        }

    private:
        T m_baseline;
    };
    //-----------------------------------------------------------------------------
} // namespace GridMate

#endif // GM_REPLICA_THROTTLING_H
