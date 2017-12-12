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

// Description : Base class from which all DirectX devices should derive
//               themselves  Allows access to the concrete IInput implementation
//               used for this platform


#ifndef CRYINCLUDE_CRYINPUT_DXINPUTDEVICE_H
#define CRYINCLUDE_CRYINPUT_DXINPUTDEVICE_H
#pragma once

#ifdef USE_DXINPUT
#include "InputDevice.h"

class CDXInput;

class CDXInputDevice
    : public CInputDevice
{
public:
    CDXInputDevice(CDXInput& input, const char* deviceName, const GUID& guid);
    virtual ~CDXInputDevice();

    //! return reference to the input interface which created the instance of this class
    CDXInput& GetDXInput() const;
    //! return DirectInput device managed by this class
    LPDIRECTINPUTDEVICE8 GetDirectInputDevice() const;

protected:
    // device acquisition
    bool                Acquire();
    bool                Unacquire();
    /*! this is a helper function called from derived devices to allocate
    resources and create a direct input device.

    @param dataformat [in] the direct input data format used to retrieve data from the device
    @param coopLevel [in] the cooperative level to be used for the device
    @param bufSize [in] maximum buffer size used for buffered input
    @return true on success, false otherwise
    */

    bool CreateDirectInputDevice(LPCDIDATAFORMAT dataformat, DWORD coopLevel, DWORD bufSize);

    /*
        void Update(bool bFocus);
    */
protected:
    /*  bool InitDirectInputDevice();


    private:
        virtual void ProcessInput();
        virtual void SetupKeyNames(const std::vector<DIOBJECTDATAFORMAT>& customFormat);
    */

protected:
private:
    CDXInput&                                   m_dxInput;          // pointer to the input interface, which created this device (initialized from constructor)
    LPDIRECTINPUTDEVICE8            m_pDevice;          // pointer to the input device represented by this object (initialized to 0)
    const GUID&                             m_guid;                 // GUID of this device instance
    LPCDIDATAFORMAT                     m_pDataFormat;  // this specifies the dinput specific data format in use
    DWORD                                           m_dwCoopLevel;  // this specifies the direct input cooperation level with other devices

    bool                                            m_bNeedsPoll;       // the device needs polling to fill the input buffers

    /*
        DIDEVICEOBJECTDATA*     m_pData;                    //!< our buffered data is stored here (automatically grown)
        DWORD                                   m_dwDataSize;           //!< the number of entries our buffer can hold
        DWORD                                   m_dwNumData;            //!< the number of entries in the buffer
        DWORD                                   m_dwFlags;              //!< flags used for custom format creation

    private:
        bool                                    m_bCustomFormat;    //!< the device has created its own custom input format, so it has to be released at a later stage
    */
};

#endif //USE_DXINPUT

#endif // CRYINCLUDE_CRYINPUT_DXINPUTDEVICE_H
