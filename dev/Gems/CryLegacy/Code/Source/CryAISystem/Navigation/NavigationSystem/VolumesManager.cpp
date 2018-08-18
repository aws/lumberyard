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

#include "CryLegacy_precompiled.h"
#include "VolumesManager.h"

bool CVolumesManager::RegisterArea(const char* volumeName)
{
    VolumesMap::const_iterator it = m_volumeAreas.find(volumeName);
    if (it == m_volumeAreas.end())
    {
        m_volumeAreas[volumeName] = NavigationVolumeID();
    }
    else
    {
        AIWarning("You are trying to register the area %s but it's already registered.", volumeName);
        return false;
    }
    return true;
}

bool CVolumesManager::SetAreaID(const char* volumeName, NavigationVolumeID id)
{
    if (IsAreaPresent(volumeName))
    {
        m_volumeAreas[volumeName] = id;
    }
    else
    {
        AIWarning("The area %s is not registered in the system.", volumeName);
        return false;
    }
    return true;
}

void CVolumesManager::UnRegisterArea(const char* volumeName)
{
    VolumesMap::iterator it = m_volumeAreas.find(volumeName);
    if (it != m_volumeAreas.end())
    {
        m_volumeAreas.erase(it);
    }
}

bool CVolumesManager::IsAreaPresent(const char* volumeName) const
{
    VolumesMap::const_iterator it = m_volumeAreas.find(volumeName);
    return it != m_volumeAreas.end();
}

NavigationVolumeID CVolumesManager::GetAreaID(const char* volumeName) const
{
    NavigationVolumeID areaID;
    VolumesMap::const_iterator it = m_volumeAreas.find(volumeName);
    if (it != m_volumeAreas.end())
    {
        areaID = it->second;
    }
    return areaID;
}

bool CVolumesManager::GetAreaName(NavigationVolumeID id, string& name) const
{
    VolumesMap::const_iterator it = m_volumeAreas.begin();
    VolumesMap::const_iterator end = m_volumeAreas.end();
    for (; it != end; ++it)
    {
        if (it->second == id)
        {
            name = it->first;
            return true;
        }
    }
    return false;
}

void CVolumesManager::UpdateNameForAreaID(const NavigationVolumeID id, const char* newName)
{
    VolumesMap::const_iterator it = m_volumeAreas.begin();
    VolumesMap::const_iterator end = m_volumeAreas.end();
    for (; it != end; ++it)
    {
        if (it->second == id)
        {
            m_volumeAreas.erase(it->first);
            break;
        }
    }
    m_volumeAreas[newName] = id;
}

void CVolumesManager::InvalidateID(NavigationVolumeID id)
{
    VolumesMap::iterator it = m_volumeAreas.begin();
    VolumesMap::iterator end = m_volumeAreas.end();
    for (; it != end; ++it)
    {
        if (it->second == id)
        {
            it->second = NavigationVolumeID();
            return;
        }
    }
    AIWarning("There is no navigation shape with assigned the ID:%d", (uint32) id);
}

void CVolumesManager::GetVolumesNames(std::vector<string>& names) const
{
    names.reserve(m_volumeAreas.size());
    VolumesMap::const_iterator it = m_volumeAreas.begin();
    VolumesMap::const_iterator end = m_volumeAreas.end();
    for (; it != end; ++it)
    {
        names.push_back(it->first);
    }
}