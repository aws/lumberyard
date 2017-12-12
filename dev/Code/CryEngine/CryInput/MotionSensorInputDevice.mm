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

#include "StdAfx.h"

#include "MotionSensorInputDevice.h"

#import <CoreMotion/CoreMotion.h>

#include <UIKit/UIKit.h>

AllocateConstIntCVar(CMotionSensorInputDevice, CV_i_DebugShowMotionSensorData);

////////////////////////////////////////////////////////////////////////////////////////////////////
inline void AlignWithDisplay(Vec3& o_Vec3, float x, float y, float z, UIInterfaceOrientation displayOrientation)
{
    switch(displayOrientation)
    {
        case UIInterfaceOrientationLandscapeLeft:
        {
            o_Vec3.x = y;
            o_Vec3.y = -z;
            o_Vec3.z = -x;
        }
        break;
        case UIInterfaceOrientationLandscapeRight:
        {
            o_Vec3.x = -y;
            o_Vec3.y = -z;
            o_Vec3.z = x;
        }
        break;
        case UIInterfaceOrientationPortraitUpsideDown:
        {
            o_Vec3.x = -x;
            o_Vec3.y = -z;
            o_Vec3.z = -y;
        }
        break;
        case UIInterfaceOrientationPortrait:
        case UIInterfaceOrientationUnknown:
        default:
        {
            o_Vec3.x = x;
            o_Vec3.y = -z;
            o_Vec3.z = y;
        }
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline void AlignWithDisplay(Quat& o_Quat, float w, float x, float y, float z, UIInterfaceOrientation displayOrientation)
{
    o_Quat.w = w;
    o_Quat.v.x = x;
    o_Quat.v.y = y;
    o_Quat.v.z = z;

    switch(displayOrientation)
    {
        case UIInterfaceOrientationLandscapeLeft:
        {
            o_Quat *= Quat::CreateRotationZ(gf_halfPI);
        }
        break;
        case UIInterfaceOrientationLandscapeRight:
        {
            o_Quat *= Quat::CreateRotationZ(-gf_halfPI);
        }
        break;
        case UIInterfaceOrientationPortraitUpsideDown:
        {
            o_Quat *= Quat::CreateRotationZ(gf_PI);
        }
        break;
        case UIInterfaceOrientationPortrait:
        case UIInterfaceOrientationUnknown:
        default: break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// CMotionSensorInputDevice::Pimpl Declaration (iOS Specific)
////////////////////////////////////////////////////////////////////////////////////////////////////
class CMotionSensorInputDevice::Pimpl
{
public:
    Pimpl(CMotionSensorInputDevice& parent);
    ~Pimpl();

    // Thread safe function to access the latest sensor data from the input thread.
    // Returns a bitmask of all values updated since the last call to this function.
    EMotionSensorFlags RequestLatestSensorData(SMotionSensorData& o_Data);

    bool IsSensorDataAvailable(EMotionSensorFlags sensorFlags) const;
    void RefreshSensors(EMotionSensorFlags activeSensorFlags, float updateIntervalSeconds);

protected:
    bool IsAccelerometerRequired(EMotionSensorFlags sensorFlags) const;
    bool IsGyroscopeRequired(EMotionSensorFlags sensorFlags) const;
    bool IsMagnetometerRequired(EMotionSensorFlags sensorFlags) const;
    bool IsCalibratedMagnetometerRequired(EMotionSensorFlags sensorFlags) const;
    bool IsDeviceMotionRequired(EMotionSensorFlags sensorFlags) const;

    void RefreshAccelerometer(bool shouldBeActive, float updateIntervalSeconds);
    void RefreshGyroscope(bool shouldBeActive, float updateIntervalSeconds);
    void RefreshMagnetometer(bool shouldBeActive, float updateIntervalSeconds);
    void RefreshDeviceMotion(bool shouldBeActive,
                             bool calibratedMagnetometerRequired,
                             float updateIntervalSeconds);

private:
    CMMotionManager* m_motionManager;
    NSOperationQueue* m_operationQueue;

    CMotionSensorInputDevice& m_parent;
    CryMutex m_threadAccessLock;
    SMotionSensorData m_inputThreadSensorData;
    int m_inputThreadUpdateFlags;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// CMotionSensorInputDevice::Pimpl Functions
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
CMotionSensorInputDevice::Pimpl::Pimpl(CMotionSensorInputDevice& parent)
    : m_motionManager(nullptr)
    , m_operationQueue(nullptr)
    , m_parent(parent)
    , m_threadAccessLock()
    , m_inputThreadSensorData()
    , m_inputThreadUpdateFlags(eMSF_None)
{
    m_motionManager = [[CMMotionManager alloc] init];
    m_operationQueue = [[NSOperationQueue alloc] init];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CMotionSensorInputDevice::Pimpl::~Pimpl()
{
    [m_operationQueue release];
    [m_motionManager release];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
EMotionSensorFlags CMotionSensorInputDevice::Pimpl::RequestLatestSensorData(SMotionSensorData& o_Data)
{
    CryAutoLock<CryMutex> autoLock(m_threadAccessLock);
    o_Data = m_inputThreadSensorData;

    const EMotionSensorFlags updateFlags = (EMotionSensorFlags)m_inputThreadUpdateFlags;
    m_inputThreadUpdateFlags = eMSF_None;

    return updateFlags;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CMotionSensorInputDevice::Pimpl::IsSensorDataAvailable(EMotionSensorFlags sensorFlags) const
{
    if (!m_motionManager.accelerometerAvailable && IsAccelerometerRequired(sensorFlags))
    {
        return false;
    }

    if (!m_motionManager.gyroAvailable && IsGyroscopeRequired(sensorFlags))
    {
        return false;
    }

    if (!m_motionManager.magnetometerAvailable && IsMagnetometerRequired(sensorFlags))
    {
        return false;
    }

    if (!m_motionManager.deviceMotionAvailable && IsDeviceMotionRequired(sensorFlags))
    {
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMotionSensorInputDevice::Pimpl::RefreshSensors(EMotionSensorFlags activeSensorFlags,
                                                     float updateIntervalSeconds)
{
    RefreshAccelerometer(IsAccelerometerRequired(activeSensorFlags), updateIntervalSeconds);
    RefreshGyroscope(IsGyroscopeRequired(activeSensorFlags), updateIntervalSeconds);
    RefreshMagnetometer(IsMagnetometerRequired(activeSensorFlags), updateIntervalSeconds);
    RefreshDeviceMotion(IsDeviceMotionRequired(activeSensorFlags),
                        IsCalibratedMagnetometerRequired(activeSensorFlags),
                        updateIntervalSeconds);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CMotionSensorInputDevice::Pimpl::IsAccelerometerRequired(EMotionSensorFlags sensorFlags) const
{
    return sensorFlags & eMSF_AccelerationRaw;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CMotionSensorInputDevice::Pimpl::IsGyroscopeRequired(EMotionSensorFlags sensorFlags) const
{
    return sensorFlags & eMSF_RotationRateRaw;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CMotionSensorInputDevice::Pimpl::IsMagnetometerRequired(EMotionSensorFlags sensorFlags) const
{
    return sensorFlags & eMSF_MagneticFieldRaw;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CMotionSensorInputDevice::Pimpl::IsCalibratedMagnetometerRequired(EMotionSensorFlags sensorFlags) const
{
    return sensorFlags & eMSF_MagneticFieldUnbiased || sensorFlags & eMSF_MagneticNorth;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CMotionSensorInputDevice::Pimpl::IsDeviceMotionRequired(EMotionSensorFlags sensorFlags) const
{
    return sensorFlags & eMSF_AccelerationUser ||
           sensorFlags & eMSF_AccelerationGravity ||
           sensorFlags & eMSF_RotationRateUnbiased ||
           sensorFlags & eMSF_MagneticFieldUnbiased ||
           sensorFlags & eMSF_MagneticNorth ||
           sensorFlags & eMSF_Orientation ||
           sensorFlags & eMSF_OrientationDelta;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMotionSensorInputDevice::Pimpl::RefreshAccelerometer(bool shouldBeActive,
                                                           float updateIntervalSeconds)
{
    if (shouldBeActive == m_motionManager.accelerometerActive)
    {
        return;
    }

    if (!shouldBeActive)
    {
        [m_motionManager stopAccelerometerUpdates];
        return;
    }

    if (!m_motionManager.accelerometerAvailable)
    {
        gEnv->pLog->Log("Accelerometer not available");
        return;
    }

    m_motionManager.accelerometerUpdateInterval = updateIntervalSeconds;

    [m_motionManager startAccelerometerUpdatesToQueue : m_operationQueue withHandler :
    ^(CMAccelerometerData* accelerometerData, NSError* error)
    {
        CryAutoLock<CryMutex> autoLock(m_threadAccessLock);

        m_inputThreadUpdateFlags |= eMSF_AccelerationRaw;
        AlignWithDisplay(m_inputThreadSensorData.accelerationRaw,
                         accelerometerData.acceleration.x,
                         accelerometerData.acceleration.y,
                         accelerometerData.acceleration.z,
                         [[UIApplication sharedApplication] statusBarOrientation]);
    }];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMotionSensorInputDevice::Pimpl::RefreshGyroscope(bool shouldBeActive,
                                                       float updateIntervalSeconds)
{
    if (shouldBeActive == m_motionManager.gyroActive)
    {
        return;
    }

    if (!shouldBeActive)
    {
        [m_motionManager stopGyroUpdates];
        return;
    }

    if (!m_motionManager.gyroAvailable)
    {
        gEnv->pLog->Log("Gyroscope not available");
        return;
    }

    m_motionManager.gyroUpdateInterval = updateIntervalSeconds;

    [m_motionManager startGyroUpdatesToQueue : m_operationQueue withHandler :
    ^(CMGyroData* gyroData, NSError* error)
    {
        CryAutoLock<CryMutex> autoLock(m_threadAccessLock);

        m_inputThreadUpdateFlags |= eMSF_RotationRateRaw;
        AlignWithDisplay(m_inputThreadSensorData.rotationRateRaw,
                         gyroData.rotationRate.x,
                         gyroData.rotationRate.y,
                         gyroData.rotationRate.z,
                         [[UIApplication sharedApplication] statusBarOrientation]);
    }];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMotionSensorInputDevice::Pimpl::RefreshMagnetometer(bool shouldBeActive,
                                                          float updateIntervalSeconds)
{
    if (shouldBeActive == m_motionManager.magnetometerActive)
    {
        return;
    }

    if (!shouldBeActive)
    {
        [m_motionManager stopMagnetometerUpdates];
        return;
    }

    if (!m_motionManager.magnetometerAvailable)
    {
        gEnv->pLog->Log("Magnetometer not available");
        return;
    }

    m_motionManager.magnetometerUpdateInterval = updateIntervalSeconds;

    [m_motionManager startMagnetometerUpdatesToQueue : m_operationQueue withHandler :
    ^(CMMagnetometerData* magnetometerData, NSError* error)
    {
        CryAutoLock<CryMutex> autoLock(m_threadAccessLock);

        m_inputThreadUpdateFlags |= eMSF_MagneticFieldRaw;
        AlignWithDisplay(m_inputThreadSensorData.magneticFieldRaw,
                         magnetometerData.magneticField.x,
                         magnetometerData.magneticField.y,
                         magnetometerData.magneticField.z,
                         [[UIApplication sharedApplication] statusBarOrientation]);
    }];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMotionSensorInputDevice::Pimpl::RefreshDeviceMotion(bool shouldBeActive,
                                                          bool calibratedMagnetometerRequired,
                                                          float updateIntervalSeconds)
{
    if (shouldBeActive == m_motionManager.deviceMotionActive &&
        calibratedMagnetometerRequired == m_motionManager.showsDeviceMovementDisplay)
    {
        return;
    }

    [m_motionManager stopDeviceMotionUpdates];
    if (!shouldBeActive)
    {
        return;
    }

    if (!m_motionManager.deviceMotionAvailable)
    {
        gEnv->pLog->Log("Device Motion not available");
        return;
    }

    m_motionManager.deviceMotionUpdateInterval = updateIntervalSeconds;
    m_motionManager.showsDeviceMovementDisplay = calibratedMagnetometerRequired;

    // Before enabling device motion updates, clear the previous orientation
    // (this may be null) to prevent future calculations using a stale value.
    m_parent.ClearPreviousOrientation();

    // For reasons unknown, calibrated magnetic field values are only returned
    // if 'showsDeviceMovementDisplay == true' and device motion updates are
    // started using a reference frame that actually uses the magnetometer.
    const CMAttitudeReferenceFrame referenceFrame = calibratedMagnetometerRequired ?
                                                    CMAttitudeReferenceFrameXArbitraryCorrectedZVertical :
                                                    CMAttitudeReferenceFrameXArbitraryZVertical; // Uses less battery
    [m_motionManager startDeviceMotionUpdatesUsingReferenceFrame : referenceFrame
                                                                   toQueue : m_operationQueue
                                                                   withHandler :
    ^(CMDeviceMotion* deviceMotionData, NSError* error)
    {
        CryAutoLock<CryMutex> autoLock(m_threadAccessLock);
        const UIInterfaceOrientation currentOrientation = [[UIApplication sharedApplication] statusBarOrientation];

        m_inputThreadUpdateFlags |= eMSF_AccelerationUser;
        AlignWithDisplay(m_inputThreadSensorData.accelerationUser,
                         deviceMotionData.userAcceleration.x,
                         deviceMotionData.userAcceleration.y,
                         deviceMotionData.userAcceleration.z,
                         currentOrientation);

        m_inputThreadUpdateFlags |= eMSF_AccelerationGravity;
        AlignWithDisplay(m_inputThreadSensorData.accelerationGravity,
                         deviceMotionData.gravity.x,
                         deviceMotionData.gravity.y,
                         deviceMotionData.gravity.z,
                         currentOrientation);

        m_inputThreadUpdateFlags |= eMSF_RotationRateUnbiased;
        AlignWithDisplay(m_inputThreadSensorData.rotationRateUnbiased,
                         deviceMotionData.rotationRate.x,
                         deviceMotionData.rotationRate.y,
                         deviceMotionData.rotationRate.z,
                         currentOrientation);

        if (deviceMotionData.magneticField.accuracy != CMMagneticFieldCalibrationAccuracyUncalibrated)
        {
            m_inputThreadUpdateFlags |= eMSF_MagneticFieldUnbiased;
            AlignWithDisplay(m_inputThreadSensorData.magneticFieldUnbiased,
                             deviceMotionData.magneticField.field.x,
                             deviceMotionData.magneticField.field.y,
                             deviceMotionData.magneticField.field.z,
                             currentOrientation);
        }

        m_inputThreadUpdateFlags |= eMSF_Orientation;
        AlignWithDisplay(m_inputThreadSensorData.orientation,
                         deviceMotionData.attitude.quaternion.w,
                         deviceMotionData.attitude.quaternion.x,
                                                deviceMotionData.attitude.quaternion.y,
                                                deviceMotionData.attitude.quaternion.z,
                         currentOrientation);

        // Orientation delta is calculated in the main thread (see MotionSensorInputDevice::Update),
        // otherwise deltas will either be missed or processed multiple times depending on whether
        // the input thread is running faster or slower than the main thread.
    }];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// CMotionSensorInputDevice Functions (iOS Specific)
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
CMotionSensorInputDevice::CMotionSensorInputDevice(IInput& input, IMotionSensorFilter* filter)
    : CInputDevice(input, "Motion Sensor")
    , m_pimpl(nullptr)
    , m_previousOrientation(nullptr)
    , m_mostRecentData()
    , m_motionSensorFilter(filter)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CMotionSensorInputDevice::~CMotionSensorInputDevice()
{
    gEnv->pRenderer->RemoveRenderDebugListener(this);

    delete m_previousOrientation;
    delete m_pimpl;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CMotionSensorInputDevice::Init()
{
    m_pimpl = new Pimpl(*this);

    gEnv->pRenderer->AddRenderDebugListener(this);

    DefineConstIntCVar3("i_DebugShowMotionSensorData",
                        CV_i_DebugShowMotionSensorData,
                        0, VF_CHEAT, "0=off, 1=show motion sensor data on screen");

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CMotionSensorInputDevice::IsSensorDataAvailable(EMotionSensorFlags sensorFlags) const
{
    return m_pimpl ? m_pimpl->IsSensorDataAvailable(sensorFlags) : false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMotionSensorInputDevice::RefreshSensors(EMotionSensorFlags activeSensorFlags,
                                              float updateIntervalSeconds)
{
    if (m_pimpl)
    {
        m_pimpl->RefreshSensors(activeSensorFlags, updateIntervalSeconds);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
EMotionSensorFlags CMotionSensorInputDevice::RequestLatestSensorData(SMotionSensorData& o_Data)
{
    return m_pimpl ? m_pimpl->RequestLatestSensorData(o_Data) : eMSF_None;
}
