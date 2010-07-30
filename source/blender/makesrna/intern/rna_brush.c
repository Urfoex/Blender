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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Contributor(s): Blender Foundation (2008), Juho Veps�l�inen
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include <stdlib.h>

#include "RNA_define.h"

#include "rna_internal.h"

#include "DNA_brush_types.h"
#include "DNA_texture_types.h"
#include "DNA_scene_types.h"

#include "BLI_math.h"

#include "IMB_imbuf.h"


#include "WM_types.h"

static EnumPropertyItem prop_direction_items[]= {
	{0, "ADD", 0, "Add", "Add effect of brush"},
	{BRUSH_DIR_IN, "SUBTRACT", 0, "Subtract", "Subtract effect of brush"},
	{0, NULL, 0, NULL, NULL}};

EnumPropertyItem brush_sculpt_tool_items[] = {
	{SCULPT_TOOL_BLOB, "BLOB", ICON_BRUSH_BLOB, "Blob", ""},
	{SCULPT_TOOL_CLAY, "CLAY", ICON_BRUSH_CLAY, "Clay", ""},
	{SCULPT_TOOL_CREASE, "CREASE",ICON_BRUSH_CREASE, "Crease", ""},
	{SCULPT_TOOL_DRAW, "DRAW", ICON_BRUSH_SCULPT_DRAW, "Draw", ""},
	{SCULPT_TOOL_FILL, "FILL", ICON_BRUSH_FILL, "Fill", ""},
	{SCULPT_TOOL_FLATTEN, "FLATTEN", ICON_BRUSH_FLATTEN, "Flatten", ""},
	{SCULPT_TOOL_GRAB, "GRAB", ICON_BRUSH_GRAB, "Grab", ""},
	{SCULPT_TOOL_INFLATE, "INFLATE", ICON_BRUSH_INFLATE, "Inflate", ""},
	{SCULPT_TOOL_LAYER, "LAYER", ICON_BRUSH_LAYER, "Layer", ""},
	{SCULPT_TOOL_NUDGE, "NUDGE", ICON_BRUSH_NUDGE, "Nudge", ""},
	{SCULPT_TOOL_PINCH, "PINCH", ICON_BRUSH_PINCH, "Pinch", ""},
	{SCULPT_TOOL_ROTATE, "ROTATE", ICON_BRUSH_ROTATE, "Rotate", ""},
	{SCULPT_TOOL_SCRAPE, "SCRAPE", ICON_BRUSH_SCRAPE, "Scrape", ""},
	{SCULPT_TOOL_SMOOTH, "SMOOTH", ICON_BRUSH_SMOOTH, "Smooth", ""},
	{SCULPT_TOOL_SNAKE_HOOK, "SNAKE_HOOK", ICON_BRUSH_SNAKE_HOOK, "Snake Hook", ""},
	{SCULPT_TOOL_THUMB, "THUMB", ICON_BRUSH_THUMB, "Thumb", ""},
	{0, NULL, 0, NULL, NULL}};


EnumPropertyItem brush_vertexpaint_tool_items[] = {
	{0, "MIX", ICON_BRUSH_MIX, "Mix", "Use mix blending mode while painting"},
	{1, "ADD", ICON_BRUSH_ADD, "Add", "Use add blending mode while painting"},
	{2, "SUB", ICON_BRUSH_SUBTRACT, "Subtract", "Use subtract blending mode while painting"},
	{3, "MUL", ICON_BRUSH_MULTIPLY, "Multiply", "Use multiply blending mode while painting"},
	{4, "BLUR", ICON_BRUSH_BLUR, "Blur", "Blur the color with surrounding values"},
	{5, "LIGHTEN", ICON_BRUSH_LIGHTEN, "Lighten", "Use lighten blending mode while painting"},
	{6, "DARKEN", ICON_BRUSH_DARKEN, "Darken", "Use darken blending mode while painting"},
	{0, NULL, 0, NULL, NULL}};
	
EnumPropertyItem brush_imagepaint_tool_items[] = {
	{PAINT_TOOL_DRAW, "DRAW", ICON_BRUSH_TEXDRAW, "Draw", ""},
	{PAINT_TOOL_SOFTEN, "SOFTEN", ICON_BRUSH_SOFTEN, "Soften", ""},
	{PAINT_TOOL_SMEAR, "SMEAR", ICON_BRUSH_SMEAR, "Smear", ""},
	{PAINT_TOOL_CLONE, "CLONE", ICON_BRUSH_CLONE, "Clone", ""},
	{0, NULL, 0, NULL, NULL}};

#ifdef RNA_RUNTIME

#include "MEM_guardedalloc.h"

#include "DNA_object_types.h"

#include "RNA_access.h"

#include "BKE_texture.h"
#include "BKE_brush.h"
#include "BKE_icons.h"

#include "BKE_paint.h"

#include "WM_api.h"

static void rna_Brush_reset_icon(Brush *br, char *type)
{
	ID *id = &br->id;

	if(br->flag & BRUSH_CUSTOM_ICON)
		return;

	if(id->icon_id >= BIFICONID_LAST) {
		BKE_icon_delete(id);
		BKE_previewimg_free_id(id);
 	}

	id->icon_id = 0;
}

static void rna_Brush_update(Main *bmain, Scene *scene, PointerRNA *ptr)
{
	Brush *br= (Brush*)ptr->data;
	WM_main_add_notifier(NC_BRUSH|NA_EDITED, br);
	//WM_main_add_notifier(NC_SPACE|ND_SPACE_VIEW3D, NULL);
}

static void rna_Brush_sculpt_tool_update(Main *bmain, Scene *scene, PointerRNA *ptr)
{
	Brush *br= (Brush*)ptr->data;
	rna_Brush_reset_icon(br, "sculpt");
	rna_Brush_update(bmain, scene, ptr);
}
 
static void rna_Brush_vertexpaint_tool_update(Main *bmain, Scene *scene, PointerRNA *ptr)
{
	Brush *br= (Brush*)ptr->data;
	rna_Brush_reset_icon(br, "vertex_paint");
	rna_Brush_update(bmain, scene, ptr);
}
 
static void rna_Brush_imagepaint_tool_update(Main *bmain, Scene *scene, PointerRNA *ptr)
{
	Brush *br= (Brush*)ptr->data;
	rna_Brush_reset_icon(br, "texture_paint");
	rna_Brush_update(bmain, scene, ptr);
}

static int rna_Brush_is_sculpt_brush(Brush *br, bContext *C)
{
	Sculpt *sd = CTX_data_tool_settings(C)->sculpt;
	return paint_has_brush(&sd->paint, br);
}

static int rna_Brush_is_vpaint_brush(Brush *br, bContext *C)
{
	VPaint *vp = CTX_data_tool_settings(C)->vpaint;
	return paint_has_brush(&vp->paint, br);
}

static int rna_Brush_is_wpaint_brush(Brush *br, bContext *C)
{
	VPaint *vp = CTX_data_tool_settings(C)->wpaint;
	return paint_has_brush(&vp->paint, br);
}

static int rna_Brush_is_imapaint_brush(Brush *me, bContext *C)
{
	ImagePaintSettings *data = &(CTX_data_tool_settings(C)->imapaint);
	int i;

	for (i= 0; i < data->paint.brush_count; i++) {
		if (strcmp(me->id.name+2, data->paint.brushes[i]->id.name+2) == 0) 
			return 1;
	}

	return 0;
}

static void rna_Brush_icon_update(Main *bmain, Scene *scene, PointerRNA *ptr)
{
	Brush *br= (Brush*)ptr->data;

	if(br->icon_imbuf) {
		IMB_freeImBuf(br->icon_imbuf);
		br->icon_imbuf= NULL;
	}

	br->id.icon_id = 0;

	if(br->flag & BRUSH_CUSTOM_ICON) {
		BKE_previewimg_get(&br->id);
		BKE_icon_changed(BKE_icon_getid(&br->id));
	}

	WM_main_add_notifier(NC_BRUSH|NA_EDITED, br);
}

static void rna_Brush_set_size(PointerRNA *ptr, int value)
{
	Brush* me = (Brush*)(ptr->data);
	brush_set_size(me, value);
}

static int rna_Brush_get_size(PointerRNA *ptr)
{
	Brush* me = (Brush*)(ptr->data);
	return brush_size(me);
}

static void rna_Brush_set_use_locked_size(PointerRNA *ptr, int value)
{
	Brush* me = (Brush*)(ptr->data);
	brush_set_use_locked_size(me, value);
}

static int rna_Brush_get_use_locked_size(PointerRNA *ptr)
{
	Brush* me = (Brush*)(ptr->data);
	return brush_use_locked_size(me);
}

static void rna_Brush_set_use_size_pressure(PointerRNA *ptr, int value)
{
	Brush* me = (Brush*)(ptr->data);
	brush_set_use_size_pressure(me, value);
}

static int rna_Brush_get_use_size_pressure(PointerRNA *ptr)
{
	Brush* me = (Brush*)(ptr->data);
	return brush_use_size_pressure(me);
}

static void rna_Brush_set_use_alpha_pressure(PointerRNA *ptr, int value)
{
	Brush* me = (Brush*)(ptr->data);
	brush_set_use_alpha_pressure(me, value);
}

static int rna_Brush_get_use_alpha_pressure(PointerRNA *ptr)
{
	Brush* me = (Brush*)(ptr->data);
	return brush_use_alpha_pressure(me);
}

static void rna_Brush_set_unprojected_radius(PointerRNA *ptr, float value)
{
	Brush* me = (Brush*)(ptr->data);
	brush_set_unprojected_radius(me, value);
}

static float rna_Brush_get_unprojected_radius(PointerRNA *ptr)
{
	Brush* me = (Brush*)(ptr->data);
	return brush_unprojected_radius(me);
}

static void rna_Brush_set_alpha(PointerRNA *ptr, float value)
{
	Brush* me = (Brush*)(ptr->data);
	brush_set_alpha(me, value);
}

static float rna_Brush_get_alpha(PointerRNA *ptr)
{
	Brush* me = (Brush*)(ptr->data);
	return brush_alpha(me);
}

static EnumPropertyItem *rna_Brush_direction_itemf(bContext *C, PointerRNA *ptr, int *free)
{
	static EnumPropertyItem prop_default_items[]= {
		{0, NULL, 0, NULL, NULL}};

	static EnumPropertyItem prop_flatten_contrast_items[]= {
		{0, "FLATTEN", 0, "Flatten", "Add effect of brush"},
		{BRUSH_DIR_IN, "CONTRAST", 0, "Contrast", "Subtract effect of brush"},
		{0, NULL, 0, NULL, NULL}};

	static EnumPropertyItem prop_fill_deepen_items[]= {
		{0, "FILL", 0, "Fill", "Add effect of brush"},
		{BRUSH_DIR_IN, "DEEPEN", 0, "Deepen", "Subtract effect of brush"},
		{0, NULL, 0, NULL, NULL}};

	static EnumPropertyItem prop_scrape_peaks_items[]= {
		{0, "SCRAPE", 0, "Scrape", "Add effect of brush"},
		{BRUSH_DIR_IN, "PEAKS", 0, "Peaks", "Subtract effect of brush"},
		{0, NULL, 0, NULL, NULL}};

	static EnumPropertyItem prop_pinch_magnify_items[]= {
		{0, "PINCH", 0, "Pinch", "Add effect of brush"},
		{BRUSH_DIR_IN, "MAGNIFY", 0, "Magnify", "Subtract effect of brush"},
		{0, NULL, 0, NULL, NULL}};

	static EnumPropertyItem prop_inflate_deflate_items[]= {
		{0, "INFLATE", 0, "Inflate", "Add effect of brush"},
		{BRUSH_DIR_IN, "DEFLATE", 0, "Deflate", "Subtract effect of brush"},
		{0, NULL, 0, NULL, NULL}};

	Brush *me= (Brush*)(ptr->data);

	switch (me->sculpt_tool) {
		case SCULPT_TOOL_DRAW:
		case SCULPT_TOOL_CREASE:
		case SCULPT_TOOL_BLOB:
		case SCULPT_TOOL_LAYER:
		case SCULPT_TOOL_CLAY:
			return prop_direction_items;

		case SCULPT_TOOL_FLATTEN:
			return prop_flatten_contrast_items;

		case SCULPT_TOOL_FILL:
			return prop_fill_deepen_items;

		case SCULPT_TOOL_SCRAPE:
			return prop_scrape_peaks_items;

		case SCULPT_TOOL_PINCH:
			return prop_pinch_magnify_items;

		case SCULPT_TOOL_INFLATE:
			return prop_inflate_deflate_items;

		default:
			return prop_default_items;
	}
}

#else

static void rna_def_brush_texture_slot(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;

	static EnumPropertyItem prop_map_mode_items[] = {
		{MTEX_MAP_MODE_FIXED, "FIXED", 0, "Fixed", ""},
		{MTEX_MAP_MODE_TILED, "TILED", 0, "Tiled", ""},
		{MTEX_MAP_MODE_3D, "3D", 0, "3D", ""},
		{0, NULL, 0, NULL, NULL}};

	srna= RNA_def_struct(brna, "BrushTextureSlot", "TextureSlot");
	RNA_def_struct_sdna(srna, "MTex");
	RNA_def_struct_ui_text(srna, "Brush Texture Slot", "Texture slot for textures in a Brush datablock");

	prop= RNA_def_property(srna, "angle", PROP_FLOAT, PROP_ANGLE);
	RNA_def_property_float_sdna(prop, NULL, "rot");
	RNA_def_property_range(prop, 0, M_PI*2);
	RNA_def_property_ui_text(prop, "Angle", "Defines brush texture rotation");
	RNA_def_property_update(prop, 0, "rna_TextureSlot_update");

	prop= RNA_def_property(srna, "map_mode", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_sdna(prop, NULL, "brush_map_mode");
	RNA_def_property_enum_items(prop, prop_map_mode_items);
	RNA_def_property_ui_text(prop, "Mode", "");
	RNA_def_property_update(prop, 0, "rna_TextureSlot_update");
}

static void rna_def_brush(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;
	
	static EnumPropertyItem prop_blend_items[] = {
		{IMB_BLEND_MIX, "MIX", 0, "Mix", "Use mix blending mode while painting"},
		{IMB_BLEND_ADD, "ADD", 0, "Add", "Use add blending mode while painting"},
		{IMB_BLEND_SUB, "SUB", 0, "Subtract", "Use subtract blending mode while painting"},
		{IMB_BLEND_MUL, "MUL", 0, "Multiply", "Use multiply blending mode while painting"},
		{IMB_BLEND_LIGHTEN, "LIGHTEN", 0, "Lighten", "Use lighten blending mode while painting"},
		{IMB_BLEND_DARKEN, "DARKEN", 0, "Darken", "Use darken blending mode while painting"},
		{IMB_BLEND_ERASE_ALPHA, "ERASE_ALPHA", 0, "Erase Alpha", "Erase alpha while painting"},
		{IMB_BLEND_ADD_ALPHA, "ADD_ALPHA", 0, "Add Alpha", "Add alpha while painting"},
		{0, NULL, 0, NULL, NULL}};
	
	static EnumPropertyItem brush_stroke_method_items[] = {
		{0, "DOTS", 0, "Dots", ""},
		{BRUSH_RESTORE_MESH, "DRAG_DOT", 0, "Drag Dot", ""},
		{BRUSH_SPACE, "SPACE", 0, "Space", ""},
		{BRUSH_ANCHORED, "ANCHORED", 0, "Anchored", ""},
		{BRUSH_AIRBRUSH, "AIRBRUSH", 0, "Airbrush", ""},
		{0, NULL, 0, NULL, NULL}};

	static EnumPropertyItem texture_angle_source_items[] = {
		{0, "USER", 0, "User", ""},
		{BRUSH_RAKE, "RAKE", 0, "Rake", ""},
		{BRUSH_RANDOM_ROTATION, "RANDOM", 0, "Random", ""},
		{0, NULL, 0, NULL, NULL}};

	static EnumPropertyItem texture_angle_source_no_random_items[] = {
		{0, "USER", 0, "User", ""},
		{BRUSH_RAKE, "RAKE", 0, "Rake", ""},
		{0, NULL, 0, NULL, NULL}};

	static EnumPropertyItem brush_sculpt_plane_items[] = {
		{SCULPT_DISP_DIR_AREA, "AREA", 0, "Area Plane", ""},
		{SCULPT_DISP_DIR_VIEW, "VIEW", 0, "View Plane", ""},
		{SCULPT_DISP_DIR_X, "X", 0, "X Plane", ""},
		{SCULPT_DISP_DIR_Y, "Y", 0, "Y Plane", ""},
		{SCULPT_DISP_DIR_Z, "Z", 0, "Z Plane", ""},
		{0, NULL, 0, NULL, NULL}};

	FunctionRNA *func;
	PropertyRNA *parm;

	srna= RNA_def_struct(brna, "Brush", "ID");
	RNA_def_struct_ui_text(srna, "Brush", "Brush datablock for storing brush settings for painting and sculpting");
	RNA_def_struct_ui_icon(srna, ICON_BRUSH_DATA);

	/* functions */
	func= RNA_def_function(srna, "is_sculpt_brush", "rna_Brush_is_sculpt_brush");
	RNA_def_function_ui_description(func, "Returns true if Brush can be used for sculpting");
	parm= RNA_def_pointer(func, "context", "Context", "", "");
	RNA_def_property_flag(parm, PROP_REQUIRED);
	parm= RNA_def_boolean(func, "ret", 0, "", "");
	RNA_def_function_return(func, parm);

	func= RNA_def_function(srna, "is_vpaint_brush", "rna_Brush_is_vpaint_brush");
	RNA_def_function_ui_description(func, "Returns true if Brush can be used for vertex painting");
	parm= RNA_def_pointer(func, "context", "Context", "", "");
	RNA_def_property_flag(parm, PROP_REQUIRED);
	parm= RNA_def_boolean(func, "ret", 0, "", "");
	RNA_def_function_return(func, parm);

	func= RNA_def_function(srna, "is_wpaint_brush", "rna_Brush_is_wpaint_brush");
	RNA_def_function_ui_description(func, "Returns true if Brush can be used for weight painting");
	parm= RNA_def_pointer(func, "context", "Context", "", "");
	RNA_def_property_flag(parm, PROP_REQUIRED);
	parm= RNA_def_boolean(func, "ret", 0, "", "");
	RNA_def_function_return(func, parm);

	func= RNA_def_function(srna, "is_imapaint_brush", "rna_Brush_is_imapaint_brush");
	RNA_def_function_ui_description(func, "Returns true if Brush can be used for image painting");
	parm= RNA_def_pointer(func, "context", "Context", "", "");
	RNA_def_property_flag(parm, PROP_REQUIRED);
	parm= RNA_def_boolean(func, "ret", 0, "", "");
	RNA_def_function_return(func, parm);

	/* enums */
	prop= RNA_def_property(srna, "blend", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_items(prop, prop_blend_items);
	RNA_def_property_ui_text(prop, "Blending mode", "Brush blending mode");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "sculpt_tool", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_items(prop, brush_sculpt_tool_items);
	RNA_def_property_ui_text(prop, "Sculpt Tool", "");
	RNA_def_property_update(prop, 0, "rna_Brush_sculpt_tool_update");

	prop= RNA_def_property(srna, "vertexpaint_tool", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_items(prop, brush_vertexpaint_tool_items);
	RNA_def_property_ui_text(prop, "Vertex/Weight Paint Tool", "");
	RNA_def_property_update(prop, 0, "rna_Brush_vertexpaint_tool_update");
	
	prop= RNA_def_property(srna, "imagepaint_tool", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_items(prop, brush_imagepaint_tool_items);
	RNA_def_property_ui_text(prop, "Image Paint Tool", "");
	RNA_def_property_update(prop, NC_SPACE|ND_SPACE_IMAGE, "rna_Brush_imagepaint_tool_update");

	prop= RNA_def_property(srna, "direction", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_bitflag_sdna(prop, NULL, "flag");
	RNA_def_property_enum_items(prop, prop_direction_items);
	RNA_def_property_enum_funcs(prop, NULL, NULL, "rna_Brush_direction_itemf");
	RNA_def_property_ui_text(prop, "Direction", "");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "stroke_method", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_bitflag_sdna(prop, NULL, "flag");
	RNA_def_property_enum_items(prop, brush_stroke_method_items);
	RNA_def_property_ui_text(prop, "Stroke Method", "");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "texture_angle_source", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_bitflag_sdna(prop, NULL, "flag");
	RNA_def_property_enum_items(prop, texture_angle_source_items);
	RNA_def_property_ui_text(prop, "Texture Angle Source", "");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "texture_angle_source_no_random", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_bitflag_sdna(prop, NULL, "flag");
	RNA_def_property_enum_items(prop, texture_angle_source_no_random_items);
	RNA_def_property_ui_text(prop, "Texture Angle Source", "");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "sculpt_plane", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_items(prop, brush_sculpt_plane_items);
	RNA_def_property_ui_text(prop, "Sculpt Plane", "");
	RNA_def_property_update(prop, 0, "rna_Brush_update");
	
	/* number values */
	prop= RNA_def_property(srna, "size", PROP_INT, PROP_DISTANCE);
	RNA_def_property_int_funcs(prop, "rna_Brush_get_size", "rna_Brush_set_size", NULL);
	RNA_def_property_range(prop, 1, MAX_BRUSH_PIXEL_RADIUS*10);
	RNA_def_property_ui_range(prop, 1, MAX_BRUSH_PIXEL_RADIUS, 1, 0);
	RNA_def_property_ui_text(prop, "Size", "Radius of the brush in pixels");
	RNA_def_property_update(prop, 0, "rna_Brush_update");
	
	prop= RNA_def_property(srna, "unprojected_radius", PROP_FLOAT, PROP_DISTANCE);
	RNA_def_property_float_funcs(prop, "rna_Brush_get_unprojected_radius", "rna_Brush_set_unprojected_radius", NULL);
	RNA_def_property_range(prop, 0.001, FLT_MAX);
	RNA_def_property_ui_range(prop, 0.001, 1, 0, 0);
	RNA_def_property_ui_text(prop, "Unprojected Radius", "Radius of brush in Blender units");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "jitter", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "jitter");
	RNA_def_property_range(prop, 0.0f, 1.0f);
	RNA_def_property_ui_text(prop, "Jitter", "Jitter the position of the brush while painting");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "spacing", PROP_INT, PROP_PERCENTAGE);
	RNA_def_property_int_sdna(prop, NULL, "spacing");
	RNA_def_property_range(prop, 1, 1000);
	RNA_def_property_ui_range(prop, 1, 500, 5, 0);
	RNA_def_property_ui_text(prop, "Spacing", "Spacing between brush daubs as a percentage of brush diameter");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "smooth_stroke_radius", PROP_INT, PROP_DISTANCE);
	RNA_def_property_range(prop, 10, 200);
	RNA_def_property_ui_text(prop, "Smooth Stroke Radius", "Minimum distance from last point before stroke continues");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "smooth_stroke_factor", PROP_FLOAT, PROP_FACTOR);
	RNA_def_property_range(prop, 0.5, 0.99);
	RNA_def_property_ui_text(prop, "Smooth Stroke Factor", "Higher values give a smoother stroke");
	RNA_def_property_update(prop, 0, "rna_Brush_update");
	
	prop= RNA_def_property(srna, "rate", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "rate");
	RNA_def_property_range(prop, 0.0001f , 10000.0f);
	RNA_def_property_ui_range(prop, 0.01f, 1.0f, 1, 3);
	RNA_def_property_ui_text(prop, "Rate", "Interval between paints for Airbrush");
	RNA_def_property_update(prop, 0, "rna_Brush_update");
	
	prop= RNA_def_property(srna, "color", PROP_FLOAT, PROP_COLOR_GAMMA);
	RNA_def_property_range(prop, 0.0, 1.0);
	RNA_def_property_float_sdna(prop, NULL, "rgb");
	RNA_def_property_ui_text(prop, "Color", "");
	RNA_def_property_update(prop, 0, "rna_Brush_update");
	
	prop= RNA_def_property(srna, "strength", PROP_FLOAT, PROP_FACTOR);
	RNA_def_property_float_sdna(prop, NULL, "alpha");
	RNA_def_property_float_funcs(prop, "rna_Brush_get_alpha", "rna_Brush_set_alpha", NULL);
	RNA_def_property_float_default(prop, 0.5f);
	RNA_def_property_range(prop, 0.0f, 10.0f);
	RNA_def_property_ui_range(prop, 0.0f, 1.0f, 0.001, 0.001);
	RNA_def_property_ui_text(prop, "Strength", "How powerful the effect of the brush is when applied");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "plane_offset", PROP_FLOAT, PROP_DISTANCE);
	RNA_def_property_float_sdna(prop, NULL, "plane_offset");
	RNA_def_property_float_default(prop, 0);
	RNA_def_property_range(prop, -2.0f, 2.0f);
	RNA_def_property_ui_range(prop, -0.5f, 0.5f, 0.001, 0.001);
	RNA_def_property_ui_text(prop, "Plane Offset", "Adjusts plane on which the brush acts towards or away from the object surface");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "plane_trim", PROP_FLOAT, PROP_DISTANCE);
	RNA_def_property_float_sdna(prop, NULL, "plane_trim");
	RNA_def_property_float_default(prop, 0.5f);
	RNA_def_property_range(prop, 0, 1.0f);
	RNA_def_property_ui_text(prop, "Plane Trim", "If a vertex is further from offset plane than this then it is not affected");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "texture_sample_bias", PROP_FLOAT, PROP_DISTANCE);
	RNA_def_property_float_sdna(prop, NULL, "texture_sample_bias");
	RNA_def_property_float_default(prop, 0);
	RNA_def_property_range(prop, -1, 1);
	RNA_def_property_ui_text(prop, "Texture Sample Bias", "Value added to texture samples");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "normal_weight", PROP_FLOAT, PROP_FACTOR);
	RNA_def_property_float_sdna(prop, NULL, "normal_weight");
	RNA_def_property_float_default(prop, 0);
	RNA_def_property_range(prop, 0.0f, 1.0f);
	RNA_def_property_ui_text(prop, "Normal Weight", "How much grab will pull vertexes out of surface during a grab");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "crease_pinch_factor", PROP_FLOAT, PROP_FACTOR);
	RNA_def_property_float_sdna(prop, NULL, "crease_pinch_factor");
	RNA_def_property_float_default(prop, 2.0f/3.0f);
	RNA_def_property_range(prop, 0.0f, 1.0f);
	RNA_def_property_ui_text(prop, "Crease Brush Pinch Factor", "How much the crease brush pinches");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "autosmooth_factor", PROP_FLOAT, PROP_FACTOR);
	RNA_def_property_float_sdna(prop, NULL, "autosmooth_factor");
	RNA_def_property_float_default(prop, 0);
	RNA_def_property_range(prop, 0.0f, 1.0f);
	RNA_def_property_ui_range(prop, 0.0f, 1.0f, 0.001, 0.001);
	RNA_def_property_ui_text(prop, "Autosmooth", "Amount of smoothing to automatically apply to each stroke");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	/* flag */
	prop= RNA_def_property(srna, "use_airbrush", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", BRUSH_AIRBRUSH);
	RNA_def_property_ui_text(prop, "Airbrush", "Keep applying paint effect while holding mouse (spray)");
	RNA_def_property_update(prop, 0, "rna_Brush_update");
	
	prop= RNA_def_property(srna, "use_original_normal", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", BRUSH_ORIGINAL_NORMAL);
	RNA_def_property_ui_text(prop, "Original Normal", "When locked keep using normal of surface where stroke was initiated");
	RNA_def_property_update(prop, 0, "rna_Brush_update");
	
	prop= RNA_def_property(srna, "use_wrap", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", BRUSH_TORUS);
	RNA_def_property_ui_text(prop, "Wrap", "Enable torus wrapping while painting");
	RNA_def_property_update(prop, 0, "rna_Brush_update");
	
	prop= RNA_def_property(srna, "use_strength_pressure", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", BRUSH_ALPHA_PRESSURE);
	RNA_def_property_boolean_funcs(prop, "rna_Brush_get_use_alpha_pressure", "rna_Brush_set_use_alpha_pressure");
	RNA_def_property_ui_icon(prop, ICON_STYLUS_PRESSURE, 0);
	RNA_def_property_ui_text(prop, "Strength Pressure", "Enable tablet pressure sensitivity for strength");
	RNA_def_property_update(prop, 0, "rna_Brush_update");
	
	prop= RNA_def_property(srna, "use_offset_pressure", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", BRUSH_OFFSET_PRESSURE);
	RNA_def_property_ui_icon(prop, ICON_STYLUS_PRESSURE, 0);
	RNA_def_property_ui_text(prop, "Plane Offset Pressure", "Enable tablet pressure sensitivity for offset");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "use_size_pressure", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", BRUSH_SIZE_PRESSURE);
	RNA_def_property_boolean_funcs(prop, "rna_Brush_get_use_size_pressure", "rna_Brush_set_use_size_pressure");
	RNA_def_property_ui_icon(prop, ICON_STYLUS_PRESSURE, 0);
	RNA_def_property_ui_text(prop, "Size Pressure", "Enable tablet pressure sensitivity for size");
	RNA_def_property_update(prop, 0, "rna_Brush_update");
	
	prop= RNA_def_property(srna, "use_jitter_pressure", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", BRUSH_JITTER_PRESSURE);
	RNA_def_property_ui_icon(prop, ICON_STYLUS_PRESSURE, 0);
	RNA_def_property_ui_text(prop, "Jitter Pressure", "Enable tablet pressure sensitivity for jitter");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "use_spacing_pressure", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", BRUSH_SPACING_PRESSURE);
	RNA_def_property_ui_icon(prop, ICON_STYLUS_PRESSURE, 0);
	RNA_def_property_ui_text(prop, "Spacing Pressure", "Enable tablet pressure sensitivity for spacing");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "use_inverse_smooth_pressure", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", BRUSH_INVERSE_SMOOTH_PRESSURE);
	RNA_def_property_ui_icon(prop, ICON_STYLUS_PRESSURE, 0);
	RNA_def_property_ui_text(prop, "Inverse Smooth Pressure", "Lighter pressure causes more smoothing to be applied");
	RNA_def_property_update(prop, 0, "rna_Brush_update");
	
	prop= RNA_def_property(srna, "use_rake", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", BRUSH_RAKE);
	RNA_def_property_ui_text(prop, "Rake", "Rotate the brush texture to match the stroke direction");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "use_random_rotation", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", BRUSH_RANDOM_ROTATION);
	RNA_def_property_ui_text(prop, "Random Rotation", "Rotate the brush texture at random");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "use_plane_trim", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", BRUSH_PLANE_TRIM);
	RNA_def_property_ui_text(prop, "Use Plane Trim", "Enable Plane Trim");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "use_frontface", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", BRUSH_FRONTFACE);
	RNA_def_property_ui_text(prop, "Use Front-Face", "Brush only affects vertexes that face the viewer");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "use_anchor", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", BRUSH_ANCHORED);
	RNA_def_property_ui_text(prop, "Anchored", "Keep the brush anchored to the initial location");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "use_space", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", BRUSH_SPACE);
	RNA_def_property_ui_text(prop, "Space", "Limit brush application to the distance specified by spacing");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "use_smooth_stroke", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", BRUSH_SMOOTH_STROKE);
	RNA_def_property_ui_text(prop, "Smooth Stroke", "Brush lags behind mouse and follows a smoother path");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "use_persistent", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", BRUSH_PERSISTENT);
	RNA_def_property_ui_text(prop, "Persistent", "Sculpts on a persistent layer of the mesh");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "use_accumulate", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", BRUSH_ACCUMULATE);
	RNA_def_property_ui_text(prop, "Accumulate", "Accumulate stroke dabs on top of each other");
	RNA_def_property_update(prop, 0, "rna_Brush_update");
	
	prop= RNA_def_property(srna, "use_space_atten", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", BRUSH_SPACE_ATTEN);
	RNA_def_property_ui_text(prop, "Use Automatic Strength Adjustment", "Automatically adjusts strength to give consistent results for different spacings");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	/* adaptive space is not implemented yet */
	prop= RNA_def_property(srna, "use_adaptive_space", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", BRUSH_ADAPTIVE_SPACE);
	RNA_def_property_ui_text(prop, "Adaptive Spacing", "Space daubs according to surface orientation instead of screen space");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "use_locked_size", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_funcs(prop, "rna_Brush_get_use_locked_size", "rna_Brush_set_use_locked_size");
	RNA_def_property_boolean_sdna(prop, NULL, "flag", BRUSH_LOCK_SIZE);
	RNA_def_property_ui_text(prop, "Use Blender Units", "When locked brush stays same size relative to object; when unlocked brush size is given in pixels");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "use_texture_overlay", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", BRUSH_TEXTURE_OVERLAY);
	RNA_def_property_ui_text(prop, "Use Texture Overlay", "Show texture in viewport");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "edge_to_edge", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", BRUSH_EDGE_TO_EDGE);
	RNA_def_property_ui_text(prop, "Edge-to-edge", "Drag anchor brush from edge-to-edge");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "restore_mesh", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", BRUSH_RESTORE_MESH);
	RNA_def_property_ui_text(prop, "Restore Mesh", "Allows a single dot to be carefully positioned");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	/* not exposed in the interface yet
	prop= RNA_def_property(srna, "fixed_tex", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", BRUSH_FIXED_TEX);
	RNA_def_property_ui_text(prop, "Fixed Texture", "Keep texture origin in fixed position");
	RNA_def_property_update(prop, 0, "rna_Brush_update"); */
	
	/* only for projection paint, TODO, other paint modes */
	prop= RNA_def_property(srna, "use_alpha", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_negative_sdna(prop, NULL, "flag", BRUSH_LOCK_ALPHA);
	RNA_def_property_ui_text(prop, "Alpha", "When this is disabled, lock alpha while painting");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "curve", PROP_POINTER, PROP_NONE);
	RNA_def_property_flag(prop, PROP_NEVER_NULL);
	RNA_def_property_ui_text(prop, "Curve", "Editable falloff curve");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	/* texture */
	prop= RNA_def_property(srna, "texture_slot", PROP_POINTER, PROP_NONE);
	RNA_def_property_struct_type(prop, "BrushTextureSlot");
	RNA_def_property_pointer_sdna(prop, NULL, "mtex");
	RNA_def_property_clear_flag(prop, PROP_EDITABLE);
	RNA_def_property_ui_text(prop, "Texture Slot", "");
	
	prop= RNA_def_property(srna, "texture", PROP_POINTER, PROP_NONE);
	RNA_def_property_pointer_sdna(prop, NULL, "mtex.tex");
	RNA_def_property_flag(prop, PROP_EDITABLE);
	RNA_def_property_ui_text(prop, "Texture", "");
	RNA_def_property_update(prop, NC_TEXTURE, "rna_Brush_update");

	prop= RNA_def_property(srna, "texture_overlay_alpha", PROP_INT, PROP_PERCENTAGE);
	RNA_def_property_int_sdna(prop, NULL, "texture_overlay_alpha");
	RNA_def_property_range(prop, 1, 100);
	RNA_def_property_ui_text(prop, "Texture Overlay Alpha", "");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "add_col", PROP_FLOAT, PROP_COLOR);
	RNA_def_property_float_sdna(prop, NULL, "add_col");
	RNA_def_property_array(prop, 3);
	RNA_def_property_ui_text(prop, "Add Color", "Color of cursor when adding");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "sub_col", PROP_FLOAT, PROP_COLOR);
	RNA_def_property_float_sdna(prop, NULL, "sub_col");
	RNA_def_property_array(prop, 3);
	RNA_def_property_ui_text(prop, "Subract Color", "Color of cursor when subtracting");
	RNA_def_property_update(prop, 0, "rna_Brush_update");

	prop= RNA_def_property(srna, "use_custom_icon", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", BRUSH_CUSTOM_ICON);
	RNA_def_property_ui_text(prop, "Custom Icon", "Set the brush icon from an image file");
	RNA_def_property_update(prop, 0, "rna_Brush_icon_update");

	prop= RNA_def_property(srna, "icon_filepath", PROP_STRING, PROP_FILEPATH);
	RNA_def_property_string_sdna(prop, NULL, "icon_filepath");
	RNA_def_property_ui_text(prop, "Brush Icon Filepath", "File path to brush icon");
	RNA_def_property_update(prop, 0, "rna_Brush_icon_update");

	/* clone tool */
	prop= RNA_def_property(srna, "clone_image", PROP_POINTER, PROP_NONE);
	RNA_def_property_pointer_sdna(prop, NULL, "clone.image");
	RNA_def_property_flag(prop, PROP_EDITABLE);
	RNA_def_property_ui_text(prop, "Clone Image", "Image for clone tool");
	RNA_def_property_update(prop, NC_SPACE|ND_SPACE_IMAGE, "rna_Brush_update");
	
	prop= RNA_def_property(srna, "clone_alpha", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "clone.alpha");
	RNA_def_property_range(prop, 0.0f, 1.0f);
	RNA_def_property_ui_text(prop, "Clone Alpha", "Opacity of clone image display");
	RNA_def_property_update(prop, NC_SPACE|ND_SPACE_IMAGE, "rna_Brush_update");

	prop= RNA_def_property(srna, "clone_offset", PROP_FLOAT, PROP_XYZ);
	RNA_def_property_float_sdna(prop, NULL, "clone.offset");
	RNA_def_property_ui_text(prop, "Clone Offset", "");
	RNA_def_property_ui_range(prop, -1.0f , 1.0f, 10.0f, 3);
	RNA_def_property_update(prop, NC_SPACE|ND_SPACE_IMAGE, "rna_Brush_update");
}


/* A brush stroke is a list of changes to the brush that
 * can occur during a stroke
 *
 *  o 3D location of the brush
 *  o 2D mouse location
 *  o Tablet pressure
 *  o Direction flip
 *  o Tool switch
 *  o Time
 */
static void rna_def_operator_stroke_element(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;

	srna= RNA_def_struct(brna, "OperatorStrokeElement", "IDPropertyGroup");
	RNA_def_struct_ui_text(srna, "Operator Stroke Element", "");

	prop= RNA_def_property(srna, "location", PROP_FLOAT, PROP_XYZ);
	RNA_def_property_flag(prop, PROP_IDPROPERTY);
	RNA_def_property_array(prop, 3);
	RNA_def_property_ui_text(prop, "Location", "");

	prop= RNA_def_property(srna, "mouse", PROP_FLOAT, PROP_XYZ);
	RNA_def_property_flag(prop, PROP_IDPROPERTY);
	RNA_def_property_array(prop, 2);
	RNA_def_property_ui_text(prop, "Mouse", "");

	prop= RNA_def_property(srna, "pressure", PROP_FLOAT, PROP_NONE);
	RNA_def_property_flag(prop, PROP_IDPROPERTY);
	RNA_def_property_range(prop, 0.0f, 1.0f);
	RNA_def_property_ui_text(prop, "Pressure", "Tablet pressure");

	prop= RNA_def_property(srna, "pen_flip", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_flag(prop, PROP_IDPROPERTY);
	RNA_def_property_ui_text(prop, "Flip", "");

	// used in uv painting
	prop= RNA_def_property(srna, "time", PROP_FLOAT, PROP_UNSIGNED);
	RNA_def_property_flag(prop, PROP_IDPROPERTY);
	RNA_def_property_ui_text(prop, "Time", "");

	/* XXX: Tool (this will be for pressing a modifier key for a different brush,
			e.g. switching to a Smooth brush in the middle of the stroke */

	// XXX: i don't think blender currently supports the ability to properly do a remappable modifier in the middle of a stroke
}

void RNA_def_brush(BlenderRNA *brna)
{
	rna_def_brush(brna);
	rna_def_brush_texture_slot(brna);
	rna_def_operator_stroke_element(brna);
}

#endif
