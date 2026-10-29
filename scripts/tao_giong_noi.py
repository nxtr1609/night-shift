# Giọng nữ hoảng sợ trong điện thoại (tiếng Anh) — Piper TTS, theo cách VoicePi của Kana (audio/piper_tts.py).
# Giọng en_US-ljspeech-high (bộ dữ liệu LJSpeech, public domain).
# Chạy bằng python của venv Piper: python tao_giong_noi.py <model.onnx> <thư mục NguonAmThanh>
import os, sys
import numpy as np
import soundfile as sf
from piper import PiperVoice, SynthesisConfig

MODEL, RA = sys.argv[1], sys.argv[2]
SR = 44100
voice = PiperVoice.load(MODEL)
rng = np.random.default_rng(403)

# (câu, độ nhanh: <1 nhanh hơn, khoảng lặng sau câu)
CAU = [
    ("Hello? Hello?! Oh God... someone's there.", 0.92, 0.55),
    ("I'm in room four oh three. I can't get out... she's in here with me.", 0.9, 0.7),
    ("Don't come in. Whatever you do... don't look back.", 1.0, 0.4),
]


def doc(cau, toc):
    cfg = SynthesisConfig(length_scale=toc, noise_scale=0.85, noise_w_scale=1.0)
    pcm = b"".join(c.audio_int16_bytes for c in voice.synthesize(cau, cfg))
    a = np.frombuffer(pcm, dtype=np.int16).astype(np.float32) / 32768.0
    sr0 = voice.config.sample_rate
    # nâng giọng một chút (hoảng) + đổi sang 44,1 kHz
    nang = 1.06
    t = np.arange(0, len(a) * SR / sr0 / nang) * sr0 * nang / SR
    a = np.interp(t, np.arange(len(a)), a)
    # run giọng: rung biên độ + rung cao độ nhẹ (đọc lệch thời gian theo sóng sin)
    n = np.arange(len(a))
    lech = 0.0018 * SR * np.sin(2 * np.pi * 5.5 * n / SR + rng.uniform(0, 6))
    a = np.interp(np.clip(n + lech, 0, len(a) - 1), n, a)
    a *= 1.0 + 0.18 * np.sin(2 * np.pi * 7.0 * n / SR)
    return a


def loc_dien_thoai(a):
    # dải tần đường dây điện thoại 300–3400 Hz + méo nhẹ
    f = np.fft.rfft(a)
    hz = np.fft.rfftfreq(len(a), 1 / SR)
    f *= ((hz > 300) & (hz < 3400)).astype(np.float32)
    a = np.fft.irfft(f, len(a))
    a = np.tanh(a * 2.2) / np.tanh(2.2)
    return a


def nhieu(n, muc):
    x = rng.normal(0, 1, n).astype(np.float32)
    x = np.convolve(x, np.ones(6) / 6, mode="same")  # rè mềm
    lop = np.zeros(n, np.float32)
    for _ in range(max(1, n // 9000)):          # lách tách
        k = rng.integers(0, n); lop[k:k + 40] += rng.uniform(0.3, 0.8)
    return (x * 0.5 + lop) * muc


doan, moc = [], []
dau = int(0.7 * SR)
doan.append(np.zeros(dau, np.float32)); vi_tri = dau
for cau, toc, nghi in CAU:
    a = doc(cau, toc)
    moc.append((round(vi_tri / SR, 2), cau))
    doan.append(a); vi_tri += len(a)
    k = int(nghi * SR); doan.append(np.zeros(k, np.float32)); vi_tri += k
# đứt máy: rè to rồi im
cut = int(0.5 * SR)
doan.append(nhieu(cut, 0.35)); vi_tri += cut
am = np.concatenate(doan)
am = loc_dien_thoai(am)
am += nhieu(len(am), 0.025)
am[-cut:] += nhieu(cut, 0.25)
am = am / (np.max(np.abs(am)) + 1e-6) * 0.89
sf.write(os.path.join(RA, "dt_giong.wav"), am.astype(np.float32), SR, subtype="PCM_16")
print("DAI", round(len(am) / SR, 2))
for t, c in moc:
    print("MOC", t, c)
