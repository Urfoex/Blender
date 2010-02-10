/**
 * $Id$
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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Contributor(s): Campbell Barton
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include <stdlib.h>

#include "RNA_define.h"
#include "RNA_types.h"

#include "rna_internal.h"

#include "DNA_group_types.h"

#ifdef RNA_RUNTIME

#include "DNA_scene_types.h"
#include "DNA_object_types.h"

#include "BKE_group.h"

#include "WM_api.h"
#include "WM_types.h"

static PointerRNA rna_Group_objects_get(CollectionPropertyIterator *iter)
{
	ListBaseIterator *internal= iter->internal;

	/* we are actually iterating a GroupObject list, so override get */
	return rna_pointer_inherit_refine(&iter->parent, &RNA_Object, ((GroupObject*)internal->link)->ob);
}

static void rna_Group_objects_link(Group *group, bContext *C, ReportList *reports, Object *object)
{
	if(!add_to_group(group, object, CTX_data_scene(C), NULL)) {
		BKE_reportf(reports, RPT_ERROR, "Object \"%s\" already in group \"%s\".", object->id.name+2, group->id.name+2);
		return;
	}

	WM_main_add_notifier(NC_OBJECT|ND_DRAW, &object->id);
}

static void rna_Group_objects_unlink(Group *group, bContext *C, ReportList *reports, Object *object)
{
	if(!rem_from_group(group, object, CTX_data_scene(C), NULL)) {
		BKE_reportf(reports, RPT_ERROR, "Object \"%s\" not in group \"%s\".", object->id.name+2, group->id.name+2);
		return;
	}

	WM_main_add_notifier(NC_OBJECT|ND_DRAW, &object->id);
}

#else

/* group.objects */
static void rna_def_group_objects(BlenderRNA *brna, PropertyRNA *cprop)
{
	StructRNA *srna;
//	PropertyRNA *prop;

	FunctionRNA *func;
	PropertyRNA *parm;
	
	RNA_def_property_srna(cprop, "GroupObjects");
	srna= RNA_def_struct(brna, "GroupObjects", NULL);
	RNA_def_struct_sdna(srna, "Group");
	RNA_def_struct_ui_text(srna, "Group Objects", "Collection of group objects");

	/* add object */
	func= RNA_def_function(srna, "link", "rna_Group_objects_link");
	RNA_def_function_flag(func, FUNC_USE_CONTEXT|FUNC_USE_REPORTS);
	RNA_def_function_ui_description(func, "Add this object to a group");
	/* object to add */
	parm= RNA_def_pointer(func, "object", "Object", "", "Object to add.");
	RNA_def_property_flag(parm, PROP_REQUIRED);

	/* remove object */
	func= RNA_def_function(srna, "unlink", "rna_Group_objects_unlink");
	RNA_def_function_ui_description(func, "Remove this object to a group");
	RNA_def_function_flag(func, FUNC_USE_CONTEXT|FUNC_USE_REPORTS);
	/* object to remove */
	parm= RNA_def_pointer(func, "object", "Object", "", "Object to remove.");
	RNA_def_property_flag(parm, PROP_REQUIRED);
}


void RNA_def_group(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;

	srna= RNA_def_struct(brna, "Group", "ID");
	RNA_def_struct_ui_text(srna, "Group", "Group of Object datablocks");
	RNA_def_struct_ui_icon(srna, ICON_GROUP);

	prop= RNA_def_property(srna, "dupli_offset", PROP_FLOAT, PROP_TRANSLATION);
	RNA_def_property_float_sdna(prop, NULL, "dupli_ofs");
	RNA_def_property_ui_text(prop, "Dupli Offset", "Offset from the origin to use when instancing as DupliGroup");
	RNA_def_property_ui_range(prop, -10000.0, 10000.0, 10, 4);

	prop= RNA_def_property(srna, "layer", PROP_BOOLEAN, PROP_LAYER);
	RNA_def_property_boolean_sdna(prop, NULL, "layer", 1);
	RNA_def_property_array(prop, 20);
	RNA_def_property_ui_text(prop, "Dupli Layers", "Layers visible when this groups is instanced as a dupli");


	prop= RNA_def_property(srna, "objects", PROP_COLLECTION, PROP_NONE);
	RNA_def_property_collection_sdna(prop, NULL, "gobject", NULL);
	RNA_def_property_struct_type(prop, "Object");
	RNA_def_property_ui_text(prop, "Objects", "A collection of this groups objects");
	RNA_def_property_collection_funcs(prop, 0, 0, 0, "rna_Group_objects_get", 0, 0, 0);

	rna_def_group_objects(brna, prop);

}

#endif

