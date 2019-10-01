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

#pragma once

#include <QDialog>

#include "DeviceFarmDriver.h"

namespace Ui
{
    class DeviceFarmDevicePoolWindow;
}

namespace DeployTool
{
    // 
    //!  Device Farm Device Pool Window
    /*!
    * Dialog for creating new Device Farm device pools. All of the supported
    * devices are shown in a list so the user can select what devices to add
    * to the new device pool.
    */
    class DeviceFarmDevicePoolWindow
        : public QDialog
    {
        Q_OBJECT

    public:

        DeviceFarmDevicePoolWindow(QWidget* parent, std::shared_ptr<DeployTool::DeviceFarmDriver> deviceFarmDriver, const AZStd::string& projectArn, const AZStd::string& platform);
        virtual ~DeviceFarmDevicePoolWindow();

        // Get the DevicePool result.
        DeployTool::DeviceFarmDriver::DevicePool GetDevicePoolResult();

    protected:

        void showEvent(QShowEvent* event) override;

    public slots:

        void OnAccept();
        void OnFormFactorComboBoxValueChanged();
        void OnMinOSVersionComboBoxValueChanged();

    private:

        void RequestDeviceList();
        void RefreshFilterLists();
        void RefreshDevicesTable();
        void OnDeviceTableRefreshTimer();

        static int GetMajorVersion(const AZStd::string& osVersion);

        QScopedPointer<Ui::DeviceFarmDevicePoolWindow> m_ui;
        std::shared_ptr<DeployTool::DeviceFarmDriver> m_deviceFarmDriver;
        AZStd::string mProjectArn;
        AZStd::string mPlatform;

        // Indices of the columns in the device table.
        const int devicesTableColumnSelected = 0;
        const int devicesTableColumnName = 1;
        const int devicesTableColumnPlatform = 2;
        const int devicesTableColumnOS = 3;
        const int devicesTableColumnWidth = 4;
        const int devicesTableColumnHeight = 5;
        const int devicesTableColumnFormFactor = 6;

        // Used to return results.
        DeployTool::DeviceFarmDriver::DevicePool m_devicePool;

        // Keep a list of all of the devices (unfiltered).
        AZStd::vector<DeployTool::DeviceFarmDriver::Device> m_allDevices;

        // Used to schedule a refresh of the device table ui widget on the main UI thread.
        // (instead of the Device Farm lambda callback)
        QScopedPointer<QTimer> m_deviceTableRefreshTimer;
        bool m_deviceTableDirty = false;
    };

} // namespace DeployTool
