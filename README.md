# Slimelight

Fork of the CS184 Project 3-2 path tracer, extended with the features
needed for the final project: subsurface scattering, wavelength-dependent
dispersion, and HDR environment lighting.

See `index.html` for the full project proposal.

## Status

| Week | Feature | Status |
|------|---------|--------|
| 1 | Fork + build | done |
| 1 | P3-2 glass/refraction BSDFs finished | done |
| 1 | Homogeneous-medium random-walk SSS | done |
| 1 | COLLADA `<sigma_a>/<sigma_s>/<phase_g>` tags | done |
| 1 | Validation renders | done |
| 2 | Spectral / hero-wavelength sampling | done |
| 2 | Dispersive dielectric (Sellmeier / Cauchy) | done |
| 2 | HDR environment map + importance sampling | done |
| 2 | Multiple importance sampling | done |
| 3 | Hero scene authoring | pending |
| 3 | Slime mesh | pending |
| 4 | Ablation grid + writeup | pending |

## Building (macOS / Linux)

```bash
mkdir -p build && cd build
cmake ..
make -j4
```

Produces `build/pathtracer`.

## Rendering

From `build/`:

```bash
# Clear glass Cornell box (regression baseline):
./pathtracer -t 8 -s 32 -l 4 -m 6 -f ../validation/out_glass_baseline.png \
    ../dae/sky/CBspheres.dae

# Translucent SSS sphere (jade-ish parameters):
./pathtracer -t 8 -s 32 -l 4 -m 6 -f ../validation/out_sss_sphere.png \
    ../dae/slime/CBsss_sphere.dae

# Heavy absorption (Beer-Lambert sanity test):
./pathtracer -t 8 -s 32 -l 4 -m 6 -f ../validation/out_sss_absorb_heavy.png \
    ../dae/slime/CBsss_zero_s.dae

# Week 2 spectral + dispersive slime render:
./pathtracer -t 8 -s 64 -l 4 -m 6 -S -f ../validation/out_spectral_slime.png \
    ../dae/slime/CBsss_sphere.dae

# With an EXR environment map:
./pathtracer -t 8 -s 64 -l 4 -m 6 -S -e ../assets/hdri/your_map.exr \
    -f ../validation/out_env_slime.png ../dae/slime/CBsss_sphere.dae
```

Command-line flags:
- `-t N` threads
- `-s N` camera samples per pixel
- `-l N` samples per area light
- `-m N` max ray depth
- `-S` enable hero-wavelength spectral sampling
- `-e PATH` load an OpenEXR lat-long environment map
- `-f PATH` output image

## Adding SSS to a glass material

In a `.dae` scene, inside the `<technique profile="CGL"><glass>` block, add
any of these optional children (backward compatible — absence disables SSS):

```xml
<glass>
  <reflectance>1 1 1</reflectance>
  <transmittance>1 1 1</transmittance>
  <roughness>0</roughness>
  <ior>1.33</ior>
  <dispersion>0.003</dispersion>
  <sigma_a>0.02 0.05 0.02</sigma_a>
  <sigma_s>3.0 3.0 3.0</sigma_s>
  <phase_g>0.0</phase_g>
</glass>
```

Units: inverse scene distance. `sigma_t = sigma_a + sigma_s` sets the mean
free path (1/σ_t). For Week 1 the phase function is isotropic — `phase_g`
is parsed but the Henyey–Greenstein code for nonzero g is deferred.

Dispersion is optional and only affects renders when `-S` is enabled. Use
`<dispersion>` or `<cauchy_b>` for the Cauchy B coefficient in micrometers
squared; if `<cauchy_a>` is omitted, the parser chooses A so the material keeps
its listed `<ior>` at 550 nm. For measured glass, use Sellmeier coefficients:

```xml
<sellmeier_b>1.03961212 0.231792344 1.01046945</sellmeier_b>
<sellmeier_c>0.00600069867 0.0200179144 103.560653</sellmeier_c>
```

## Repository layout

- `src/pathtracer/medium.{h,cpp}` — `HomogeneousMedium` with Beer-Lambert
  transmittance, exponential free-flight sampling, isotropic phase.
- `src/pathtracer/pathtracer.cpp::random_walk_radiance` — the volumetric
  random walk; called from `at_least_one_bounce_radiance` when a refract-in
  direction sends the ray into a medium.
- `src/pathtracer/advanced_bsdf.cpp` — completed P3-2 Mirror/Refraction/Glass
  BSDFs + `reflect`/`refract` helpers, including wavelength-dependent IOR.
- `src/pathtracer/spectral.h` — hero-wavelength sampling and RGB
  reconstruction weights.
- `src/scene/environment_light.cpp` — EXR lat-long lookup, luminance/solid-angle
  importance sampling, and pdf queries for MIS.
- `src/scene/collada/collada.cpp` — extended glass parser.
- `dae/slime/` — our custom SSS test scenes.
- `assets/hdri/`, `assets/meshes/` — Week 2+ assets (placeholders for now).
- `validation/` — reference renders for the writeup.

## References

- PBRT 4th ed., volumes and SSS chapters.
- Ray Tracing: The Next Week (Shirley) — intro to volumetric scattering.
- Mitsuba `homogeneous` medium docs — our reference for parameter semantics.

## Upstream

Forked from `hw3-pathtracer-seanmoo` (CS184 Project 3-2 starter). The
original README is kept as `README_p3.md`.
