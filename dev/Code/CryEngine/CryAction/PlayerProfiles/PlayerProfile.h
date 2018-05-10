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

#ifndef CRYINCLUDE_CRYACTION_PLAYERPROFILES_PLAYERPROFILE_H
#define CRYINCLUDE_CRYACTION_PLAYERPROFILES_PLAYERPROFILE_H
#pragma once

#include <IPlayerProfiles.h>
#include "PlayerProfileManager.h"

class CPlayerProfile
    : public IPlayerProfile
{
public:
    static const char* ATTRIBUTES_TAG; // "Attributes";
    static const char* ACTIONMAPS_TAG; // "ActionMaps";
    static const char* VERSION_TAG; // "Version";

    typedef std::map<string, TFlowInputData, std::less<string>, stl::STLGlobalAllocator<std::pair<const string, TFlowInputData> > > TAttributeMap;

    CPlayerProfile(CPlayerProfileManager* pManager, const char* name, const char* userId, bool bIsPreview = false);
    virtual ~CPlayerProfile();

    // IPlayerProfile
    virtual bool Reset();

    // is this the default profile? it cannot be modified
    virtual bool IsDefault() const;

    // override values with console player profile defaults
    void LoadGamerProfileDefaults();

    // name of the profile
    virtual const char* GetName();

    // Id of the profile user
    virtual const char* GetUserId();

    // retrieve an action map
    virtual IActionMap* GetActionMap(const char* name);

    // set the value of an attribute
    virtual bool SetAttribute(const char* name, const TFlowInputData& value);

    // re-set attribute to default value (basically removes it from this profile)
    virtual bool ResetAttribute(const char* name);

    //delete an attribute from attribute map (regardless if has a default)
    virtual void DeleteAttribute(const char* name);

    // get the value of an attribute. if not specified optionally lookup in default profile
    virtual bool GetAttribute(const char* name, TFlowInputData& val, bool bUseDefaultFallback = true) const;

    // get all attributes available
    // all in this profile and inherited from default profile
    virtual IAttributeEnumeratorPtr CreateAttributeEnumerator();

    // save game stuff
    virtual ISaveGameEnumeratorPtr CreateSaveGameEnumerator();
    virtual ISaveGame* CreateSaveGame();
    virtual ILoadGame* CreateLoadGame();
    virtual bool DeleteSaveGame(const char* name);
    virtual ILevelRotationFile* GetLevelRotationFile(const char* name);
    // ~IPlayerProfile

    bool SerializeXML(CPlayerProfileManager::IProfileXMLSerializer* pSerializer);

    void SetName(const char* name);
    void SetUserId(const char* userId);

    const TAttributeMap& GetAttributeMap() const
    {
        return m_attributeMap;
    }

    const TAttributeMap& GetDefaultAttributeMap() const;

    void GetMemoryStatistics(ICrySizer* s);

protected:
    bool LoadAttributes(const XmlNodeRef& root, int requiredVersion);
    bool SaveAttributes(const XmlNodeRef& root);

    friend class CAttributeEnumerator;

    CPlayerProfileManager* m_pManager;
    string m_name;
    string m_userId;
    TAttributeMap m_attributeMap;
    int m_attributesVersion;
    bool m_bIsPreview;
};

#endif // CRYINCLUDE_CRYACTION_PLAYERPROFILES_PLAYERPROFILE_H

