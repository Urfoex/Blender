/*
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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file blender/editors/sculpt_paint/paint_ops.c
 *  \ingroup edsculpt
 */

#include "MEM_guardedalloc.h"

#include <stdlib.h>
#include "BLI_listbase.h"
#include "BLI_string.h"
#include "BLI_utildefines.h"

#include "DNA_object_types.h"
#include "DNA_scene_types.h"
#include "DNA_brush_types.h"

#include "BKE_brush.h"
#include "BKE_context.h"
#include "BKE_paint.h"
#include "BKE_main.h"

#include "ED_sculpt.h"
#include "ED_screen.h"
#include "UI_resources.h"

#include "WM_api.h"
#include "WM_types.h"

#include "RNA_access.h"
#include "RNA_define.h"
#include "RNA_enum_types.h"

#include "paint_intern.h"
#include "sculpt_intern.h"

#include <string.h>
//#include <stdio.h>
#include <stddef.h>

/* Brush operators */
static int brush_add_exec(bContext *C, wmOperator *UNUSED(op))
{
	/*int type = RNA_enum_get(op->ptr, "type");*/
	Paint *paint = paint_get_active_from_context(C);
	struct Brush *br = paint_brush(paint);
	Main *bmain = CTX_data_main(C);

	if (br)
		br = BKE_brush_copy(br);
	else
		br = BKE_brush_add(bmain, "Brush");

	paint_brush_set(paint, br);

	return OPERATOR_FINISHED;
}

static void BRUSH_OT_add(wmOperatorType *ot)
{
	/* identifiers */
	ot->name = "Add Brush";
	ot->description = "Add brush by mode type";
	ot->idname = "BRUSH_OT_add";
	
	/* api callbacks */
	ot->exec = brush_add_exec;
	
	/* flags */
	ot->flag = OPTYPE_REGISTER | OPTYPE_UNDO;
}


static int brush_scale_size_exec(bContext *C, wmOperator *op)
{
	Scene *scene = CTX_data_scene(C);
	Paint  *paint =  paint_get_active_from_context(C);
	struct Brush  *brush =  paint_brush(paint);
	// Object *ob = CTX_data_active_object(C);
	float scalar = RNA_float_get(op->ptr, "scalar");

	if (brush) {
		// pixel radius
		{
			const int old_size = BKE_brush_size_get(scene, brush);
			int size = (int)(scalar * old_size);

			if (old_size == size) {
				if (scalar > 1) {
					size++;
				}
				else if (scalar < 1) {
					size--;
				}
			}
			CLAMP(size, 1, 2000); // XXX magic number

			BKE_brush_size_set(scene, brush, size);
		}

		// unprojected radius
		{
			float unprojected_radius = scalar * BKE_brush_unprojected_radius_get(scene, brush);

			if (unprojected_radius < 0.001f) // XXX magic number
				unprojected_radius = 0.001f;

			BKE_brush_unprojected_radius_set(scene, brush, unprojected_radius);
		}
	}

	return OPERATOR_FINISHED;
}

static void BRUSH_OT_scale_size(wmOperatorType *ot)
{
	/* identifiers */
	ot->name = "Scale Sculpt/Paint Brush Size";
	ot->description = "Change brush size by a scalar";
	ot->idname = "BRUSH_OT_scale_size";
	
	/* api callbacks */
	ot->exec = brush_scale_size_exec;
	
	/* flags */
	ot->flag = OPTYPE_REGISTER | OPTYPE_UNDO;

	RNA_def_float(ot->srna, "scalar", 1, 0, 2, "Scalar", "Factor to scale brush size by", 0, 2);
}

static int vertex_color_set_exec(bContext *C, wmOperator *UNUSED(op))
{
	Scene *scene = CTX_data_scene(C);
	Object *obact = CTX_data_active_object(C);
	unsigned int paintcol = vpaint_get_current_col(scene->toolsettings->vpaint);
	vpaint_fill(obact, paintcol);
	
	ED_region_tag_redraw(CTX_wm_region(C)); // XXX - should redraw all 3D views
	return OPERATOR_FINISHED;
}

static void PAINT_OT_vertex_color_set(wmOperatorType *ot)
{
	/* identifiers */
	ot->name = "Set Vertex Colors";
	ot->idname = "PAINT_OT_vertex_color_set";
	ot->description = "Fill the active vertex color layer with the current paint color";
	
	/* api callbacks */
	ot->exec = vertex_color_set_exec;
	ot->poll = vertex_paint_mode_poll;
	
	/* flags */
	ot->flag = OPTYPE_REGISTER | OPTYPE_UNDO;
}

static int brush_reset_exec(bContext *C, wmOperator *UNUSED(op))
{
	Paint *paint = paint_get_active_from_context(C);
	struct Brush *brush = paint_brush(paint);
	Object *ob = CTX_data_active_object(C);

	if (!ob) return OPERATOR_CANCELLED;

	if (ob->mode & OB_MODE_SCULPT)
		BKE_brush_sculpt_reset(brush);
	/* TODO: other modes */

	return OPERATOR_FINISHED;
}

static void BRUSH_OT_reset(wmOperatorType *ot)
{
	/* identifiers */
	ot->name = "Reset Brush";
	ot->description = "Return brush to defaults based on current tool";
	ot->idname = "BRUSH_OT_reset";
	
	/* api callbacks */
	ot->exec = brush_reset_exec;
	
	/* flags */
	ot->flag = OPTYPE_REGISTER | OPTYPE_UNDO;
}

static int brush_tool(const Brush *brush, size_t tool_offset)
{
	return *(((char *)brush) + tool_offset);
}

static void brush_tool_set(const Brush *brush, size_t tool_offset, int tool)
{
	*(((char *)brush) + tool_offset) = tool;
}

/* generic functions for setting the active brush based on the tool */
static Brush *brush_tool_cycle(Main *bmain, Brush *brush_orig, const int tool, const size_t tool_offset, const int ob_mode)
{
	struct Brush *brush;

	if (!brush_orig && !(brush_orig = bmain->brush.first)) {
		return NULL;
	}

	/* get the next brush with the active tool */
	for (brush = brush_orig->id.next ? brush_orig->id.next : bmain->brush.first;
	     brush != brush_orig;
	     brush = brush->id.next ? brush->id.next : bmain->brush.first)
	{
		if ((brush->ob_mode & ob_mode) &&
		    (brush_tool(brush, tool_offset) == tool))
		{
			return brush;
		}
	}

	return NULL;
}

static Brush *brush_tool_toggle(Main *bmain, Brush *brush_orig, const int tool, const size_t tool_offset, const int ob_mode)
{
	if (!brush_orig || brush_tool(brush_orig, tool_offset) != tool) {
		Brush *br;
		/* if the current brush is not using the desired tool, look
		 * for one that is */
		br = brush_tool_cycle(bmain, brush_orig, tool, tool_offset, ob_mode);
		/* store the previously-selected brush */
		if (br)
			br->toggle_brush = brush_orig;
		
		return br;
	}
	else if (brush_orig->toggle_brush &&
	         BLI_findindex(bmain->brush.first, brush_orig->toggle_brush) != -1)
	{
		/* if current brush is using the desired tool, try to toggle
		 * back to the previously selected brush (if it was set, and
		 * if it still exists) */
		return brush_orig->toggle_brush;
	}
	else
		return NULL;
}

static int brush_generic_tool_set(Main *bmain, Paint *paint, const int tool,
                                  const size_t tool_offset, const int ob_mode,
                                  const char *tool_name, int create_missing,
                                  int toggle)
{
	struct Brush *brush, *brush_orig = paint_brush(paint);

	if (toggle)
		brush = brush_tool_toggle(bmain, brush_orig, tool, tool_offset, ob_mode);
	else
		brush = brush_tool_cycle(bmain, brush_orig, tool, tool_offset, ob_mode);

	if (!brush && brush_tool(brush_orig, tool_offset) != tool && create_missing) {
		brush = BKE_brush_add(bmain, tool_name);
		brush_tool_set(brush, tool_offset, tool);
		brush->ob_mode = ob_mode;
		brush->toggle_brush = brush_orig;
	}

	if (brush) {
		paint_brush_set(paint, brush);
		WM_main_add_notifier(NC_BRUSH | NA_EDITED, brush);
		return OPERATOR_FINISHED;
	}
	else {
		return OPERATOR_CANCELLED;
	}
}

/* used in the PAINT_OT_brush_select operator */
#define OB_MODE_ACTIVE 0

static int brush_select_exec(bContext *C, wmOperator *op)
{
	Main *bmain = CTX_data_main(C);
	ToolSettings *toolsettings = CTX_data_tool_settings(C);
	Paint *paint = NULL;
	int tool, paint_mode = RNA_enum_get(op->ptr, "paint_mode");
	int create_missing = RNA_boolean_get(op->ptr, "create_missing");
	int toggle = RNA_boolean_get(op->ptr, "toggle");
	const char *tool_name = "Brush";
	size_t tool_offset;

	if (paint_mode == OB_MODE_ACTIVE) {
		Object *ob = CTX_data_active_object(C);
		if (ob) {
			/* select current paint mode */
			paint_mode = ob->mode &
			             (OB_MODE_SCULPT |
			              OB_MODE_VERTEX_PAINT |
			              OB_MODE_WEIGHT_PAINT |
			              OB_MODE_TEXTURE_PAINT);
		}
		else {
			return OPERATOR_CANCELLED;
		}
	}

	switch (paint_mode) {
		case OB_MODE_SCULPT:
			paint = &toolsettings->sculpt->paint;
			tool_offset = offsetof(Brush, sculpt_tool);
			tool = RNA_enum_get(op->ptr, "sculpt_tool");
			RNA_enum_name_from_value(brush_sculpt_tool_items, tool, &tool_name);
			break;
		case OB_MODE_VERTEX_PAINT:
			paint = &toolsettings->vpaint->paint;
			tool_offset = offsetof(Brush, vertexpaint_tool);
			tool = RNA_enum_get(op->ptr, "vertex_paint_tool");
			RNA_enum_name_from_value(brush_vertex_tool_items, tool, &tool_name);
			break;
		case OB_MODE_WEIGHT_PAINT:
			paint = &toolsettings->wpaint->paint;
			/* vertexpaint_tool is used for weight paint mode */
			tool_offset = offsetof(Brush, vertexpaint_tool);
			tool = RNA_enum_get(op->ptr, "weight_paint_tool");
			RNA_enum_name_from_value(brush_vertex_tool_items, tool, &tool_name);
			break;
		case OB_MODE_TEXTURE_PAINT:
			paint = &toolsettings->imapaint.paint;
			tool_offset = offsetof(Brush, imagepaint_tool);
			tool = RNA_enum_get(op->ptr, "texture_paint_tool");
			RNA_enum_name_from_value(brush_image_tool_items, tool, &tool_name);
			break;
		default:
			/* invalid paint mode */
			return OPERATOR_CANCELLED;
	}

	return brush_generic_tool_set(bmain, paint, tool, tool_offset,
	                              paint_mode, tool_name, create_missing,
	                              toggle);
}

static void PAINT_OT_brush_select(wmOperatorType *ot)
{
	static EnumPropertyItem paint_mode_items[] = {
		{OB_MODE_ACTIVE, "ACTIVE", 0, "Current", "Set brush for active paint mode"},
		{OB_MODE_SCULPT, "SCULPT", ICON_SCULPTMODE_HLT, "Sculpt", ""},
		{OB_MODE_VERTEX_PAINT, "VERTEX_PAINT", ICON_VPAINT_HLT, "Vertex Paint", ""},
		{OB_MODE_WEIGHT_PAINT, "WEIGHT_PAINT", ICON_WPAINT_HLT, "Weight Paint", ""},
		{OB_MODE_TEXTURE_PAINT, "TEXTURE_PAINT", ICON_TPAINT_HLT, "Texture Paint", ""},
		{0, NULL, 0, NULL, NULL}
	};

	/* identifiers */
	ot->name = "Brush Select";
	ot->description = "Select a paint mode's brush by tool type";
	ot->idname = "PAINT_OT_brush_select";

	/* api callbacks */
	ot->exec = brush_select_exec;

	/* flags */
	ot->flag = OPTYPE_REGISTER | OPTYPE_UNDO;

	/* props */
	RNA_def_enum(ot->srna, "paint_mode", paint_mode_items, OB_MODE_ACTIVE, "Paint Mode", "");
	RNA_def_enum(ot->srna, "sculpt_tool", brush_sculpt_tool_items, 0, "Sculpt Tool", "");
	RNA_def_enum(ot->srna, "vertex_paint_tool", brush_vertex_tool_items, 0, "Vertex Paint Tool", "");
	RNA_def_enum(ot->srna, "weight_paint_tool", brush_vertex_tool_items, 0, "Weight Paint Tool", "");
	RNA_def_enum(ot->srna, "texture_paint_tool", brush_image_tool_items, 0, "Texture Paint Tool", "");

	RNA_def_boolean(ot->srna, "toggle", 0, "Toggle", "Toggle between two brushes rather than cycling");
	RNA_def_boolean(ot->srna, "create_missing", 0, "Create Missing", "If the requested brush type does not exist, create a new brush");
}

static wmKeyMapItem *keymap_brush_select(wmKeyMap *keymap, int paint_mode,
                                         int tool, int keymap_type,
                                         int keymap_modifier)
{
	wmKeyMapItem *kmi;
	kmi = WM_keymap_add_item(keymap, "PAINT_OT_brush_select",
	                         keymap_type, KM_PRESS, keymap_modifier, 0);

	RNA_enum_set(kmi->ptr, "paint_mode", paint_mode);
	
	switch (paint_mode) {
		case OB_MODE_SCULPT:
			RNA_enum_set(kmi->ptr, "sculpt_tool", tool);
			break;
		case OB_MODE_VERTEX_PAINT:
			RNA_enum_set(kmi->ptr, "vertex_paint_tool", tool);
			break;
		case OB_MODE_WEIGHT_PAINT:
			RNA_enum_set(kmi->ptr, "weight_paint_tool", tool);
			break;
		case OB_MODE_TEXTURE_PAINT:
			RNA_enum_set(kmi->ptr, "texture_paint_tool", tool);
			break;
	}

	return kmi;
}

static int brush_uv_sculpt_tool_set_exec(bContext *C, wmOperator *op)
{
	Brush *brush;
	Scene *scene = CTX_data_scene(C);
	ToolSettings *ts = scene->toolsettings;
	ts->uv_sculpt_tool = RNA_enum_get(op->ptr, "tool");
	brush = ts->uvsculpt->paint.brush;
	/* To update toolshelf */
	WM_event_add_notifier(C, NC_BRUSH | NA_EDITED, brush);

	return OPERATOR_FINISHED;
}

static void BRUSH_OT_uv_sculpt_tool_set(wmOperatorType *ot)
{
	/* from rna_scene.c */
	extern EnumPropertyItem uv_sculpt_tool_items[];
	/* identifiers */
	ot->name = "UV Sculpt Tool Set";
	ot->description = "Set the UV sculpt tool";
	ot->idname = "BRUSH_OT_uv_sculpt_tool_set";

	/* api callbacks */
	ot->exec = brush_uv_sculpt_tool_set_exec;
	ot->poll = uv_sculpt_poll;

	/* flags */
	ot->flag = 0;

	/* props */
	ot->prop = RNA_def_enum(ot->srna, "tool", uv_sculpt_tool_items, 0, "Tool", "");
}

/**************************** registration **********************************/

void ED_operatortypes_paint(void)
{
	/* brush */
	WM_operatortype_append(BRUSH_OT_add);
	WM_operatortype_append(BRUSH_OT_scale_size);
	WM_operatortype_append(BRUSH_OT_curve_preset);
	WM_operatortype_append(BRUSH_OT_reset);

	/* note, particle uses a different system, can be added with existing operators in wm.py */
	WM_operatortype_append(PAINT_OT_brush_select);
	WM_operatortype_append(BRUSH_OT_uv_sculpt_tool_set);

	/* image */
	WM_operatortype_append(PAINT_OT_texture_paint_toggle);
	WM_operatortype_append(PAINT_OT_image_paint);
	WM_operatortype_append(PAINT_OT_sample_color);
	WM_operatortype_append(PAINT_OT_grab_clone);
	WM_operatortype_append(PAINT_OT_project_image);
	WM_operatortype_append(PAINT_OT_image_from_view);

	/* weight */
	WM_operatortype_append(PAINT_OT_weight_paint_toggle);
	WM_operatortype_append(PAINT_OT_weight_paint);
	WM_operatortype_append(PAINT_OT_weight_set);
	WM_operatortype_append(PAINT_OT_weight_from_bones);
	WM_operatortype_append(PAINT_OT_weight_gradient);
	WM_operatortype_append(PAINT_OT_weight_sample);
	WM_operatortype_append(PAINT_OT_weight_sample_group);

	/* uv */
	WM_operatortype_append(SCULPT_OT_uv_sculpt_stroke);

	/* vertex selection */
	WM_operatortype_append(PAINT_OT_vert_select_all);
	WM_operatortype_append(PAINT_OT_vert_select_ungrouped);

	/* vertex */
	WM_operatortype_append(PAINT_OT_vertex_paint_toggle);
	WM_operatortype_append(PAINT_OT_vertex_paint);
	WM_operatortype_append(PAINT_OT_vertex_color_set);

	/* face-select */
	WM_operatortype_append(PAINT_OT_face_select_linked);
	WM_operatortype_append(PAINT_OT_face_select_linked_pick);
	WM_operatortype_append(PAINT_OT_face_select_all);
	WM_operatortype_append(PAINT_OT_face_select_hide);
	WM_operatortype_append(PAINT_OT_face_select_reveal);

	/* partial visibility */
	WM_operatortype_append(PAINT_OT_hide_show);

	/* paint masking */
	WM_operatortype_append(PAINT_OT_mask_flood_fill);
}


static void ed_keymap_paint_brush_switch(wmKeyMap *keymap, const char *mode)
{
	wmKeyMapItem *kmi;
	int i;
	/* index 0-9 (zero key is tenth), shift key for index 10-19 */
	for (i = 0; i < 20; i++) {
		kmi = WM_keymap_add_item(keymap, "BRUSH_OT_active_index_set",
		                         ZEROKEY + ((i + 1) % 10), KM_PRESS, i < 10 ? 0 : KM_SHIFT, 0);
		RNA_string_set(kmi->ptr, "mode", mode);
		RNA_int_set(kmi->ptr, "index", i);
	}
}

static void ed_keymap_paint_brush_size(wmKeyMap *keymap, const char *UNUSED(path))
{
	wmKeyMapItem *kmi;

	kmi = WM_keymap_add_item(keymap, "BRUSH_OT_scale_size", LEFTBRACKETKEY, KM_PRESS, 0, 0);
	RNA_float_set(kmi->ptr, "scalar", 0.9);

	kmi = WM_keymap_add_item(keymap, "BRUSH_OT_scale_size", RIGHTBRACKETKEY, KM_PRESS, 0, 0);
	RNA_float_set(kmi->ptr, "scalar", 10.0 / 9.0); // 1.1111....
}

typedef enum {
	RC_COLOR = 1,
	RC_ROTATION = 2,
	RC_ZOOM = 4
} RCFlags;

static void set_brush_rc_path(PointerRNA *ptr, const char *brush_path,
                              const char *output_name, const char *input_name)
{
	char *path;

	path = BLI_sprintfN("%s.%s", brush_path, input_name);
	RNA_string_set(ptr, output_name, path);
	MEM_freeN(path);
}

static void set_brush_rc_props(PointerRNA *ptr, const char *paint,
                               const char *prop, const char *secondary_prop,
                               RCFlags flags)
{
	const char *ups_path = "tool_settings.unified_paint_settings";
	char *brush_path;

	brush_path = BLI_sprintfN("tool_settings.%s.brush", paint);

	set_brush_rc_path(ptr, brush_path, "data_path_primary", prop);
	if (secondary_prop) {
		set_brush_rc_path(ptr, ups_path, "use_secondary", secondary_prop);
		set_brush_rc_path(ptr, ups_path, "data_path_secondary", prop);
	}
	else {
		RNA_string_set(ptr, "use_secondary", "");
		RNA_string_set(ptr, "data_path_secondary", "");
	}
	set_brush_rc_path(ptr, brush_path, "color_path", "cursor_color_add");
	set_brush_rc_path(ptr, brush_path, "rotation_path", "texture_slot.angle");
	RNA_string_set(ptr, "image_id", brush_path);

	if (flags & RC_COLOR)
		set_brush_rc_path(ptr, brush_path, "fill_color_path", "color");
	else
		RNA_string_set(ptr, "fill_color_path", "");
	if (flags & RC_ZOOM)
		RNA_string_set(ptr, "zoom_path", "space_data.zoom");
	else
		RNA_string_set(ptr, "zoom_path", "");

	MEM_freeN(brush_path);
}

static void ed_keymap_paint_brush_radial_control(wmKeyMap *keymap, const char *paint,
                                                 RCFlags flags)
{
	wmKeyMapItem *kmi;
	/* only size needs to follow zoom, strength shows fixed size circle */
	int flags_nozoom = flags & (~RC_ZOOM);

	kmi = WM_keymap_add_item(keymap, "WM_OT_radial_control", FKEY, KM_PRESS, 0, 0);
	set_brush_rc_props(kmi->ptr, paint, "size", "use_unified_size", flags);

	kmi = WM_keymap_add_item(keymap, "WM_OT_radial_control", FKEY, KM_PRESS, KM_SHIFT, 0);
	set_brush_rc_props(kmi->ptr, paint, "strength", "use_unified_strength", flags_nozoom);

	kmi = WM_keymap_add_item(keymap, "WM_OT_radial_control", WKEY, KM_PRESS, 0, 0);
	set_brush_rc_props(kmi->ptr, paint, "weight", "use_unified_weight", flags_nozoom);

	if (flags & RC_ROTATION) {
		kmi = WM_keymap_add_item(keymap, "WM_OT_radial_control", FKEY, KM_PRESS, KM_CTRL, 0);
		set_brush_rc_props(kmi->ptr, paint, "texture_slot.angle", NULL, flags_nozoom);
	}
}

static void paint_partial_visibility_keys(wmKeyMap *keymap)
{
	wmKeyMapItem *kmi;
	
	/* Partial visibility */
	kmi = WM_keymap_add_item(keymap, "PAINT_OT_hide_show", HKEY, KM_PRESS, KM_SHIFT, 0);
	RNA_enum_set(kmi->ptr, "action", PARTIALVIS_SHOW);
	RNA_enum_set(kmi->ptr, "area", PARTIALVIS_INSIDE);
	kmi = WM_keymap_add_item(keymap, "PAINT_OT_hide_show", HKEY, KM_PRESS, 0, 0);
	RNA_enum_set(kmi->ptr, "action", PARTIALVIS_HIDE);
	RNA_enum_set(kmi->ptr, "area", PARTIALVIS_INSIDE);
	kmi = WM_keymap_add_item(keymap, "PAINT_OT_hide_show", HKEY, KM_PRESS, KM_ALT, 0);
	RNA_enum_set(kmi->ptr, "action", PARTIALVIS_SHOW);
	RNA_enum_set(kmi->ptr, "area", PARTIALVIS_ALL);
}

void ED_keymap_paint(wmKeyConfig *keyconf)
{
	wmKeyMap *keymap;
	wmKeyMapItem *kmi;
	int i;
	
	/* Sculpt mode */
	keymap = WM_keymap_find(keyconf, "Sculpt", 0, 0);
	keymap->poll = sculpt_mode_poll;

	RNA_enum_set(WM_keymap_add_item(keymap, "SCULPT_OT_brush_stroke", LEFTMOUSE, KM_PRESS, 0,        0)->ptr, "mode", BRUSH_STROKE_NORMAL);
	RNA_enum_set(WM_keymap_add_item(keymap, "SCULPT_OT_brush_stroke", LEFTMOUSE, KM_PRESS, KM_CTRL,  0)->ptr, "mode", BRUSH_STROKE_INVERT);
	RNA_enum_set(WM_keymap_add_item(keymap, "SCULPT_OT_brush_stroke", LEFTMOUSE, KM_PRESS, KM_SHIFT, 0)->ptr, "mode", BRUSH_STROKE_SMOOTH);

	/* Partial visibility, sculpt-only for now */
	paint_partial_visibility_keys(keymap);

	for (i = 0; i <= 5; i++)
		RNA_int_set(WM_keymap_add_item(keymap, "OBJECT_OT_subdivision_set", ZEROKEY + i, KM_PRESS, KM_CTRL, 0)->ptr, "level", i);

	/* Clear mask */
	kmi = WM_keymap_add_item(keymap, "PAINT_OT_mask_flood_fill", MKEY, KM_PRESS, KM_ALT, 0);
	RNA_enum_set(kmi->ptr, "mode", PAINT_MASK_FLOOD_VALUE);
	RNA_float_set(kmi->ptr, "value", 0);

	/* Invert mask */
	kmi = WM_keymap_add_item(keymap, "PAINT_OT_mask_flood_fill", IKEY, KM_PRESS, KM_CTRL, 0);
	RNA_enum_set(kmi->ptr, "mode", PAINT_MASK_INVERT);

	/* Toggle dynamic topology */
	WM_keymap_add_item(keymap, "SCULPT_OT_dynamic_topology_toggle", DKEY, KM_PRESS, KM_CTRL, 0);

	/* Dynamic-topology detail size
	 * 
	 * This should be improved further, perhaps by showing a triangle
	 * grid rather than brush alpha */
	kmi = WM_keymap_add_item(keymap, "WM_OT_radial_control", DKEY, KM_PRESS, KM_SHIFT, 0);
	set_brush_rc_props(kmi->ptr, "sculpt", "detail_size", NULL, 0);
	RNA_string_set(kmi->ptr, "data_path_primary", "tool_settings.sculpt.detail_size");

	/* multires switch */
	kmi = WM_keymap_add_item(keymap, "OBJECT_OT_subdivision_set", PAGEUPKEY, KM_PRESS, 0, 0);
	RNA_int_set(kmi->ptr, "level", 1);
	RNA_boolean_set(kmi->ptr, "relative", TRUE);

	kmi = WM_keymap_add_item(keymap, "OBJECT_OT_subdivision_set", PAGEDOWNKEY, KM_PRESS, 0, 0);
	RNA_int_set(kmi->ptr, "level", -1);
	RNA_boolean_set(kmi->ptr, "relative", TRUE);

	ed_keymap_paint_brush_switch(keymap, "sculpt");
	ed_keymap_paint_brush_size(keymap, "tool_settings.sculpt.brush.size");
	ed_keymap_paint_brush_radial_control(keymap, "sculpt", RC_ROTATION);

	keymap_brush_select(keymap, OB_MODE_SCULPT, SCULPT_TOOL_DRAW, DKEY, 0);
	keymap_brush_select(keymap, OB_MODE_SCULPT, SCULPT_TOOL_SMOOTH, SKEY, 0);
	keymap_brush_select(keymap, OB_MODE_SCULPT, SCULPT_TOOL_PINCH, PKEY, 0);
	keymap_brush_select(keymap, OB_MODE_SCULPT, SCULPT_TOOL_INFLATE, IKEY, 0);
	keymap_brush_select(keymap, OB_MODE_SCULPT, SCULPT_TOOL_GRAB, GKEY, 0);
	keymap_brush_select(keymap, OB_MODE_SCULPT, SCULPT_TOOL_LAYER, LKEY, 0);
	keymap_brush_select(keymap, OB_MODE_SCULPT, SCULPT_TOOL_FLATTEN, TKEY, KM_SHIFT);
	keymap_brush_select(keymap, OB_MODE_SCULPT, SCULPT_TOOL_CLAY, CKEY, 0);
	keymap_brush_select(keymap, OB_MODE_SCULPT, SCULPT_TOOL_CREASE, CKEY, KM_SHIFT);
	keymap_brush_select(keymap, OB_MODE_SCULPT, SCULPT_TOOL_SNAKE_HOOK, KKEY, 0);
	kmi = keymap_brush_select(keymap, OB_MODE_SCULPT, SCULPT_TOOL_MASK, MKEY, 0);
	RNA_boolean_set(kmi->ptr, "toggle", 1);
	RNA_boolean_set(kmi->ptr, "create_missing", 1);

	/* */
	kmi = WM_keymap_add_item(keymap, "WM_OT_context_menu_enum", AKEY, KM_PRESS, 0, 0);
	RNA_string_set(kmi->ptr, "data_path", "tool_settings.sculpt.brush.stroke_method");

	kmi = WM_keymap_add_item(keymap, "WM_OT_context_toggle", SKEY, KM_PRESS, KM_SHIFT, 0);
	RNA_string_set(kmi->ptr, "data_path", "tool_settings.sculpt.brush.use_smooth_stroke");

	kmi = WM_keymap_add_item(keymap, "WM_OT_context_menu_enum", RKEY, KM_PRESS, 0, 0);
	RNA_string_set(kmi->ptr, "data_path", "tool_settings.sculpt.brush.texture_angle_source_random");

	/* Vertex Paint mode */
	keymap = WM_keymap_find(keyconf, "Vertex Paint", 0, 0);
	keymap->poll = vertex_paint_mode_poll;

	WM_keymap_verify_item(keymap, "PAINT_OT_vertex_paint", LEFTMOUSE, KM_PRESS, 0, 0);
	WM_keymap_add_item(keymap, "PAINT_OT_sample_color", RIGHTMOUSE, KM_PRESS, 0, 0);

	WM_keymap_add_item(keymap,
	                   "PAINT_OT_vertex_color_set", KKEY, KM_PRESS, KM_SHIFT, 0);

	ed_keymap_paint_brush_switch(keymap, "vertex_paint");
	ed_keymap_paint_brush_size(keymap, "tool_settings.vertex_paint.brush.size");
	ed_keymap_paint_brush_radial_control(keymap, "vertex_paint", RC_COLOR | RC_ROTATION);

	kmi = WM_keymap_add_item(keymap, "WM_OT_context_toggle", MKEY, KM_PRESS, 0, 0); /* mask toggle */
	RNA_string_set(kmi->ptr, "data_path", "vertex_paint_object.data.use_paint_mask");

	kmi = WM_keymap_add_item(keymap, "WM_OT_context_toggle", SKEY, KM_PRESS, KM_SHIFT, 0);
	RNA_string_set(kmi->ptr, "data_path", "tool_settings.vertex_paint.brush.use_smooth_stroke");

	kmi = WM_keymap_add_item(keymap, "WM_OT_context_menu_enum", RKEY, KM_PRESS, 0, 0);
	RNA_string_set(kmi->ptr, "data_path", "tool_settings.vertex_paint.brush.texture_angle_source_random");

	/* Weight Paint mode */
	keymap = WM_keymap_find(keyconf, "Weight Paint", 0, 0);
	keymap->poll = weight_paint_mode_poll;

	WM_keymap_verify_item(keymap, "PAINT_OT_weight_paint", LEFTMOUSE, KM_PRESS, 0, 0);

	/* these keys are from 2.4x but could be changed */
	WM_keymap_verify_item(keymap, "PAINT_OT_weight_sample", ACTIONMOUSE, KM_PRESS, KM_CTRL, 0);
	WM_keymap_verify_item(keymap, "PAINT_OT_weight_sample_group", ACTIONMOUSE, KM_PRESS, KM_SHIFT, 0);

	RNA_enum_set(WM_keymap_add_item(keymap, "PAINT_OT_weight_gradient", LEFTMOUSE, KM_PRESS, KM_ALT, 0)->ptr,           "type", WPAINT_GRADIENT_TYPE_LINEAR);
	RNA_enum_set(WM_keymap_add_item(keymap, "PAINT_OT_weight_gradient", LEFTMOUSE, KM_PRESS, KM_ALT | KM_CTRL, 0)->ptr, "type", WPAINT_GRADIENT_TYPE_RADIAL);

	WM_keymap_add_item(keymap,
	                   "PAINT_OT_weight_set", KKEY, KM_PRESS, KM_SHIFT, 0);

	ed_keymap_paint_brush_switch(keymap, "weight_paint");
	ed_keymap_paint_brush_size(keymap, "tool_settings.weight_paint.brush.size");
	ed_keymap_paint_brush_radial_control(keymap, "weight_paint", 0);

	kmi = WM_keymap_add_item(keymap, "WM_OT_context_toggle", MKEY, KM_PRESS, 0, 0); /* face mask toggle */
	RNA_string_set(kmi->ptr, "data_path", "weight_paint_object.data.use_paint_mask");

	/* note, conflicts with vertex paint, but this is more useful */
	kmi = WM_keymap_add_item(keymap, "WM_OT_context_toggle", VKEY, KM_PRESS, 0, 0); /* vert mask toggle */
	RNA_string_set(kmi->ptr, "data_path", "weight_paint_object.data.use_paint_mask_vertex");

	WM_keymap_verify_item(keymap, "PAINT_OT_weight_from_bones", WKEY, KM_PRESS, 0, 0);

	kmi = WM_keymap_add_item(keymap, "WM_OT_context_toggle", SKEY, KM_PRESS, KM_SHIFT, 0);
	RNA_string_set(kmi->ptr, "data_path", "tool_settings.weight_paint.brush.use_smooth_stroke");

	/*Weight paint's Vertex Selection Mode */
	keymap = WM_keymap_find(keyconf, "Weight Paint Vertex Selection", 0, 0);
	keymap->poll = vert_paint_poll;
	WM_keymap_add_item(keymap, "PAINT_OT_vert_select_all", AKEY, KM_PRESS, 0, 0);
	kmi = WM_keymap_add_item(keymap, "PAINT_OT_vert_select_all", IKEY, KM_PRESS, KM_CTRL, 0);
	RNA_enum_set(kmi->ptr, "action", SEL_INVERT);
	WM_keymap_add_item(keymap, "VIEW3D_OT_select_border", BKEY, KM_PRESS, 0, 0);
	kmi = WM_keymap_add_item(keymap, "VIEW3D_OT_select_lasso", EVT_TWEAK_A, KM_ANY, KM_CTRL, 0);
	RNA_boolean_set(kmi->ptr, "deselect", FALSE);
	kmi = WM_keymap_add_item(keymap, "VIEW3D_OT_select_lasso", EVT_TWEAK_A, KM_ANY, KM_SHIFT | KM_CTRL, 0);
	RNA_boolean_set(kmi->ptr, "deselect", TRUE);
	WM_keymap_add_item(keymap, "VIEW3D_OT_select_circle", CKEY, KM_PRESS, 0, 0);

	/* Image/Texture Paint mode */
	keymap = WM_keymap_find(keyconf, "Image Paint", 0, 0);
	keymap->poll = image_texture_paint_poll;

	RNA_enum_set(WM_keymap_add_item(keymap, "PAINT_OT_image_paint", LEFTMOUSE, KM_PRESS, 0,        0)->ptr, "mode", BRUSH_STROKE_NORMAL);
	RNA_enum_set(WM_keymap_add_item(keymap, "PAINT_OT_image_paint", LEFTMOUSE, KM_PRESS, KM_CTRL,  0)->ptr, "mode", BRUSH_STROKE_INVERT);
	WM_keymap_add_item(keymap, "PAINT_OT_grab_clone", RIGHTMOUSE, KM_PRESS, 0, 0);
	WM_keymap_add_item(keymap, "PAINT_OT_sample_color", RIGHTMOUSE, KM_PRESS, 0, 0);

	ed_keymap_paint_brush_switch(keymap, "image_paint");
	ed_keymap_paint_brush_size(keymap, "tool_settings.image_paint.brush.size");
	ed_keymap_paint_brush_radial_control(keymap, "image_paint", RC_COLOR | RC_ZOOM | RC_ROTATION);

	kmi = WM_keymap_add_item(keymap, "WM_OT_context_toggle", MKEY, KM_PRESS, 0, 0); /* mask toggle */
	RNA_string_set(kmi->ptr, "data_path", "image_paint_object.data.use_paint_mask");

	kmi = WM_keymap_add_item(keymap, "WM_OT_context_toggle", SKEY, KM_PRESS, KM_SHIFT, 0);
	RNA_string_set(kmi->ptr, "data_path", "tool_settings.image_paint.brush.use_smooth_stroke");

	kmi = WM_keymap_add_item(keymap, "WM_OT_context_menu_enum", RKEY, KM_PRESS, 0, 0);
	RNA_string_set(kmi->ptr, "data_path", "tool_settings.image_paint.brush.texture_angle_source_random");

	/* face-mask mode */
	keymap = WM_keymap_find(keyconf, "Face Mask", 0, 0);
	keymap->poll = facemask_paint_poll;

	WM_keymap_add_item(keymap, "PAINT_OT_face_select_all", AKEY, KM_PRESS, 0, 0);
	kmi = WM_keymap_add_item(keymap, "PAINT_OT_face_select_all", IKEY, KM_PRESS, KM_CTRL, 0);
	RNA_enum_set(kmi->ptr, "action", SEL_INVERT);
	kmi = WM_keymap_add_item(keymap, "PAINT_OT_face_select_hide", HKEY, KM_PRESS, 0, 0);
	RNA_boolean_set(kmi->ptr, "unselected", FALSE);
	kmi = WM_keymap_add_item(keymap, "PAINT_OT_face_select_hide", HKEY, KM_PRESS, KM_SHIFT, 0);
	RNA_boolean_set(kmi->ptr, "unselected", TRUE);
	WM_keymap_add_item(keymap, "PAINT_OT_face_select_reveal", HKEY, KM_PRESS, KM_ALT, 0);

	WM_keymap_add_item(keymap, "PAINT_OT_face_select_linked", LKEY, KM_PRESS, KM_CTRL, 0);
	WM_keymap_add_item(keymap, "PAINT_OT_face_select_linked_pick", LKEY, KM_PRESS, 0, 0);

	keymap = WM_keymap_find(keyconf, "UV Sculpt", 0, 0);
	keymap->poll = uv_sculpt_poll;

	kmi = WM_keymap_add_item(keymap, "WM_OT_context_toggle", QKEY, KM_PRESS, 0, 0);
	RNA_string_set(kmi->ptr, "data_path", "tool_settings.use_uv_sculpt");

	RNA_enum_set(WM_keymap_add_item(keymap, "SCULPT_OT_uv_sculpt_stroke", LEFTMOUSE, KM_PRESS, 0,        0)->ptr, "mode", BRUSH_STROKE_NORMAL);
	RNA_enum_set(WM_keymap_add_item(keymap, "SCULPT_OT_uv_sculpt_stroke", LEFTMOUSE, KM_PRESS, KM_CTRL,  0)->ptr, "mode", BRUSH_STROKE_INVERT);
	RNA_enum_set(WM_keymap_add_item(keymap, "SCULPT_OT_uv_sculpt_stroke", LEFTMOUSE, KM_PRESS, KM_SHIFT, 0)->ptr, "mode", BRUSH_STROKE_SMOOTH);

	ed_keymap_paint_brush_size(keymap, "tool_settings.uv_sculpt.brush.size");
	ed_keymap_paint_brush_radial_control(keymap, "uv_sculpt", 0);

	RNA_enum_set(WM_keymap_add_item(keymap, "BRUSH_OT_uv_sculpt_tool_set", SKEY, KM_PRESS, 0, 0)->ptr, "tool", UV_SCULPT_TOOL_RELAX);
	RNA_enum_set(WM_keymap_add_item(keymap, "BRUSH_OT_uv_sculpt_tool_set", PKEY, KM_PRESS, 0, 0)->ptr, "tool", UV_SCULPT_TOOL_PINCH);
	RNA_enum_set(WM_keymap_add_item(keymap, "BRUSH_OT_uv_sculpt_tool_set", GKEY, KM_PRESS, 0, 0)->ptr, "tool", UV_SCULPT_TOOL_GRAB);

	/* paint stroke */
	keymap = paint_stroke_modal_keymap(keyconf);
	WM_modalkeymap_assign(keymap, "SCULPT_OT_brush_stroke");
}
