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

#ifndef CRYINCLUDE_CRYANIMATION_ATTACHMENTVCLOTH_H
#define CRYINCLUDE_CRYANIMATION_ATTACHMENTVCLOTH_H
#pragma once

class CCharInstance;
class CClothProxies;
class CAttachmentManager;
struct SVertexAnimationJob;
class CClothPhysics;
struct SBuffers;

#include "AttachmentBase.h"
#include "Vertex/VertexData.h"
#include "Vertex/VertexAnimation.h"
#include "ModelSkin.h"
#include "SkeletonPose.h"
#include <AzCore/Casting/numeric_cast.h>



#ifdef CLOTH_SSE
struct Mask
{
    __m128 q;

    Mask() { }

    Mask(type_zero)
    {
        q = _mm_setzero_ps();
    }

    int GetPacked()
    {
        return _mm_movemask_ps(q);
    }

    bool AllSet()
    {
        return _mm_movemask_ps(q) == 0xf;
    }

    bool AnySet()
    {
        return _mm_movemask_ps(q) != 0;
    }

    void And(const Mask& m)
    {
        q = _mm_and_ps(q, m.q);
    }

    void AndNot(const Mask& m)
    {
        q = _mm_andnot_ps(m.q, q);
    }

    void Or(const Mask& m)
    {
        q = _mm_or_ps(q, m.q);
    }
};

static const __m128 s_Vec3Mask = _mm_castsi128_ps(_mm_setr_epi32(-1, -1, -1, 0));

struct Vector4
{
    union
    {
        __m128 q;
        float f[4];
        struct
        {
            float x, y, z, w;
        };
    };

    Vector4() { }

    Vector4(type_zero)
    {
        q = _mm_setzero_ps();
    }

    Vector4(float x, float y, float z)
    {
        q = _mm_set_ps(0, z, y, x);
    }

    Vector4(float x, float y, float z, float w)
    {
        q = _mm_set_ps(w, z, y, x);
    }

    explicit Vector4(float s)
    {
        q = _mm_set_ps1(s);
    }

    explicit Vector4(Vec3 v)
    {
        q = _mm_set_ps(0, v.z, v.y, v.x);
    }

    void Load(const float* v)
    {
        q = _mm_set_ps(0, v[2], v[1], v[0]);

        // working alternative and potentially faster but not verified; load 4 floats, mask out w, reading accross page boundaries shouldn't be an issue

        //static const _MS_ALIGN(16) uint wmask[4] = { 0u, 0u, 0u, ~0u };
        //const __m128 w = _mm_set_ps(0.f, 0.f, 0.f, 0.f);
        //q = _mm_loadu_ps(v);
        //q = _mm_blendv_ps(q, w, *(const __m128 *)&wmask);
    }

    Vec3 ToVec3() const
    {
        return Vec3(x, y, z);
    }

    void StoreAligned(float* v)
    {
        _mm_store_ps(v, q);
    }

    void zero()
    {
        q = _mm_setzero_ps();
    }

    void Set(float x, float y, float z)
    {
        q = _mm_set_ps(0, z, y, x);
    }

    __m128 Dot(const Vector4& v) const
    {
#ifdef WIN64
        // TODO: ensure w is 0 before?
        __m128 q1 = _mm_and_ps(s_Vec3Mask, q);
        __m128 q2 = _mm_and_ps(s_Vec3Mask, v.q);
        __m128 r1 = _mm_mul_ps(q1, q2);
        __m128 r2 = _mm_hadd_ps(r1, r1);
        return _mm_hadd_ps(r2, r2);
#else
        return _mm_dp_ps(q, v.q, 0x7F);
#endif
    }

    float len2() const
    {
        return _mm_cvtss_f32(Dot(*this));
    }

    float len() const
    {
        return _mm_cvtss_f32(_mm_sqrt_ss(Dot(*this)));
    }

    Vector4& normalizeAccurate()
    {
        q = _mm_div_ps(q, _mm_sqrt_ps(Dot(*this)));
        return *this;
    }

    Vector4& normalize()
    {
        q = _mm_mul_ps(q, _mm_rsqrt_ps(Dot(*this)));
        return *this;
    }

    Vector4 normalizedAccurate() const
    {
        Vector4 v = *this;
        v.normalizeAccurate();
        return v;
    }

    Vector4 normalized() const
    {
        Vector4 v = *this;
        v.normalize();
        return v;
    }

    Vector4& operator *=(float s)
    {
        q = _mm_mul_ps(q, _mm_set_ps1(s));
        return *this;
    }

    Vector4& operator -=(const Vector4& v)
    {
        q = _mm_sub_ps(q, v.q);
        return *this;
    }

    Vector4& operator +=(const Vector4& v)
    {
        q = _mm_add_ps(q, v.q);
        return *this;
    }

    Mask CompareGt(float s)
    {
        Mask m;
        m.q = _mm_cmpgt_ps(q, _mm_set_ps1(s));
        return m;
    }

    Mask CompareGt(const Vector4& v)
    {
        Mask m;
        m.q = _mm_cmpgt_ps(q, v.q);
        return m;
    }

    Mask CompareLt(float s)
    {
        Mask m;
        m.q = _mm_cmplt_ps(q, _mm_set_ps1(s));
        return m;
    }

    static Vector4 Select(const Mask& mask, const Vector4& a, const Vector4& b)
    {
        Vector4 ret;
        ret.q = _mm_or_ps(_mm_and_ps(mask.q, a.q), _mm_andnot_ps(mask.q, b.q));
        return ret;
    }
};

static const __m128 s_SignMask = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));

inline Vector4 vabs(const Vector4& v)
{
    Vector4 r;
    r.q = _mm_andnot_ps(s_SignMask, v.q);
    return r;
}

inline Vector4 operator +(const Vector4& a, const Vector4& b)
{
    Vector4 v;
    v.q = _mm_add_ps(a.q, b.q);
    return v;
}

inline Vector4 operator -(const Vector4& a, const Vector4& b)
{
    Vector4 v;
    v.q = _mm_sub_ps(a.q, b.q);
    return v;
}

inline Vector4 operator /(const Vector4& a, const Vector4& b)
{
    Vector4 v;
    v.q = _mm_div_ps(a.q, b.q);     // TODO: try mul by rcp
    return v;
}

inline Vector4 operator^(const Vector4& a, const Vector4& b)
{
    Vector4 v;
    v.q = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(a.q, a.q, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(b.q, b.q, _MM_SHUFFLE(3, 1, 0, 2))),
            _mm_mul_ps(_mm_shuffle_ps(a.q, a.q, _MM_SHUFFLE(3, 1, 0, 2)), _mm_shuffle_ps(b.q, b.q, _MM_SHUFFLE(3, 0, 2, 1)))
            );
    return v;
}

inline float operator *(const Vector4& a, const Vector4& b)
{
    return _mm_cvtss_f32(a.Dot(b));
}

inline Vector4 operator *(const Vector4& v, float s)
{
    Vector4 r = v;
    r *= s;
    return r;
}

inline Vector4 operator *(float s, const Vector4& v)
{
    Vector4 r = v;
    r *= s;
    return r;
}

struct FloatMatrix3
{
    float m00, m01, m02, m03,
          m10, m11, m12, m13,
          m20, m21, m22, m23;

    float Determinant() const
    {
        return (m00 * m11 * m22) + (m01 * m12 * m20) + (m02 * m10 * m21) - (m02 * m11 * m20) - (m00 * m12 * m21) - (m01 * m10 * m22);
    }

    ILINE bool Invert()
    {
        //rescue members
        FloatMatrix3 m = *this;
        //calculate the cofactor-matrix (=transposed adjoint-matrix)
        m00 = m.m22 * m.m11 - m.m12 * m.m21;
        m01 = m.m02 * m.m21 - m.m22 * m.m01;
        m02 = m.m12 * m.m01 - m.m02 * m.m11;
        m10 = m.m12 * m.m20 - m.m22 * m.m10;
        m11 = m.m22 * m.m00 - m.m02 * m.m20;
        m12 = m.m02 * m.m10 - m.m12 * m.m00;
        m20 = m.m10 * m.m21 - m.m20 * m.m11;
        m21 = m.m20 * m.m01 - m.m00 * m.m21;
        m22 = m.m00 * m.m11 - m.m10 * m.m01;
        // calculate determinant
        float det = (m.m00 * m00 + m.m10 * m01 + m.m20 * m02);
        if (fabs_tpl(det) < 1E-20f)
        {
            return 0;
        }
        //divide the cofactor-matrix by the determinant
        float idet = 1.0f / det;
        m00 *= idet;
        m01 *= idet;
        m02 *= idet;
        m10 *= idet;
        m11 *= idet;
        m12 *= idet;
        m20 *= idet;
        m21 *= idet;
        m22 *= idet;
        return 1;
    }
};

struct Matrix3
{
    Vector4 row1, row2, row3;

    Matrix3() { }

    explicit Matrix3(Matrix33 m)
    {
        row1.Set(m.m00, m.m01, m.m02);
        row2.Set(m.m10, m.m11, m.m12);
        row3.Set(m.m20, m.m21, m.m22);
    }

    explicit Matrix3(Quat q)
    {
        Vec3 v2 = q.v + q.v;
        float xx = 1 - v2.x * q.v.x;
        float yy = v2.y * q.v.y;
        float xw = v2.x * q.w;
        float xy = v2.y * q.v.x;
        float yz = v2.z * q.v.y;
        float yw = v2.y * q.w;
        float xz = v2.z * q.v.x;
        float zz = v2.z * q.v.z;
        float zw = v2.z * q.w;
        row1.Set(1 - yy - zz, xy - zw, xz + yw);
        row2.Set(xy + zw, xx - zz, yz - xw);
        row3.Set(xz - yw, yz + xw, xx - yy);
    }

    Matrix33 ToMatrix33()
    {
        Matrix33 m;
        m.m00 = row1.x;
        m.m01 = row1.y;
        m.m02 = row1.z;
        m.m10 = row2.x;
        m.m11 = row2.y;
        m.m12 = row2.z;
        m.m20 = row3.x;
        m.m21 = row3.y;
        m.m22 = row3.z;
        return m;
    }

    void SetZero()
    {
        row1.zero();
        row2.zero();
        row3.zero();
    }

    void SetIdentity()
    {
        row1.Set(1, 0, 0);
        row2.Set(0, 1, 0);
        row3.Set(0, 0, 1);
    }

    void Transpose()
    {
        __m128 row4 = _mm_setzero_ps();
        _MM_TRANSPOSE4_PS(row1.q, row2.q, row3.q, row4);
    }

    Vector4 TransformVector(const Vector4& v) const
    {
        return Vector4(row1 * v, row2 * v, row3 * v);
    }

    Matrix3& operator -=(const Matrix3& m)
    {
        row1 -= m.row1;
        row2 -= m.row2;
        row3 -= m.row3;
        return *this;
    }

    float Determinant() const
    {
        return ((FloatMatrix3*)this)->Determinant();
    }

    Matrix3 GetInverted()
    {
        Matrix3 m = *this;
        ((FloatMatrix3*)&m)->Invert();
        return m;
    }
};

inline Vector4 operator *(const Matrix3& m, const Vector4& v)
{
    return m.TransformVector(v);
}

inline Vector4 operator *(const Vector4& v, const Matrix3& m)
{
    Matrix3 t = m;
    t.Transpose();     // TODO: make it explicit
    return t.TransformVector(v);
}

inline Matrix3 operator*(const Matrix3& a, const Matrix3& b)
{
    Matrix3 c;
    __m128 d1, d2, d3;
    d1 = _mm_mul_ps(b.row1.q, _mm_shuffle_ps(a.row1.q, a.row1.q, 0));
    d2 = _mm_mul_ps(b.row2.q, _mm_shuffle_ps(a.row1.q, a.row1.q, 0x55));
    d3 = _mm_mul_ps(b.row3.q, _mm_shuffle_ps(a.row1.q, a.row1.q, 0xaa));
    c.row1.q = _mm_add_ps(d1, _mm_add_ps(d2, d3));
    d1 = _mm_mul_ps(b.row1.q, _mm_shuffle_ps(a.row2.q, a.row2.q, 0));
    d2 = _mm_mul_ps(b.row2.q, _mm_shuffle_ps(a.row2.q, a.row2.q, 0x55));
    d3 = _mm_mul_ps(b.row3.q, _mm_shuffle_ps(a.row2.q, a.row2.q, 0xaa));
    c.row2.q = _mm_add_ps(d1, _mm_add_ps(d2, d3));
    d1 = _mm_mul_ps(b.row1.q, _mm_shuffle_ps(a.row3.q, a.row3.q, 0));
    d2 = _mm_mul_ps(b.row2.q, _mm_shuffle_ps(a.row3.q, a.row3.q, 0x55));
    d3 = _mm_mul_ps(b.row3.q, _mm_shuffle_ps(a.row3.q, a.row3.q, 0xaa));
    c.row3.q = _mm_add_ps(d1, _mm_add_ps(d2, d3));

    return c;
}

static const __m128 s_QuatIdentity = _mm_setr_ps(0, 0, 0, 1);

struct Quaternion
{
    union
    {
        __m128 q;
        float f[4];
        struct
        {
            float x, y, z, w;
        };
    };

    Quaternion() { }

    Quaternion(type_identity)
    {
        q = s_QuatIdentity;
    }

    explicit Quaternion(float a, float b, float c, float d)
    {
        q = _mm_setr_ps(a, b, c, d);
    }

    Quat ToFloat() const
    {
        return Quat(w, x, y, z);
    }

    explicit Quaternion(const Matrix3& mat)
    {
        const FloatMatrix3* pm = (const FloatMatrix3*)&mat;
        const FloatMatrix3& m = *pm;
        float s, p, tr = m.m00 + m.m11 + m.m22;
        w = 1, x = 0, y = 0, z = 0;
        if (tr > 0)
        {
            s = sqrt_tpl(tr + 1.0f), p = 0.5f / s, w = s * 0.5f, x = (m.m21 - m.m12) * p, y = (m.m02 - m.m20) * p, z = (m.m10 - m.m01) * p;
        }
        else if ((m.m00 >= m.m11) && (m.m00 >= m.m22))
        {
            s = sqrt_tpl(m.m00 - m.m11 - m.m22 + 1.0f), p = 0.5f / s, w = (m.m21 - m.m12) * p, x = s * 0.5f, y = (m.m10 + m.m01) * p, z = (m.m20 + m.m02) * p;
        }
        else if ((m.m11 >= m.m00) && (m.m11 >= m.m22))
        {
            s = sqrt_tpl(m.m11 - m.m22 - m.m00 + 1.0f), p = 0.5f / s, w = (m.m02 - m.m20) * p, x = (m.m01 + m.m10) * p, y = s * 0.5f, z = (m.m21 + m.m12) * p;
        }
        else if ((m.m22 >= m.m00) && (m.m22 >= m.m11))
        {
            s = sqrt_tpl(m.m22 - m.m00 - m.m11 + 1.0f), p = 0.5f / s, w = (m.m10 - m.m01) * p, x = (m.m02 + m.m20) * p, y = (m.m12 + m.m21) * p, z = s * 0.5f;
        }
    }

    explicit Quaternion(Quat r)
    {
        q = _mm_setr_ps(r.v.x, r.v.y, r.v.z, r.w);
    }

    Quaternion operator !() const { return Quaternion(-x, -y, -z, w); }

    void SetNlerp(const Quaternion& p, const Quaternion& tq, float t)
    {
        Quaternion q = tq;
        //if( (p|q) < 0 ) { q=-q;   } // TODO: dot
        x = p.x * (1.0f - t) + q.x * t;
        y = p.y * (1.0f - t) + q.y * t;
        z = p.z * (1.0f - t) + q.z * t;
        w = p.w * (1.0f - t) + q.w * t;
    }
};

inline Vector4 operator *(const Quaternion& q, const Vector4& v)
{
    // TODO: dot product version
    const float w = q.w;
    Vector4 qv;
    qv.q = q.q;
    qv.w = 0;                             // is the zero needed?
    Vector4 r2 = (qv ^ v) + w * v;
    Vector4 r = qv ^ r2;
    r += r + v;
    return r;
}

inline Vector4 operator *(const Vector4& v, const Quaternion& q)
{
    const float w = q.w;
    Vector4 qv;
    qv.q = q.q;
    qv.w = 0;
    Vector4 r2 = (v ^ qv) + w * v;
    Vector4 r = r2 ^ qv;
    r += r + v;
    return r;
}

inline Quaternion operator *(const Quaternion& q, const Quaternion& p)
{
    Quaternion r;
    r.w = q.w * p.w - (q.x * p.x + q.y * p.y + q.z * p.z);
    r.x = q.y * p.z - q.z * p.y + q.w * p.x + q.x * p.w;
    r.y = q.z * p.x - q.x * p.z + q.w * p.y + q.y * p.w;
    r.z = q.x * p.y - q.y * p.x + q.w * p.z + q.z * p.w;
    return r;
}

inline Vec3 SseToVec3(const Vector4& v)
{
    return v.ToVec3();
}

inline Quat SseToQuat(const Quaternion& q)
{
    return q.ToFloat();
}
#else
typedef Vec3 Vector4;
typedef Matrix33 Matrix3;
typedef Quat Quaternion;

inline Vec3 SseToVec3(const Vector4& v)
{
    return v;
}

inline Quat SseToQuat(const Quaternion& q)
{
    return q;
}
#endif

enum ECollisionPrimitiveType
{
    eCPT_Invalid,
    eCPT_Capsule,
    eCPT_Sphere,
};

struct SPrimitive
{
    ECollisionPrimitiveType type;
    SPrimitive()
        : type(eCPT_Invalid) { }
    virtual ~SPrimitive() { }
};

struct SSphere
    : SPrimitive
{
    Vector4 center;
    float r;
    SSphere() { type = eCPT_Sphere; }
};

struct SCapsule
    : SPrimitive
{
    Vector4 axis, center;
    float hh, r;
    SCapsule() { type = eCPT_Capsule; }
};

enum EPhysicsLayer
{
    PHYS_LAYER_INVALID,
    PHYS_LAYER_ARTICULATED,
    PHYS_LAYER_CLOTH,
};

struct SPhysicsGeom
{
    SPrimitive* primitive;
    EPhysicsLayer layer;

    SPhysicsGeom()
        : layer(PHYS_LAYER_INVALID) { }
};

struct SBonePhysics
{
    quaternionf q;
    Vec3 t;
    f32 cr, ca;

    SBonePhysics()
    {
        cr = 0;
        ca = 0;
        t = Vec3(ZERO);
        q.SetIdentity();
    }
};


struct STangents
{
    Vector4 t, n;
    STangents()
    {
        t.zero();
        n.zero();
    }
};

struct SBuffers
{
    // temporary buffer for simulated cloth vertices
    DynArray<Vector4> m_tmpClothVtx;
    std::vector<Vector4> m_normals;

    // output buffers for sw-skinning
    DynArray<Vec3> m_arrDstPositions;
    DynArray<SPipTangents> m_arrDstTangents;     // TODO: static

    std::vector<STangents> m_tangents;
};

struct SSkinMapEntry
{
    int iMap;     // vertex index in the sim mesh
    int iTri;     // triangle index in the sim mesh
    float s, t;     // barycentric coordinates in the adjacent triangle
    float h;     // distance from triangle
};

struct STangentData
{
    float t1, t2, r;
};

struct SClothGeometry
{
    enum
    {
        MAX_LODS = 2,
    };

    // the number of vertices in the sim mesh
    int nVtx;
    // registered physical geometry constructed from sim mesh
    phys_geometry* pPhysGeom;
    // mapping between original mesh and welded one
    vtx_idx* weldMap;
    // number of non-welded vertices in the sim mesh
    int nUniqueVtx;
    // blending weights between skinning and sim (coming from vertex colors)
    float* weights;
    // mapping between render mesh and sim mesh vertices
    SSkinMapEntry* skinMap[MAX_LODS];
    // render mesh indices
    uint numIndices[MAX_LODS];
    vtx_idx* pIndices[MAX_LODS];
    // tangent frame uv data
    STangentData* tangentData[MAX_LODS];
    // maximum number for vertices (for LOD0)
    int maxVertices;

    // memory pool. keeping it as a list for now to avoid reallocation
    // because the jobs are using pointers inside it
    std::list<SBuffers> pool;
    std::vector<int>freePoolSlots;
    volatile int poolLock;

    SClothGeometry()
        : nVtx(0)
        , pPhysGeom(NULL)
        , weldMap(NULL)
        , nUniqueVtx(0)
        , weights(NULL)
        , poolLock(0)
        , maxVertices(0)
    {
        for (int i = 0; i < MAX_LODS; i++)
        {
            skinMap[i] = NULL;
            pIndices[i] = NULL;
            tangentData[i] = NULL;
        }
    }

    void Cleanup();
    int GetBuffers();
    void ReleaseBuffers(int idx);
    SBuffers* GetBufferPtr(int idx);
    void AllocateBuffer();
};

struct SClothInfo
{
    uint64 key;
    float distance;
    bool visible;
    int frame;

    SClothInfo() { }
    SClothInfo(uint64 k, float d, bool v, int f)
        : key(k)
        , distance(d)
        , visible(v)
        , frame(f) { }
};

struct SPrimitive;

struct SContact
{
    Vector4 n, pt;
    int particleIdx;
    float depth;

    SContact()
    {
        n.zero();
        pt.zero();
        particleIdx = -1;
        depth = 0.f;
    }
};

struct SParticleCold
{
    Vector4 prevPos;
    Vector4 oldPos;
    Vector4 skinnedPos;
    SContact* pContact;
    int permIdx;
    int bAttached;

    SParticleCold()
    {
        prevPos.zero();
        pContact = NULL;
        bAttached = 0;
        skinnedPos.zero();
        permIdx = -1;
        oldPos.zero();
    }
};

struct SParticleHot
{
    Vector4 pos;
    float alpha;     // for blending with animation
    int timer;

    SParticleHot()
    {
        pos.zero();
        timer = 0;
        alpha = 0;
    }
};

struct SLink
{
    int i1, i2;
    float lenSqr, weight1, weight2;
    bool skip;

    SLink()
        : i1(0)
        , i2(0)
        , lenSqr(0.0f)
        , weight1(0.f)
        , weight2(0.f)
        , skip(false) { }
};

struct SCollidable
{
    Matrix3 R, oldR;
    Vector4 offset, oldOffset;
    f32 cr, ca;

    SCollidable()
    {
        cr = 0;
        ca = 0;
        offset.zero();
        oldOffset.zero();
        R.SetIdentity();
        oldR.SetIdentity();
    }
};

class CClothSimulator
{
public:

    CClothSimulator()
    {
        m_particlesHot = NULL;
        m_particlesCold = NULL;
        m_links = NULL;
        m_gravity.Set(0, 0, -9.8f);
        m_time = 0;
        m_steps = 0;
        m_maxSteps = 3;
        m_idx0 = -1;
        m_color = ColorB(aznumeric_caster(cry_random(0, 127)), aznumeric_caster(cry_random(0, 127)), aznumeric_caster(cry_random(0, 127)));
        m_contacts = NULL;
        m_nContacts = 0;
        m_bBlendSimMesh = false;
    }

    ~CClothSimulator()
    {
        SAFE_DELETE_ARRAY(m_particlesHot);
        SAFE_DELETE_ARRAY(m_particlesCold);
        SAFE_DELETE_ARRAY(m_links);
        SAFE_DELETE_ARRAY(m_contacts);
    }


    bool AddGeometry(phys_geometry* pgeom);
    int SetParams(const SVClothParams& params, float* weights);
    void SetSkinnedPositions(const Vector4* points);
    void GetVertices(Vector4* pWorldCoords);

    int Awake(int bAwake = 1, int iSource = 0);

    void StartStep(float time_interval, const QuatT& loacation);
    float GetMaxTimeStep(float time_interval);
    int Step(float animBlend);

    void DrawHelperInformation();
    void SwitchToBlendSimMesh();
    bool IsBlendSimMeshOn();
    const CAttachmentManager* m_pAttachmentManager;

    SVClothParams m_config;
private:
    void AddContact(int i, const Vector4& n, const Vector4& pt, int permIdx);
    void PrepareEdge(SLink& link);
    void SolveEdge(const SLink& link, float stretch);
    void DetectCollisions();
    void RigidBodyDamping();


private:

    int m_nVtx; // the number of particles
    int m_nEdges; // the number of links between particles
    SParticleCold* m_particlesCold; // the simulation particles
    SParticleHot* m_particlesHot; // the simulation particles
    SLink* m_links; // the structural links between particles
    Vector4 m_gravity; // the gravity vector
    float m_time; // time accumulator over frames (used to determine the number of sub-steps)
    int m_steps;
    int m_maxSteps;
    int m_idx0; // an attached point chosen as the origin of axes
    Quat m_lastqHost; // the last orientation of the part cloth is attached to
    std::vector<SCollidable> m_permCollidables; // list of contact parts (no collision with the world)
    std::vector<SLink> m_shearLinks; // shear and bend links
    std::vector<SLink> m_bendLinks;
    ColorB m_color;

    // contact information list
    SContact* m_contacts;
    int m_nContacts;

    bool m_bBlendSimMesh;
};

ILINE void CClothSimulator::AddContact(int i, const Vector4& n, const Vector4& pt, int permIdx)
{
    float depth = (pt - m_particlesHot[i].pos) * n;
    SContact* pContact = NULL;
    if (m_particlesCold[i].pContact)
    {
        if (depth <= m_particlesCold[i].pContact->depth)
        {
            return;
        }
        pContact = m_particlesCold[i].pContact;
    }
    else
    {
        pContact = &m_contacts[m_nContacts++];
        m_particlesCold[i].pContact = pContact;
    }
    m_particlesCold[i].permIdx = permIdx;
    pContact->depth = depth;
    pContact->n = n;
    pContact->pt = pt;
    pContact->particleIdx = i;
}

ILINE void CClothSimulator::PrepareEdge(SLink& link)
{
    int n1 = !(m_particlesCold[link.i1].bAttached);
    int n2 = !(m_particlesCold[link.i2].bAttached);
    link.skip = n1 + n2 == 0;
    if (link.skip)
    {
        return;
    }
    float m1Inv = n1 ? 1.f : 0.f;
    float m2Inv = n2 ? 1.f : 0.f;
    float mu = 1.0f / (m1Inv + m2Inv);
    float stretch = 1.f;
    if (m_config.stiffnessGradient != 0.f)
    {
        stretch = 1.f - (m_particlesHot[link.i1].alpha + m_particlesHot[link.i2].alpha) * 0.5f;
        if (stretch != 1.f)
        {
            stretch = min(1.f, m_config.stiffnessGradient * stretch);
        }
    }
    link.weight1 = m1Inv * mu * stretch;
    link.weight2 = m2Inv * mu * stretch;
}

ILINE void CClothSimulator::SolveEdge(const SLink& link, float stretch)
{
    if (link.skip)
    {
        return;
    }
    Vector4& v1 = m_particlesHot[link.i1].pos;
    Vector4& v2 = m_particlesHot[link.i2].pos;
    Vector4 delta = v1 - v2;
    const float lenSqr = delta.len2();
    delta *= stretch * (lenSqr - link.lenSqr) / (lenSqr + link.lenSqr);
    v1 -= link.weight1 * delta;
    v2 += link.weight2 * delta;
}


class CClothPiece
{
private:
    friend struct VertexCommandClothSkin;

    enum
    {
        HIDE_INTERVAL = 50
    };

public:
    CClothPiece()
    {
        m_pCharInstance = NULL;
        m_animBlend = 0.f;
        m_pVClothAttachment = NULL;
        m_bHidden = false;
        m_numLods = 0;
        m_clothGeom = NULL;
        //  m_bSingleThreaded = false;
        m_lastVisible = false;
        m_hideCounter = 0;
        m_bAlwaysVisible = false;
        m_currentLod = 0;
        m_buffers = NULL;
        m_poolIdx = -1;
        m_initialized = false;
    }

    // initializes the object given a skin and a stat obj
    bool Initialize(const CAttachmentVCLOTH* pVClothAttachment);

    void Dettach();

    int GetNumLods() { return m_numLods; }
    //  bool IsSingleThreaded() { return m_bSingleThreaded; }
    bool IsAlwaysVisible() { return m_bAlwaysVisible; }

    bool PrepareCloth(CSkeletonPose& skeletonPose, const Matrix34& worldMat, bool visible, int lod);
    bool CompileCommand(SVertexSkinData& skinData, CVertexCommandBuffer& commandBuffer);

    void SetBlendWeight(float weight);

    bool NeedsDebugDraw() const { return Console::GetInst().ca_DrawCloth & 2 ? true : false; }
    void DrawDebug(const SVertexAnimationJob* pVertexAnimation);

    void SetClothParams(const SVClothParams& params);
    const SVClothParams& GetClothParams();

private:

    bool PrepareRenderMesh(int lod);
    Vector4 SkinByTriangle(int i, strided_pointer<Vec3>& pVtx, int lod);

    void UpdateSimulation(const DualQuat* pTransformations, const uint transformationCount);
    template<bool PREVIOUS_POSITIONS>
    void SkinSimulationToRenderMesh(int lod, CVertexData& vertexData, const strided_pointer<const Vec3>& pVertexPositionsPrevious);

    void WaitForJob(bool bPrev);


private:

    CAttachmentVCLOTH* m_pVClothAttachment;
    CClothSimulator m_simulator;
    SVClothParams m_clothParams;

    // structure containing all sim mesh related data and working buffers
    SClothGeometry* m_clothGeom;
    SBuffers* m_buffers;
    int m_poolIdx;

    // used for blending with animation
    float m_animBlend;

    // used for hiding the cloth (mainly in the editor)
    bool m_bHidden;

    // the number of loaded LODs
    int m_numLods;

    // enables single threaded simulation updated
    //  bool m_bSingleThreaded;

    // the current rotation of the character
    QuatT m_charLocation;

    // for easing in and out
    bool m_lastVisible;
    int m_hideCounter;

    // flags that the cloth is always simulated
    bool m_bAlwaysVisible;

    int m_currentLod;

    // used to get the correct time from the character
    CCharInstance* m_pCharInstance;

    bool m_initialized;
};

ILINE Vector4 CClothPiece::SkinByTriangle(int i, strided_pointer<Vec3>& pVtx, int lod)
{
    DynArray<Vector4>& tmpClothVtx = m_buffers->m_tmpClothVtx;
    std::vector<Vector4>& normals = m_buffers->m_normals;
    const SSkinMapEntry& skinMap = m_clothGeom->skinMap[lod][i];
    int tri = skinMap.iTri;
    if (tri >= 0)
    {
        mesh_data* md = (mesh_data*)m_clothGeom->pPhysGeom->pGeom->GetData();
        const int i2 = skinMap.iMap;
        const int base = tri * 3;
        const int idx = md->pIndices[base + i2];
        const int idx0 = md->pIndices[base + inc_mod3[i2]];
        const int idx1 = md->pIndices[base + dec_mod3[i2]];
        const Vector4 u = tmpClothVtx[idx0] - tmpClothVtx[idx];
        const Vector4 v = tmpClothVtx[idx1] - tmpClothVtx[idx];
        const Vector4 n = (1.f - skinMap.s - skinMap.t) * normals[idx] + skinMap.s * normals[idx0] + skinMap.t * normals[idx1];
        return tmpClothVtx[idx] + skinMap.s * u + skinMap.t * v + skinMap.h * n;
    }
    else
    {
        return tmpClothVtx[skinMap.iMap];
    }
}

ILINE void CClothPiece::SetBlendWeight(float weight)
{
    weight = max(0.f, min(1.f, weight));
    m_animBlend = Console::GetInst().ca_ClothBlending == 0 ? 0.f : weight;
}

class CAttachmentVCLOTH
    : public IAttachment
    , public IAttachmentSkin
    , public SAttachmentBase
{
public:

    CAttachmentVCLOTH()
    {
        m_clothCacheKey = -1;
        for (uint32 j = 0; j < 2; ++j)
        {
            m_pRenderMeshsSW[j] = NULL;
        }
        memset(m_arrSkinningRendererData, 0, sizeof(m_arrSkinningRendererData));
        m_pRenderSkin = 0;
    };

    virtual ~CAttachmentVCLOTH();

    virtual void AddRef()
    {
        ++m_nRefCounter;
    }

    virtual void Release()
    {
        if (--m_nRefCounter == 0)
        {
            delete this;
        }
    }

    virtual uint32 GetType() const { return CA_VCLOTH; }
    virtual uint32 SetJointName(const char* szJointName = 0) {return 0; } //not possible

    virtual const char* GetName() const { return m_strSocketName; };
    virtual uint32 GetNameCRC() const { return m_nSocketCRC32; }
    virtual uint32 ReName(const char* strSocketName, uint32 crc) {    m_strSocketName.clear();    m_strSocketName = strSocketName; m_nSocketCRC32 = crc;    return 1;   };

    virtual uint32 GetFlags() { return m_AttFlags; }
    virtual void SetFlags(uint32 flags) { m_AttFlags = flags; }

    void ReleaseRenderRemapTablePair();
    void ReleaseSimRemapTablePair();
    void ReleaseSoftwareRenderMeshes();

    virtual uint32 AddBinding(IAttachmentObject* pIAttachmentObject, _smart_ptr<ISkin> pISkinRender = nullptr, uint32 nLoadingFlags = 0);

    void PatchRemapping(CDefaultSkeleton* pDefaultSkeleton);

    uint32 AddSimBinding(const ISkin& pISkinRender, uint32 nLoadingFlags = 0);
    virtual void ClearBinding(uint32 nLoadingFlags = 0);
    virtual uint32 SwapBinding(IAttachment* pNewAttachment);
    virtual IAttachmentObject* GetIAttachmentObject() const { return m_pIAttachmentObject; }
    virtual IAttachmentSkin* GetIAttachmentSkin() { return this; }

    virtual void HideAttachment(uint32 x)
    {
        if (x)
        {
            m_AttFlags |= (FLAGS_ATTACH_HIDE_MAIN_PASS | FLAGS_ATTACH_HIDE_SHADOW_PASS | FLAGS_ATTACH_HIDE_RECURSION);
        }
        else
        {
            m_AttFlags &= ~(FLAGS_ATTACH_HIDE_MAIN_PASS | FLAGS_ATTACH_HIDE_SHADOW_PASS | FLAGS_ATTACH_HIDE_RECURSION);
        }
    }
    virtual uint32 IsAttachmentHidden() { return m_AttFlags & FLAGS_ATTACH_HIDE_MAIN_PASS; }
    virtual void HideInRecursion(uint32 x)
    {
        if (x)
        {
            m_AttFlags |= FLAGS_ATTACH_HIDE_RECURSION;
        }
        else
        {
            m_AttFlags &= ~FLAGS_ATTACH_HIDE_RECURSION;
        }
    }
    virtual uint32 IsAttachmentHiddenInRecursion() { return m_AttFlags & FLAGS_ATTACH_HIDE_RECURSION; }
    virtual void HideInShadow(uint32 x)
    {
        if (x)
        {
            m_AttFlags |= FLAGS_ATTACH_HIDE_SHADOW_PASS;
        }
        else
        {
            m_AttFlags &= ~FLAGS_ATTACH_HIDE_SHADOW_PASS;
        }
    }
    virtual uint32 IsAttachmentHiddenInShadow() { return m_AttFlags & FLAGS_ATTACH_HIDE_SHADOW_PASS; }

    virtual void SetAttAbsoluteDefault(const QuatT& qt) {   };
    virtual void SetAttRelativeDefault(const QuatT& qt) {   };
    virtual const QuatT& GetAttAbsoluteDefault() const { return g_IdentityQuatT; };
    virtual const QuatT& GetAttRelativeDefault() const { return g_IdentityQuatT; };

    virtual const QuatT& GetAttModelRelative() const { return g_IdentityQuatT;  }; //this is relative to the animated bone
    virtual const QuatTS GetAttWorldAbsolute() const;
    virtual void UpdateAttModelRelative();

    virtual uint32 GetJointID() const { return -1; };
    virtual void AlignJointAttachment() {};
    virtual uint32 ProjectAttachment(const char* szJointName = 0)  { return 0; };

    virtual void Serialize(TSerialize ser);
    virtual size_t SizeOfThis();
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    virtual void TriggerMeshStreaming(uint32 nDesiredRenderLOD, const SRenderingPassInfo& passInfo);

    void DrawAttachment(SRendParams& rParams, const SRenderingPassInfo& passInfo, const Matrix34& rWorldMat34, f32 fZoomFactor = 1);
    void RecreateDefaultSkeleton(CCharInstance* pInstanceSkel, uint32 nLoadingFlags);
    void UpdateRemapTable();

    void PatchSimRemapping(CDefaultSkeleton* pDefaultSkeleton);

    void ComputeClothCacheKey();
    uint64 GetClothCacheKey() const {return m_clothCacheKey; };
    void AddClothParams(const SVClothParams& clothParams);
    bool InitializeCloth();
    const SVClothParams& GetClothParams();

    // Vertex Transformation
public:
    SSkinningData* GetVertexTransformationData(const bool bVertexAnimation, uint8 nRenderLOD);
    _smart_ptr<IRenderMesh> CreateVertexAnimationRenderMesh(uint lod, uint id);

#ifdef EDITOR_PCDEBUGCODE
    void DrawWireframeStatic(const Matrix34& m34, int nLOD, uint32 color);
    void SoftwareSkinningDQ_VS_Emulator(CModelMesh* pModelMesh, Matrix34 rRenderMat34, uint8 tang, uint8 binorm, uint8 norm, uint8 wire, const DualQuat* const pSkinningTransformations);
#endif

    virtual IVertexAnimation* GetIVertexAnimation() { return &m_vertexAnimation; }
    virtual ISkin* GetISkin() { return m_pRenderSkin; };
    virtual float GetExtent(EGeomForm eForm);
    virtual void GetRandomPos(PosNorm& ran, EGeomForm eForm) const;
    virtual void ComputeGeometricMean(SMeshLodInfo& lodInfo) const override;

    int GetGuid() const;

    DynArray<JointIdType> m_arrRemapTable;  // maps skin's bone indices to skeleton's bone indices
    DynArray<JointIdType> m_arrSimRemapTable;  // maps skin's bone indices to skeleton's bone indices
    _smart_ptr<CSkin> m_pRenderSkin;
    _smart_ptr<CSkin> m_pSimSkin;
    _smart_ptr<IRenderMesh> m_pRenderMeshsSW[2];
    string m_sSoftwareMeshName;
    CVertexData m_vertexData;
    CVertexAnimation m_vertexAnimation;
    CClothPiece m_clothPiece;
    uint64 m_clothCacheKey;

    // history for skinning data, needed for motion blur
    static const int tripleBufferSize = 3;
    struct
    {
        SSkinningData* pSkinningData;
        int nNumBones;
        int nFrameID;
    } m_arrSkinningRendererData[tripleBufferSize];                                                      // triple buffered for motion blur

    //! Lock whenever creating/releasing bone remappings
    AZStd::mutex m_remapMutex;

    //! Keep track of whether the bone remapping has been done or not to prevent multiple remappings from occurring in multi-threaded situations
    AZStd::vector<bool> m_hasRemappedRenderMesh;
    bool m_hasRemappedSimMesh;

private:
    // functions to keep in sync ref counts on skins and cleanup of remap tables
    void ReleaseRenderSkin();
    void ReleaseSimSkin();
};


#endif // CRYINCLUDE_CRYANIMATION_ATTACHMENTVCLOTH_H
