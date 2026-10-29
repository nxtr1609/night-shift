# Chuẩn bị mẫu "Ghost Girl Animated" (Shaban Hafizsalim, CC BY 4.0) cho Unreal:
# - bỏ camera / đèn / empty của Cinema 4D
# - váy (gắn cứng vào xương Spine) và tóc (gắn vào HeadTop_End qua empty) → lưới có da, trọng số 100% một xương
# - hoạt ảnh zombie walk: bỏ phần hông trôi ngang (đi tại chỗ, game tự dời nhân vật)
# - texture .dds → .png đi kèm
# Chạy: blender -b --factory-startup --python lam_bong_ma.py -- <shabannn.fbx> <thư mục textures png> <file .fbx ra>
import bpy, sys, os

a = sys.argv[sys.argv.index("--") + 1:]
VAO, ANH, RA = a[0], a[1], a[2]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=VAO)
arm = bpy.data.objects["zombie_Walk"]
dg = bpy.context.evaluated_depsgraph_get()

# Tính mọi thứ ở tư thế gốc
arm.data.pose_position = "REST"
bpy.context.view_layer.update()


def thanh_luoi_da(ten, xuong):
    o = bpy.data.objects[ten]
    m = o.matrix_world.copy()
    o.parent = None
    o.matrix_world = m
    bpy.context.view_layer.update()
    # nướng biến đổi vào lưới
    o.data.transform(o.matrix_world)
    o.matrix_world = arm.matrix_world.copy()
    o.data.transform(arm.matrix_world.inverted())
    o.parent = arm
    o.parent_type = "OBJECT"
    o.matrix_parent_inverse.identity()
    o.matrix_world = arm.matrix_world.copy()
    g = o.vertex_groups.new(name=xuong)
    g.add(range(len(o.data.vertices)), 1.0, "REPLACE")
    md = o.modifiers.new("Armature", "ARMATURE")
    md.object = arm


thanh_luoi_da("elbise", "mixamorig_Spine")
thanh_luoi_da("saclar_2", "mixamorig_Head")
for o in list(bpy.data.objects):
    if o.type not in {"MESH", "ARMATURE"}:
        bpy.data.objects.remove(o, do_unlink=True)

# Gộp váy + tóc vào lưới thân (Unreal nhập sai lưới riêng chỉ gắn vào một xương: váy biến mất)
bpy.ops.object.select_all(action="DESELECT")
for t in ("elbise", "saclar_2", "vucut"):
    bpy.data.objects[t].select_set(True)
bpy.context.view_layer.objects.active = bpy.data.objects["vucut"]
bpy.ops.object.join()
print("GOP", [o.name for o in bpy.data.objects if o.type == "MESH"])

# Kiểm tra: ở tư thế gốc, lưới váy / tóc không được xê dịch so với lúc nhập
arm.data.pose_position = "POSE"

# Hoạt ảnh: xóa trôi ngang của hông (giữ nhấp nhô lên xuống)
act = arm.animation_data.action if arm.animation_data else None
if act:
    duong = 'pose.bones["mixamorig_Hips"].location'
    fcs = [fc for fc in getattr(act, "fcurves", []) if fc.data_path == duong]
    if not fcs:
        # Blender 5.x: hoạt ảnh nằm trong layer/strip/channelbag
        for layer in act.layers:
            for strip in layer.strips:
                for cb in strip.channelbags:
                    fcs += [fc for fc in cb.fcurves if fc.data_path == duong]
    for fc in fcs:
        gt = [k.co[1] for k in fc.keyframe_points]
        if not gt:
            continue
        bien = max(gt) - min(gt)
        print("HONG truc", fc.array_index, "bien do", round(bien, 3))
        # trục trôi = biên độ lớn và đơn điệu (đầu-cuối chênh nhiều)
        if abs(gt[-1] - gt[0]) > 0.5 * bien and bien > 5.0:
            for k in fc.keyframe_points:
                k.co[1] = gt[0]
                k.handle_left[1] = gt[0]
                k.handle_right[1] = gt[0]
            print("  -> bo troi truc", fc.array_index)

# Nướng đủ độ dài hoạt ảnh (mặc định cảnh chỉ 250 khung)
if act:
    bpy.context.scene.frame_start = int(act.frame_range[0])
    bpy.context.scene.frame_end = int(act.frame_range[1])

# Texture: dùng bản PNG
for img in bpy.data.images:
    ten = os.path.splitext(os.path.basename(img.filepath))[0]
    png = os.path.join(ANH, ten + ".png")
    if os.path.exists(png):
        img.filepath = png
        img.reload()
        print("ANH", ten)

bpy.ops.export_scene.fbx(filepath=RA, use_selection=False, object_types={"ARMATURE", "MESH"},
                         add_leaf_bones=False, bake_anim=True, bake_anim_use_all_actions=False,
                         bake_anim_use_nla_strips=False, path_mode="COPY", embed_textures=True,
                         mesh_smooth_type="FACE", use_armature_deform_only=True)
print("XONG", RA)
