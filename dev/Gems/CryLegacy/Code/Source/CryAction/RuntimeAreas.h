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

#ifndef RUNTIME_AREAS_H
#define RUNTIME_AREAS_H

class CRuntimeAreaManager
    : public ISystemEventListener
{
public:

    CRuntimeAreaManager();
    virtual ~CRuntimeAreaManager();

    void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;

private:

    typedef std::vector< EntityId > TEntityArray;

    struct SXMLTags
    {
        static char const* const sMergedMeshSurfaceTypesRoot;
        static char const* const sMergedMeshSurfaceTag;
        static char const* const sAudioTag;

        static char const* const sNameAttribute;
        static char const* const sATLTriggerAttribute;
        static char const* const sATLRtpcAttribute;
    };

    void FillAudioControls();
    void DestroyAreas();
    void CreateAreas();

    TEntityArray m_areas;
    TEntityArray m_areaObjects;
};

#endif