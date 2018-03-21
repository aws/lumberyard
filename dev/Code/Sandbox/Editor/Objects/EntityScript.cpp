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

// Description : CEntityScript class implementation.


#include "StdAfx.h"
#include "EntityScript.h"
#include "EntityObject.h"
#include "IconManager.h"

#include <IScriptSystem.h>
#include <IEntitySystem.h>
#include "LuaCommentParser.h"

struct CScriptMethodsDump
    : public IScriptTableDumpSink
{
    QStringList methods;
    QStringList events;

    virtual void OnElementFound(int nIdx, ScriptVarType type) {}

    virtual void OnElementFound(const char* sName, ScriptVarType type)
    {
        if (type == svtFunction)
        {
            if (strncmp(sName, EVENT_PREFIX, 6) == 0)
            {
                events.push_back(sName + 6);
            }
            else
            {
                methods.push_back(sName);
            }
        }
    }
};

enum EScriptParamFlags
{
    SCRIPTPARAM_POSITIVE = 0x01,
};

//////////////////////////////////////////////////////////////////////////
static struct
{
    const char* prefix;
    IVariable::EType type;
    IVariable::EDataType dataType;
    IEntityPropertyHandler::EPropertyType propertyType;
    int flags;
    bool bExactName;
} s_paramTypes[] =
{
    { "n",              IVariable::INT,         IVariable::DT_SIMPLE, IEntityPropertyHandler::Int, SCRIPTPARAM_POSITIVE, false },
    { "i",              IVariable::INT,         IVariable::DT_SIMPLE, IEntityPropertyHandler::Int, 0, false },
    { "b",              IVariable::BOOL,        IVariable::DT_SIMPLE, IEntityPropertyHandler::Bool, 0, false },
    { "f",              IVariable::FLOAT,       IVariable::DT_SIMPLE, IEntityPropertyHandler::Float, 0, false },
    { "s",              IVariable::STRING,  IVariable::DT_SIMPLE, IEntityPropertyHandler::String, 0, false },

    { "ei",             IVariable::INT,         IVariable::DT_UIENUM, IEntityPropertyHandler::Int, 0, false },
    { "es",             IVariable::STRING,  IVariable::DT_UIENUM, IEntityPropertyHandler::String, 0, false },

    { "shader",     IVariable::STRING,  IVariable::DT_SHADER, IEntityPropertyHandler::String, 0, false },
    { "clr",            IVariable::VECTOR,  IVariable::DT_COLOR, IEntityPropertyHandler::Vector, 0, false },
    { "color",      IVariable::VECTOR,  IVariable::DT_COLOR, IEntityPropertyHandler::Vector, 0, false },

    { "vector",     IVariable::VECTOR,  IVariable::DT_SIMPLE, IEntityPropertyHandler::Vector, 0, false },

    { "tex",            IVariable::STRING,  IVariable::DT_TEXTURE, IEntityPropertyHandler::String, 0, false },
    { "texture",    IVariable::STRING,  IVariable::DT_TEXTURE, IEntityPropertyHandler::String, 0, false },

    { "obj",            IVariable::STRING,  IVariable::DT_OBJECT, IEntityPropertyHandler::String, 0, false },
    { "object",     IVariable::STRING,  IVariable::DT_OBJECT, IEntityPropertyHandler::String, 0, false },

    { "file",           IVariable::STRING,  IVariable::DT_FILE, IEntityPropertyHandler::String, 0, false },
    { "aibehavior", IVariable::STRING, IVariable::DT_AI_BEHAVIOR, IEntityPropertyHandler::String, 0, false },
#ifdef USE_DEPRECATED_AI_CHARACTER_SYSTEM
    { "aicharacter", IVariable::STRING, IVariable::DT_AI_CHARACTER, IEntityPropertyHandler::String, 0, false },
#endif
    { "aipfpropertieslist", IVariable::STRING, IVariable::DT_AI_PFPROPERTIESLIST, IEntityPropertyHandler::String, 0, false },
    { "aientityclasses", IVariable::STRING, IVariable::DT_AIENTITYCLASSES, IEntityPropertyHandler::String, 0, false },
    { "aiterritory", IVariable::STRING, IVariable::DT_AITERRITORY, IEntityPropertyHandler::String, 0, false },
    { "aiwave",   IVariable::STRING,  IVariable::DT_AIWAVE, IEntityPropertyHandler::String, 0, false },

    { "text",           IVariable::STRING,  IVariable::DT_LOCAL_STRING, IEntityPropertyHandler::String, 0, false },
    { "equip",      IVariable::STRING,  IVariable::DT_EQUIP, IEntityPropertyHandler::String, 0, false },
    { "reverbpreset", IVariable::STRING,    IVariable::DT_REVERBPRESET, IEntityPropertyHandler::String, 0, false },
    { "eaxpreset", IVariable::STRING,   IVariable::DT_REVERBPRESET, IEntityPropertyHandler::String, 0, false },

    { "aianchor",   IVariable::STRING,  IVariable::DT_AI_ANCHOR, IEntityPropertyHandler::String, 0, false },

    { "soclass",    IVariable::STRING,  IVariable::DT_SOCLASS, IEntityPropertyHandler::String, 0, false },
    { "soclasses",  IVariable::STRING,  IVariable::DT_SOCLASSES, IEntityPropertyHandler::String, 0, false },
    { "sostate",    IVariable::STRING,  IVariable::DT_SOSTATE, IEntityPropertyHandler::String, 0, false },
    { "sostates",   IVariable::STRING,  IVariable::DT_SOSTATES, IEntityPropertyHandler::String, 0, false },
    { "sopattern",  IVariable::STRING,  IVariable::DT_SOSTATEPATTERN, IEntityPropertyHandler::String, 0, false },
    { "soaction",   IVariable::STRING,  IVariable::DT_SOACTION, IEntityPropertyHandler::String, 0, false },
    { "sohelper",   IVariable::STRING,  IVariable::DT_SOHELPER, IEntityPropertyHandler::String, 0, false },
    { "sonavhelper", IVariable::STRING, IVariable::DT_SONAVHELPER, IEntityPropertyHandler::String, 0, false },
    { "soanimhelper", IVariable::STRING,    IVariable::DT_SOANIMHELPER, IEntityPropertyHandler::String, 0, false },
    { "soevent",    IVariable::STRING,  IVariable::DT_SOEVENT, IEntityPropertyHandler::String, 0, false },
    { "sotemplate", IVariable::STRING,  IVariable::DT_SOTEMPLATE, IEntityPropertyHandler::String, 0, false },
    { "customaction",   IVariable::STRING,  IVariable::DT_CUSTOMACTION, IEntityPropertyHandler::String, 0, false },
    { "gametoken", IVariable::STRING, IVariable::DT_GAMETOKEN, IEntityPropertyHandler::String, 0, false },
    { "seq_",      IVariable::STRING, IVariable::DT_SEQUENCE, IEntityPropertyHandler::String, 0, false },
    { "mission_", IVariable::STRING, IVariable::DT_MISSIONOBJ, IEntityPropertyHandler::String, 0, false },
    { "seqid_",     IVariable::INT,         IVariable::DT_SEQUENCE_ID, IEntityPropertyHandler::String, 0, false },
    { "lightanimation_", IVariable::STRING, IVariable::DT_LIGHT_ANIMATION, IEntityPropertyHandler::String, 0, false },
    { "flare_", IVariable::STRING, IVariable::DT_FLARE, IEntityPropertyHandler::String, 0, false },
    { "ParticleEffect", IVariable::STRING, IVariable::DT_PARTICLE_EFFECT, IEntityPropertyHandler::String, 0, true },
    { "geomcache", IVariable::STRING, IVariable::DT_GEOM_CACHE, IEntityPropertyHandler::String, 0, false },
    { "material", IVariable::STRING, IVariable::DT_MATERIAL, IEntityPropertyHandler::String, 0 },
    { "audioTrigger", IVariable::STRING, IVariable::DT_AUDIO_TRIGGER, IEntityPropertyHandler::String, 0 },
    { "audioSwitch", IVariable::STRING, IVariable::DT_AUDIO_SWITCH, IEntityPropertyHandler::String, 0 },
    { "audioSwitchState", IVariable::STRING, IVariable::DT_AUDIO_SWITCH_STATE, IEntityPropertyHandler::String, 0 },
    { "audioRTPC", IVariable::STRING, IVariable::DT_AUDIO_RTPC, IEntityPropertyHandler::String, 0 },
    { "audioEnvironment", IVariable::STRING, IVariable::DT_AUDIO_ENVIRONMENT, IEntityPropertyHandler::String, 0 },
    { "audioPreloadRequest", IVariable::STRING, IVariable::DT_AUDIO_PRELOAD_REQUEST, IEntityPropertyHandler::String, 0 },
};

int s_propertyDataTypeCount = sizeof(s_paramTypes) / sizeof(s_paramTypes[0]);

class PropertyVarBlockBuilder
{
public:
    PropertyVarBlockBuilder(CVarBlock* varBlock, IEntityPropertyHandler* propertyHandler,
        uint32* currentProperty, IVariable* folder = 0)
        : m_varBlock(varBlock)
        , m_propertyHandler(propertyHandler)
        , m_currentProperty(*currentProperty)
        , m_folder(folder)
    {
        assert(m_varBlock);
        assert(m_propertyHandler);
    }

    void operator()()
    {
        int propertyCount = m_propertyHandler->GetPropertyCount();
        assert(m_currentProperty < propertyCount);

        while (m_currentProperty < propertyCount)
        {
            int varId = m_currentProperty++;

            IEntityPropertyHandler::SPropertyInfo property;
            m_propertyHandler->GetPropertyInfo(varId, property);

            switch (property.type)
            {
            case IEntityPropertyHandler::FolderEnd:
                return;
            case IEntityPropertyHandler::FolderBegin:
            {
                IVariable* folder = CreateFolder();
                folder->SetName(property.name);
                folder->SetDescription(property.description);

                PropertyVarBlockBuilder childBuilder(m_varBlock, m_propertyHandler, &m_currentProperty, folder);
                childBuilder();

                AddVar(folder);
            }
            break;
            default:
            {
                IVariable* var = CreateVar(property, varId);
                if (var)
                {
                    AddVar(var);
                }
            }
            }
        }
    }

    void AddVar(IVariable* var)
    {
        if (m_folder)
        {
            m_folder->AddVariable(var);
        }
        else
        {
            m_varBlock->AddVariable(var);
        }
    }

    IVariable* CreateVarByType(IEntityPropertyHandler::EPropertyType type)
    {
        switch (type)
        {
        case IEntityPropertyHandler::Bool:
            return new CVariable<bool>();
        case IEntityPropertyHandler::Int:
            return new CVariable<int>();
        case IEntityPropertyHandler::Float:
            return new CVariable<float>();
        case IEntityPropertyHandler::String:
            return new CVariable<QString>();
        case IEntityPropertyHandler::Vector:
            return new CVariable<Vec3>();
        //case IEntityClass::PROP_QUAT: return new CVariable<Quat>;
        default:
            assert(0);
        }
        return 0;
    }

    IVariable* CreateFolder()
    {
        return new CVariableArray();
    }

    IVariable::EDataType GetDataType(IEntityPropertyHandler::SPropertyInfo& info)
    {
        IVariable::EDataType dataType = IVariable::DT_SIMPLE;

        if (info.flags & IEntityPropertyHandler::ePropertyFlag_UIEnum)
        {
            dataType = IVariable::DT_UIENUM;
        }
        else
        {
            for (int i = 0; i < s_propertyDataTypeCount; ++i)
            {
                if (info.type == s_paramTypes[i].propertyType)
                {
                    if (!info.editType)
                    {
                        continue;
                    }

                    if (!_stricmp(info.editType, s_paramTypes[i].prefix))
                    {
                        dataType = s_paramTypes[i].dataType;
                        break;
                    }
                }
            }
        }

        return dataType;
    }

    IVariable* CreateVar(IEntityPropertyHandler::SPropertyInfo& info, int id)
    {
        IVariable* var = CreateVarByType(info.type);
        if (!var)
        {
            return 0;
        }

        var->SetName(info.name);
        var->SetHumanName(info.name);
        var->SetDescription(info.description);
        var->SetDataType(GetDataType(info));
        var->SetUserData(id);

        if (info.flags & IEntityPropertyHandler::ePropertyFlag_Unsorted)
        {
            var->SetFlags(var->GetFlags() | IVariable::UI_UNSORTED);
        }

        switch (info.type)
        {
        case IEntityPropertyHandler::Vector:
        case IEntityPropertyHandler::Float:
        case IEntityPropertyHandler::Int:
            var->SetLimits(info.limits.min, info.limits.max, 0.f, true, true);
            break;
        case IEntityPropertyHandler::String:
        case IEntityPropertyHandler::Bool:
            break;
        default:
            assert(0);
            return 0;
        }

        return var;
    }

private:
    CVarBlock* m_varBlock;
    IEntityPropertyHandler* m_propertyHandler;
    IVariable* m_folder;
    uint32& m_currentProperty;
};


class PropertyVarBlockValueSetter
{
public:
    PropertyVarBlockValueSetter(CVarBlock* varBlock, IEntity* entity, IEntityPropertyHandler* propertyHandler)
        : m_varBlock(varBlock)
        , m_entity(entity)
        , m_propertyHandler(propertyHandler)
    {
        assert(m_varBlock);
        assert(m_propertyHandler);
    }

    void operator()()
    {
        int varCount = m_varBlock->GetNumVariables();

        for (int i = 0; i < varCount; ++i)
        {
            SetVar(m_varBlock->GetVariable(i));
        }

        m_propertyHandler->PropertiesChanged(m_entity);
    }

    void SetVar(IVariable* var)
    {
        if (var->GetType() == IVariable::ARRAY)
        {
            int childCount = var->GetNumVariables();
            for (int i = 0; i < childCount; ++i)
            {
                SetVar(var->GetVariable(i));
            }
        }
        else
        {
            QString value;
            var->Get(value);

            m_propertyHandler->SetProperty(m_entity, var->GetUserData().toInt(), value.toUtf8().data());
        }
    }

private:
    CVarBlock* m_varBlock;
    IEntity* m_entity;
    IEntityPropertyHandler* m_propertyHandler;
};


class PropertyVarBlockValueGetter
{
public:
    PropertyVarBlockValueGetter(CVarBlock* varBlock, IEntity* entity, IEntityPropertyHandler* propertyHandler)
        : m_varBlock(varBlock)
        , m_entity(entity)
        , m_propertyHandler(propertyHandler)
    {
        assert(m_varBlock);
        assert(m_propertyHandler);
    }

    void operator()()
    {
        int varCount = m_varBlock->GetNumVariables();

        for (int i = 0; i < varCount; ++i)
        {
            GetVar(m_varBlock->GetVariable(i));
        }
    }

    void GetVar(IVariable* var)
    {
        if (var->GetType() == IVariable::ARRAY)
        {
            int childCount = var->GetNumVariables();
            for (int i = 0; i < childCount; ++i)
            {
                GetVar(var->GetVariable(i));
            }
        }
        else
        {
            const char* value = 0;

            if (m_entity)
            {
                value = m_propertyHandler->GetProperty(m_entity, var->GetUserData().toInt());
            }
            else
            {
                value = m_propertyHandler->GetDefaultProperty(var->GetUserData().toInt());
            }

            var->Set(value);
        }
    }

private:
    CVarBlock* m_varBlock;
    IEntity* m_entity;
    IEntityPropertyHandler* m_propertyHandler;
};

//////////////////////////////////////////////////////////////////////////
struct CScriptPropertiesDump
    : public IScriptTableDumpSink
{
private:
    struct Variable
    {
        QString name;
        ScriptVarType type;
    };

    std::vector<Variable> m_elements;

    CVarBlock* m_varBlock;
    IVariable* m_parentVar;

public:
    explicit CScriptPropertiesDump(CVarBlock* pVarBlock, IVariable* pParentVar = NULL)
    {
        m_varBlock = pVarBlock;
        m_parentVar = pParentVar;
    }

    //////////////////////////////////////////////////////////////////////////
    inline bool IsPropertyTypeMatch(const char* type, const char* name, int nameLen, bool exactName)
    {
        if (exactName)
        {
            return strcmp(type, name) == 0;
        }

        // if starts from capital no type encoded.
        int typeLen = strlen(type);
        if (typeLen < nameLen && name[0] == tolower(name[0]))
        {
            // After type name Must follow Upper case or _.
            if (name[typeLen] != tolower(name[typeLen]) || name[typeLen] == '_')
            {
                if (strncmp(name, type, strlen(type)) == 0)
                {
                    return true;
                }
            }
        }
        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    IVariable* CreateVarByType(IVariable::EType type)
    {
        switch (type)
        {
        case IVariable::FLOAT:
            return new CVariable<float>();
        case IVariable::INT:
            return new CVariable<int>();
        case IVariable::STRING:
            return new CVariable<QString>();
        case IVariable::BOOL:
            return new CVariable<bool>();
        case IVariable::VECTOR:
            return new CVariable<Vec3>();
        case IVariable::QUAT:
            return new CVariable<Quat>();
        default:
            assert(0);
        }
        return NULL;
    }

    //////////////////////////////////////////////////////////////////////////
    IVariable* CreateVar(const char* name, IVariable::EType defaultType, const char*& displayName)
    {
        displayName = name;
        // Resolve type from variable name.
        int nameLen = strlen(name);

        int nFlags = 0;
        int iStartIndex = 0;
        if (name[0] == '_')
        {
            nFlags |= IVariable::UI_INVISIBLE;
            iStartIndex++;
        }

        {
            // Try to detect type.
            for (int i = 0; i < sizeof(s_paramTypes) / sizeof(s_paramTypes[0]); i++)
            {
                if (IsPropertyTypeMatch(s_paramTypes[i].prefix, name + iStartIndex, nameLen - iStartIndex, s_paramTypes[i].bExactName))
                {
                    displayName = name + strlen(s_paramTypes[i].prefix) + iStartIndex;
                    if (displayName[0] == '_')
                    {
                        displayName++;
                    }
                    if (displayName[0] == '\0')
                    {
                        displayName = name;
                    }

                    IVariable::EType     type     = s_paramTypes[i].type;
                    IVariable::EDataType dataType = s_paramTypes[i].dataType;

                    IVariable* var;
                    if (type == IVariable::STRING &&
                        (dataType == IVariable::DT_AITERRITORY || dataType == IVariable::DT_AIWAVE))
                    {
                        var = new CVariableEnum<QString>();
                    }
                    else
                    {
                        var = CreateVarByType(type);
                    }

                    if (!var)
                    {
                        continue;
                    }

                    var->SetName(name);
                    var->SetHumanName(displayName);
                    var->SetDataType(s_paramTypes[i].dataType);
                    var->SetFlags(nFlags);
                    if (s_paramTypes[i].flags & SCRIPTPARAM_POSITIVE)
                    {
                        float lmin = 0, lmax = 10000;
                        var->GetLimits(lmin, lmax);
                        // set min Limit to 0 hard, to make it positive only value.
                        var->SetLimits(0, lmax, 0.f, true, false);
                    }
                    return var;
                }
            }
        }
        if (defaultType != IVariable::UNKNOWN)
        {
            IVariable* var = CreateVarByType(defaultType);
            var->SetName(name);
            return var;
        }
        return 0;
    }

    //////////////////////////////////////////////////////////////////////////
    virtual void OnElementFound(int nIdx, ScriptVarType type)
    {
        /* ignore non string indexed values */
    }

    //////////////////////////////////////////////////////////////////////////
    virtual void OnElementFound(const char* sName, ScriptVarType type)
    {
        if (sName && sName[0] != 0)
        {
            Variable var;
            var.name = sName;
            var.type = type;
            m_elements.push_back(var);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void Dump(IScriptTable* pObject, string sTablePath)
    {
        m_elements.reserve(20);
        pObject->Dump(this);
        typedef std::map<QString, IVariablePtr, stl::less_stricmp<QString> > NodesMap;
        NodesMap nodes;
        NodesMap listNodes;

        for (int i = 0; i < m_elements.size(); i++)
        {
            ScriptVarType type = m_elements[i].type;

            //save the name and display name in temporary QByteArrays so the sName and sDisplayName
            //remain valid during calls to CreateVar which manipulates them.
            QByteArray name = m_elements[i].name.toUtf8();
            const char *sName = name.data();

            QByteArray sd = sName;
            const char* sDisplayName = sd.data();

            if (type == svtNumber)
            {
                float fVal;
                pObject->GetValue(sName, fVal);
                IVariable* var = CreateVar(sName, IVariable::FLOAT, sDisplayName);
                if (var)
                {
                    var->Set(fVal);

                    //use lua comment parser to read comment about limit settings
                    float minVal, maxVal, stepVal;
                    string desc;
                    if (LuaCommentParser::GetInstance()->ParseComment(sTablePath, sName, &minVal, &maxVal, &stepVal, &desc))
                    {
                        var->SetLimits(minVal, maxVal, stepVal);
                        var->SetDescription(desc);
                    }

                    nodes[sDisplayName] = var;
                }
            }
            else if (type == svtString)
            {
                const char* sVal;
                pObject->GetValue(sName, sVal);
                IVariable* var = CreateVar(sName, IVariable::STRING, sDisplayName);
                if (var)
                {
                    var->Set(sVal);
                    nodes[sDisplayName] = var;
                }
            }
            else if (type == svtBool)
            {
                bool val = false;
                pObject->GetValue(sName, val);
                IVariable* pVar = CreateVar(sName, IVariable::BOOL, sDisplayName);
                if (pVar)
                {
                    pVar->Set(val);
                    nodes[sDisplayName] = pVar;
                }
            }
            else if (type == svtFunction)
            {
                // Ignore functions.
            }
            else if (type == svtObject)
            {
                // Some Table.
                SmartScriptTable pTable(GetIEditor()->GetSystem()->GetIScriptSystem(), true);
                if (pObject->GetValue(sName, pTable))
                {
                    IVariable* var = CreateVar(sName, IVariable::UNKNOWN, sDisplayName);
                    if (var && var->GetType() == IVariable::VECTOR)
                    {
                        nodes[sDisplayName] = var;
                        float x, y, z;
                        if (pTable->GetValue("x", x) && pTable->GetValue("y", y) && pTable->GetValue("z", z))
                        {
                            var->Set(Vec3(x, y, z));
                        }
                        else
                        {
                            pTable->GetAt(1, x);
                            pTable->GetAt(2, y);
                            pTable->GetAt(3, z);
                            var->Set(Vec3(x, y, z));
                        }
                    }
                    else
                    {
                        if (var)
                        {
                            var->Release();
                        }

                        var = new CVariableArray();
                        var->SetName(sName);
                        listNodes[sName] = var;

                        CScriptPropertiesDump dump(m_varBlock, var);
                        dump.Dump(*pTable, sTablePath + "." + sName);
                    }
                }
            }
        }

        for (NodesMap::iterator nit = nodes.begin(); nit != nodes.end(); nit++)
        {
            if (m_parentVar)
            {
                m_parentVar->AddVariable(nit->second);
            }
            else
            {
                m_varBlock->AddVariable(nit->second);
            }
        }

        for (NodesMap::iterator nit1 = listNodes.begin(); nit1 != listNodes.end(); nit1++)
        {
            if (m_parentVar)
            {
                m_parentVar->AddVariable(nit1->second);
            }
            else
            {
                m_varBlock->AddVariable(nit1->second);
            }
        }
    }
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CEntityScript::CEntityScript(IEntityClass* pClass)
{
    SetClass(pClass);
    m_bValid = false;
    m_haveEventsTable = false;
    m_visibilityMask = 0;

    m_nTextureIcon = 0;

    m_nFlags = 0;
    m_pOnPropertyChangedFunc = 0;
}

//////////////////////////////////////////////////////////////////////////
CEntityScript::~CEntityScript()
{
    if (m_pOnPropertyChangedFunc)
    {
        gEnv->pScriptSystem->ReleaseFunc(m_pOnPropertyChangedFunc);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityScript::SetClass(IEntityClass* pClass)
{
    assert(pClass);
    m_pClass = pClass;
    m_usable = !(pClass->GetFlags() & ECLF_INVISIBLE);
}

//////////////////////////////////////////////////////////////////////////
QString CEntityScript::GetName() const
{
    return m_pClass->GetName();
}

//////////////////////////////////////////////////////////////////////////
QString CEntityScript::GetFile() const
{
    if (IEntityScriptFileHandler* pScriptFileHandler = m_pClass->GetScriptFileHandler())
    {
        return pScriptFileHandler->GetScriptFile();
    }

    return m_pClass->GetScriptFile();
}

//////////////////////////////////////////////////////////////////////////
int CEntityScript::GetEventCount()
{
    if (IEntityEventHandler* pEventHandler = m_pClass->GetEventHandler())
    {
        return pEventHandler->GetEventCount();
    }

    return ( int )m_events.size();
}

//////////////////////////////////////////////////////////////////////////
QString CEntityScript::GetEvent(int i)
{
    if (IEntityEventHandler* pEventHandler = m_pClass->GetEventHandler())
    {
        IEntityEventHandler::SEventInfo info;
        pEventHandler->GetEventInfo(i, info);

        return info.name;
    }

    return m_events[i];
}

//////////////////////////////////////////////////////////////////////////
int CEntityScript::GetEmptyLinkCount()
{
    return ( int )m_emptyLinks.size();
}

//////////////////////////////////////////////////////////////////////////
const QString& CEntityScript::GetEmptyLink(int i)
{
    return m_emptyLinks[i];
}

//////////////////////////////////////////////////////////////////////////
bool CEntityScript::Load()
{
    m_bValid = false;

    bool result = true;
    if (strlen(m_pClass->GetScriptFile()) > 0)
    {
        if (m_pClass->LoadScript(false))
        {
            //Feed script file to the lua comment parser for processing
            LuaCommentParser::GetInstance()->OpenScriptFile(m_pClass->GetScriptFile());

            // If class have script parse this script.
            result = ParseScript();

            LuaCommentParser::GetInstance()->CloseScriptFile();
        }
        else
        {
            CErrorRecord err;
            err.error = QObject::tr("Entity class %1 failed to load, (script: %2 could not be loaded)").arg(m_pClass->GetName(), m_pClass->GetScriptFile());
            err.file = m_pClass->GetScriptFile();
            err.severity = CErrorRecord::ESEVERITY_WARNING;
            err.flags = CErrorRecord::FLAG_SCRIPT;
            GetIEditor()->GetErrorReport()->ReportError(err);
        }
    }
    else
    {
        // No script file: parse the script tables
        // which should have been specified by game code.
        ParseScript();

        IEntityPropertyHandler* pPropertyHandler = m_pClass->GetPropertyHandler();

        if (pPropertyHandler && pPropertyHandler->GetPropertyCount() > 0)
        {
            m_pDefaultProperties = new CVarBlock();

            uint32 currentProperty = 0;
            PropertyVarBlockBuilder build(m_pDefaultProperties, m_pClass->GetPropertyHandler(), &currentProperty);
            build();

            PropertyVarBlockValueGetter get(m_pDefaultProperties, 0, m_pClass->GetPropertyHandler());
            get();
        }
    }

    if (result)
    {
        const SEditorClassInfo& editorClassInfo = m_pClass->GetEditorClassInfo();
        const char* classVisualObject = editorClassInfo.sHelper;
        if (classVisualObject && classVisualObject[0] && m_visualObject.isEmpty())
        {
            m_visualObject = classVisualObject;
        }

        const char* classIcon = editorClassInfo.sIcon;
        if (classIcon && classIcon[0] && !m_nTextureIcon)
        {
            QString iconfile = Path::Make("Editor/ObjectIcons", classIcon);
            m_nTextureIcon = GetIEditor()->GetIconManager()->GetIconTexture(iconfile.toUtf8().data());
        }

        m_bValid = true;
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityScript::ParseScript()
{
    // Parse .lua file.
    IScriptSystem* script = GetIEditor()->GetSystem()->GetIScriptSystem();

    SmartScriptTable pEntity(script, true);
    if (!script->GetGlobalValue(GetName().toUtf8().data(), pEntity))
    {
        return false;
    }

    m_bValid = true;

    pEntity->GetValue("OnPropertyChange", m_pOnPropertyChangedFunc);

    CScriptMethodsDump dump;
    pEntity->Dump(&dump);
    m_methods = dump.methods;
    m_events = dump.events;

    //! Sort methods and events.
    std::sort(m_methods.begin(), m_methods.end());
    std::sort(m_events.begin(), m_events.end());

    m_emptyLinks.clear();

    {
        // Normal properties.
        m_pDefaultProperties = 0;
        SmartScriptTable pProps(script, true);
        if (pEntity->GetValue(PROPERTIES_TABLE, pProps))
        {
            // Properties found in entity.
            m_pDefaultProperties = new CVarBlock;
            CScriptPropertiesDump dump(m_pDefaultProperties);
            dump.Dump(*pProps, PROPERTIES_TABLE);
        }
    }

    {
        // Second set of properties.
        m_pDefaultProperties2 = 0;
        SmartScriptTable pProps(script, true);
        if (pEntity->GetValue(PROPERTIES2_TABLE, pProps))
        {
            // Properties found in entity.
            m_pDefaultProperties2 = new CVarBlock;
            CScriptPropertiesDump dump(m_pDefaultProperties2);
            dump.Dump(*pProps, PROPERTIES2_TABLE);
        }
    }

    // Destroy variable block if empty.
    /*if ( m_pDefaultProperties && m_pDefaultProperties->IsEmpty() )
    {
        m_pDefaultProperties = nullptr;
    }

    // Destroy variable block if empty.
    if ( m_pDefaultProperties2 && m_pDefaultProperties2->IsEmpty() )
    {
        m_pDefaultProperties2 = nullptr;
    }*/

    if (m_pDefaultProperties != 0 && m_pDefaultProperties->GetNumVariables() < 1)
    {
        m_pDefaultProperties = 0;
    }

    // Destroy variable block if empty.
    if (m_pDefaultProperties2 != 0 && m_pDefaultProperties2->GetNumVariables() < 1)
    {
        m_pDefaultProperties2 = 0;
    }

    m_nTextureIcon = 0;

    // Load visual object.
    SmartScriptTable pEditorTable(script, true);
    if (pEntity->GetValue("Editor", pEditorTable))
    {
        const char* modelName;
        if (pEditorTable->GetValue("Model", modelName))
        {
            m_visualObject = modelName;
        }

        bool bShowBounds = false;
        pEditorTable->GetValue("ShowBounds", bShowBounds);
        if (bShowBounds)
        {
            m_nFlags |= ENTITY_SCRIPT_SHOWBOUNDS;
        }
        else
        {
            m_nFlags &= ~ENTITY_SCRIPT_SHOWBOUNDS;
        }

        bool isScalable = true;
        pEditorTable->GetValue("IsScalable", isScalable);
        if (!isScalable)
        {
            m_nFlags |= ENTITY_SCRIPT_ISNOTSCALABLE;
        }
        else
        {
            m_nFlags &= ~ENTITY_SCRIPT_ISNOTSCALABLE;
        }

        bool isRotatable = true;
        pEditorTable->GetValue("IsRotatable", isRotatable);
        if (!isRotatable)
        {
            m_nFlags |= ENTITY_SCRIPT_ISNOTROTATABLE;
        }
        else
        {
            m_nFlags &= ~ENTITY_SCRIPT_ISNOTROTATABLE;
        }

        bool bAbsoluteRadius = false;
        pEditorTable->GetValue("AbsoluteRadius", bAbsoluteRadius);
        if (bAbsoluteRadius)
        {
            m_nFlags |= ENTITY_SCRIPT_ABSOLUTERADIUS;
        }
        else
        {
            m_nFlags &= ~ENTITY_SCRIPT_ABSOLUTERADIUS;
        }

        const char* iconName = "";
        if (pEditorTable->GetValue("Icon", iconName))
        {
            QString iconfile = Path::Make("Editor/ObjectIcons", iconName);
            m_nTextureIcon = GetIEditor()->GetIconManager()->GetIconTexture(iconfile.toUtf8().data());
        }

        bool bIconOnTop = false;
        pEditorTable->GetValue("IconOnTop", bIconOnTop);
        if (bIconOnTop)
        {
            m_nFlags |= ENTITY_SCRIPT_ICONONTOP;
        }
        else
        {
            m_nFlags &= ~ENTITY_SCRIPT_ICONONTOP;
        }

        bool bArrow = false;
        pEditorTable->GetValue("DisplayArrow", bArrow);
        if (bArrow)
        {
            m_nFlags |= ENTITY_SCRIPT_DISPLAY_ARROW;
        }
        else
        {
            m_nFlags &= ~ENTITY_SCRIPT_DISPLAY_ARROW;
        }

        SmartScriptTable pLinksTable(script, true);
        if (pEditorTable->GetValue("Links", pLinksTable))
        {
            IScriptTable::Iterator iter = pLinksTable->BeginIteration();
            while (pLinksTable->MoveNext(iter))
            {
                if (iter.value.type == ANY_TSTRING)
                {
                    const char* sLinkName = iter.value.str;
                    m_emptyLinks.push_back(sLinkName);
                }
            }
            pLinksTable->EndIteration(iter);
        }

        const char* editorPath = "";
        if (pEditorTable->GetValue("EditorPath", editorPath))
        {
            m_userPath = editorPath;
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CEntityScript::Reload()
{
    if (IEntityScriptFileHandler* pScriptFileHandler = m_pClass->GetScriptFileHandler())
    {
        pScriptFileHandler->ReloadScriptFile();
    }

    bool bReloaded = false;
    IEntityScript* pScript = m_pClass->GetIEntityScript();
    if (pScript)
    {
        // First try compiling script and see if it have any errors.
        bool bLoadScript = CFileUtil::CompileLuaFile(m_pClass->GetScriptFile());
        if (bLoadScript)
        {
            bReloaded = m_pClass->LoadScript(true);
        }
    }

    if (bReloaded)
    {
        // Script compiled successfully.
        Load();
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityScript::GotoMethod(const QString& method)
{
    QString line;
    line = QStringLiteral("%1:%2").arg(GetName(), method);

    // Search this line in script file.
    int lineNum = FindLineNum(line);
    if (lineNum >= 0)
    {
        // Open UltraEdit32 with this line.
        CFileUtil::EditTextFile(GetFile().toUtf8().data(), lineNum);
    }
}

void CEntityScript::AddMethod(const QString& method)
{
    // Add a new method to the file. and start Editing it.
    FILE* f = fopen(GetFile().toUtf8().data(), "at");
    if (f)
    {
        fprintf(f, "\n");
        fprintf(f, "-------------------------------------------------------\n");
        fprintf(f, "function %s:%s()\n", m_pClass->GetName(), method.toUtf8().data());
        fprintf(f, "end\n");
        fclose(f);
    }
}

const QString& CEntityScript::GetDisplayPath()
{
    if (m_userPath.isEmpty())
    {
        IScriptSystem* script = GetIEditor()->GetSystem()->GetIScriptSystem();

        SmartScriptTable pEntity(script, true);
        if (!script->GetGlobalValue(GetName().toUtf8().data(), pEntity))
        {
            m_userPath = "Default/" + GetName();
            return m_userPath;
        }

        SmartScriptTable pEditorTable(script, true);
        if (pEntity->GetValue("Editor", pEditorTable))
        {
            const char* editorPath = "";
            if (pEditorTable->GetValue("EditorPath", editorPath))
            {
                m_userPath = editorPath;
            }
        }
    }

    return m_userPath;
}

//////////////////////////////////////////////////////////////////////////
int CEntityScript::FindLineNum(const QString& line)
{
    FILE* fileHandle = fopen(GetFile().toUtf8().data(), "rb");
    if (!fileHandle)
    {
        return -1;
    }

    int lineFound = -1;
    int lineNum = 1;

    fseek(fileHandle, 0, SEEK_END);
    int size = ftell(fileHandle);
    fseek(fileHandle, 0, SEEK_SET);

    char* text = new char[size + 16];
    fread(text, size, 1, fileHandle);
    text[size] = 0;

    char* token = strtok(text, "\n");
    while (token)
    {
        if (strstr(token, line.toUtf8().data()) != 0)
        {
            lineFound = lineNum;
            break;
        }
        token = strtok(NULL, "\n");
        lineNum++;
    }

    fclose(fileHandle);
    delete []text;

    return lineFound;
}

//////////////////////////////////////////////////////////////////////////
void CEntityScript::CopyPropertiesToScriptTable(IEntity* pEntity, CVarBlock* pVarBlock, bool bCallUpdate)
{
    CopyPropertiesToScriptInternal(pEntity, pVarBlock, bCallUpdate, PROPERTIES_TABLE);
}

//////////////////////////////////////////////////////////////////////////
void CEntityScript::CopyProperties2ToScriptTable(IEntity* pEntity, CVarBlock* pVarBlock, bool bCallUpdate)
{
    CopyPropertiesToScriptInternal(pEntity, pVarBlock, bCallUpdate, PROPERTIES2_TABLE);
}

//////////////////////////////////////////////////////////////////////////
void CEntityScript::CopyPropertiesToScriptInternal(IEntity* pEntity, CVarBlock* pVarBlock, bool bCallUpdate, const char* tableKey)
{
    if (!IsValid())
    {
        return;
    }

    assert(pEntity != 0);
    assert(pVarBlock != 0);

    IScriptTable* scriptTable = pEntity->GetScriptTable();
    if (!scriptTable)
    {
        if (IEntityPropertyHandler* pPropertyHandler = m_pClass->GetPropertyHandler())
        {
            PropertyVarBlockValueSetter set(pVarBlock, pEntity, pPropertyHandler);
            set();
        }

        return;
    }

    IScriptSystem* pScriptSystem = GetIEditor()->GetSystem()->GetIScriptSystem();

    SmartScriptTable table(pScriptSystem, true);
    if (!scriptTable->GetValue(tableKey, table))
    {
        return;
    }

    for (int i = 0; i < pVarBlock->GetNumVariables(); i++)
    {
        VarToScriptTable(pVarBlock->GetVariable(i), table);
    }

    if (bCallUpdate)
    {
        CallOnPropertyChange(pEntity);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityScript::CallOnPropertyChange(IEntity* pEntity)
{
    if (!IsValid())
    {
        return;
    }

    if (m_pOnPropertyChangedFunc)
    {
        assert(pEntity != 0);
        IScriptTable* pScriptObject = pEntity->GetScriptTable();
        if (!pScriptObject)
        {
            return;
        }
        Script::CallMethod(pScriptObject, m_pOnPropertyChangedFunc);
    }

    SEntityEvent entityEvent(ENTITY_EVENT_EDITOR_PROPERTY_CHANGED);
    pEntity->SendEvent(entityEvent);
}

//////////////////////////////////////////////////////////////////////////
void CEntityScript::VarToScriptTable(IVariable* pVariable, IScriptTable* pScriptTable)
{
    assert(pVariable);

    IScriptSystem* pScriptSystem = GetIEditor()->GetSystem()->GetIScriptSystem();

    if (pVariable->GetType() == IVariable::ARRAY)
    {
        int type = pScriptTable->GetValueType(pVariable->GetName().toUtf8().data());
        if (type != svtObject)
        {
            return;
        }

        SmartScriptTable table(pScriptSystem, true);
        if (pScriptTable->GetValue(pVariable->GetName().toUtf8().data(), table))
        {
            for (int i = 0; i < pVariable->GetNumVariables(); i++)
            {
                IVariable* child = pVariable->GetVariable(i);
                VarToScriptTable(child, table);
            }
        }
        return;
    }

    QString name = pVariable->GetName();
    int type = pScriptTable->GetValueType(name.toUtf8().data());

    if (type == svtString)
    {
        QString value;
        pVariable->Get(value);
        pScriptTable->SetValue(name.toUtf8().data(), value.toUtf8().data());
    }
    else if (type == svtNumber)
    {
        float val = 0;
        pVariable->Get(val);
        pScriptTable->SetValue(name.toUtf8().data(), val);
    }
    else if (type == svtBool)
    {
        bool val = false;
        pVariable->Get(val);
        pScriptTable->SetValue(name.toUtf8().data(), val);
    }
    else if (type == svtObject)
    {
        // Probably Color/Vector.
        SmartScriptTable table(pScriptSystem, true);
        if (pScriptTable->GetValue(name.toUtf8().data(), table))
        {
            if (pVariable->GetType() == IVariable::VECTOR)
            {
                Vec3 vec;
                pVariable->Get(vec);

                float temp;
                if (table->GetValue("x", temp))
                {
                    // Named vector.
                    table->SetValue("x", vec.x);
                    table->SetValue("y", vec.y);
                    table->SetValue("z", vec.z);
                }
                else
                {
                    // Indexed vector.
                    table->SetAt(1, vec.x);
                    table->SetAt(2, vec.y);
                    table->SetAt(3, vec.z);
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityScript::CopyPropertiesFromScriptTable(IEntity* pEntity, CVarBlock* pVarBlock)
{
    CopyPropertiesFromScriptInternal(pEntity, pVarBlock, PROPERTIES_TABLE);
}

//////////////////////////////////////////////////////////////////////////
void CEntityScript::CopyProperties2FromScriptTable(IEntity* pEntity, CVarBlock* pVarBlock)
{
    CopyPropertiesFromScriptInternal(pEntity, pVarBlock, PROPERTIES2_TABLE);
}

//////////////////////////////////////////////////////////////////////////
void CEntityScript::CopyPropertiesFromScriptInternal(IEntity* pEntity, CVarBlock* pVarBlock, const char* tableKey)
{
    if (!IsValid())
    {
        return;
    }

    assert(pEntity != 0);
    assert(pVarBlock != 0);

    IScriptTable* scriptTable = pEntity->GetScriptTable();
    if (!scriptTable)
    {
        return;
    }

    IScriptSystem* pScriptSystem = GetIEditor()->GetSystem()->GetIScriptSystem();

    SmartScriptTable table(pScriptSystem, true);
    if (!scriptTable->GetValue(tableKey, table))
    {
        return;
    }

    for (int i = 0; i < pVarBlock->GetNumVariables(); i++)
    {
        ScriptTableToVar(table, pVarBlock->GetVariable(i));
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityScript::ScriptTableToVar(IScriptTable* pScriptTable, IVariable* pVariable)
{
    assert(pVariable);

    IScriptSystem* pScriptSystem = GetIEditor()->GetSystem()->GetIScriptSystem();

    if (pVariable->GetType() == IVariable::ARRAY)
    {
        int type = pScriptTable->GetValueType(pVariable->GetName().toUtf8().data());
        if (type != svtObject)
        {
            return;
        }

        SmartScriptTable table(pScriptSystem, true);
        if (pScriptTable->GetValue(pVariable->GetName().toUtf8().data(), table))
        {
            for (int i = 0; i < pVariable->GetNumVariables(); i++)
            {
                IVariable* child = pVariable->GetVariable(i);
                ScriptTableToVar(table, child);
            }
        }
        return;
    }

    QString name = pVariable->GetName();
    int type = pScriptTable->GetValueType(name.toUtf8().data());

    if (type == svtString)
    {
        const char* value;
        pScriptTable->GetValue(name.toUtf8().data(), value);
        pVariable->Set(value);
    }
    else if (type == svtNumber)
    {
        float val = 0;
        pScriptTable->GetValue(name.toUtf8().data(), val);
        pVariable->Set(val);
    }
    else if (type == svtBool)
    {
        bool val = false;
        pScriptTable->GetValue(name.toUtf8().data(), val);
        pVariable->Set(val);
    }
    else if (type == svtObject)
    {
        // Probably Color/Vector.
        SmartScriptTable table(pScriptSystem, true);
        if (pScriptTable->GetValue(name.toUtf8().data(), table))
        {
            if (pVariable->GetType() == IVariable::VECTOR)
            {
                Vec3 vec;

                float temp;
                if (table->GetValue("x", temp))
                {
                    // Named vector.
                    table->GetValue("x", vec.x);
                    table->GetValue("y", vec.y);
                    table->GetValue("z", vec.z);
                }
                else
                {
                    // Indexed vector.
                    table->GetAt(1, vec.x);
                    table->GetAt(2, vec.y);
                    table->GetAt(3, vec.z);
                }

                pVariable->Set(vec);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void    CEntityScript::RunMethod(IEntity* pEntity, const QString& method)
{
    if (!IsValid())
    {
        return;
    }

    assert(pEntity != 0);

    IScriptTable* scriptTable = pEntity->GetScriptTable();
    if (!scriptTable)
    {
        return;
    }

    IScriptSystem* scriptSystem = GetIEditor()->GetSystem()->GetIScriptSystem();

    Script::CallMethod(scriptTable, method.toUtf8().data());
}

//////////////////////////////////////////////////////////////////////////
void    CEntityScript::SendEvent(IEntity* pEntity, const QString& method)
{
    if (IEntityEventHandler* pEventHandler = m_pClass->GetEventHandler())
    {
        pEventHandler->SendEvent(pEntity, method.toUtf8().data());
    }

    // Fire event on Entity.
    IComponentScriptPtr scriptComponent = pEntity->GetComponent<IComponentScript>();
    if (scriptComponent)
    {
        scriptComponent->CallEvent(method.toUtf8().data());
    }
}

//////////////////////////////////////////////////////////////////////////
void    CEntityScript::SetEventsTable(CEntityObject* pEntity)
{
    if (!IsValid())
    {
        return;
    }
    assert(pEntity != 0);

    IEntity* ientity = pEntity->GetIEntity();
    if (!ientity)
    {
        return;
    }

    IScriptTable* scriptTable = ientity->GetScriptTable();
    if (!scriptTable)
    {
        return;
    }

    // If events target table is null, set event table to null either.
    if (pEntity->GetEventTargetCount() == 0)
    {
        if (m_haveEventsTable)
        {
            scriptTable->SetToNull("Events");
        }
        m_haveEventsTable = false;
        return;
    }

    IScriptSystem* scriptSystem = GetIEditor()->GetSystem()->GetIScriptSystem();
    SmartScriptTable pEvents(scriptSystem, false);

    scriptTable->SetValue("Events", *pEvents);
    m_haveEventsTable = true;

    std::set<QString> sourceEvents;
    for (int i = 0; i < pEntity->GetEventTargetCount(); i++)
    {
        CEntityEventTarget& et = pEntity->GetEventTarget(i);
        sourceEvents.insert(et.sourceEvent);
    }
    for (std::set<QString>::iterator it = sourceEvents.begin(); it != sourceEvents.end(); it++)
    {
        SmartScriptTable pTrgEvents(scriptSystem, false);
        QString sourceEvent = *it;

        pEvents->SetValue(sourceEvent.toUtf8().data(), *pTrgEvents);

        // Put target events to table.
        int trgEventIndex = 1;
        for (int i = 0; i < pEntity->GetEventTargetCount(); i++)
        {
            CEntityEventTarget& et = pEntity->GetEventTarget(i);
            if (QString::compare(et.sourceEvent, sourceEvent, Qt::CaseInsensitive) == 0)
            {
                EntityId entityId = 0;
                if (et.target)
                {
                    if (qobject_cast<CEntityObject*>(et.target))
                    {
                        entityId = (( CEntityObject* )et.target)->GetEntityId();
                    }
                }

                SmartScriptTable pTrgEvent(scriptSystem, false);
                pTrgEvents->SetAt(trgEventIndex, *pTrgEvent);
                trgEventIndex++;

                ScriptHandle idHandle;
                idHandle.n = entityId;

                pTrgEvent->SetAt(1, idHandle);
                pTrgEvent->SetAt(2, et.event.toUtf8().data());
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityScript::UpdateTextureIcon(IEntity* pEntity)
{
    // Try to call a function GetEditorIcon on the script, to give it a change to return
    // a custom icon.
    IScriptTable* pScriptTable = pEntity->GetScriptTable();
    if (pScriptTable != NULL && pScriptTable->GetValueType("GetEditorIcon") == svtFunction)
    {
        SmartScriptTable iconData(gEnv->pScriptSystem);
        if (Script::CallMethod(pScriptTable, "GetEditorIcon", iconData))
        {
            const char* iconName = NULL;
            iconData->GetValue("Icon", iconName);

            QString iconfile = Path::Make("Editor/ObjectIcons", iconName);
            m_nTextureIcon = GetIEditor()->GetIconManager()->GetIconTexture(iconfile.toUtf8().data());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// CEntityScriptRegistry implementation.
//////////////////////////////////////////////////////////////////////////
CEntityScriptRegistry* CEntityScriptRegistry::m_instance = 0;

CEntityScriptRegistry::CEntityScriptRegistry()
{
    gEnv->pEntitySystem->GetClassRegistry()->RegisterListener(this);
}

CEntityScriptRegistry::~CEntityScriptRegistry()
{
    m_instance = 0;
    m_scripts.Clear();
    gEnv->pEntitySystem->GetClassRegistry()->UnregisterListener(this);
}

//////////////////////////////////////////////////////////////////////////
void CEntityScriptRegistry::OnEntityClassRegistryEvent(EEntityClassRegistryEvent event, const IEntityClass* pEntityClass)
{
    switch (event)
    {
    case ECRE_CLASS_REGISTERED:
    case ECRE_CLASS_MODIFIED:
    {
        CEntityScript*  pScript = Find(pEntityClass->GetName());
        if (pScript != NULL)
        {
            pScript->SetClass(const_cast<IEntityClass*>(pEntityClass));
        }
        else
        {
            Insert(new CEntityScript(const_cast<IEntityClass*>(pEntityClass)));
        }
    }
    }
}

CEntityScript* CEntityScriptRegistry::Find(const QString& name)
{
    CEntityScriptPtr script = 0;
    if (m_scripts.Find(name, script))
    {
        return script;
    }
    return 0;
}

void CEntityScriptRegistry::Insert(CEntityScript* pEntityScript)
{
    // Check if inserting already exist script, if so ignore.
    CEntityScriptPtr temp;
    if (m_scripts.Find(pEntityScript->GetName(), temp))
    {
        Error("Inserting duplicate Entity Script %s", pEntityScript->GetName().toUtf8().data());
        return;
    }
    m_scripts[pEntityScript->GetName()] = pEntityScript;

    SetClassCategory(pEntityScript);
}

void CEntityScriptRegistry::SetClassCategory(CEntityScript* script)
{
    IScriptSystem* scriptSystem = GetIEditor()->GetSystem()->GetIScriptSystem();

    SmartScriptTable pEntity(scriptSystem, true);
    if (!scriptSystem->GetGlobalValue(script->GetName().toUtf8().data(), pEntity))
    {
        return;
    }

    SmartScriptTable pEditor(scriptSystem, true);
    if (pEntity->GetValue("Editor", pEditor))
    {
        const char* clsCategory;
        if (pEditor->GetValue("Category", clsCategory))
        {
            SEditorClassInfo editorClassInfo(script->GetClass()->GetEditorClassInfo());
            editorClassInfo.sCategory = clsCategory;
            script->GetClass()->SetEditorClassInfo(editorClassInfo);
        }
    }
}

void CEntityScriptRegistry::GetScripts(std::vector<CEntityScript*>& scripts)
{
    std::vector<CEntityScriptPtr> s;
    m_scripts.GetAsVector(s);

    scripts.resize(s.size());
    for (int i = 0; i < s.size(); i++)
    {
        scripts[i] = s[i];
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityScriptRegistry::Reload()
{
    IEntityClassRegistry* pClassRegistry = gEnv->pEntitySystem->GetClassRegistry();
    pClassRegistry->LoadClasses("Entities", true);
    LoadScripts();
}

//////////////////////////////////////////////////////////////////////////
void CEntityScriptRegistry::LoadScripts()
{
    m_scripts.Clear();

    IEntityClassRegistry* pClassRegistry = gEnv->pEntitySystem->GetClassRegistry();

    IEntityClass* pClass = NULL;
    pClassRegistry->IteratorMoveFirst();
    while (pClass = pClassRegistry->IteratorNext())
    {
        CEntityScript* script = new CEntityScript(pClass);
        Insert(script);
    }
}

//////////////////////////////////////////////////////////////////////////
CEntityScriptRegistry* CEntityScriptRegistry::Instance()
{
    if (!m_instance)
    {
        m_instance = new CEntityScriptRegistry;
    }
    return m_instance;
}

//////////////////////////////////////////////////////////////////////////
void CEntityScriptRegistry::Release()
{
    if (m_instance)
    {
        delete m_instance;
    }
    m_instance = 0;
}
