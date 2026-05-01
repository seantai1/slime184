#!/usr/bin/env python3
"""Convert per-frame OBJs (frames/slime_NNNN.obj) into per-frame DAEs.

Computes the global min Y across all frames so we can shift everything up so
the slime's lowest point lands at Y=0 (on the floor). Re-uses the same scene
template (lights, camera, materials) from slime_on_ground.dae for each frame.
"""

import re
from pathlib import Path

ROOT = Path(__file__).parent
FRAMES_DIR = ROOT / "frames"
SRC_DAE = ROOT / "slime_on_ground.dae"
START, END = 1, 41

template = SRC_DAE.read_text()


def parse_obj(path):
    positions, normals, texcoords, faces = [], [], [], []
    with open(path) as f:
        for line in f:
            if line.startswith("v "):
                _, x, y, z = line.split()
                positions.append((float(x), float(y), float(z)))
            elif line.startswith("vn "):
                _, x, y, z = line.split()
                normals.append((float(x), float(y), float(z)))
            elif line.startswith("vt "):
                p = line.split()
                texcoords.append((float(p[1]), float(p[2])))
            elif line.startswith("f "):
                face = []
                for tok in line.split()[1:]:
                    v, vt, vn = (tok.split("/") + ["", ""])[:3]
                    face.append((int(v) - 1,
                                 int(vt) - 1 if vt else 0,
                                 int(vn) - 1 if vn else 0))
                faces.append(face)
    if not texcoords:
        texcoords = [(0.0, 0.0)]
    return positions, normals, texcoords, faces


# Pass 1: scan all frames, find global Y range so we can shift up to ground
# AND so we know the highest point any frame ever reaches.
print("scanning frames for Y range...")
global_min_y = float("inf")
global_max_y_after_shift = float("-inf")
parsed = {}
for i in range(START, END + 1):
    p, n, t, f = parse_obj(FRAMES_DIR / f"slime_{i:04d}.obj")
    parsed[i] = (p, n, t, f)
    frame_min = min(y for _, y, _ in p)
    frame_max = max(y for _, y, _ in p)
    global_min_y = min(global_min_y, frame_min)
    print(f"  frame {i:02d}: y={frame_min:.3f}..{frame_max:.3f}")

y_shift = -global_min_y + 0.05  # tiny lift to avoid coincidence with floor at Y=0
# After shifting, the highest point the slime ever reaches is the max Y of the
# uppermost frame plus the shift. We add an anchor vert at exactly this height
# so the scene bbox is identical across all frames, preventing the renderer's
# auto-camera from drifting frame-to-frame.
global_max_y_after_shift = max(
    max(y for _, y, _ in parsed[i][0]) + y_shift for i in range(START, END + 1)
)
print(f"global min Y = {global_min_y:.3f}, shifting all frames by +{y_shift:.3f}")
print(f"anchor vertex at world Y = {global_max_y_after_shift:.3f}")


def build_geom_block(positions, normals, texcoords, faces, y_offset):
    pos = " ".join(
        f"{x:.6f} {y + y_offset:.6f} {z:.6f}" for x, y, z in positions
    )
    nrm = " ".join(f"{x:.6f} {y:.6f} {z:.6f}" for x, y, z in normals)
    tex = " ".join(f"{u:.6f} {v:.6f}" for u, v in texcoords)
    vcount = " ".join(str(len(f)) for f in faces)
    p_idx = " ".join(
        f"{v} {vn} {vt}" for face in faces for (v, vt, vn) in face
    )
    return f'''<geometry id="Slime-data" name="Slime">
      <mesh>
        <source id="Slime-positions">
          <float_array id="Slime-positions-array" count="{len(positions) * 3}">{pos}</float_array>
          <technique_common>
            <accessor source="#Slime-positions-array" count="{len(positions)}" stride="3">
              <param name="X" type="float"/>
              <param name="Y" type="float"/>
              <param name="Z" type="float"/>
            </accessor>
          </technique_common>
        </source>
        <source id="Slime-normals">
          <float_array id="Slime-normals-array" count="{len(normals) * 3}">{nrm}</float_array>
          <technique_common>
            <accessor source="#Slime-normals-array" count="{len(normals)}" stride="3">
              <param name="X" type="float"/>
              <param name="Y" type="float"/>
              <param name="Z" type="float"/>
            </accessor>
          </technique_common>
        </source>
        <source id="Slime-map-0">
          <float_array id="Slime-map-0-array" count="{len(texcoords) * 2}">{tex}</float_array>
          <technique_common>
            <accessor source="#Slime-map-0-array" count="{len(texcoords)}" stride="2">
              <param name="S" type="float"/>
              <param name="T" type="float"/>
            </accessor>
          </technique_common>
        </source>
        <vertices id="Slime-vertices">
          <input semantic="POSITION" source="#Slime-positions"/>
        </vertices>
        <polylist material="slime-material" count="{len(faces)}">
          <input semantic="VERTEX" source="#Slime-vertices" offset="0"/>
          <input semantic="NORMAL" source="#Slime-normals" offset="1"/>
          <input semantic="TEXCOORD" source="#Slime-map-0" offset="2" set="0"/>
          <vcount>{vcount}</vcount>
          <p>{p_idx}</p>
        </polylist>
      </mesh>
    </geometry>'''


# Pass 2: write per-frame DAE files.
for i in range(START, END + 1):
    p, n, t, f = parsed[i]
    # Inject 3 anchor vertices at the highest Y any frame ever reaches. The
    # renderer rejects polygons with duplicate vertex indices, so we use 3
    # distinct vertex indices with tiny coincident positions (sub-pixel area,
    # effectively invisible). Subtracting y_shift here because build_geom_block
    # will re-add +y_shift; we want post-shift world Y to be exactly the global max.
    base_y = global_max_y_after_shift - y_shift
    eps = 1e-4
    p = list(p) + [
        (0.0, base_y, 0.0),
        (eps, base_y, 0.0),
        (0.0, base_y, eps),
    ]
    n = list(n) + [(0.0, 1.0, 0.0)]
    a0, a1, a2 = len(p) - 3, len(p) - 2, len(p) - 1
    nrm = len(n) - 1
    f = list(f) + [
        [(a0, 0, nrm), (a1, 0, nrm), (a2, 0, nrm)]
    ]
    dae = re.sub(
        r'<geometry id="Slime-data".*?</geometry>',
        build_geom_block(p, n, t, f, y_shift),
        template,
        count=1,
        flags=re.DOTALL,
    )
    dae = re.sub(
        r'(<node id="Slime"[^>]*>\s*<matrix sid="transform">)[^<]+(</matrix>)',
        r'\g<1>1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1\g<2>',
        dae,
        count=1,
    )
    out = FRAMES_DIR / f"slime_{i:04d}.dae"
    out.write_text(dae)
    print(f"  frame {i:02d} -> {out.name} ({out.stat().st_size / 1e6:.1f} MB)")

print("done.")
