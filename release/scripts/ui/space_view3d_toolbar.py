# ##### BEGIN GPL LICENSE BLOCK #####
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# ##### END GPL LICENSE BLOCK #####

# <pep8 compliant>
import bpy


class View3DPanel(bpy.types.Panel):
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'TOOLS'


# ********** default tools for objectmode ****************


class VIEW3D_PT_tools_objectmode(View3DPanel):
    bl_context = "objectmode"
    bl_label = "Object Tools"

    def draw(self, context):
        layout = self.layout

        col = layout.column(align=True)
        col.itemL(text="Transform:")
        col.itemO("tfm.translate")
        col.itemO("tfm.rotate")
        col.itemO("tfm.resize", text="Scale")

        col = layout.column(align=True)
        col.itemL(text="Object:")
        col.itemO("object.duplicate_move")
        col.itemO("object.delete")
        col.itemO("object.join")

        active_object = context.active_object
        if active_object and active_object.type == 'MESH':

            col = layout.column(align=True)
            col.itemL(text="Shading:")
            col.itemO("object.shade_smooth", text="Smooth")
            col.itemO("object.shade_flat", text="Flat")

        col = layout.column(align=True)
        col.itemL(text="Keyframes:")
        col.itemO("anim.insert_keyframe_menu", text="Insert")
        col.itemO("anim.delete_keyframe_v3d", text="Remove")

        col = layout.column(align=True)
        col.itemL(text="Repeat:")
        col.itemO("screen.repeat_last")
        col.itemO("screen.repeat_history", text="History...")

        col = layout.column(align=True)
        col.itemL(text="Grease Pencil:")
        row = col.row()
        row.item_enumO("gpencil.draw", "mode", 'DRAW', text="Draw")
        row.item_enumO("gpencil.draw", "mode", 'DRAW_STRAIGHT', text="Line")
        row.item_enumO("gpencil.draw", "mode", 'ERASER', text="Erase")

        
        

# ********** default tools for editmode_mesh ****************


class VIEW3D_PT_tools_meshedit(View3DPanel):
    bl_context = "mesh_edit"
    bl_label = "Mesh Tools"

    def draw(self, context):
        layout = self.layout

        col = layout.column(align=True)
        col.itemL(text="Transform:")
        col.itemO("tfm.translate")
        col.itemO("tfm.rotate")
        col.itemO("tfm.resize", text="Scale")
        col.itemO("tfm.shrink_fatten", text="Along Normal")
        

        col = layout.column(align=True)
        col.itemL(text="Deform:")
        col.itemO("tfm.edge_slide")
        col.itemO("mesh.rip_move")
        col.itemO("mesh.vertices_smooth")
        

        col = layout.column(align=True)
        col.itemL(text="Add:")
        col.itemO("mesh.extrude_move")
        col.itemO("mesh.subdivide")
        col.itemO("mesh.loopcut")
        col.itemO("mesh.duplicate_move")
        col.itemO("mesh.spin")
        col.itemO("mesh.screw")
        
        col = layout.column(align=True)
        col.itemL(text="Remove:")
        col.itemO("mesh.delete")
        col.itemO("mesh.merge")
        col.itemO("mesh.remove_doubles")
        
        col = layout.column(align=True)
        col.itemL(text="Normals:")
        col.itemO("mesh.normals_make_consistent", text="Recalculate")
        col.itemO("mesh.flip_normals", text="Flip Direction")

        col = layout.column(align=True)
        col.itemL(text="UV Mapping:")
        col.item_stringO("wm.call_menu", "name", "VIEW3D_MT_uv_map", text="Unwrap")
        col.itemO("mesh.mark_seam")
        col.item_booleanO("mesh.mark_seam", "clear", True, text="Clear Seam")
        
        
        col = layout.column(align=True)
        col.itemL(text="Shading:")
        col.itemO("mesh.faces_shade_smooth", text="Smooth")
        col.itemO("mesh.faces_shade_flat", text="Flat")
        
        col = layout.column(align=True)
        col.itemL(text="Repeat:")
        col.itemO("screen.repeat_last")
        col.itemO("screen.repeat_history", text="History...")

        col = layout.column(align=True)
        col.itemL(text="Grease Pencil:")
        row = col.row()
        row.item_enumO("gpencil.draw", "mode", 'DRAW', text="Draw")
        row.item_enumO("gpencil.draw", "mode", 'DRAW_STRAIGHT', text="Line")
        row.item_enumO("gpencil.draw", "mode", 'ERASER', text="Erase")


class VIEW3D_PT_tools_meshedit_options(View3DPanel):
    bl_context = "mesh_edit"
    bl_label = "Mesh Options"

    def draw(self, context):
        layout = self.layout

        ob = context.active_object

        if ob:
            mesh = context.active_object.data
            col = layout.column(align=True)
            col.itemR(mesh, "use_mirror_x")

# ********** default tools for editmode_curve ****************


class VIEW3D_PT_tools_curveedit(View3DPanel):
    bl_context = "curve_edit"
    bl_label = "Curve Tools"

    def draw(self, context):
        layout = self.layout

        col = layout.column(align=True)
        col.itemL(text="Transform:")
        col.itemO("tfm.translate")
        col.itemO("tfm.rotate")
        col.itemO("tfm.resize", text="Scale")

        col = layout.column(align=True)
        col.itemL(text="Curve:")
        col.itemO("curve.duplicate")
        col.itemO("curve.delete")
        col.itemO("curve.cyclic_toggle")
        col.itemO("curve.switch_direction")
        col.itemO("curve.spline_type_set")

        col = layout.column(align=True)
        col.itemL(text="Handles:")
        col.item_enumO("curve.handle_type_set", "type", 'AUTOMATIC')
        col.item_enumO("curve.handle_type_set", "type", 'VECTOR')
        col.item_enumO("curve.handle_type_set", "type", 'ALIGN')
        col.item_enumO("curve.handle_type_set", "type", 'FREE_ALIGN')

        col = layout.column(align=True)
        col.itemL(text="Modeling:")
        col.itemO("curve.extrude")
        col.itemO("curve.subdivide")
        
        col = layout.column(align=True)
        col.itemL(text="Repeat:")
        col.itemO("screen.repeat_last")
        col.itemO("screen.repeat_history", text="History...")

        col = layout.column(align=True)
        col.itemL(text="Grease Pencil:")
        row = col.row()
        row.item_enumO("gpencil.draw", "mode", 'DRAW', text="Draw")
        row.item_enumO("gpencil.draw", "mode", 'DRAW_STRAIGHT', text="Line")
        row.item_enumO("gpencil.draw", "mode", 'ERASER', text="Erase")

        
        

# ********** default tools for editmode_surface ****************


class VIEW3D_PT_tools_surfaceedit(View3DPanel):
    bl_context = "surface_edit"
    bl_label = "Surface Tools"

    def draw(self, context):
        layout = self.layout

        col = layout.column(align=True)
        col.itemL(text="Transform:")
        col.itemO("tfm.translate")
        col.itemO("tfm.rotate")
        col.itemO("tfm.resize", text="Scale")

        col = layout.column(align=True)
        col.itemL(text="Curve:")
        col.itemO("curve.duplicate")
        col.itemO("curve.delete")
        col.itemO("curve.cyclic_toggle")
        col.itemO("curve.switch_direction")

        col = layout.column(align=True)
        col.itemL(text="Modeling:")
        col.itemO("curve.extrude")
        col.itemO("curve.subdivide")
        
        col = layout.column(align=True)
        col.itemL(text="Repeat:")
        col.itemO("screen.repeat_last")
        col.itemO("screen.repeat_history", text="History...")

        col = layout.column(align=True)
        col.itemL(text="Grease Pencil:")
        row = col.row()
        row.item_enumO("gpencil.draw", "mode", 'DRAW', text="Draw")
        row.item_enumO("gpencil.draw", "mode", 'DRAW_STRAIGHT', text="Line")
        row.item_enumO("gpencil.draw", "mode", 'ERASER', text="Erase")

        
        

# ********** default tools for editmode_text ****************


class VIEW3D_PT_tools_textedit(View3DPanel):
    bl_context = "text_edit"
    bl_label = "Text Tools"

    def draw(self, context):
        layout = self.layout

        col = layout.column(align=True)
        col.itemL(text="Text Edit:")
        col.itemO("font.text_copy", text="Copy")
        col.itemO("font.text_cut", text="Cut")
        col.itemO("font.text_paste", text="Paste")
        
        col = layout.column(align=True)
        col.itemL(text="Set Case:")
        col.item_enumO("font.case_set", "case", 'UPPER', text="To Upper")
        col.item_enumO("font.case_set", "case", 'LOWER', text="To Lower")
        
        col = layout.column(align=True)
        col.itemL(text="Style:")
        col.item_enumO("font.style_toggle", "style", 'BOLD')
        col.item_enumO("font.style_toggle", "style", 'ITALIC')
        col.item_enumO("font.style_toggle", "style", 'UNDERLINE')
        
        col = layout.column(align=True)
        col.itemL(text="Repeat:")
        col.itemO("screen.repeat_last")
        col.itemO("screen.repeat_history", text="History...")


# ********** default tools for editmode_armature ****************


class VIEW3D_PT_tools_armatureedit(View3DPanel):
    bl_context = "armature_edit"
    bl_label = "Armature Tools"

    def draw(self, context):
        layout = self.layout

        col = layout.column(align=True)
        col.itemL(text="Transform:")
        col.itemO("tfm.translate")
        col.itemO("tfm.rotate")
        col.itemO("tfm.resize", text="Scale")

        col = layout.column(align=True)
        col.itemL(text="Bones:")
        col.itemO("armature.bone_primitive_add", text="Add")
        col.itemO("armature.duplicate_move", text="Duplicate")
        col.itemO("armature.delete", text="Delete")

        col = layout.column(align=True)
        col.itemL(text="Modeling:")
        col.itemO("armature.extrude_move")
        col.itemO("armature.subdivide_multi", text="Subdivide")

        col = layout.column(align=True)
        col.itemL(text="Repeat:")
        col.itemO("screen.repeat_last")
        col.itemO("screen.repeat_history", text="History...")

        col = layout.column(align=True)
        col.itemL(text="Grease Pencil:")
        row = col.row()
        row.item_enumO("gpencil.draw", "mode", 'DRAW', text="Draw")
        row.item_enumO("gpencil.draw", "mode", 'DRAW_STRAIGHT', text="Line")
        row.item_enumO("gpencil.draw", "mode", 'ERASER', text="Erase")

        
        


class VIEW3D_PT_tools_armatureedit_options(View3DPanel):
    bl_context = "armature_edit"
    bl_label = "Armature Options"

    def draw(self, context):
        layout = self.layout

        arm = context.active_object.data

        col = layout.column(align=True)
        col.itemR(arm, "x_axis_mirror")

# ********** default tools for editmode_mball ****************


class VIEW3D_PT_tools_mballedit(View3DPanel):
    bl_context = "mball_edit"
    bl_label = "Meta Tools"

    def draw(self, context):
        layout = self.layout

        col = layout.column(align=True)
        col.itemL(text="Transform:")
        col.itemO("tfm.translate")
        col.itemO("tfm.rotate")
        col.itemO("tfm.resize", text="Scale")
        
        col = layout.column(align=True)
        col.itemL(text="Repeat:")
        col.itemO("screen.repeat_last")
        col.itemO("screen.repeat_history", text="History...")

        col = layout.column(align=True)
        col.itemL(text="Grease Pencil:")
        row = col.row()
        row.item_enumO("gpencil.draw", "mode", 'DRAW', text="Draw")
        row.item_enumO("gpencil.draw", "mode", 'DRAW_STRAIGHT', text="Line")
        row.item_enumO("gpencil.draw", "mode", 'ERASER', text="Erase")

        
        

# ********** default tools for editmode_lattice ****************


class VIEW3D_PT_tools_latticeedit(View3DPanel):
    bl_context = "lattice_edit"
    bl_label = "Lattice Tools"

    def draw(self, context):
        layout = self.layout

        col = layout.column(align=True)
        col.itemL(text="Transform:")
        col.itemO("tfm.translate")
        col.itemO("tfm.rotate")
        col.itemO("tfm.resize", text="Scale")
        
        col = layout.column(align=True)
        col.itemO("lattice.make_regular")
        
        col = layout.column(align=True)
        col.itemL(text="Repeat:")
        col.itemO("screen.repeat_last")
        col.itemO("screen.repeat_history", text="History...")
        
        col = layout.column(align=True)
        col.itemL(text="Grease Pencil:")
        row = col.row()
        row.item_enumO("gpencil.draw", "mode", 'DRAW', text="Draw")
        row.item_enumO("gpencil.draw", "mode", 'DRAW_STRAIGHT', text="Line")
        row.item_enumO("gpencil.draw", "mode", 'ERASER', text="Erase")


# ********** default tools for posemode ****************


class VIEW3D_PT_tools_posemode(View3DPanel):
    bl_context = "posemode"
    bl_label = "Pose Tools"

    def draw(self, context):
        layout = self.layout

        col = layout.column(align=True)
        col.itemL(text="Transform:")
        col.itemO("tfm.translate")
        col.itemO("tfm.rotate")
        col.itemO("tfm.resize", text="Scale")
        
        col = layout.column(align=True)
        col.itemL(text="In-Between:")
        row = col.row()
        row.itemO("pose.push", text="Push")
        row.itemO("pose.relax", text="Relax")
        col.itemO("pose.breakdown", text="Breakdowner")
        
        col = layout.column(align=True)
        col.itemL(text="Pose:")
        row = col.row()
        row.itemO("pose.copy", text="Copy")
        row.itemO("pose.paste", text="Paste")
        
        col = layout.column(align=True)
        col.itemO("poselib.pose_add", text="Add To Library")

        col = layout.column(align=True)
        col.itemL(text="Keyframes:")
        
        col.itemO("anim.insert_keyframe_menu", text="Insert")
        col.itemO("anim.delete_keyframe_v3d", text="Remove")

        col = layout.column(align=True)
        col.itemL(text="Repeat:")
        col.itemO("screen.repeat_last")
        col.itemO("screen.repeat_history", text="History...")
        
        col = layout.column(align=True)
        col.itemL(text="Grease Pencil:")
        row = col.row()
        row.item_enumO("gpencil.draw", "mode", 'DRAW', text="Draw")
        row.item_enumO("gpencil.draw", "mode", 'DRAW_STRAIGHT', text="Line")
        row.item_enumO("gpencil.draw", "mode", 'ERASER', text="Erase")

class VIEW3D_PT_tools_posemode_options(View3DPanel):
    bl_context = "posemode"
    bl_label = "Pose Options"

    def draw(self, context):
        layout = self.layout

        arm = context.active_object.data

        col = layout.column(align=True)
        col.itemR(arm, "x_axis_mirror")
        col.itemR(arm, "auto_ik")

# ********** default tools for paint modes ****************


class PaintPanel(bpy.types.Panel):
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'TOOLS'

    def paint_settings(self, context):
        ts = context.tool_settings

        if context.sculpt_object:
            return ts.sculpt
        elif context.vertex_paint_object:
            return ts.vertex_paint
        elif context.weight_paint_object:
            return ts.weight_paint
        elif context.texture_paint_object:
            return ts.image_paint
        elif context.particle_edit_object:
            return ts.particle_edit

        return False


class VIEW3D_PT_tools_brush(PaintPanel):
    bl_label = "Brush"

    def poll(self, context):
        return self.paint_settings(context)

    def draw(self, context):
        layout = self.layout

        settings = self.paint_settings(context)
        brush = settings.brush

        if not context.particle_edit_object:
            col = layout.split().column()
            row = col.row()
            row.template_list(settings, "brushes", settings, "active_brush_index", rows=2)

            col.template_ID(settings, "brush", new="brush.add")

        # Particle Mode #

        # XXX This needs a check if psys is editable.
        if context.particle_edit_object:
            # XXX Select Particle System
            layout.column().itemR(settings, "tool", expand=True)

            if settings.tool != 'NONE':
                col = layout.column()
                col.itemR(brush, "size", slider=True)
                col.itemR(brush, "strength", slider=True)

            if settings.tool == 'ADD':
                col = layout.column()
                col.itemR(settings, "add_interpolate")
                sub = col.column(align=True)
                sub.active = settings.add_interpolate
                sub.itemR(brush, "steps", slider=True)
                sub.itemR(settings, "add_keys", slider=True)
            elif settings.tool == 'LENGTH':
                layout.itemR(brush, "length_mode", expand=True)
            elif settings.tool == 'PUFF':
                layout.itemR(brush, "puff_mode", expand=True)

        # Sculpt Mode #

        elif context.sculpt_object and brush:
            col = layout.column()
            col.itemS()
            col.itemR(brush, "sculpt_tool", expand=True)
            col.itemS()

            row = col.row(align=True)
            row.itemR(brush, "size", slider=True)
            row.itemR(brush, "use_size_pressure", toggle=True, text="")

            if brush.sculpt_tool != 'GRAB':
                row = col.row(align=True)
                row.itemR(brush, "strength", slider=True)
                row.itemR(brush, "use_strength_pressure", text="")

                # XXX - TODO
                #row = col.row(align=True)
                #row.itemR(brush, "jitter", slider=True)
                #row.itemR(brush, "use_jitter_pressure", toggle=True, text="")

                col = layout.column()

                if brush.sculpt_tool in ('DRAW', 'PINCH', 'INFLATE', 'LAYER', 'CLAY'):
                    col.row().itemR(brush, "direction", expand=True)

                if brush.sculpt_tool == 'LAYER':
                    col.itemR(brush, "use_persistent")
                    col.itemO("sculpt.set_persistent_base")

        # Texture Paint Mode #

        elif context.texture_paint_object and brush:
            col = layout.column(align=True)
            col.item_enumR(settings, "tool", 'DRAW')
            col.item_enumR(settings, "tool", 'SOFTEN')
            col.item_enumR(settings, "tool", 'CLONE')
            col.item_enumR(settings, "tool", 'SMEAR')

            col = layout.column()
            col.itemR(brush, "color", text="")

            row = col.row(align=True)
            row.itemR(brush, "size", slider=True)
            row.itemR(brush, "use_size_pressure", toggle=True, text="")

            row = col.row(align=True)
            row.itemR(brush, "strength", slider=True)
            row.itemR(brush, "use_strength_pressure", toggle=True, text="")

            row = col.row(align=True)
            row.itemR(brush, "jitter", slider=True)
            row.itemR(brush, "use_jitter_pressure", toggle=True, text="")

            col.itemR(brush, "blend", text="Blend")

        # Weight Paint Mode #

        elif context.weight_paint_object and brush:
            layout.itemR(context.tool_settings, "vertex_group_weight", text="Weight", slider=True)
            layout.itemR(context.tool_settings, "auto_normalize", text="Auto Normalize")

            col = layout.column()
            row = col.row(align=True)
            row.itemR(brush, "size", slider=True)
            row.itemR(brush, "use_size_pressure", toggle=True, text="")

            row = col.row(align=True)
            row.itemR(brush, "strength", slider=True)
            row.itemR(brush, "use_strength_pressure", toggle=True, text="")

            row = col.row(align=True)
            row.itemR(brush, "jitter", slider=True)
            row.itemR(brush, "use_jitter_pressure", toggle=True, text="")

        # Vertex Paint Mode #

        elif context.vertex_paint_object and brush:
            col = layout.column()
            col.itemR(brush, "color", text="")

            row = col.row(align=True)
            row.itemR(brush, "size", slider=True)
            row.itemR(brush, "use_size_pressure", toggle=True, text="")

            row = col.row(align=True)
            row.itemR(brush, "strength", slider=True)
            row.itemR(brush, "use_strength_pressure", toggle=True, text="")

            # XXX - TODO
            #row = col.row(align=True)
            #row.itemR(brush, "jitter", slider=True)
            #row.itemR(brush, "use_jitter_pressure", toggle=True, text="")


class VIEW3D_PT_tools_brush_stroke(PaintPanel):
    bl_label = "Stroke"
    bl_default_closed = True

    def poll(self, context):
        settings = self.paint_settings(context)
        return (settings and settings.brush and (context.sculpt_object or
                             context.vertex_paint_object or
                             context.weight_paint_object or
                             context.texture_paint_object))

    def draw(self, context):
        layout = self.layout

        settings = self.paint_settings(context)
        brush = settings.brush
        texture_paint = context.texture_paint_object

        if context.sculpt_object:
            if brush.sculpt_tool != 'LAYER':
                layout.itemR(brush, "use_anchor")
            layout.itemR(brush, "use_rake")

        layout.itemR(brush, "use_airbrush")
        col = layout.column()
        col.active = brush.use_airbrush
        col.itemR(brush, "rate", slider=True)

        if not texture_paint:
            layout.itemR(brush, "use_smooth_stroke")
            col = layout.column()
            col.active = brush.use_smooth_stroke
            col.itemR(brush, "smooth_stroke_radius", text="Radius", slider=True)
            col.itemR(brush, "smooth_stroke_factor", text="Factor", slider=True)

        layout.itemR(brush, "use_space")
        row = layout.row(align=True)
        row.active = brush.use_space
        row.itemR(brush, "spacing", text="Distance", slider=True)
        if texture_paint:
            row.itemR(brush, "use_spacing_pressure", toggle=True, text="")


class VIEW3D_PT_tools_brush_curve(PaintPanel):
    bl_label = "Curve"
    bl_default_closed = True

    def poll(self, context):
        settings = self.paint_settings(context)
        return (settings and settings.brush and settings.brush.curve)

    def draw(self, context):
        layout = self.layout

        settings = self.paint_settings(context)
        brush = settings.brush

        layout.template_curve_mapping(brush, "curve")
        layout.item_menu_enumO("brush.curve_preset", property="shape")


class VIEW3D_PT_sculpt_options(PaintPanel):
    bl_label = "Options"

    def poll(self, context):
        return context.sculpt_object

    def draw(self, context):
        layout = self.layout

        sculpt = context.tool_settings.sculpt

        col = layout.column()
        col.itemR(sculpt, "partial_redraw", text="Partial Refresh")
        col.itemR(sculpt, "show_brush")

        split = self.layout.split()

        col = split.column()
        col.itemL(text="Symmetry:")
        col.itemR(sculpt, "symmetry_x", text="X")
        col.itemR(sculpt, "symmetry_y", text="Y")
        col.itemR(sculpt, "symmetry_z", text="Z")

        col = split.column()
        col.itemL(text="Lock:")
        col.itemR(sculpt, "lock_x", text="X")
        col.itemR(sculpt, "lock_y", text="Y")
        col.itemR(sculpt, "lock_z", text="Z")

# ********** default tools for weightpaint ****************


class VIEW3D_PT_tools_weightpaint(View3DPanel):
    bl_context = "weightpaint"
    bl_label = "Weight Tools"

    def draw(self, context):
        layout = self.layout

        col = layout.column()
        # col.itemL(text="Blend:")
        col.itemO("object.vertex_group_normalize_all", text="Normalize All")
        col.itemO("object.vertex_group_normalize", text="Normalize")
        col.itemO("object.vertex_group_invert", text="Invert")
        col.itemO("object.vertex_group_clean", text="Clean")


class VIEW3D_PT_tools_weightpaint_options(View3DPanel):
    bl_context = "weightpaint"
    bl_label = "Options"

    def draw(self, context):
        layout = self.layout

        wpaint = context.tool_settings.weight_paint

        col = layout.column()
        col.itemL(text="Blend:")
        col.itemR(wpaint, "mode", text="")
        col.itemR(wpaint, "all_faces")
        col.itemR(wpaint, "normals")
        col.itemR(wpaint, "spray")
        col.itemR(wpaint, "vertex_dist", text="Distance")


        data = context.weight_paint_object.data
        if type(data) == bpy.types.Mesh:
            col.itemR(data, "use_mirror_x")

# Commented out because the Apply button isn't an operator yet, making these settings useless
#		col.itemL(text="Gamma:")
#		col.itemR(wpaint, "gamma", text="")
#		col.itemL(text="Multiply:")
#		col.itemR(wpaint, "mul", text="")

# Also missing now:
# Soft, Vgroup, X-Mirror and "Clear" Operator.

# ********** default tools for vertexpaint ****************


class VIEW3D_PT_tools_vertexpaint(View3DPanel):
    bl_context = "vertexpaint"
    bl_label = "Options"

    def draw(self, context):
        layout = self.layout

        vpaint = context.tool_settings.vertex_paint

        col = layout.column()
        col.itemL(text="Blend:")
        col.itemR(vpaint, "mode", text="")
        col.itemR(vpaint, "all_faces")
        col.itemR(vpaint, "normals")
        col.itemR(vpaint, "spray")
        col.itemR(vpaint, "vertex_dist", text="Distance")
# Commented out because the Apply button isn't an operator yet, making these settings useless
#		col.itemL(text="Gamma:")
#		col.itemR(vpaint, "gamma", text="")
#		col.itemL(text="Multiply:")
#		col.itemR(vpaint, "mul", text="")

# ********** default tools for texturepaint ****************


class VIEW3D_PT_tools_projectpaint(View3DPanel):
    bl_context = "texturepaint"
    bl_label = "Project Paint"

    def poll(self, context):
        return context.tool_settings.image_paint.tool != 'SMEAR'

    def draw_header(self, context):
        ipaint = context.tool_settings.image_paint

        self.layout.itemR(ipaint, "use_projection", text="")

    def draw(self, context):
        layout = self.layout

        ipaint = context.tool_settings.image_paint
        settings = context.tool_settings.image_paint
        use_projection = ipaint.use_projection

        col = layout.column()
        sub = col.column()
        sub.active = use_projection
        sub.itemR(ipaint, "use_occlude")
        sub.itemR(ipaint, "use_backface_cull")

        split = layout.split()

        col = split.column()
        col.active = (use_projection)
        col.itemR(ipaint, "use_normal_falloff")

        col = split.column()
        col.active = (ipaint.use_normal_falloff and use_projection)
        col.itemR(ipaint, "normal_angle", text="")

        split = layout.split(percentage=0.7)

        col = split.column(align=False)
        col.active = (use_projection)
        col.itemR(ipaint, "use_stencil_layer")

        col = split.column(align=False)
        col.active = (use_projection and ipaint.use_stencil_layer)
        col.itemR(ipaint, "invert_stencil", text="Inv")

        col = layout.column()
        sub = col.column()
        sub.active = (settings.tool == 'CLONE')
        sub.itemR(ipaint, "use_clone_layer")

        sub = col.column()
        sub.itemR(ipaint, "seam_bleed")


class VIEW3D_PT_tools_particlemode(View3DPanel):
    '''default tools for particle mode'''
    bl_context = "particlemode"
    bl_label = "Options"

    def draw(self, context):
        layout = self.layout

        pe = context.tool_settings.particle_edit
        ob = pe.object

        layout.itemR(pe, "type", text="")

        ptcache = None

        if pe.type == 'PARTICLES':
            if ob.particle_systems:
                if len(ob.particle_systems) > 1:
                    layout.template_list(ob, "particle_systems", ob, "active_particle_system_index", type='ICONS')

                ptcache = ob.particle_systems[ob.active_particle_system_index].point_cache
        else:
            for md in ob.modifiers:
                if md.type == pe.type:
                    ptcache = md.point_cache

        if ptcache and len(ptcache.point_cache_list) > 1:
            layout.template_list(ptcache, "point_cache_list", ptcache, "active_point_cache_index", type='ICONS')


        if not pe.editable:
            layout.itemL(text="Point cache must be baked")
            layout.itemL(text="to enable editing!")

        col = layout.column(align=True)
        if pe.hair:
            col.active = pe.editable
            col.itemR(pe, "emitter_deflect", text="Deflect emitter")
            sub = col.row()
            sub.active = pe.emitter_deflect
            sub.itemR(pe, "emitter_distance", text="Distance")

        col = layout.column(align=True)
        col.active = pe.editable
        col.itemL(text="Keep:")
        col.itemR(pe, "keep_lengths", text="Lenghts")
        col.itemR(pe, "keep_root", text="Root")
        if not pe.hair:
            col.itemL(text="Correct:")
            col.itemR(pe, "auto_velocity", text="Velocity")

        col = layout.column(align=True)
        col.active = pe.editable
        col.itemL(text="Draw:")
        col.itemR(pe, "draw_step", text="Path Steps")
        if pe.type == 'PARTICLES':
            col.itemR(pe, "draw_particles", text="Particles")
        col.itemR(pe, "fade_time")
        sub = col.row()
        sub.active = pe.fade_time
        sub.itemR(pe, "fade_frames", slider=True)


bpy.types.register(VIEW3D_PT_tools_weightpaint)
bpy.types.register(VIEW3D_PT_tools_objectmode)
bpy.types.register(VIEW3D_PT_tools_meshedit)
bpy.types.register(VIEW3D_PT_tools_meshedit_options)
bpy.types.register(VIEW3D_PT_tools_curveedit)
bpy.types.register(VIEW3D_PT_tools_surfaceedit)
bpy.types.register(VIEW3D_PT_tools_textedit)
bpy.types.register(VIEW3D_PT_tools_armatureedit)
bpy.types.register(VIEW3D_PT_tools_armatureedit_options)
bpy.types.register(VIEW3D_PT_tools_mballedit)
bpy.types.register(VIEW3D_PT_tools_latticeedit)
bpy.types.register(VIEW3D_PT_tools_posemode)
bpy.types.register(VIEW3D_PT_tools_posemode_options)
bpy.types.register(VIEW3D_PT_tools_brush)
bpy.types.register(VIEW3D_PT_tools_brush_stroke)
bpy.types.register(VIEW3D_PT_tools_brush_curve)
bpy.types.register(VIEW3D_PT_sculpt_options)
bpy.types.register(VIEW3D_PT_tools_vertexpaint)
bpy.types.register(VIEW3D_PT_tools_weightpaint_options)
bpy.types.register(VIEW3D_PT_tools_projectpaint)
bpy.types.register(VIEW3D_PT_tools_particlemode)
