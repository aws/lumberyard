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

#ifndef CRYINCLUDE_EDITOR_AI_AIBEHAVIORLIBRARY_H
#define CRYINCLUDE_EDITOR_AI_AIBEHAVIORLIBRARY_H
#pragma once


#include "AiBehavior.h"

/*!
 * CAIBehaviorLibrary is collection of global AI behaviors.
 */
class CAIBehaviorLibrary
{
public:
    CAIBehaviorLibrary();
    ~CAIBehaviorLibrary() {};

    //! Add new behavior to the library.
    void AddBehavior(CAIBehavior* behavior);
    //! Remove behavior from the library.
    void RemoveBehavior(CAIBehavior* behavior);

    CAIBehavior* FindBehavior(const QString& name) const;

    //! Clear all behaviors from library.
    void ClearBehaviors();

    //! Get all stored behaviors as a vector.
    void GetBehaviors(std::vector<CAIBehaviorPtr>& behaviors);

    //! Load all behaviors from given path and add them to library.
    void LoadBehaviors(const QString& path);

    //! Reload behavior scripts.
    void ReloadScripts();

    //! Get all available characters in system.
    void GetCharacters(std::vector<CAICharacterPtr>& characters);

    //! Add new behavior to the library.
    void AddCharacter(CAICharacter* chr);
    //! Remove behavior from the library.
    void RemoveCharacter(CAICharacter* chr);

    // Finds specified character.
    CAICharacter* FindCharacter(const QString& name) const;

private:
    void LoadCharacters();

    StdMap<QString, TSmartPtr<CAIBehavior> > m_behaviors;
    StdMap<QString, TSmartPtr<CAICharacter> > m_characters;
    QString m_scriptsPath;
};

#endif // CRYINCLUDE_EDITOR_AI_AIBEHAVIORLIBRARY_H
