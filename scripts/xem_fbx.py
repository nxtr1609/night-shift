import bpy, sys, mathutils
f = sys.argv[sys.argv.index("--") + 1]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=f)
for o in bpy.data.objects:
    print("OBJ", o.name, o.type, "parent", o.parent.name if o.parent else "-", "dims", tuple(round(v, 3) for v in o.dimensions), "scale", tuple(round(v, 3) for v in o.scale))
    if o.type == "MESH":
        print("   tris", sum(len(p.vertices) - 2 for p in o.data.polygons), "groups", len(o.vertex_groups), "mats", [m.name for m in o.data.materials])
    if o.type == "ARMATURE":
        print("   bones", len(o.data.bones))
        for b in o.data.bones:
            print("    B", b.name, "<-", b.parent.name if b.parent else "-", tuple(round(v, 3) for v in (o.matrix_world @ b.head_local)))
for a in bpy.data.actions:
    print("ACTION", a.name, a.frame_range[:])
