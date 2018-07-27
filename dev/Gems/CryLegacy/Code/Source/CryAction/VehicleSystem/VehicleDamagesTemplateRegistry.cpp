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

// Description : Implements a registry for vehicle damages templates


#include "CryLegacy_precompiled.h"
#include "CryAction.h"
#include "Serialization/XMLScriptLoader.h"
#include "IVehicleSystem.h"
#include "Vehicle.h"
#include "VehicleDamagesGroup.h"
#include "VehicleDamagesTemplateRegistry.h"

//------------------------------------------------------------------------
bool CVehicleDamagesTemplateRegistry::Init(const string& defaultDefFilename, const string& damagesTemplatesPath)
{
    if (damagesTemplatesPath.empty())
    {
        return false;
    }

    m_templateFiles.clear();
    m_templates.clear();

    m_defaultDefFilename = defaultDefFilename;

    ICryPak* pCryPak = gEnv->pCryPak;

    _finddata_t fd;
    int ret;
    intptr_t handle;

    if ((handle = pCryPak->FindFirst(damagesTemplatesPath + string("*") + ".xml", &fd)) != -1)
    {
        do
        {
            string name(fd.name);

            if (name.substr(0, 4) != "def_")
            {
                string filename = damagesTemplatesPath + name;

                if (!RegisterTemplates(filename, m_defaultDefFilename))
                {
                    CryLog("VehicleDamagesTemplateRegistry: error parsing template file <%s>.",  filename.c_str());
                }
            }

            ret = pCryPak->FindNext(handle, &fd);
        }
        while (ret >= 0);

        pCryPak->FindClose(handle);
    }

    return true;
}

//------------------------------------------------------------------------
bool CVehicleDamagesTemplateRegistry::RegisterTemplates(const string& filename, const string& defFilename)
{
    XmlNodeRef table = gEnv->pSystem->LoadXmlFromFile(filename);
    if (!table)
    {
        return false;
    }

    m_templateFiles.resize(m_templateFiles.size() + 1);
    STemplateFile& templateFile = m_templateFiles.back();

    templateFile.defFilename = defFilename;
    templateFile.filename = filename;
    templateFile.templateTable = table;

    if (XmlNodeRef damagesGroupsTable = table->findChild("DamagesGroups"))
    {
        int i = 0;
        int c = damagesGroupsTable->getChildCount();

        for (; i < c; i++)
        {
            if (XmlNodeRef damagesGroupTable = damagesGroupsTable->getChild(i))
            {
                string name = damagesGroupTable->getAttr("name");
                if (!name.empty())
                {
                    m_templates.insert(TTemplateMap::value_type(name, damagesGroupTable));
                }
            }
        }
    }

    return true;
}

//------------------------------------------------------------------------
bool CVehicleDamagesTemplateRegistry::UseTemplate(const string& templateName, IVehicleDamagesGroup* pDamagesGroup)
{
    TTemplateMap::iterator ite = m_templates.find(templateName);
    if (ite != m_templates.end())
    {
        CVehicleModificationParams noModifications;
        CVehicleParams tmpVehicleParams(ite->second, noModifications);
        return pDamagesGroup->ParseDamagesGroup(tmpVehicleParams);
    }

    return false;
}


