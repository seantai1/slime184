#!/usr/bin/env python3
"""Splice blender_slime.obj into slime_on_ground.dae, replacing the sphere primitive
with a polygon mesh. Output: slime_on_ground_mesh.dae."""

import re
from pathlib import Path

ROOT = Path(__file__).parent
OBJ = ROOT / "blender_slime.obj"
SRC_DAE = ROOT / "slime_on_ground.dae"
DST_DAE = ROOT / "slime_on_ground_mesh.dae"

positions, normals, texcoords, faces = [], [], [], []

with open(OBJ) as f:
    for line in f:
        if line.startswith("v "):
            _, x, y, z = line.split()
            positions.append((float(x), float(y), float(z)))
        elif line.startswith("vn "):
            _, x, y, z = line.split()
            normals.append((float(x), float(y), float(z)))
        elif line.startswith("vt "):
            parts = line.split()
            texcoords.append((float(parts[1]), float(parts[2])))
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

print(f"parsed {len(positions)} verts, {len(normals)} normals, "
      f"{len(texcoords)} texcoords, {len(faces)} faces")

pos_str = " ".join(f"{x:.6f} {y:.6f} {z:.6f}" for x, y, z in positions)
nrm_str = " ".join(f"{x:.6f} {y:.6f} {z:.6f}" for x, y, z in normals)
tex_str = " ".join(f"{u:.6f} {v:.6f}" for u, v in texcoords)
vcount_str = " ".join(str(len(f)) for f in faces)
# Match floor's input ordering: VERTEX(0) NORMAL(1) TEXCOORD(2)
p_str = " ".join(
    f"{v} {vn} {vt}" for face in faces for (v, vt, vn) in face
)

geom_block = f'''<geometry id="Slime-data" name="Slime">
      <mesh>
        <source id="Slime-positions">
          <float_array id="Slime-positions-array" count="{len(positions) * 3}">{pos_str}</float_array>
          <technique_common>
            <accessor source="#Slime-positions-array" count="{len(positions)}" stride="3">
              <param name="X" type="float"/>
              <param name="Y" type="float"/>
              <param name="Z" type="float"/>
            </accessor>
          </technique_common>
        </source>
        <source id="Slime-normals">
          <float_array id="Slime-normals-array" count="{len(normals) * 3}">{nrm_str}</float_array>
          <technique_common>
            <accessor source="#Slime-normals-array" count="{len(normals)}" stride="3">
              <param name="X" type="float"/>
              <param name="Y" type="float"/>
              <param name="Z" type="float"/>
            </accessor>
          </technique_common>
        </source>
        <source id="Slime-map-0">
          <float_array id="Slime-map-0-array" count="{len(texcoords) * 2}">{tex_str}</float_array>
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
          <vcount>{vcount_str}</vcount>
          <p>{p_str}</p>
        </polylist>
      </mesh>
    </geometry>'''

dae = SRC_DAE.read_text()

# 1. Replace the <geometry id="Slime-data"> ... </geometry> block.
dae = re.sub(
    r'<geometry id="Slime-data".*?</geometry>',
    geom_block,
    dae,
    count=1,
    flags=re.DOTALL,
)

# 2. The OBJ contains absolute world-space coordinates (bottom at y=0), so the
# slime node's translate-by-0.8-y transform must become identity.
dae = re.sub(
    r'(<node id="Slime"[^>]*>\s*<matrix sid="transform">)[^<]+(</matrix>)',
    r'\g<1>1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1\g<2>',
    dae,
    count=1,
)

DST_DAE.write_text(dae)
print(f"wrote {DST_DAE}  ({DST_DAE.stat().st_size / 1e6:.1f} MB)")
