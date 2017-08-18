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

#ifndef CRYINCLUDE_CRYAISYSTEM_NAVIGATION_NAVIGATIONSYSTEM_VOLUMESMANAGER_H
#define CRYINCLUDE_CRYAISYSTEM_NAVIGATION_NAVIGATIONSYSTEM_VOLUMESMANAGER_H
#pragma once

#include <INavigationSystem.h>

/*
This class is needed to keep track of the navigation shapes created in Sandbox and their
respective IDs. It's necessary to keep consistency with the exported Navigation data
since the AISystem has no knowledge of the Editor's shape
*/

class CVolumesManager
{
public:
    CVolumesManager() {}

    bool RegisterArea(const char* volumeName);
    void UnRegisterArea(const char* volumeName);

    bool SetAreaID(const char* volumeName, NavigationVolumeID id);
    void InvalidateID(NavigationVolumeID id);
    void UpdateNameForAreaID(const NavigationVolumeID id, const char* newName);

    bool IsAreaPresent(const char* volumeName) const;
    NavigationVolumeID GetAreaID(const char* volumeName) const;
    bool GetAreaName(NavigationVolumeID id, string& name) const;

    void GetVolumesNames(std::vector<string>& names) const;

private:
    typedef std::map<string, NavigationVolumeID> VolumesMap;
    VolumesMap m_volumeAreas;
};

#endif // CRYINCLUDE_CRYAISYSTEM_NAVIGATION_NAVIGATIONSYSTEM_VOLUMESMANAGER_H
