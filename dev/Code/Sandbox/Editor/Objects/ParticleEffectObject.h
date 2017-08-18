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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_PARTICLEEFFECTOBJECT_H
#define CRYINCLUDE_EDITOR_OBJECTS_PARTICLEEFFECTOBJECT_H
#pragma once


#include "EntityObject.h"

class CParticleItem;

class CRYEDIT_API CParticleEffectObject
    : public CEntityObject
{
    Q_OBJECT
public:
    static const GUID& GetClassID()
    {
        // {9f916533-769f-44ab-9319-b94cb4ebe432}
        static const GUID guid = {
            0x9f916533, 0x769f, 0x44ab, { 0x93, 0x19, 0xb9, 0x4c, 0xb4, 0xeb, 0xe4, 0x32 }
        };
        return guid;
    }

    void BeginEditParams(IEditor* ie, int flags);
    void EndEditParams(IEditor* ie);
    bool Init(IEditor* ie, CBaseObject* prev, const QString& file);

    void Display(DisplayContext& disp);

    static bool IsGroup(const QString& effectName);
    QString GetParticleName() const;
    QString GetComment() const;

    void OnMenuGoToDatabase() const;
    // Confetti Begin: Jurecka
    void OnMenuGoToEditor() const;
    // Confetti End: Jurecka

    class CFastParticleParser
    {
    public:
        void ExtractParticlePathes(const QString& particlePath);
        void ExtractLevelParticles();
        size_t  GetCount()  {   return m_ParticleList.size();   }
        QString GetParticlePath(size_t index) {return m_ParticleList[index].pathName; }
        bool HaveParticleChildren(size_t index)   {   return m_ParticleList[index].bHaveChildren; }

    private:
        void ParseParticle(XmlNodeRef& node, const QString& parentPath);

    private:
        struct SParticleInfo
        {
            SParticleInfo()
            {
                bHaveChildren = false;
                pathName = "";
            }
            ~SParticleInfo(){}
            bool bHaveChildren;
            QString pathName;
        };

        std::vector<SParticleInfo> m_ParticleList;
    };

private:

    CParticleItem* GetChildParticleItem(CParticleItem* pParentItem, QString composedName, const QString& wishName) const;

private:
    QString m_ParticleEntityName;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_PARTICLEEFFECTOBJECT_H
