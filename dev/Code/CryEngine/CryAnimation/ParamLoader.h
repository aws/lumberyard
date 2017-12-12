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

// Description : The .chrparams file format is a XML file which stores all the information
//               that was stored in the .ik, .setup and .cal files before in the sections
//               <lod>, <IK_Definition> and <AnimationList>
//               The loading code is partly copy-paste from the old LoaderCHR and CalParser

#ifndef CRYINCLUDE_CRYANIMATION_PARAMLOADER_H
#define CRYINCLUDE_CRYANIMATION_PARAMLOADER_H
#pragma once

#include "ModelAnimationSet.h"  //embedded
#include "ParamNodeTypes.h"
class CDefaultSkeleton;

struct CAF_ID
{
    int32 m_nCafID;
    int32 m_nType;  //regular CAF-file or AIM-Pose (which is also a CAF file)
    CAF_ID(int32 id, int32 type)
    {
        m_nCafID = id;
        m_nType = type;
    };
};


// information about an animation to load
struct SAnimFile
{
    char m_FilePathQ[256];
    char m_AnimNameQ[256];
    uint32 m_crcAnim;

    SAnimFile(const stack_string& szFilePath, const stack_string& szAnimName);
};

class CryCHRLoader;

// holds information about a loaded animation list
// (the <AnimationList> section of a .chrparams file)
// this looks more scary than it is (memory wise)
// for a total level its about 150 kb in memory

// LDS: This is a representation of a single chrparams file
//      It is used in the load process to grab the individual files
//      for loading in LoadAnimations. The CParamLoader owns an array
//      of these for use in that function.
struct SAnimListInfo
{
    string fileName;
    DynArray<int> dependencies;

    // LDS: The LoadAnimations function only uses the first of these directories found
    //      in the ParamLoader's list of chrparams files. Subsequent entries will be discarded
    string faceLibFile;
    string faceLibDir;
    string animEventDatabase;

    // LDS: This is a list of all the animation needs for this chrparams file. The wildcards
    //      are explicitly expanded in the LoadAnimations process.
    DynArray<string> modelTracksDatabases;
    DynArray<string> lockedDatabases;
    DynArray<SAnimFile> arrAnimFiles;
    DynArray<SAnimFile> arrWildcardAnimFiles;
    CAnimationSet::FacialAnimationSet::container_type facialAnimations;

    // has the animation list been parsed and the animations been loaded?
    // then headers are available and just have to be loaded into an
    // AnimationSet. This tells us if they are in the global animation asset
    // array
    bool headersLoaded;

    // this is the only thing that really takes up memory after loading
    DynArray<ModelAnimationHeader> arrLoadedAnimFiles;

    SAnimListInfo(const char* paramFileName)
    {
        fileName.append(paramFileName);
        // reserve a large block to prevent fragmentation
        // it gets deleted once the headers are loaded anyway
        arrAnimFiles.reserve(512);
        arrWildcardAnimFiles.reserve(512);
        arrLoadedAnimFiles.reserve(0);
        headersLoaded = false;
    }
    void HeadersLoaded()
    {
        arrAnimFiles.clear();
        arrLoadedAnimFiles.reserve(arrLoadedAnimFiles.size());
        headersLoaded = true;
    }
    void PrepareHeaderList()
    {
#ifndef EDITOR_PCDEBUGCODE
        arrWildcardAnimFiles.clear();
#endif
        arrLoadedAnimFiles.reserve(arrAnimFiles.size());
    }
};

class CChrParams;

class CBoneLodNode
    : public IChrParamsBoneLodNode
{
public:
    CBoneLodNode(const CChrParams* owner);
    virtual ~CBoneLodNode();

    friend class CChrParams;

    virtual void SerializeToXml(XmlNodeRef parentNode) const;
    virtual bool SerializeFromXml(XmlNodeRef inputNode);
    virtual EChrParamNodeType GetType() const { return e_chrParamsNodeBoneLod; }

    uint GetLodCount() const { return m_boneLods.size(); }
    int GetLodLevel(uint lod) const { return m_boneLods[lod].m_level; }
    uint GetJointCount(uint lod) const { return m_boneLods[lod].m_jointNames.size(); }
    string GetJointName(uint lod, uint joint) const { return m_boneLods[lod].m_jointNames[joint]; }

protected:
    void AddBoneLod(uint level, const DynArray<string>& jointList);

private:
    struct SBoneLod
    {
        int m_level;
        DynArray<string> m_jointNames;
    };
    DynArray<SBoneLod> m_boneLods;
    const CChrParams* m_owner;
};

class CBBoxExcludeNode
    : public IChrParamsBBoxExcludeNode
{
public:
    CBBoxExcludeNode(const CChrParams* owner);
    virtual ~CBBoxExcludeNode();

    friend class CChrParams;

    virtual EChrParamNodeType GetType() const { return e_chrParamsNodeBBoxExcludeList; }
    uint GetJointCount() const { return m_jointNames.size(); }
    string GetJointName(uint jointIndex) const { return m_jointNames[jointIndex]; }

protected:
    virtual void SerializeToXml(XmlNodeRef parentNode) const;
    virtual bool SerializeFromXml(XmlNodeRef inputNode);
    void AddExcludeJoints(const DynArray<string>& jointList);

private:
    DynArray<string> m_jointNames;
    const CChrParams* m_owner;
};

class CBBoxIncludeListNode
    : public IChrParamsBBoxIncludeNode
{
public:
    CBBoxIncludeListNode(const CChrParams* owner);
    virtual ~CBBoxIncludeListNode();

    friend class CChrParams;

    virtual EChrParamNodeType GetType() const { return e_chrParamsNodeBBoxIncludeList; }
    virtual uint GetJointCount() const { return m_jointNames.size(); }
    virtual string GetJointName(uint jointIndex) const { return m_jointNames[jointIndex]; }

protected:
    virtual void SerializeToXml(XmlNodeRef parentNode) const;
    virtual bool SerializeFromXml(XmlNodeRef inputNode);
    void AddIncludeJoints(const DynArray<string>& jointList);

private:
    DynArray<string> m_jointNames;
    const CChrParams* m_owner;
};

class CBBoxExtensionNode
    : public IChrParamsBBoxExtensionNode
{
public:
    CBBoxExtensionNode(const CChrParams* owner);
    virtual ~CBBoxExtensionNode();

    friend class CChrParams;
    virtual EChrParamNodeType GetType() const { return e_chrParamsNodeBBoxExtensionList; }
    virtual void GetBBoxMaxMin(Vec3& bboxMin, Vec3& bboxMax) const;

protected:
    virtual void SerializeToXml(XmlNodeRef parentNode) const;
    virtual bool SerializeFromXml(XmlNodeRef inputNode);
    void SetBBoxExpansion(Vec3 bbMin, Vec3 bbMax);

private:
    Vec3 m_bbMin;
    Vec3 m_bbMax;
    const CChrParams* m_owner;
};

class CIKDefinitionNode;

class CLimbIKDefs
    : public IChrParamsLimbIKDef
{
public:
    CLimbIKDefs(const CChrParams* owner);
    virtual ~CLimbIKDefs();

    friend class CIKDefinitionNode;
    virtual EChrParamNodeType GetType() const { return e_chrParamsNodeIKDefinition; }
    virtual EChrParamsIKNodeSubType GetSubType() const { return e_chrParamsIKNodeLimb; }

    uint GetLimbIKDefCount() const { return m_limbIKDefs.size(); }
    const SLimbIKDef* GetLimbIKDef(uint limbIKIndex) const { return &m_limbIKDefs[limbIKIndex]; }
    void SetLimbIKDefs(const DynArray<SLimbIKDef>& newLimbIKDefs){ m_limbIKDefs = newLimbIKDefs; }

protected:
    virtual void SerializeToXml(XmlNodeRef parentNode) const;
    virtual bool SerializeFromXml(XmlNodeRef inputNode);

private:
    DynArray<SLimbIKDef> m_limbIKDefs;
    const CChrParams* m_owner;
};

class CAnimDrivenIKDefs
    : public IChrParamsAnimDrivenIKDef
{
public:
    CAnimDrivenIKDefs(const CChrParams* owner);
    virtual ~CAnimDrivenIKDefs();

    friend class CIKDefinitionNode;
    virtual EChrParamNodeType GetType() const { return e_chrParamsNodeIKDefinition; }
    virtual EChrParamsIKNodeSubType GetSubType() const { return e_chrParamsIKNodeAnimDriven; }

    uint GetAnimDrivenIKDefCount() const { return m_animDrivenIKDefs.size(); }
    const SAnimDrivenIKDef* GetAnimDrivenIKDef(uint animDrivenIKIndex) const { return &m_animDrivenIKDefs[animDrivenIKIndex]; }
    void SetAnimDrivenIKDefs(const DynArray<SAnimDrivenIKDef>& newAnimDrivenIKDefs){ m_animDrivenIKDefs = newAnimDrivenIKDefs; }

protected:
    virtual void SerializeToXml(XmlNodeRef parentNode) const;
    virtual bool SerializeFromXml(XmlNodeRef inputNode);

private:
    DynArray<SAnimDrivenIKDef> m_animDrivenIKDefs;
    const CChrParams* m_owner;
};

class CFootLockIKDefs
    : public IChrParamsFootLockIKDef
{
public:
    CFootLockIKDefs(const CChrParams* owner);
    virtual ~CFootLockIKDefs();

    friend class CIKDefinitionNode;
    virtual EChrParamNodeType GetType() const { return e_chrParamsNodeIKDefinition; }
    virtual EChrParamsIKNodeSubType GetSubType() const { return e_chrParamsIKNodeAnimDriven; }

    uint GetFootLockIKDefCount() const { return m_footLockDefs.size(); }
    const string GetFootLockIKDef(uint jointIndex) const { return m_footLockDefs[jointIndex]; }
    void SetFootLockIKDefs(const DynArray<string>& newFootLockDefs){ m_footLockDefs = newFootLockDefs; }

protected:
    virtual void SerializeToXml(XmlNodeRef parentNode) const;
    virtual bool SerializeFromXml(XmlNodeRef inputNode);

private:
    DynArray<string> m_footLockDefs;
    const CChrParams* m_owner;
};

class CRecoilIKDef
    : public IChrParamsRecoilIKDef
{
public:
    CRecoilIKDef(const CChrParams* owner);
    virtual ~CRecoilIKDef();

    friend class CIKDefinitionNode;
    virtual EChrParamNodeType GetType() const { return e_chrParamsNodeIKDefinition; }
    virtual EChrParamsIKNodeSubType GetSubType() const { return e_chrParamsIKNodeRecoil; }

    string GetRightIKHandle() const { return m_rightIKHandle; }
    void SetRightIKHandle(string newRightIKHandle){ m_rightIKHandle = newRightIKHandle; }

    string GetLeftIKHandle() const { return m_leftIKHandle; }
    void SetLeftIKHandle(string newLeftIKHandle){ m_leftIKHandle = newLeftIKHandle; }

    string GetRightWeaponJoint() const { return m_rightWeaponJoint; }
    void SetRightWeaponJoint(string newRightWeaponJoint){ m_rightWeaponJoint = newRightWeaponJoint; }

    string GetLeftWeaponJoint() const { return m_leftWeaponJoint; }
    void SetLeftWeaponJoint(string newLeftWeaponJoint){ m_leftWeaponJoint = newLeftWeaponJoint; }

    uint GetImpactJointCount() const { return m_impactJoints.size(); }
    const SRecoilImpactJoint* GetImpactJoint(uint impactJointIndex) const { return &m_impactJoints[impactJointIndex]; }
    void SetImpactJoints(const DynArray<SRecoilImpactJoint>& newImpactJoints){ m_impactJoints = newImpactJoints; }

protected:
    virtual void SerializeToXml(XmlNodeRef parentNode) const;
    virtual bool SerializeFromXml(XmlNodeRef inputNode);

private:
    string m_rightIKHandle;
    string m_leftIKHandle;
    string m_rightWeaponJoint;
    string m_leftWeaponJoint;
    DynArray<SRecoilImpactJoint> m_impactJoints;
    const CChrParams* m_owner;
};

template<typename T>
class CAimLookBase
    : public T
{
public:
    CAimLookBase(const CChrParams* owner)
        : m_owner(owner) {};
    virtual ~CAimLookBase();

    friend class CIKDefinitionNode;

    uint GetDirectionalBlendCount() const { return m_directionalBlends.size(); }
    const SAimLookDirectionalBlend* GetDirectionalBlend(uint directionalBlendIndex) const { return &m_directionalBlends[directionalBlendIndex]; }
    void SetDirectionalBlends(const DynArray<SAimLookDirectionalBlend>& newDirectionalBlends){ m_directionalBlends = newDirectionalBlends; }

    uint GetRotationCount() const { return m_rotationList.size(); }
    const SAimLookPosRot* GetRotation(uint rotationIndex) const { return &m_rotationList[rotationIndex]; }
    void SetRotationList(const DynArray<SAimLookPosRot>& newRotationList){ m_rotationList = newRotationList; }

    uint GetPositionCount() const { return m_positionList.size(); }
    const SAimLookPosRot* GetPosition(uint positionIndex) const { return &m_positionList[positionIndex]; }
    void SetPositionList(const DynArray<SAimLookPosRot>& newPositionList){ m_positionList = newPositionList; }

protected:
    virtual void SerializeToXml(XmlNodeRef parentNode) const;
    virtual bool SerializeFromXml(XmlNodeRef inputNode) = 0;
    virtual bool HandleUnknownNode(XmlNodeRef inputNode);
    DynArray<SAimLookDirectionalBlend> m_directionalBlends;
    DynArray<SAimLookPosRot> m_rotationList;
    DynArray<SAimLookPosRot> m_positionList;
    const CChrParams* m_owner;

private:
    bool SerializeDirectionalBlendsFromXml(XmlNodeRef dirBlendsNode);
    bool SerializeRotationListFromXml(XmlNodeRef rotationListNode);
    bool SerializePositionListFromXml(XmlNodeRef positionListNode);
};

class CLookIKDef
    : public CAimLookBase<IChrParamsLookIKDef>
{
public:
    CLookIKDef(const CChrParams* owner);
    virtual ~CLookIKDef();

    friend class CIKDefinitionNode;
    virtual EChrParamNodeType GetType() const { return e_chrParamsNodeIKDefinition; }
    virtual EChrParamsIKNodeSubType GetSubType() const { return e_chrParamsIKNodeLook; }

    string GetLeftEyeAttachment() const { return m_leftEyeAttachment; }
    void SetLeftEyeAttachment(string newLeftEyeAttachment){ m_leftEyeAttachment = newLeftEyeAttachment; }
    string GetRightEyeAttachment() const { return m_rightEyeAttachment; }
    void SetRightEyeAttachment(string newRightEyeAttachment){ m_rightEyeAttachment = newRightEyeAttachment; }
    const SEyeLimits* GetEyeLimits() const { return &m_eyeLimits; }
    void SetEyeLimits(const SEyeLimits& newEyeLimits){ m_eyeLimits = newEyeLimits; }

protected:
    virtual void SerializeToXml(XmlNodeRef parentNode) const;
    virtual bool SerializeFromXml(XmlNodeRef inputNode);

private:
    string m_leftEyeAttachment;
    string m_rightEyeAttachment;
    SEyeLimits m_eyeLimits;
};

class CAimIKDef
    : public CAimLookBase<IChrParamsAimIKDef>
{
public:
    CAimIKDef(const CChrParams* owner);
    virtual ~CAimIKDef();

    friend class CIKDefinitionNode;
    virtual EChrParamNodeType GetType() const { return e_chrParamsNodeIKDefinition; }
    virtual EChrParamsIKNodeSubType GetSubType() const { return e_chrParamsIKNodeAim; }

    uint GetProcAdjustmentCount() const { return m_procAdjustmentJoints.size(); }
    string GetProcAdjustmentJoint(uint jointIndex) const { return m_procAdjustmentJoints[jointIndex]; }
    void SetProcAdjustments(const DynArray<string>& newProcAdjustments){ m_procAdjustmentJoints = newProcAdjustments; }

protected:
    virtual void SerializeToXml(XmlNodeRef parentNode) const;
    virtual bool SerializeFromXml(XmlNodeRef inputNode);

private:
    DynArray<string> m_procAdjustmentJoints;
};

class CPhysProxyBBoxOptionNode
    : public IChrParamsPhysProxyBBoxOptionNode
{
public:
    CPhysProxyBBoxOptionNode(const CChrParams* owner);
    virtual ~CPhysProxyBBoxOptionNode();

    virtual bool GetUsePhysProxyBBoxNode() const;
    virtual void SetUsePhysProxyBBoxNode(bool value);

protected:
    virtual void SerializeToXml(XmlNodeRef parentNode) const;
    virtual bool SerializeFromXml(XmlNodeRef inputNode);
    virtual EChrParamNodeType GetType() const;

private:
    bool m_usePhysProxyBBoxNode;
    const CChrParams* m_owner;
};

class CIKDefinitionNode
    : public IChrParamsIKDefNode
{
public:
    CIKDefinitionNode(const CChrParams* owner);
    virtual ~CIKDefinitionNode();

    virtual EChrParamNodeType GetType() const { return e_chrParamsNodeIKDefinition; }

    friend class CChrParams;

    bool HasIKSubNodeType(EChrParamsIKNodeSubType type) const;
    const IChrParamsNode* GetIKSubNode(EChrParamsIKNodeSubType type) const { return m_ikSubTypeNodes[type]; }
    IChrParamsNode* GetEditableIKSubNode(EChrParamsIKNodeSubType type);

protected:
    virtual void SerializeToXml(XmlNodeRef parentNode) const;
    virtual bool SerializeFromXml(XmlNodeRef inputNode);

private:
    void CreateNewIKSubNode(EChrParamsIKNodeSubType type);

    IChrParamsNode* m_ikSubTypeNodes[e_chrParamsIKNodeSubTypeCount];
    const CChrParams* m_owner;
};

class CAnimationListNode
    : public IChrParamsAnimationListNode
{
public:
    CAnimationListNode(CChrParams* owner);
    virtual ~CAnimationListNode();

    friend class CChrParams;

    virtual EChrParamNodeType GetType() const { return e_chrParamsNodeAnimationList; }
    virtual uint GetAnimationCount() const { return m_parsedAnimations.size(); }
    virtual const SAnimNode* GetAnimation(uint i) const { return &m_parsedAnimations[i]; }

protected:
    virtual void SerializeToXml(XmlNodeRef parentNode) const;
    virtual bool SerializeFromXml(XmlNodeRef inputNode);
    void AddAnimations(const DynArray<SAnimNode>& animations);

private:
    bool IsAnimationWildcard(const char* const path);
    EChrParamsAnimationNodeType GetAnimationNodeType(const char* const name, const char* const path);
    DynArray<SAnimNode> m_parsedAnimations;
    DynArray<const CChrParams*> m_includedChrParams;
    const CChrParams* m_owner;
};

class CParamLoader;

class CChrParams
    : public IChrParams
{
public:
    explicit CChrParams(string name, CParamLoader* paramLoader);
    ~CChrParams();

    string GetName() const { return m_name; }

    const IChrParamsNode* GetCategoryNode(EChrParamNodeType type) const { return m_categoryNodes[type]; }
    IChrParamsNode* GetEditableCategoryNode(EChrParamNodeType type);

    void ClearLists();
    void ClearCategoryNode(EChrParamNodeType type);

    bool AddNode(XmlNodeRef newNode, EChrParamNodeType type);

    // Used by editing tools and unit tests
    void AddBoneLod(uint level, const DynArray<string>& jointNames);
    void AddBBoxExcludeJoints(const DynArray<string>& jointNames);
    void AddBBoxIncludeJoints(const DynArray<string>& jointNames);
    void SetBBoxExpansion(Vec3 bbMin, Vec3 bbMax);
    void AddAnimations(const DynArray<SAnimNode>& animations);
    void SetUsePhysProxyBBox(bool value);

    bool SerializeToFile() const;

    CParamLoader* GetParamLoader() const { return m_paramLoader; }

private:
    IChrParamsNode* CreateCategoryNodeIfNeeded(EChrParamNodeType type);
    string m_name;
    IChrParamsNode* m_categoryNodes[e_chrParamsNodeCount];
    CParamLoader* m_paramLoader;
};

class CParamLoader
    : public IChrParamsParser
{
public:
    CParamLoader();
    ~CParamLoader(void);

    void ClearLists();

    const IChrParams* GetChrParams(const char* const paramFileName, const std::string* optionalBuffer = nullptr) override;
    bool LoadXML(CDefaultSkeleton* pDefaultSkeleton, string defaultAnimDir, const char* const paramFileName, DynArray<uint32>& listIDs, const std::string* optionalBuffer = nullptr);
    bool WriteXML(const char* const paramFileName) override;
    bool ExpandWildcards(uint32 listID);

    const SAnimListInfo& GetParsedList(const uint32 i) { return m_parsedLists[i]; };
    SAnimListInfo& GetParsedListWritable(const uint32 i) { return m_parsedLists[i]; };
    uint32 GetParsedListNumber() { return m_parsedLists.size(); };

    void PrepareHeaderList(uint32 listID) { m_parsedLists[listID].PrepareHeaderList(); };
    void SetHeadersLoaded(uint32 listID) { m_parsedLists[listID].HeadersLoaded(); };
    void AddLoadedHeader(uint32 listID, const ModelAnimationHeader& modelAnimationHeader) { m_parsedLists[listID].arrLoadedAnimFiles.push_back(modelAnimationHeader); }

    // Parse Step
    IChrParams* ParseXML(const char* const paramFileName, const std::string* optionalBuffer = nullptr);

    IChrParams* ClearChrParams(const char* const paramFileName) override;
    IChrParams* ClearChrParams(const char* const paramFileName, EChrParamNodeType type) override;
    IChrParams* GetEditableChrParams(const char* const paramFileName) override;
    void DeleteParsedChrParams(const char* const paramFileName) override;
    bool IsChrParamsParsed(const char* const paramFileName) override;

    bool IsAnimationWithinFilters(const char* animationPath, uint32 listID);

private:

    // Load Step
    bool LoadCategoryType(const CChrParams* const chrParams, EChrParamNodeType type);
    bool LoadRuntimeData(const CChrParams* rootChrParams, DynArray<uint32>& listIDs);
    bool LoadBoneLod(const CBoneLodNode* const boneLodNode);
    bool LoadBBoxExclusionList(const CBBoxExcludeNode* const bboxExcludeNode);
    bool LoadBBoxInclusionList(const CBBoxIncludeListNode* const bboxIncludeNode);
    bool LoadBBoxExtension(const CBBoxExtensionNode* const bboxExtensionNode);
    bool LoadUsePhysProxyBBox(const CPhysProxyBBoxOptionNode* const usePhysProxyBBoxNode);
    bool LoadIKDefs(const CIKDefinitionNode* const ikDefinitionNode);
    bool LoadIKDefLimbIK(const CLimbIKDefs* limbIKDefs);
    bool LoadIKDefAnimDrivenIKTargets(const CAnimDrivenIKDefs* animDrivenIKDefs);
    bool LoadIKDefFeetLock(const CFootLockIKDefs* footLockIKDefs);
    bool LoadIKDefRecoil(const CRecoilIKDef* recoilIKDef);
    uint32 LoadAimLookIKRotationList(const IChrParamsAimLookBase* aimLookIKDef, DynArray<SJointsAimIK_Rot>& runtimeList);
    uint32 LoadAimLookIKPositionList(const IChrParamsAimLookBase* aimLookIKDef, DynArray<SJointsAimIK_Pos>& runtimeList);
    uint32 LoadAimLookIKDirectionalBlends(const IChrParamsAimLookBase* aimLookIKDef, DynArray<DirectionalBlends>& runtimeList);
    void UpdateSharedPosRotJoints(const DynArray<SJointsAimIK_Pos>& positionList, DynArray<SJointsAimIK_Rot>& rotationList);
    bool LoadIKDefLookIK(const CLookIKDef* lookIKDef);
    bool LoadIKDefAimIK(const CAimIKDef* aimIKDef);
    int32 LoadAnimList(const CAnimationListNode* const animListNode, const char* const paramFileName, string& strAnimDirName);

    // Load Helpers
    void ExpandWildcardsForPath(SAnimListInfo& animList, const char* szMask, uint32 crcFolder, const char* szAnimNamePre, const char* szAnimNamePost);
    bool BuildDependencyList(int rootListID, DynArray<uint32>& listIDs);
    CAF_ID MemFindFirst(const char** ppAnimPath, const char* szMask, uint32 crcFolder, CAF_ID nCafID);

    int32 ListProcessed(const char* paramFileName);

    // helper functions for interfacing SAnimListInfo
    bool AddIfNewAnimationAlias(SAnimListInfo& animList, const char* animName, const char* szFileName);
    const SAnimFile* FindAnimationAliasInDependencies(SAnimListInfo& animList, const char* szFileName);

    bool AddIfNewFacialAnimationAlias(SAnimListInfo& animList,  const char* animName, const char* szFileName);
    bool NoFacialAnimationAliasInDependencies(SAnimListInfo& animList, const char* animName);
    bool AddIfNewModelTracksDatabase(SAnimListInfo& animList, const char* dataBase);
    bool NoModelTracksDatabaseInDependencies(SAnimListInfo& animList, const char* dataBase);

    DynArray<CChrParams*> m_parsedChrParams;
    DynArray<SAnimListInfo> m_parsedLists;
    CDefaultSkeleton* m_pDefaultSkeleton;
    string m_defaultAnimDir;
};

#endif // CRYINCLUDE_CRYANIMATION_PARAMLOADER_H
