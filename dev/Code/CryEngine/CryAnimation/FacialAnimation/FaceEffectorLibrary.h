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

#ifndef CRYINCLUDE_CRYANIMATION_FACIALANIMATION_FACEEFFECTORLIBRARY_H
#define CRYINCLUDE_CRYANIMATION_FACIALANIMATION_FACEEFFECTORLIBRARY_H
#pragma once

#include "FaceEffector.h"

class CFacialAnimation;

//////////////////////////////////////////////////////////////////////////
class CFacialEffectorsLibrary
    : public IFacialEffectorsLibrary
{
public:
    CFacialEffectorsLibrary(CFacialAnimation* pFaceAnim);
    ~CFacialEffectorsLibrary();

    //////////////////////////////////////////////////////////////////////////
    // IFacialEffectorsLibrary interface.
    //////////////////////////////////////////////////////////////////////////
    virtual void AddRef() { ++m_nRefCounter; }
    virtual void Release()
    {
        if (--m_nRefCounter <= 0)
        {
            delete this;
        }
    }

    virtual void SetName(const char* name) { m_name = name; };
    virtual const char* GetName() { return m_name.c_str(); };

    virtual IFacialEffector* Find(CFaceIdentifierHandle ident);
    virtual IFacialEffector* Find(const char* identStr);
    virtual IFacialEffector* GetRoot();

    virtual void VisitEffectors(IFacialEffectorsLibraryEffectorVisitor* pVisitor);

    IFacialEffector* CreateEffector(EFacialEffectorType nType, CFaceIdentifierHandle ident);
    IFacialEffector* CreateEffector(EFacialEffectorType nType, const char* identStr);
    virtual void RemoveEffector(IFacialEffector* pEffector);

    virtual void Serialize(XmlNodeRef& node, bool bLoading);
    virtual void SerializeEffector(IFacialEffector* pEffector, XmlNodeRef& node, bool bLoading);
    //////////////////////////////////////////////////////////////////////////

    int GetLastIndex() const { return m_nLastIndex; }
    void RenameEffector(CFacialEffector* pEffector, CFaceIdentifierHandle ident);

    CFacialEffector* GetEffectorFromIndex(int nIndex);

    void MergeLibrary(IFacialEffectorsLibrary* pMergeLibrary, const Functor1wRet<const char*, MergeCollisionAction>& collisionStrategy);

    int GetEffectorCount() const { return m_crcToEffectorMap.size(); }

    void GetMemoryUsage(ICrySizer* pSizer) const;
private:
    void NewRoot();
    void SerializeEffectorSelf(CFacialEffector* pEffector, XmlNodeRef& node, bool bLoading);
    void SerializeEffectorSubCtrls(CFacialEffector* pEffector, XmlNodeRef& node, bool bLoading);
    void SetEffectorIndex(CFacialEffector* pEffector);
    void ReplaceEffector(CFacialEffector* pOriginalEffector, CFacialEffector* pNewEffector);

private:
    friend class CFacialModel;

    int m_nRefCounter;
    string m_name;

    CFacialAnimation* m_pFacialAnim;

    typedef std::map<uint32, _smart_ptr<CFacialEffector> > CrcToEffectorMap;
    CrcToEffectorMap m_crcToEffectorMap;

    _smart_ptr<CFacialEffector> m_pRoot;

    std::vector<_smart_ptr<CFacialEffector> > m_indexToEffector;

    int m_nLastIndex;
};

#endif // CRYINCLUDE_CRYANIMATION_FACIALANIMATION_FACEEFFECTORLIBRARY_H
