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

#ifndef CRYINCLUDE_EDITOR_AI_AIBEHAVIOR_H
#define CRYINCLUDE_EDITOR_AI_AIBEHAVIOR_H
#pragma once


/** AI Behavior definition.
*/
class CAIBehavior
    : public CRefCountBase
{
public:
    CAIBehavior() {};
    virtual ~CAIBehavior() {};

    void SetName(const QString& name) { m_name = name; }
    const QString& GetName() { return m_name; }

    //! Set name of script that implements this behavior.
    void SetScript(const QString& script) { m_script = script; };
    const QString& GetScript() const { return m_script; };

    //! Get human readable description of this goal.
    const QString& GetDescription() { return m_description; }
    //! Set human readable description of this goal.
    void SetDescription(const QString& desc) { m_description = desc; }

    //! Force reload of script file.
    void ReloadScript();

    //! Start editing script file in Text editor.
    void Edit();

private:
    QString m_name;
    QString m_description;
    QString m_script;
};

/** AICharacter behaviour definition.
*/
class CAICharacter
    : public CRefCountBase
{
public:
    CAICharacter() {};
    virtual ~CAICharacter() {};

    void SetName(const QString& name) { m_name = name; }
    const QString& GetName() { return m_name; }

    //! Set name of script that implements this behavior.
    void SetScript(const QString& script) { m_script = script; };
    const QString& GetScript() const { return m_script; };

    //! Get human readable description of this goal.
    const QString& GetDescription() { return m_description; }
    //! Set human readable description of this goal.
    void SetDescription(const QString& desc) { m_description = desc; }

    //! Force reload of script file.
    void ReloadScript();

    //! Start editing script file in Text editor.
    void Edit();

private:
    QString m_name;
    QString m_description;
    QString m_script;
};

typedef TSmartPtr<CAIBehavior> CAIBehaviorPtr;
typedef TSmartPtr<CAICharacter> CAICharacterPtr;

#endif // CRYINCLUDE_EDITOR_AI_AIBEHAVIOR_H
