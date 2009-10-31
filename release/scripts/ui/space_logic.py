# This software is distributable under the terms of the GNU
# General Public License (GPL) v2, the text of which can be found at
# http://www.gnu.org/copyleft/gpl.html. Installing, importing or otherwise
# using this module constitutes acceptance of the terms of this License.

# <pep8 compliant>
import bpy


class LOGIC_PT_properties(bpy.types.Panel):
    bl_space_type = 'LOGIC_EDITOR'
    bl_region_type = 'UI'
    bl_label = "Properties"

    def poll(self, context):
        ob = context.active_object
        return ob and ob.game

    def draw(self, context):
        layout = self.layout

        ob = context.active_object
        game = ob.game

        layout.itemO("object.game_property_new", text="Add Game Property")

        for i, prop in enumerate(game.properties):

            row = layout.row(align=True)
            row.itemR(prop, "name", text="")
            row.itemR(prop, "type", text="")
            row.itemR(prop, "value", text="", toggle=True) # we dont care about the type. rna will display correctly
            row.itemR(prop, "debug", text="", toggle=True, icon='ICON_INFO')
            row.item_intO("object.game_property_remove", "index", i, text="", icon='ICON_X')

bpy.types.register(LOGIC_PT_properties)
