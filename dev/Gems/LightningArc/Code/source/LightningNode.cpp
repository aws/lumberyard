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
#include "LightningArc_precompiled.h"
#include "LightningNode.h"
#include "LightningGameEffectCry.h"
#include "IRenderAuxGeom.h"

namespace
{
    inline float CatmullRom(float point0, float point1, float point2, float point3, float x)
    {
        float xx = x * x;
        float xxx = xx * x;
        return
            0.5f * ((2.0f * point1) +
                    (-point0 + point2) * x +
                    (2.0f * point0 - 5.0f * point1 + 4.0f * point2 - point3) * xx +
                    (-point0 + 3.0f * point1 - 3.0f * point2 + point3) * xxx);
    }

    inline Vec3 CatmullRom(Vec3 point0, Vec3 point1, Vec3 point2, Vec3 point3, float x)
    {
        return Vec3(
            CatmullRom(point0.x, point1.x, point2.x, point3.x, x),
            CatmullRom(point0.y, point1.y, point2.y, point3.y, x),
            CatmullRom(point0.z, point1.z, point2.z, point3.z, x));
    }

    Vec3 RandomUnitSphere()
    {
        Vec3 result;
        result.x = cry_random<float>(-1.0f, 1.0f);
        result.y = cry_random<float>(-1.0f, 1.0f);
        result.z = cry_random<float>(-1.0f, 1.0f);
        result.NormalizeSafe(Vec3_OneX);
        return result;
    }
}

void CLightningRenderNode::CTriStrip::Reset()
{
    m_vertices.clear();
    m_indices.clear();
}

void CLightningRenderNode::CTriStrip::PushVertex(SLightningVertex v)
{
    int numTris = m_vertices.size() - m_firstVertex - 2;
    if (numTris < 0)
    {
        numTris = 0;
    }
    int vidx = m_vertices.size();
    if (numTris > 0)
    {
        if (numTris % 2)
        {
            m_indices.push_back(vidx - 1);
            m_indices.push_back(vidx - 2);
        }
        else
        {
            m_indices.push_back(vidx - 2);
            m_indices.push_back(vidx - 1);
        }
    }
    m_vertices.push_back(v);
    m_indices.push_back(vidx);
}

void CLightningRenderNode::CTriStrip::Branch()
{
    m_firstVertex = m_vertices.size();
}

void CLightningRenderNode::CTriStrip::Draw(const SRendParams& renderParams, const SRenderingPassInfo& passInfo,
    IRenderer* pRenderer, CRenderObject* pRenderObject, _smart_ptr<IMaterial>  pMaterial,
    float distanceToCamera)
{
    if (m_vertices.empty())
    {
        return;
    }

    bool nAfterWater = true;

    pRenderObject->m_II.m_Matrix = Matrix34(IDENTITY);
    pRenderObject->m_ObjFlags |= FOB_NO_FOG;
    pRenderObject->m_ObjFlags &= ~FOB_ALLOW_TESSELLATION;
    pRenderObject->m_nSort = fastround_positive(distanceToCamera * 2.0f);
    pRenderObject->m_fDistance = renderParams.fDistance;
    pRenderObject->m_pCurrMaterial = pMaterial;

    // For this specific object the render node is not used and should not be set.
    pRenderObject->m_pRenderNode = nullptr;

    pRenderer->EF_AddPolygonToScene(pMaterial->GetShaderItem(), m_vertices.size(), &m_vertices[0], 0, pRenderObject,
        passInfo, &m_indices[0], m_indices.size(), nAfterWater,
        SRendItemSorter(renderParams.rendItemSorter));
}

void CLightningRenderNode::CTriStrip::Clear()
{
    m_indices.clear();
    m_vertices.clear();
}

void CLightningRenderNode::CTriStrip::AddStats(LightningStats* pStats) const
{
    pStats->m_memory.Increment(m_vertices.capacity() * sizeof(SLightningVertex));
    pStats->m_memory.Increment(m_indices.capacity() * sizeof(uint16));
    pStats->m_triCount.Increment(m_indices.size() / 3);
}

void CLightningRenderNode::CSegment::Create(const LightningArcParams& desc, SPointData* pointData,
    int _parentSegmentIdx, int _parentPointIdx, Vec3 _origin, Vec3 _destany,
    float _duration, float _intensity)
{
    m_firstPoint = pointData->m_points.size();
    m_firstFuzzyPoint = pointData->m_fuzzyPoints.size();
    m_origin = _origin;
    m_destany = _destany;
    m_intensity = _intensity;
    m_duration = _duration;
    m_parentSegmentIdx = _parentSegmentIdx;
    m_parentPointIdx = _parentPointIdx;
    m_time = 0.0f;
    
    //Do not let numSegs or numSubSegs be 0
    //In ::GetPoint we have the same behavior to avoid a crash. This is just for consistency
    const int numSegs = desc.m_strikeNumSegments > 0 ? desc.m_strikeNumSegments : 1;
    const int numSubSegs = desc.m_strikeNumPoints > 0 ? desc.m_strikeNumPoints : 1;
    const int numFuzzySegs = numSegs * numSubSegs;

    pointData->m_points.push_back(Vec3(ZERO));
    pointData->m_velocity.push_back(Vec3(ZERO));
    for (int i = 1; i < numSegs; ++i)
    {
        pointData->m_points.push_back(cry_random_unit_vector<Vec3>());
        pointData->m_velocity.push_back(cry_random_unit_vector<Vec3>() + Vec3(0.0f, 0.0f, 1.0f));
    }
    pointData->m_points.push_back(Vec3(ZERO));
    pointData->m_velocity.push_back(Vec3(ZERO));

    pointData->m_fuzzyPoints.push_back(Vec3(ZERO));
    for (int i = 1; i < numFuzzySegs; ++i)
    {
        pointData->m_fuzzyPoints.push_back(cry_random_unit_vector<Vec3>());
    }
    pointData->m_fuzzyPoints.push_back(Vec3(ZERO));

    m_numPoints = pointData->m_points.size() - m_firstPoint;
    m_numFuzzyPoints = pointData->m_fuzzyPoints.size() - m_firstFuzzyPoint;
}

void CLightningRenderNode::CSegment::Update(const LightningArcParams& desc)
{
    m_time += gEnv->pTimer->GetFrameTime();
}

void CLightningRenderNode::CSegment::Draw(const LightningArcParams& desc, const SPointData& pointData, CTriStrip* strip,
    Vec3 cameraPosition, float deviationMult)
{
    float fade = 1.0f;
    if (m_time > m_duration)
    {
        fade = 1.0f - (m_time - m_duration) / (desc.m_strikeFadeOut + 0.001f);
    }
    fade *= m_intensity;
    float halfSize = fade * desc.m_beamSize;
    UCol white;
    white.dcolor = ~0;

    const float frameHeight = 1.0f / desc.m_beamTexFrames;
    const float minY = fmod_tpl(floor_tpl(m_time * desc.m_beamTexFPS), desc.m_beamTexFrames) * frameHeight;
    const float maxY = minY + frameHeight;
    float distanceTraveled = 0.0f;

    strip->Branch();

    Vec3 p0 = GetPoint(desc, pointData, 0, deviationMult);
    Vec3 p1 = GetPoint(desc, pointData, 1, deviationMult);
    Vec3 p2 = GetPoint(desc, pointData, 2, deviationMult);

    {
        Vec3 front = p0 - cameraPosition;
        Vec3 dir1 = p0 - p1;
        Vec3 up = dir1.Cross(front).GetNormalized();

        SLightningVertex v;
        v.color = white;
        v.xyz = p0 - up * halfSize;
        v.st.x = -m_time * desc.m_beamTexShift;
        v.st.y = minY;
        strip->PushVertex(v);
        v.xyz = p0 + up * halfSize;
        v.st.y = maxY;
        strip->PushVertex(v);
    }

    for (int i = 1; i < m_numFuzzyPoints - 1; ++i)
    {
        p0 = p1;
        p1 = p2;
        p2 = GetPoint(desc, pointData, i + 1, deviationMult);
        distanceTraveled += p0.GetDistance(p1);

        Vec3 front = p1 - cameraPosition;
        Vec3 dir0 = p0 - p1;
        Vec3 dir1 = p1 - p2;
        Vec3 up0 = dir0.Cross(front);
        Vec3 up1 = dir1.Cross(front);
        Vec3 up = (up0 + up1).GetNormalized();
        float t = i / float(m_numFuzzyPoints - 1);

        SLightningVertex v;
        v.color = white;
        v.xyz = p1 - up * halfSize;
        v.st.x = desc.m_beamTexTiling * distanceTraveled - m_time * desc.m_beamTexShift;
        v.st.y = minY;
        strip->PushVertex(v);
        v.xyz = p1 + up * halfSize;
        v.st.y = maxY;
        strip->PushVertex(v);
    }

    {
        p0 = p1;
        p1 = p2;
        distanceTraveled += p0.GetDistance(p1);

        Vec3 front = p1 - cameraPosition;
        Vec3 dir0 = p0 - p1;
        Vec3 up = dir0.Cross(front).GetNormalized();

        SLightningVertex v;
        v.color = white;
        v.xyz = p1 - up * halfSize;
        v.st.x = desc.m_beamTexTiling * distanceTraveled - m_time * desc.m_beamTexShift;
        v.st.y = minY;
        strip->PushVertex(v);
        v.xyz = p1 + up * halfSize;
        v.st.y = maxY;
        strip->PushVertex(v);
    }
}

bool CLightningRenderNode::CSegment::IsDone(const LightningArcParams& desc)
{
    return m_time > (m_duration + desc.m_strikeFadeOut);
}

void CLightningRenderNode::CSegment::SetOrigin(Vec3 _origin)
{
    m_origin = _origin;
}

void CLightningRenderNode::CSegment::SetDestany(Vec3 _destany)
{
    m_destany = _destany;
}

CLightningRenderNode::CLightningRenderNode()
    : m_pMaterial(0)
    , m_dirtyBBox(true)
    , m_deviationMult(1.0f)
{
}

CLightningRenderNode::~CLightningRenderNode()
{
    if (gEnv && gEnv->p3DEngine)
    {
        // IEntityRender state has a comment and assert to verify that the
        // render node state has been freed when an IRenderNode is destroyed.
        gEnv->p3DEngine->FreeRenderNodeState(this);
    }
}

const char* CLightningRenderNode::GetEntityClassName() const
{
    return "Lightning";
}

const char* CLightningRenderNode::GetName() const
{
    return "Lightning";
}

void CLightningRenderNode::Render(const struct SRendParams& rParam, const SRenderingPassInfo& passInfo)
{
    if (!m_pMaterial)
    {
        return;
    }

    IRenderer* pRenderer = gEnv->pRenderer;
    const CCamera& camera = pRenderer->GetCamera();
    CRenderObject* pRenderObject = pRenderer->EF_GetObject_Temp(passInfo.ThreadID());
    Vec3 cameraPosition = camera.GetPosition();
    float distanceToCamera = sqrt_tpl(Distance::Point_AABBSq(cameraPosition, GetBBox())) * passInfo.GetZoomFactor();

    m_triStrip.Reset();
    Update();
    Draw(&m_triStrip, cameraPosition);

    m_triStrip.Draw(rParam, passInfo, pRenderer, pRenderObject, m_pMaterial, distanceToCamera);
}

IPhysicalEntity* CLightningRenderNode::GetPhysics() const
{
    return 0;
}

void CLightningRenderNode::SetPhysics(IPhysicalEntity*)
{
}

void CLightningRenderNode::SetMaterial(_smart_ptr<IMaterial> pMat)
{
    m_pMaterial = pMat;
}

_smart_ptr<IMaterial>  CLightningRenderNode::GetMaterialOverride()
{
    return m_pMaterial;
}

void CLightningRenderNode::Precache()
{
}

void CLightningRenderNode::GetMemoryUsage(ICrySizer* pSizer) const
{
}

void CLightningRenderNode::SetBBox(const AABB& WSBBox)
{
    m_dirtyBBox = false;
    m_aabb = WSBBox;
}

bool CLightningRenderNode::IsAllocatedOutsideOf3DEngineDLL()
{
    return true;
}

void CLightningRenderNode::SetEmiterPosition(Vec3 emiterPosition)
{
    m_emmitterPosition = emiterPosition;
    m_dirtyBBox = true;
}

void CLightningRenderNode::SetReceiverPosition(Vec3 receiverPosition)
{
    m_receiverPosition = receiverPosition;
    m_dirtyBBox = true;
}

void CLightningRenderNode::SetSparkDeviationMult(float deviationMult)
{
    m_deviationMult = max(0.0f, deviationMult);
}

void CLightningRenderNode::AddStats(LightningStats* pStats) const
{
    m_triStrip.AddStats(pStats);
    int lightningMem = 0;
    lightningMem += sizeof(CLightningRenderNode);
    lightningMem += m_pointData.m_points.capacity() * sizeof(Vec3);
    lightningMem += m_pointData.m_fuzzyPoints.capacity() * sizeof(Vec3);
    lightningMem += m_pointData.m_velocity.capacity() * sizeof(Vec3);
    lightningMem += m_segments.capacity() * sizeof(CSegment);
    pStats->m_memory.Increment(lightningMem);
    pStats->m_branches.Increment(m_segments.size());
}

void CLightningRenderNode::Reset()
{
    m_dirtyBBox = true;
    m_segments.clear();
    m_pointData.m_fuzzyPoints.clear();
    m_pointData.m_points.clear();
    m_pointData.m_velocity.clear();
    m_deviationMult = 1.0f;
}

float CLightningRenderNode::TriggerSpark()
{
    m_dirtyBBox = true;
    m_deviationMult = 1.0f;

    float lightningTime = cry_random<float>(m_lightningDesc.m_strikeTimeMin, m_lightningDesc.m_strikeTimeMax);

    CreateSegment(m_emmitterPosition, -1, -1, lightningTime, 0);

    return lightningTime + m_lightningDesc.m_strikeFadeOut;
}

void CLightningRenderNode::SetLightningParams(const LightningArcParams& pDescriptor)
{
    m_lightningDesc = pDescriptor;
}

void CLightningRenderNode::Update()
{
    m_dirtyBBox = true;

    for (TSegments::iterator it = m_segments.begin(); it != m_segments.end(); ++it)
    {
        it->Update(m_lightningDesc);
    }

    for (TSegments::iterator it = m_segments.begin(); it != m_segments.end(); ++it)
    {
        if (it->GetParentSegmentIdx() != -1)
        {
            it->SetOrigin(m_segments[it->GetParentSegmentIdx()].GetPoint(m_lightningDesc, m_pointData,
                    it->GetParentPointIdx(), m_deviationMult));
        }
        else
        {
            it->SetOrigin(m_emmitterPosition);
        }
        it->SetDestany(m_receiverPosition);
    }

    while (!m_segments.empty() && m_segments[0].IsDone(m_lightningDesc))
    {
        PopSegment();
    }
}

void CLightningRenderNode::Draw(CTriStrip* strip, Vec3 cameraPosition)
{
    for (TSegments::iterator it = m_segments.begin(); it != m_segments.end(); ++it)
    {
        it->Draw(m_lightningDesc, m_pointData, strip, cameraPosition, m_deviationMult);
    }
}

void CLightningRenderNode::CreateSegment(Vec3 originPosition, int parentIdx, int parentPointIdx, float duration,
    int level)
{
    if (level > m_lightningDesc.m_branchMaxLevel)
    {
        return;
    }
    if (m_segments.size() == m_lightningDesc.m_maxNumStrikes)
    {
        return;
    }
    int segmentIdx = m_segments.size();

    CSegment segmentData;
    segmentData.Create(m_lightningDesc, &m_pointData, parentIdx, parentPointIdx, originPosition, m_receiverPosition,
        duration, 1.0f / (level + 1));
    m_segments.push_back(segmentData);

    float randVal = cry_random<float>(0.0f, max(1.0f, m_lightningDesc.m_branchProbability));
    float prob = m_lightningDesc.m_branchProbability;
    int numBranches = int(randVal);
    prob -= std::floor(prob);
    if (randVal <= prob)
    {
        numBranches++;
    }

    for (int i = 0; i < numBranches; ++i)
    {
        int point = cry_random<int>(0, (m_lightningDesc.m_strikeNumSegments * m_lightningDesc.m_strikeNumPoints) - 1);
        CreateSegment(segmentData.GetPoint(m_lightningDesc, m_pointData, point, m_deviationMult), segmentIdx, point,
            duration, level + 1);
    }
}

void CLightningRenderNode::PopSegment()
{
    m_segments.erase(m_segments.begin());
    for (TSegments::iterator it = m_segments.begin(); it != m_segments.end(); ++it)
    {
        if (it->GetParentSegmentIdx() != -1)
        {
            it->DecrementParentIdx();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
const AABB CLightningRenderNode::GetBBox() const
{
    if (m_dirtyBBox)
    {
        const float borderSize = m_lightningDesc.m_beamSize;
        AABB box(m_emmitterPosition);
        box.Add(m_receiverPosition);

        for (unsigned int i = 0; i < m_segments.size(); ++i)
        {
            const CSegment& segment = m_segments[i];
            for (int j = 0; j < segment.GetNumPoints(); ++j)
            {
                box.Add(segment.GetPoint(m_lightningDesc, m_pointData, j, m_deviationMult));
            }
        }

        box.min -= Vec3(borderSize, borderSize, borderSize);
        box.max += Vec3(borderSize, borderSize, borderSize);

        m_aabb = box;
        m_dirtyBBox = false;
    }
    return m_aabb;
}

///////////////////////////////////////////////////////////////////////////////
Vec3 CLightningRenderNode::CSegment::GetPoint(const LightningArcParams& desc, const SPointData& pointData, int point, float deviationMult) const
{
    //Do not let numSegs or numSubSegs be 0
    //Doing so would allow 2 possible divide by 0 crashes
    const int numSegs = desc.m_strikeNumSegments > 0 ? desc.m_strikeNumSegments : 1;
    const int numSubSegs = desc.m_strikeNumPoints > 0 ? desc.m_strikeNumPoints : 1;

    const float deviation = desc.m_lightningDeviation;
    const float fuzzyness = desc.m_lightningFuzziness;

    int i = point / numSubSegs;
    int j = point % numSubSegs;

    int maxIndex = static_cast<int>(pointData.m_points.size()) - 1 - m_firstPoint;

    int idx[4] =
    {
        min(max(i - 1, 0), maxIndex),
        min(min(i, maxIndex), maxIndex),
        min(min(i + 1, numSegs), maxIndex),
        min(min(i + 2, numSegs), maxIndex)
    };

    Vec3 positions[4];
    for (int l = 0; l < 4; ++l)
    {
        positions[l] = LERP(m_origin, m_destany, idx[l] / float(numSegs));
        positions[l] += pointData.m_points[m_firstPoint + idx[l]] * deviation * deviationMult;
        positions[l] += pointData.m_velocity[m_firstPoint + idx[l]] * desc.m_lightningVelocity * m_time * deviationMult;
    }

    float x = j / float(numSubSegs);
    int k = i * numSubSegs + j;
    int maxFuzzyIndex = static_cast<int>(pointData.m_fuzzyPoints.size()) - 1 - m_firstFuzzyPoint;
    k = min(k, maxFuzzyIndex);

    Vec3 result = CatmullRom(
            positions[0], positions[1],
            positions[2], positions[3], x);
    result = result + pointData.m_fuzzyPoints[m_firstFuzzyPoint + k] * fuzzyness * deviationMult;

    return result;
}

///////////////////////////////////////////////////////////////////////////////
void CLightningRenderNode::FillBBox(AABB& aabb)
{
    aabb = CLightningRenderNode::GetBBox();
}

///////////////////////////////////////////////////////////////////////////////
EERType CLightningRenderNode::GetRenderNodeType()
{
    return eERType_GameEffect;
}

///////////////////////////////////////////////////////////////////////////////
float CLightningRenderNode::GetMaxViewDist()
{
    // TODO : needs to use standard view distance ratio calculation used by other render nodes
    const float maxViewDistance = 2000.0f;
    return maxViewDistance;
}

///////////////////////////////////////////////////////////////////////////////
Vec3 CLightningRenderNode::GetPos(bool bWorldOnly) const
{
    return Vec3(ZERO);
}

///////////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial>  CLightningRenderNode::GetMaterial(Vec3* pHitPos)
{
    return m_pMaterial;
}
