// ConsoleSCB.h: interface for the CConsoleSCB class.
//
//////////////////////////////////////////////////////////////////////

#ifndef CRYINCLUDE_EDITOR_UTILS_REFLECTEDPROPERTYTESTS_H
#define CRYINCLUDE_EDITOR_UTILS_REFLECTEDPROPERTYTESTS_H
#pragma once

#include <AzCore/Serialization/SerializeContext.h>
#include "Util/Variable.h"

class CReflectedVar;
class CPropertyContainer;
class ReflectedPropertyControl;

namespace AzToolsFramework{
    class ReflectedPropertyEditor;
}


//Tests for the ReflectedPropertyCtrl (uses IVariable instances to populate CReflectedVars)
class ReflectedPropertyCtrlTest
{
public:
    static void Test(QWidget *parent = nullptr);
    static CVarBlock* GenerateTestData();

private:
    static CVarBlock *s_varBlock;
    static CVarBlock *s_varBlockParticle;
    template <class T> static IVariable* CreateVariable(const char *varName, const T &value, IVariable *parent, unsigned char dataType = IVariable::DT_SIMPLE);
    static void OnVariableUpdated(IVariable *pVar);
};

// test for raw CReflectedVars.  Show them in ReflectedPropertyEditor
class ReflectedVarTest
{
public:
    //Single call to automatically generate test data and show property editor
    static void Test();

    //generate some test data
    static CPropertyContainer* GenerateTestData();

    //show a property editor containing the specified data
    static void ShowPropertyEditor(CPropertyContainer *data, const QString &title, bool addEachVarAsInstance = false, QWidget *parent = nullptr);

private:
    static AZ::SerializeContext* s_serializeContext;
};



#endif // CRYINCLUDE_EDITOR_UTILS_REFLECTEDPROPERTYTESTS_H
