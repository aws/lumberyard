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
#ifndef CRYINCLUDE_CRYINPUT_MOTIONSENSORINPUTDEVICE_H
#define CRYINCLUDE_CRYINPUT_MOTIONSENSORINPUTDEVICE_H
#pragma once

#include "InputDevice.h"

#include <IRenderer.h>
#include <LyShine/IDraw2d.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class CMotionSensorInputDevice
    : public CInputDevice
    , public IRenderDebugListener
{
public:
    DeclareStaticConstIntCVar(CV_i_DebugShowMotionSensorData, 0);

    CMotionSensorInputDevice(IInput& input, IMotionSensorFilter* filter = nullptr);
    ~CMotionSensorInputDevice() override;

    int GetDeviceIndex() const override { return 0; }
    bool Init() override;
    inline void Update(bool bFocus) override;
    inline void OnDebugDraw() override;

    bool IsSensorDataAvailable(EMotionSensorFlags sensorFlags) const;
    void RefreshSensors(EMotionSensorFlags activeSensorFlags, float updateIntervalSeconds);

    inline const SMotionSensorData& GetMostRecentData() const { return m_mostRecentData; }
    inline void SetMotionSensorFilter(IMotionSensorFilter* filter) { m_motionSensorFilter = filter; }

private:
    EMotionSensorFlags RequestLatestSensorData(SMotionSensorData& o_Data);
    inline void ClearPreviousOrientation() { SAFE_DELETE(m_previousOrientation); }

    class Pimpl;
    Pimpl* m_pimpl;

    Quat* m_previousOrientation;
    SMotionSensorData m_mostRecentData;
    IMotionSensorFilter* m_motionSensorFilter;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Declared inline and defined in here so it can be used by all platform specific implementations.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CMotionSensorInputDevice::Update(bool bFocus)
{
    EMotionSensorFlags updateFlags = RequestLatestSensorData(m_mostRecentData);
    if (updateFlags == eMSF_None)
    {
        return;
    }

    if ((updateFlags & eMSF_AccelerationGravity) && (updateFlags & eMSF_MagneticFieldUnbiased))
    {
        updateFlags = static_cast<EMotionSensorFlags>(updateFlags | eMSF_MagneticNorth);
        const Vec3& gravity = m_mostRecentData.accelerationGravity.GetNormalized();
        const Vec3& magneticField = m_mostRecentData.magneticFieldUnbiased.GetNormalized();
        const Vec3 east = gravity.Cross(magneticField).GetNormalized();
        m_mostRecentData.magneticNorth = east.Cross(gravity).GetNormalized();
    }

    if (updateFlags & eMSF_Orientation)
    {
        if (m_previousOrientation)
        {
            updateFlags = static_cast<EMotionSensorFlags>(updateFlags | eMSF_OrientationDelta);

            // ToDo: Figure out how to calculate the delta without converting to Euler angles.
            // m_mostRecentData.orientationDelta = m_mostRecentData.orientation * m_previousOrientation->GetInverted();
            m_mostRecentData.orientationDelta = Quat(Ang3(m_mostRecentData.orientation) - Ang3(*m_previousOrientation));

            *m_previousOrientation = m_mostRecentData.orientation;
        }
        else
        {
            m_previousOrientation = new Quat(m_mostRecentData.orientation);
        }
    }

    if (m_motionSensorFilter)
    {
        m_motionSensorFilter->Filter(m_mostRecentData, updateFlags);
    }

    SMotionSensorEvent sensorEvent(m_mostRecentData, updateFlags);
    GetIInput().PostMotionSensorEvent(sensorEvent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Declared inline and defined in here so it can be used by all platform specific implementations.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CMotionSensorInputDevice::OnDebugDraw()
{
#if !defined(_RELEASE)
    if (!CV_i_DebugShowMotionSensorData)
    {
        return;
    }

    char textBuffer[256];
    Draw2dHelper draw2dHelper;
    const float textPointSize = 20.0f;
    const float lineHeight = 20.0f;
    AZ::Vector2 textPosition(5.0f, lineHeight * 10);

    sprintf_s(textBuffer,
        "Acceleration Raw: %f, %f, %f",
        m_mostRecentData.accelerationRaw.x,
        m_mostRecentData.accelerationRaw.y,
        m_mostRecentData.accelerationRaw.z);
    draw2dHelper.DrawText(textBuffer, textPosition, textPointSize);

    textPosition.SetY(textPosition.GetY() + lineHeight);
    sprintf_s(textBuffer,
        "Acceleration User: %f, %f, %f",
        m_mostRecentData.accelerationUser.x,
        m_mostRecentData.accelerationUser.y,
        m_mostRecentData.accelerationUser.z);
    draw2dHelper.DrawText(textBuffer, textPosition, textPointSize);

    textPosition.SetY(textPosition.GetY() + lineHeight);
    sprintf_s(textBuffer,
        "Acceleration Gravity: %f, %f, %f",
        m_mostRecentData.accelerationGravity.x,
        m_mostRecentData.accelerationGravity.y,
        m_mostRecentData.accelerationGravity.z);
    draw2dHelper.DrawText(textBuffer, textPosition, textPointSize);

    textPosition.SetY(textPosition.GetY() + lineHeight);
    sprintf_s(textBuffer,
        "Rotation Raw: %f, %f, %f",
        m_mostRecentData.rotationRateRaw.x,
        m_mostRecentData.rotationRateRaw.y,
        m_mostRecentData.rotationRateRaw.z);
    draw2dHelper.DrawText(textBuffer, textPosition, textPointSize);

    textPosition.SetY(textPosition.GetY() + lineHeight);
    sprintf_s(textBuffer,
        "Rotation Unbiased: %f, %f, %f",
        m_mostRecentData.rotationRateUnbiased.x,
        m_mostRecentData.rotationRateUnbiased.y,
        m_mostRecentData.rotationRateUnbiased.z);
    draw2dHelper.DrawText(textBuffer, textPosition, textPointSize);

    textPosition.SetY(textPosition.GetY() + lineHeight);
    sprintf_s(textBuffer,
        "Magnetic Field Raw: %f, %f, %f",
        m_mostRecentData.magneticFieldRaw.x,
        m_mostRecentData.magneticFieldRaw.y,
        m_mostRecentData.magneticFieldRaw.z);
    draw2dHelper.DrawText(textBuffer, textPosition, textPointSize);

    textPosition.SetY(textPosition.GetY() + lineHeight);
    sprintf_s(textBuffer,
        "Magnetic Field Unbiased: %f, %f, %f",
        m_mostRecentData.magneticFieldUnbiased.x,
        m_mostRecentData.magneticFieldUnbiased.y,
        m_mostRecentData.magneticFieldUnbiased.z);
    draw2dHelper.DrawText(textBuffer, textPosition, textPointSize);

    textPosition.SetY(textPosition.GetY() + lineHeight);
    sprintf_s(textBuffer,
        "Magnetic North: %f, %f, %f",
        m_mostRecentData.magneticNorth.x,
        m_mostRecentData.magneticNorth.y,
        m_mostRecentData.magneticNorth.z);
    draw2dHelper.DrawText(textBuffer, textPosition, textPointSize);

    Ang3 orientationEulerAngles(m_mostRecentData.orientation);
    textPosition.SetY(textPosition.GetY() + lineHeight);
    sprintf_s(textBuffer,
        "Orientation: %f, %f, %f",
        orientationEulerAngles.x,
        orientationEulerAngles.y,
        orientationEulerAngles.z);
    draw2dHelper.DrawText(textBuffer, textPosition, textPointSize);

    Ang3 orientationDeltaEulerAngles(m_mostRecentData.orientationDelta);
    textPosition.SetY(textPosition.GetY() + lineHeight);
    sprintf_s(textBuffer,
        "Orientation Delta: %f, %f, %f",
        orientationDeltaEulerAngles.x,
        orientationDeltaEulerAngles.y,
        orientationDeltaEulerAngles.z);
    draw2dHelper.DrawText(textBuffer, textPosition, textPointSize);
#endif // !defined(_RELEASE)
}

#endif // CRYINCLUDE_CRYINPUT_MOTIONSENSORINPUTDEVICE_H
