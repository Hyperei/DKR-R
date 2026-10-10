#include "modern_camera_dkr.hpp"

#include <cmath>
#include <cstdio>
#include <string_view>

using namespace dkr::runtime::modern_camera;

static_assert(to_binary_angle(0.0) == 0);
static_assert(to_binary_angle(kPi / 2.0) == 0x4000);
static_assert(to_binary_angle(-kPi / 2.0) == -0x4000);
static_assert(to_binary_angle(kPi) == -0x8000 && to_binary_angle(-kPi) == -0x8000);
static_assert(to_binary_angle(2.0 * kPi + kPi / 2.0) == 0x4000);

namespace {

bool close(double a, double b, double tolerance) {
    return std::abs(a - b) <= tolerance;
}

int angle_error(std::int16_t a, std::int16_t b) {
    return std::abs(static_cast<int>(static_cast<std::int16_t>(add_angles(a, static_cast<std::int16_t>(-b)))));
}

// The retail car camera sits behind the nose with yaw 0x8000 - racer yaw.
// modern_cam looking along the nose must map back to exactly that yaw.
bool ConventionsMatchRetail() {
    for (const int yaw : {0, 0x1234, 0x4000, -0x3000, 0x7FFF, -0x8000}) {
        const auto racer = static_cast<std::int16_t>(yaw);
        const auto retail = static_cast<std::int16_t>(0x8000 - yaw);
        if (angle_error(camera_yaw_from_view(heading_from_racer_yaw(racer)), retail) > 1) return false;
    }
    return camera_pitch_from_view(-0.2) == to_binary_angle(0.2) &&
        steer_from_racer(70) == -1.0F && steer_from_racer(-35) == 0.5F &&
        slope_from_racer_pitch(0x1000) > 0.0;
}

struct Drive {
    MCOutput out{};
    float x = 0.0F, z = 0.0F;
};

// A racer facing racer_yaw drives along its nose at 600 units/s for 3 s.
Drive drive(std::int16_t racer_yaw, bool look_back) {
    const MCParams p = dkr_params(0U);
    const double heading = heading_from_racer_yaw(racer_yaw);
    const float dx = static_cast<float>(std::sin(heading)), dz = static_cast<float>(std::cos(heading));
    MCState s{};
    Drive d;
    const float dt = 1.0F / 30.0F;
    for (int i = 1; i <= 90; ++i) {
        MCInput in{};
        d.x = 600.0F * dt * i * dx;
        d.z = 600.0F * dt * i * dz;
        in.pos = MCVec3{d.x, 0.0F, d.z};
        in.vel = MCVec3{600.0F * dx, 0.0F, 600.0F * dz};
        in.yaw = static_cast<float>(heading);
        in.grounded = 1;
        in.lookBack = look_back && i > 60 ? 1 : 0;
        ModernCam_Update(&s, &p, &in, dt, nullptr, nullptr, &d.out);
    }
    return d;
}

bool FollowsBehindInDkrUnits() {
    const std::int16_t racer_yaw = 0x2345;
    const double heading = heading_from_racer_yaw(racer_yaw);
    const Drive d = drive(racer_yaw, false);
    // Behind the nose, about the retail distance, above the racer, looking down.
    const double back = -((d.out.eye.x - d.x) * std::sin(heading) + (d.out.eye.z - d.z) * std::cos(heading));
    const double up = d.out.eye.y;
    const std::int16_t yaw = camera_yaw_from_view(d.out.yaw);
    const std::int16_t pitch = camera_pitch_from_view(d.out.pitch);
    std::printf("[test][modern-camera] 600 u/s: %.0f units back, %.0f up, yaw error %d, pitch 0x%04X\n",
                back, up, angle_error(yaw, static_cast<std::int16_t>(0x8000 - racer_yaw)),
                static_cast<unsigned>(static_cast<std::uint16_t>(pitch)));
    if (back < 120.0 || back > 260.0 || up < 25.0 || up > 70.0 ||
        angle_error(yaw, static_cast<std::int16_t>(0x8000 - racer_yaw)) > 0x200 || pitch <= 0) {
        return false;
    }
    // Look back puts the camera ahead of the nose, looking back at the racer.
    const Drive b = drive(racer_yaw, true);
    const double ahead = (b.out.eye.x - b.x) * std::sin(heading) + (b.out.eye.z - b.z) * std::cos(heading);
    // The Bancada position spring trails the racer at speed, so less than at rest.
    return ahead > 40.0 &&
        angle_error(camera_yaw_from_view(b.out.yaw), static_cast<std::int16_t>(-racer_yaw)) < 0x400;
}

bool LookBackWithRetailCamera() {
    Pose camera;
    camera.x = 100.0 + 192.0 * std::sin(0.3);
    camera.y = 50.0;
    camera.z = -50.0 - 192.0 * std::cos(0.3);
    camera.yaw = to_binary_angle(0.3);
    camera.pitch = 0x400;
    const Pose back = turn_around(camera, 100.0, -50.0, kPi);
    return close(back.x - 100.0, -(camera.x - 100.0), 1e-6) &&
        close(back.z + 50.0, -(camera.z + 50.0), 1e-6) && back.y == camera.y &&
        back.pitch == camera.pitch &&
        static_cast<std::uint16_t>(back.yaw - camera.yaw) == 0x8000U;
}

bool FramingAndFov() {
    const MCParams one = dkr_params(0U), two = dkr_params(1U), four = dkr_params(3U);
    const MCParams p = one;
    return close(one.distMin * one.unitScale, 188.8, 0.1) &&
        close(two.distMin, 5.9 * kTwoPlayerFraming, 1e-5) &&
        close(four.heightMin, 1.35 * kMultiPlayerFraming, 1e-5) &&
        close(effective_fov(70.0F, p.fovBase + 6.0F, p), 76.0, 1e-5) &&
        effective_fov(85.0F, p.fovBase + 18.0F, p) == kMaximumFov;
}

// Every preset value sits inside the panel's range (so loading a saved file
// never moves it), and every key names one value.
bool TuningRangesHoldPresets() {
    for (const TuningPreset& preset : kTuningPresets) {
        const Tuning tuning = preset.make();
        for (const TuningField& field : kTuningFields) {
            const double scale = field.degrees ? kPi / 180.0 : 1.0;
            const double value = field_value(tuning, field);
            if (value < field.minimum * scale - 1e-6 || value > field.maximum * scale + 1e-6) {
                std::fprintf(stderr, "[test][modern-camera] %s: %s=%g is outside %g..%g\n", preset.name,
                             field.key, value, field.minimum * scale, field.maximum * scale);
                return false;
            }
        }
    }
    for (std::size_t a = 0; a < kTuningFields.size(); ++a) {
        for (std::size_t b = a + 1; b < kTuningFields.size(); ++b) {
            if (std::string_view(kTuningFields[a].key) == kTuningFields[b].key) return false;
        }
        for (const TuningFeature& feature : kTuningFeatures) {
            if (std::string_view(kTuningFields[a].key) == feature.key) return false;
        }
    }
    return true;
}

bool TuningFileRoundTrip() {
    Tuning tuning = comfort_tuning();
    tuning.params.yawFreq = 1.234567F;
    tuning.params.steerLead = 0.3F;
    tuning.params.enableLandingDip = 1;
    tuning.retailShake = 0.5F;
    tuning.hitReaction = 40.0F;
    tuning.recenterEase = 1.5F;
    if (!same_tuning(parse_tuning(serialize_tuning(tuning)), tuning)) return false;
    // Comments, CRLF, unknown keys, junk and out-of-range values.
    const Tuning odd = parse_tuning("# note\r\nyawFreq=99\r\nnope=1\r\ndistMin=abc\r\n"
                                    "rollMax=-5\r\nenableBoostKick=0\r\n");
    const Tuning defaults = default_tuning();
    return odd.params.yawFreq == 6.0F && odd.params.distMin == defaults.params.distMin &&
        odd.params.rollMax == 0.0F && odd.params.enableBoostKick == 0 &&
        same_tuning(parse_tuning(""), defaults) &&
        tuning_code(defaults).find("p.distMin = 5.9F;") != std::string::npos &&
        tuning_code(defaults).find("p.enableSteerLead = 1;") != std::string::npos;
}

// Comfort drops what usually makes a chase camera nauseating.
bool ComfortIsCalm() {
    const Tuning comfort = comfort_tuning();
    const MCParams& p = comfort.params;
    return p.rollMax == 0.0F && p.rollFromYawRate == 0.0F && p.boostTrauma == 0.0F &&
        p.landingTrauma == 0.0F && comfort.retailShake == 0.0F && p.fovBoost == 0.0F &&
        p.enableLandingDip == 0 && p.enableBoostKick == 0 && p.enableSteerLead == 0 &&
        p.yawDamping >= 1.0F && p.distMax == p.distMin && p.heightMax == p.heightMin;
}

// One tick of a scripted drive: where the nose points and how the racer moves.
struct Tick {
    double heading;
    MCVec3 velocity;
    bool drifting = false;
};

MCVec3 along(double angle, float speed) {
    return MCVec3{speed * static_cast<float>(std::sin(angle)), 0.0F, speed * static_cast<float>(std::cos(angle))};
}

struct DriveResult {
    double worst = 0.0; // degrees between the view and the nose, from tick `from` on
    double moved = 0.0; // degrees the view turned away from where it was just before `from`
    bool held = false;  // the bump guard held the velocity at some point
};

template <typename Script>
DriveResult scripted_drive(bool guarded, float dt, int ticks, int from, Script script) {
    const MCParams p = dkr_params(0U);
    MCState s{};
    MCOutput out{};
    BumpGuard guard{};
    float x = 0.0F, z = 0.0F;
    double before = 0.0;
    DriveResult result;
    for (int i = 0; i < ticks; ++i) {
        const Tick tick = script(i);
        x += tick.velocity.x * dt;
        z += tick.velocity.z * dt;
        MCInput in{};
        in.pos = MCVec3{x, 0.0F, z};
        in.yaw = static_cast<float>(tick.heading);
        in.vel = guarded ? bump_steady_velocity(tick.velocity, in.yaw, dt, guard) : tick.velocity;
        in.grounded = 1;
        in.drifting = tick.drifting ? 1 : 0;
        ModernCam_Update(&s, &p, &in, dt, nullptr, nullptr, &out);
        result.held = result.held || guard.hold > 0.0F;
        if (i == from - 1) before = out.yaw;
        if (i >= from) {
            result.worst = std::max(result.worst,
                                    std::fabs(std::remainder(out.yaw - tick.heading, 2.0 * kPi)) * 180.0 / kPi);
            result.moved = std::max(result.moved, std::fabs(std::remainder(out.yaw - before, 2.0 * kPi)) * 180.0 / kPi);
        }
    }
    return result;
}

constexpr double kNose = 0.7;

// Driving into something at 500 units/s and bouncing off at 130 degrees for
// four ticks: bare modern_cam whips round to face the bounce.
Tick bounce_tick(int i) {
    if (i < 60) return Tick{kNose, along(kNose, 500.0F)};
    if (i < 64) return Tick{kNose, along(kNose + 2.3, 270.0F)};
    return Tick{kNose, along(kNose, 100.0F)};
}

bool BumpDoesNotWhip() {
    const DriveResult bare = scripted_drive(false, 1.0F / 30.0F, 90, 60, bounce_tick);
    const DriveResult guarded = scripted_drive(true, 1.0F / 30.0F, 90, 60, bounce_tick);
    std::printf("[test][modern-camera] bounce: view off the nose by %.1f degrees bare, %.1f guarded\n",
                bare.worst, guarded.worst);
    return bare.worst > 20.0 && guarded.worst < 3.0 && guarded.held;
}

// Bouncing off, then steering away from the wall while accelerating: the view
// keeps up with the nose instead of swinging back towards the wall.
bool BumpThenSteerAway() {
    const auto script = [](int i) {
        if (i < 64) return bounce_tick(i);
        const double heading = kNose - 1.57 * std::min((i - 64) / 16.0, 1.0);
        return Tick{heading, along(heading, std::min(100.0F + 20.0F * static_cast<float>(i - 64), 450.0F))};
    };
    const DriveResult guarded = scripted_drive(true, 1.0F / 30.0F, 120, 60, script);
    std::printf("[test][modern-camera] bounce then steer away: view off the nose by at most %.1f degrees\n",
                guarded.worst);
    return guarded.worst < 35.0;
}

// Bouncing off something mid-drift, with the camera out to the side: the
// view stays where it is instead of swinging to the nose and back.
bool BumpInDriftKeepsTheView() {
    const auto script = [](int i) {
        if (i < 60) return Tick{kNose, along(kNose + 0.5, 500.0F), true};
        if (i < 64) return Tick{kNose, along(kNose + 2.3, 270.0F), true};
        return Tick{kNose, along(kNose + 0.5, 150.0F), true};
    };
    const DriveResult bare = scripted_drive(false, 1.0F / 30.0F, 120, 60, script);
    const DriveResult guarded = scripted_drive(true, 1.0F / 30.0F, 120, 60, script);
    std::printf("[test][modern-camera] bounce mid-drift: view moved %.1f degrees bare, %.1f guarded\n",
                bare.moved, guarded.moved);
    return bare.moved > 20.0 && guarded.moved < 5.0;
}

// A spin turns the nose, not the motion: the guard stays out of it.
bool SpinIsNotABump() {
    const auto script = [](int i) {
        const double heading = i < 30 ? kNose : kNose + 2.0 * kPi * (i - 30) / 30.0;
        return Tick{heading, along(kNose, 400.0F)};
    };
    return !scripted_drive(true, 1.0F / 30.0F, 90, 0, script).held;
}

// Hard cornering never trips it, at 30 or 20 game ticks a second.
bool CorneringIsNotABump() {
    for (const float dt : {1.0F / 30.0F, 1.0F / 20.0F}) {
        const auto script = [dt](int i) {
            const double heading = kNose + 300.0 * kPi / 180.0 * dt * i;
            return Tick{heading, along(heading, 500.0F)};
        };
        if (scripted_drive(true, dt, 60, 0, script).held) return false;
    }
    return true;
}

// The view's fastest turn, in degrees/s, from tick `from` on.
template <typename Script>
double fastest_turn(bool guarded, float dt, int ticks, int from, Script script) {
    const MCParams p = dkr_params(0U);
    MCState s{};
    MCOutput out{};
    BumpGuard guard{};
    float x = 0.0F, z = 0.0F;
    double previous = 0.0, fastest = 0.0;
    for (int i = 0; i < ticks; ++i) {
        const Tick tick = script(i);
        x += tick.velocity.x * dt;
        z += tick.velocity.z * dt;
        MCInput in{};
        in.pos = MCVec3{x, 0.0F, z};
        in.yaw = static_cast<float>(tick.heading);
        in.vel = guarded ? bump_steady_velocity(tick.velocity, in.yaw, dt, guard) : tick.velocity;
        in.grounded = 1;
        in.drifting = tick.drifting ? 1 : 0;
        ModernCam_Update(&s, &p, &in, dt, nullptr, nullptr, &out);
        if (i >= from) {
            fastest = std::max(fastest, std::fabs(std::remainder(out.yaw - previous, 2.0 * kPi)) / dt * 180.0 / kPi);
        }
        previous = out.yaw;
    }
    return fastest;
}

// Sliding sideways in a drift, the camera follows the motion, well off the
// nose; then a wall stops the racer dead. Bare modern_cam drops the motion it
// was following and swings to the nose at once.
Tick drift_stop_tick(int i) {
    if (i < 60) return Tick{kNose, along(kNose + 1.2, 600.0F), true};
    if (i == 60) return Tick{kNose, along(kNose + 1.2, 150.0F), true};
    return Tick{kNose, along(kNose, 30.0F)};
}

bool StopInDriftDoesNotSwing() {
    const double bare = fastest_turn(false, 1.0F / 30.0F, 120, 60, drift_stop_tick);
    const double guarded = fastest_turn(true, 1.0F / 30.0F, 120, 60, drift_stop_tick);
    std::printf("[test][modern-camera] stop mid-drift: view turns at most %.0f degrees/s bare, %.0f guarded\n",
                bare, guarded);
    return bare > 200.0 && guarded < 0.6 * bare;
}

// Hits at 0 and 0 still sees the hit but leaves the velocity alone.
bool HitsOffLeavesTheVelocity() {
    BumpGuard guard{};
    const float dt = 1.0F / 30.0F;
    for (int i = 0; i < 64; ++i) {
        const Tick tick = bounce_tick(i);
        const MCVec3 v = bump_steady_velocity(tick.velocity, static_cast<float>(tick.heading), dt, guard, 0.0F, 0.0F);
        if (v.x != tick.velocity.x || v.z != tick.velocity.z) return false;
        if (i == 60 && !guard.hit) return false;
    }
    return true;
}

// Braking as hard as a kart can (30 m/s^2 from 700 units/s) is not a hit.
bool BrakingIsNotAHit() {
    for (const float dt : {1.0F / 60.0F, 1.0F / 30.0F, 1.0F / 20.0F}) {
        BumpGuard guard{};
        float speed = 700.0F;
        while (speed > 0.0F) {
            bump_steady_velocity(along(kNose, speed), static_cast<float>(kNose), dt, guard);
            if (guard.hit) return false;
            speed -= 960.0F * dt;
        }
    }
    return true;
}

// Straight at 700 units/s into a wall. The position spring trails the racer
// at speed; the hit hold keeps the eye at that distance instead of letting
// it lunge in, then eases it to modern_cam's.
struct WallResult {
    double before = 0.0;     // eye to racer, flat, just before the hit
    double after = 0.0;      // the same, three ticks after it
    double settled = 0.0;    // and two seconds later
    double modern = 0.0;     // modern_cam's own distance then
};

WallResult wall_hit(const Tuning& tuning) {
    const MCParams p = layout_params(tuning, 0U);
    const float dt = 1.0F / 30.0F;
    MCState s{};
    BumpGuard guard{};
    HitHold hold{};
    float x = 0.0F, z = 0.0F;
    WallResult result;
    const HitTiming timing = hit_timing(tuning.hitReaction);
    for (int i = 0; i < 180; ++i) {
        const MCVec3 velocity = along(kNose, i < 60 ? 700.0F : 0.0F);
        x += velocity.x * dt;
        z += velocity.z * dt;
        MCInput in{};
        in.pos = MCVec3{x, 0.0F, z};
        in.yaw = static_cast<float>(kNose);
        in.vel = hit_steady_input(velocity, in.yaw, dt, guard, timing).velocity;
        in.grounded = 1;
        const bool reset = !s.initialized;
        MCOutput out{};
        ModernCam_Update(&s, &p, &in, dt, nullptr, nullptr, &out);
        const ViewPose shown = hit_steady_pose(
            ViewPose{out.eye.x, out.eye.y, out.eye.z, out.target.x, out.target.y, out.target.z, out.yaw, out.pitch},
            in.pos, guard.hit, false, reset, dt, timing.hold, timing.release, hold);
        const double distance = std::hypot(shown.x - x, shown.z - z);
        if (i == 59) result.before = distance;
        if (i == 63) result.after = distance;
        if (i == 179) {
            result.settled = distance;
            result.modern = std::hypot(out.eye.x - x, out.eye.z - z);
        }
    }
    return result;
}

bool HitHoldsTheDistance() {
    Tuning off = default_tuning();
    off.hitReaction = 100.0F;
    const WallResult bare = wall_hit(off);
    const WallResult held = wall_hit(default_tuning());
    std::printf("[test][modern-camera] wall at 700 u/s: eye %.0f -> %.0f units bare, %.0f -> %.0f held\n",
                bare.before, bare.after, held.before, held.after);
    return bare.before - bare.after > 40.0 && std::fabs(held.after - held.before) < 2.0 &&
        std::fabs(held.settled - held.modern) < 0.5;
}

// Moving the camera ends the hold at once and a reset drops the blend.
bool HitHoldYieldsToTheCamera() {
    HitHold hold{};
    const ViewPose near{0.0, 40.0, -100.0, 0.0, 30.0, 50.0, 0.0, -0.2};
    const ViewPose far{0.0, 40.0, -200.0, 0.0, 30.0, 50.0, 0.0, -0.2};
    const MCVec3 racer{0.0F, 0.0F, 0.0F};
    hit_steady_pose(far, racer, false, false, false, 0.05F, 0.3F, 0.8F, hold);
    const ViewPose kept = hit_steady_pose(near, racer, true, false, false, 0.05F, 0.3F, 0.8F, hold);
    if (!close(kept.z, -200.0, 1e-6)) return false;
    hit_steady_pose(near, racer, false, true, false, 0.05F, 0.3F, 0.8F, hold);
    if (hold.hold != 0.0F || hold.blend != 1.0F) return false;
    hit_steady_pose(near, racer, false, false, false, 0.05F, 0.3F, 0.8F, hold);
    if (!(hold.blend < 1.0F)) return false;
    const ViewPose cut = hit_steady_pose(near, racer, false, false, true, 0.05F, 0.3F, 0.8F, hold);
    return hold.blend == 0.0F && close(cut.z, -100.0, 1e-6);
}

// 0% is the calmest, 100% is raw modern_cam.
bool HitReactionScale() {
    const HitTiming calm = hit_timing(0.0F), raw = hit_timing(100.0F), mid = hit_timing(50.0F);
    return calm.hold == kHitHoldMax && calm.release == kHitReleaseMax && calm.heading_lag == kHitHeadingLagMax &&
        raw.hold == 0.0F && raw.release == 0.0F && raw.heading_lag == 0.0F &&
        close(mid.release, kHitReleaseMax / 2.0, 1e-6) && hit_timing(150.0F).hold == 0.0F;
}

// A hit that knocks the nose 30 degrees round in two ticks and partly back:
// raw, the view turns with it; steadied, the racer turns on screen instead.
template <typename Script>
double fastest_turn_with(const HitTiming& timing, float dt, int ticks, int from, Script script) {
    const MCParams p = dkr_params(0U);
    MCState s{};
    MCOutput out{};
    BumpGuard guard{};
    float x = 0.0F, z = 0.0F;
    double previous = 0.0, fastest = 0.0;
    for (int i = 0; i < ticks; ++i) {
        const Tick tick = script(i);
        x += tick.velocity.x * dt;
        z += tick.velocity.z * dt;
        const HitInput given = hit_steady_input(tick.velocity, static_cast<float>(tick.heading), dt, guard, timing);
        MCInput in{};
        in.pos = MCVec3{x, 0.0F, z};
        in.yaw = given.heading;
        in.vel = given.velocity;
        in.grounded = 1;
        in.drifting = tick.drifting ? 1 : 0;
        ModernCam_Update(&s, &p, &in, dt, nullptr, nullptr, &out);
        if (i >= from) {
            fastest = std::max(fastest, std::fabs(std::remainder(out.yaw - previous, 2.0 * kPi)) / dt * 180.0 / kPi);
        }
        previous = out.yaw;
    }
    return fastest;
}

Tick knocked_tick(int i) {
    const double knock = i < 60 ? 0.0 : i == 60 ? -0.26 : i < 64 ? -0.52 : -0.35;
    return Tick{kNose + knock, along(kNose, i < 60 ? 600.0F : 40.0F)};
}

bool KnockedNoseTurnsOnScreen() {
    const double raw = fastest_turn_with(hit_timing(100.0F), 1.0F / 30.0F, 120, 60, knocked_tick);
    const double calm = fastest_turn_with(hit_timing(0.0F), 1.0F / 30.0F, 120, 60, knocked_tick);
    std::printf("[test][modern-camera] knocked nose: view turns at most %.0f degrees/s raw, %.0f calm\n", raw,
                calm);
    return calm < 0.5 * raw;
}

// A second hit while the first eases out starts from where the camera is,
// not from the first hit's velocity: driving away along a new direction after
// a drift hit, then stopping again, must not swing the view back.
bool SecondHitDoesNotGoStale() {
    const auto script = [](int i) {
        if (i < 60) return Tick{kNose, along(kNose + 1.2, 600.0F), true};
        if (i < 62) return Tick{kNose, along(kNose, 30.0F)};
        // Turned 90 degrees and drove off, then stopped dead again.
        const double heading = kNose - 1.57;
        if (i < 110) return Tick{heading, along(heading, 450.0F)};
        return Tick{heading, along(heading, 20.0F)};
    };
    const double turn = fastest_turn_with(hit_timing(25.0F), 1.0F / 30.0F, 170, 105, script);
    std::printf("[test][modern-camera] second hit after turning away: view turns at most %.0f degrees/s\n", turn);
    return turn < 60.0;
}

// Recentring builds up from nothing instead of starting at full speed.
bool RecentringEasesIn() {
    return eased_recenter_sharpness(3.0F, 1.2F, 1.2F, 0.8F) == 0.0F &&
        close(eased_recenter_sharpness(3.0F, 1.6F, 1.2F, 0.8F), 1.5, 1e-5) &&
        eased_recenter_sharpness(3.0F, 2.5F, 1.2F, 0.8F) == 3.0F &&
        eased_recenter_sharpness(3.0F, 1.21F, 1.2F, 0.0F) == 3.0F;
}

bool TuningScalesWithLayout() {
    Tuning tuning = default_tuning();
    tuning.params.distMin = 4.0F;
    tuning.params.unitScale = 1.0F;
    const MCParams two = layout_params(tuning, 1U);
    return close(two.distMin, 4.0 * kTwoPlayerFraming, 1e-5) && two.unitScale == kUnitsPerMetre;
}

} // namespace

int main() {
    const struct {
        const char* name;
        bool (*run)();
    } tests[] = {
        {"conventions", ConventionsMatchRetail},
        {"follow", FollowsBehindInDkrUnits},
        {"retail look back", LookBackWithRetailCamera},
        {"framing and fov", FramingAndFov},
        {"tuning ranges", TuningRangesHoldPresets},
        {"tuning file", TuningFileRoundTrip},
        {"comfort preset", ComfortIsCalm},
        {"tuning layout", TuningScalesWithLayout},
        {"bump does not whip", BumpDoesNotWhip},
        {"bump then steer away", BumpThenSteerAway},
        {"bump mid-drift", BumpInDriftKeepsTheView},
        {"spin is not a bump", SpinIsNotABump},
        {"cornering is not a bump", CorneringIsNotABump},
        {"stop mid-drift", StopInDriftDoesNotSwing},
        {"hits off", HitsOffLeavesTheVelocity},
        {"braking is not a hit", BrakingIsNotAHit},
        {"hit holds the distance", HitHoldsTheDistance},
        {"hit hold yields", HitHoldYieldsToTheCamera},
        {"recentring eases in", RecentringEasesIn},
        {"hit reaction scale", HitReactionScale},
        {"knocked nose", KnockedNoseTurnsOnScreen},
        {"second hit", SecondHitDoesNotGoStale},
    };
    for (const auto& test : tests) {
        if (!test.run()) {
            std::fprintf(stderr, "[test][modern-camera] %s FAIL\n", test.name);
            return 1;
        }
    }
    std::puts("[test][modern-camera] PASS");
    return 0;
}
