# Dựng trailer: thẻ chữ (PIL) + bản trộn âm thanh (numpy/soundfile) theo dòng thời gian, xuất danh sách cho Blender ghép.
# Chạy bằng python có numpy + soundfile + PIL (venv Piper ở D:\AICache\piper):  python dung_trailer.py
import json, os, glob
import numpy as np
import soundfile as sf
from PIL import Image, ImageDraw, ImageFont, ImageFilter

GOC = os.path.dirname(os.path.abspath(__file__))
KHUNG = os.path.join(GOC, "khung")            # ảnh các cảnh quay (mỗi cảnh một thư mục)
THE = os.path.join(GOC, "the_chu"); os.makedirs(THE, exist_ok=True)
AM = r"D:\Dự Án Claude\Hành Lang Lặp\NguonAmThanh"
NHAC = os.path.join(GOC, "am_thanh", "trailer_nhac.wav")
FPS = 30
W, H = 1920, 1080
SR = 44100

# Dòng thời gian: thẻ chữ (giây) và cảnh quay (độ dài = số khung đã quay); âm thanh theo mốc trong từng đoạn
DONG = [
    ("the", "den", 1.0, []),
    ("the", "03:00 AM", 2.6, [(0.2, "dong_ho", 0.5)]),
    ("canh", "s01_hanh_lang", None, [(0.0, "nen_den_huynh_quang", 0.35)]),
    ("the", "ONE NIGHT SHIFT.", 2.2, []),
    ("canh", "s02_dong_ho", None, [(0.0, "dong_ho", 0.6), (0.0, "nen_den_huynh_quang", 0.3)]),
    ("the", "ONE CORRIDOR.", 2.0, []),
    ("canh", "s03_cua_mo", None, [(0.6, "cua_mo", 0.8), (0.0, "nen_u_am_1", 0.35)]),
    ("the", "THAT NEVER ENDS.", 2.2, [(0.3, "ro_ri", 0.5)]),
    ("canh", "s04_dien_thoai", None, [(0.0, "dien_thoai_reo", 0.7), (0.0, "nen_u_am_1", 0.3)]),
    ("canh", "s05_mua_set", None, [(0.0, "nen_mua", 0.6), (1.2, "sam_set", 0.9)]),
    ("canh", "s06_tay_kinh", None, [(0.0, "nen_mua", 0.5), (1.4, "go_kinh", 0.9), (1.9, "go_kinh", 0.7), (0.4, "dt_giong", 0.9, 3.0)]),
    ("canh", "s07_co_giat", None, [(0.0, "co_giat", 0.8), (1.0, "giat_nhe", 0.5), (0.0, "nen_mua", 0.4)]),
    ("canh", "s08_sam_bong", None, [(0.4, "sam_set", 1.0), (0.6, "cup_dien", 0.6), (1.0, "dien_lai", 0.4)]),
    ("the", "DON'T LOOK BACK.", 2.4, [(0.2, "thi_tham", 0.7)]),
    ("canh", "s09_nhen", None, [(0.0, "nhen_bo", 0.9), (0.2, "giat_nhe", 0.6), (0.0, "nhip_tim", 0.5)]),
    ("canh", "s10_tien_lai", None, [(0.5, "den_chap", 0.6), (1.6, "den_chap", 0.6), (2.7, "den_chap", 0.6), (3.0, "giat_minh", 1.0)]),
    ("canh", "s11_nhin_lai", None, [(1.2, "giat_nhe", 0.7), (2.7, "giat_minh", 0.9), (0.0, "nhip_tim", 0.6)]),
    ("canh", "s12_duoi", None, [(0.0, "giat_minh", 0.9), (0.0, "duoi_bat", 0.9), (0.0, "sam_set", 0.8), (3.3, "giat_minh", 1.2)]),
    ("the", "den", 1.2, []),
    ("the", "NIGHT SHIFT", 5.0, [(0.0, "ket_thuc", 0.0)]),
]
FONT = "C:/Windows/Fonts/arial.ttf"
FONT_DAM = "C:/Windows/Fonts/arialbd.ttf"


def ve_the(chu, ten):
    im = Image.new("RGB", (W, H), (0, 0, 0))
    if chu != "den":
        d = ImageDraw.Draw(im)
        lon = chu == "NIGHT SHIFT"
        f = ImageFont.truetype(FONT_DAM if lon else FONT, 150 if lon else 72)
        # chữ hơi nhòe đỏ phía sau cho cảm giác phim kinh dị
        bong = Image.new("RGB", (W, H), (0, 0, 0))
        ImageDraw.Draw(bong).text((W / 2, H / 2), chu, font=f, fill=(120, 10, 10), anchor="mm")
        im = Image.composite(bong.filter(ImageFilter.GaussianBlur(14)), im, bong.convert("L").point(lambda v: 255))
        d = ImageDraw.Draw(im)
        d.text((W / 2, H / 2), chu, font=f, fill=(225, 218, 210), anchor="mm")
        if lon:
            f2 = ImageFont.truetype(FONT, 40)
            d.text((W / 2, H / 2 + 130), "Don't look back.", font=f2, fill=(150, 30, 30), anchor="mm")
            f3 = ImageFont.truetype(FONT, 26)
            d.text((W / 2, H - 70), "Made with Unreal Engine 5.8  ·  Ghost: \"Ghost Girl Animated\" by Shaban Hafizsalim (CC BY 4.0)  ·  Sound: Kana's Audio Studio MCP", font=f3, fill=(110, 110, 110), anchor="mm")
    p = os.path.join(THE, ten + ".png")
    im.save(p)
    return p


def doc_am(ten):
    a, sr = sf.read(os.path.join(AM, ten + ".wav") if ten != "trailer_nhac" else NHAC, dtype="float32", always_2d=True)
    a = a.mean(axis=1) if a.shape[1] > 1 else a[:, 0]
    if sr != SR:
        t = np.arange(0, len(a) * SR / sr) * sr / SR
        a = np.interp(t, np.arange(len(a)), a).astype(np.float32)
    return a


# Dựng danh sách đoạn với độ dài thật
doan, t = [], 0.0
for loai, ten, dai, am in DONG:
    if loai == "the":
        anh = ve_the(ten, "the_%02d" % len(doan))
        doan.append({"loai": "the", "anh": anh, "bat_dau": t, "dai": dai})
    else:
        khung = sorted(glob.glob(os.path.join(KHUNG, ten, "f*.png")))
        dai = len(khung) / FPS
        doan.append({"loai": "canh", "thu_muc": os.path.join(KHUNG, ten), "so_khung": len(khung), "bat_dau": t, "dai": dai})
    doan[-1]["am"] = am
    t += dai
TONG = t
print("Tổng", round(TONG, 2), "giây")

# Trộn âm thanh: nhạc trailer nền + hiệu ứng theo mốc (âm lặp thì cắt theo hết đoạn)
mix = np.zeros(int(TONG * SR) + SR, np.float32)
if os.path.exists(NHAC):
    n = doc_am("trailer_nhac")
    n = n[: len(mix)]
    vao = np.linspace(0, 1, int(1.5 * SR))
    n[: len(vao)] *= vao
    mix[: len(n)] += n * 0.55
for d in doan:
    for cue in d["am"]:
        moc, ten, am = cue[0], cue[1], cue[2]
        if am <= 0:
            continue
        a = doc_am(ten)
        con = d["dai"] - moc
        if len(cue) > 3:
            con = min(con, cue[3]) if cue[3] > 0 else con
        lap = ten.startswith("nen_") or ten in ("nhip_tim", "duoi_bat", "dien_thoai_reo", "nhen_bo", "dong_ho")
        if lap and len(a) < con * SR:
            a = np.tile(a, int(con * SR / len(a)) + 1)
        if lap or len(cue) > 3:
            a = a[: int(con * SR)]
            tat = min(len(a), int(0.25 * SR))
            if tat > 0:
                a[-tat:] *= np.linspace(1, 0, tat)
        i = int((d["bat_dau"] + moc) * SR)
        j = min(len(mix), i + len(a))
        mix[i:j] += a[: j - i] * am
mix = np.tanh(mix * 1.1) / np.tanh(1.1)
mix = mix / (np.max(np.abs(mix)) + 1e-6) * 0.92
sf.write(os.path.join(GOC, "am_thanh", "tron.wav"), np.stack([mix, mix], 1), SR, subtype="PCM_16")
json.dump({"fps": FPS, "w": W, "h": H, "tong": TONG, "doan": doan, "am": os.path.join(GOC, "am_thanh", "tron.wav")},
          open(os.path.join(GOC, "dong_thoi_gian.json"), "w", encoding="utf-8"), ensure_ascii=False, indent=1)
print("xong")
