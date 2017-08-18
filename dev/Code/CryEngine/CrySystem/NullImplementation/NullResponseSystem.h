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

// Description : A null implementation that does nothing. Just a stub,
//               if a real implementation was not provided, failed to initialize

#ifndef NULL_RESPONSE_SYSTEM_H_
#define NULL_RESPONSE_SYSTEM_H_

#include <IDynamicResponseSystem.h>

namespace NULLDynamicResponse
{
    struct CVariableCollection
        : public DRS::IVariableCollection
    {
        virtual DRS::CResponseVariable* CreateVariable(const char* name, int initialValue) { return 0; }
        virtual DRS::CResponseVariable* CreateVariable(const char* name, float initialValue) { return 0; }
        virtual DRS::CResponseVariable* CreateVariable(const char* name, const char* initialValue) { return 0; }

        //changes the value of the variable with the given name (with the option to create a variable with the name if not existing before)
        virtual bool SetVariableValue(const char* name, int newValue, bool createIfNotExisting = true, float resetTime = -1.0f) { return true; }
        virtual bool SetVariableValue(const char* name, float newValue, bool createIfNotExisting = true, float resetTime = -1.0f) { return true; }
        virtual bool SetVariableValue(const char* name, const char* newValue, bool createIfNotExisting = true, float resetTime = -1.0f) { return true; }
        //changes the value of the specified variable
        virtual bool SetVariableValue(DRS::CResponseVariable* pVariable, int newValue, float resetTime = -1.0f) { return true; }
        virtual bool SetVariableValue(DRS::CResponseVariable* pVariable, float newValue, float resetTime = -1.0f) { return true; }
        virtual bool SetVariableValue(DRS::CResponseVariable* pVariable, const char* newValue, float resetTime = -1.0f) { return true; }

        virtual DRS::CResponseVariable* GetVariable(const char* name) { return 0; }

        virtual const char* GetNameAsString() const { return "NullImplVariableCollection"; }
    };

    struct CResponseActor
        : public DRS::IResponseActor
    {
        //IResponseActor INTERFACE
        virtual const char* GetName() const { return "NullActor"; }
        virtual DRS::IVariableCollection* GetLocalVariables() { return (DRS::IVariableCollection*)&m_pLocalVariables; }
        virtual const DRS::IVariableCollection* GetLocalVariables() const { return (DRS::IVariableCollection*)&m_pLocalVariables; }
        virtual uint GetUserData() const { return 0; }
        //end interface

    protected:
        CVariableCollection m_pLocalVariables;
    };

    struct CSystem
        : public DRS::IDynamicResponseSystem
    {
    public:
        CSystem() {}
        virtual ~CSystem() {}

        virtual bool Init(const char* pFilesFolder) { return true; }
        virtual bool ReInit() { return true; }
        virtual void Update() {}

        virtual DRS::IVariableCollection* CreateVariableCollection(const char* name) { return &m_nullVariableCollection; }
        virtual void ReleaseVariableCollection(DRS::IVariableCollection* pToBeReleased) {}
        virtual DRS::IVariableCollection* GetCollection(const char* name) { return &m_nullVariableCollection; }

        virtual void QueueSignal(const char* signalName, DRS::IResponseActor* pSender, DRS::IVariableCollection* pSignalContext = 0, float delayBeforeFiring = 0.0f, bool autoReleaseContext = true) {}
        virtual void CancelSignalProcessing(const char* signalName, DRS::IResponseActor* pSender, float delayBeforeCancel = 0.0f) {}

        virtual DRS::IResponseActor* CreateResponseActor(const char* pActorName, const uint userData = 0) { return &m_nullActor; }
        virtual bool ReleaseResponseActor(DRS::IResponseActor* pActorToFree) { return true; }
        virtual DRS::IResponseActor* GetResponseActor(const char* pActorName) { return 0; }

        virtual void RegisterNewAction(const char* pActionTypeName, DRS::IResponseActionCreator* pActionCreator) {}
        virtual void RegisterNewSpecialCondition(const char* pConditionTypeName, DRS::IResponseConditionCreator* pConditionCreator) {}

        virtual void Reset() {}

    protected:
        CVariableCollection m_nullVariableCollection;
        CResponseActor m_nullActor;
    };
}

#endif  //NULL_RESPONSE_SYSTEM_H_