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

#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEMODIFICATIONPARAMS_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEMODIFICATIONPARAMS_H
#pragma once


class CVehicleModificationParams
{
public:
    CVehicleModificationParams();
    CVehicleModificationParams(XmlNodeRef xmlVehicleData, const char* modificationName);
    virtual ~CVehicleModificationParams();

    template< typename T >
    void ApplyModification(const char* nodeId, const char* attrName, T& attrValueOut) const
    {
        XmlNodeRef modificationNode = GetModificationNode(nodeId, attrName);
        if (modificationNode)
        {
            modificationNode->getAttr("value", attrValueOut);
        }
    }

private:
    void InitModification(XmlNodeRef xmlModificationData);

    static XmlNodeRef FindModificationNodeByName(const char* name, XmlNodeRef xmlModificationsGroup);

    void InitModificationElem(XmlNodeRef xmlElem);

    virtual XmlNodeRef GetModificationNode(const char* nodeId, const char* attrName) const;

private:
    struct Implementation;
    Implementation* m_pImpl;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEMODIFICATIONPARAMS_H
