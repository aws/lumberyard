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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_AICOVERSURFACE_H
#define CRYINCLUDE_EDITOR_OBJECTS_AICOVERSURFACE_H
#pragma once



#include "BaseObject.h"

#include <ICoverSystem.h>


class CAICoverSurface
    : public CBaseObject
{
    Q_OBJECT
public:
    enum EGeneratedResult
    {
        None = 0,
        Success,
        Error,
    };

    virtual bool Init(IEditor* editor, CBaseObject* prev, const QString& file);
    virtual bool CreateGameObject();
    virtual void Done();
    virtual void InvalidateTM(int whyFlags);
    virtual void SetSelected(bool bSelect);

    virtual void Serialize(CObjectArchive& archive);
    virtual XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode);

    virtual void DeleteThis();
    virtual void Display(DisplayContext& disp);

    virtual int MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);
    virtual bool HitTest(HitContext& hitContext);
    virtual void GetBoundBox(AABB& aabb);
    virtual void GetLocalBounds(AABB& aabb);

    virtual void SetHelperScale(float scale);
    virtual float GetHelperScale();

    virtual void BeginEditParams(IEditor* editor, int flags);
    virtual void EndEditParams(IEditor* editor);

    virtual void BeginEditMultiSelParams(bool allSameType);
    virtual void EndEditMultiSelParams();

    float GetHelperSize() const;

    const CoverSurfaceID& GetSurfaceID() const;
    void SetSurfaceID(const CoverSurfaceID& coverSurfaceID);
    void Generate();
    void ValidateGenerated();

    template<typename T>
    void SerializeVar(CObjectArchive& archive, const char* name, CSmartVariable<T>& value)
    {
        if (archive.bLoading)
        {
            T saved;
            if (archive.node->getAttr(name, saved))
            {
                value = saved;
            }
        }
        else
        {
            archive.node->setAttr(name, value);
        }
    };

    template<typename T>
    void SerializeVarEnum(CObjectArchive& archive, const char* name, CSmartVariableEnum<T>& value)
    {
        if (archive.bLoading)
        {
            T saved;
            if (archive.node->getAttr(name, saved))
            {
                value = saved;
            }
        }
        else
        {
            archive.node->setAttr(name, (T&)value);
        }
    };

    template<typename T>
    void SerializeValue(CObjectArchive& archive, const char* name, T& value)
    {
        if (archive.bLoading)
        {
            archive.node->getAttr(name, value);
        }
        else
        {
            archive.node->setAttr(name, value);
        }
    };



protected:
    friend class CTemplateObjectClassDesc<CAICoverSurface>;

    CAICoverSurface();
    virtual ~CAICoverSurface();

    static const GUID& GetClassID()
    {
        // {27D79905-BC77-4175-BF36-D2D162B0309C}
        static const GUID guid = {
            0x27d79905, 0xbc77, 0x4175, { 0xbf, 0x36, 0xd2, 0xd1, 0x62, 0xb0, 0x30, 0x9c }
        };

        return guid;
    }

    void CreateSampler();
    void ReleaseSampler();
    void StartSampling();

    void SetPropertyVarsFromParams(const ICoverSampler::Params& params);
    ICoverSampler::Params GetParamsFromPropertyVars();

    void CommitSurface();
    void ClearSurface();

    void DisplayBadCoverSurfaceObject();

    void OnPropertyVarChange(IVariable* var);

    void CreatePropertyVars();
    void ClonePropertyVars(CVarBlockPtr originalPropertyVars);
    _smart_ptr<CVarBlock> m_propertyVars;

    struct PropertyValues
    {
        CSmartVariableEnum<QString> sampler;
        CSmartVariable<float> limitLeft;
        CSmartVariable<float> limitRight;
        CSmartVariable<float> limitDepth;
        CSmartVariable<float> limitHeight;
        CSmartVariable<float> widthInterval;
        CSmartVariable<float> heightInterval;
        CSmartVariable<float> maxStartHeight;
        CSmartVariable<float> simplifyThreshold;

        void Serialize(CAICoverSurface& object, CObjectArchive& archive);

        // Needed because CSmartVariable copy constructor/assignment operator actually copies the pointer, and not the value
        PropertyValues& operator=(const PropertyValues& other)
        {
            sampler = *other.sampler;

            limitLeft = *other.limitLeft;
            limitRight =  *other.limitRight;
            limitDepth = *other.limitDepth;
            limitHeight = *other.limitHeight;

            widthInterval = *other.widthInterval;
            heightInterval = *other.heightInterval;

            maxStartHeight = *other.maxStartHeight;

            simplifyThreshold = *other.simplifyThreshold;

            return *this;
        }
    } m_propertyValues;

    AABB m_aabb;
    AABB m_aabbLocal;
    ICoverSampler* m_sampler;
    CoverSurfaceID m_surfaceID;
    float m_helperScale;

    EGeneratedResult    m_samplerResult;
};

template<>
void CAICoverSurface::SerializeVarEnum<QString>(CObjectArchive& archive, const char* name, CSmartVariableEnum<QString>& value);

template<>
void CAICoverSurface::SerializeValue(CObjectArchive& archive, const char* name, CoverSurfaceID& value);

template<>
void CAICoverSurface::SerializeValue<QString>(CObjectArchive& archive, const char* name, QString& value);

#endif // CRYINCLUDE_EDITOR_OBJECTS_AICOVERSURFACE_H
