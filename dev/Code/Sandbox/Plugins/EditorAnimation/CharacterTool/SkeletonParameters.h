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

#pragma once

#include <vector>
#include "../Shared/Strings.h"
#include "Serialization.h"

class XmlNodeRef;

namespace Serialization {
    class IArchive;
}

namespace CharacterTool
{
    using std::vector;

    struct AnimationFilterWildcard
    {
        string renameMask;
        string fileWildcard;

        AnimationFilterWildcard()
            : renameMask("*")
            , fileWildcard("*/*.caf")
        {
        }

        void Serialize(IArchive& ar)
        {
            ar(renameMask, "renameMask", "^");
            ar(fileWildcard, "fileWildcard", "^ <- ");
        }

        bool operator==(const AnimationFilterWildcard& rhs) const
        {
            return renameMask == rhs.renameMask && fileWildcard == rhs.fileWildcard;
        }
        bool operator!=(const AnimationFilterWildcard& rhs) const{ return !operator==(rhs); }
    };

    struct AnimationFilterFolder
    {
        string path;
        vector<AnimationFilterWildcard> wildcards;

        AnimationFilterFolder()
        {
            wildcards.resize(3);
            wildcards[0].fileWildcard = "*/*.caf";
            wildcards[1].fileWildcard = "*/*.bspace";
            wildcards[2].fileWildcard = "*/*.comb";
        }

        bool operator==(const AnimationFilterFolder& rhs) const
        {
            if (path != rhs.path)
            {
                return false;
            }
            if (wildcards.size() != rhs.wildcards.size())
            {
                return false;
            }
            for (size_t i = 0; i < wildcards.size(); ++i)
            {
                if (wildcards[i] != rhs.wildcards[i])
                {
                    return false;
                }
            }
            return true;
        }
        bool operator!=(const AnimationFilterFolder& rhs) const{ return !operator==(rhs); }

        void Serialize(IArchive& ar)
        {
            ar(ResourceFolderPath(path), "path", "^");
            ar(wildcards, "wildcards", "^");
        }
    };

    struct AnimationSetFilter
    {
        std::vector<AnimationFilterFolder> folders;

        void Serialize(IArchive& ar)
        {
            ar(folders, "folders", "^");
        }

        bool Matches(const char* cafPath) const;

        bool operator==(const AnimationSetFilter& rhs) const
        {
            if (folders.size() != rhs.folders.size())
            {
                return false;
            }
            for (size_t i = 0; i < folders.size(); ++i)
            {
                if (folders[i] != rhs.folders[i])
                {
                    return false;
                }
            }
            return true;
        }
        bool operator!=(const AnimationSetFilter& rhs) const{ return !operator==(rhs); }
    };

    struct SkeletonParametersInclude
    {
        string filename;
    };

    inline bool Serialize(IArchive& ar, SkeletonParametersInclude& ref, const char* name, const char* label)
    {
        return ar(SkeletonParamsPath(ref.filename), name, label);
    }

    struct SkeletonParametersDBA
    {
        string filename;
        bool persistent;

        SkeletonParametersDBA()
            : persistent(false)
        {
        }

        void Serialize(IArchive& ar)
        {
            ar(ResourceFilePath(filename, "Animation Database"), "filename", "^");
            ar(persistent, "persistent", "^Persistent");
        }
    };

    struct BBoxExtension
    {
        Vec3 pos;
        Vec3 neg;

        BBoxExtension();

        void Serialize(IArchive& ar);
        void Clear();
    };

    struct BBoxIncludes
    {
        bool m_initialized;
        std::vector<string> joints;

        BBoxIncludes();

        void Serialize(IArchive& ar);
        void Clear();
    };

    struct LODJoints
    {
        std::vector<string> joints;

        LODJoints();

        void Serialize(IArchive& ar);
    };

    struct LODJointsList
    {
        static const int s_maxLevels = 6;

        std::vector<LODJoints> levels;

        void LoadFromXML(const XmlNodeRef& node);
        void SaveToXML(const XmlNodeRef& node) const;

        void Serialize(IArchive& ar);
        void Clear();

        void InitializeNewLevels();
    };

    struct IKDefinition
    {
        struct Rotation
        {
            string joint;
            bool additive;
            bool primary;

            Rotation(const string& joint = "");

            void Serialize(IArchive& ar);
        };

        struct RotationList
        {
            std::vector<Rotation> rotations;

            void LoadFromXML(const XmlNodeRef& node);
            void SaveToXML(const XmlNodeRef& node) const;

            void Serialize(IArchive& ar);
            void Clear();
        };

        struct Position
        {
            string joint;
            bool additive;

            Position(const string& joint = "");

            void Serialize(IArchive& ar);
        };

        struct PositionList
        {
            std::vector<Position> positions;

            void LoadFromXML(const XmlNodeRef& node);
            void SaveToXML(const XmlNodeRef& node) const;

            void Serialize(IArchive& ar);
            void Clear();
        };

        struct LimbIKEntry
        {
            static const int s_maxHandleLength = 8;

            enum class Solver
            {
                TwoBone,
                ThreeBone,
                CCDX
            };

            Solver solver;
            string handle;
            string root;
            string endEffector;
            float stepSize;
            float threshold;
            int maxIteration;

            LimbIKEntry();

            void Serialize(IArchive& ar);
        };

        struct LimbIK
        {
            bool enabled;
            vector<LimbIKEntry> entries;

            LimbIK();

            void LoadFromXML(const XmlNodeRef& node);
            void SaveToXML(const XmlNodeRef& node) const;

            void Serialize(IArchive& ar);
            void Clear();
        };

        struct DirectionalBlend
        {
            string animToken;
            string parameterJoint;
            string startJoint;
            string referenceJoint;

            void Serialize(IArchive& ar);
        };

        struct DirectionalBlendList
        {
            bool m_enforceStartEqualsParameterJoint;
            vector<DirectionalBlend> directionalBlends;

            DirectionalBlendList(bool enforceStartEqualsParameterJoint);

            void LoadFromXML(const XmlNodeRef& node);
            void SaveToXML(const XmlNodeRef& node) const;

            void Serialize(IArchive& ar);
            void Clear();
        };

        struct AimIK
        {
            bool enabled;

            DirectionalBlendList directionalBlends;
            RotationList rotations;
            PositionList positions;

            AimIK();

            void LoadFromXML(const XmlNodeRef& node);
            void SaveToXML(const XmlNodeRef& node) const;

            void Serialize(IArchive& ar);
            void Clear();
        };

        struct LookIK
        {
            bool enabled;

            DirectionalBlendList directionalBlends;
            RotationList rotations;
            PositionList positions;
            bool leftEyeAttachmentEnabled;
            string leftEyeAttachment;
            bool rightEyeAttachmentEnabled;
            string rightEyeAttachment;

            LookIK();

            void LoadFromXML(const XmlNodeRef& node);
            void SaveToXML(const XmlNodeRef& node) const;

            void Serialize(IArchive& ar);
            void Clear();
        };

        struct ImpactJoint
        {
            string joint;
            float arm;
            float delay;
            float weight;

            ImpactJoint(const string& joint);

            void Serialize(IArchive& ar);
        };

        struct RecoilIK
        {
            bool enabled;

            string leftHandle;
            string rightHandle;
            string leftWeaponJoint;
            string rightWeaponJoint;
            vector<ImpactJoint> impactJoints;

            RecoilIK();

            void LoadFromXML(const XmlNodeRef& node);
            void SaveToXML(const XmlNodeRef& node) const;

            void Serialize(IArchive& ar);
            void Clear();
        };

        struct FeetLockIK
        {
            bool enabled;

            string leftHandle;
            string rightHandle;

            FeetLockIK();

            void LoadFromXML(const XmlNodeRef& node);
            void SaveToXML(const XmlNodeRef& node) const;

            void Serialize(IArchive& ar);
            void Clear();
        };

        struct AnimationDrivenIKTarget
        {
            string handle;
            string target;
            string weight;

            void Serialize(IArchive& ar);
        };

        struct AnimationDrivenIKTargetList
        {
            bool enabled;

            vector<AnimationDrivenIKTarget> targets;

            AnimationDrivenIKTargetList();

            void LoadFromXML(const XmlNodeRef& node);
            void SaveToXML(const XmlNodeRef& node) const;

            void Serialize(IArchive& ar);
            void Clear();
        };

        LimbIK limbIK;
        AimIK aimIK;
        LookIK lookIK;
        RecoilIK recoilIK;
        FeetLockIK feetLockIK;
        AnimationDrivenIKTargetList animationDrivenIKTargets;

        void LoadFromXML(const XmlNodeRef& node);
        void SaveToXML(const XmlNodeRef& node) const;

        void Serialize(IArchive& ar);
        void Clear();

        bool HasEnabledDefinitions() const;
    };

    struct SkeletonParameters
    {
        string skeletonFileName;
        vector<XmlNodeRef> unknownNodes;

        AnimationSetFilter animationSetFilter;
        vector<SkeletonParametersInclude> includes;
        string animationEventDatabase;
        string faceLibFile;

        string dbaPath;
        vector<SkeletonParametersDBA> individualDBAs;

        BBoxIncludes bboxIncludes;
        BBoxExtension bboxExtension;
        IKDefinition ikDefinition;

        LODJointsList jointLods;

        bool LoadFromXMLFile(const char* filename, bool* dataLost = 0);
        bool LoadFromXML(XmlNodeRef xml, bool* dataLost = 0);
        XmlNodeRef SaveToXML();
        bool SaveToMemory(vector<char>* buffer);

        void Serialize(Serialization::IArchive& ar);
    };
}
