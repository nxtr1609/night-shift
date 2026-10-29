# Ghép trailer bằng Video Sequence Editor của Blender: cảnh quay (chuỗi ảnh) + thẻ chữ + âm thanh → MP4 H.264.
# Chạy: blender -b --factory-startup --python ghep_video.py -- <dong_thoi_gian.json> <file mp4>
import bpy, json, os, sys

a = sys.argv[sys.argv.index("--") + 1:]
D = json.load(open(a[0], encoding="utf-8"))
RA = a[1]
sc = bpy.context.scene
sc.render.resolution_x, sc.render.resolution_y = D["w"], D["h"]
sc.render.resolution_percentage = 100
sc.render.fps = D["fps"]
sc.frame_start = 1
sc.frame_end = int(round(D["tong"] * D["fps"]))
se = sc.sequence_editor_create()
kenh = 1
for i, d in enumerate(D["doan"]):
    bat = int(round(d["bat_dau"] * D["fps"])) + 1
    so = max(1, int(round(d["dai"] * D["fps"])))
    if d["loai"] == "the":
        s = se.strips.new_image(name="the%d" % i, filepath=d["anh"], channel=2, frame_start=bat)
        s.frame_final_duration = so
    else:
        anh = sorted(f for f in os.listdir(d["thu_muc"]) if f.endswith(".png"))
        s = se.strips.new_image(name="canh%d" % i, filepath=os.path.join(d["thu_muc"], anh[0]), channel=1 + (i % 2), frame_start=bat)
        for f in anh[1:]:
            s.elements.append(f)
        s.frame_final_duration = len(anh)
    # mờ vào / mờ ra ngắn ở mỗi đoạn
    s.blend_type = "ALPHA_OVER"
    nh = min(6, so // 3)
    s.blend_alpha = 1.0
    s.keyframe_insert("blend_alpha", frame=bat)
    if nh > 0 and d["loai"] == "the":
        s.blend_alpha = 0.0; s.keyframe_insert("blend_alpha", frame=bat)
        s.blend_alpha = 1.0; s.keyframe_insert("blend_alpha", frame=bat + nh)
        s.blend_alpha = 1.0; s.keyframe_insert("blend_alpha", frame=bat + so - nh)
        s.blend_alpha = 0.0; s.keyframe_insert("blend_alpha", frame=bat + so - 1)
am = se.strips.new_sound(name="am", filepath=D["am"], channel=5, frame_start=1)
r = sc.render
r.image_settings.media_type = "VIDEO" if hasattr(r.image_settings, "media_type") else None
r.image_settings.file_format = "FFMPEG"
r.ffmpeg.format = "MPEG4"
r.ffmpeg.codec = "H264"
r.ffmpeg.constant_rate_factor = "HIGH"
r.ffmpeg.audio_codec = "AAC"
r.ffmpeg.audio_bitrate = 256
r.filepath = RA
bpy.ops.render.render(animation=True)
print("XONG", RA)
