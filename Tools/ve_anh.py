# Vẽ texture cho hành lang bệnh viện bằng PIL. Chạy: python -I -X utf8 ve_anh.py <thư mục ra>
import math, os, random, sys
from PIL import Image, ImageDraw, ImageFilter, ImageFont, ImageChops

RA = sys.argv[1]
os.makedirs(RA, exist_ok=True)
random.seed(7)
FONT = "C:/Windows/Fonts/arialbd.ttf"
FONT_TAY = "C:/Windows/Fonts/arialbi.ttf"


def nhieu(w, h, o, sang=128, bien=40, seed=0):
    """Vết loang mềm: ảnh ngẫu nhiên nhỏ phóng to."""
    r = random.Random(seed)
    nho = Image.new("L", (max(1, w // o), max(1, h // o)))
    nho.putdata([max(0, min(255, int(sang + r.uniform(-bien, bien)))) for _ in range(nho.width * nho.height)])
    return nho.resize((w, h), Image.BICUBIC)


def hat(w, h, bien, seed=0):
    r = random.Random(seed)
    im = Image.new("L", (w, h))
    im.putdata([max(0, min(255, int(128 + r.gauss(0, bien)))) for _ in range(w * h)])
    return im


def nhan_mau(nen, lop, muc):
    """Trộn kiểu overlay đơn giản: lop (L, 128 = trung tính) làm sáng/tối nền."""
    ket = ImageChops.add(nen, lop.convert("RGB"), 1.0, -128)
    return ket if muc >= 1.0 else Image.blend(nen, ket, muc)


def luu(im, ten):
    im.save(os.path.join(RA, ten + ".png"))
    print("ve", ten, im.size)


# 1. Sàn linoleum: 2x2 ô 60 cm (ảnh = 120 cm)
S = 1024
san = Image.new("RGB", (S, S))
d = ImageDraw.Draw(san)
mau_o = [(214, 210, 196), (176, 186, 172)]
for i in range(2):
    for j in range(2):
        d.rectangle([i * S // 2, j * S // 2, (i + 1) * S // 2, (j + 1) * S // 2], fill=mau_o[(i + j) % 2])
san = nhan_mau(san, hat(S, S, 14, 1), 1.0)
san = nhan_mau(san, nhieu(S, S, 64, 128, 9, 2), 1.0)
d = ImageDraw.Draw(san)
for k in (0, S // 2):
    d.line([(k, 0), (k, S)], fill=(120, 118, 108), width=3)
    d.line([(0, k), (S, k)], fill=(120, 118, 108), width=3)
for _ in range(60):  # vết xước nhỏ
    x, y = random.randrange(S), random.randrange(S)
    d.line([(x, y), (x + random.randint(-30, 30), y + random.randint(-6, 6))], fill=(150, 148, 138), width=1)
luu(san, "san")

# 2. Tường: rộng 200 cm, cao 280 cm (V=0 ở trần). Dưới 110 cm xanh bạc hà, nẹp, trên kem.
W, H = 1024, 1434
tuong = Image.new("RGB", (W, H))
d = ImageDraw.Draw(tuong)
y_nep = int(H * (1 - 120 / 280)); y_nep2 = int(H * (1 - 110 / 280)); y_chan = int(H * (1 - 10 / 280))
d.rectangle([0, 0, W, y_nep], fill=(206, 202, 186))
d.rectangle([0, y_nep, W, y_nep2], fill=(84, 112, 98))
d.rectangle([0, y_nep2, W, H], fill=(140, 172, 156))
d.rectangle([0, y_chan, W, H], fill=(70, 78, 72))
tuong = nhan_mau(tuong, hat(W, H, 8, 3), 1.0)
tuong = nhan_mau(tuong, nhieu(W, H, 96, 128, 7, 4), 1.0)
# vệt ố chảy dọc
o = Image.new("L", (W, H), 128)
do = ImageDraw.Draw(o)
for _ in range(14):
    x = random.randrange(W); y0 = random.randrange(0, H // 2); dai = random.randrange(80, 500)
    do.line([(x, y0), (x + random.randint(-6, 6), y0 + dai)], fill=random.randint(100, 118), width=random.randint(3, 10))
o = o.filter(ImageFilter.GaussianBlur(4))
tuong = nhan_mau(tuong, o, 1.0)
# bẩn gần chân tường
ban = Image.new("L", (W, H), 128)
db = ImageDraw.Draw(ban)
for y in range(y_chan - 160, y_chan):
    t = (y - (y_chan - 160)) / 160
    db.line([(0, y), (W, y)], fill=int(128 - 26 * t * t))
tuong = nhan_mau(tuong, ImageChops.multiply(ban.convert("L"), nhieu(W, H, 32, 255, 60, 5)).point(lambda v: 128 - (128 - v) // 2 if v < 128 else 128), 1.0)
luu(tuong, "tuong")

# 3. Trần ô 60 cm (ảnh = 120 cm)
tran = Image.new("RGB", (S, S), (222, 220, 212))
tran = nhan_mau(tran, hat(S, S, 18, 6), 1.0)
d = ImageDraw.Draw(tran)
for _ in range(5000):
    x, y = random.randrange(S), random.randrange(S)
    d.ellipse([x, y, x + 2, y + 2], fill=(180, 178, 170))
for k in (0, S // 2):
    d.rectangle([k - 5, 0, k + 5, S], fill=(170, 170, 166))
    d.rectangle([0, k - 5, S, k + 5], fill=(170, 170, 166))
tran = nhan_mau(tran, nhieu(S, S, 128, 128, 7, 7), 1.0)
luu(tran, "tran")

# 4. Bê tông (buồng cầu thang)
bt = Image.new("RGB", (S, S), (128, 126, 120))
bt = nhan_mau(bt, hat(S, S, 22, 8), 1.0)
bt = nhan_mau(bt, nhieu(S, S, 48, 128, 30, 9), 1.0)
bt = nhan_mau(bt, nhieu(S, S, 8, 128, 12, 10), 1.0)
luu(bt, "be_tong")

# 5. Cửa gỗ sơn (100 x 210 cm)
cw, ch = 512, 1076
cua = Image.new("RGB", (cw, ch), (150, 168, 176))
cua = nhan_mau(cua, hat(cw, ch, 6, 11), 1.0)
cua = nhan_mau(cua, nhieu(cw, ch, 60, 128, 6, 12), 1.0)
d = ImageDraw.Draw(cua)
d.rectangle([0, ch - 150, cw, ch], fill=(150, 152, 150))   # tấm chắn chân inox
d.rectangle([0, 0, cw - 1, ch - 1], outline=(110, 124, 130), width=6)
luu(cua, "cua")


def bien(ten, chu, w, h, nen, mau_chu, co=None, vien=None):
    im = Image.new("RGB", (w, h), nen)
    d = ImageDraw.Draw(im)
    co = co or int(h * 0.42)
    f = ImageFont.truetype(FONT, co)
    while d.textlength(chu, font=f) > w * 0.9:
        co -= 2; f = ImageFont.truetype(FONT, co)
    d.text((w / 2, h / 2), chu, font=f, fill=mau_chu, anchor="mm")
    if vien:
        d.rectangle([4, 4, w - 5, h - 5], outline=vien, width=6)
    luu(im, ten)


for so in (401, 402, 403, 405):
    bien("bien_%d" % so, "ROOM %d" % so, 512, 160, (236, 236, 230), (40, 70, 110), vien=(40, 70, 110))
bien("bien_khoa", "INTENSIVE CARE  ·  4TH FLOOR", 1024, 160, (40, 70, 110), (240, 240, 240))
bien("bien_loi_thoat", "EXIT", 512, 192, (20, 140, 60), (240, 255, 240))
bien("bien_loi_ra", "WAY OUT", 512, 192, (240, 240, 230), (20, 120, 50))
bien("bien_cau_thang", "STAIRS", 512, 160, (230, 230, 224), (60, 60, 60), vien=(60, 60, 60))
bien("bien_trong", "", 512, 160, (230, 230, 224), (0, 0, 0))

# 6. Mặt đồng hồ
mt = Image.new("RGB", (512, 512), (238, 236, 228))
d = ImageDraw.Draw(mt)
d.ellipse([6, 6, 506, 506], outline=(30, 30, 30), width=14)
f = ImageFont.truetype(FONT, 52)
for k in range(1, 13):
    a = math.radians(k * 30)
    d.text((256 + 190 * math.sin(a), 256 - 190 * math.cos(a)), str(k), font=f, fill=(25, 25, 25), anchor="mm")
for k in range(60):
    a = math.radians(k * 6); r1 = 236; r2 = 224 if k % 5 else 212
    d.line([(256 + r1 * math.sin(a), 256 - r1 * math.cos(a)), (256 + r2 * math.sin(a), 256 - r2 * math.cos(a))], fill=(25, 25, 25), width=3 if k % 5 else 6)
luu(mt, "mat_dong_ho")

# 7. Mặt tiền tòa nhà đối diện (ban đêm, ô cửa sáng tối)
mw, mh = 1024, 1024
mtien = Image.new("RGB", (mw, mh), (26, 28, 34))
mtien = nhan_mau(mtien, nhieu(mw, mh, 64, 128, 10, 13), 1.0)
d = ImageDraw.Draw(mtien)
for i in range(8):
    for j in range(6):
        x0 = 30 + i * 125; y0 = 40 + j * 165
        sang = random.random()
        mau = (230, 200, 140) if sang > 0.82 else ((140, 170, 190) if sang > 0.74 else (12, 14, 18))
        d.rectangle([x0, y0, x0 + 80, y0 + 100], fill=mau)
luu(mtien, "mat_tien")


def decal(ten, ve, w=1024, h=1024):
    """Ảnh RGBA: ve(draw_rgba) vẽ lên nền trong suốt."""
    im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    ve(im)
    luu(im, ten)


MAU_MAU = (92, 6, 6)


def ve_chu(im):
    d = ImageDraw.Draw(im)
    f = ImageFont.truetype(FONT_TAY, 170)
    d.text((512, 330), "DON'T", font=f, fill=MAU_MAU + (235,), anchor="mm")
    d.text((512, 600), "LOOK BACK", font=f, fill=MAU_MAU + (235,), anchor="mm")
    a = im.split()[3]
    # vệt chảy xuống từ chữ
    d2 = ImageDraw.Draw(a)
    for _ in range(26):
        x = random.randrange(150, 880); y = random.randrange(380, 700)
        d2.line([(x, y), (x + random.randint(-3, 3), y + random.randint(40, 300))], fill=200, width=random.randint(3, 7))
    a = a.filter(ImageFilter.GaussianBlur(1.2))
    im.paste(Image.new("RGBA", im.size, MAU_MAU + (255,)), (0, 0), a)


decal("chu_mau", ve_chu)


def ve_tay(im):
    a = Image.new("L", im.size, 0)
    d = ImageDraw.Draw(a)
    d.ellipse([340, 430, 680, 820], fill=230)                      # lòng bàn tay
    for (cx, dai, goc) in ((390, 260, -14), (465, 320, -5), (545, 330, 4), (620, 290, 13)):
        for t in range(0, dai, 6):
            x = cx + math.sin(math.radians(goc)) * t; y = 470 - t
            d.ellipse([x - 34, y - 34, x + 34, y + 34], fill=230)
    for t in range(0, 220, 6):  # ngón cái
        x = 360 - t * 0.9; y = 640 - t * 0.55
        d.ellipse([x - 38, y - 38, x + 38, y + 38], fill=230)
    a = ImageChops.multiply(a, nhieu(1024, 1024, 12, 200, 60, 14))
    for _ in range(10):
        x = random.randrange(380, 660); y = random.randrange(700, 800)
        d.line([(x, y), (x, y + random.randint(60, 220))], fill=180, width=random.randint(4, 9))
    a = a.filter(ImageFilter.GaussianBlur(2))
    im.paste(Image.new("RGBA", im.size, MAU_MAU + (255,)), (0, 0), a)


decal("dau_tay", ve_tay)


def ve_vet(im):
    a = Image.new("L", im.size, 0)
    d = ImageDraw.Draw(a)
    x, y, r = 80, 512, 70
    while x < 960:
        d.ellipse([x - r, y - r * 0.7, x + r, y + r * 0.7], fill=random.randint(150, 230))
        x += random.randint(10, 26); y += random.randint(-8, 8); r = max(20, min(90, r + random.randint(-6, 6)))
    for _ in range(120):
        px, py = random.randrange(40, 990), random.randrange(330, 700)
        rr = random.randint(3, 14); d.ellipse([px - rr, py - rr, px + rr, py + rr], fill=random.randint(120, 220))
    a = ImageChops.multiply(a, nhieu(1024, 1024, 10, 210, 50, 15)).filter(ImageFilter.GaussianBlur(3))
    im.paste(Image.new("RGBA", im.size, MAU_MAU + (255,)), (0, 0), a)


decal("vet_mau", ve_vet)


def ve_nuoc(im):
    a = nhieu(1024, 1024, 128, 0, 1, 16)
    d = ImageDraw.Draw(a)
    d.ellipse([200, 260, 820, 760], fill=160)
    d.ellipse([560, 520, 900, 820], fill=140)
    a = a.filter(ImageFilter.GaussianBlur(30))
    im.paste(Image.new("RGBA", im.size, (20, 22, 24, 255)), (0, 0), a)


decal("vung_nuoc", ve_nuoc)


def ve_cao(im):
    """Vết cào trên tường."""
    a = Image.new("L", im.size, 0)
    d = ImageDraw.Draw(a)
    for k in range(4):
        x0 = 330 + k * 95
        pts = [(x0 + random.randint(-6, 6) + t * 0.22, 150 + t) for t in range(0, 720, 30)]
        d.line(pts, fill=220, width=12 - k)
    a = a.filter(ImageFilter.GaussianBlur(1.5))
    im.paste(Image.new("RGBA", im.size, (40, 10, 8, 255)), (0, 0), a)


decal("vet_cao", ve_cao)


def ve_mua_kinh(im):
    """Vài vệt nước mảnh chảy dọc (lặp liền theo chiều dọc để trượt xuống chậm)."""
    a = Image.new("L", im.size, 0)
    d = ImageDraw.Draw(a)
    w, h = im.size
    for _ in range(26):
        x = random.randrange(w); y0 = random.randrange(h); dai = random.randrange(120, 520)
        for dy in (-h, 0, h):
            pts, xx = [], x
            for t in range(0, dai, 8):
                xx += random.choice((-1, 0, 0, 0, 1))
                pts.append((xx, y0 + t + dy))
            d.line(pts, fill=random.randint(40, 90), width=random.choice((1, 2)))
            d.ellipse([xx - 2, y0 + dai + dy - 2, xx + 2, y0 + dai + dy + 3], fill=110)
    a = a.filter(ImageFilter.GaussianBlur(0.6))
    im.paste(Image.new("RGBA", im.size, (150, 160, 170, 255)), (0, 0), a)


def ve_giot_kinh(im):
    """Giọt nước nhỏ đứng yên, mờ, có viền sáng nhẹ."""
    a = Image.new("L", im.size, 0)
    d = ImageDraw.Draw(a)
    w, h = im.size
    for _ in range(1400):
        x, y = random.randrange(w), random.randrange(h)
        r = random.choice((1, 1, 1, 2, 2, 3))
        d.ellipse([x - r, y - r, x + r, y + int(r * 1.3)], fill=random.randint(35, 85))
    a = a.filter(ImageFilter.GaussianBlur(0.5))
    im.paste(Image.new("RGBA", im.size, (140, 150, 160, 255)), (0, 0), a)


decal("giot_kinh", ve_giot_kinh, 1024, 1024)
decal("vet_mua_kinh", ve_mua_kinh, 512, 1024)


def ve_tay_kinh(im):
    """Dấu tay mờ (mỡ / hơi nước) trên kính."""
    tam = Image.new("RGBA", im.size, (0, 0, 0, 0))
    ve_tay(tam)
    a = tam.split()[3].point(lambda v: int(v * 0.55))
    a = ImageChops.multiply(a, nhieu(im.size[0], im.size[1], 6, 220, 60, 17))
    im.paste(Image.new("RGBA", im.size, (215, 222, 228, 255)), (0, 0), a)


decal("tay_kinh", ve_tay_kinh)
print("xong")
