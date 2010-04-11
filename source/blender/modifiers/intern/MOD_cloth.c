/*
* $Id:
*
* ***** BEGIN GPL LICENSE BLOCK *****
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software  Foundation,
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
* The Original Code is Copyright (C) 2005 by the Blender Foundation.
* All rights reserved.
*
* Contributor(s): Daniel Dunbar
*                 Ton Roosendaal,
*                 Ben Batt,
*                 Brecht Van Lommel,
*                 Campbell Barton
*
* ***** END GPL LICENSE BLOCK *****
*
*/

#include "stddef.h"
#include "string.h"
#include "stdarg.h"
#include "math.h"
#include "float.h"

#include "BLI_kdtree.h"
#include "BLI_rand.h"
#include "BLI_uvproject.h"

#include "MEM_guardedalloc.h"

#include "DNA_armature_types.h"
#include "DNA_camera_types.h"
#include "DNA_curve_types.h"
#include "DNA_key_types.h"
#include "DNA_material_types.h"
#include "DNA_object_fluidsim.h"


#include "BKE_action.h"
#include "BKE_bmesh.h"
#include "BKE_booleanops.h"
#include "BKE_cloth.h"
#include "BKE_cdderivedmesh.h"
#include "BKE_displist.h"
#include "BKE_fluidsim.h"
#include "BKE_global.h"
#include "BKE_multires.h"
#include "BKE_key.h"
#include "BKE_lattice.h"
#include "BKE_material.h"
#include "BKE_mesh.h"
#include "BKE_modifier.h"
#include "BKE_object.h"
#include "BKE_paint.h"
#include "BKE_particle.h"
#include "BKE_pointcache.h"
#include "BKE_scene.h"
#include "BKE_smoke.h"
#include "BKE_softbody.h"
#include "BKE_subsurf.h"
#include "BKE_texture.h"

#include "depsgraph_private.h"
#include "BKE_deform.h"
#include "BKE_shrinkwrap.h"

#include "LOD_decimation.h"

#include "CCGSubSurf.h"

#include "RE_shader_ext.h"

#include "MOD_modifiertypes.h"


static void initData(ModifierData *md) 
{
	ClothModifierData *clmd = (ClothModifierData*) md;
	
	clmd->sim_parms = MEM_callocN(sizeof(ClothSimSettings), "cloth sim parms");
	clmd->coll_parms = MEM_callocN(sizeof(ClothCollSettings), "cloth coll parms");
	clmd->point_cache = BKE_ptcache_add(&clmd->ptcaches);
	
	/* check for alloc failing */
	if(!clmd->sim_parms || !clmd->coll_parms || !clmd->point_cache)
		return;
	
	cloth_init (clmd);
}

static DerivedMesh *applyModifier(ModifierData *md, Object *ob,
		DerivedMesh *derivedData, int useRenderParams, int isFinalCalc)
{
	ClothModifierData *clmd = (ClothModifierData*) md;
	DerivedMesh *result=NULL;
	
	/* check for alloc failing */
	if(!clmd->sim_parms || !clmd->coll_parms)
	{
		initData(md);
		
		if(!clmd->sim_parms || !clmd->coll_parms)
			return derivedData;
	}

	result = clothModifier_do(clmd, md->scene, ob, derivedData, useRenderParams, isFinalCalc);

	if(result)
	{
		CDDM_calc_normals(result);
		return result;
	}
	return derivedData;
}

static void updateDepgraph(
					 ModifierData *md, DagForest *forest, Scene *scene, Object *ob,
	  DagNode *obNode)
{
	ClothModifierData *clmd = (ClothModifierData*) md;
	
	Base *base;
	
	if(clmd)
	{
		for(base = scene->base.first; base; base= base->next) 
		{
			Object *ob1= base->object;
			if(ob1 != ob)
			{
				CollisionModifierData *coll_clmd = (CollisionModifierData *)modifiers_findByType(ob1, eModifierType_Collision);
				if(coll_clmd)
				{
					DagNode *curNode = dag_get_node(forest, ob1);
					dag_add_relation(forest, curNode, obNode, DAG_RL_DATA_DATA|DAG_RL_OB_DATA, "Cloth Collision");
				}
			}
		}
	}
}

static CustomDataMask requiredDataMask(Object *ob, ModifierData *md)
{
	CustomDataMask dataMask = 0;
	ClothModifierData *clmd = (ClothModifierData*)md;

	if(cloth_uses_vgroup(clmd))
		dataMask |= (1 << CD_MDEFORMVERT);

	if(clmd->sim_parms->shapekey_rest != 0)
		dataMask |= (1 << CD_CLOTH_ORCO);

	return dataMask;
}

static void copyData(ModifierData *md, ModifierData *target)
{
	ClothModifierData *clmd = (ClothModifierData*) md;
	ClothModifierData *tclmd = (ClothModifierData*) target;
	
	if(tclmd->sim_parms)
		MEM_freeN(tclmd->sim_parms);
	if(tclmd->coll_parms)
		MEM_freeN(tclmd->coll_parms);
	
	BKE_ptcache_free_list(&tclmd->ptcaches);
	tclmd->point_cache = NULL;
	
	tclmd->sim_parms = MEM_dupallocN(clmd->sim_parms);
	if(clmd->sim_parms->effector_weights)
		tclmd->sim_parms->effector_weights = MEM_dupallocN(clmd->sim_parms->effector_weights);
	tclmd->coll_parms = MEM_dupallocN(clmd->coll_parms);
	tclmd->point_cache = BKE_ptcache_copy_list(&tclmd->ptcaches, &clmd->ptcaches);
	tclmd->clothObject = NULL;
}

static int dependsOnTime(ModifierData *md)
{
	return 1;
}

static void freeData(ModifierData *md)
{
	ClothModifierData *clmd = (ClothModifierData*) md;
	
	if (clmd) 
	{
		if(G.rt > 0)
			printf("clothModifier_freeData\n");
		
		cloth_free_modifier_extern (clmd);
		
		if(clmd->sim_parms) {
			if(clmd->sim_parms->effector_weights)
				MEM_freeN(clmd->sim_parms->effector_weights);
			MEM_freeN(clmd->sim_parms);
		}
		if(clmd->coll_parms)
			MEM_freeN(clmd->coll_parms);	
		
		BKE_ptcache_free_list(&clmd->ptcaches);
		clmd->point_cache = NULL;
	}
}


ModifierTypeInfo modifierType_Cloth = {
	/* name */              "Cloth",
	/* structName */        "ClothModifierData",
	/* structSize */        sizeof(ClothModifierData),
	/* type */              eModifierTypeType_Nonconstructive,
	/* flags */             eModifierTypeFlag_AcceptsMesh |
							eModifierTypeFlag_UsesPointCache |
							eModifierTypeFlag_Single,

	/* copyData */          copyData,
	/* deformVerts */       0,
	/* deformVertsEM */     0,
	/* deformMatricesEM */  0,
	/* applyModifier */     applyModifier,
	/* applyModifierEM */   0,
	/* initData */          0,
	/* requiredDataMask */  requiredDataMask,
	/* freeData */          freeData,
	/* isDisabled */        0,
	/* updateDepgraph */    updateDepgraph,
	/* dependsOnTime */     dependsOnTime,
	/* foreachObjectLink */ 0,
	/* foreachIDLink */     0,
};
