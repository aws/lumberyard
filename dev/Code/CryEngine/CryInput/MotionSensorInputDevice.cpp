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

#include <AzCore/Debug/Trace.h>

#include <AzCore/Android/JNI/JNI.h>
#include <AzCore/Android/JNI/Object.h>
#include <AzCore/Android/Utils.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>


AllocateConstIntCVar(CMotionSensorInputDevice, CV_i_DebugShowMotionSensorData);

namespace
{
    static const char* refreshMotionSensorsName = "RefreshMotionSensors";
    static const char* isMotionSensorDataAvailableName = "IsMotionSensorDataAvailable";
    static const char* requestLatestMotionSensorDataName = "RequestLatestMotionSensorData";
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// CMotionSensorInputDevice::Pimpl Declaration (Android Specific)
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

private:
    enum ESensorDataPackedIndices
    {
        eSDPI_AccelerationRawUpdated = 0,
        eSDPI_AccelerationRawX = 1,
        eSDPI_AccelerationRawY = 2,
        eSDPI_AccelerationRawZ = 3,
        eSDPI_AccelerationUserUpdated = 4,
        eSDPI_AccelerationUserX = 5,
        eSDPI_AccelerationUserY = 6,
        eSDPI_AccelerationUserZ = 7,
        eSDPI_AccelerationGravityUpdated = 8,
        eSDPI_AccelerationGravityX = 9,
        eSDPI_AccelerationGravityY = 10,
        eSDPI_AccelerationGravityZ = 11,
        eSDPI_RotationRateRawUpdated = 12,
        eSDPI_RotationRateRawX = 13,
        eSDPI_RotationRateRawY = 14,
        eSDPI_RotationRateRawZ = 15,
        eSDPI_RotationRateUnbiasedUpdated = 16,
        eSDPI_RotationRateUnbiasedX = 17,
        eSDPI_RotationRateUnbiasedY = 18,
        eSDPI_RotationRateUnbiasedZ = 19,
        eSDPI_MagneticFieldRawUpdated = 20,
        eSDPI_MagneticFieldRawX = 21,
        eSDPI_MagneticFieldRawY = 22,
        eSDPI_MagneticFieldRawZ = 23,
        eSDPI_MagneticFieldUnbiasedUpdated = 24,
        eSDPI_MagneticFieldUnbiasedX = 25,
        eSDPI_MagneticFieldUnbiasedY = 26,
        eSDPI_MagneticFieldUnbiasedZ = 27,
        eSDPI_OrientationUpdated = 28,
        eSDPI_OrientationX = 29,
        eSDPI_OrientationY = 30,
        eSDPI_OrientationZ = 31,
        eSDPI_OrientationW = 32,
        eSDPI_OrientationAdjustmentRadiansZ = 33,

        // Packed Array Length
        eSDPI_SensorDataPackedLength
    };

    inline bool IsValid() const { return static_cast<bool>(m_motionSensorManager); }


    AZStd::unique_ptr<AZ::Android::JNI::Object> m_motionSensorManager;

    float* m_latestSensorDataPacked;
    int m_latestSensorDataPackedLength;
    SMotionSensorData m_latestSensorData;
    EMotionSensorFlags m_activeSensorFlags;
    CMotionSensorInputDevice& m_parent;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// CMotionSensorInputDevice::Pimpl Functions
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
CMotionSensorInputDevice::Pimpl::Pimpl(CMotionSensorInputDevice& parent)
    : m_motionSensorManager()
    , m_latestSensorDataPacked(nullptr)
    , m_latestSensorDataPackedLength(0)
    , m_latestSensorData()
    , m_activeSensorFlags(eMSF_None)
    , m_parent(parent)
{
    // get the motion sensor manager from the activity
    AZ::Android::JNI::Object activity(AZ::Android::Utils::GetActivityClassRef(), AZ::Android::Utils::GetActivityRef());

    activity.RegisterMethod("GetMotionSensorManager", "()Lcom/amazon/lumberyard/input/MotionSensorManager;");
    jobject motionSensorManagerRef = activity.InvokeObjectMethod<jobject>("GetMotionSensorManager");

    jclass motionSensorManagerClass = AZ::Android::JNI::LoadClass("com/amazon/lumberyard/input/MotionSensorManager");

    // initialize the native motion sensor manager
    m_motionSensorManager.reset(aznew AZ::Android::JNI::Object(motionSensorManagerClass, motionSensorManagerRef, true));

    m_motionSensorManager->RegisterMethod(refreshMotionSensorsName, "(FIIIIIIII)V");
    m_motionSensorManager->RegisterMethod(isMotionSensorDataAvailableName, "(ZZZZZZZZ)Z");
    m_motionSensorManager->RegisterMethod(requestLatestMotionSensorDataName, "()[F");

    m_motionSensorManager->RegisterStaticField("MOTION_SENSOR_DATA_PACKED_LENGTH", "I");
    m_latestSensorDataPackedLength = m_motionSensorManager->GetStaticIntField("MOTION_SENSOR_DATA_PACKED_LENGTH");

    m_latestSensorDataPacked = new float[m_latestSensorDataPackedLength];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CMotionSensorInputDevice::Pimpl::~Pimpl()
{
    delete[] m_latestSensorDataPacked;
    m_motionSensorManager->DestroyInstance();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
EMotionSensorFlags CMotionSensorInputDevice::Pimpl::RequestLatestSensorData(SMotionSensorData& o_Data)
{
    if (!IsValid())
    {
        o_Data = m_latestSensorData;
        return eMSF_None;
    }

    if (m_activeSensorFlags == eMSF_None)
    {
        // Early out to avoid expensive JNI calls.
        o_Data = m_latestSensorData;
        return eMSF_None;
    }

    JNIEnv* jniEnv = AZ::Android::JNI::GetEnv();

    jfloatArray latestSensorData =  m_motionSensorManager->InvokeObjectMethod<jfloatArray>(requestLatestMotionSensorDataName);
    AZ_Assert(jniEnv->GetArrayLength(latestSensorData) == m_latestSensorDataPackedLength,
        "Mismatch in Sensor data requested array size");

    int updatedSensorDataFlags = eMSF_None;
    jniEnv->GetFloatArrayRegion(latestSensorData, 0, m_latestSensorDataPackedLength, m_latestSensorDataPacked);
#ifdef _DEBUG
    if (jniEnv->ExceptionCheck())
    {
        AZ_Warning("MotionSensorInputDevice", false, "Failed to get the extract the latest sensor data");
        HANDLE_JNI_EXCEPTION(jniEnv);

        o_Data = m_latestSensorData;
        return eMSF_None;
    }
#endif // _DEBUG

    // While we would ideally like to encapsulate and return all this info
    // in a Java class, we would then need to 'reach back' through the JNI
    // for each field just to access the raw data. Simply accessing a float
    // array packed with all the required values is far more efficient, at
    // the expense of the native C++ code having to access the data through
    // array indices instead of explicitly named class fields.
    //
    // Additionally, while explicitly calling out the class fields provides
    // a modicum of safety, lots of boilerplate code is needed, and while we
    // may in future append additional sensor data the existing elements are
    // unlikely to ever change. Combined with the above mentioned performance
    // considerations, this approach seems preferable on most fronts.
    AZ_Assert(m_latestSensorDataPackedLength == eSDPI_SensorDataPackedLength,
        "The value of MotionSensorManager::MOTION_SENSOR_DATA_PACKED_LENGTH has changed,\
                        please revise the following array accesses and/or update this assert accordingly.");
    if (m_latestSensorDataPacked[eSDPI_AccelerationRawUpdated])
    {
        m_latestSensorData.accelerationRaw.x = m_latestSensorDataPacked[eSDPI_AccelerationRawX];
        m_latestSensorData.accelerationRaw.y = m_latestSensorDataPacked[eSDPI_AccelerationRawY];
        m_latestSensorData.accelerationRaw.z = m_latestSensorDataPacked[eSDPI_AccelerationRawZ];
        updatedSensorDataFlags |= eMSF_AccelerationRaw;
    }
    if (m_latestSensorDataPacked[eSDPI_AccelerationUserUpdated])
    {
        m_latestSensorData.accelerationUser.x = m_latestSensorDataPacked[eSDPI_AccelerationUserX];
        m_latestSensorData.accelerationUser.y = m_latestSensorDataPacked[eSDPI_AccelerationUserY];
        m_latestSensorData.accelerationUser.z = m_latestSensorDataPacked[eSDPI_AccelerationUserZ];
        updatedSensorDataFlags |= eMSF_AccelerationUser;
    }
    if (m_latestSensorDataPacked[eSDPI_AccelerationGravityUpdated])
    {
        m_latestSensorData.accelerationGravity.x = m_latestSensorDataPacked[eSDPI_AccelerationGravityX];
        m_latestSensorData.accelerationGravity.y = m_latestSensorDataPacked[eSDPI_AccelerationGravityY];
        m_latestSensorData.accelerationGravity.z = m_latestSensorDataPacked[eSDPI_AccelerationGravityZ];
        updatedSensorDataFlags |= eMSF_AccelerationGravity;
    }
    if (m_latestSensorDataPacked[eSDPI_RotationRateRawUpdated])
    {
        m_latestSensorData.rotationRateRaw.x = m_latestSensorDataPacked[eSDPI_RotationRateRawX];
        m_latestSensorData.rotationRateRaw.y = m_latestSensorDataPacked[eSDPI_RotationRateRawY];
        m_latestSensorData.rotationRateRaw.z = m_latestSensorDataPacked[eSDPI_RotationRateRawZ];
        updatedSensorDataFlags |= eMSF_RotationRateRaw;
    }
    if (m_latestSensorDataPacked[eSDPI_RotationRateUnbiasedUpdated])
    {
        m_latestSensorData.rotationRateUnbiased.x = m_latestSensorDataPacked[eSDPI_RotationRateUnbiasedX];
        m_latestSensorData.rotationRateUnbiased.y = m_latestSensorDataPacked[eSDPI_RotationRateUnbiasedY];
        m_latestSensorData.rotationRateUnbiased.z = m_latestSensorDataPacked[eSDPI_RotationRateUnbiasedZ];
        updatedSensorDataFlags |= eMSF_RotationRateUnbiased;
    }
    if (m_latestSensorDataPacked[eSDPI_MagneticFieldRawUpdated])
    {
        m_latestSensorData.magneticFieldRaw.x = m_latestSensorDataPacked[eSDPI_MagneticFieldRawX];
        m_latestSensorData.magneticFieldRaw.y = m_latestSensorDataPacked[eSDPI_MagneticFieldRawY];
        m_latestSensorData.magneticFieldRaw.z = m_latestSensorDataPacked[eSDPI_MagneticFieldRawZ];
        updatedSensorDataFlags |= eMSF_MagneticFieldRaw;
    }
    if (m_latestSensorDataPacked[eSDPI_MagneticFieldUnbiasedUpdated])
    {
        m_latestSensorData.magneticFieldUnbiased.x = m_latestSensorDataPacked[eSDPI_MagneticFieldUnbiasedX];
        m_latestSensorData.magneticFieldUnbiased.y = m_latestSensorDataPacked[eSDPI_MagneticFieldUnbiasedY];
        m_latestSensorData.magneticFieldUnbiased.z = m_latestSensorDataPacked[eSDPI_MagneticFieldUnbiasedZ];
        updatedSensorDataFlags |= eMSF_MagneticFieldUnbiased;
    }
    if (m_latestSensorDataPacked[eSDPI_OrientationUpdated])
    {
        m_latestSensorData.orientation.w = m_latestSensorDataPacked[eSDPI_OrientationW];
        m_latestSensorData.orientation.v.x = m_latestSensorDataPacked[eSDPI_OrientationX];
        m_latestSensorData.orientation.v.y = m_latestSensorDataPacked[eSDPI_OrientationY];
        m_latestSensorData.orientation.v.z = m_latestSensorDataPacked[eSDPI_OrientationZ];

        // Android doesn't provide us with any quaternion math,
        // so we do the alignment here instead of the Java code.
        m_latestSensorData.orientation *= Quat::CreateRotationZ(m_latestSensorDataPacked[eSDPI_OrientationAdjustmentRadiansZ]);
        updatedSensorDataFlags |= eMSF_Orientation;

        // Orientation delta is calculated in the main thread (see MotionSensorInputDevice::Update),
        // otherwise deltas will either be missed or processed multiple times depending on whether
        // the input thread is running faster or slower than the main thread.
    }

    jniEnv->DeleteGlobalRef(latestSensorData);

    o_Data = m_latestSensorData;
    return (EMotionSensorFlags)updatedSensorDataFlags;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int GetUpdatedSensorState(EMotionSensorFlags sensor, int sensorsToUpdate, int activeSensorFlags)
{
    if (!(sensorsToUpdate & sensor))
    {
        return 0; // Unchanged
    }
    else if (activeSensorFlags & sensor)
    {
        return 1; // Enable
    }
    else
    {
        return -1; // Disable
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CMotionSensorInputDevice::Pimpl::IsSensorDataAvailable(EMotionSensorFlags sensorFlags) const
{
    if (!IsValid())
    {
        return false;
    }

    const bool accelerationRaw = sensorFlags & eMSF_AccelerationRaw;
    const bool accelerationUser = sensorFlags & eMSF_AccelerationUser;
    const bool accelerationGravity = sensorFlags & eMSF_AccelerationGravity ||
        sensorFlags & eMSF_MagneticNorth;

    const bool rotationRateRaw = sensorFlags & eMSF_RotationRateRaw;
    const bool rotationRateUnbiased = sensorFlags & eMSF_RotationRateUnbiased;

    const bool magneticFieldRaw = sensorFlags & eMSF_MagneticFieldRaw;
    const bool magneticFieldUnbiased = sensorFlags & eMSF_MagneticFieldUnbiased ||
        sensorFlags & eMSF_MagneticNorth;

    const bool orientation = sensorFlags & eMSF_Orientation ||
        sensorFlags & eMSF_OrientationDelta;

    return m_motionSensorManager->InvokeBooleanMethod(
        isMotionSensorDataAvailableName,
        accelerationRaw,
        accelerationUser,
        accelerationGravity,
        rotationRateRaw,
        rotationRateUnbiased,
        magneticFieldRaw,
        magneticFieldUnbiased,
        orientation);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMotionSensorInputDevice::Pimpl::RefreshSensors(EMotionSensorFlags activeSensorFlags,
    float updateIntervalSeconds)
{
    if (!IsValid())
    {
        return;
    }

    if (activeSensorFlags & eMSF_OrientationDelta)
    {
        // To calculate the orientation delta we need to turn on the orientation sensor.
        activeSensorFlags = static_cast<EMotionSensorFlags>(activeSensorFlags | eMSF_Orientation);
    }

    if (activeSensorFlags & eMSF_MagneticNorth)
    {
        // To calculate magnetic north we need to turn on both the gravity and magnetic field sensors.
        activeSensorFlags = static_cast<EMotionSensorFlags>(activeSensorFlags |
                                                            eMSF_AccelerationGravity |
                                                            eMSF_MagneticFieldUnbiased);
    }

    if (m_activeSensorFlags == activeSensorFlags)
    {
        // Early out to avoid expensive JNI calls.
        return;
    }

    const int sensorsToUpdate = m_activeSensorFlags ^ activeSensorFlags;
    m_activeSensorFlags = activeSensorFlags;

    const int orientationSensorState = GetUpdatedSensorState(eMSF_Orientation, sensorsToUpdate, activeSensorFlags);
    if (orientationSensorState != 0)
    {
        // The orientation sensor is being enabled or disabled, so clear the previous orientation
        // (this may already be null) in order to prevent future calculations using a stale value.
        m_parent.ClearPreviousOrientation();
    }

    m_motionSensorManager->InvokeVoidMethod(
        refreshMotionSensorsName,
        updateIntervalSeconds,
        GetUpdatedSensorState(eMSF_AccelerationRaw, sensorsToUpdate, activeSensorFlags),
        GetUpdatedSensorState(eMSF_AccelerationUser, sensorsToUpdate, activeSensorFlags),
        GetUpdatedSensorState(eMSF_AccelerationGravity, sensorsToUpdate, activeSensorFlags),
        GetUpdatedSensorState(eMSF_RotationRateRaw, sensorsToUpdate, activeSensorFlags),
        GetUpdatedSensorState(eMSF_RotationRateUnbiased, sensorsToUpdate, activeSensorFlags),
        GetUpdatedSensorState(eMSF_MagneticFieldRaw, sensorsToUpdate, activeSensorFlags),
        GetUpdatedSensorState(eMSF_MagneticFieldUnbiased, sensorsToUpdate, activeSensorFlags),
        GetUpdatedSensorState(eMSF_Orientation, sensorsToUpdate, activeSensorFlags));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// CMotionSensorInputDevice Functions (Android Specific)
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
