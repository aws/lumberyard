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

#include "StdAfx.h"
#include "SurfaceTypeValidator.h"
#include "IMaterial.h"
#include "IMaterialEffects.h"
#include "IGame.h"
#include "IGameFramework.h"
#include "Material/Material.h"

void CSurfaceTypeValidator::Validate()
{
    IObjectManager* pObjectManager = GetIEditor()->GetObjectManager();
    ISurfaceTypeManager* pSurfaceTypeManager = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();
    std::set<string> reportedMaterialNames;

    CBaseObjectsArray objects;
    pObjectManager->GetObjects(objects);
    for (CBaseObjectsArray::iterator itObject = objects.begin(); itObject != objects.end(); ++itObject)
    {
        CBaseObject* pObject = *itObject;
        IPhysicalEntity* pPhysicalEntity = pObject ? pObject->GetCollisionEntity() : 0;

        // query part number with GetStatus(pe_status_nparts)
        pe_status_nparts numPartsQuery;
        int numParts = pPhysicalEntity ? pPhysicalEntity->GetStatus(&numPartsQuery) : 0;

        // iterate the parts, query GetParams(pe_params_part) for each part index (0..n-1)
        // (set ipart to the part index and MARK_UNUSED partid before each getstatus call)
        pe_params_part partQuery;
        for (partQuery.ipart = 0; partQuery.ipart < numParts; ++partQuery.ipart)
        {
            MARK_UNUSED partQuery.partid;
            int queryResult = pPhysicalEntity->GetParams(&partQuery);

            // if flags & (geom_colltype0|geom_colltype_player), check nMats entries in pMatMapping. they should contain valid game mat ids
            if (queryResult != 0 && partQuery.flagsAND & (geom_colltype0 | geom_colltype_player))
            {
                CMaterial* pEditorMaterial = pObject ? pObject->GetRenderMaterial() : 0;
                _smart_ptr<IMaterial> pMaterial = pEditorMaterial ? pEditorMaterial->GetMatInfo() : 0;

                char usedSubMaterials[MAX_SUB_MATERIALS];
                memset(usedSubMaterials, 0, sizeof(usedSubMaterials));
                if (pMaterial && reportedMaterialNames.insert(pMaterial->GetName()).second)
                {
                    GetUsedSubMaterials(&partQuery, usedSubMaterials);
                }

                int surfaceTypeIDs[MAX_SUB_MATERIALS];
                memset(surfaceTypeIDs, 0, sizeof(surfaceTypeIDs));
                int numSurfaces = pMaterial ? pMaterial->FillSurfaceTypeIds(surfaceTypeIDs) : 0;

                for (int surfaceIDIndex = 0; surfaceIDIndex < numSurfaces; ++surfaceIDIndex)
                {
                    string materialSpec;
                    materialSpec.Format("%s:%d", pMaterial->GetName(), surfaceIDIndex + 1);
                    if (usedSubMaterials[surfaceIDIndex] && surfaceTypeIDs[surfaceIDIndex] < 0 && reportedMaterialNames.insert(materialSpec).second)
                    {
                        CErrorRecord err;
                        err.error = QObject::tr("Physicalized object has material (%1) with invalid surface type.").arg(materialSpec.c_str());
                        err.pObject = pObject;
                        err.severity = CErrorRecord::ESEVERITY_WARNING;
                        GetIEditor()->GetErrorReport()->ReportError(err);
                    }
                }
            }
        }

        pe_status_placeholder spc;
        pe_action_reset ar;
        if (pPhysicalEntity && pPhysicalEntity->GetStatus(&spc) && spc.pFullEntity)
        {
            pPhysicalEntity->Action(&ar, 1);
        }
    }
}

void CSurfaceTypeValidator::GetUsedSubMaterials(pe_params_part* pPart, char usedSubMaterials[])
{
    phys_geometry* pGeometriesToCheck[2];
    int numGeometriesToCheck = 0;
    if (pPart->pPhysGeom)
    {
        pGeometriesToCheck[numGeometriesToCheck++] = pPart->pPhysGeom;
    }
    if (pPart->pPhysGeomProxy && pPart->pPhysGeomProxy != pPart->pPhysGeom)
    {
        pGeometriesToCheck[numGeometriesToCheck++] = pPart->pPhysGeomProxy;
    }

    for (int geometryToCheck = 0; geometryToCheck < numGeometriesToCheck; ++geometryToCheck)
    {
        phys_geometry* pGeometry = pGeometriesToCheck[geometryToCheck];

        IGeometry* pCollGeometry = pGeometry ? pGeometry->pGeom : 0;
        mesh_data* pMesh = (mesh_data*)(pCollGeometry && pCollGeometry->GetType() == GEOM_TRIMESH ? pCollGeometry->GetData() : 0);

        if (pMesh && pMesh->pMats)
        {
            for (int i = 0; i < pMesh->nTris; ++i)
            {
                const int subMatId = pMesh->pMats[i];
                if (subMatId >= 0 && subMatId < MAX_SUB_MATERIALS)
                {
                    usedSubMaterials[subMatId] = 1;
                }
            }
        }
        else
        {
            if (pGeometry)
            {
                const int subMatId = pGeometry->surface_idx;
                if (subMatId >= 0 && subMatId < MAX_SUB_MATERIALS)
                {
                    usedSubMaterials[subMatId] = 1;
                }
            }
        }
    }
}
