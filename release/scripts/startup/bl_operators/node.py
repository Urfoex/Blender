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

# <pep8-80 compliant>

import bpy
from bpy.types import Operator
from bpy.props import EnumProperty, StringProperty

# Base class for node 'Add' operators
class NodeAddOperator():
    @staticmethod
    def store_mouse_cursor(context, event):
        space = context.space_data
        v2d = context.region.view2d

        # convert mouse position to the View2D for later node placement
        space.cursor_location = v2d.region_to_view(event.mouse_region_x,
                                                   event.mouse_region_y)

    def create_node(self, context, node_type):
        space = context.space_data
        tree = space.edit_tree

        node = tree.nodes.new(type=node_type)

        # select only the new node
        for n in tree.nodes:
            n.select = (n == node)
        tree.nodes.active = node
        node.location = space.cursor_location
        return node

    @classmethod
    def poll(cls, context):
        space = context.space_data
        # needs active node editor and a tree to add nodes to
        return (space.type == 'NODE_EDITOR' and space.edit_tree)

    # Default invoke stores the mouse position to place the node correctly
    def invoke(self, context, event):
        self.store_mouse_cursor(context, event)
        return self.execute(context)


# Simple basic operator for adding a node
class NODE_OT_add_node(NodeAddOperator, Operator):
    '''Add a node to the active tree'''
    bl_idname = "node.add_node"
    bl_label = "Add Node"

    type = StringProperty(name="Node Type", description="Node type")
    
    # optional group tree parameter for group nodes
    group_tree = StringProperty(name="Group tree", description="Group node tree name")

    def execute(self, context):
        node = self.create_node(context, self.type)

        # set the node group tree of a group node
        if self.properties.is_property_set('group_tree'):
            node.node_tree = bpy.data.node_groups[self.group_tree]

        return {'FINISHED'}


# Adds a node and immediately starts the transform operator for inserting in a tree
class NODE_OT_add_node_move(NODE_OT_add_node):
    '''Add a node to the active tree and start transform'''
    bl_idname = "node.add_node_move"
    bl_label = "Add Node and Move"

    type = StringProperty(name="Node Type", description="Node type")
    
    # optional group tree parameter for group nodes
    group_tree = StringProperty(name="Group tree", description="Group node tree name")

    def invoke(self, context, event):
        self.store_mouse_cursor(context, event)
        self.execute(context)
        return bpy.ops.transform.translate('INVOKE_DEFAULT')


# XXX These node item lists should actually be generated by a callback at
# operator execution time (see node_type_items below),
# using the active node tree from the context.
# Due to a difficult bug in bpy this is not possible
# (item list memory gets freed too early),
# so for now just copy the static item lists to these global variables.
#
# In the custom_nodes branch, the static per-tree-type node items are replaced
# by a single independent type list anyway (with a poll function to limit node
# types to the respective trees). So this workaround is only temporary.

# lazy init
node_type_items_dict = {}

# Prefixes used to distinguish base node types and node groups
node_type_prefix = 'NODE_'
node_group_prefix = 'GROUP_'


# Generate a list of enum items for a given node class
# Copy existing type enum, adding a prefix to distinguish from node groups
# Skip the base node group type,
# node groups will be added below for all existing group trees
def node_type_items(node_class):
    return [(node_type_prefix + item.identifier, item.name, item.description)
            for item in node_class.bl_rna.properties['type'].enum_items
            if item.identifier != 'GROUP']


# Generate items for node group types
# Filter by the given tree_type
# Node group trees don't have a description property yet
# (could add this as a custom property though)
def node_group_items(tree_type):
    return [(node_group_prefix + group.name, group.name, '')
            for group in bpy.data.node_groups if group.type == tree_type]


# Returns the enum item list for the edited tree in the context
def node_type_items_cb(self, context):
    snode = context.space_data
    if not snode:
        return ()
    tree = snode.edit_tree
    if not tree:
        return ()

    # Lists of basic node types for each
    if not node_type_items_dict:
        node_type_items_dict.update({
            'SHADER': node_type_items(bpy.types.ShaderNode),
            'COMPOSITING': node_type_items(bpy.types.CompositorNode),
            'TEXTURE': node_type_items(bpy.types.TextureNode),
            })

    # XXX Does not work correctly, see comment above
    '''
    return [(item.identifier, item.name, item.description, item.value)
            for item in
            tree.nodes.bl_rna.functions['new'].parameters['type'].enum_items]
    '''

    if tree.type in node_type_items_dict:
        return node_type_items_dict[tree.type] + node_group_items(tree.type)
    else:
        return ()


class NODE_OT_add_search(Operator):
    '''Add a node to the active tree'''
    bl_idname = "node.add_search"
    bl_label = "Search and Add Node"
    bl_options = {'REGISTER', 'UNDO'}

    # XXX this should be called 'node_type' but the operator search
    # property is hardcoded to 'type' by a hack in bpy_operator_wrap.c ...
    type = EnumProperty(
            name="Node Type",
            description="Node type",
            items=node_type_items_cb,
            )

    _node_type_items_dict = None

    def create_node(self, context):
        space = context.space_data
        tree = space.edit_tree

        # Enum item identifier has an additional prefix to
        # distinguish base node types from node groups
        item = self.type
        if item.startswith(node_type_prefix):
            # item means base node type
            node = tree.nodes.new(type=item[len(node_type_prefix):])
        elif item.startswith(node_group_prefix):
            # item means node group type
            node = tree.nodes.new(
                    type='GROUP',
                    group=bpy.data.node_groups[item[len(node_group_prefix):]])
        else:
            return None

        for n in tree.nodes:
            if n == node:
                node.select = True
                tree.nodes.active = node
            else:
                node.select = False
        node.location = space.cursor_location
        return node

    @classmethod
    def poll(cls, context):
        space = context.space_data
        # needs active node editor and a tree to add nodes to
        return (space.type == 'NODE_EDITOR' and space.edit_tree)

    def execute(self, context):
        self.create_node(context)
        return {'FINISHED'}

    def invoke(self, context, event):
        space = context.space_data
        v2d = context.region.view2d

        # convert mouse position to the View2D for later node placement
        space.cursor_location = v2d.region_to_view(event.mouse_region_x,
                                                   event.mouse_region_y)

        context.window_manager.invoke_search_popup(self)
        return {'CANCELLED'}


class NODE_OT_collapse_hide_unused_toggle(Operator):
    '''Toggle collapsed nodes and hide unused sockets'''
    bl_idname = "node.collapse_hide_unused_toggle"
    bl_label = "Collapse and Hide Unused Sockets"
    bl_options = {'REGISTER', 'UNDO'}

    @classmethod
    def poll(cls, context):
        space = context.space_data
        # needs active node editor and a tree
        return (space.type == 'NODE_EDITOR' and space.edit_tree)

    def execute(self, context):
        space = context.space_data
        tree = space.edit_tree

        for node in tree.nodes:
            if node.select:
                hide = (not node.hide)

                node.hide = hide
                # Note: connected sockets are ignored internally
                for socket in node.inputs:
                    socket.hide = hide
                for socket in node.outputs:
                    socket.hide = hide

        return {'FINISHED'}
