# Tạo âm thanh cho game bằng Audio Studio (Stable Audio 3, của Kana) — gọi thẳng backend, không qua MCP.
# Chạy bằng python của .venv audio-studio-mcp:  python -I -X utf8 tao_am_thanh.py <thư mục am_thanh> [tên ...]
import asyncio, os, sys
import numpy as np
from audio_studio_mcp.backends import create_backend
from audio_studio_mcp.backends.base import GenerationRequest
from audio_studio_mcp.prompts import prepare_prompt
from audio_studio_mcp import post

RA = sys.argv[1]
CHON = set(sys.argv[2:])
os.makedirs(RA, exist_ok=True)

# tên: (kiểu, mô tả, giây, vòng lặp?, mono?, seed)
DS = {
    "nen_den_huynh_quang": ("sfx", "fluorescent ceiling lights electrical hum and faint buzz, empty hospital corridor at night, steady, no music", 20, True, False, 11),
    "nen_mua": ("sfx", "heavy rain against large windows heard from inside a quiet building, distant thunder rumble, steady", 24, True, False, 12),
    "nen_u_am_1": ("sfx", "dark ambient drone, low sub bass rumble, faint distant metallic resonance, unsettling, slow, horror film atmosphere", 30, True, False, 13),
    "nen_u_am_2": ("sfx", "intense horror atmosphere, dissonant bowed strings swelling, deep pulsing drone, scraping metal textures, dread", 30, True, False, 14),
    "nhip_tim": ("sfx", "slow heavy human heartbeat, close, muffled, steady rhythm, dry", 12, True, True, 15),
    "duoi_bat": ("sfx", "frantic horror chase, pounding low drums, screeching dissonant strings, chaotic and terrifying", 20, True, False, 16),
    "binh_minh": ("sfx", "calm early morning outdoor ambience, gentle birdsong, soft breeze, after rain, peaceful", 20, True, False, 17),
    "dong_ho": ("sfx", "old wall clock ticking, mechanical tick tock, quiet room, close mic, dry", 8, True, True, 18),
    "dien_thoai_reo": ("sfx", "old landline desk telephone ringing, mechanical bell ring, indoors", 8, True, True, 19),
    "giot_nuoc": ("sfx", "single water drop dripping into a small puddle, quiet tiled room, slight echo", 3, False, True, 20),
    "buoc_chan": ("sfx", "single footstep of a soft shoe on hospital linoleum floor, close, dry, one step only", 1.5, False, True, 21),
    "cua_mo": ("sfx", "heavy old wooden hospital door slowly opening, long creaking hinge", 3.5, False, True, 22),
    "cua_dong": ("sfx", "wooden door closing softly, latch click", 2, False, True, 23),
    "cua_dap": ("sfx", "heavy door slammed shut violently, loud bang, echo in corridor", 2.5, False, True, 24),
    "go_cua": ("sfx", "three slow heavy knocks on a wooden door from the other side, muffled, echo", 4, False, True, 25),
    "nhieu_dien_thoai": ("sfx", "telephone line static and crackle with slow distorted heavy breathing, eerie, lo-fi", 7, False, True, 26),
    "xe_lan": ("sfx", "wheelchair rolling slowly on hard floor, squeaking wheels, creaking frame", 5, False, True, 27),
    "den_chap": ("sfx", "fluorescent tube flickering, rapid electrical clicks and buzzing, sparks", 2.5, False, True, 28),
    "thi_tham": ("sfx", "eerie unintelligible whispering voices, breathy, ghostly, close, reverb", 6, False, True, 29),
    "tho": ("sfx", "slow heavy raspy breathing very close behind the listener, creepy", 5, False, True, 30),
    "giat_minh": ("sfx", "horror jump scare stinger, sudden loud dissonant orchestral hit with screeching strings", 3, False, False, 31),
    "khay_roi": ("sfx", "metal surgical tray falling on hard floor, loud clatter of instruments", 2.5, False, True, 32),
    "may_tim_bip": ("sfx", "single short beep of a hospital patient heart monitor, electronic, clean", 1, False, True, 33),
    "may_tim_thang": ("sfx", "hospital patient monitor flatline, continuous electronic tone", 4, False, True, 34),
    "tay_nam_cua": ("sfx", "door handle rattling hard, someone trying to open a locked door", 2.5, False, True, 35),
    "cang_xe_day": ("sfx", "hospital gurney rolling slowly, rattling metal wheels, creak", 4, False, True, 36),
    "ro_ri": ("sfx", "low distant inhuman groan echoing in a long empty corridor, horror", 5, False, True, 37),
    "sam_set": ("sfx", "very close lightning strike, violent thunder crack with sharp snap and deep rumbling tail, heard from inside a building", 6, False, False, 39),
    "sam_xa": ("sfx", "distant rolling thunder, long low rumble across the sky, muffled through windows", 8, False, False, 40),
    "cao_kinh": ("sfx", "fingernails scratching slowly across a large glass window, squeaking skin on wet glass, a few soft thumps of palms", 5, False, True, 41),
    "go_kinh": ("sfx", "palm slapping hard against a window glass pane three times, dull thud and glass rattle", 3, False, True, 42),
    "cup_dien": ("sfx", "electrical power cut, heavy breaker clunk, fluorescent lights dying with a falling electric whine", 2.5, False, False, 43),
    "dien_lai": ("sfx", "fluorescent tube lights flickering back on, ballast tick and rising hum", 2.5, False, False, 44),
    "giat_nhe": ("sfx", "short sharp horror sting, high dissonant string screech, quick decay", 1.5, False, False, 45),
    "co_giat": ("sfx", "wet bone cracking and joints popping, twitching body convulsing, creepy, close", 4, False, True, 46),
    "nhen_bo": ("sfx", "many hard spindly legs skittering fast across a wall, rapid chitinous clicking and scratching, insect-like, close, dry", 8, True, True, 47),
    "trailer_nhac": ("music", "Dark cinematic horror trailer score, slow ominous low drone and sparse detuned music box notes, building dread with dissonant bowed strings and ticking pulses, rising tension into heavy sub bass braams and a final huge impact, tense and terrifying", 75, False, False, 48),
    "ket_thuc": ("sfx", "soft warm ambient pad, gentle resolution, calm and sad, slow fade", 14, False, False, 38),
}

def can_rms(a, muc_db):
    rms = float(np.sqrt(np.mean(a ** 2))) + 1e-9
    return a * (post.db_to_lin(muc_db) / rms)

async def chay():
    be = create_backend()
    sr = be.sample_rate
    for ten, (kieu, mo_ta, giay, lap, mono, seed) in DS.items():
        if CHON and ten not in CHON:
            continue
        text, _ = prepare_prompt(mo_ta, kieu, giay)
        res = await be.generate([seed], GenerationRequest(prompt=text, seconds=float(max(giay, 2))))
        a = res.audios[0].astype(np.float32)
        if mono:
            a = post.to_mono(a)
        if lap:
            # bỏ 1 s đầu và phần nhỏ dần ở cuối, ghép vòng lặp liền
            cuoi = min(a.shape[-1] / sr, post.audible_end(a, sr, -45.0)) - 0.8
            a = post.trim(a, sr, 1.0, max(cuoi, 2.5))
            a = post.make_loop(a, sr, 400)
            a = can_rms(a, -20.0)
        else:
            a = post.trim_silence(a, sr, -48.0, 10)
            a = post.fade(a, sr, 3, 60)
        a, _ = post.peak_ceiling(a, -1.0) if lap else post.peak_normalize(a, -1.0)
        duong = os.path.join(RA, ten + ".wav")
        post.write(duong, a, sr, "wav16")
        print("OK", ten, round(a.shape[-1] / sr, 2), "s", round(res.wall_seconds, 1), "s tao", flush=True)

asyncio.run(chay())
