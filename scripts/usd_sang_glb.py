# Chuyển mẫu SimReady (USD) sang GLB cho Godot.
# Chạy: blender -b --factory-startup --python usd_sang_glb.py -- <thư mục simready> <thư mục models> [tên ...]
import bpy, sys, os, json, mathutils

args = sys.argv[sys.argv.index("--") + 1:]
NGUON, DICH = args[0], args[1]
TEN = args[2:] or sorted(os.listdir(NGUON))
CO_ANH = 1024  # cạnh tối đa texture sau khi thu nhỏ
os.makedirs(DICH, exist_ok=True)
bao_cao = {}

def anh_noi_vao(node, ten_cong):
    # Ảnh nối (trực tiếp hoặc qua một nút trung gian) vào cổng ten_cong
    cong = node.inputs.get(ten_cong)
    if not cong or not cong.is_linked:
        return None
    n = cong.links[0].from_node
    for _ in range(3):
        if n.type == "TEX_IMAGE":
            return n.image
        vao = [i for i in n.inputs if i.is_linked]
        if not vao:
            return None
        n = vao[0].links[0].from_node
    return None


def don_gian_hoa_vat_lieu():
    # Ô vật liệu trống làm exporter glTF hỏng → gán vật liệu xám
    xam = bpy.data.materials.get("xam_mac_dinh") or bpy.data.materials.new("xam_mac_dinh")
    xam.use_nodes = True
    for o in bpy.data.objects:
        if o.type == "MESH":
            if not o.material_slots:
                o.data.materials.append(xam)
            for sl in o.material_slots:
                if sl.material is None:
                    sl.material = xam
    # Dựng lại mọi vật liệu thành Principled BSDF + ảnh màu + ảnh pháp tuyến (bỏ phần exporter không đọc được)
    for m in list(bpy.data.materials):
        if not m.use_nodes:
            continue
        p = next((n for n in m.node_tree.nodes if n.type == "BSDF_PRINCIPLED"), None)
        mau = anh_noi_vao(p, "Base Color") if p else None
        phap = anh_noi_vao(p, "Normal") if p else None
        mau_goc = tuple(p.inputs["Base Color"].default_value) if p else (0.7, 0.7, 0.7, 1)
        nham = float(p.inputs["Roughness"].default_value) if p else 0.6
        kl = float(p.inputs["Metallic"].default_value) if p else 0.0
        nt = m.node_tree
        nt.nodes.clear()
        ra = nt.nodes.new("ShaderNodeOutputMaterial")
        b = nt.nodes.new("ShaderNodeBsdfPrincipled")
        b.inputs["Base Color"].default_value = mau_goc
        b.inputs["Roughness"].default_value = nham
        b.inputs["Metallic"].default_value = kl
        nt.links.new(b.outputs["BSDF"], ra.inputs["Surface"])
        if mau:
            t = nt.nodes.new("ShaderNodeTexImage"); t.image = mau
            nt.links.new(t.outputs["Color"], b.inputs["Base Color"])
        if phap:
            t = nt.nodes.new("ShaderNodeTexImage"); t.image = phap
            t.image.colorspace_settings.name = "Non-Color"
            nm = nt.nodes.new("ShaderNodeNormalMap")
            nt.links.new(t.outputs["Color"], nm.inputs["Color"])
            nt.links.new(nm.outputs["Normal"], b.inputs["Normal"])


def xoa_canh():
    bpy.ops.wm.read_factory_settings(use_empty=True)

for ten in TEN:
    thu_muc = os.path.join(NGUON, ten, "simready_usd")
    if not os.path.isdir(thu_muc):
        continue
    goc = [f for f in os.listdir(thu_muc) if f.startswith("sm_") and f.endswith((".usd", ".usda"))]
    if not goc:
        print("THIEU USD", ten); continue
    xoa_canh()
    bpy.ops.wm.usd_import(filepath=os.path.join(thu_muc, goc[0]), import_guide=False,
                          import_proxy=False, import_render=True, import_cameras=False,
                          import_lights=False, apply_unit_conversion_scale=True)
    # bỏ vật không phải lưới hiển thị (va chạm vật lý, khung rỗng giữ lại làm cha)
    for o in list(bpy.data.objects):
        n = o.name.lower()
        if o.type == "MESH" and ("collision" in n or "collider" in n or "_coll" in n or o.display_type in {"WIRE", "BOUNDS"} or o.hide_render):
            bpy.data.objects.remove(o, do_unlink=True)
    for img in bpy.data.images:
        if img.size[0] > CO_ANH or img.size[1] > CO_ANH:
            k = CO_ANH / max(img.size)
            img.scale(max(1, int(img.size[0] * k)), max(1, int(img.size[1] * k)))
    # gộp mọi lưới thành một: lấy từng thể hiện đã tính (kể cả instance USD), tạo lưới thật đặt đúng chỗ, rồi join
    dg = bpy.context.evaluated_depsgraph_get()
    that = []
    for inst in dg.object_instances:
        ob = inst.object
        if ob.type != "MESH":
            continue
        me = bpy.data.meshes.new_from_object(ob, preserve_all_data_layers=True, depsgraph=dg)
        if not me.materials and ob.material_slots:
            for sl in ob.material_slots:
                me.materials.append(sl.material)
        moi_ob = bpy.data.objects.new("gop", me)
        moi_ob.matrix_world = inst.matrix_world.copy()
        that.append(moi_ob)
    giu = {o.name for o in that}
    for o in list(bpy.data.objects):
        if o.name not in giu:
            bpy.data.objects.remove(o, do_unlink=True)
    for o in that:
        bpy.context.scene.collection.objects.link(o)
    if len(that) > 1:
        bpy.ops.object.select_all(action="DESELECT")
        for o in that:
            o.select_set(True)
        bpy.context.view_layer.objects.active = that[0]
        bpy.ops.object.join()
    print("SO_LUOI", ten, len(that), len([o for o in bpy.data.objects if o.type == "MESH"]), flush=True)
    # hộp bao ở toạ độ thế giới
    bpy.context.view_layer.update()
    lo = mathutils.Vector((1e9,) * 3); hi = mathutils.Vector((-1e9,) * 3)
    so_tam_giac = 0
    for o in bpy.data.objects:
        if o.type != "MESH": continue
        so_tam_giac += sum(len(p.vertices) - 2 for p in o.data.polygons)
        for c in o.bound_box:
            w = o.matrix_world @ mathutils.Vector(c)
            lo = mathutils.Vector(map(min, lo, w)); hi = mathutils.Vector(map(max, hi, w))
    ra = os.path.join(DICH, ten + ".glb")
    def xuat():
        bpy.ops.export_scene.gltf(filepath=ra, export_format="GLB", export_image_format="JPEG",
                                  export_jpeg_quality=88, export_apply=True, export_yup=True)
    try:
        xuat()
    except Exception as loi:
        print("XUAT LOI, dung lai vat lieu don gian:", ten, loi, flush=True)
        don_gian_hoa_vat_lieu()
        try:
            xuat()
        except Exception as loi2:
            print("BO QUA", ten, loi2, flush=True)
            continue
    bao_cao[ten] = {"kich_thuoc_blender_xyz": [round(v, 3) for v in (hi - lo)],
                    "tam_giac": so_tam_giac, "mb": round(os.path.getsize(ra) / 1e6, 2)}
    print("OK", ten, bao_cao[ten], flush=True)

with open(os.path.join(DICH, "_bao_cao.json"), "a", encoding="utf-8") as f:
    json.dump(bao_cao, f, ensure_ascii=False, indent=1)
