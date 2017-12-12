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
#include "DayCountManager.h"
#include "NetPromoterScoreDialog.h"
#include <QVersionNumber>
#include <AzQtComponents/Utilities/AutoSettingsGroup.h>

// constant variables for updating the registry data
const char* g_netPromoterScore = "NetPromoterScore";
const char* g_startDate = "StartDate";
const char* g_endDate = "EndDate";
const char* g_isReadyToBeRated = "IsRated";
const char* g_dayCount = "DayCount";
const char* g_previousVersionUsed = "PreviousVersionUsed";
const char* g_previousRateStatus = "PreviousRateStatus";
const char* g_previousSecondRateStatus = "PreviousSecondRateStatus";
const char* g_ratingInterval = "RatingInterval";

const char* g_shortTimeInterval = "debug";

DayCountManager::DayCountManager(QObject* parent)
    : QObject(parent)
{
    m_versionNumber = METRICS_VERSION;
    // for debug
    //m_versionNumber = "1.9.0.0";

    // Group NetPromoterScore
    {
        QSettings settings("Amazon", "Lumberyard");
        AzQtComponents::AutoSettingsGroup settingsGroupGuard(&settings, g_netPromoterScore);

        // For debug purposes
        if (!settings.value(g_shortTimeInterval).isNull())
        {
            m_timeIntervalMilliSecond = 15000; // 15 seconds
        }

        startTimer(m_timeIntervalMilliSecond);

        // check if the previous (later) version had been rated yet
        // if it's rated, do nothing
        // else check if the current version exists in the group
        bool isPreviousVersionRated = settings.value(g_previousRateStatus).toBool();

        bool isReadyToBeRated = false;

        if (IsLatestVersion() || !isPreviousVersionRated)
        {
            isReadyToBeRated = UpdateRegistryInfo();

            // This is to prevent the condition when the editor is crashed.
            // It records the previous version of the editor was used
            settings.setValue(g_previousVersionUsed, m_versionNumber);
            settings.setValue(g_previousRateStatus, isReadyToBeRated);
        }
    }
}

bool DayCountManager::UpdateRegistryInfo()
{
    QSettings settings("Amazon", "Lumberyard");
    AzQtComponents::AutoSettingsGroup settingsGroupGuard(&settings, g_netPromoterScore);

    QStringList groupNames = settings.childGroups();

    bool isReadyToBeRated = false;

    // check if the current version exists in the registry
    // Since this feature starts from version 1.9,
    // assuming the version is 1.9 and above
    if (groupNames.contains(m_versionNumber))
    {
        isReadyToBeRated = UpdateDayCountOnStartup();
    }

    else
    {
        isReadyToBeRated = InitializeVersionInfo();
    }

    return isReadyToBeRated;
}

bool DayCountManager::InitializeVersionInfo()
{
    QSettings settings("Amazon", "Lumberyard");
    AzQtComponents::AutoSettingsGroup settingsGroupGuard(&settings, g_netPromoterScore);
    
    settings.setValue(g_previousSecondRateStatus, false);

    // Version group
    AzQtComponents::AutoSettingsGroup versionSettingsGroupGuard(&settings, m_versionNumber);

    QDate currentDate = QDate::currentDate();

    settings.setValue(g_startDate, currentDate);
    settings.setValue(g_endDate, currentDate);

    bool isReadyToBeRated = false;
    settings.setValue(g_isReadyToBeRated, isReadyToBeRated);
    settings.setValue(g_dayCount, 1);

    return isReadyToBeRated;
}

bool DayCountManager::CheckTime()
{
    QSettings settings("Amazon", "Lumberyard");
    AzQtComponents::AutoSettingsGroup settingsGroupGuard(&settings, g_netPromoterScore);

    // Current Version folder
    AzQtComponents::AutoSettingsGroup versionSettingsGroupGuard(&settings, m_versionNumber);

    bool isReadyToBeRated = settings.value(g_isReadyToBeRated).toBool();

    int dayCount = settings.value(g_dayCount).toInt();

    QDate currentDate = QDate::currentDate();

    settings.setValue(g_endDate, currentDate);

    QDate startDateInRegistry = settings.value(g_startDate).toDate();

    int count = startDateInRegistry.daysTo(currentDate);

    // if users change the date backward, the number of count will
    // be negative, so always set to zero if it's negative
    if (count < 0)
    {
        count = 0;
    }

    dayCount += count;

    settings.setValue(g_dayCount, dayCount);
    settings.setValue(g_startDate, currentDate);

    return isReadyToBeRated;
}

bool DayCountManager::UpdateDayCountOnStartup()
{
    QSettings settings("Amazon", "Lumberyard");
    AzQtComponents::AutoSettingsGroup settingsGroupGuard(&settings, g_netPromoterScore);

    // Version group
    AzQtComponents::AutoSettingsGroup versionSettingsGroupGuard(&settings, m_versionNumber);

    // for preventing crashed condition
    bool isReadyToBeRated = settings.value(g_isReadyToBeRated).toBool();

    int dayCount = settings.value(g_dayCount).toInt();

    QDate currentDate = QDate::currentDate();
    QDate prevEndDate = settings.value(g_endDate).toDate();

    if (currentDate > prevEndDate)
    {
        dayCount++;
        settings.setValue(g_dayCount, dayCount);
    }

    // update the start date as the current date
    settings.setValue(g_startDate, currentDate);

    return isReadyToBeRated;
}

bool DayCountManager::IsLatestVersion()
{
    QSettings settings("Amazon", "Lumberyard");
    AzQtComponents::AutoSettingsGroup settingsGroupGuard(&settings, g_netPromoterScore);

    // check if version number is bigger than the current one in the registry
    QString prevVersionUsed = settings.value(g_previousVersionUsed).toString();

    QVersionNumber currentVersion = QVersionNumber::fromString(m_versionNumber);
    QVersionNumber prevVersion = QVersionNumber::fromString(prevVersionUsed);

    return currentVersion >= prevVersion;
}

void DayCountManager::timerEvent(QTimerEvent* ev)
{

    // Group NetPromoterScore
    QSettings settings("Amazon", "Lumberyard");
    AzQtComponents::AutoSettingsGroup settingsGroupGuard(&settings, g_netPromoterScore);

    CheckTime();
}

void DayCountManager::OnUpdatePreviousUsedData()
{
    // Group NetPromoterScore
    QSettings settings("Amazon", "Lumberyard");
    AzQtComponents::AutoSettingsGroup settingsGroupGuard(&settings, g_netPromoterScore);

    settings.setValue(g_previousVersionUsed, m_versionNumber);
    settings.setValue(g_previousRateStatus, true);
}

bool DayCountManager::ShouldShowNetPromoterScoreDialog()
{
    timerEvent(nullptr);

    // Group NetPromoterScore
    QSettings settings("Amazon", "Lumberyard");
    AzQtComponents::AutoSettingsGroup settingsGroupGuard(&settings, g_netPromoterScore);

    bool result = false;
    bool isPreviouslyRated = settings.value(g_previousRateStatus).toBool();
    bool isPreviouslyRatedForSecondPopUp = settings.value(g_previousSecondRateStatus).toBool();
    bool isReadyToBeRated = false;
    bool isReadyToBeRatedSecondTime = false;

    int ratingInterval = 0;

    if (IsLatestVersion())
    {
        // Version group
        AzQtComponents::AutoSettingsGroup settingsGroupGuard(&settings, m_versionNumber);
        int dayCount = settings.value(g_dayCount).toInt();

        if (dayCount >= 5)
        {
            isReadyToBeRated = true;
            settings.setValue(g_isReadyToBeRated, isReadyToBeRated);

            if (dayCount >= 30 && isPreviouslyRated)
            {
                ratingInterval = 30; 
                isReadyToBeRatedSecondTime = true;

            }
            else if (dayCount >= 5)
            {
                ratingInterval = 5;
            }
            
            settings.setValue(g_ratingInterval, ratingInterval);
        }

        result = ((isReadyToBeRatedSecondTime && !isPreviouslyRatedForSecondPopUp) || (isReadyToBeRated && !isPreviouslyRated));
    }

    if (isReadyToBeRatedSecondTime && !isPreviouslyRatedForSecondPopUp)
    {
        settings.setValue(g_previousSecondRateStatus, true);
    }

    return result;
}

int DayCountManager::GetRatingInterval()
{
    QSettings settings("Amazon", "Lumberyard");
    AzQtComponents::AutoSettingsGroup settingsGroupGuard(&settings, g_netPromoterScore);
    AzQtComponents::AutoSettingsGroup versionSettingsGroupGuard(&settings, METRICS_VERSION);

    int ratingInterval = settings.value(g_ratingInterval).toInt();
    return ratingInterval;
}
#include <NetPromoterScore/DayCountManager.moc>
