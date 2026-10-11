"""The camera flight the track select screen plays over a track.

Hovering a track on the track select screen loads it behind the menu with
``load_level_for_menu(level, -1, 1)`` (``menu.c``): no players, cutscene 1. What
moves the camera is not code but data - a chain of ``Animation`` objects in the
track's object map whose ``channel`` is 1. Every retail race carries one, 8 to
33 nodes long; a track without one shows a camera that never moves.

How the chain is read, all in ``objects.c`` and ``object_functions.c``:

* ``obj_init_animation`` keeps only the nodes whose channel is the cutscene
  being played (or 20, which plays in all of them), so channel-1 nodes are inert
  in a race and cost nothing there.
* The node with ``order`` 0 spawns ``objectIdToSpawn`` - ``ASSET_OBJECT_ANIMCAMERA``
  - and that object flies a Catmull-Rom spline through the nodes of its
  ``actorIndex`` in ``order``. ``obj_loop_animcamera`` hands its position to the
  camera each frame.
* ``nodeSpeed`` is units per frame times ten; -1 keeps the previous node's. A
  last node with ``goToNode`` 0 loops the flight back to the first.
* ``rotateType`` 0 interpolates each node's own angles, wrapping across the
  turn. ``obj_init_animation`` reads them as bytes, 256 to a turn, shifted into
  the object's 16-bit angles. The camera gets ``0x8000 - y_rotation`` and
  ``-x_rotation``, so a node faces along a direction ``(dx, dy, dz)`` with
  ``y_rotation = atan2(dx, dz) - 180°`` and looks down by ``t`` with
  ``x_rotation = -t`` - checked against the retail chains, whose yaw follows the
  path exactly that way.

What :func:`build` writes copies retail's own chains field for field: the one
camera on node 0, ``nodeSpeed`` 127 there and -1 after, the loop on the last
node, retail's ``unk`` bytes. Only the path is new - the computer racers' line,
lifted above the road, which is a lap of the track by construction.

Deliberately free of ``bpy``.
"""

from __future__ import annotations

import math
from typing import Iterable, List, Optional, Sequence, Tuple

from . import race_ai
from .gltf_io import MapObject

ANIMATION = "ASSET_OBJECT_ANIMATION"
ANIM_CAMERA = "ASSET_OBJECT_ANIMCAMERA"

#: ``load_level_for_menu(gTrackmenuLoadedLevel, -1, 1)`` in ``menu.c``.
PREVIEW_CHANNEL = 1

#: The node name retail's object maps give an ``Animation``.
NODE_NAME = "Animation"

#: Measured on the retail races: their preview cameras sit a median 22 to 95
#: units above the nearest checkpoint, and a node every 1100 to 2000 units.
#: The line here is the racers', which hugs the road more closely than a hand
#: placed camera does, so the nodes are denser to keep the spline off the walls.
HEIGHT = 60.0
SPACING = 900.0

#: How far the camera looks down, on top of following the road's slope.
TILT = 8.0

#: Fewer and the spline is a loose polygon; more and it starts eating into the
#: game's 128 animation slots (``D_8011AE74``), which every channel shares.
MIN_NODES = 8
MAX_NODES = 40

#: Retail's first preview node, every race: 12.7 units a frame.
NODE_SPEED = 127

#: ``u8`` angle, 256 steps to a turn.
ANGLE_STEPS = 256

#: Every field the retail preview nodes agree on. ``pauseFrameCount`` is left
#: out, as retail leaves it out: absent encodes as -1, no pause, where 0 would
#: hold the camera on every node for a frame.
_COMMON = {
    "actorIndex": 0, "animationStartDelay": 0.0, "channel": PREVIEW_CHANNEL,
    "fadeAlpha": 10, "fadeOptions": 0, "goToNode": -1,
    "messageId": "ASSET_GAME_TEXT_255", "nextAnim": 0, "nodeSpeed": -1,
    "objAnimIndex": -1, "objAnimLoopType": 0, "objAnimSpeed": 0, "order": 0,
    "pitchSpinSpeed": 0, "rollSpinSpeed": 0, "rotateType": 0, "scale": 1.0,
    "soundEffect": 0, "specialHide": 0, "unk13": -128, "unk15": -1,
    "unk16": 0, "unk1E": 0, "unk1F": 0, "unk20": 0, "unk22": 0, "unk23": 0,
    "unk28": -1, "unk29": 127, "unk2A": -1, "unk2D": 0, "unk30": 0,
    "unk31": 0, "x_rotation": 0.0, "y_rotation": 0.0, "yawSpinSpeed": 0,
    "z_rotation": 0.0,
}

Point = Tuple[float, float, float]


class FlythroughError(Exception):
    pass


def is_preview_node(obj) -> bool:
    return (obj.object_id == ANIMATION
            and int(obj.fields.get("channel", 0)) == PREVIEW_CHANNEL)


def has_preview(objects: Iterable) -> bool:
    """Whether a map already carries a track select flight of its own."""
    return any(is_preview_node(obj) for obj in objects)


def free_actor(objects: Iterable) -> int:
    """The lowest ``actorIndex`` no ``Animation`` in the map uses yet."""
    taken = {int(obj.fields.get("actorIndex", 0)) for obj in objects
             if obj.object_id == ANIMATION}
    actor = 0
    while actor in taken:
        actor += 1
    return actor


def choose_route(objects: Sequence, preferred_set: Optional[int] = None):
    """The route to fly: ``preferred_set`` when it is a lap, else the biggest set."""
    candidates = []
    if preferred_set is not None:
        candidates.append(int(preferred_set))
    counts = race_ai.vehicle_sets(objects)
    candidates += sorted(counts, key=lambda key: (-counts[key], key))
    for vehicle_set in candidates:
        route = race_ai.build_route(objects, vehicle_set)
        if len(route.main) >= 3:
            return route
    return None


def centre_line(route) -> List[Point]:
    """The middle of the four lanes, as a closed polyline, first point repeated."""
    lanes = [race_ai.lane_line(route, lane) for lane in range(race_ai.LANES)]
    if not lanes[0] or any(len(lane) != len(lanes[0]) for lane in lanes):
        return []
    return [tuple(sum(lane[i][axis] for lane in lanes) / len(lanes)
                  for axis in range(3))
            for i in range(len(lanes[0]))]


def resample(points: Sequence[Point], count: int) -> List[Point]:
    """``count`` points evenly spaced along a closed polyline, the first first."""
    lengths = [math.dist(a, b) for a, b in zip(points, points[1:])]
    total = sum(lengths)
    if total <= 0.0:
        return []
    out: List[Point] = []
    segment, walked = 0, 0.0
    for i in range(count):
        target = total * i / count
        while segment < len(lengths) - 1 and walked + lengths[segment] < target:
            walked += lengths[segment]
            segment += 1
        span = lengths[segment] or 1.0
        t = min(max((target - walked) / span, 0.0), 1.0)
        a, b = points[segment], points[segment + 1]
        out.append(tuple(a[axis] + (b[axis] - a[axis]) * t for axis in range(3)))
    return out


def node_count(length: float, spacing: float = SPACING) -> int:
    return max(MIN_NODES, min(MAX_NODES, int(round(length / spacing))))


def quantise(degrees: float) -> float:
    """An angle on the ``u8`` the entry stores, in [0, 360)."""
    step = int(round(degrees / 360.0 * ANGLE_STEPS)) % ANGLE_STEPS
    return step * 360.0 / ANGLE_STEPS


def facing(direction: Point, tilt: float = TILT) -> Tuple[float, float]:
    """``(y_rotation, x_rotation)`` in degrees for a node facing ``direction``."""
    dx, dy, dz = direction
    yaw = math.degrees(math.atan2(dx, dz)) - 180.0
    climb = math.degrees(math.atan2(dy, math.hypot(dx, dz)))
    return quantise(yaw), quantise(climb - tilt)


def build(objects: Sequence, preferred_set: Optional[int] = None,
          height: float = HEIGHT, spacing: float = SPACING,
          tilt: float = TILT) -> List[MapObject]:
    """A looping track select flight along the racers' line, as map objects.

    ``objects`` is the whole level - both slots - so the checkpoints are found
    wherever they sit. The result goes at the end of the structure map, in
    ``order``, which is how ``obj_init_animation`` wants a chain spawned.
    """
    route = choose_route(objects, preferred_set)
    if route is None:
        raise FlythroughError("no checkpoint set has the three checkpoints a "
                              "lap needs, so there is no line to fly along")
    line = centre_line(route)
    length = sum(math.dist(a, b) for a, b in zip(line, line[1:]))
    points = resample(line, node_count(length, spacing))
    if len(points) < MIN_NODES:
        raise FlythroughError("the racers' line has no length to fly along")

    actor = free_actor(objects)
    nodes: List[MapObject] = []
    count = len(points)
    for order, point in enumerate(points):
        ahead, behind = points[(order + 1) % count], points[order - 1]
        yaw, pitch = facing(tuple(ahead[axis] - behind[axis] for axis in range(3)),
                            tilt)
        fields = dict(_COMMON)
        fields.update(actorIndex=actor, order=order, y_rotation=yaw,
                      x_rotation=pitch)
        if order == 0:
            fields.update(objectIdToSpawn=ANIM_CAMERA, nodeSpeed=NODE_SPEED)
        if order == count - 1:
            fields["goToNode"] = 0
        nodes.append(MapObject(
            object_id=ANIMATION, name=NODE_NAME,
            translation=[round(point[0]), round(point[1] + height), round(point[2])],
            fields=fields,
        ))
    return nodes
