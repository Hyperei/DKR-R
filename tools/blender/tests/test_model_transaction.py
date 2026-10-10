"""Hermetic failure-injection checks; never opens a user's track."""
import importlib.util
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location("model_transaction",
    Path(__file__).resolve().parents[1] / "track_lab" / "model_transaction.py")
transaction = importlib.util.module_from_spec(spec)
spec.loader.exec_module(transaction)


class ModelTransactionTests(unittest.TestCase):
    def fixture(self, root):
        result = []
        for name in ("a.model", "b.model"):
            path = Path(root) / name
            path.write_bytes(b"original " + name.encode())
            result.append((path, path.read_bytes(), b"replacement " + name.encode()))
        return result

    def test_complete_commit(self):
        with tempfile.TemporaryDirectory() as root:
            files = self.fixture(root)
            transaction.rewrite_models(files)
            for path, _, replacement in files:
                self.assertEqual(path.read_bytes(), replacement)
            self.assertEqual(len(list(Path(root).iterdir())), 2)

    def test_second_replace_failure_restores_first(self):
        with tempfile.TemporaryDirectory() as root:
            files = self.fixture(root)
            replace = os.replace
            calls = 0
            def fail_second(source, target):
                nonlocal calls
                calls += 1
                if calls == 2:
                    raise OSError("injected write failure")
                replace(source, target)
            with patch.object(transaction.os, "replace", side_effect=fail_second):
                with self.assertRaises(OSError):
                    transaction.rewrite_models(files)
            for path, original, _ in files:
                self.assertEqual(path.read_bytes(), original)
            self.assertEqual(len(list(Path(root).iterdir())), 2)

    def test_validation_race_does_not_replace_any_file(self):
        with tempfile.TemporaryDirectory() as root:
            files = self.fixture(root)
            files[1][0].write_bytes(b"author changed this")
            with self.assertRaisesRegex(OSError, "changed since validation"):
                transaction.rewrite_models(files)
            self.assertEqual(files[0][0].read_bytes(), files[0][1])
            self.assertEqual(files[1][0].read_bytes(), b"author changed this")
            self.assertEqual(len(list(Path(root).iterdir())), 2)

    def test_restoration_failure_retains_original_backup(self):
        with tempfile.TemporaryDirectory() as root:
            files = self.fixture(root)
            replace = os.replace
            calls = 0
            def fail_commit_and_restore(source, target):
                nonlocal calls
                calls += 1
                if calls >= 2:
                    raise OSError("injected device failure")
                replace(source, target)
            with patch.object(transaction.os, "replace", side_effect=fail_commit_and_restore):
                with self.assertRaisesRegex(OSError, "recovery files retained"):
                    transaction.rewrite_models(files)
            backups = list(Path(root).glob(".dkr-model-backup-*"))
            self.assertEqual(len(backups), 1)
            self.assertEqual(backups[0].read_bytes(), files[0][1])
            self.assertEqual(files[1][0].read_bytes(), files[1][1])


if __name__ == "__main__":
    unittest.main()
