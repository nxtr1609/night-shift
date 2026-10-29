# Chuyển hoạt ảnh mannequin (đứng / đi / chạy...) sang khung xương Mixamo của bóng ma bằng IK Retargeter.
# Chạy: UnrealEditor-Cmd.exe <du_an>.uproject -run=pythonscript -script="<file nay>" -unattended -nosplash
import os
import unreal

at = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
THU = "/Game/BongMa/Retarget"
LOG = []


def ghi(s):
    unreal.log(s)
    LOG.append(str(s))


def doc(ham):
    ghi("DOC %s: %s" % (getattr(ham, "__name__", ham), (ham.__doc__ or "").strip().replace("\n", " ")[:600]))


manny = eal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple")
ma = eal.load_asset("/Game/BongMa/bong_ma")
if False:  # thư mục cũ được xóa từ ngoài trước khi chạy (xóa rồi tạo lại cùng tên trong một phiên sẽ hỏng)
    eal.delete_directory(THU)


def tao_rig(ten, mesh):
    rig = at.create_asset(ten, THU, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    c = unreal.IKRigController.get_controller(rig)
    c.set_skeletal_mesh(mesh)
    ok = c.apply_auto_generated_retarget_definition()
    ghi("RIG %s auto=%s root=%s" % (ten, ok, c.get_retarget_root()))
    if not ok:
        # Khung Mixamo (tên mixamorig_*): tự khai báo chuỗi trùng tên với mannequin để ghép tự động
        doc(c.add_retarget_chain)
        c.set_retarget_root("mixamorig_Hips")
        for ten_c, dau, cuoi in [("Spine", "mixamorig_Spine", "mixamorig_Spine2"), ("Neck", "mixamorig_Neck", "mixamorig_Neck"),
                                 ("Head", "mixamorig_Head", "mixamorig_Head"),
                                 ("LeftClavicle", "mixamorig_LeftShoulder", "mixamorig_LeftShoulder"), ("LeftArm", "mixamorig_LeftArm", "mixamorig_LeftHand"),
                                 ("RightClavicle", "mixamorig_RightShoulder", "mixamorig_RightShoulder"), ("RightArm", "mixamorig_RightArm", "mixamorig_RightHand"),
                                 ("LeftLeg", "mixamorig_LeftUpLeg", "mixamorig_LeftFoot"), ("LeftFoot", "mixamorig_LeftToeBase", "mixamorig_LeftToeBase"),
                                 ("RightLeg", "mixamorig_RightUpLeg", "mixamorig_RightFoot"), ("RightFoot", "mixamorig_RightToeBase", "mixamorig_RightToeBase")]:
            try:
                c.add_retarget_chain(ten_c, dau, cuoi, "")
            except Exception as e:
                ghi("chain %s LOI %s" % (ten_c, e))
    for x in c.get_retarget_chains():
        n = x.get_editor_property("chain_name")
        ghi("   CHUOI %s: %s -> %s" % (n, c.get_retarget_chain_start_bone(n), c.get_retarget_chain_end_bone(n)))
    eal.save_loaded_asset(rig)
    return rig


try:
    rig_nguon = tao_rig("IK_Manny", manny)
    rig_dich = tao_rig("IK_BongMa", ma)
    rt = at.create_asset("RTG_Manny_BongMa", THU, unreal.IKRetargeter, unreal.IKRetargetFactory())
    rc = unreal.IKRetargeterController.get_controller(rt)
    for h in (rc.set_ik_rig, rc.add_default_ops, rc.assign_ik_rig_to_all_ops, rc.auto_map_chains, rc.auto_align_all_bones,
              unreal.IKRetargetBatchOperation.duplicate_and_retarget):
        doc(h)
    rc.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, rig_nguon)
    rc.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, rig_dich)
    try:
        rc.add_default_ops()
    except Exception as e:
        ghi("add_default_ops LOI %s" % e)
    try:
        rc.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.SOURCE, rig_nguon)
        rc.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.TARGET, rig_dich)
    except Exception as e:
        ghi("assign_ik_rig_to_all_ops LOI %s" % e)
    try:
        rc.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
    except Exception as e:
        ghi("auto_map_chains LOI %s" % e)
    try:
        if os.environ.get("CAN_XUONG") != "0": rc.auto_align_all_bones(unreal.RetargetSourceOrTarget.TARGET, unreal.RetargetAutoAlignMethod.CHAIN_TO_CHAIN)
    except Exception as e:
        ghi("auto_align_all_bones LOI %s" % e)
    eal.save_loaded_asset(rt)

    CAN = ["/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle",
           "/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd",
           "/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd"]
    du_lieu = [unreal.EditorAssetLibrary.find_asset_data(p) for p in CAN]
    kq = unreal.IKRetargetBatchOperation.duplicate_and_retarget(du_lieu, manny, ma, rt, search="", replace="", prefix="", suffix="_Ma", target_path=THU, use_source_path=False, include_referenced_assets=False, overwrite_existing_files=True)
    ghi("KET QUA %s" % [str(x.package_name) if hasattr(x, "package_name") else str(x) for x in kq])
    # duplicate_and_retarget chỉ tạo trong bộ nhớ: phải lưu ra đĩa
    for x in kq:
        ghi("LUU %s %s" % (x.package_name, eal.save_asset(str(x.package_name), only_if_is_dirty=False)))
except Exception as e:
    import traceback
    ghi("LOI " + traceback.format_exc())

for a in eal.list_assets("/Game", True):
    if a.endswith("_Ma") or "_Ma." in a:
        ghi("TAI_SAN " + a)
with open(os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "Saved", "chuyen_hoat_anh.txt"), "w", encoding="utf-8") as f:
    f.write("\n".join(LOG))
