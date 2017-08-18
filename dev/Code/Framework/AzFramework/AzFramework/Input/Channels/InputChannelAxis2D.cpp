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

#include <AzFramework/Input/Channels/InputChannelAxis2D.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelAxis2D::InputChannelAxis2D(const InputChannelId& inputChannelId,
                                           const InputDevice& inputDevice)
        : InputChannel(inputChannelId, inputDevice)
        , m_axisData2D()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputChannelAxis2D::GetValue() const
    {
        return m_axisData2D.m_values.GetLength();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputChannelAxis2D::GetDelta() const
    {
        return m_axisData2D.m_deltas.GetLength();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputChannel::CustomData* InputChannelAxis2D::GetCustomData() const
    {
        return &m_axisData2D;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelAxis2D::ResetState()
    {
        m_axisData2D = AxisData2D();
        InputChannel::ResetState();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelAxis2D::ProcessRawInputEvent(const AZ::Vector2& rawValues)
    {
        const AZ::Vector2 oldValues = m_axisData2D.m_values;
        m_axisData2D.m_values = rawValues;
        m_axisData2D.m_deltas = rawValues - oldValues;

        const bool isChannelActive = (m_axisData2D.m_values.GetX() != 0.0f) ||
                                     (m_axisData2D.m_values.GetY() != 0.0f);
        UpdateState(isChannelActive);
    }
} // namespace AzFramework
