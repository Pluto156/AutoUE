import bpy
import os
import math
from mathutils import Vector
import shutil
from dotenv import load_dotenv
load_dotenv()
# ==================================================
# Configuration
# ==================================================
OUTPUT_ROOT = os.getenv("OUTPUT_DIR")

START_ANGLE = 30
DELTA_ANGLE = 120
NUM_SHOTS = 4
RENDER_SIZE = 1024


# ==================================================
# Utility Functions
# ==================================================
def clear_scene():
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    bpy.ops.outliner.orphans_purge(do_recursive=True)


def find_gltf_file(folder):
    """Recursively find the first .gltf / .glb file under the given folder."""
    for root, dirs, files in os.walk(folder):
        for f in files:
            if f.lower().endswith((".gltf", ".glb")):
                return os.path.join(root, f)
    return None


def set_render_engine(scene):
    engines = scene.render.bl_rna.properties["engine"].enum_items.keys()
    if "BLENDER_EEVEE_NEXT" in engines:
        scene.render.engine = "BLENDER_EEVEE_NEXT"
    elif "BLENDER_EEVEE" in engines:
        scene.render.engine = "BLENDER_EEVEE"
    else:
        scene.render.engine = "CYCLES"


# ==================================================
# Process a Single Model
# ==================================================
def process_one_model(demo_name, model_uid, gltf_path, output_dir):
    print(f"[{demo_name}][{model_uid}] Processing model")

    clear_scene()

    scene = bpy.context.scene
    set_render_engine(scene)
    scene.render.resolution_x = RENDER_SIZE
    scene.render.resolution_y = RENDER_SIZE
    scene.render.film_transparent = True
    scene.view_settings.view_transform = 'Standard'

    # -----------------------------
    # Import glTF file
    # -----------------------------
    bpy.ops.import_scene.gltf(filepath=gltf_path)
    bpy.context.view_layer.update()

    mesh_objects = [o for o in bpy.data.objects if o.type == 'MESH']
    if not mesh_objects:
        print(f"[{model_uid}] No mesh found")
        return

    # -----------------------------
    # Compute Axis-Aligned Bounding Box (AABB)
    # -----------------------------
    verts = []
    for obj in mesh_objects:
        for v in obj.data.vertices:
            verts.append(obj.matrix_world @ v.co)

    min_corner = Vector((
        min(v.x for v in verts),
        min(v.y for v in verts),
        min(v.z for v in verts)
    ))
    max_corner = Vector((
        max(v.x for v in verts),
        max(v.y for v in verts),
        max(v.z for v in verts)
    ))

    center = (min_corner + max_corner) / 2
    radius = (max_corner - min_corner).length / 2
    height = max_corner.z - min_corner.z

    # -----------------------------
    # Create target (Empty object)
    # -----------------------------
    target = bpy.data.objects.new(f"Target_{model_uid}", None)
    bpy.context.collection.objects.link(target)
    target.location = center

    # -----------------------------
    # Create camera
    # -----------------------------
    cam_data = bpy.data.cameras.new(f"Cam_{model_uid}")
    cam = bpy.data.objects.new(f"Cam_{model_uid}", cam_data)
    bpy.context.collection.objects.link(cam)

    constraint = cam.constraints.new(type='TRACK_TO')
    constraint.target = target
    constraint.track_axis = 'TRACK_NEGATIVE_Z'
    constraint.up_axis = 'UP_Y'

    scene.camera = cam

    # -----------------------------
    # Create light (attached to camera)
    # -----------------------------
    light_data = bpy.data.lights.new(f"CamLight_{model_uid}", type="POINT")
    light_data.energy = 1600
    light_data.shadow_soft_size = 0.5
    light_data.use_custom_distance = True
    light_data.cutoff_distance = radius * 6

    light = bpy.data.objects.new(f"CamLight_{model_uid}", light_data)
    bpy.context.collection.objects.link(light)
    light.parent = cam
    light.location = (0, 0, 0)

    # -----------------------------
    # Output directory (one folder per model)
    # -----------------------------
    os.makedirs(output_dir, exist_ok=True)

    distance = radius * 3 + height / 2

    # -----------------------------
    # Render multiple views
    # -----------------------------
    for i in range(NUM_SHOTS):
        angle = START_ANGLE + i * DELTA_ANGLE
        rad = math.radians(angle)

        cam.location = (
            center.x + math.sin(rad) * distance,
            center.y + math.cos(rad) * distance,
            center.z
        )

        scene.render.filepath = os.path.join(output_dir, f"view_{i}.png")
        bpy.ops.render.render(write_still=True)

        print(f"[{model_uid}] Rendered view {i}")


def process_one_demo(demo_name, demo_dir):
    gltf_root = os.path.join(demo_dir, "gltf")
    if not os.path.exists(gltf_root):
        print(f"[{demo_name}] gltf folder not found")
        return

    output_models_root = os.path.join(
        demo_dir, "MyPCG", "eval", "models"
    )

    if os.path.exists(output_models_root):
        print(f"[{demo_name}] Removing old models folder: {output_models_root}")
        shutil.rmtree(output_models_root)

    os.makedirs(output_models_root, exist_ok=True)

    # ==========================================================
    # Process each model
    # ==========================================================
    for model_uid in os.listdir(gltf_root):
        model_dir = os.path.join(gltf_root, model_uid)
        if not os.path.isdir(model_dir):
            continue

        extracted_dir = os.path.join(model_dir, "extracted")
        if not os.path.exists(extracted_dir):
            print(f"[{model_uid}] extracted folder missing")
            continue

        gltf_path = find_gltf_file(extracted_dir)
        if not gltf_path:
            print(f"[{model_uid}] No glTF file found")
            continue

        model_output_dir = os.path.join(output_models_root, model_uid)
        process_one_model(
            demo_name,
            model_uid,
            gltf_path,
            model_output_dir
        )


# ==================================================
# Main
# ==================================================
def main():
    if not os.path.exists(OUTPUT_ROOT):
        print("OUTPUT_ROOT not found")
        return

    for demo_name in os.listdir(OUTPUT_ROOT):
        demo_dir = os.path.join(OUTPUT_ROOT, demo_name)
        if not os.path.isdir(demo_dir):
            continue

        print(f"\n===== Processing {demo_name} =====")
        process_one_demo(demo_name, demo_dir)

    print("\n====== ALL DEMOS FINISHED ======")


def render_gltf():
    if not os.path.exists(OUTPUT_ROOT):
        print("OUTPUT_ROOT not found")
        return

    for demo_name in os.listdir(OUTPUT_ROOT):
        demo_dir = os.path.join(OUTPUT_ROOT, demo_name)
        if not os.path.isdir(demo_dir):
            continue

        print(f"\n===== Processing {demo_name} =====")
        process_one_demo(demo_name, demo_dir)

    print("\n====== ALL DEMOS FINISHED ======")


# ==================================================
# Blender Entry Point
# ==================================================
if __name__ == "__main__":
    main()