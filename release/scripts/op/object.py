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
from bpy.props import *


class SelectPattern(bpy.types.Operator):
    '''Select object matching a naming pattern.'''
    bl_idname = "object.select_pattern"
    bl_label = "Select Pattern"
    bl_register = True
    bl_undo = True

    pattern = StringProperty(name="Pattern", description="Name filter using '*' and '?' wildcard chars", maxlen=32, default="*")
    case_sensitive = BoolProperty(name="Case Sensitive", description="Do a case sensitive compare", default=False)
    extend = BoolProperty(name="Extend", description="Extend the existing selection", default=True)

    def execute(self, context):

        import fnmatch

        if self.properties.case_sensitive:
            pattern_match = fnmatch.fnmatchcase
        else:
            pattern_match = lambda a, b: fnmatch.fnmatchcase(a.upper(), b.upper())

        obj = context.object
        if obj and obj.mode == 'POSE':
            items = obj.data.bones
        elif obj and obj.type == 'ARMATURE' and obj.mode == 'EDIT':
            items = obj.data.edit_bones
        else:
            items = context.visible_objects

        # Can be pose bones or objects
        for item in items:
            if pattern_match(item.name, self.properties.pattern):
                item.selected = True
            elif not self.properties.extend:
                item.selected = False

        return ('FINISHED',)

    def invoke(self, context, event):
        wm = context.manager
        # return wm.invoke_props_popup(self, event)
        wm.invoke_props_popup(self, event)
        return ('RUNNING_MODAL',)

    def draw(self, context):
        layout = self.layout
        props = self.properties

        layout.prop(props, "pattern")
        row = layout.row()
        row.prop(props, "case_sensitive")
        row.prop(props, "extend")


class SubdivisionSet(bpy.types.Operator):
    '''Sets a Subdivision Surface Level (1-5)'''

    bl_idname = "object.subdivision_set"
    bl_label = "Subdivision Set"
    bl_register = True
    bl_undo = True

    level = IntProperty(name="Level",
            default=1, min=-100, max=100, soft_min=-6, soft_max=6)

    relative = BoolProperty(name="Relative", description="Apply the subsurf level as an offset relative to the current level", default=False)

    def poll(self, context):
        obs = context.selected_editable_objects
        return (obs is not None)

    def execute(self, context):
        level = self.properties.level
        relative = self.properties.relative

        if relative and level == 0:
            return ('CANCELLED',) # nothing to do

        def set_object_subd(obj):
            for mod in obj.modifiers:
                if mod.type == 'MULTIRES':
                    if level <= mod.total_levels:
                        if obj.mode == 'SCULPT':
                            if relative:
                                mod.sculpt_levels += level
                            else:
                                if mod.sculpt_levels != level:
                                    mod.sculpt_levels = level
                        elif obj.mode == 'OBJECT':
                            if relative:
                                mod.levels += level
                            else:
                                if mod.levels != level:
                                    mod.levels = level
                    return
                elif mod.type == 'SUBSURF':
                    if relative:
                        mod.levels += level
                    else:
                        if mod.levels != level:
                            mod.levels = level

                    return

            # adda new modifier
            mod = obj.modifiers.new("Subsurf", 'SUBSURF')
            mod.levels = level

        for obj in context.selected_editable_objects:
            set_object_subd(obj)

        return ('FINISHED',)


class Retopo(bpy.types.Operator):
    '''TODO - doc'''

    bl_idname = "object.retopology"
    bl_label = "Retopology from Grease Pencil"
    bl_register = True
    bl_undo = True

    def execute(self, context):
        import retopo
        retopo.main()
        return ('FINISHED',)


bpy.ops.add(SelectPattern)
bpy.ops.add(SubdivisionSet)
bpy.ops.add(Retopo)
