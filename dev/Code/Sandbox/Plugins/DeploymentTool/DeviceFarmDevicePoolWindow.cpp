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
#include "DeploymentTool_precompiled.h"

#include "DeviceFarmDevicePoolWindow.h"
#include "ui_DeviceFarmDevicePoolWindow.h"

#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QTimer>

#include <AzCore/std/string/string.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzCore/std/sort.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace DeployTool
{
    // Subclass QTableWidgetItem to allow the selected checkbox to be sorted. 
    class QTableWidgetCheckBoxItem : public QTableWidgetItem
    {
    public:
        bool operator<(const QTableWidgetItem &other) const
        {
            return checkState() < other.checkState();
        }
    };

    DeviceFarmDevicePoolWindow::DeviceFarmDevicePoolWindow(QWidget* parent, std::shared_ptr<DeployTool::DeviceFarmDriver> deviceFarmDriver, const AZStd::string& projectArn, const AZStd::string& platform)
        : QDialog(parent)
        , m_ui(new Ui::DeviceFarmDevicePoolWindow())
        , m_deviceFarmDriver(deviceFarmDriver)
        , mProjectArn(projectArn)
        , mPlatform(platform)
        , m_devicePool("", "")
        , m_deviceTableRefreshTimer(new QTimer(this))
    {
        m_ui->setupUi(this);

        // Setup Device Table Labels
        QStringList headerLabels;
        headerLabels.append("");
        headerLabels.append("Name");
        headerLabels.append("Platform");
        headerLabels.append("OS");
        headerLabels.append("Width");
        headerLabels.append("Height");
        headerLabels.append("Form Factor");
        m_ui->devicesTableWidget->setColumnCount(headerLabels.size());
        m_ui->devicesTableWidget->setHorizontalHeaderLabels(headerLabels);
            
        // Set the name column to stretch and use up remaining space.
        m_ui->devicesTableWidget->horizontalHeader()->setSectionResizeMode(devicesTableColumnSelected, QHeaderView::ResizeToContents);
        m_ui->devicesTableWidget->horizontalHeader()->setSectionResizeMode(devicesTableColumnName, QHeaderView::Stretch);
        m_ui->devicesTableWidget->horizontalHeader()->setSectionResizeMode(devicesTableColumnPlatform, QHeaderView::ResizeToContents);
        m_ui->devicesTableWidget->horizontalHeader()->setSectionResizeMode(devicesTableColumnOS, QHeaderView::ResizeToContents);
        m_ui->devicesTableWidget->horizontalHeader()->setSectionResizeMode(devicesTableColumnWidth, QHeaderView::ResizeToContents);
        m_ui->devicesTableWidget->horizontalHeader()->setSectionResizeMode(devicesTableColumnHeight, QHeaderView::ResizeToContents);
        m_ui->devicesTableWidget->horizontalHeader()->setSectionResizeMode(devicesTableColumnFormFactor, QHeaderView::ResizeToContents);

        // Default to sorting the table by device name.
        m_ui->devicesTableWidget->setSortingEnabled(true);
        m_ui->devicesTableWidget->horizontalHeader()->setSortIndicatorShown(true);
        m_ui->devicesTableWidget->horizontalHeader()->setSortIndicator(devicesTableColumnName, Qt::AscendingOrder);

        // hide the column of numbers on the left side of the table.
        m_ui->devicesTableWidget->verticalHeader()->setVisible(false);

        m_ui->devicesTableWidget->setAlternatingRowColors(true);

        m_ui->devicesTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

        connect(m_ui->formFactorComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &DeviceFarmDevicePoolWindow::OnFormFactorComboBoxValueChanged);
        connect(m_ui->minOSVersionComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &DeviceFarmDevicePoolWindow::OnMinOSVersionComboBoxValueChanged);

        connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(OnAccept()));
        connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

        connect(m_deviceTableRefreshTimer.data(), &QTimer::timeout, this, &DeviceFarmDevicePoolWindow::OnDeviceTableRefreshTimer);
        m_deviceTableRefreshTimer->start(0);
    }

    DeviceFarmDevicePoolWindow::~DeviceFarmDevicePoolWindow()
    {
    }

    DeployTool::DeviceFarmDriver::DevicePool DeviceFarmDevicePoolWindow::GetDevicePoolResult()
    {
        return m_devicePool;
    }

    void DeviceFarmDevicePoolWindow::showEvent(QShowEvent* event)
    {
        QWidget::showEvent(event);

        // Disable the UI until the results are back from the server
        QWidget::setEnabled(false);
        QWidget::blockSignals(true);

        RequestDeviceList();
    }

    void DeviceFarmDevicePoolWindow::OnAccept()
    {
        const AZStd::string poolName = m_ui->poolNameLineEdit->text().toUtf8().data();
        const AZStd::string poolDescription = m_ui->poolDescriptionLineEdit->text().toUtf8().data();

        // Makes sure there is a valid name
        if (poolName.empty())
        {
            QMessageBox::warning(AzToolsFramework::GetActiveWindow(), QObject::tr("Device Farm Pool Error"), QObject::tr("Please enter a valid pool name."));
            return;
        }

        // Make a list of all of the selected devices.
        AZStd::vector<AZStd::string> deviceArns;
        for (int row = 0; row < m_ui->devicesTableWidget->rowCount(); row++)
        {
            if (m_ui->devicesTableWidget->item(row, devicesTableColumnSelected)->checkState() == Qt::Checked)
            {
                deviceArns.push_back(m_ui->devicesTableWidget->item(row, devicesTableColumnName)->data(Qt::UserRole).toString().toUtf8().data());
            }
        }

        // Make sure there is at least one device selected.
        if (deviceArns.size() == 0)
        {
            QMessageBox::warning(AzToolsFramework::GetActiveWindow(), QObject::tr("Device Farm Pool Error"), QObject::tr("A Device Pool must contain at least one device."));
            return;
        }

        bool finished = false;
        bool success = false;
        AZStd::string msg;

        // Create the device farm pool
        m_deviceFarmDriver->CreateDevicePool(
            poolName,
            mProjectArn,
            poolDescription,
            deviceArns,
            [&](bool _success, const AZStd::string& _msg, const DeployTool::DeviceFarmDriver::DevicePool& devicePool)
        {
            success = _success; 
            msg = _msg;
            m_devicePool = devicePool;
            finished = true;
        });

        // for now, just block until we get a result back from the CreateDevicePool request
        while (!finished)
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(100));
        }

        if (success)
        {
            accept();
        }
        else
        {
            QString errorMsg = QStringLiteral("Failed to create Device Pool: '%1'").arg(msg.c_str());
            QMessageBox::warning(AzToolsFramework::GetActiveWindow(), QObject::tr("Device Farm Pool Error"), errorMsg);
        }
    }

    void DeviceFarmDevicePoolWindow::OnFormFactorComboBoxValueChanged()
    {
        RefreshDevicesTable();
    }

    void DeviceFarmDevicePoolWindow::OnMinOSVersionComboBoxValueChanged()
    {
        RefreshDevicesTable();
    }

    void DeviceFarmDevicePoolWindow::RequestDeviceList()
    {
        m_deviceFarmDriver->ListDevices(mProjectArn, [&](bool success, const AZStd::string& msg, AZStd::vector<DeployTool::DeviceFarmDriver::Device>& devices)
        {
            m_allDevices = devices;
            m_deviceTableDirty = true;
        });
    }

    void DeviceFarmDevicePoolWindow::RefreshFilterLists()
    {
        AZStd::vector<AZStd::string> formFactors;
        AZStd::vector<int> majorOsVersions;
        for (auto& device : m_allDevices)
        {
            // Filter out devices that do not match platform
            if (!mPlatform.empty() && mPlatform != device.platform)
            {
                continue;
            }

            // Add the form factor to the list if it was not found.
            if (AZStd::find(formFactors.begin(), formFactors.end(), device.formFactor) == formFactors.end())
            {
                formFactors.push_back(device.formFactor);
            }

            // Add the os version to the list if it was not found.
            int majorOsVersion = GetMajorVersion(device.os);
            if (AZStd::find(majorOsVersions.begin(), majorOsVersions.end(), majorOsVersion) == majorOsVersions.end())
            {
                majorOsVersions.push_back(majorOsVersion);
            }
        }

        // Sort the lists
        AZStd::sort(formFactors.begin(), formFactors.end());
        AZStd::sort(majorOsVersions.begin(), majorOsVersions.end());

        // Populate combo boxes
        m_ui->formFactorComboBox->blockSignals(true);
        m_ui->formFactorComboBox->clear();
        m_ui->formFactorComboBox->addItem("All");
        for (auto& formFactor : formFactors)
        {
            m_ui->formFactorComboBox->addItem(formFactor.c_str());
        }
        m_ui->formFactorComboBox->setCurrentIndex(0);
        m_ui->formFactorComboBox->blockSignals(false);

        m_ui->minOSVersionComboBox->blockSignals(true);
        m_ui->minOSVersionComboBox->clear();
        for (auto& majorOsVersion : majorOsVersions)
        {
            m_ui->minOSVersionComboBox->addItem(QStringLiteral("%1.0").arg(majorOsVersion));
        }
        m_ui->minOSVersionComboBox->setCurrentIndex(0);
        m_ui->minOSVersionComboBox->blockSignals(false);
    }

    void DeviceFarmDevicePoolWindow::RefreshDevicesTable()
    {
        AZStd::unordered_map<AZStd::string, bool> deviceIsVisible;

        AZStd::string formFactorSelection = m_ui->formFactorComboBox->currentText().toUtf8().data();
        int majorOsVersionSelection = GetMajorVersion(m_ui->minOSVersionComboBox->currentText().toUtf8().data());

        // count the number of filtered rows
        int row = 0;
        for (auto device : m_allDevices)
        {
            // Filter out devices that do not match platform
            if (!mPlatform.empty() && mPlatform != device.platform)
            {
                continue;
            }
           
            // Filter out devices that do not match form factor
            if (!AzFramework::StringFunc::Equal(formFactorSelection.c_str(), "All") && 
                !AzFramework::StringFunc::Equal(device.formFactor.c_str(), formFactorSelection.c_str()))
            {
                continue;
            }

            // Filter by min os version
            int majorOsVersion = GetMajorVersion(device.os);
            if (majorOsVersion < majorOsVersionSelection)
            {
                continue;
            }

            deviceIsVisible[device.arn] = true;

            row++;
        }

        m_ui->devicesTableWidget->setSortingEnabled(false);
        m_ui->devicesTableWidget->clearContents();
        m_ui->devicesTableWidget->setRowCount(row);

        row = 0;
        for (auto device : m_allDevices)
        {
            // Skip devices that have been filtered.
            if (deviceIsVisible.find(device.arn) == deviceIsVisible.end())
            {
                continue;
            }

            QTableWidgetCheckBoxItem* checkBoxItem = new QTableWidgetCheckBoxItem();
            checkBoxItem->setCheckState(Qt::Unchecked);
            QTableWidgetItem* nameItem = new QTableWidgetItem;
            nameItem->setText(device.name.c_str());
            nameItem->setData(Qt::UserRole, QString(device.arn.c_str()));
            QTableWidgetItem* platformItem = new QTableWidgetItem(device.platform.c_str());
            QTableWidgetItem* osItem = new QTableWidgetItem(device.os.c_str());;
            QTableWidgetItem* widthItem = new QTableWidgetItem();
            widthItem->setData(Qt::EditRole, device.resolutionWidth);
            QTableWidgetItem* heightItem = new QTableWidgetItem();
            heightItem->setData(Qt::EditRole, device.resolutionHeight);
            QTableWidgetItem* formFactorItem = new QTableWidgetItem(device.formFactor.c_str());
            m_ui->devicesTableWidget->setItem(row, devicesTableColumnSelected, checkBoxItem);
            m_ui->devicesTableWidget->setItem(row, devicesTableColumnPlatform, platformItem);
            m_ui->devicesTableWidget->setItem(row, devicesTableColumnOS, osItem);
            m_ui->devicesTableWidget->setItem(row, devicesTableColumnWidth, widthItem);
            m_ui->devicesTableWidget->setItem(row, devicesTableColumnHeight, heightItem);
            m_ui->devicesTableWidget->setItem(row, devicesTableColumnFormFactor, formFactorItem);
            m_ui->devicesTableWidget->setItem(row, devicesTableColumnName, nameItem);

            row++;
        }

        m_ui->devicesTableWidget->setSortingEnabled(true);
    }

    void DeviceFarmDevicePoolWindow::OnDeviceTableRefreshTimer()
    {
        if (m_deviceTableDirty)
        {
            RefreshFilterLists();
            RefreshDevicesTable();

            // Enable the UI
            QWidget::setEnabled(true);
            QWidget::blockSignals(false);

            m_deviceTableDirty = false;
        }

        m_deviceTableRefreshTimer->start(100);
    }

    int DeviceFarmDevicePoolWindow::GetMajorVersion(const AZStd::string& osVersion)
    {
        AZStd::vector<AZStd::string> tokens;
        AzFramework::StringFunc::Tokenize(osVersion.c_str(), tokens, '.');
        return AzFramework::StringFunc::ToInt(tokens[0].c_str());
    }

} // namespace DeployTool

#include <DeviceFarmDevicePoolWindow.moc>
