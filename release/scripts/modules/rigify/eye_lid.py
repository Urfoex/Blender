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
from rna_prop_ui import rna_idprop_ui_prop_get
from math import acos
from Mathutils import Vector
from rigify import RigifyError
from rigify_utils import copy_bone_simple

#METARIG_NAMES = ("cpy",)
RIG_TYPE = "eye_lid"

def mark_actions():
    for action in bpy.data.actions:
        action.tag = True

def get_unmarked_action():
    for action in bpy.data.actions:
        if action.tag != True:
            return action
    return None

def add_action(name=None):
    mark_actions()
    bpy.ops.action.new()
    action = get_unmarked_action()
    if name is not None:
        action.name = name
    return action


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
    bb = obj.data.bones
    bone = bb[orig_bone_name]
    chain = []

    try:
        chain += [bone.parent.parent.name, bone.parent.name, bone.name]
    except AttributeError:
        raise RigifyError("'%s' rig type requires a chain of two parents (bone: %s)" % (RIG_TYPE, orig_bone_name))

    chain += [child.name for child in bone.children_recursive_basename]

    if len(chain) < 10:
        raise RigifyError("'%s' rig type requires a chain of 10 bones (bone: %s)" % (RIG_TYPE, orig_bone_name))

    chain = chain[:10]

    try:
        chain += [bb[chain[9]].children[0].name]
        chain += [bb[chain[10]].children[0].name]
    except IndexError:
        raise RigifyError("'%s' rig type requires a chain of 10 bones (bone: %s)" % (RIG_TYPE, orig_bone_name))

    return chain


def deform(obj, definitions, base_names, options):
    bpy.ops.object.mode_set(mode='EDIT')

    eb = obj.data.edit_bones
    pb = obj.pose.bones


    # Upper lid MCH
    lid1  = make_lid_stretch_bone(obj, "MCH-lid", definitions[2], definitions[3], 1.0)
    lid2  = make_lid_stretch_bone(obj, "MCH-lid", definitions[3], definitions[4], 1.0)
    lid22 = make_lid_stretch_bone(obj, "MCH-lid", definitions[4], definitions[5], 1.0)
    lid33 = make_lid_stretch_bone(obj, "MCH-lid", definitions[4], definitions[3], 1.0)
    lid3  = make_lid_stretch_bone(obj, "MCH-lid", definitions[5], definitions[4], 1.0)
    lid4  = make_lid_stretch_bone(obj, "MCH-lid", definitions[6], definitions[5], 1.0)

    dlid22 = copy_bone_simple(obj.data, lid22, "MCH-lid", parent=True).name
    dlid33 = copy_bone_simple(obj.data, lid33, "MCH-lid", parent=True).name
    eb[dlid22].bbone_segments = 8
    eb[dlid33].bbone_segments = 8

    eb[lid1].parent = eb[definitions[2]]
    eb[lid2].parent = eb[definitions[3]]
    eb[lid22].parent = eb[definitions[4]]
    eb[lid33].parent = eb[definitions[4]]
    eb[lid3].parent = eb[definitions[5]]
    eb[lid4].parent = eb[definitions[6]]

    # Lower lid MCH
    lid5 =  make_lid_stretch_bone(obj, "MCH-lid", definitions[6], definitions[7], 1.0)
    lid6 =  make_lid_stretch_bone(obj, "MCH-lid", definitions[7], definitions[8], 1.0)
    lid66 = make_lid_stretch_bone(obj, "MCH-lid", definitions[8], definitions[9], 1.0)
    lid77 = make_lid_stretch_bone(obj, "MCH-lid", definitions[8], definitions[7], 1.0)
    lid7 =  make_lid_stretch_bone(obj, "MCH-lid", definitions[9], definitions[8], 1.0)
    lid8 =  make_lid_stretch_bone(obj, "MCH-lid", definitions[2], definitions[9], 1.0)

    dlid66 = copy_bone_simple(obj.data, lid66, "MCH-lid", parent=True).name
    dlid77 = copy_bone_simple(obj.data, lid77, "MCH-lid", parent=True).name
    eb[dlid66].bbone_segments = 8
    eb[dlid77].bbone_segments = 8

    eb[lid5].parent = eb[definitions[6]]
    eb[lid6].parent = eb[definitions[7]]
    eb[lid66].parent = eb[definitions[8]]
    eb[lid77].parent = eb[definitions[8]]
    eb[lid7].parent = eb[definitions[9]]
    eb[lid8].parent = eb[definitions[2]]

    # Upper lid DEF
    dlid1 = copy_bone_simple(obj.data, lid1, "DEF-" + base_names[definitions[2]], parent=True).name
    dlid2 = copy_bone_simple(obj.data, lid2, "DEF-" + base_names[definitions[3]], parent=True).name
    dlid3 = copy_bone_simple(obj.data, lid3, "DEF-" + base_names[definitions[4]], parent=True).name
    dlid4 = copy_bone_simple(obj.data, lid4, "DEF-" + base_names[definitions[5]], parent=True).name

    eb[dlid2].parent  = eb[dlid1]
    eb[dlid22].parent = eb[dlid2]

    eb[dlid3].parent  = eb[dlid4]
    eb[dlid33].parent = eb[dlid3]

    eb[dlid2].connected  = True
    eb[dlid22].connected = True
    eb[dlid3].connected  = True
    eb[dlid33].connected = True

    eb[dlid1].bbone_segments = 8
    eb[dlid2].bbone_segments = 8
    eb[dlid3].bbone_segments = 8
    eb[dlid4].bbone_segments = 8

    # Lower lid DEF
    dlid5 = copy_bone_simple(obj.data, lid5, "DEF-" + base_names[definitions[6]], parent=True).name
    dlid6 = copy_bone_simple(obj.data, lid6, "DEF-" + base_names[definitions[7]], parent=True).name
    dlid7 = copy_bone_simple(obj.data, lid7, "DEF-" + base_names[definitions[8]], parent=True).name
    dlid8 = copy_bone_simple(obj.data, lid8, "DEF-" + base_names[definitions[9]], parent=True).name

    eb[dlid6].parent = eb[dlid5]
    eb[dlid66].parent = eb[dlid6]

    eb[dlid7].parent = eb[dlid8]
    eb[dlid77].parent = eb[dlid7]

    eb[dlid6].connected = True
    eb[dlid66].connected = True
    eb[dlid7].connected = True
    eb[dlid77].connected = True

    eb[dlid5].bbone_segments = 8
    eb[dlid6].bbone_segments = 8
    eb[dlid7].bbone_segments = 8
    eb[dlid8].bbone_segments = 8


    bpy.ops.object.mode_set(mode='OBJECT')

    # Constraints
    con = pb[dlid1].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = lid1

    con = pb[dlid22].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = lid22

    con = pb[dlid33].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = lid33

    con = pb[dlid2].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = lid2

    con = pb[dlid3].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = lid3

    con = pb[dlid4].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = lid4

    con = pb[dlid5].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = lid5

    con = pb[dlid6].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = lid6

    con = pb[dlid66].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = lid66

    con = pb[dlid77].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = lid77

    con = pb[dlid7].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = lid7

    con = pb[dlid8].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = lid8

    return (None,)




def control(obj, definitions, base_names, options):
    bpy.ops.object.mode_set(mode='EDIT')

    eb = obj.data.edit_bones
    bb = obj.data.bones
    pb = obj.pose.bones

    head_e = eb[definitions[0]]
    eye_e = eb[definitions[1]]


    # Make eye "flower"
    flo1 = copy_bone_simple(obj.data, definitions[1], "MCH-"+base_names[definitions[2]]+".flower", parent=True).name
    flo2 = copy_bone_simple(obj.data, definitions[1], "MCH-"+base_names[definitions[3]]+".flower", parent=True).name
    flo3 = copy_bone_simple(obj.data, definitions[1], "MCH-"+base_names[definitions[4]]+".flower", parent=True).name
    flo4 = copy_bone_simple(obj.data, definitions[1], "MCH-"+base_names[definitions[5]]+".flower", parent=True).name
    flo5 = copy_bone_simple(obj.data, definitions[1], "MCH-"+base_names[definitions[6]]+".flower", parent=True).name
    flo6 = copy_bone_simple(obj.data, definitions[1], "MCH-"+base_names[definitions[7]]+".flower", parent=True).name
    flo7 = copy_bone_simple(obj.data, definitions[1], "MCH-"+base_names[definitions[8]]+".flower", parent=True).name
    flo8 = copy_bone_simple(obj.data, definitions[1], "MCH-"+base_names[definitions[9]]+".flower", parent=True).name

    eb[flo1].tail = eb[definitions[2]].head
    eb[flo2].tail = eb[definitions[3]].head
    eb[flo3].tail = eb[definitions[4]].head
    eb[flo4].tail = eb[definitions[5]].head
    eb[flo5].tail = eb[definitions[6]].head
    eb[flo6].tail = eb[definitions[7]].head
    eb[flo7].tail = eb[definitions[8]].head
    eb[flo8].tail = eb[definitions[9]].head


    # Make eye lids on tips of flowers
    flid1 = copy_bone_simple(obj.data, definitions[2], "MCH-"+base_names[definitions[2]]).name
    flid2 = copy_bone_simple(obj.data, definitions[3], "MCH-"+base_names[definitions[3]]).name
    flid3 = copy_bone_simple(obj.data, definitions[4], "MCH-"+base_names[definitions[4]]).name
    flid4 = copy_bone_simple(obj.data, definitions[5], "MCH-"+base_names[definitions[5]]).name
    flid5 = copy_bone_simple(obj.data, definitions[6], "MCH-"+base_names[definitions[6]]).name
    flid6 = copy_bone_simple(obj.data, definitions[7], "MCH-"+base_names[definitions[7]]).name
    flid7 = copy_bone_simple(obj.data, definitions[8], "MCH-"+base_names[definitions[8]]).name
    flid8 = copy_bone_simple(obj.data, definitions[9], "MCH-"+base_names[definitions[9]]).name

    eb[flid1].parent = eb[flo1]
    eb[flid2].parent = eb[flo2]
    eb[flid3].parent = eb[flo3]
    eb[flid4].parent = eb[flo4]
    eb[flid5].parent = eb[flo5]
    eb[flid6].parent = eb[flo6]
    eb[flid7].parent = eb[flo7]
    eb[flid8].parent = eb[flo8]


    # Make eye lid controls
    lid1 = copy_bone_simple(obj.data, definitions[2], base_names[definitions[2]]).name
    lid2 = copy_bone_simple(obj.data, definitions[3], base_names[definitions[3]]).name
    lid3 = copy_bone_simple(obj.data, definitions[4], base_names[definitions[4]]).name
    lid4 = copy_bone_simple(obj.data, definitions[5], base_names[definitions[5]]).name
    lid5 = copy_bone_simple(obj.data, definitions[6], base_names[definitions[6]]).name
    lid6 = copy_bone_simple(obj.data, definitions[7], base_names[definitions[7]]).name
    lid7 = copy_bone_simple(obj.data, definitions[8], base_names[definitions[8]]).name
    lid8 = copy_bone_simple(obj.data, definitions[9], base_names[definitions[9]]).name

    size = eb[lid1].length
    eb[lid1].tail = eb[lid1].head + Vector(0,size,0)
    eb[lid2].tail = eb[lid2].head + Vector(0,size,0)
    eb[lid3].tail = eb[lid3].head + Vector(0,size,0)
    eb[lid4].tail = eb[lid4].head + Vector(0,size,0)
    eb[lid5].tail = eb[lid5].head + Vector(0,size,0)
    eb[lid6].tail = eb[lid6].head + Vector(0,size,0)
    eb[lid7].tail = eb[lid7].head + Vector(0,size,0)
    eb[lid8].tail = eb[lid8].head + Vector(0,size,0)

    eb[lid1].roll = 0
    eb[lid2].roll = 0
    eb[lid3].roll = 0
    eb[lid4].roll = 0
    eb[lid5].roll = 0
    eb[lid6].roll = 0
    eb[lid7].roll = 0
    eb[lid8].roll = 0

    eb[lid1].parent = head_e
    eb[lid2].parent = head_e
    eb[lid3].parent = head_e
    eb[lid4].parent = head_e
    eb[lid5].parent = head_e
    eb[lid6].parent = head_e
    eb[lid7].parent = head_e
    eb[lid8].parent = head_e

    lower_lid_ctrl = copy_bone_simple(obj.data, definitions[10], base_names[definitions[10]]).name
    upper_lid_ctrl = copy_bone_simple(obj.data, definitions[11], base_names[definitions[11]]).name
    eb[lower_lid_ctrl].parent = head_e
    eb[upper_lid_ctrl].parent = head_e
    distance = (eb[lower_lid_ctrl].head - eb[upper_lid_ctrl].head).length


    bpy.ops.object.mode_set(mode='OBJECT')

    # Axis locks
    pb[lower_lid_ctrl].lock_location = True, False, True
    pb[upper_lid_ctrl].lock_location = True, False, True

    # Add eye close action if it doesn't already exist
    action_name = "eye_close"
    if action_name in bpy.data.actions:
        close_action = bpy.data.actions[action_name]
    else:
        close_action = add_action(name=action_name)

    # Add close property (useful when making the animation in the action)
    prop_name = "close_action"
    prop = rna_idprop_ui_prop_get(pb[upper_lid_ctrl], prop_name, create=True)
    pb[upper_lid_ctrl][prop_name] = 1.0
    prop["soft_min"] = 0.0
    prop["soft_max"] = 1.0
    prop["min"] = 0.0
    prop["max"] = 1.0

    close_driver_path = pb[upper_lid_ctrl].path_to_id() + '["close_action"]'

    # Constraints

    # Flowers track lid controls
    con = pb[flo1].constraints.new('DAMPED_TRACK')
    con.target = obj
    con.subtarget = lid1

    con = pb[flo2].constraints.new('DAMPED_TRACK')
    con.target = obj
    con.subtarget = lid2

    con = pb[flo3].constraints.new('DAMPED_TRACK')
    con.target = obj
    con.subtarget = lid3

    con = pb[flo4].constraints.new('DAMPED_TRACK')
    con.target = obj
    con.subtarget = lid4

    con = pb[flo5].constraints.new('DAMPED_TRACK')
    con.target = obj
    con.subtarget = lid5

    con = pb[flo6].constraints.new('DAMPED_TRACK')
    con.target = obj
    con.subtarget = lid6

    con = pb[flo7].constraints.new('DAMPED_TRACK')
    con.target = obj
    con.subtarget = lid7

    con = pb[flo8].constraints.new('DAMPED_TRACK')
    con.target = obj
    con.subtarget = lid8


    # ORG bones to flower lids
    con = pb[definitions[2]].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = flid1

    con = pb[definitions[3]].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = flid2

    con = pb[definitions[4]].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = flid3

    con = pb[definitions[5]].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = flid4

    con = pb[definitions[6]].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = flid5

    con = pb[definitions[7]].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = flid6

    con = pb[definitions[8]].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = flid7

    con = pb[definitions[9]].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = flid8


    # Action constraints, upper lid
    con = pb[lid1].constraints.new('ACTION')
    con.target = obj
    con.subtarget = upper_lid_ctrl
    con.action = close_action
    con.transform_channel = 'LOCATION_Y'
    con.start_frame = -30
    con.end_frame = 30
    con.minimum = -distance*2
    con.maximum = distance
    con.target_space = 'LOCAL'
    fcurve = con.driver_add("influence", 0)
    driver = fcurve.driver
    driver.type = 'AVERAGE'
    var = driver.variables.new()
    var.targets[0].id_type = 'OBJECT'
    var.targets[0].id = obj
    var.targets[0].data_path = close_driver_path


    con = pb[lid2].constraints.new('ACTION')
    con.target = obj
    con.subtarget = upper_lid_ctrl
    con.action = close_action
    con.transform_channel = 'LOCATION_Y'
    con.start_frame = -30
    con.end_frame = 30
    con.minimum = -distance*2
    con.maximum = distance
    con.target_space = 'LOCAL'
    fcurve = con.driver_add("influence", 0)
    driver = fcurve.driver
    driver.type = 'AVERAGE'
    var = driver.variables.new()
    var.targets[0].id_type = 'OBJECT'
    var.targets[0].id = obj
    var.targets[0].data_path = close_driver_path

    con = pb[lid3].constraints.new('ACTION')
    con.target = obj
    con.subtarget = upper_lid_ctrl
    con.action = close_action
    con.transform_channel = 'LOCATION_Y'
    con.start_frame = -30
    con.end_frame = 30
    con.minimum = -distance*2
    con.maximum = distance
    con.target_space = 'LOCAL'
    fcurve = con.driver_add("influence", 0)
    driver = fcurve.driver
    driver.type = 'AVERAGE'
    var = driver.variables.new()
    var.targets[0].id_type = 'OBJECT'
    var.targets[0].id = obj
    var.targets[0].data_path = close_driver_path

    con = pb[lid4].constraints.new('ACTION')
    con.target = obj
    con.subtarget = upper_lid_ctrl
    con.action = close_action
    con.transform_channel = 'LOCATION_Y'
    con.start_frame = -30
    con.end_frame = 30
    con.minimum = -distance*2
    con.maximum = distance
    con.target_space = 'LOCAL'
    fcurve = con.driver_add("influence", 0)
    driver = fcurve.driver
    driver.type = 'AVERAGE'
    var = driver.variables.new()
    var.targets[0].id_type = 'OBJECT'
    var.targets[0].id = obj
    var.targets[0].data_path = close_driver_path

    con = pb[lid5].constraints.new('ACTION')
    con.target = obj
    con.subtarget = upper_lid_ctrl
    con.action = close_action
    con.transform_channel = 'LOCATION_Y'
    con.start_frame = -30
    con.end_frame = 30
    con.minimum = -distance*2
    con.maximum = distance
    con.target_space = 'LOCAL'
    fcurve = con.driver_add("influence", 0)
    driver = fcurve.driver
    driver.type = 'AVERAGE'
    var = driver.variables.new()
    var.targets[0].id_type = 'OBJECT'
    var.targets[0].id = obj
    var.targets[0].data_path = close_driver_path

    # Action constraints, lower lid
    con = pb[lid5].constraints.new('ACTION')
    con.target = obj
    con.subtarget = lower_lid_ctrl
    con.action = close_action
    con.transform_channel = 'LOCATION_Y'
    con.start_frame = -30
    con.end_frame = 30
    con.minimum = -distance
    con.maximum = distance*2
    con.target_space = 'LOCAL'
    fcurve = con.driver_add("influence", 0)
    driver = fcurve.driver
    driver.type = 'AVERAGE'
    var = driver.variables.new()
    var.targets[0].id_type = 'OBJECT'
    var.targets[0].id = obj
    var.targets[0].data_path = close_driver_path

    con = pb[lid6].constraints.new('ACTION')
    con.target = obj
    con.subtarget = lower_lid_ctrl
    con.action = close_action
    con.transform_channel = 'LOCATION_Y'
    con.start_frame = -30
    con.end_frame = 30
    con.minimum = -distance
    con.maximum = distance*2
    con.target_space = 'LOCAL'
    fcurve = con.driver_add("influence", 0)
    driver = fcurve.driver
    driver.type = 'AVERAGE'
    var = driver.variables.new()
    var.targets[0].id_type = 'OBJECT'
    var.targets[0].id = obj
    var.targets[0].data_path = close_driver_path

    con = pb[lid7].constraints.new('ACTION')
    con.target = obj
    con.subtarget = lower_lid_ctrl
    con.action = close_action
    con.transform_channel = 'LOCATION_Y'
    con.start_frame = -30
    con.end_frame = 30
    con.minimum = -distance
    con.maximum = distance*2
    con.target_space = 'LOCAL'
    fcurve = con.driver_add("influence", 0)
    driver = fcurve.driver
    driver.type = 'AVERAGE'
    var = driver.variables.new()
    var.targets[0].id_type = 'OBJECT'
    var.targets[0].id = obj
    var.targets[0].data_path = close_driver_path

    con = pb[lid8].constraints.new('ACTION')
    con.target = obj
    con.subtarget = lower_lid_ctrl
    con.action = close_action
    con.transform_channel = 'LOCATION_Y'
    con.start_frame = -30
    con.end_frame = 30
    con.minimum = -distance
    con.maximum = distance*2
    con.target_space = 'LOCAL'
    fcurve = con.driver_add("influence", 0)
    driver = fcurve.driver
    driver.type = 'AVERAGE'
    var = driver.variables.new()
    var.targets[0].id_type = 'OBJECT'
    var.targets[0].id = obj
    var.targets[0].data_path = close_driver_path

    con = pb[lid1].constraints.new('ACTION')
    con.target = obj
    con.subtarget = lower_lid_ctrl
    con.action = close_action
    con.transform_channel = 'LOCATION_Y'
    con.start_frame = -30
    con.end_frame = 30
    con.minimum = -distance
    con.maximum = distance*2
    con.target_space = 'LOCAL'
    fcurve = con.driver_add("influence", 0)
    driver = fcurve.driver
    driver.type = 'AVERAGE'
    var = driver.variables.new()
    var.targets[0].id_type = 'OBJECT'
    var.targets[0].id = obj
    var.targets[0].data_path = close_driver_path




    # Set layers
    layer = list(bb[definitions[2]].layer)
    bb[lid1].layer = layer
    bb[lid2].layer = layer
    bb[lid3].layer = layer
    bb[lid4].layer = layer
    bb[lid5].layer = layer
    bb[lid6].layer = layer
    bb[lid7].layer = layer
    bb[lid8].layer = layer


    return (None,)




def main(obj, bone_definition, base_names, options):
    # Create control rig
    control(obj, bone_definition, base_names, options)
    # Create deform rig
    deform(obj, bone_definition, base_names, options)

    return (None,)




def make_lid_stretch_bone(obj, name, bone1, bone2, roll_alpha):
    eb = obj.data.edit_bones
    pb = obj.pose.bones

    # Create the bone, pointing from bone1 to bone2
    bone_e = copy_bone_simple(obj.data, bone1, name, parent=True)
    bone_e.connected = False
    bone_e.tail = eb[bone2].head
    bone = bone_e.name

    # Align the bone roll with the average direction of bone1 and bone2
    vec = bone_e.y_axis.cross(((1.0-roll_alpha)*eb[bone1].y_axis) + (roll_alpha*eb[bone2].y_axis)).normalize()

    ang = acos(vec * bone_e.x_axis)

    bone_e.roll += ang
    c1 = vec * bone_e.x_axis
    bone_e.roll -= (ang*2)
    c2 = vec * bone_e.x_axis

    if c1 > c2:
        bone_e.roll += (ang*2)

    bpy.ops.object.mode_set(mode='OBJECT')
    bone_p = pb[bone]

    # Constrains
    con = bone_p.constraints.new('COPY_LOCATION')
    con.target = obj
    con.subtarget = bone1

    con = bone_p.constraints.new('DAMPED_TRACK')
    con.target = obj
    con.subtarget = bone2

    con = bone_p.constraints.new('STRETCH_TO')
    con.target = obj
    con.subtarget = bone2
    con.volume = 'NO_VOLUME'

    bpy.ops.object.mode_set(mode='EDIT')

    return bone
