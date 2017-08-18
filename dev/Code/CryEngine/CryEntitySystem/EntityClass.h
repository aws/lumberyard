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

#ifndef CRYINCLUDE_CRYENTITYSYSTEM_ENTITYCLASS_H
#define CRYINCLUDE_CRYENTITYSYSTEM_ENTITYCLASS_H
#pragma once

#include <IEntityClass.h>

//////////////////////////////////////////////////////////////////////////
// Description:
//    Standard implementation of the IEntityClass interface.
//////////////////////////////////////////////////////////////////////////
class CEntityClass
    : public IEntityClass
{
public:
    CEntityClass();
    virtual ~CEntityClass();

    //////////////////////////////////////////////////////////////////////////
    // IEntityClass interface implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void Release() { delete this; };

    virtual uint32 GetFlags() const { return m_nFlags; };
    virtual void SetFlags(uint32 nFlags) { m_nFlags = nFlags; };

    virtual const char* GetName() const { return m_sName.c_str(); }
    virtual const char* GetScriptFile() const { return m_sScriptFile.c_str(); }

    virtual IEntityScript* GetIEntityScript() const { return m_pEntityScript; }
    virtual IScriptTable* GetScriptTable() const;
    virtual bool LoadScript(bool bForceReload);
    virtual ComponentUserCreateFunc GetComponentUserCreateFunc() const { return m_pfnComponentUserCreate; };
    virtual void* GetComponentUserData() const { return m_pComponentUserData; };

    virtual IEntityPropertyHandler* GetPropertyHandler() const;
    virtual IEntityEventHandler* GetEventHandler() const;
    virtual IEntityScriptFileHandler* GetScriptFileHandler() const;

    virtual const SEditorClassInfo& GetEditorClassInfo() const;
    virtual void SetEditorClassInfo(const SEditorClassInfo& editorClassInfo);

    virtual int GetEventCount();
    virtual IEntityClass::SEventInfo GetEventInfo(int nIndex);
    virtual bool FindEventInfo(const char* sEvent, SEventInfo& event);

    virtual TEntityAttributeArray& GetClassAttributes() override;
    virtual const TEntityAttributeArray& GetClassAttributes() const override;
    virtual TEntityAttributeArray& GetEntityAttributes() override;
    virtual const TEntityAttributeArray& GetEntityAttributes() const override;

    //////////////////////////////////////////////////////////////////////////

    void SetName(const char* sName);
    void SetScriptFile(const char* sScriptFile);
    void SetEntityScript(IEntityScript* pScript);

    void SetComponentUserCreateFunc(ComponentUserCreateFunc pFunc, void* pUserData = NULL);
    void SetPropertyHandler(IEntityPropertyHandler* pPropertyHandler);
    void SetEventHandler(IEntityEventHandler* pEventHandler);
    void SetScriptFileHandler(IEntityScriptFileHandler* pScriptFileHandler);
    void SetEntityAttributes(const TEntityAttributeArray& attributes);
    void SetClassAttributes(const TEntityAttributeArray& attributes);

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_sName);
        pSizer->AddObject(m_sScriptFile);
        pSizer->AddObject(m_pEntityScript);
        pSizer->AddObject(m_pPropertyHandler);
        pSizer->AddObject(m_pEventHandler);
        pSizer->AddObject(m_pScriptFileHandler);
    }
private:
    uint32 m_nFlags;
    string m_sName;
    string m_sScriptFile;
    IEntityScript* m_pEntityScript;

    ComponentUserCreateFunc m_pfnComponentUserCreate;
    void* m_pComponentUserData;

    bool m_bScriptLoaded;

    IEntityPropertyHandler* m_pPropertyHandler;
    IEntityEventHandler* m_pEventHandler;
    IEntityScriptFileHandler* m_pScriptFileHandler;

    SEditorClassInfo m_EditorClassInfo;

    TEntityAttributeArray   m_entityAttributes;
    TEntityAttributeArray   m_classAttributes;
};

#endif // CRYINCLUDE_CRYENTITYSYSTEM_ENTITYCLASS_H
