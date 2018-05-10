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

#ifndef CRYINCLUDE_EDITOR_UTILS_REFLECTEDVARWRAPPER_H
#define CRYINCLUDE_EDITOR_UTILS_REFLECTEDVARWRAPPER_H
#pragma once

#include "Util/Variable.h"
#include <Util/VariablePropertyType.h>
#include "ReflectedVar.h"

#include <QScopedPointer>

struct CUIEnumsDatabase_SEnum;
class ReflectedPropertyItem;

// Class to wrap the CReflectedVars and sync them with corresponding IVariable.
// Most of this code is ported from CPropertyItem functions that marshal data between
// IVariable and editor widgets.

class ReflectedVarAdapter
{
public:
    virtual ~ReflectedVarAdapter(){};

    // update the range limits in CReflectedVar to range specified in IVariable
    virtual void UpdateRangeLimits(IVariable* pVariable) {};

    //set IVariable for this property and create a CReflectedVar to represent it
    virtual void SetVariable(IVariable* pVariable) = 0;

    //update the ReflectedVar to current value of IVar
    virtual void SyncReflectedVarToIVar(IVariable* pVariable) = 0;

    //update the internal IVariable as result of ReflectedVar changing
    virtual void SyncIVarToReflectedVar(IVariable* pVariable) = 0;

    // Callback called when variable change. SyncReflectedVarToIVar will be called after
    virtual void OnVariableChange(IVariable* var) {};

    virtual bool UpdateReflectedVarEnums() { return false; }

    virtual CReflectedVar* GetReflectedVar() = 0;

    //needed for containers that can have new values filled in
    virtual void ReplaceVarBlock(CVarBlock* varBlock) {};

    virtual bool Contains(CReflectedVar* var) { return GetReflectedVar() == var; }
};


class ReflectedVarIntAdapter
    : public ReflectedVarAdapter
{
public:
    void SetVariable(IVariable* pVariable) override;
    void UpdateRangeLimits(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }
private:
    QScopedPointer<CReflectedVarInt > m_reflectedVar;
    float m_valueMultiplier = 1.0f;
};

class ReflectedVarFloatAdapter
    : public ReflectedVarAdapter
{
public:
    void SetVariable(IVariable* pVariable) override;
    void UpdateRangeLimits(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }
private:
    QScopedPointer<CReflectedVarFloat > m_reflectedVar;
    float m_valueMultiplier = 1.0f;
};

class ReflectedVarStringAdapter
    : public ReflectedVarAdapter
{
public:
    void SetVariable(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }
private:
    QScopedPointer<CReflectedVarString > m_reflectedVar;
};

class ReflectedVarBoolAdapter
    : public ReflectedVarAdapter
{
public:
    void SetVariable(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }
private:
    QScopedPointer<CReflectedVarBool > m_reflectedVar;
};

class ReflectedVarEnumAdapter
    : public ReflectedVarAdapter
{
public:
    ReflectedVarEnumAdapter();
    void SetVariable(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    virtual void OnVariableChange(IVariable* var);
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }

protected:
    //update the ReflectedVar with the allowable enum options
    bool UpdateReflectedVarEnums() override;
    
    //virtual function to allow derived classes (AIWave and AITerritory) to update the enum list before syncing with ReflectedVar.
    virtual void updateIVariableEnumList(IVariable* pVariable) {};

private:
    QScopedPointer<CReflectedVarEnum<AZStd::string>  > m_reflectedVar;
    IVariable* m_pVariable;
    IVarEnumListPtr m_enumList;
    bool m_updatingEnums;
};


class ReflectedVarAIWaveAdapter : public ReflectedVarEnumAdapter
{
public:
    void SetCurrentTerritory(const QString& territory);

private:
    void updateIVariableEnumList(IVariable* pVariable) override;
    QString m_currentTerritory;
};

class ReflectedVarAITerritoryAdapter : public ReflectedVarEnumAdapter
{
public:
    ReflectedVarAITerritoryAdapter();
    void SetAIWaveAdapter(ReflectedVarAIWaveAdapter* adapter);
    void SyncIVarToReflectedVar(IVariable* pVariable) override;

private:
    void OnVariableChange(IVariable* var) override;
    void updateIVariableEnumList(IVariable* pVariable) override;
    ReflectedVarAIWaveAdapter* m_pWaveAdapter;
};


class ReflectedVarDBEnumAdapter
    : public ReflectedVarAdapter
{
public:
    void SetVariable(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }
private:
    QScopedPointer<CReflectedVarEnum<AZStd::string>  > m_reflectedVar;
    CUIEnumsDatabase_SEnum* m_pEnumDBItem;
};

class ReflectedVarVector2Adapter
    : public ReflectedVarAdapter
{
public:
    void SetVariable(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }
private:
    QScopedPointer<CReflectedVarVector2 > m_reflectedVar;
};

class ReflectedVarVector3Adapter
    : public ReflectedVarAdapter
{
public:
    void SetVariable(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }
private:
    QScopedPointer<CReflectedVarVector3 > m_reflectedVar;
};

class ReflectedVarVector4Adapter
    : public ReflectedVarAdapter
{
public:
    void SetVariable(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }
private:
    QScopedPointer<CReflectedVarVector4 > m_reflectedVar;
};


class ReflectedVarColorAdapter
    : public ReflectedVarAdapter
{
public:
    void SetVariable(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }
private:
    QScopedPointer<CReflectedVarColor > m_reflectedVar;
};

class ReflectedVarAnimationAdapter
    : public ReflectedVarAdapter
{
public:
    void SetVariable(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }
private:
    QScopedPointer<CReflectedVarAnimation > m_reflectedVar;
};

class ReflectedVarResourceAdapter
    : public ReflectedVarAdapter
{
public:
    void SetVariable(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }
private:
    QScopedPointer<CReflectedVarResource> m_reflectedVar;
};

class ReflectedVarUserAdapter 
    : public ReflectedVarAdapter
{
public:
    void SetVariable(IVariable *pVariable) override;
    void SyncReflectedVarToIVar(IVariable *pVariable) override;
    void SyncIVarToReflectedVar(IVariable *pVariable) override;
    CReflectedVar *GetReflectedVar() override {
        return m_reflectedVar.data();
    }
private:
    QScopedPointer<CReflectedVarUser> m_reflectedVar;
};

class ReflectedVarSplineAdapter
    : public ReflectedVarAdapter
{
public:
    ReflectedVarSplineAdapter(ReflectedPropertyItem *parentItem, PropertyType propertyType);
    void SetVariable(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override {
        return m_reflectedVar.data();
    }
private:
    QScopedPointer<CReflectedVarSpline > m_reflectedVar;
    bool m_bDontSendToControl;
    PropertyType m_propertyType;
    ReflectedPropertyItem *m_parentItem;
};


class ReflectedVarGenericPropertyAdapter
    : public ReflectedVarAdapter
{
public:
    ReflectedVarGenericPropertyAdapter(PropertyType propertyType);
    void SetVariable(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }
private:
    QScopedPointer<CReflectedVarGenericProperty > m_reflectedVar;
    PropertyType m_propertyType;
};

class ReflectedVarMotionAdapter
    : public ReflectedVarAdapter
{
public:
    void SetVariable(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }
private:
    QScopedPointer<CReflectedVarMotion > m_reflectedVar;
};


#endif // CRYINCLUDE_EDITOR_UTILS_REFLECTEDVARWRAPPER_H
