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

// Description : Equipment Pack (ported from SandBox 1)


#ifndef CRYINCLUDE_EDITOR_EQUIPPACK_H
#define CRYINCLUDE_EDITOR_EQUIPPACK_H
#pragma once

#include <vector>
#include <deque>

struct SEquipment
{
    SEquipment()
    {
        sName = "";
        sType = "";
        sSetup = "";
    }
    bool operator==(const SEquipment& e)
    {
        if (sName != e.sName)
        {
            return false;
        }
        return true;
    }
    bool operator<(const SEquipment& e)
    {
        if (sName < e.sName)
        {
            return true;
        }
        if (sName != e.sName)
        {
            return false;
        }
        return false;
    }
    QString sName;
    QString sType;
    QString sSetup;
};

struct SAmmo
{
    bool operator==(const SAmmo& e)
    {
        if (sName != e.sName)
        {
            return false;
        }
        return true;
    }
    bool operator<(const SAmmo& e)
    {
        if (sName < e.sName)
        {
            return true;
        }
        if (sName != e.sName)
        {
            return false;
        }
        return false;
    }
    QString sName;
    int nAmount;
};

typedef std::deque<SEquipment>      TEquipmentVec;
typedef std::vector<SAmmo>          TAmmoVec;

class CEquipPackLib;

class CEquipPack
{
public:
    void Release();

    const QString& GetName() const { return m_name; }

    bool AddEquip(const SEquipment& equip);
    bool RemoveEquip(const SEquipment& equip);
    bool AddAmmo(const SAmmo& ammo);

    void Clear();
    void Load(XmlNodeRef node);
    bool Save(XmlNodeRef node);
    int  Count() const { return m_equipmentVec.size(); }

    TEquipmentVec& GetEquip() { return m_equipmentVec; }
    TAmmoVec& GetAmmoVec() { return m_ammoVec; }

    void SetModified(bool bModified) { m_bModified = bModified; }
    bool IsModified() const { return m_bModified; }

protected:
    friend CEquipPackLib;

    CEquipPack(CEquipPackLib* pCreator);
    ~CEquipPack();

    void SetName(const QString& name)
    {
        m_name = name;
        SetModified(true);
    }

    bool InternalSetPrimary(const QString& primary);

protected:
    QString m_name;
    CEquipPackLib* m_pCreator;
    TEquipmentVec m_equipmentVec;
    TAmmoVec m_ammoVec;
    bool m_bModified;
};

#endif // CRYINCLUDE_EDITOR_EQUIPPACK_H
