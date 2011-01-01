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
#  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# ##### END GPL LICENSE BLOCK #####

# <pep8 compliant>

# To support reload properly, try to access a package var, if it's there, reload everything
if "bpy" in locals():
    import imp
    if "import_bvh" in locals():
        imp.reload(import_bvh)


import bpy
from bpy.props import *
from io_utils import ImportHelper, ExportHelper


class BvhImporter(bpy.types.Operator, ImportHelper):
    '''Load a BVH motion capture file'''
    bl_idname = "import_anim.bvh"
    bl_label = "Import BVH"

    filename_ext = ".bvh"
    filter_glob = StringProperty(default="*.bvh", options={'HIDDEN'})

    global_scale = FloatProperty(name="Scale", description="Scale the BVH by this value", min=0.0001, max=1000000.0, soft_min=0.001, soft_max=100.0, default=1.0)
    frame_start = IntProperty(name="Start Frame", description="Starting frame for the animation", default=1)
    use_cyclic = BoolProperty(name="Loop", description="Loop the animation playback", default=False)
    rotate_mode = EnumProperty(items=(
            ('QUATERNION', "Quaternion", "Convert rotations to quaternions"),
            ('NATIVE', "Euler (Native)", "Use the rotation order defined in the BVH file"),
            ('XYZ', "Euler (XYZ)", "Convert rotations to euler XYZ"),
            ('XZY', "Euler (XZY)", "Convert rotations to euler XZY"),
            ('YXZ', "Euler (YXZ)", "Convert rotations to euler YXZ"),
            ('YZX', "Euler (YZX)", "Convert rotations to euler YZX"),
            ('ZXY', "Euler (ZXY)", "Convert rotations to euler ZXY"),
            ('ZYX', "Euler (ZYX)", "Convert rotations to euler ZYX"),
            ),
                name="Rotation",
                description="Rotation conversion.",
                default='NATIVE')

    def execute(self, context):
        from . import import_bvh
        return import_bvh.load(self, context, **self.as_keywords(ignore=("filter_glob",)))


class BvhExporter(bpy.types.Operator, ExportHelper):
    '''Save a BVH motion capture file from an armature'''
    bl_idname = "export_anim.bvh"
    bl_label = "Export BVH"

    filename_ext = ".bvh"
    filter_glob = StringProperty(default="*.bvh", options={'HIDDEN'})

    global_scale = FloatProperty(name="Scale", description="Scale the BVH by this value", min=0.0001, max=1000000.0, soft_min=0.001, soft_max=100.0, default=1.0)
    frame_start = IntProperty(name="Start Frame", description="Starting frame to export", default=0)
    frame_end = IntProperty(name="End Frame", description="End frame to export", default=0)

    @classmethod
    def poll(cls, context):
        obj = context.object
        return obj and obj.type == 'ARMATURE'

    def invoke(self, context, event):
        self.frame_start = context.scene.frame_start
        self.frame_end = context.scene.frame_end

        return super().invoke(context, event)

    def execute(self, context):
        if self.frame_start == 0 and self.frame_end == 0:
            self.frame_start = context.scene.frame_start
            self.frame_end = context.scene.frame_end

        from . import export_bvh
        return export_bvh.save(self, context, **self.as_keywords(ignore=("check_existing", "filter_glob")))


def menu_func_import(self, context):
    self.layout.operator(BvhImporter.bl_idname, text="Motion Capture (.bvh)")


def menu_func_export(self, context):
    self.layout.operator(BvhExporter.bl_idname, text="Motion Capture (.bvh)")


def register():
    bpy.types.INFO_MT_file_import.append(menu_func_import)
    bpy.types.INFO_MT_file_export.append(menu_func_export)


def unregister():
    bpy.types.INFO_MT_file_import.remove(menu_func_import)
    bpy.types.INFO_MT_file_export.remove(menu_func_export)

if __name__ == "__main__":
    register()
