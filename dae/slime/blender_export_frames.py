"""Export per-frame OBJs of the simulated liquid mesh.

How to run:
1. In Blender, click the Liquid Domain in the 3D viewport (the object that
   visibly changes shape across frames).
2. Switch to the 'Scripting' workspace tab at the top of Blender.
3. In the text editor pane, click 'Open' and pick this file. Or paste it in.
4. Click 'Run Script' (or press Alt+P with the cursor in the editor).
5. Watch the system console / info area — it will print one line per frame.
"""

import bpy
import os

OUT_DIR = "/Users/seantai/Desktop/slime184/dae/slime/frames"
START_FRAME = 1
END_FRAME = 41

scene = bpy.context.scene
target = bpy.context.active_object
if target is None:
    raise RuntimeError(
        "No active object selected. Click the Liquid Domain in the viewport "
        "before running this script."
    )

print(f"exporting object: {target.name}")
os.makedirs(OUT_DIR, exist_ok=True)

bpy.ops.object.select_all(action='DESELECT')
target.select_set(True)
bpy.context.view_layer.objects.active = target

for f in range(START_FRAME, END_FRAME + 1):
    scene.frame_set(f)
    out_path = os.path.join(OUT_DIR, f"slime_{f:04d}.obj")
    bpy.ops.wm.obj_export(
        filepath=out_path,
        export_selected_objects=True,
        forward_axis='NEGATIVE_Z',
        up_axis='Y',
        apply_modifiers=True,
        export_triangulated_mesh=True,
        export_materials=False,
    )
    print(f"  frame {f:02d} -> {out_path}")

print(f"done. {END_FRAME - START_FRAME + 1} frames exported to {OUT_DIR}")
