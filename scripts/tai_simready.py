# Tải các mẫu SimReady đã chọn (bỏ source_assets trùng lặp và ảnh .thumbs).
import json, os, sys, urllib.request, urllib.parse, concurrent.futures as cf
ds = sys.argv[1]; out = sys.argv[2]
BASE = "https://huggingface.co/datasets/nvidia/simready-assets/resolve/main/"
jobs = []
for f in sorted(os.listdir(ds)):
    for i in json.load(open(os.path.join(ds, f), encoding="utf-8")):
        p = i["path"]
        if i["type"] != "file" or "/source_assets/" in p or "/.thumbs/" in p:
            continue
        dst = os.path.join(out, *p.split("/"))
        if os.path.exists(dst) and os.path.getsize(dst) == i.get("size", -1):
            continue
        jobs.append((p, dst, i.get("size", 0)))
print("can tai", len(jobs), "file", round(sum(j[2] for j in jobs) / 1e6), "MB", flush=True)
def get(j):
    p, dst, _ = j
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    urllib.request.urlretrieve(BASE + urllib.parse.quote(p), dst + ".part")
    os.replace(dst + ".part", dst)
    return p
n = 0
with cf.ThreadPoolExecutor(6) as ex:
    for p in ex.map(get, jobs):
        n += 1
        if n % 50 == 0: print(n, "/", len(jobs), flush=True)
print("xong", n, flush=True)
