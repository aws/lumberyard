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


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEDAMAGESTEMPLATEREGISTRY_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEDAMAGESTEMPLATEREGISTRY_H
#pragma once

class CVehicleDamagesGroup;

class CVehicleDamagesTemplateRegistry
    : public IVehicleDamagesTemplateRegistry
{
public:

    CVehicleDamagesTemplateRegistry() {}
    virtual ~CVehicleDamagesTemplateRegistry() {}

    virtual bool Init(const string& defaultDefFilename, const string& damagesTemplatesPath);
    virtual void Release() { delete this; }

    virtual bool RegisterTemplates(const string& filename, const string& defFilename);
    virtual bool UseTemplate(const string& templateName, IVehicleDamagesGroup* pDamagesGroup);

protected:

    string m_defaultDefFilename;

    struct STemplateFile
    {
        string filename;
        string defFilename;
        XmlNodeRef templateTable;
    };

    typedef std::vector<STemplateFile> TTemplateFileVector;
    TTemplateFileVector m_templateFiles;

    typedef std::map<string, XmlNodeRef> TTemplateMap;
    TTemplateMap m_templates;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEDAMAGESTEMPLATEREGISTRY_H
