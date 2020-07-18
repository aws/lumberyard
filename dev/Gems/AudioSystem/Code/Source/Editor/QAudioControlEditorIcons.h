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

#pragma once

#include <QIcon>
#include <AudioControl.h>

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    inline QIcon GetControlTypeIcon(EACEControlType type)
    {
        switch (type)
        {
        case AudioControls::eACET_TRIGGER:
            return QIcon(":/Editor/Icons/Trigger_Icon.png");
        case AudioControls::eACET_RTPC:
            return QIcon(":/Editor/Icons/RTPC_Icon.png");
        case AudioControls::eACET_SWITCH:
            return QIcon(":/Editor/Icons/Switch_Icon.png");
        case AudioControls::eACET_SWITCH_STATE:
            return QIcon(":/Editor/Icons/Property_Icon.png");
        case AudioControls::eACET_ENVIRONMENT:
            return QIcon(":/Editor/Icons/Environment_Icon.png");
        case AudioControls::eACET_PRELOAD:
            return QIcon(":/Editor/Icons/Bank_Icon.png");
        default:
            // should make a "default"/empty icon...
            return QIcon(":/Editor/Icons/RTPC_Icon.png");
        }
    }

    //-------------------------------------------------------------------------------------------//
    inline QIcon GetFolderIcon()
    {
        return QIcon(":/Editor/Icons/Folder_Icon.png");
    }

    //-------------------------------------------------------------------------------------------//
    inline QIcon GetSoundBankIcon()
    {
        return QIcon(":/Editor/Icons/Preload_Icon.png");
    }

    //-------------------------------------------------------------------------------------------//
    inline QIcon GetGroupIcon(int group)
    {
        const int numberOfGroups = 5;
        group = group % numberOfGroups;
        QString path;
        switch (group)
        {
        case 0:
            path = ":/Editor/Icons/folder purple.svg";
            break;
        case 1:
            path = ":/Editor/Icons/folder blue.svg";
            break;
        case 2:
            path = ":/Editor/Icons/folder green.svg";
            break;
        case 3:
            path = ":/Editor/Icons/folder red.svg";
            break;
        case 4:
            path = ":/Editor/Icons/folder yellow.svg";
            break;
        default:
            path = "Editor/Icons/folder red.svg";
            break;
        }

        QIcon icon;
        icon.addFile(path);
        icon.addFile(path, QSize(), QIcon::Selected);
        return icon;
    }
} // namespace AudioControls
