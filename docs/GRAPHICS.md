# Graphics and camera settings

Use **Graphics** in the launcher or overlay. The same settings are available
in both, with the game kept visible behind the overlay.

## Accurate and Modern

Accurate keeps the original 4:3/30 FPS presentation as the reference.
Modern opens widescreen, higher presentation rates, filtering, HUD and scenery
options. Interpolation improves the image between authored frames; it does not
make the simulation run faster.

Battery, Balanced and Quality are optional presets. Nothing changes until you
press Apply. Start at 60 FPS when diagnosing performance, then raise the target
once your chosen settings are steady. More pixels, MSAA, supersampling, retained
scenery and high-detail models can all add work.

## HUD and camera

**HUD layout** offers **4:3** or **Fit to window** for Modern one/two-player
presentation. Whole groups move together; their artwork is not stretched.
Three/four-player HUD placement remains separate. The custom HUD editor and
size controls are not enabled. See [HUD layouts](HUD-WORKSHOP.md).

FOV, clipping and scenery controls affect what the camera shows. The near-plane
and camera-obstruction improvements reduce seeing through nearby surfaces;
report reproducible gaps with the track, position, aspect ratio and FOV.

The optional **Modern camera** is for local races. Enable it and use
**Tune the Modern camera (F7)** for its tuning panel. It is disabled online;
changing a local follow camera inside shared simulation is not treated as a
safe renderer-only setting.

The camera keeps up with the kart through a steady corner and sits at the
tuned distance at speed; the kart turns on screen only while a turn starts or
ends, and when a drift starts or ends the view realigns over about half a
second. It does not roll, and its field of view breathes only a little with
speed. On a wall or kart hit the camera moves back with the bounce, as the
original camera does, and its aim, zoom and FOV ignore the stop until the
kart has its speed back; the **Hits** slider in the panel sets how long it
waits for a kart that stays slow. A `modern-camera.ini` saved by an earlier
build is updated on the first launch: values you had not changed follow the
new defaults, and a changed **Distance at top speed** grows by the lag the
old position spring used to add.

## Vehicles and scenery

Maximum vehicle detail, keep-scenery switches, the ultrawide guard, view
distances and retention let you choose how much remains visible. Bigger ranges
can increase CPU work as well as GPU load. Turning them up is not a free
performance improvement.

Experimental Rollback restores eligible retail scenery billboards in a local
visual cache, including removed tree tops. Keep-scenery, view distance and
retention apply to that cache; they do not yet extend every actor or level
mesh. Vehicle LOD, FOV, animated/effect ranges, water detail and CPU culling
remain canonical there. Your saved preferences remain available for offline
and legacy online play.

## Ray tracing and water

The **Ray tracing & water** section has:

- **Water quality:** Original, Enhanced or High.
- **Reflections:** Sky, Screen-space or Hardware ray tracing (experimental).
- **Racer wakes** and **Water spray**.

Original keeps the game's water. Enhanced/High add the new water presentation
to eligible flat and animated water. Waterfalls and fountains retain their
authored appearance with added flowing detail. Recognised custom-track water
can use the same renderer path; arbitrary mod surfaces still need valid water
classification.

Screen-space reflections use visible scene colour/depth. They are not hardware
ray tracing and cannot fully reconstruct off-screen objects. The hardware
path needs compatible GPU ray queries and drivers, traces opaque level geometry,
and uses screen-space reflections for moving racers. Off-screen surfaces use
vertex colours rather than texture-pack detail.

Unsupported hardware or scenes exceeding the safety budget fall back to
screen-space reflections. This does not change collisions, buoyancy or online
simulation. High water and hardware tracing can be expensive; compare the same
scene with Original before attributing a slowdown to netplay.

## Textures and sound

Texture packs, their priorities and CRT filters belong in
[Textures](TEXTURE_PACKS.md). Audio settings are covered in [Sound](SOUND.md).
For failures and useful reports, see [troubleshooting](TROUBLESHOOTING.md).
