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

#include <QObject>
#include <QSettings>
#include <QDate>

class DayCountManager
    : public QObject
{
    Q_OBJECT

public:
    explicit DayCountManager(QObject* parent);

    bool ShouldShowNetPromoterScoreDialog();
    int GetRatingInterval();

public Q_SLOTS:
    void OnUpdatePreviousUsedData();

private:
    int m_timeIntervalMilliSecond = 7200000; // 2 hours = 7200000 millisecond

    QString m_versionNumber;

    void timerEvent(QTimerEvent* ev);

    bool UpdateRegistryInfo();
    bool InitializeVersionInfo();

    // Will be used when the program starts
    bool UpdateDayCountOnStartup();

    // Helper function to be used for updating the registry every 2 hours (consecutive order)
    bool CheckTime();

    // Helper function to check the if it's the latest version
    bool IsLatestVersion();
};
