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

narrowui = 180


class DataButtonsPanel(bpy.types.Panel):
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "data"

    def poll(self, context):
        return context.armature


class DATA_PT_context_arm(DataButtonsPanel):
    bl_label = ""
    bl_show_header = False

    def draw(self, context):
        layout = self.layout

        ob = context.object
        arm = context.armature
        space = context.space_data
        wide_ui = context.region.width > narrowui

        if wide_ui:
            split = layout.split(percentage=0.65)
            if ob:
                split.template_ID(ob, "data")
                split.itemS()
            elif arm:
                split.template_ID(space, "pin_id")
                split.itemS()
        else:
            layout.template_ID(ob, "data")


class DATA_PT_skeleton(DataButtonsPanel):
    bl_label = "Skeleton"

    def draw(self, context):
        layout = self.layout

        arm = context.armature
        wide_ui = context.region.width > narrowui

        layout.itemR(arm, "pose_position", expand=True)

        split = layout.split()

        col = split.column()
        col.itemL(text="Layers:")
        col.itemR(arm, "layer", text="")
        col.itemL(text="Protected Layers:")
        col.itemR(arm, "layer_protection", text="")

        if wide_ui:
            col = split.column()
        col.itemL(text="Deform:")
        col.itemR(arm, "deform_vertexgroups", text="Vertex Groups")
        col.itemR(arm, "deform_envelope", text="Envelopes")
        col.itemR(arm, "deform_quaternion", text="Quaternion")
        col.itemR(arm, "deform_bbone_rest", text="B-Bones Rest")


class DATA_PT_display(DataButtonsPanel):
    bl_label = "Display"

    def draw(self, context):
        layout = self.layout

        arm = context.armature
        wide_ui = context.region.width > narrowui

        if wide_ui:
            layout.row().itemR(arm, "drawtype", expand=True)
        else:
            layout.row().itemR(arm, "drawtype", text="")

        split = layout.split()

        col = split.column()
        col.itemR(arm, "draw_names", text="Names")
        col.itemR(arm, "draw_axes", text="Axes")
        col.itemR(arm, "draw_custom_bone_shapes", text="Shapes")

        if wide_ui:
            col = split.column()
        col.itemR(arm, "draw_group_colors", text="Colors")
        col.itemR(arm, "delay_deform", text="Delay Refresh")


class DATA_PT_bone_groups(DataButtonsPanel):
    bl_label = "Bone Groups"

    def poll(self, context):
        return (context.object and context.object.type == 'ARMATURE' and context.object.pose)

    def draw(self, context):
        layout = self.layout

        ob = context.object
        pose = ob.pose
        wide_ui = context.region.width > narrowui

        row = layout.row()
        row.template_list(pose, "bone_groups", pose, "active_bone_group_index", rows=2)

        col = row.column(align=True)
        col.active = (ob.proxy is None)
        col.itemO("pose.group_add", icon='ICON_ZOOMIN', text="")
        col.itemO("pose.group_remove", icon='ICON_ZOOMOUT', text="")

        group = pose.active_bone_group
        if group:
            col = layout.column()
            col.active = (ob.proxy is None)
            col.itemR(group, "name")

            split = layout.split()
            split.active = (ob.proxy is None)

            col = split.column()
            col.itemR(group, "color_set")
            if group.color_set:
                if wide_ui:
                    col = split.column()
                col.template_triColorSet(group, "colors")

        row = layout.row(align=True)
        row.active = (ob.proxy is None)

        row.itemO("pose.group_assign", text="Assign")
        row.itemO("pose.group_remove", text="Remove") #row.itemO("pose.bone_group_remove_from", text="Remove")
        #row.itemO("object.bone_group_select", text="Select")
        #row.itemO("object.bone_group_deselect", text="Deselect")


class DATA_PT_paths(DataButtonsPanel):
    bl_label = "Paths"

    def draw(self, context):
        layout = self.layout

        arm = context.armature
        wide_ui = context.region.width > narrowui

        if wide_ui:
            layout.itemR(arm, "paths_type", expand=True)
        else:
            layout.itemR(arm, "paths_type", text="")

        split = layout.split()

        col = split.column()
        sub = col.column(align=True)
        if (arm.paths_type == 'CURRENT_FRAME'):
            sub.itemR(arm, "path_before_current", text="Before")
            sub.itemR(arm, "path_after_current", text="After")
        elif (arm.paths_type == 'RANGE'):
            sub.itemR(arm, "path_start_frame", text="Start")
            sub.itemR(arm, "path_end_frame", text="End")

        sub.itemR(arm, "path_size", text="Step")
        col.row().itemR(arm, "paths_location", expand=True)

        if wide_ui:
            col = split.column()
        col.itemL(text="Display:")
        col.itemR(arm, "paths_show_frame_numbers", text="Frame Numbers")
        col.itemR(arm, "paths_highlight_keyframes", text="Keyframes")
        col.itemR(arm, "paths_show_keyframe_numbers", text="Keyframe Numbers")

        layout.itemS()

        split = layout.split()

        col = split.column()
        col.itemO("pose.paths_calculate", text="Calculate Paths")

        if wide_ui:
            col = split.column()
        col.itemO("pose.paths_clear", text="Clear Paths")


class DATA_PT_ghost(DataButtonsPanel):
    bl_label = "Ghost"

    def draw(self, context):
        layout = self.layout

        arm = context.armature
        wide_ui = context.region.width > narrowui

        if wide_ui:
            layout.itemR(arm, "ghost_type", expand=True)
        else:
            layout.itemR(arm, "ghost_type", text="")

        split = layout.split()

        col = split.column()

        sub = col.column(align=True)
        if arm.ghost_type == 'RANGE':
            sub.itemR(arm, "ghost_start_frame", text="Start")
            sub.itemR(arm, "ghost_end_frame", text="End")
            sub.itemR(arm, "ghost_size", text="Step")
        elif arm.ghost_type == 'CURRENT_FRAME':
            sub.itemR(arm, "ghost_step", text="Range")
            sub.itemR(arm, "ghost_size", text="Step")

        if wide_ui:
            col = split.column()
        col.itemL(text="Display:")
        col.itemR(arm, "ghost_only_selected", text="Selected Only")

bpy.types.register(DATA_PT_context_arm)
bpy.types.register(DATA_PT_skeleton)
bpy.types.register(DATA_PT_display)
bpy.types.register(DATA_PT_bone_groups)
bpy.types.register(DATA_PT_paths)
bpy.types.register(DATA_PT_ghost)
