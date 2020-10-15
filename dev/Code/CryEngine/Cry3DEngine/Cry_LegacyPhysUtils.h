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

// Copied utils functions from CryPhysics that are used by non-physics systems
// This functions will be eventually removed, DO *NOT* use these functions
// TO-DO: Re-implement users using new code
// LY-109806

#pragma once

#include "stridedptr.h"
#include "Cry_Vector3.h"

namespace LegacyCryPhysicsUtils
{
    typedef int index_t;

    // Workaround for bug in GCC 4.8. The kind of access patterns here leads to an internal
    // compiler error in GCC 4.8 when optimizing with debug symbols. Two possible solutions
    // are available, compile in Profile mode without debug symbols or remove optimizations
    // in the code where the bug occurs
    // see http://gcc.gnu.org/bugzilla/show_bug.cgi?id=59776
#if defined(_PROFILE) && !defined(__clang__) && (__GNUC__ == 4) && (__GNUC_MINOR__ == 8)
    // Cannot use #pragma GCC optimize("O0") because it causes a system crash when using
    // the gcc compiler for another platform
#define CRY_GCC48_AVOID_OPTIMIZE __attribute__((optimize("-O0")))
#else
#define CRY_GCC48_AVOID_OPTIMIZE
#endif
    // unused_marker deliberately fills a variable with invalid data,
    // so that later is_unused() can check whether it was initialized
    // (this is used in all physics params/status/action structures)
    class unused_marker
    {
    public:
        union f2i
        {
            float f;
            uint32 i;
        };
        union d2i
        {
            double d;
            uint32 i[2];
        };
        unused_marker() {}
        unused_marker& operator,(float& x) CRY_GCC48_AVOID_OPTIMIZE;
        unused_marker& operator,(double& x) CRY_GCC48_AVOID_OPTIMIZE;
        unused_marker& operator,(int& x) CRY_GCC48_AVOID_OPTIMIZE;
        unused_marker& operator,(unsigned int& x) CRY_GCC48_AVOID_OPTIMIZE;
        template<class ref>
        unused_marker& operator,(ref*& x) { x = (ref*)-1; return *this; }
        template<class F>
        unused_marker& operator,(Vec3_tpl<F>& x) { return *this, x.x; }
        template<class F>
        unused_marker& operator,(Quat_tpl<F>& x) { return *this, x.w; }
        template<class F>
        unused_marker& operator,(strided_pointer<F>& x) { return *this, x.data; }
    };
    inline unused_marker& unused_marker::operator,(float& x) { *alias_cast<int*>(&x) = 0xFFBFFFFF; return *this; }
    inline unused_marker& unused_marker::operator,(double& x) { (alias_cast<int*>(&x))[false ? 1 : 0] = 0xFFF7FFFF; return *this; }
    inline unused_marker& unused_marker::operator,(int& x) { x = 1 << 31; return *this; }
    inline unused_marker& unused_marker::operator,(unsigned int& x) { x = 1u << 31; return *this; }

#undef CRY_GCC48_AVOID_OPTIMIZE

    inline bool is_unused(const float& x) { unused_marker::f2i u; u.f = x; return (u.i & 0xFFA00000) == 0xFFA00000; }

    inline bool is_unused(int x) { return x == 1 << 31; }
    inline bool is_unused(unsigned int x) { return x == 1u << 31; }
    template<class ref>
    bool is_unused(ref* x) { return x == (ref*)-1; }
    template<class ref>
    bool is_unused(strided_pointer<ref> x) { return is_unused(x.data); }
    template<class F>
    bool is_unused(const Ang3_tpl<F>& x) { return is_unused(x.x); }
    template<class F>
    bool is_unused(const Vec3_tpl<F>& x) { return is_unused(x.x); }
    template<class F>
    bool is_unused(const Quat_tpl<F>& x) { return is_unused(x.w); }
    inline bool is_unused(const double& x) { unused_marker::d2i u; u.d = x; return (u.i[eLittleEndian ? 1 : 0] & 0xFFF40000) == 0xFFF40000; }
#ifndef MARK_UNUSED
#define MARK_UNUSED LegacyCryPhysicsUtils::unused_marker(),
#endif
    typedef void* (*qhullmalloc)(size_t);
    namespace qhull_IMPL
    {
        struct ptitem
        {
            ptitem* next, * prev;
        };

        struct qhtritem
        {
            qhtritem* next, * prev;
            ptitem* ptassoc;
            Vec3 n, pt0;
            int idx[3];
            qhtritem* buddy[3];
            int deleted;
        };

        inline bool e_cansee(const Vec3& dp, const Vec3& n, float e = 0.002f) { return sqr_signed(dp * n) > sqr_signed(e)* dp.len2()* n.len2(); }

        inline void relocate_ptritem(qhtritem*& ptr, intptr_t diff)
        {
            ptr = (qhtritem*)((intptr_t)ptr + diff & ~- iszero((intptr_t)ptr));    // offset the pointer, but leave out 0s
        }

        inline void relocate_tritem(qhtritem* ptr, intptr_t diff)
        {
            relocate_ptritem(ptr->next, diff);
            relocate_ptritem(ptr->prev, diff);
            relocate_ptritem(ptr->buddy[0], diff);
            relocate_ptritem(ptr->buddy[1], diff);
            relocate_ptritem(ptr->buddy[2], diff);
        }

        template<class item>
        void delete_item_from_list(item* p)
        {
            if (p->prev)
            {
                p->prev->next = p->next;
            }
            if (p->next)
            {
                p->next->prev = p->prev;
            }
            p->prev = p->next = 0;
        }
        template<class item>
        void add_item_to_list(item*& pin, item* pnew)
        {
            if (!pin)
            {
                pin = pnew->next = pnew->prev = pnew;
            }
            else
            {
                pnew->next = pin->next;
                pnew->prev = pin;
                pin->next->prev = pnew;
                pin->next = pnew;
            }
        }
        template<class item>
        void merge_lists(item*& plist, item* pnew)
        {
            if (!pnew)
            {
                return;
            }
            if (!plist)
            {
                plist = pnew;
            }
            else
            {
                plist->next->prev = pnew->prev;
                pnew->prev->next = plist->next;
                plist->next = pnew;
                pnew->prev = plist;
            }
        }

        struct Vec3mem
            : Vec3
        {
            Vec3 operator-(const Vec3& op) const { return sub(op); }
            float operator*(const Vec3& op) const { return dot(op); }
            Vec3 operator^(const Vec3& op) const { return cross(op); }
        };

        static void associate_ptlist_with_trilist(ptitem* ptlist, qhtritem* trilist, ptitem* pt0, strided_pointer<Vec3mem> pvtx)
        {
            if (!ptlist)
            {
                return;
            }
            ptitem* pt = ptlist, * ptnext, * ptlast = ptlist->prev;
            qhtritem* tr;
            int i;
            do
            {
                ptnext = pt->next;
                delete_item_from_list(pt);
                tr = trilist;
                i = static_cast<int>(pt - pt0);
                do
                {
                    if (e_cansee(pvtx[i] - tr->pt0, tr->n))
                    {
                        add_item_to_list(tr->ptassoc, pt);
                        break;
                    }
                    tr = tr->next;
                } while (tr != trilist);
                if (pt == ptlast)
                {
                    break;
                }
                pt = ptnext;
            } while (true);
        }

        static inline void swap(int* v, void** p, int i1, int i2)
        {
            int ti = v[i1];
            v[i1] = v[i2];
            v[i2] = ti;
            void* tp = p[i1];
            p[i1] = p[i2];
            p[i2] = tp;
        }
        static void qsort(int* v, void** p, int left, int right)
        {
            if (left >= right)
            {
                return;
            }
            int i, last;
            swap(v, p, left, (left + right) >> 1);
            for (last = left, i = left + 1; i <= right; i++)
            {
                if (v[i] < v[left])
                {
                    swap(v, p, ++last, i);
                }
            }
            swap(v, p, left, last);

            qsort(v, p, left, last - 1);
            qsort(v, p, last + 1, right);
        }
        static int bin_search(int* v, int n, int idx)
        {
            int left = 0, right = n, m;
            do
            {
                m = (left + right) >> 1;
                if (v[m] == idx)
                {
                    return m;
                }
                if (v[m] < idx)
                {
                    left = m;
                }
                else
                {
                    right = m;
                }
            } while (left < right - 1);
            return left;
        }

        static int __qhullcalled = 0;

        static int qhull(strided_pointer<Vec3> _pts, int npts, index_t*& pTris, qhullmalloc qmalloc)
        {
#if defined(PLATFORM_64BIT)
            static ptitem ptbuf[4096];
            static qhtritem trbuf[4096];
            static qhtritem* tmparr_ptr_buf[2048];
            static int tmparr_idx_buf[2048];
#else
            static ptitem ptbuf[1024];
            static qhtritem trbuf[1024];
            static qhtritem* tmparr_ptr_buf[512];
            static int tmparr_idx_buf[512];
#endif
            static volatile int g_lockQhull;
            int iter = 0, maxiter = 0;
            __qhullcalled++;
            strided_pointer<Vec3mem> pts = strided_pointer<Vec3mem>((Vec3mem*)_pts.data, _pts.iStride);

            ptitem* pt, * ptmax, * ptdeleted, * ptlist = npts > sizeof(ptbuf) / sizeof(ptbuf[0]) ? new ptitem[npts] : ptbuf;
            qhtritem* tr, * trnext, * trend, * trnew, * trdata = trbuf, * trstart = 0, * trlast, * trbest;
            int i, j, k, ti, trdatasz = sizeof(trbuf) / sizeof(trbuf[0]), bidx[6], n, next_iter, delbuds;
            qhtritem** tmparr_ptr = tmparr_ptr_buf;
            int* tmparr_idx = tmparr_idx_buf, tmparr_sz = 512;
            float dist, maxdist /*,e*/;
            Vec3 pmin(VMAX), pmax(VMIN);
            WriteLock lock(g_lockQhull);

            // select points for initial tetrahedron
            // first, find 6 points corresponding to min and max coordinates
            for (i = 1; i < npts; i++)
            {
                if (pts[i].x > pmax.x)
                {
                    pmax.x = pts[i].x;
                    bidx[0] = i;
                }
                if (pts[i].x < pmin.x)
                {
                    pmin.x = pts[i].x;
                    bidx[1] = i;
                }
                if (pts[i].y > pmax.y)
                {
                    pmax.y = pts[i].y;
                    bidx[2] = i;
                }
                if (pts[i].y < pmin.y)
                {
                    pmin.y = pts[i].y;
                    bidx[3] = i;
                }
                if (pts[i].z > pmax.z)
                {
                    pmax.z = pts[i].z;
                    bidx[4] = i;
                }
                if (pts[i].z < pmin.z)
                {
                    pmin.z = pts[i].z;
                    bidx[5] = i;
                }
            }
            //  e = max(max(pmax.x-pmin.x,pmax.y-pmin.y),pmax.z-pmin.z)*0.01f;

            for (bidx[0] = 0, i = 1; i < npts; i++)
            {
                bidx[0] += i - bidx[0] & -isneg(pts[i].x - pts[bidx[0]].x);
            }
            for (bidx[1] = 0, i = 1; i < npts; i++)
            {
                bidx[1] += i - bidx[1] & -isneg((pts[bidx[1]] - pts[bidx[0]]).len2() - (pts[i] - pts[bidx[0]]).len2());
            }
            Vec3 norm = pts[bidx[1]] - pts[bidx[0]];
            for (bidx[2] = 0, i = 1; i < npts; i++)
            {
                bidx[2] += i - bidx[2] & -isneg((norm ^ pts[bidx[2]] - pts[bidx[0]]).len2() - (norm ^ pts[i] - pts[bidx[0]]).len2());
            }
            norm = pts[bidx[1]] - pts[bidx[0]] ^ pts[bidx[2]] - pts[bidx[0]];
            for (bidx[3] = 0, i = 1; i < npts; i++)
            {
                bidx[3] += i - bidx[3] & -isneg(fabs_tpl((pts[bidx[3]] - pts[bidx[0]]) * norm) - fabs_tpl((pts[i] - pts[bidx[0]]) * norm));
            }
            if ((pts[bidx[3]] - pts[bidx[0]]) * norm > 0)
            {
                i = bidx[1];
                bidx[1] = bidx[2];
                bidx[2] = i;
            }

            // build a double linked list from all points
            for (i = 0; i < npts; i++)
            {
                ptlist[i].prev = ptlist + i - 1;
                ptlist[i].next = ptlist + i + 1;
            }
            ptlist[0].prev = ptlist + npts - 1;
            ptlist[npts - 1].next = ptlist;
            // remove selected points from the list
            for (i = 0; i < 4; i++)
            {
                delete_item_from_list(ptlist + bidx[i]);
            }
            // assign 3 points to each of 4 initial triangles
            for (i = 0; i < 4; i++)
            {
                for (j = k = 0; j < 4; j++)
                {
                    if (j != i)
                    {
                        trbuf[i].idx[k++] = bidx[j];     // skip i.th point in i.th triangle
                    }
                }
                trbuf[i].n = pts[trbuf[i].idx[1]] - pts[trbuf[i].idx[0]] ^ pts[trbuf[i].idx[2]] - pts[trbuf[i].idx[0]];
                trbuf[i].pt0 = pts[trbuf[i].idx[0]];
                if (e_cansee(pts[bidx[i]] - trbuf[i].pt0, trbuf[i].n, 0)) // flip the orientation so that ccw normal points outwards
                {
                    ti = trbuf[i].idx[0];
                    trbuf[i].idx[0] = trbuf[i].idx[2];
                    trbuf[i].idx[2] = ti;
                    trbuf[i].n = -trbuf[i].n;
                }
                trbuf[i].ptassoc = 0;
                trbuf[i].deleted = 0;
                add_item_to_list(trstart, trbuf + i);
            }
            // fill buddy links for each triangle
            for (i = 0; i < 4; i++)
            {
                for (j = 0; j < 4; j++)
                {
                    if (j != i)
                    {
                        for (k = 0; k < 3; k++)
                        {
                            for (ti = 0; ti < 3; ti++)
                            {
                                if (trbuf[i].idx[k] == trbuf[j].idx[ti] && trbuf[i].idx[k == 2 ? 0 : k + 1] == trbuf[j].idx[ti == 0 ? 2 : ti - 1])
                                {
                                    trbuf[i].buddy[k] = trbuf + j;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            trend = trstart + 4;
            for (i = 0; i < 4; i++)
            {
                if (trbuf[i].n.len2() < 1E-6f)
                {
#ifdef _DEBUG
                    //OutputDebugString("WARNING: convex hull not computed because of degenerate initial triangles\n");
#endif
                    n = 0;
                    goto endqhull;  // some degenerate case, don't party with it
                }
            }
            // associate points with one of the initial triangles
            for (i = 0; i < npts; i++)
            {
                if (ptlist[i].next)
                {
                    break;
                }
            }
            associate_ptlist_with_trilist(ptlist + i, trstart, ptlist, pts);

#define DELETE_TRI(ptri) {                                \
        merge_lists(ptdeleted, (ptri)->ptassoc);          \
        if ((ptri) == trstart) {trstart = (ptri)->next; } \
        if ((ptri) == trnext) {trnext = (ptri)->next; }   \
        delete_item_from_list(ptri); (ptri)->deleted = 1; }


            // main loop
            iter = 0;
            maxiter = npts * npts * 2;
            ptmax = trstart->ptassoc;
            tr = trstart;
            do
            {
                trnext = tr->next;
                pt = tr->ptassoc;
                if (pt)
                {
                    // find the fartherst of the associated with the triangle points
                    maxdist = -1E37f;
                    do
                    {
                        if ((dist = pts[(int)(pt - ptlist)] * tr->n) > maxdist)
                        {
                            maxdist = dist;
                            ptmax = pt;
                        }
                        pt = pt->next;
                    } while (pt != tr->ptassoc);
                    ptdeleted = 0;
                    if (tr->ptassoc == ptmax)
                    {
                        tr->ptassoc = ptmax->next;
                    }
                    delete_item_from_list(ptmax);
                    if (tr->ptassoc == ptmax)
                    {
                        tr->ptassoc = 0;
                    }

                    // find the triangle that the point can see "most confidently"
                    tr = trstart;
                    trlast = tr->prev;
                    ti = static_cast<int>(ptmax - ptlist);
                    maxdist = -1E37f;
                    do
                    {
                        trnext = tr->next;
                        if ((pts[ti] - tr->pt0) * tr->n > maxdist)
                        {
                            maxdist = (pts[ti] - tr->pt0) * tr->n;
                            trbest = tr;
                        }
                        if (tr == trlast)
                        {
                            break;
                        }
                        tr = trnext;
                    } while (true);

                    // "flood fill" triangles that the point can see around that one
                    DELETE_TRI(trbest)
                        tr = trbest->next = trbest->prev = trbest;
                    do
                    {
                        if (tr->buddy[0] && !tr->buddy[0]->deleted && e_cansee(pts[ti] - tr->buddy[0]->pt0, tr->buddy[0]->n, 0))
                        {
                            DELETE_TRI(tr->buddy[0])
                                add_item_to_list(tr, tr->buddy[0]);
                        }
                        if (tr->buddy[1] && !tr->buddy[1]->deleted && e_cansee(pts[ti] - tr->buddy[1]->pt0, tr->buddy[1]->n, 0))
                        {
                            DELETE_TRI(tr->buddy[1])
                                add_item_to_list(tr, tr->buddy[1]);
                        }
                        if (tr->buddy[2] && !tr->buddy[2]->deleted && e_cansee(pts[ti] - tr->buddy[2]->pt0, tr->buddy[2]->n, 0))
                        {
                            DELETE_TRI(tr->buddy[2])
                                add_item_to_list(tr, tr->buddy[2]);
                        }
                        tr = tr->next;
                    } while (tr != trbest);

                    // delete near-visible triangles around deleted area edges to preserve hole convexity
                    // do as many iterations as needed
                    do
                    {
                        tr = trstart;
                        trlast = tr->prev;
                        next_iter = 0;
                        do
                        {
                            trnext = tr->next;
                            if (e_cansee(pts[ti] - tr->pt0, tr->n, -0.001f))
                            {
                                delbuds = tr->buddy[0]->deleted + tr->buddy[1]->deleted + tr->buddy[2]->deleted;
                                if (delbuds >= 2)   // delete triangles that have 2+ buddies deleted
                                {
                                    if (tr == trlast)
                                    {
                                        trlast = tr->next;
                                    }
                                    DELETE_TRI(tr);
                                    next_iter = 1;
                                }
                                else if (delbuds == 1)   // follow triangle fan around both shared edge ends
                                {
                                    int bi, bi0, bi1, nfantris, fandir;
                                    qhtritem* fantris[64], * tr1;

                                    for (bi0 = 0; bi0 < 3 && !tr->buddy[bi0]->deleted; bi0++)
                                    {
                                        ;                                                 // bi0 - deleted buddy index
                                    }
                                    for (fandir = -1; fandir <= 1; fandir += 2) // follow fans in 2 possible directions
                                    {
                                        tr1 = tr;
                                        bi1 = bi0;
                                        nfantris = 0;
                                        do
                                        {
                                            if (nfantris == 64)
                                            {
                                                break;
                                            }
                                            bi = bi1 + fandir;
                                            if (bi > 2)
                                            {
                                                bi -= 3;
                                            }
                                            if (bi < 0)
                                            {
                                                bi += 3;
                                            }
                                            for (bi1 = 0; bi1 < 3 && tr1->buddy[bi]->buddy[bi1] != tr1; bi1++)
                                            {
                                                ;
                                            }
                                            fantris[nfantris++] = tr1; // store this triangle in a temporary fan list
                                            tr1 = tr1->buddy[bi];
                                            bi = bi1;                   // go to the next fan triangle
                                            if (!e_cansee(pts[ti] - tr1->pt0, tr1->n, -0.002f))
                                            {
                                                break; // discard this fan
                                            }
                                            if (tr1->deleted)
                                            {
                                                if (tr1 != tr->buddy[bi0])
                                                {
                                                    // delete fan only if it ended on _another_ deleted triangle
                                                    for (--nfantris; nfantris >= 0; nfantris--)
                                                    {
                                                        if (fantris[nfantris] == trlast)
                                                        {
                                                            trlast = fantris[nfantris]->next;
                                                        }
                                                        DELETE_TRI(fantris[nfantris])
                                                    }
                                                    next_iter = 1;
                                                }
                                                break;     // fan end
                                            }
                                        } while (true);
                                    }
                                }
                            }
                            if (tr == trlast)
                            {
                                break;
                            }
                            tr = trnext;
                        } while (tr);
                    } while (next_iter && trstart);

                    if (!trstart || trstart->deleted)
                    {
                        n = 0;
                        goto endqhull;
                    }

                    // find triangles that shared an edge with deleted triangles
                    trnew = 0;
                    tr = trstart;
                    do
                    {
                        for (i = 0; i < 3; i++)
                        {
                            if (tr->buddy[i]->deleted)
                            {
                                // create a new triangle
                                if (trend >= trdata + trdatasz)
                                {
                                    qhtritem* trdata_new = new qhtritem[trdatasz += 256];
                                    memcpy(trdata_new, trdata, (trend - trdata) * sizeof(qhtritem));
                                    intptr_t diff = (intptr_t)trdata_new - (intptr_t)trdata;
                                    for (n = 0; n < trdatasz - 256; n++)
                                    {
                                        relocate_tritem(trdata_new + n, diff);
                                    }
                                    relocate_ptritem(trend, diff);
                                    relocate_ptritem(trstart, diff);
                                    relocate_ptritem(trnext, diff);
                                    relocate_ptritem(tr, diff);
                                    relocate_ptritem(trbest, diff);
                                    relocate_ptritem(trnew, diff);
                                    if (trdata != trbuf)
                                    {
                                        delete[] trdata;
                                    }
                                    trdata = trdata_new;
                                }
                                trend->idx[0] = static_cast<int>(ptmax - ptlist);
                                trend->idx[1] = tr->idx[i == 2 ? 0 : i + 1];
                                trend->idx[2] = tr->idx[i];
                                trend->ptassoc = 0;
                                trend->deleted = 0;
                                trend->n = pts[trend->idx[1]] - pts[trend->idx[0]] ^ pts[trend->idx[2]] - pts[trend->idx[0]];
                                trend->pt0 = pts[trend->idx[0]];
                                trend->buddy[1] = tr;
                                tr->buddy[i] = trend;
                                trend->buddy[0] = trend->buddy[2] = 0;
                                add_item_to_list(trnew, trend++);
                            }
                        }
                        tr = tr->next;
                    } while (tr != trstart);

                    // sort pointers to the new triangles by their 2nd vertex index
                    n = static_cast<int>(trend - trnew);
                    if (tmparr_sz < n)
                    {
                        if (tmparr_idx != tmparr_idx_buf)
                        {
                            delete[] tmparr_idx;
                        }
                        if (tmparr_ptr != tmparr_ptr_buf)
                        {
                            delete[] tmparr_ptr;
                        }
                        tmparr_idx = new int[n];
                        tmparr_ptr = new qhtritem * [n];
                    }
                    for (tr = trnew, i = 0; tr < trend; tr++, i++)
                    {
                        tmparr_idx[i] = tr->idx[2];
                        tmparr_ptr[i] = tr;
                    }
                    qsort(tmparr_idx, (void**)tmparr_ptr, 0, static_cast<int>(trend - trnew - 1));

                    // find 0th buddy for each new triangle (i.e. the triangle, which has its idx[2]==tr->idx[1]
                    for (tr = trnew; tr < trend; tr++)
                    {
                        i = bin_search(tmparr_idx, n, tr->idx[1]);
                        tr->buddy[0] = tmparr_ptr[i];
                        tmparr_ptr[i]->buddy[2] = tr;
                    }
                    for (tr = trnew; tr < trend; tr++)
                    {
                        if (!tr->buddy[0] || !tr->buddy[2])
                        {
                            goto endqh;
                        }
                    }

                    // assign all points from the deleted triangles to the new triangles
                    associate_ptlist_with_trilist(ptdeleted, trnew, ptlist, pts);
                    // add new triangles to the list
                    merge_lists(trnext, trnew);
                }
                else if (trnext == trstart)
                {
                    break; // all triangles in queue have no associated vertices
                }
                tr = trnext;
            } while (++iter < maxiter);

        endqh:

            // build the final triangle list
            for (tr = trstart, n = 1; tr->next != trstart; tr = tr->next, n++)
            {
                ;
            }
            if (!pTris)
            {
                pTris = !qmalloc ? new index_t[n * 3] : (index_t*)qmalloc(sizeof(index_t) * n * 3);
            }
            i = 0;
            tr = trstart;
            do
            {
                pTris[i] = tr->idx[0];
                pTris[i + 1] = tr->idx[1];
                pTris[i + 2] = tr->idx[2];
                tr = tr->next;
                i += 3;
            } while (tr != trstart);

        endqhull:
            if (ptlist != ptbuf)
            {
                delete[] ptlist;
            }
            if (tmparr_idx != tmparr_idx_buf)
            {
                delete[] tmparr_idx;
            }
            if (tmparr_ptr != tmparr_ptr_buf)
            {
                delete[] tmparr_ptr;
            }
            if (trdata != trbuf)
            {
                delete[] trdata;
            }

            return n;
        }
#undef DELETE_TRI
    } // namespace qhull_IMPL
    static int qhull(strided_pointer<Vec3> _pts, int npts, index_t*& pTris, qhullmalloc qmalloc)
    {
        return qhull_IMPL::qhull(_pts, npts, pTris, qmalloc);
    }

    namespace TriangulatePoly_IMPL
    {
        static int g_bSaferBooleans = 1;
        static int g_nTriangulationErrors;
        static int g_bBruteforceTriangulation = 0;


        struct vtxthunk
        {
            vtxthunk* next[2];
            vtxthunk* jump;
            vector2df* pt;
            int bProcessed;
        };

        static int TriangulatePolyBruteforce(vector2df* pVtx, int nVtx, int* pTris, int szTriBuf)
        {
            int i, nThunks, nNonEars, nTris = 0;
            vtxthunk* ptr, * ptr0, bufThunks[32], * pThunks = nVtx <= 31 ? bufThunks : new vtxthunk[nVtx + 1];

            ptr = ptr0 = pThunks;
            for (i = nThunks = 0; i < nVtx; i++)
            {
                if (!is_unused(pVtx[i].x))
                {
                    pThunks[nThunks].next[0] = pThunks + nThunks - 1;
                    pThunks[nThunks].next[1] = pThunks + nThunks + 1;
                    pThunks[nThunks].pt = pVtx + i;
                    ptr = pThunks + nThunks++;
                }
            }
            if (nThunks < 3)
            {
                return 0;
            }
            ptr->next[1] = ptr0;
            ptr0->next[0] = ptr;
            for (i = 0; i < nThunks; i++)
            {
                pThunks[i].bProcessed = (*pThunks[i].next[1]->pt - *pThunks[i].pt ^ *pThunks[i].next[0]->pt - *pThunks[i].pt) > 0;
            }

            for (nNonEars = 0; nNonEars < nThunks && nTris < szTriBuf; ptr0 = ptr0->next[1])
            {
                if (nThunks == 3)
                {
                    pTris[nTris * 3] = static_cast<int>(ptr0->pt - pVtx);
                    pTris[nTris * 3 + 1] = static_cast<int>(ptr0->next[1]->pt - pVtx);
                    pTris[nTris * 3 + 2] = static_cast<int>(ptr0->next[0]->pt - pVtx);
                    nTris++;
                    break;
                }
                for (i = 0; (*ptr0->next[1]->pt - *ptr0->pt ^ *ptr0->next[0]->pt - *ptr0->pt) < 0 && i < nThunks; ptr0 = ptr0->next[1], i++)
                {
                    ;
                }
                if (i == nThunks)
                {
                    break;
                }
                for (ptr = ptr0->next[1]->next[1]; ptr != ptr0->next[0] && ptr->bProcessed; ptr = ptr->next[1])
                {
                    ;                                                                                       // find the 1st non-convex vertex after ptr0
                }
                for (; ptr != ptr0->next[0] && min(min(*ptr0->pt - *ptr0->next[0]->pt ^ *ptr->pt - *ptr0->next[0]->pt,
                    *ptr0->next[1]->pt - *ptr0->pt ^ *ptr->pt - *ptr0->pt),
                    *ptr0->next[0]->pt - *ptr0->next[1]->pt ^ *ptr->pt - *ptr0->next[1]->pt) < 0; ptr = ptr->next[1])
                {
                    ;
                }
                if (ptr == ptr0->next[0]) // vertex is an ear, output the corresponding triangle
                {
                    pTris[nTris * 3] = static_cast<int>(ptr0->pt - pVtx);
                    pTris[nTris * 3 + 1] = static_cast<int>(ptr0->next[1]->pt - pVtx);
                    pTris[nTris * 3 + 2] = static_cast<int>(ptr0->next[0]->pt - pVtx);
                    nTris++;
                    ptr0->next[1]->next[0] = ptr0->next[0];
                    ptr0->next[0]->next[1] = ptr0->next[1];
                    nThunks--;
                    nNonEars = 0;
                }
                else
                {
                    nNonEars++;
                }
            }

            if (pThunks != bufThunks)
            {
                delete[] pThunks;
            }
            return nTris;
        }

        static int TriangulatePoly(vector2df* pVtx, int nVtx, int* pTris, int szTriBuf)
        {
            if (nVtx < 3)
            {
                return 0;
            }
            vtxthunk* pThunks, * pPrevThunk, * pContStart, ** pSags, ** pBottoms, * pPinnacle, * pBounds[2], * pPrevBounds[2], * ptr, * ptr_next;
            vtxthunk bufThunks[32], * bufSags[16], * bufBottoms[16];
            int i, nThunks, nBottoms = 0, nSags = 0, iBottom = 0, nConts = 0, j, isag, nThunks0, nTris = 0, nPrevSags, nTrisCnt, iter, nDegenTris = 0;
            float ymax, ymin, e, area0 = 0, area1 = 0, cntarea, minCntArea;

            isag = is_unused(pVtx[0].x);
            ymin = ymax = pVtx[isag].y;
            for (i = isag; i < nVtx; i++)
            {
                if (!is_unused(pVtx[i].x))
                {
                    ymin = min(ymin, pVtx[i].y);
                    ymax = max(ymax, pVtx[i].y);
                }
            }
            e = (ymax - ymin) * 0.0005f;
            for (i = 1 + isag; i < nVtx; i++)
            {
                if (!is_unused(pVtx[i].x))
                {
                    j = i < nVtx - 1 && !is_unused(pVtx[i + 1].x) ? i + 1 : isag;
                    if ((ymin = min(pVtx[j].y, pVtx[i - 1].y)) > pVtx[i].y - e)
                    {
                        if ((pVtx[j] - pVtx[i] ^ pVtx[i - 1] - pVtx[i]) > 0)
                        {
                            nBottoms++; // we have a bottom
                        }
                        else if (ymin > pVtx[i].y + 1E-8f)
                        {
                            nSags++; // we have a sag
                        }
                    }
                }
                else
                {
                    nConts++;
                    isag = ++i;
                }
            }
            nSags += nConts;
            if ((nConts - 2) >> 31 & g_bBruteforceTriangulation)
            {
                return TriangulatePolyBruteforce(pVtx, nVtx, pTris, szTriBuf);
            }
            pThunks = nVtx + nSags * 2 <= sizeof(bufThunks) / sizeof(bufThunks[0]) ? bufThunks : new vtxthunk[nVtx + nSags * 2];

            for (i = nThunks = 0, pContStart = pPrevThunk = pThunks; i < nVtx; i++)
            {
                if (!is_unused(pVtx[i].x))
                {
                    pThunks[nThunks].next[1] = pThunks + nThunks;
                    pThunks[nThunks].next[1] = pPrevThunk->next[1];
                    pPrevThunk->next[1] = pThunks + nThunks;
                    pThunks[nThunks].next[0] = pPrevThunk;
                    pThunks[nThunks].jump = 0;
                    pPrevThunk = pThunks + nThunks;
                    pThunks[nThunks].bProcessed = 0;
                    pThunks[nThunks++].pt = &pVtx[i];
                }
                else
                {
                    pPrevThunk->next[1] = pContStart;
                    pContStart->next[0] = pThunks + nThunks - 1;
                    pContStart = pPrevThunk = pThunks + nThunks;
                }
            }

            for (i = j = 0, cntarea = 0, minCntArea = 1; i < nThunks; i++)
            {
                cntarea += *pThunks[i].pt ^ *pThunks[i].next[1]->pt;
                j++;
                if (pThunks[i].next[1] != pThunks + i + 1)
                {
                    if (j >= 3)
                    {
                        area0 += cntarea;
                        minCntArea = min(cntarea, minCntArea);
                    }
                    cntarea = 0;
                    j = 0;
                }
            }
            if (minCntArea > 0 && nConts > 1)
            {
                // if all contours are positive, triangulate them as separate (it's more safe)
                for (i = 0; i < nThunks; i++)
                {
                    if (pThunks[i].next[0] != pThunks + i - 1)
                    {
                        nTrisCnt = TriangulatePoly(pThunks[i].pt, static_cast<int>((pThunks[i].next[0]->pt - pThunks[i].pt) + 2), pTris + nTris * 3, szTriBuf - nTris * 3);
                        for (j = 0, isag = static_cast<int>(pThunks[i].pt - pVtx); j < nTrisCnt * 3; j++)
                        {
                            pTris[nTris * 3 + j] += isag;
                        }
                        i = static_cast<int>(pThunks[i].next[0] - pThunks);
                        nTris += nTrisCnt;
                    }
                }
                if (pThunks != bufThunks)
                {
                    delete[] pThunks;
                }
                return nTris;
            }

            pSags = nSags <= sizeof(bufSags) / sizeof(bufSags[0]) ? bufSags : new vtxthunk * [nSags];
            pBottoms = nSags + nBottoms <= sizeof(bufBottoms) / sizeof(bufBottoms[0]) ? bufBottoms : new vtxthunk * [nSags + nBottoms];

            for (i = nSags = nBottoms = 0; i < nThunks; i++)
            {
                if ((ymin = min(pThunks[i].next[1]->pt->y, pThunks[i].next[0]->pt->y)) > pThunks[i].pt->y - e)
                {
                    if ((*pThunks[i].next[1]->pt - *pThunks[i].pt ^ *pThunks[i].next[0]->pt - *pThunks[i].pt) >= 0)
                    {
                        pBottoms[nBottoms++] = pThunks + i; // we have a bottom
                    }
                    else if (ymin > pThunks[i].pt->y + e)
                    {
                        pSags[nSags++] = pThunks + i; // we have a sag
                    }
                }
            }
            iBottom = -1;
            pBounds[0] = pBounds[1] = pPrevBounds[0] = pPrevBounds[1] = 0;
            nThunks0 = nThunks;
            nPrevSags = nSags;
            iter = nThunks * 4;

            do
            {
            nextiter:
                if (!pBounds[0])   // if bounds are empty, get the next available bottom
                {
                    for (++iBottom; iBottom < nBottoms && !pBottoms[iBottom]->next[0]; iBottom++)
                    {
                        ;
                    }
                    if (iBottom >= nBottoms)
                    {
                        break;
                    }
                    pBounds[0] = pBounds[1] = pPinnacle = pBottoms[iBottom];
                }
                pBounds[0]->bProcessed = pBounds[1]->bProcessed = 1;
                if (pBounds[0] == pPrevBounds[0] && pBounds[1] == pPrevBounds[1] && nSags == nPrevSags || !pBounds[0]->next[0] || !pBounds[1]->next[0])
                {
                    pBounds[0] = pBounds[1] = 0;
                    continue;
                }
                pPrevBounds[0] = pBounds[0];
                pPrevBounds[1] = pBounds[1];
                nPrevSags = nSags;

                // check if left or right is a top
                for (i = 0; i < 2; i++)
                {
                    if (pBounds[i]->next[0]->pt->y < pBounds[i]->pt->y && pBounds[i]->next[1]->pt->y <= pBounds[i]->pt->y &&
                        (*pBounds[i]->next[0]->pt - *pBounds[i]->pt ^ *pBounds[i]->next[1]->pt - *pBounds[i]->pt) > 0)
                    {
                        if (pBounds[i]->jump)
                        {
                            do
                            {
                                ptr = pBounds[i]->jump;
                                pBounds[i]->jump = 0;
                                pBounds[i] = ptr;
                            } while (pBounds[i]->jump);
                        }
                        else
                        {
                            pBounds[i]->jump = pBounds[i ^ 1];
                            pBounds[0] = pBounds[1] = 0;
                            goto nextiter;
                        }
                        if (!pBounds[0]->next[0] || !pBounds[1]->next[0])
                        {
                            pBounds[0] = pBounds[1] = 0;
                            goto nextiter;
                        }
                    }
                }
                i = isneg(pBounds[1]->next[1]->pt->y - pBounds[0]->next[0]->pt->y);
                ymax = pBounds[i ^ 1]->next[i ^ 1]->pt->y;
                ymin = min(pBounds[0]->pt->y, pBounds[1]->pt->y);

                for (j = 0, isag = -1; j < nSags; j++)
                {
                    if (inrange(pSags[j]->pt->y, ymin, ymax) &&                           // find a sag in next left-left-right-next right quad
                        pSags[j] != pBounds[0]->next[0] && pSags[j] != pBounds[1]->next[1] &&
                        (*pBounds[0]->pt - *pBounds[0]->next[0]->pt ^ *pSags[j]->pt - *pBounds[0]->next[0]->pt) >= 0 &&
                        (*pBounds[1]->pt - *pBounds[0]->pt ^ *pSags[j]->pt - *pBounds[0]->pt) >= 0 &&
                        (*pBounds[1]->next[1]->pt - *pBounds[1]->pt ^ *pSags[j]->pt - *pBounds[1]->pt) >= 0 &&
                        (*pBounds[0]->next[0]->pt - *pBounds[1]->next[1]->pt ^ *pSags[j]->pt - *pBounds[1]->next[1]->pt) >= 0)
                    {
                        ymax = pSags[j]->pt->y;
                        isag = j;
                    }
                }

                if (isag >= 0) // build a bridge between the sag and the highest active point
                {
                    if (pSags[isag]->next[0])
                    {
                        pPinnacle->next[1]->next[0] = pThunks + nThunks;
                        pSags[isag]->next[0]->next[1] = pThunks + nThunks + 1;
                        pThunks[nThunks].next[0] = pThunks + nThunks + 1;
                        pThunks[nThunks].next[1] = pPinnacle->next[1];
                        pThunks[nThunks + 1].next[1] = pThunks + nThunks;
                        pThunks[nThunks + 1].next[0] = pSags[isag]->next[0];
                        pPinnacle->next[1] = pSags[isag];
                        pSags[isag]->next[0] = pPinnacle;
                        pThunks[nThunks].pt = pPinnacle->pt;
                        pThunks[nThunks + 1].pt = pSags[isag]->pt;
                        pThunks[nThunks].jump = pThunks[nThunks + 1].jump = 0;
                        pThunks[nThunks].bProcessed = pThunks[nThunks + 1].bProcessed = 0;
                        if (pBounds[1] == pPinnacle)
                        {
                            pBounds[1] = pThunks + nThunks;
                        }
                        for (ptr = pThunks + nThunks, j = 0; ptr != pBounds[1]->next[1] && j < nThunks; ptr = ptr->next[1], j++)
                        {
                            if (min(ptr->next[0]->pt->y, ptr->next[1]->pt->y) > ptr->pt->y)  // ptr is a bottom
                            {
                                pBottoms[nBottoms++] = ptr;
                                break;
                            }
                        }
                        pBounds[1] = pPinnacle;
                        pPinnacle = pSags[isag];
                        nThunks += 2;
                    }
                    for (j = isag; j < nSags - 1; j++)
                    {
                        pSags[j] = pSags[j + 1];
                    }
                    --nSags;
                    continue;
                }

                // create triangles featuring the new vertex
                for (ptr = pBounds[i]; ptr != pBounds[i ^ 1] && nTris < szTriBuf; ptr = ptr_next)
                {
                    if ((*ptr->next[i ^ 1]->pt - *ptr->pt ^ *ptr->next[i]->pt - *ptr->pt) * (1 - i * 2) > 0 || pBounds[0]->next[0] == pBounds[1]->next[1])
                    {
                        // output the triangle
                        pTris[nTris * 3] = static_cast<int>(pBounds[i]->next[i]->pt - pVtx);
                        pTris[nTris * 3 + 1 + i] = static_cast<int>(ptr->pt - pVtx);
                        pTris[nTris * 3 + 2 - i] = static_cast<int>(ptr->next[i ^ 1]->pt - pVtx);
                        vector2df edge0 = pVtx[pTris[nTris * 3 + 1]] - pVtx[pTris[nTris * 3]], edge1 = pVtx[pTris[nTris * 3 + 2]] - pVtx[pTris[nTris * 3]];
                        float darea = edge0 ^ edge1;
                        area1 += darea;
                        nDegenTris += isneg(sqr(darea) - sqr(0.02f) * (edge0 * edge0) * (edge1 * edge1));
                        nTris++;
                        ptr->next[i ^ 1]->next[i] = ptr->next[i];
                        ptr->next[i]->next[i ^ 1] = ptr->next[i ^ 1];
                        pBounds[i] = ptr_next = ptr->next[i ^ 1];
                        if (pPinnacle == ptr)
                        {
                            pPinnacle = ptr->next[i];
                        }
                        ptr->next[0] = ptr->next[1] = 0;
                        ptr->bProcessed = 1;
                    }
                    else
                    {
                        break;
                    }
                }

                if ((pBounds[i] = pBounds[i]->next[i]) == pBounds[i ^ 1]->next[i ^ 1])
                {
                    pBounds[0] = pBounds[1] = 0;
                }
                else if (pBounds[i]->pt->y > pPinnacle->pt->y)
                {
                    pPinnacle = pBounds[i];
                }
            } while (nTris < szTriBuf && --iter);

            if (pThunks != bufThunks)
            {
                delete[] pThunks;
            }
            if (pBottoms != bufBottoms)
            {
                delete[] pBottoms;
            }
            if (pSags != bufSags)
            {
                delete[] pSags;
            }

            int bProblem = nTris<nThunks0 - nConts * 2 || fabs_tpl(area0 - area1)>area0 * 0.003f || nTris >= szTriBuf;
            if (bProblem || nDegenTris)
            {
                if (nConts == 1)
                {
                    return TriangulatePolyBruteforce(pVtx, nVtx, pTris, szTriBuf);
                }
                else
                {
                    g_nTriangulationErrors += bProblem;
                }
            }

            return nTris;
        }
    } // namespace TriangulatePoly_IMPL
    static int TriangulatePoly(vector2df* pVtx, int nVtx, int* pTris, int szTriBuf)
    {
        return TriangulatePoly_IMPL::TriangulatePoly(pVtx, nVtx, pTris, szTriBuf);
    }

    namespace polynomial_tpl_IMPL
    {
        template<class ftype, int degree>
        class polynomial_tpl
        {
        public:
            explicit polynomial_tpl() { denom = (ftype)1; };
            explicit polynomial_tpl(ftype op) { zero(); data[degree] = op; }
            AZ_FORCE_INLINE polynomial_tpl& zero()
            {
                for (int i = 0; i <= degree; i++)
                {
                    data[i] = 0;
                }
                denom = (ftype)1;
                return *this;
            }
            polynomial_tpl(const polynomial_tpl<ftype, degree>& src) { *this = src; }
            polynomial_tpl& operator=(const polynomial_tpl<ftype, degree>& src)
            {
                denom = src.denom;
                for (int i = 0; i <= degree; i++)
                {
                    data[i] = src.data[i];
                }
                return *this;
            }
            template<int degree1>
            AZ_FORCE_INLINE polynomial_tpl& operator=(const polynomial_tpl<ftype, degree1>& src)
            {
                int i;
                denom = src.denom;
                for (i = 0; i <= min(degree, degree1); i++)
                {
                    data[i] = src.data[i];
                }
                for (; i < degree; i++)
                {
                    data[i] = 0;
                }
                return *this;
            }
            AZ_FORCE_INLINE polynomial_tpl& set(ftype* pdata)
            {
                for (int i = 0; i <= degree; i++)
                {
                    data[degree - i] = pdata[i];
                }
                return *this;
            }

            AZ_FORCE_INLINE ftype& operator[](int idx) { return data[idx]; }

            void calc_deriviative(polynomial_tpl<ftype, degree>& deriv, int curdegree = degree) const;

            AZ_FORCE_INLINE polynomial_tpl& fixsign()
            {
                ftype sg = sgnnz(denom);
                denom *= sg;
                for (int i = 0; i <= degree; i++)
                {
                    data[i] *= sg;
                }
                return *this;
            }

            int findroots(ftype start, ftype end, ftype* proots, int nIters = 20, int curdegree = degree, bool noDegreeCheck = false) const;
            int nroots(ftype start, ftype end) const;

            AZ_FORCE_INLINE ftype eval(ftype x) const
            {
                ftype res = 0;
                for (int i = degree; i >= 0; i--)
                {
                    res = res * x + data[i];
                }
                return res;
            }
            AZ_FORCE_INLINE ftype eval(ftype x, int subdegree) const
            {
                ftype res = data[subdegree];
                for (int i = subdegree - 1; i >= 0; i--)
                {
                    res = res * x + data[i];
                }
                return res;
            }

            AZ_FORCE_INLINE polynomial_tpl& operator+=(ftype op) { data[0] += op * denom; return *this; }
            AZ_FORCE_INLINE polynomial_tpl& operator-=(ftype op) { data[0] -= op * denom; return *this; }
            AZ_FORCE_INLINE polynomial_tpl operator*(ftype op) const
            {
                polynomial_tpl<ftype, degree> res;
                res.denom = denom;
                for (int i = 0; i <= degree; i++)
                {
                    res.data[i] = data[i] * op;
                }
                return res;
            }
            AZ_FORCE_INLINE polynomial_tpl& operator*=(ftype op)
            {
                for (int i = 0; i <= degree; i++)
                {
                    data[i] *= op;
                }
                return *this;
            }
            AZ_FORCE_INLINE polynomial_tpl operator/(ftype op) const
            {
                polynomial_tpl<ftype, degree> res = *this;
                res.denom = denom * op;
                return res;
            }
            AZ_FORCE_INLINE polynomial_tpl& operator/=(ftype op) { denom *= op;   return *this; }

            AZ_FORCE_INLINE polynomial_tpl<ftype, degree * 2> sqr() const { return *this * *this; }

            ftype denom;
            ftype data[degree + 1];
        };

        template <class ftype>
        struct tagPolyE
        {
            inline static ftype polye() { return (ftype)1E-10; }
        };

        template<>
        inline float tagPolyE<float>::polye() { return 1e-6f; }

        template <class ftype>
        inline ftype polye() { return tagPolyE<ftype>::polye(); }

        // Don't use this macro; use AZStd::max instead. This is only here to make the template const arguments below readable
        // and because Visual Studio 2013 doesn't have a const_expr version of std::max
        #define deprecated_degmax(degree1, degree2) (((degree1) > (degree2)) ? (degree1) : (degree2))

        template<class ftype, int degree>
        AZ_FORCE_INLINE polynomial_tpl<ftype, degree> operator+(const polynomial_tpl<ftype, degree>& pn, ftype op)
        {
            polynomial_tpl<ftype, degree> res = pn;
            res.data[0] += op * res.denom;
            return res;
        }
        template<class ftype, int degree>
        AZ_FORCE_INLINE polynomial_tpl<ftype, degree> operator-(const polynomial_tpl<ftype, degree>& pn, ftype op)
        {
            polynomial_tpl<ftype, degree> res = pn;
            res.data[0] -= op * res.denom;
            return res;
        }

        template<class ftype, int degree>
        AZ_FORCE_INLINE polynomial_tpl<ftype, degree> operator+(ftype op, const polynomial_tpl<ftype, degree>& pn)
        {
            polynomial_tpl<ftype, degree> res = pn;
            res.data[0] += op * res.denom;
            return res;
        }
        template<class ftype, int degree>
        AZ_FORCE_INLINE polynomial_tpl<ftype, degree> operator-(ftype op, const polynomial_tpl<ftype, degree>& pn)
        {
            polynomial_tpl<ftype, degree> res = pn;
            res.data[0] -= op * res.denom;
            for (int i = 0; i <= degree; i++)
            {
                res.data[i] = -res.data[i];
            }
            return res;
        }
        template<class ftype, int degree>
        polynomial_tpl<ftype, degree * 2> AZ_FORCE_INLINE psqr(const polynomial_tpl<ftype, degree>& op) { return op * op; }

        template <class ftype, int degree1, int degree2>
        AZ_FORCE_INLINE polynomial_tpl<ftype, deprecated_degmax(degree1, degree2)> operator+(const polynomial_tpl<ftype, degree1>& op1, const polynomial_tpl<ftype, degree2>& op2)
        {
            polynomial_tpl<ftype, deprecated_degmax(degree1, degree2)> res;
            int i;
            for (i = 0; i <= min(degree1, degree2); i++)
            {
                res.data[i] = op1.data[i] * op2.denom + op2.data[i] * op1.denom;
            }
            for (; i <= degree1; i++)
            {
                res.data[i] = op1.data[i] * op2.denom;
            }
            for (; i <= degree2; i++)
            {
                res.data[i] = op2.data[i] * op1.denom;
            }
            res.denom = op1.denom * op2.denom;
            return res;
        }
        template <class ftype, int degree1, int degree2>
        AZ_FORCE_INLINE polynomial_tpl<ftype, deprecated_degmax(degree1, degree2)> operator-(const polynomial_tpl<ftype, degree1>& op1, const polynomial_tpl<ftype, degree2>& op2)
        {
            polynomial_tpl<ftype, deprecated_degmax(degree1, degree2)> res;
            int i;
            for (i = 0; i <= min(degree1, degree2); i++)
            {
                res.data[i] = op1.data[i] * op2.denom - op2.data[i] * op1.denom;
            }
            for (; i <= degree1; i++)
            {
                res.data[i] = op1.data[i] * op2.denom;
            }
            for (; i <= degree2; i++)
            {
                res.data[i] = op2.data[i] * op1.denom;
            }
            res.denom = op1.denom * op2.denom;
            return res;
        }

        template <class ftype, int degree1, int degree2>
        AZ_FORCE_INLINE polynomial_tpl<ftype, degree1>& operator+=(polynomial_tpl<ftype, degree1>& op1, const polynomial_tpl<ftype, degree2>& op2)
        {
            for (int i = 0; i < min(degree1, degree2); i++)
            {
                op1.data[i] = op1.data[i] * op2.denom + op2.data[i] * op1.denom;
            }
            op1.denom *= op2.denom;
            return op1;
        }
        template <class ftype, int degree1, int degree2>
        AZ_FORCE_INLINE polynomial_tpl<ftype, degree1>& operator-=(polynomial_tpl<ftype, degree1>& op1, const polynomial_tpl<ftype, degree2>& op2)
        {
            for (int i = 0; i < min(degree1, degree2); i++)
            {
                op1.data[i] = op1.data[i] * op2.denom - op2.data[i] * op1.denom;
            }
            op1.denom *= op2.denom;
            return op1;
        }

        template <class ftype, int degree1, int degree2>
        AZ_FORCE_INLINE polynomial_tpl<ftype, degree1 + degree2> operator*(const polynomial_tpl<ftype, degree1>& op1, const polynomial_tpl<ftype, degree2>& op2)
        {
            polynomial_tpl<ftype, degree1 + degree2> res;
            res.zero();
            int j;
            switch (degree1)
            {
            case 8:
                for (j = 0; j <= degree2; j++)
                {
                    res.data[8 + j] += op1.data[8] * op2.data[j];
                }
            case 7:
                for (j = 0; j <= degree2; j++)
                {
                    res.data[7 + j] += op1.data[7] * op2.data[j];
                }
            case 6:
                for (j = 0; j <= degree2; j++)
                {
                    res.data[6 + j] += op1.data[6] * op2.data[j];
                }
            case 5:
                for (j = 0; j <= degree2; j++)
                {
                    res.data[5 + j] += op1.data[5] * op2.data[j];
                }
            case 4:
                for (j = 0; j <= degree2; j++)
                {
                    res.data[4 + j] += op1.data[4] * op2.data[j];
                }
            case 3:
                for (j = 0; j <= degree2; j++)
                {
                    res.data[3 + j] += op1.data[3] * op2.data[j];
                }
            case 2:
                for (j = 0; j <= degree2; j++)
                {
                    res.data[2 + j] += op1.data[2] * op2.data[j];
                }
            case 1:
                for (j = 0; j <= degree2; j++)
                {
                    res.data[1 + j] += op1.data[1] * op2.data[j];
                }
            case 0:
                for (j = 0; j <= degree2; j++)
                {
                    res.data[0 + j] += op1.data[0] * op2.data[j];
                }
            }
            res.denom = op1.denom * op2.denom;
            return res;
        }


        template <class ftype>
        AZ_FORCE_INLINE void polynomial_divide(const polynomial_tpl<ftype, 8>& num, const polynomial_tpl<ftype, 8>& den, polynomial_tpl<ftype, 8>& quot,
            polynomial_tpl<ftype, 8>& rem, int degree1, int degree2)
        {
            int i, j, k, l;
            ftype maxel;
            for (i = 0; i <= degree1; i++)
            {
                rem.data[i] = num.data[i];
            }
            for (i = 0; i <= degree1 - degree2; i++)
            {
                quot.data[i] = 0;
            }
            for (i = 1, maxel = fabs_tpl(num.data[0]); i <= degree1; i++)
            {
                maxel = max(maxel, num.data[i]);
            }
            for (maxel *= polye<ftype>(); degree1 >= 0 && fabs_tpl(num.data[degree1]) < maxel; degree1--)
            {
                ;
            }
            for (i = 1, maxel = fabs_tpl(den.data[0]); i <= degree2; i++)
            {
                maxel = max(maxel, den.data[i]);
            }
            for (maxel *= polye<ftype>(); degree2 >= 0 && fabs_tpl(den.data[degree2]) < maxel; degree2--)
            {
                ;
            }
            rem.denom = num.denom;
            quot.denom = (ftype)1;
            if (degree1 < 0 || degree2 < 0)
            {
                return;
            }

            for (k = degree1 - degree2, l = degree1; l >= degree2; l--, k--)
            {
                quot.data[k] = rem.data[l] * den.denom;
                quot.denom *= den.data[degree2];
                for (i = degree1 - degree2; i > k; i--)
                {
                    quot.data[i] *= den.data[degree2];
                }
                for (i = degree2 - 1, j = l - 1; i >= 0; i--, j--)
                {
                    rem.data[j] = rem.data[j] * den.data[degree2] - den.data[i] * rem.data[l];
                }
                for (; j >= 0; j--)
                {
                    rem.data[j] *= den.data[degree2];
                }
                rem.denom *= den.data[degree2];
            }
        }

        template <class ftype, int degree1, int degree2>
        AZ_FORCE_INLINE polynomial_tpl<ftype, degree1 - degree2> operator/(const polynomial_tpl<ftype, degree1>& num, const polynomial_tpl<ftype, degree2>& den)
        {
            polynomial_tpl<ftype, degree1 - degree2> quot;
            polynomial_tpl<ftype, degree1> rem;
            polynomial_divide((polynomial_tpl<ftype, 8>&)num, (polynomial_tpl<ftype, 8>&)den, (polynomial_tpl<ftype, 8>&)quot,
                (polynomial_tpl<ftype, 8>&)rem, degree1, degree2);
            return quot;
        }
        template <class ftype, int degree1, int degree2>
        AZ_FORCE_INLINE polynomial_tpl<ftype, degree2 - 1> operator%(const polynomial_tpl<ftype, degree1>& num, const polynomial_tpl<ftype, degree2>& den)
        {
            polynomial_tpl<ftype, degree1 - degree2> quot;
            polynomial_tpl<ftype, degree1> rem;
            polynomial_divide((polynomial_tpl<ftype, 8>&)num, (polynomial_tpl<ftype, 8>&)den, (polynomial_tpl<ftype, 8>&)quot,
                (polynomial_tpl<ftype, 8>&)rem, degree1, degree2);
            return (polynomial_tpl<ftype, degree2 - 1>&)rem;
        }

        template <class ftype, int degree>
        AZ_FORCE_INLINE void polynomial_tpl<ftype, degree>::calc_deriviative(polynomial_tpl<ftype, degree>& deriv, int curdegree) const
        {
            for (int i = 0; i < curdegree; i++)
            {
                deriv.data[i] = data[i + 1] * (i + 1);
            }
            deriv.denom = denom;
        }

        template<typename to_t, typename from_t>
        to_t* convert_type(from_t* input)
        {
            typedef union
            {
                to_t* to;
                from_t* from;
            } convert_union;
            convert_union u;
            u.from = input;
            return u.to;
        }

        template <class ftype, int degree>
        AZ_FORCE_INLINE int polynomial_tpl<ftype, degree>::nroots(ftype start, ftype end) const
        {
            polynomial_tpl<ftype, degree> f[degree + 1];
            int i, j, sg_a, sg_b;
            ftype val, prevval;

            calc_deriviative(f[0]);
            polynomial_divide(*convert_type<polynomial_tpl<ftype, 8> >(this), *convert_type< polynomial_tpl<ftype, 8> >(&f[0]), *convert_type<polynomial_tpl<ftype, 8> >(&f[degree]),
                *convert_type<polynomial_tpl<ftype, 8> >(&f[1]), degree, degree - 1);
            f[1].denom = -f[1].denom;
            for (i = 2; i < degree; i++)
            {
                polynomial_divide(*convert_type<polynomial_tpl<ftype, 8> >(&f[i - 2]), *convert_type<polynomial_tpl<ftype, 8> >(&f[i - 1]), *convert_type<polynomial_tpl<ftype, 8> >(&f[degree]),
                    *convert_type<polynomial_tpl<ftype, 8> >(&f[i]), degree + 1 - i, degree - i);
                f[i].denom = -f[i].denom;
                if (fabs_tpl(f[i].denom) > (ftype)1E10)
                {
                    for (j = 0; j <= degree - 1 - i; j++)
                    {
                        f[i].data[j] *= (ftype)1E-10;
                    }
                    f[i].denom *= (ftype)1E-10;
                }
            }

            prevval = eval(start) * denom;
            for (i = sg_a = 0; i < degree; i++, prevval = val)
            {
                val = f[i].eval(start, degree - 1 - i) * f[i].denom;
                sg_a += isneg(val * prevval);
            }

            prevval = eval(end) * denom;
            for (i = sg_b = 0; i < degree; i++, prevval = val)
            {
                val = f[i].eval(end, degree - 1 - i) * f[i].denom;
                sg_b += isneg(val * prevval);
            }

            return fabs_tpl(sg_a - sg_b);
        }

        template<class ftype>
        AZ_FORCE_INLINE ftype cubert_tpl(ftype x) { return fabs_tpl(x) > (ftype)1E-20 ? exp_tpl(log_tpl(fabs_tpl(x)) * (ftype)(1.0 / 3)) * sgnnz(x) : x; }
        template<class ftype>
        AZ_FORCE_INLINE ftype pow_tpl(ftype x, ftype pow) { return fabs_tpl(x) > (ftype)1E-20 ? exp_tpl(log_tpl(fabs_tpl(x)) * pow) * sgnnz(x) : x; }
        template<class ftype>
        AZ_FORCE_INLINE void swap(ftype* ptr, int i, int j) { ftype t = ptr[i]; ptr[i] = ptr[j]; ptr[j] = t; }

        template <class ftype, int maxdegree>
        int polynomial_tpl<ftype, maxdegree>::findroots(ftype start, ftype end, ftype* proots, int nIters, int degree, bool noDegreeCheck) const
        {
            int i, j, nRoots = 0;
            ftype maxel;
            if (!noDegreeCheck)
            {
                for (i = 1, maxel = fabs_tpl(data[0]); i <= degree; i++)
                {
                    maxel = max(maxel, data[i]);
                }
                for (maxel *= polye<ftype>(); degree > 0 && fabs_tpl(data[degree]) <= maxel; degree--)
                {
                    ;
                }
            }

            if constexpr (maxdegree >= 1)
            {
                if (degree == 1)
                {
                    proots[0] = data[0] / data[1];
                    nRoots = 1;
                }
            }

            if constexpr (maxdegree >= 2)
            {
                if (degree == 2)
                {
                    ftype a, b, c, d, bound[2], sg;

                    a = data[2];
                    b = data[1];
                    c = data[0];
                    d = aznumeric_cast<ftype>(sgnnz(a));
                    a *= d;
                    b *= d;
                    c *= d;
                    d = b * b - a * c * 4;
                    bound[0] = start * a * 2 + b;
                    bound[1] = end * a * 2 + b;
                    sg = aznumeric_cast<ftype>((sgnnz(bound[0] * bound[1]) + 1) >> 1);
                    bound[0] *= bound[0];
                    bound[1] *= bound[1];
                    bound[isneg(fabs_tpl(bound[1]) - fabs_tpl(bound[0]))] *= sg;

                    if (isnonneg(d) & inrange(d, bound[0], bound[1]))
                    {
                        d = sqrt_tpl(d);
                        a = (ftype)0.5 / a;
                        proots[nRoots] = (-b - d) * a;
                        nRoots += inrange(proots[nRoots], start, end);
                        proots[nRoots] = (-b + d) * a;
                        nRoots += inrange(proots[nRoots], start, end);
                    }
                }
            }

            if constexpr (maxdegree >= 3)
            {
                if (degree == 3)
                {
                    ftype t, a, b, c, a3, p, q, Q, Qr, Ar, Ai, phi;

                    t = (ftype)1.0 / data[3];
                    a = data[2] * t;
                    b = data[1] * t;
                    c = data[0] * t;
                    a3 = a * (ftype)(1.0 / 3);
                    p = b - a * a3;
                    q = (a3 * b - c) * (ftype)0.5 - cube(a3);
                    Q = cube(p * (ftype)(1.0 / 3)) + q * q;
                    Qr = sqrt_tpl(fabs_tpl(Q));

                    if (Q > 0)
                    {
                        proots[0] = cubert_tpl(q + Qr) + cubert_tpl(q - Qr) - a3;
                        nRoots = 1;
                    }
                    else
                    {
                        phi = atan2_tpl(Qr, q) * (ftype)(1.0 / 3);
                        t = pow_tpl(Qr * Qr + q * q, (ftype)(1.0 / 6));
                        Ar = t * cos_tpl(phi);
                        Ai = t * sin_tpl(phi);
                        proots[0] = 2 * Ar - a3;
                        proots[1] = aznumeric_cast<ftype>(-Ar + Ai * sqrt3 - a3);
                        proots[2] = aznumeric_cast<ftype>(-Ar - Ai * sqrt3 - a3);
                        i = idxmax3(proots);
                        swap(proots, i, 2);
                        i = isneg(proots[0] - proots[1]);
                        swap(proots, i, 1);
                        nRoots = 3;
                    }
                }
            }

            if constexpr (maxdegree >= 4)
            {
                if (degree == 4)
                {
                    ftype t, a3, a2, a1, a0, y, R, D, E, subroots[3];
                    const ftype e = (ftype)1E-9;

                    t = (ftype)1.0 / data[4];
                    a3 = data[3] * t;
                    a2 = data[2] * t;
                    a1 = data[1] * t;
                    a0 = data[0] * t;
                    polynomial_tpl<ftype, 3> p3aux;
                    ftype kp3aux[] = { 1, -a2, a1 * a3 - 4 * a0, 4 * a2 * a0 - a1 * a1 - a3 * a3 * a0 };
                    p3aux.set(kp3aux);
                    if (!p3aux.findroots((ftype)-1E20, (ftype)1E20, subroots))
                    {
                        return 0;
                    }
                    R = a3 * a3 * (ftype)0.25 - a2 + (y = subroots[0]);

                    if (R > -e)
                    {
                        if (R < e)
                        {
                            D = E = a3 * a3 * (ftype)(3.0 / 4) - 2 * a2;
                            t = y * y - 4 * a0;
                            if (t < -e)
                            {
                                return 0;
                            }
                            t = 2 * sqrt_tpl(max((ftype)0, t));
                        }
                        else
                        {
                            R = sqrt_tpl(max((ftype)0, R));
                            D = E = a3 * a3 * (ftype)(3.0 / 4) - R * R - 2 * a2;
                            t = (4 * a3 * a2 - 8 * a1 - a3 * a3 * a3) / R * (ftype)0.25;
                        }
                        if (D + t > -e)
                        {
                            D = sqrt_tpl(max((ftype)0, D + t));
                            proots[nRoots++] = a3 * (ftype)-0.25 + (R - D) * (ftype)0.5;
                            proots[nRoots++] = a3 * (ftype)-0.25 + (R + D) * (ftype)0.5;
                        }
                        if (E - t > -e)
                        {
                            E = sqrt_tpl(max((ftype)0, E - t));
                            proots[nRoots++] = a3 * (ftype)-0.25 - (R + E) * (ftype)0.5;
                            proots[nRoots++] = a3 * (ftype)-0.25 - (R - E) * (ftype)0.5;
                        }
                        if (nRoots == 4)
                        {
                            i = idxmax3(proots);
                            if (proots[3] < proots[i])
                            {
                                swap(proots, i, 3);
                            }
                            i = idxmax3(proots);
                            swap(proots, i, 2);
                            i = isneg(proots[0] - proots[1]);
                            swap(proots, i, 1);
                        }
                    }
                }
            }

            if constexpr (maxdegree > 4)
            {
                if (degree > 4)
                {
                    ftype roots[maxdegree + 1], prevroot, val, prevval[2], curval, bound[2], middle;
                    polynomial_tpl<ftype, maxdegree> deriv;
                    int nExtremes, iter, iBound;
                    calc_deriviative(deriv);

                    // find a subset of deriviative extremes between start and end
                    for (nExtremes = deriv.findroots(start, end, roots + 1, nIters, degree - 1) + 1; nExtremes > 1 && roots[nExtremes - 1] > end; nExtremes--)
                    {
                        ;
                    }
                    for (i = 1; i < nExtremes && roots[i] < start; i++)
                    {
                        ;
                    }
                    roots[i - 1] = start;
                    PREFAST_ASSUME(nExtremes < maxdegree + 1);
                    roots[nExtremes++] = end;

                    for (prevroot = start, prevval[0] = eval(start, degree), nRoots = 0; i < nExtremes; prevval[0] = val, prevroot = roots[i++])
                    {
                        val = eval(roots[i], degree);
                        if (val * prevval[0] < 0)
                        {
                            // we have exactly one root between prevroot and roots[i]
                            bound[0] = prevroot;
                            bound[1] = roots[i];
                            iter = 0;
                            do
                            {
                                middle = (bound[0] + bound[1]) * (ftype)0.5;
                                curval = eval(middle, degree);
                                iBound = isneg(prevval[0] * curval);
                                bound[iBound] = middle;
                                prevval[iBound] = curval;
                            } while (++iter < nIters);
                            proots[nRoots++] = middle;
                        }
                    }
                }
            }

            for (i = 0; i < nRoots && proots[i] < start; i++)
            {
                ;
            }
            for (; nRoots > i&& proots[nRoots - 1] > end; nRoots--)
            {
                ;
            }
            for (j = i; j < nRoots; j++)
            {
                proots[j - i] = proots[j];
            }

            return nRoots - i;
        }
    } // namespace polynomial_tpl_IMPL
    template<class ftype, int degree>
    using polynomial_tpl = polynomial_tpl_IMPL::polynomial_tpl<ftype, degree>;

    typedef polynomial_tpl<real, 3> P3;
    typedef polynomial_tpl<real, 2> P2;
    typedef polynomial_tpl<real, 1> P1;
    typedef polynomial_tpl<float, 3> P3f;
    typedef polynomial_tpl<float, 2> P2f;
    typedef polynomial_tpl<float, 1> P1f;
} // namespace LegacyCryPhysicsUtils
