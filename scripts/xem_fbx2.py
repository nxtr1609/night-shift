import bpy, sys, math, mathutils
f = sys.argv[sys.argv.index("--") + 1]
ra = sys.argv[sys.argv.index("--") + 2]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=f)
for o in bpy.data.objects:
    if o.type in {"MESH", "EMPTY"}:
        print("P", o.name, o.parent_type, o.parent_bone, [m.type for m in o.modifiers])
arm = bpy.data.objects["zombie_Walk"]
for n in ["CINEMA_4D_Editor", "Light", "Light_1"]:
    if n in bpy.data.objects: bpy.data.objects.remove(bpy.data.objects[n])
sc = bpy.context.scene
sc.frame_set(1)
sc.render.engine = "BLENDER_EEVEE"
sc.render.resolution_x, sc.render.resolution_y = 900, 900
cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam")); sc.collection.objects.link(cam); sc.camera = cam
cam.location = (0, -3.2, 1.0); cam.rotation_euler = (math.radians(88), 0, 0)
for i, (loc, e) in enumerate([((2, -3, 3), 1000), ((-2, -2, 2), 500), ((0, 2, 3), 600)]):
    l = bpy.data.objects.new("l%d" % i, bpy.data.lights.new("l%d" % i, "POINT")); l.data.energy = e; l.location = loc; sc.collection.objects.link(l)
w = bpy.data.worlds.new("w"); sc.world = w
for pf, fr in (("rest", None), ("f60", 60)):
    arm.data.pose_position = "REST" if fr is None else "POSE"
    if fr: sc.frame_set(fr)
    sc.render.filepath = ra + "_" + pf + ".png"
    bpy.ops.render.render(write_still=True)
