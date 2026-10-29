# Tạo vật liệu, bản đồ trống; nhập texture (NguonMau/anh), mẫu đồ vật (NguonMau/do_vat/*.glb), âm thanh (NguonAmThanh/*.wav).
# Chạy: UnrealEditor-Cmd.exe <du_an>.uproject -run=pythonscript -script="<file nay>" -unattended -nosplash
import os
import unreal

at = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
DU_AN = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NGUON = os.path.join(DU_AN, "NguonMau")
NHAT_KY = []
# Âm thanh lặp vòng (nền, nhịp tim, chuông điện thoại...)
LAP = {"nhen_bo", "nen_den_huynh_quang", "nen_mua", "nen_u_am_1", "nen_u_am_2", "nhip_tim", "duoi_bat", "binh_minh", "dong_ho", "dien_thoai_reo"}


def ghi(s):
    unreal.log(s)
    NHAT_KY.append(s)


def bieu_thuc(m, lop, x, y, **tc):
    e = mel.create_material_expression(m, lop, x, y)
    for k, v in tc.items():
        e.set_editor_property(k, v)
    return e


def vat_lieu_moi(ten):
    duong = "/Game/VatLieu/" + ten
    if eal.does_asset_exist(duong):
        return None
    return at.create_asset(ten, "/Game/VatLieu", unreal.Material, unreal.MaterialFactoryNew())


def xong(m):
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    ghi("Da tao vat lieu " + m.get_name())


def nhap(file, dich, ten=None):
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", file)
    t.set_editor_property("destination_path", dich)
    if ten:
        t.set_editor_property("destination_name", ten)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", True)
    at.import_asset_tasks([t])
    ghi("Nhap %s -> %s" % (os.path.basename(file), dich))


# ---------- Texture ----------
for f in sorted(os.listdir(os.path.join(NGUON, "anh"))):
    ten = os.path.splitext(f)[0]
    if f.endswith(".png") and not eal.does_asset_exist("/Game/Anh/T_" + ten):
        nhap(os.path.join(NGUON, "anh", f), "/Game/Anh", "T_" + ten)

TEX_MAC_DINH = eal.load_asset("/Game/Anh/T_tuong")

# ---------- Vật liệu ----------
# M_Mau: màu phẳng (khối phụ, khung)
m = vat_lieu_moi("M_Mau")
if m:
    mau = bieu_thuc(m, unreal.MaterialExpressionVectorParameter, -400, 0, parameter_name="MauSac", default_value=unreal.LinearColor(0.7, 0.7, 0.7, 1))
    nham = bieu_thuc(m, unreal.MaterialExpressionScalarParameter, -400, 200, parameter_name="DoNham", default_value=0.6)
    kl = bieu_thuc(m, unreal.MaterialExpressionScalarParameter, -400, 300, parameter_name="KimLoai", default_value=0.0)
    ps = bieu_thuc(m, unreal.MaterialExpressionVectorParameter, -400, 400, parameter_name="PhatSang", default_value=unreal.LinearColor(0, 0, 0, 1))
    mel.connect_material_property(mau, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(nham, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(kl, "", unreal.MaterialProperty.MP_METALLIC)
    mel.connect_material_property(ps, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    xong(m)

# M_BeMat: texture x màu nhân (tường, sàn, trần, cửa) + phát sáng theo texture (biển, mặt tiền)
m = vat_lieu_moi("M_BeMat")
if m:
    tex = bieu_thuc(m, unreal.MaterialExpressionTextureSampleParameter2D, -700, 0, parameter_name="Anh", texture=TEX_MAC_DINH)
    nhan = bieu_thuc(m, unreal.MaterialExpressionVectorParameter, -700, 300, parameter_name="Nhan", default_value=unreal.LinearColor(1, 1, 1, 1))
    nham = bieu_thuc(m, unreal.MaterialExpressionScalarParameter, -400, 400, parameter_name="DoNham", default_value=0.7)
    ps = bieu_thuc(m, unreal.MaterialExpressionScalarParameter, -700, 500, parameter_name="PhatSang", default_value=0.0)
    x1 = bieu_thuc(m, unreal.MaterialExpressionMultiply, -400, 0)
    mel.connect_material_expressions(tex, "RGB", x1, "A")
    mel.connect_material_expressions(nhan, "", x1, "B")
    x2 = bieu_thuc(m, unreal.MaterialExpressionMultiply, -400, 200)
    mel.connect_material_expressions(x1, "", x2, "A")
    mel.connect_material_expressions(ps, "", x2, "B")
    mel.connect_material_property(x1, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(nham, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(x2, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    xong(m)

# M_PhuMo: trong suốt không chiếu sáng (kính, vệt mưa)
m = vat_lieu_moi("M_PhuMo")
if m:
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("two_sided", True)
    mau = bieu_thuc(m, unreal.MaterialExpressionVectorParameter, -400, 0, parameter_name="MauSac", default_value=unreal.LinearColor(0, 0, 0, 1))
    mo = bieu_thuc(m, unreal.MaterialExpressionScalarParameter, -400, 200, parameter_name="DoMo", default_value=0.5)
    mel.connect_material_property(mau, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(mo, "", unreal.MaterialProperty.MP_OPACITY)
    xong(m)

# M_Sang: phát sáng màu (bóng đèn huỳnh quang, biển thoát hiểm, mắt)
m = vat_lieu_moi("M_Sang")
if m:
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mau = bieu_thuc(m, unreal.MaterialExpressionVectorParameter, -600, 0, parameter_name="MauSac", default_value=unreal.LinearColor(1, 1, 1, 1))
    sang = bieu_thuc(m, unreal.MaterialExpressionScalarParameter, -600, 200, parameter_name="DoSang", default_value=4.0)
    x = bieu_thuc(m, unreal.MaterialExpressionMultiply, -300, 0)
    mel.connect_material_expressions(mau, "", x, "A")
    mel.connect_material_expressions(sang, "", x, "B")
    mel.connect_material_property(x, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    xong(m)

# M_Decal: vết máu, chữ, nước đọng (texture RGBA)
m = vat_lieu_moi("M_Decal")
if m:
    m.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    tex = bieu_thuc(m, unreal.MaterialExpressionTextureSampleParameter2D, -700, 0, parameter_name="Anh", texture=eal.load_asset("/Game/Anh/T_dau_tay"))
    mo = bieu_thuc(m, unreal.MaterialExpressionScalarParameter, -700, 300, parameter_name="DoMo", default_value=1.0)
    nham = bieu_thuc(m, unreal.MaterialExpressionScalarParameter, -400, 400, parameter_name="DoNham", default_value=0.25)
    x = bieu_thuc(m, unreal.MaterialExpressionMultiply, -400, 200)
    mel.connect_material_expressions(tex, "A", x, "A")
    mel.connect_material_expressions(mo, "", x, "B")
    mel.connect_material_property(tex, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(x, "", unreal.MaterialProperty.MP_OPACITY)
    mel.connect_material_property(nham, "", unreal.MaterialProperty.MP_ROUGHNESS)
    xong(m)

# M_Kinh: kính cửa sổ — phản chiếu Fresnel, vệt mưa trượt xuống (MuaCo = có mưa, MuaChay = tốc độ trượt)
m = vat_lieu_moi("M_Kinh")
if m:
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    mau = bieu_thuc(m, unreal.MaterialExpressionVectorParameter, -900, -200, parameter_name="MauKinh", default_value=unreal.LinearColor(0.015, 0.02, 0.025, 1))
    uv = bieu_thuc(m, unreal.MaterialExpressionTextureCoordinate, -1400, 0)
    tg = bieu_thuc(m, unreal.MaterialExpressionTime, -1400, 150)
    chay = bieu_thuc(m, unreal.MaterialExpressionScalarParameter, -1400, 250, parameter_name="MuaChay", default_value=0.0)
    x_tg = bieu_thuc(m, unreal.MaterialExpressionMultiply, -1200, 200)
    mel.connect_material_expressions(tg, "", x_tg, "A")
    mel.connect_material_expressions(chay, "", x_tg, "B")
    khong = bieu_thuc(m, unreal.MaterialExpressionConstant, -1200, 320, r=0.0)
    lech = bieu_thuc(m, unreal.MaterialExpressionAppendVector, -1050, 250)
    mel.connect_material_expressions(khong, "", lech, "A")
    mel.connect_material_expressions(x_tg, "", lech, "B")
    uv2 = bieu_thuc(m, unreal.MaterialExpressionAdd, -900, 100)
    mel.connect_material_expressions(uv, "", uv2, "A")
    mel.connect_material_expressions(lech, "", uv2, "B")
    tex = bieu_thuc(m, unreal.MaterialExpressionTextureSampleParameter2D, -700, 100, parameter_name="VetMua", texture=eal.load_asset("/Game/Anh/T_vet_mua_kinh"))
    mel.connect_material_expressions(uv2, "", tex, "UVs")
    co = bieu_thuc(m, unreal.MaterialExpressionScalarParameter, -700, 350, parameter_name="MuaCo", default_value=0.0)
    giot = bieu_thuc(m, unreal.MaterialExpressionMultiply, -450, 200)
    mel.connect_material_expressions(tex, "A", giot, "A")
    mel.connect_material_expressions(co, "", giot, "B")
    fr = bieu_thuc(m, unreal.MaterialExpressionFresnel, -700, 500)
    fr_x = bieu_thuc(m, unreal.MaterialExpressionMultiply, -450, 500, const_b=0.45)
    mel.connect_material_expressions(fr, "", fr_x, "A")
    mo0 = bieu_thuc(m, unreal.MaterialExpressionAdd, -300, 400, const_b=0.07)
    mel.connect_material_expressions(fr_x, "", mo0, "A")
    giot_x = bieu_thuc(m, unreal.MaterialExpressionMultiply, -300, 250, const_b=0.8)
    mel.connect_material_expressions(giot, "", giot_x, "A")
    mo = bieu_thuc(m, unreal.MaterialExpressionAdd, -150, 300)
    mel.connect_material_expressions(mo0, "", mo, "A")
    mel.connect_material_expressions(giot_x, "", mo, "B")
    nham_g = bieu_thuc(m, unreal.MaterialExpressionMultiply, -300, 600, const_b=0.25)
    mel.connect_material_expressions(giot, "", nham_g, "A")
    nham = bieu_thuc(m, unreal.MaterialExpressionAdd, -150, 500, const_b=0.03)
    mel.connect_material_expressions(nham_g, "", nham, "A")
    pk = bieu_thuc(m, unreal.MaterialExpressionConstant, -150, 700, r=1.0)
    trang = bieu_thuc(m, unreal.MaterialExpressionConstant3Vector, -450, -300, constant=unreal.LinearColor(0.55, 0.6, 0.65, 1))
    tron = bieu_thuc(m, unreal.MaterialExpressionLinearInterpolate, -150, -200)
    mel.connect_material_expressions(mau, "", tron, "A")
    mel.connect_material_expressions(trang, "", tron, "B")
    mel.connect_material_expressions(giot, "", tron, "Alpha")
    mel.connect_material_property(tron, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(mo, "", unreal.MaterialProperty.MP_OPACITY)
    mel.connect_material_property(nham, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(pk, "", unreal.MaterialProperty.MP_SPECULAR)
    # giọt nước hắt ánh đèn hành lang (kính đứng gần như không nhận ánh đèn trần chiếu xuống)
    sang = bieu_thuc(m, unreal.MaterialExpressionScalarParameter, -450, 800, parameter_name="SangGiot", default_value=25.0)
    ps = bieu_thuc(m, unreal.MaterialExpressionMultiply, -150, 850)
    mel.connect_material_expressions(giot, "", ps, "A")
    mel.connect_material_expressions(sang, "", ps, "B")
    ps3 = bieu_thuc(m, unreal.MaterialExpressionMultiply, 0, 850)
    mel.connect_material_expressions(ps, "", ps3, "A")
    mel.connect_material_expressions(trang, "", ps3, "B")
    mel.connect_material_property(ps3, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    xong(m)

# M_InKinh: dấu tay / vết in trên mặt kính (texture RGBA, trong suốt có chiếu sáng)
m = vat_lieu_moi("M_InKinh")
if m:
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    m.set_editor_property("two_sided", True)
    tex = bieu_thuc(m, unreal.MaterialExpressionTextureSampleParameter2D, -700, 0, parameter_name="Anh", texture=eal.load_asset("/Game/Anh/T_tay_kinh"))
    mo = bieu_thuc(m, unreal.MaterialExpressionScalarParameter, -700, 300, parameter_name="DoMo", default_value=1.0)
    x = bieu_thuc(m, unreal.MaterialExpressionMultiply, -400, 200)
    mel.connect_material_expressions(tex, "A", x, "A")
    mel.connect_material_expressions(mo, "", x, "B")
    nham = bieu_thuc(m, unreal.MaterialExpressionConstant, -400, 400, r=0.35)
    mel.connect_material_property(tex, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(x, "", unreal.MaterialProperty.MP_OPACITY)
    mel.connect_material_property(nham, "", unreal.MaterialProperty.MP_ROUGHNESS)
    xong(m)

# M_NhieuTV: màn hình nhiễu sóng (nhiễu thay đổi theo thời gian)
m = vat_lieu_moi("M_NhieuTV")
if m:
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    uv = bieu_thuc(m, unreal.MaterialExpressionTextureCoordinate, -900, 0, u_tiling=160.0, v_tiling=120.0)
    tg = bieu_thuc(m, unreal.MaterialExpressionTime, -900, 200)
    nhan_tg = bieu_thuc(m, unreal.MaterialExpressionMultiply, -700, 200, const_b=37.0)
    mel.connect_material_expressions(tg, "", nhan_tg, "A")
    ghep = bieu_thuc(m, unreal.MaterialExpressionAppendVector, -550, 100)
    mel.connect_material_expressions(uv, "", ghep, "A")
    mel.connect_material_expressions(nhan_tg, "", ghep, "B")
    nh = bieu_thuc(m, unreal.MaterialExpressionNoise, -350, 0, scale=1.0, levels=1, output_min=0.0, output_max=1.0, noise_function=unreal.NoiseFunction.NOISEFUNCTION_VALUE_ALU)
    mel.connect_material_expressions(ghep, "", nh, "Position")
    sang = bieu_thuc(m, unreal.MaterialExpressionScalarParameter, -350, 250, parameter_name="DoSang", default_value=1.5)
    x = bieu_thuc(m, unreal.MaterialExpressionMultiply, -150, 0)
    mel.connect_material_expressions(nh, "", x, "A")
    mel.connect_material_expressions(sang, "", x, "B")
    mel.connect_material_property(x, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    xong(m)

# ---------- Bản đồ trống ----------
if not eal.does_asset_exist("/Game/Maps/HanhLang"):
    w = at.create_asset("HanhLang", "/Game/Maps", unreal.World, unreal.WorldFactory())
    eal.save_loaded_asset(w)
    ghi("Da tao /Game/Maps/HanhLang")

# ---------- Mẫu đồ vật (SimReady → GLB) ----------
thu_muc = os.path.join(NGUON, "do_vat")
BO_MAU = os.environ.get("BO_MAU") == "1"   # chỉ nhập texture / âm thanh (mẫu chưa chuyển xong)
for f in ([] if BO_MAU else sorted(os.listdir(thu_muc))):
    if not f.lower().endswith(".glb"):
        continue
    ten = os.path.splitext(f)[0]
    dich = "/Game/Mau/" + ten
    if eal.does_directory_exist(dich) and eal.list_assets(dich, False):
        continue
    nhap(os.path.join(thu_muc, f), dich)
for a in eal.list_assets("/Game/Mau", True):
    luoi = eal.load_asset(a)
    if not isinstance(luoi, unreal.StaticMesh):
        continue
    doi = False
    bs = luoi.get_editor_property("body_setup")
    if bs and bs.get_editor_property("collision_trace_flag") != unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE:
        bs.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        doi = True
    # Mẫu SimReady rất dày (tới hơn 1 triệu tam giác): bật Nanite; va chạm dùng lưới rút gọn của Nanite là đủ cho đồ vật
    ns = luoi.get_editor_property("nanite_settings")
    if not ns.get_editor_property("enabled"):
        ns.set_editor_property("enabled", True)
        luoi.set_editor_property("nanite_settings", ns)
        doi = True
    if doi:
        eal.save_loaded_asset(luoi)

# ---------- Âm thanh ----------
thu_muc = os.path.join(DU_AN, "NguonAmThanh")
if os.path.isdir(thu_muc):
    for f in sorted(os.listdir(thu_muc)):
        if not f.lower().endswith(".wav"):
            continue
        ten = os.path.splitext(f)[0]
        duong = "/Game/AmThanh/" + ten
        if not eal.does_asset_exist(duong):
            nhap(os.path.join(thu_muc, f), "/Game/AmThanh")
        sw = eal.load_asset(duong)
        if sw and sw.get_editor_property("looping") != (ten in LAP):
            sw.set_editor_property("looping", ten in LAP)
            eal.save_loaded_asset(sw)

for a in eal.list_assets("/Game/Mau", True) + eal.list_assets("/Game/AmThanh", True):
    d = eal.find_asset_data(a)
    ghi("TAI_SAN %s %s" % (d.asset_class_path.asset_name, a))

os.makedirs(os.path.join(DU_AN, "Saved"), exist_ok=True)
with open(os.path.join(DU_AN, "Saved", "nhap_mau.txt"), "w", encoding="utf-8") as f:
    f.write("\n".join(NHAT_KY))

# Kích thước hộp bao (cm) của từng mẫu để đặt đúng chỗ
with open(os.path.join(DU_AN, "Saved", "kich_thuoc_mau.txt"), "w", encoding="utf-8") as f:
    for a in eal.list_assets("/Game/Mau", True):
        luoi = eal.load_asset(a)
        if isinstance(luoi, unreal.StaticMesh):
            b = luoi.get_bounding_box()
            f.write("%s min=(%.0f,%.0f,%.0f) max=(%.0f,%.0f,%.0f)\n" % (a.split("/")[3], b.min.x, b.min.y, b.min.z, b.max.x, b.max.y, b.max.z))

# Vật liệu dùng cho hình người (skeletal) và mưa (instanced) phải bật cờ, không thì game thay bằng vật liệu xám mặc định
for ten in ("M_Mau", "M_PhuMo", "M_Sang", "M_BeMat", "M_Kinh", "M_InKinh"):
    m = eal.load_asset("/Game/VatLieu/" + ten)
    doi = False
    for co in ("used_with_skeletal_mesh", "used_with_instanced_static_meshes"):
        if not m.get_editor_property(co):
            m.set_editor_property(co, True)
            doi = True
    if doi:
        mel.recompile_material(m)
        eal.save_loaded_asset(m)

# ---------- Bóng ma: "Ghost Girl Animated" (Shaban Hafizsalim, CC BY 4.0) đã chuẩn bị bằng nguon/_scripts/lam_bong_ma.py ----------
FBX_MA = os.path.join(NGUON, "bong_ma_ue", "bong_ma.fbx")
if os.path.exists(FBX_MA) and not (eal.does_directory_exist("/Game/BongMa") and eal.list_assets("/Game/BongMa", True)):
    nhap(FBX_MA, "/Game/BongMa")
with open(os.path.join(DU_AN, "Saved", "bong_ma.txt"), "w", encoding="utf-8") as f:
    for a in eal.list_assets("/Game/BongMa", True):
        d = eal.find_asset_data(a)
        o = eal.load_asset(a)
        dong = "%s %s" % (d.asset_class_path.asset_name, a)
        if isinstance(o, unreal.SkeletalMesh):
            b = o.get_bounds()
            dong += " goc=%s mo_rong=%s" % (b.origin, b.box_extent)
        if isinstance(o, unreal.AnimSequence):
            dong += " dai=%.2fs" % o.get_editor_property("sequence_length") if hasattr(o, "sequence_length") else ""
        f.write(dong + "\n")

# Váy bóng ma là vải một lớp (pháp tuyến quay vào trong) → vật liệu texture vẽ hai mặt
m = eal.load_asset("/Game/VatLieu/M_BeMat")
if not m.get_editor_property("two_sided"):
    m.set_editor_property("two_sided", True)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
