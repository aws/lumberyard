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
#ifndef CRYINCLUDE_EDITOR_EQUIPPACKLIB_H
#define CRYINCLUDE_EDITOR_EQUIPPACKLIB_H

#include <map>

class CEquipPack;
typedef std::map<QString, CEquipPack*>   TEquipPackMap;

class CEquipPackLib
{
public:
    CEquipPackLib();
    ~CEquipPackLib();
    CEquipPack* CreateEquipPack(const QString& name);
    // currently we ignore the bDeleteFromDisk.
    // will have to be manually removed via Source control
    bool RemoveEquipPack(const QString& name, bool bDeleteFromDisk = false);
    bool RenameEquipPack(const QString& name, const QString& newName);
    CEquipPack* FindEquipPack(const QString& name);

    void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bResetWhenLoad = true);
    bool LoadLibs(bool bExportToGame);
    bool SaveLibs(bool bExportToGame);
    void ExportToGame();
    void Reset();
    int  Count() const { return m_equipmentPacks.size(); }
    const TEquipPackMap& GetEquipmentPacks() const { return m_equipmentPacks; }
private:
    TEquipPackMap m_equipmentPacks;
};

#endif // CRYINCLUDE_EDITOR_EQUIPPACKLIB_H
