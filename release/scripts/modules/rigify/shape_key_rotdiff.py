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
from rigify import RigifyError

#METARIG_NAMES = ("cpy",)
RIG_TYPE = "shape_key_rotdiff"


def addget_shape_key(obj, name="Key"):
    """ Fetches a shape key, or creates it if it doesn't exist
    """
    # Create a shapekey set if it doesn't already exist
    if obj.data.shape_keys is None:
        shape = obj.add_shape_key(name="Basis", from_mix=False)
        obj.active_shape_key_index = 0

    # Get the shapekey, or create it if it doesn't already exist
    if name in obj.data.shape_keys.keys:
        shape_key = obj.data.shape_keys.keys[name]
    else:
        shape_key = obj.add_shape_key(name=name, from_mix=False)

    return shape_key


def addget_shape_key_driver(obj, name="Key"):
    """ Fetches the driver for the shape key, or creates it if it doesn't
        already exist.
    """
    driver_path = 'keys["' + name + '"].value'
    fcurve = None
    driver = None
    if obj.data.shape_keys.animation_data is not None:
        for driver_s in obj.data.shape_keys.animation_data.drivers:
            if driver_s.data_path == driver_path:
                fcurve = driver_s
    if fcurve == None:
        fcurve = obj.data.shape_keys.keys[name].driver_add("value", 0)
    fcurve.driver.type = 'AVERAGE'

    return fcurve




def metarig_template():
    # generated by rigify.write_meta_rig
    bpy.ops.object.mode_set(mode='EDIT')
    obj = bpy.context.active_object
    arm = obj.data
    bone = arm.edit_bones.new('Bone')
    bone.head[:] = 0.0000, 0.0000, 0.0000
    bone.tail[:] = 0.0000, 0.0000, 1.0000
    bone.roll = 0.0000
    bone.connected = False

    bpy.ops.object.mode_set(mode='OBJECT')
    pbone = obj.pose.bones['Bone']
    pbone['type'] = 'copy'


def metarig_definition(obj, orig_bone_name):
    bone = obj.data.bones[orig_bone_name]
    return [bone.name]


def deform(obj, definitions, base_names, options):
    bpy.ops.object.mode_set(mode='EDIT')
    eb = obj.data.edit_bones

    bone_from = definitions[0]


    # Options
    req_options = ["to", "mesh", "shape_key"]
    for option in req_options:
        if option not in options:
            raise RigifyError("'%s' rig type requires a '%s' option (bone: %s)" % (RIG_TYPE, option, base_names[definitions[0]]))

    bone_to = "ORG-" + options["to"]
    meshes = options["mesh"].replace(" ", "").split(",")
    shape_key_name = options["shape_key"]

    if "dmul" in options:
        shape_blend_fac = options["dmul"]
    else:
        shape_blend_fac = 1.0


    # Calculate the rotation difference between the bones
    rotdiff = (eb[bone_from].matrix.to_quat() * eb[bone_to].matrix.to_quat()) * 2

    bpy.ops.object.mode_set(mode='OBJECT')

    # For every listed mesh object
    for mesh_name in meshes:
        mesh_obj = bpy.data.objects[mesh_name]

        # Add/get the shape key
        shape_key = addget_shape_key(mesh_obj, name=shape_key_name)

        # Add/get the shape key driver
        fcurve = addget_shape_key_driver(mesh_obj, name=shape_key_name)
        driver = fcurve.driver

        # Get the variable, or create it if it doesn't already exist
        var_name = base_names[bone_from]
        if var_name in driver.variables:
            var = driver.variables[var_name]
        else:
            var = driver.variables.new()
            var.name = var_name

        # Set up the variable
        var.type = "ROTATION_DIFF"
        var.targets[0].id_type = 'OBJECT'
        var.targets[0].id = obj
        var.targets[0].bone_target = bone_from
        var.targets[1].id_type = 'OBJECT'
        var.targets[1].id = obj
        var.targets[1].bone_target = bone_to

        # Set fcurve offset, so zero is at the rest distance

        mod = fcurve.modifiers[0]
        if rotdiff > 0.00001:
            mod.coefficients[0] = -shape_blend_fac
            mod.coefficients[1] = shape_blend_fac / rotdiff

    return (None,)




def control(obj, definitions, base_names, options):
    """ options:
        mesh: name of mesh object with the shape key
        shape_key: name of shape key
        to: name of bone to measure distance from
    """
    pass




def main(obj, bone_definition, base_names, options):
    # Create control rig
    #control(obj, bone_definition, base_names, options)
    # Create deform rig
    deform(obj, bone_definition, base_names, options)

    return (None,)

