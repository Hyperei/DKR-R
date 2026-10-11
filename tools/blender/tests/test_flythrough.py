"""Gate for :mod:`flythrough`: the camera flight behind the track select menu.

What is checked, and why it is enough to trust the flight in game:

* **The chain is shaped the way the game walks it** - one camera, on node 0,
  ``order`` counting up from there, a speed on the first node, the loop on the
  last, every node on channel 1.
* **The path is the track.** On a ring of checkpoints every node sits on the
  ring, lifted by the height, and faces along it.
* **The angle convention is retail's.** Across every retail preview node the
  yaw :func:`flythrough.facing` gives agrees with the authored one far more
  often than any other reading of the angle does.
* **A node encodes to retail's bytes.** Built over a retail race, the first
  node and an ordinary one encode to the same bytes as that race's own nodes,
  position and angles aside.
* **An author's flight is never touched**, and an actor index in use is not
  reused.

Run with any Python 3.8+; it does not need Blender.

    python tools/blender/tests/test_flythrough.py
"""

from __future__ import annotations

import glob
import math
import os
import sys

_HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(_HERE))
sys.path.insert(0, _HERE)

from track_lab import (  # noqa: E402
    assets, catalog as catalog_module, flythrough, gltf_io, object_map_encoder,
    race_ai,
)

from test_roundtrip import VANILLA, find_object_maps  # noqa: E402

RADIUS = 6000.0
GATES = 24


def _gate(index, angle_degrees, y=100.0, vehicle=0):
    angle = math.radians(angle_degrees)
    fields = {"scale": 64, "index": index, "angleY": 0.0, "isAltCheckpoint": 0,
              "vehicleType": vehicle}
    fields.update(zip(race_ai.LATERAL_FIELDS, (0, 0, 0, 0)))
    fields.update(zip(race_ai.VERTICAL_FIELDS, (0, 0, 0, 0)))
    return gltf_io.MapObject(
        object_id=race_ai.CHECKPOINT, name="Checkpoint",
        translation=[RADIUS * math.sin(angle), y, RADIUS * math.cos(angle)],
        fields=fields)


def _ring(vehicle=0):
    return [_gate(i, 360.0 * i / GATES, vehicle=vehicle) for i in range(GATES)]


def _preview_nodes(object_map):
    return sorted((o for o in object_map.objects if flythrough.is_preview_node(o)),
                  key=lambda o: (o.fields["actorIndex"], o.fields["order"]))


def _angle_gap(a, b):
    return abs((a - b + 180.0) % 360.0 - 180.0)


# ---------------------------------------------------------------------------

def check_chain_shape():
    nodes = flythrough.build(_ring())
    count = len(nodes)
    if not flythrough.MIN_NODES <= count <= flythrough.MAX_NODES:
        return "%d nodes, outside %d..%d" % (
            count, flythrough.MIN_NODES, flythrough.MAX_NODES)
    if [n.fields["order"] for n in nodes] != list(range(count)):
        return "orders must count up from 0 in document order"
    if any(n.fields["channel"] != flythrough.PREVIEW_CHANNEL for n in nodes):
        return "every node belongs on channel 1, the one the menu plays"
    cameras = [n for n in nodes if "objectIdToSpawn" in n.fields]
    if cameras != nodes[:1] or cameras[0].fields["objectIdToSpawn"] != flythrough.ANIM_CAMERA:
        return "only node 0 spawns, and what it spawns is the animation camera"
    if nodes[0].fields["nodeSpeed"] != flythrough.NODE_SPEED:
        return "node 0 sets the speed"
    if any(n.fields["nodeSpeed"] != -1 for n in nodes[1:]):
        return "later nodes keep the speed (-1)"
    if nodes[-1].fields["goToNode"] != 0 or any(
            n.fields["goToNode"] != -1 for n in nodes[:-1]):
        return "only the last node loops, back to node 0"
    if any("pauseFrameCount" in n.fields for n in nodes):
        return "pauseFrameCount stays absent (-1); 0 pauses on every node"
    print("  %d nodes on a %d-unit ring" % (count, round(2 * math.pi * RADIUS)))
    return None


def check_path_follows_the_ring():
    nodes = flythrough.build(_ring())
    for node in nodes:
        x, y, z = node.translation
        if abs(math.hypot(x, z) - RADIUS) > RADIUS * 0.02:
            return "node at %r is off the ring" % (node.translation,)
        if abs(y - (100.0 + flythrough.HEIGHT)) > 1.0:
            return "node at height %r, expected the road plus %r" % (
                y, flythrough.HEIGHT)
    # The gates run clockwise seen from above, from +Z towards +X, so the
    # camera at a point faces along the tangent (cos a, 0, -sin a).
    for node in nodes:
        x, _y, z = node.translation
        angle = math.atan2(x, z)
        tangent = (math.cos(angle), 0.0, -math.sin(angle))
        expected, _pitch = flythrough.facing(tangent)
        if _angle_gap(node.fields["y_rotation"], expected) > 360.0 / GATES:
            return "node at %r faces %r, the ring runs at %r" % (
                node.translation, node.fields["y_rotation"], expected)
    # Flat road: looking down by the tilt, as a u8 angle.
    pitch = flythrough.quantise(-flythrough.TILT)
    if any(_angle_gap(n.fields["x_rotation"], pitch) > 1e-6 for n in nodes):
        return "a flat ring should look down by %r everywhere" % flythrough.TILT
    return None


def check_set_choice():
    plane_only = _ring(vehicle=2)
    nodes = flythrough.build(plane_only, preferred_set=0)
    if not nodes:
        return "an empty preferred set should fall back to the set that has a lap"
    try:
        flythrough.build(_ring()[:2])
    except flythrough.FlythroughError:
        pass
    else:
        return "two checkpoints are not a lap; build should refuse"
    return None


def check_author_flight_kept():
    ring = _ring()
    nodes = flythrough.build(ring)
    if not flythrough.has_preview(ring + nodes[:1]):
        return "a channel-1 Animation is a preview flight"
    other = gltf_io.MapObject(object_id=flythrough.ANIMATION, name="Animation",
                              translation=[0, 0, 0],
                              fields={"channel": 0, "actorIndex": 0})
    if flythrough.has_preview(ring + [other]):
        return "a channel-0 Animation is a race cutscene, not the preview"
    if flythrough.build(ring + [other])[0].fields["actorIndex"] != 1:
        return "actor 0 is taken by the cutscene; the flight needs another"
    return None


def check_retail_convention():
    """Our yaw reading against retail's authored angles, and the alternatives."""
    readings = {
        "ours": lambda dx, dz: math.degrees(math.atan2(dx, dz)) - 180.0,
        "without the half turn": lambda dx, dz: math.degrees(math.atan2(dx, dz)),
        "mirrored": lambda dx, dz: math.degrees(math.atan2(-dx, dz)) - 180.0,
    }
    hits = {name: 0 for name in readings}
    total = 0
    for path in find_object_maps():
        nodes = _preview_nodes(gltf_io.load(path))
        for i, node in enumerate(nodes):
            ahead, behind = nodes[(i + 1) % len(nodes)], nodes[i - 1]
            dx = ahead.translation[0] - behind.translation[0]
            dz = ahead.translation[2] - behind.translation[2]
            total += 1
            for name, reading in readings.items():
                if _angle_gap(node.fields["y_rotation"], reading(dx, dz)) < 30.0:
                    hits[name] += 1
    if not total:
        print("  skip: no extracted retail object maps")
        return None
    ours = hits.pop("ours")
    print("  %d of %d retail nodes face along the path; best other reading %d"
          % (ours, total, max(hits.values())))
    if ours * 2 < total or any(ours < 2 * other for other in hits.values()):
        return "the yaw convention does not match retail's: %r" % hits
    return None


def check_retail_bytes():
    """Built over a retail race, nodes encode like that race's own nodes."""
    tree = None
    for version in sorted(glob.glob(os.path.join(VANILLA, "*"))):
        tree = assets.AssetTree.find(version)
        if tree is not None:
            break
    table = tree.translation_table() if tree else []
    if not table:
        print("  skip: no extracted assets with a translation table")
        return None
    catalog = catalog_module.load()
    encoder = object_map_encoder.ObjectMapEncoder(catalog, table, tree.asset_index)

    races = 0
    for path in find_object_maps():
        object_map = gltf_io.load(path)
        retail = _preview_nodes(object_map)
        if len(retail) < 3 or retail[0].fields.get("actorIndex") != 0:
            continue
        rest = [o for o in object_map.objects if not flythrough.is_preview_node(o)]
        if flythrough.has_preview(rest) or flythrough.choose_route(rest) is None:
            continue
        built = flythrough.build(rest)
        # Animation actors in a race map are cutscenes on other channels; the
        # retail flight shares actor 0 with them, ours steps past. Compare
        # with the actor byte (offset 0x10) aside, and position and angles.
        # Two bytes retail is not consistent about, and the game reads neither
        # difference: ``unk13`` is used only when it is not negative (-1 and
        # -128 both mean "leave the particles"), and ``goToNode`` only on the
        # chain's last node.
        skip = set(range(2, 11)) | {0x10, 0x11}

        def same(i, a, b, last):
            if i == 0x13:
                return (a[i] >= 0x80) == (b[i] >= 0x80) or a[i] == b[i]
            if i == 0x1D and not last:
                return True
            return a[i] == b[i]

        for ours, theirs in ((built[0], retail[0]), (built[1], retail[1]),
                             (built[-1], retail[-1])):
            a, b = encoder.encode_entry(ours), encoder.encode_entry(theirs)
            last = ours is built[-1]
            diff = [i for i in range(len(a))
                    if i not in skip and not same(i, a, b, last)]
            if len(a) != len(b) or diff:
                return "%s: node %d differs from retail at %s" % (
                    os.path.basename(path), ours.fields["order"],
                    ", ".join("0x%02X" % i for i in diff))
        races += 1
    print("  %d retail races rebuilt" % races)
    if not races:
        return "no retail race could be rebuilt"
    return None


CHECKS = [
    ("chain shape", check_chain_shape),
    ("path follows the ring", check_path_follows_the_ring),
    ("checkpoint set choice", check_set_choice),
    ("author flight kept", check_author_flight_kept),
    ("retail angle convention", check_retail_convention),
    ("retail bytes", check_retail_bytes),
]


def main():
    failures = 0
    for name, check in CHECKS:
        problem = check()
        if problem:
            failures += 1
            print("FAIL %s: %s" % (name, problem))
        else:
            print("ok   %s" % name)
    print()
    print("FAIL: %d check(s)" % failures if failures else "PASS: flythrough")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
