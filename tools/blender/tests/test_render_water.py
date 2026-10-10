"""Headless optical metadata contract; does not require Blender or a ROM."""
import os
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from track_lab.dkrmap import TrackPackage


class RenderWaterTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.package = TrackPackage(self.temp.name, "water-test", "Water test")

    def test_optional_and_round_trip(self):
        self.assertIsNone(self.package.enhanced_water)
        self.package.set_render_water({"deep": [0.01, 0.1, 0.2], "roughness": 0.4},
                                      [{"texture": 9, "kind": "waves"}])
        self.assertEqual(self.package.enhanced_water["version"], 1)
        self.assertEqual(self.package.enhanced_water["materials"][0]["texture"], 9)
        self.assertEqual(self.package.enhanced_water["profile"]["roughness"], 0.4)

    def test_invalid_values_do_not_replace_previous_profile(self):
        self.package.set_render_water({"roughness": 0.4})
        before = self.package.enhanced_water
        for invalid in ({"visibility": -1}, {"deep": [0, float("nan"), 1]},
                        {"roughness": float("inf")}, {"unknown": 1}, {"sun": [0, 0, 0]}):
            with self.assertRaises(ValueError):
                self.package.set_render_water(invalid)
            self.assertIs(self.package.enhanced_water, before)

    def test_material_bounds_and_duplicates(self):
        for invalid in ([{"texture": 256, "kind": "calm"}],
                        [{"texture": 7, "kind": "lava"}],
                        [{"texture": 7, "kind": "calm"}, {"texture": 7, "kind": "exclude"}]):
            with self.assertRaises(ValueError):
                self.package.set_render_water({}, invalid)


if __name__ == "__main__":
    unittest.main()
