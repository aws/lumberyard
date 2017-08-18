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

#ifndef CRYINCLUDE_CRYPHYSICS_UNPROJECTIONCHECKS_H
#define CRYINCLUDE_CRYPHYSICS_UNPROJECTIONCHECKS_H
#pragma once


struct unprojection_mode
{
    unprojection_mode() { bCheckContact = 0; tmin = 0; maxcos = 0.1f; }
    int imode;
    Vec3 dir;   // direction or rotation axis
    Vec3 center; // center of rotation
    float vel; // linear or angular velocity
    float tmax; // maximum unprojection length (not time)
    float tmin; // minimum unprojection length
    float minPtDist; // tolerance value
    float maxcos;
    int bCheckContact;

    Matrix33 R0;
    Vec3 offset0;
};

typedef int (* unprojection_check)(unprojection_mode*, const primitive*, int, const primitive*, int, contact*, geom_contact_area*);

int default_unprojection(unprojection_mode*, const primitive*, int, const primitive*, int, contact*, geom_contact_area*);
int tri_tri_lin_unprojection(unprojection_mode* pmode, const triangle* ptri1, int iFeature1, const triangle* ptri2, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int tri_box_lin_unprojection(unprojection_mode* pmode, const triangle* ptri, int iFeature1, const box* pbox, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int box_tri_lin_unprojection(unprojection_mode* pmode, const box* pbox, int iFeature1, const triangle* ptri, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int tri_cylinder_lin_unprojection(unprojection_mode* pmode, const triangle* ptri, int iFeature1, const cylinder* pcyl, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int cylinder_tri_lin_unprojection(unprojection_mode* pmode, const cylinder* pcyl, int iFeature1, const triangle* ptri, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int tri_sphere_lin_unprojection(unprojection_mode* pmode, const triangle* ptri, int iFeature1, const sphere* psphere, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int sphere_tri_lin_unprojection(unprojection_mode* pmode, const sphere* psphere, int iFeature1, const triangle* ptri, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int tri_capsule_lin_unprojection(unprojection_mode* pmode, const triangle* ptri, int iFeature1, const capsule* pcaps, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int capsule_tri_lin_unprojection(unprojection_mode* pmode, const capsule* pcaps, int iFeature1, const triangle* ptri, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int tri_ray_lin_unprojection(unprojection_mode* pmode, const triangle* ptri, int iFeature1, const ray* pray, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int ray_tri_lin_unprojection(unprojection_mode* pmode, const ray* pray, int iFeature1, const triangle* ptri, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int tri_plane_lin_unprojection(unprojection_mode* pmode, const triangle* ptri, int iFeature1, const plane* pplane, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int plane_tri_lin_unprojection(unprojection_mode* pmode, const plane* pplane, int iFeature1, const triangle* ptri, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int box_box_lin_unprojection(unprojection_mode* pmode, const box* pbox1, int iFeature1, const box* pbox2, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int box_cylinder_lin_unprojection(unprojection_mode* pmode, const box* pbox, int iFeature1, const cylinder* pcyl, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int cylinder_box_lin_unprojection(unprojection_mode* pmode, const cylinder* pcyl, int iFeature1, const box* pbox, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int box_sphere_lin_unprojection(unprojection_mode* pmode, const box* pbox, int iFeature1, const sphere* psph, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int sphere_box_lin_unprojection(unprojection_mode* pmode, const sphere* psph, int iFeature1, const box* pbox, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int box_capsule_lin_unprojection(unprojection_mode* pmode, const box* pbox, int iFeature1, const capsule* pcaps, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int capsule_box_lin_unprojection(unprojection_mode* pmode, const capsule* pcaps, int iFeature1, const box* pbox, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int cyl_cyl_lin_unprojection(unprojection_mode* pmode, const cylinder* pcyl1, int iFeature1, const cylinder* pcyl2, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int cylinder_sphere_lin_unprojection(unprojection_mode* pmode, const cylinder* pcyl, int iFeature1, const sphere* psph, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int sphere_cylinder_lin_unprojection(unprojection_mode* pmode, const sphere* psph, int iFeature1, const cylinder* pcyl, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int cylinder_capsule_lin_unprojection(unprojection_mode* pmode, const cylinder* pbox, int iFeature1, const capsule* pcaps, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int capsule_cylinder_lin_unprojection(unprojection_mode* pmode, const capsule* pcaps, int iFeature1, const cylinder* pcyl, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int capsule_capsule_lin_unprojection(unprojection_mode* pmode, const capsule* pcaps1, int iFeature1, const capsule* pcaps2, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int sphere_sphere_lin_unprojection(unprojection_mode* pmode, const sphere* psph1, int iFeature1, const sphere* psph2, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int sphere_capsule_lin_unprojection(unprojection_mode* pmode, const sphere* psph, int iFeature1, const capsule* pcaps, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int capsule_sphere_lin_unprojection(unprojection_mode* pmode, const capsule* pcaps, int iFeature1, const sphere* psph, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int ray_box_lin_unprojection(unprojection_mode* pmode, const ray* pray, int iFeature1, const box* pbox, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int box_ray_lin_unprojection(unprojection_mode* pmode, const box* pbox, int iFeature1, const ray* pray, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int ray_cylinder_lin_unprojection(unprojection_mode* pmode, const ray* pray, int iFeature1, const cylinder* pcyl, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int cylinder_ray_lin_unprojection(unprojection_mode* pmode, const cylinder* pcyl, int iFeature1, const ray* pray, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int ray_sphere_lin_unprojection(unprojection_mode* pmode, const ray* pray, int iFeature1, const sphere* psph, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int sphere_ray_lin_unprojection(unprojection_mode* pmode, const sphere* psph, int iFeature1, const ray* pray, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int ray_capsule_lin_unprojection(unprojection_mode* pmode, const ray* pray, int iFeature1, const capsule* pcaps, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int capsule_ray_lin_unprojection(unprojection_mode* pmode, const capsule* pcaps, int iFeature1, const ray* pray, int iFeature2,
    contact* pcontact, geom_contact_area* parea);

int tri_tri_rot_unprojection(unprojection_mode* pmode, const triangle* ptri1, int iFeature1, const triangle* ptri2, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int tri_ray_rot_unprojection(unprojection_mode* pmode, const triangle* ptri, int iFeature1, const ray* pray, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int ray_tri_rot_unprojection(unprojection_mode* pmode, const ray* pray, int iFeature1, const triangle* ptri, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int cyl_ray_rot_unprojection(unprojection_mode* pmode, const cylinder* pcyl, int iFeature1, const ray* pray, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int ray_cyl_rot_unprojection(unprojection_mode* pmode, const ray* pray, int iFeature1, const cylinder* pcyl, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int box_ray_rot_unprojection(unprojection_mode* pmode, const box* pbox, int iFeature1, const ray* pray, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int ray_box_rot_unprojection(unprojection_mode* pmode, const ray* pray, int iFeature1, const box* pbox, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int capsule_ray_rot_unprojection(unprojection_mode* pmode, const capsule* pcaps, int iFeature1, const ray* pray, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int ray_capsule_rot_unprojection(unprojection_mode* pmode, const ray* pray, int iFeature1, const capsule* pcaps, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int sphere_ray_rot_unprojection(unprojection_mode* pmode, const sphere* psph, int iFeature1, const ray* pray, int iFeature2,
    contact* pcontact, geom_contact_area* parea);
int ray_sphere_rot_unprojection(unprojection_mode* pmode, const ray* pray, int iFeature1, const sphere* psph, int iFeature2,
    contact* pcontact, geom_contact_area* parea);


class CUnprojectionChecker
{
public:
    //CUnprojectionChecker();
    ILINE int Check(unprojection_mode* pmode, int type1, int type2, const primitive* prim1, int iFeature1, const primitive* prim2, int iFeature2,
        contact* pcontact, geom_contact_area* parea = 0)
    {
        unprojection_check func = table[pmode->imode][type1][type2];
        return func(pmode, prim1, iFeature1, prim2, iFeature2, pcontact, parea);
    }
    ILINE int CheckExists(int imode, int type1, int type2)
    {
        unprojection_check func = table[imode][type1][type2];
        return func != default_unprojection;
    }

    static unprojection_check table[2][NPRIMS][NPRIMS];
};
extern CUnprojectionChecker g_Unprojector;

#endif // CRYINCLUDE_CRYPHYSICS_UNPROJECTIONCHECKS_H
