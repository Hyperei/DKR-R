#!/usr/bin/env python3
"""Zero-dependency tests for the build tooling: the dependency patch pipeline,
the reviewed-ELF image digest, and repository contracts the shell build
entry points rely on. Needs only git and Python."""

import hashlib
import json
import os
import re
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))

import apply_dependency_patches as patches  # noqa: E402
import legacy_character_presentation_policy as presentation  # noqa: E402
import validate_release_metadata as release_metadata  # noqa: E402


# Hermetic: the developer's global config (signing, hooks, autocrlf) must not
# reach the fixture repositories.
GIT_ENV = {**os.environ, "GIT_CONFIG_GLOBAL": os.devnull, "GIT_CONFIG_NOSYSTEM": "1",
           "GIT_AUTHOR_NAME": "t", "GIT_AUTHOR_EMAIL": "t@example.invalid",
           "GIT_COMMITTER_NAME": "t", "GIT_COMMITTER_EMAIL": "t@example.invalid"}


def git(repo, *args):
    return subprocess.run(["git", "-C", str(repo), *args], check=True, env=GIT_ENV,
                          capture_output=True, text=True).stdout.strip()


def write(path, text):
    # Byte-exact: Windows text mode would otherwise turn \n into \r\n.
    path.write_text(text, newline="\n")


class ReleaseMetadataTests(unittest.TestCase):
    def test_current_release_metadata(self):
        self.assertEqual(release_metadata.main(), 0)

    def test_invalid_version_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for version in ("1.0.5-beta.0", "1.0.5-beta.15-playtest.0",
                            "1.0.5-beta.15-playtest.15-fix.0", "1.0.5-garbage"):
                with self.subTest(version=version):
                    (root / "VERSION").write_text(version)
                    with patch.object(release_metadata, "ROOT", root):
                        with self.assertRaisesRegex(RuntimeError, "semantic version"):
                            release_metadata.main()


class PatchPipelineTests(unittest.TestCase):
    """Stacked patches where the second edits the first's file and adds a new
    one, which is exactly what broke the old all-or-reset logic."""

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        upstream = self.root / "upstream"
        upstream.mkdir()
        git(upstream, "init", "-q")
        write(upstream / "a.txt", "one\ntwo\nthree\n")
        write(upstream / "unrelated.txt", "original\n")
        (upstream / "blob.bin").write_bytes(bytes(range(256)))
        git(upstream, "add", ".")
        git(upstream, "commit", "-qm", "base")
        self.commit = git(upstream, "rev-parse", "HEAD")

        def make_patch(name, edit):
            edit(upstream)
            git(upstream, "add", "-A")
            diff = git(upstream, "diff", "--cached", "--binary") + "\n"
            git(upstream, "commit", "-qm", name)
            path = self.root / "patches" / f"{name}.patch"
            path.parent.mkdir(exist_ok=True)
            write(path, diff)
            return path

        first = make_patch("0001", lambda r: write(r / "a.txt", "one\nTWO\nthree\n"))
        # The second patch also creates a binary and an empty file and deletes
        # one: shapes a patch-text parser gets wrong.
        second = make_patch("0002", lambda r: (write(r / "a.txt", "one\nTWO\nTHREE\n"),
                                               write(r / "new.txt", "created\n"),
                                               (r / "new.bin").write_bytes(b"\0\xff" * 8),
                                               (r / "empty.txt").write_bytes(b""),
                                               (r / "blob.bin").unlink()))
        self.final_a = "one\nTWO\nTHREE\n"
        self.patches = [first, second]
        # The dependency is checked out through `git worktree`, giving it a
        # .git *file* like a submodule (the old pipeline skipped those).
        self.repo = self.root / "dep"
        git(upstream, "worktree", "add", "-q", "--detach", str(self.repo), self.commit)
        self.manifest = self.root / "manifest.json"
        self.manifest.write_text(json.dumps({"schemaVersion": 1, "dependencies": [{
            "name": "dep", "repositoryPath": "dep", "expectedCommit": self.commit,
            "patches": [{"path": str(p.relative_to(self.root)),
                         "sha256": hashlib.sha256(p.read_bytes()).hexdigest()}
                        for p in self.patches]}]}))

    def tearDown(self):
        self.temp.cleanup()

    def apply(self):
        return patches.apply_manifest(self.root, self.manifest)

    def assert_patched(self):
        self.assertEqual((self.repo / "a.txt").read_text(), self.final_a)
        self.assertEqual((self.repo / "new.txt").read_text(), "created\n")
        self.assertEqual((self.repo / "new.bin").read_bytes(), b"\0\xff" * 8)
        self.assertEqual((self.repo / "empty.txt").read_bytes(), b"")
        self.assertFalse((self.repo / "blob.bin").exists())

    def test_submodule_style_checkout_is_patched(self):
        self.assertTrue((self.repo / ".git").is_file())
        self.apply()
        self.assert_patched()

    def test_rerun_is_a_no_op(self):
        self.apply()
        stamps = {}
        for name in ("a.txt", "new.txt", "new.bin", "empty.txt"):
            stamp = (self.repo / name).stat().st_mtime_ns - 10**9
            os.utime(self.repo / name, ns=(stamp, stamp))
            stamps[name] = (self.repo / name).stat().st_mtime_ns
        self.assertEqual(patches.sync_dependency(self.repo, self.patches), "unchanged")
        for name, stamp in stamps.items():
            self.assertEqual((self.repo / name).stat().st_mtime_ns, stamp, name)
        self.assert_patched()

    def test_half_applied_checkout_is_repaired(self):
        # First patch applied, plus the second patch's new file left behind
        # by an interrupted run (`git checkout -- .` doesn't remove it).
        git(self.repo, "apply", str(self.patches[0]))
        (self.repo / "new.txt").write_text("stale\n")
        self.apply()
        self.assert_patched()

    def test_local_edit_is_reverted_and_unrelated_untracked_kept(self):
        self.apply()
        (self.repo / "a.txt").write_text("local edit\n")
        (self.repo / "helper-symlink-target").write_text("keep me\n")
        self.apply()
        self.assert_patched()
        self.assertEqual((self.repo / "helper-symlink-target").read_text(), "keep me\n")

    def test_checksum_is_enforced(self):
        write(self.patches[0], self.patches[0].read_text() + "\n")
        with self.assertRaisesRegex(SystemExit, "checksum mismatch"):
            self.apply()

    def test_commit_is_enforced(self):
        manifest = json.loads(self.manifest.read_text())
        manifest["dependencies"][0]["expectedCommit"] = "0" * 40
        self.manifest.write_text(json.dumps(manifest))
        with self.assertRaisesRegex(SystemExit, "commit mismatch"):
            self.apply()

    def test_unrelated_tracked_edit_is_preserved(self):
        (self.repo / "unrelated.txt").write_text("developer edit\n")
        with self.assertRaisesRegex(SystemExit, "local edits outside the patch set"):
            self.apply()
        self.assertEqual((self.repo / "unrelated.txt").read_text(), "developer edit\n")
        self.assertEqual((self.repo / "a.txt").read_text(), "one\ntwo\nthree\n")


def elf32be(sections):
    """Minimal big-endian ELF32 with the given (name, type, flags, addr, data)."""
    names = b"\0" + b"".join(n.encode() + b"\0" for n, *_ in sections) + b".shstrtab\0"
    body, offsets, cursor = b"", [], 52
    for _, kind, _, _, data in sections:
        offsets.append(cursor)
        if kind != 8:
            body += data
            cursor += len(data)
    shstr_offset = cursor
    body += names
    cursor += len(names)
    headers = b"\0" * 40
    name_at = 1
    for (name, kind, flags, addr, data), offset in zip(sections, offsets):
        headers += struct.pack(">10I", name_at, kind, flags, addr, offset, len(data), 0, 0, 4, 0)
        name_at += len(name) + 1
    headers += struct.pack(">10I", name_at, 3, 0, 0, shstr_offset, len(names), 0, 0, 1, 0)
    count = len(sections) + 2
    header = b"\x7fELF\x01\x02\x01" + b"\0" * 9 + struct.pack(
        ">HHIIIIIHHHHHH", 2, 8, 1, 0, 0, cursor, 0, 52, 0, 0, 40, count, count - 1)
    return header + body + headers


class ElfImageDigestTests(unittest.TestCase):
    def digest(self, sections):
        with tempfile.NamedTemporaryFile(suffix=".elf", delete=False) as handle:
            handle.write(elf32be(sections))
        try:
            return presentation.elf_image_digest(Path(handle.name))
        finally:
            os.unlink(handle.name)

    def base(self, symtab=b"\0" * 16, code=b"\x27\xbd\xff\xe0", bss=8):
        return [(".main", 1, 6, 0x80000400, code),
                (".bss", 8, 3, 0x80100000, b"\0" * bss),
                (".symtab", 2, 0, 0, symtab)]

    def test_symbol_table_differences_are_ignored(self):
        self.assertEqual(self.digest(self.base()), self.digest(self.base(symtab=b"\1" * 32)))

    def test_code_and_layout_changes_are_detected(self):
        reference = self.digest(self.base())
        self.assertNotEqual(reference, self.digest(self.base(code=b"\x27\xbd\xff\xd8")))
        self.assertNotEqual(reference, self.digest(self.base(bss=16)))

    def test_non_elf_is_rejected(self):
        with tempfile.NamedTemporaryFile(delete=False) as handle:
            handle.write(b"not an elf")
        try:
            with self.assertRaises(ValueError):
                presentation.elf_image_digest(Path(handle.name))
        finally:
            os.unlink(handle.name)

    def test_reviewed_revision_gates_composition(self):
        with tempfile.NamedTemporaryFile(suffix=".elf", delete=False) as handle:
            handle.write(elf32be(self.base()))
        path = Path(handle.name)
        try:
            with patch.dict(presentation.ELFS, {"us.v77": presentation.elf_image_digest(path)}):
                self.assertEqual(presentation.reviewed_revision(path), "us.v77")
                with self.assertRaises(ValueError):
                    presentation.compose_presentation({}, path, "us.v80", [], {})
                path.write_bytes(elf32be(self.base(code=b"\x27\xbd\xff\xd8")))
                self.assertIsNone(presentation.reviewed_revision(path))
        finally:
            os.unlink(path)


class RepositoryContractTests(unittest.TestCase):
    @unittest.skipUnless(Path("/bin/bash").exists(), "requires system Bash")
    def test_build_prepares_without_cli_arguments_in_system_bash(self):
        # macOS ships Bash 3.2: expanding an empty array under set -u fails.
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "scripts").mkdir()
            (root / "bin").mkdir()
            (root / "scripts/macos_build_lock.py").write_text((ROOT / "scripts/macos_build_lock.py").read_text())
            build = root / "Build-macOS.sh"
            build.write_text((ROOT / "Build-macOS.sh").read_text())
            prepare = root / "scripts" / "Prepare-DKR-Runtime-macOS.sh"
            prepare.write_text('#!/bin/sh\necho "prepare called"\nexit 42\n')
            prepare.chmod(0o755)
            uname = root / "bin" / "uname"
            uname.write_text('#!/bin/sh\necho Darwin\n')
            uname.chmod(0o755)
            env = dict(os.environ, PATH=str(root / "bin") + os.pathsep + os.environ["PATH"],
                       CMAKE_BUILD_PARALLEL_LEVEL="1")
            env.pop("DKR_MAC_GENERATED_SOURCE", None)
            env.pop("DKR_MAC_V80_GENERATED_SOURCE", None)
            result = subprocess.run(["/bin/bash", str(build)], env=env,
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 42, result.stdout + result.stderr)
            self.assertIn("prepare called", result.stdout)

    @unittest.skipIf(os.name == "nt", "macOS shell scripts; Windows has no guaranteed bash")
    def test_macos_option_errors_are_actionable(self):
        # Argument probes must not contend with the real build running CTest.
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "scripts").mkdir()
            (root / "scripts/macos_build_lock.py").write_text((ROOT / "scripts/macos_build_lock.py").read_text())
            for script in ("Build-macOS.sh", "scripts/Prepare-DKR-Runtime-macOS.sh"):
                (root / script).write_text((ROOT / script).read_text())
                for option in ("--rom-v77", "--rom-v80", "--jobs"):
                    with self.subTest(script=script, option=option):
                        result = subprocess.run(["bash", str(root / script), option],
                                                capture_output=True, text=True)
                        self.assertNotEqual(result.returncode, 0)
                        self.assertIn("requires", result.stdout + result.stderr)
                        self.assertNotIn("unbound variable", result.stdout + result.stderr)

    @unittest.skipIf(os.name == "nt", "stage-tool checks require Bash")
    def test_macos_decomp_ready_requires_all_stage_tools(self):
        script = (ROOT / "scripts" / "Prepare-DKR-Runtime-macOS.sh").read_text()
        helper = re.search(r"(?ms)^decomp_ready\(\) \{\n.*?^\}", script)
        self.assertIsNotNone(helper)
        with tempfile.TemporaryDirectory() as temporary:
            tree = Path(temporary) / "isolated revision with spaces"
            python = tree / ".venv/bin/python3"
            tools = [tree / "tools/dkr_assets_tool", tree / "tools/n64crc",
                     tree / "tools/ido-recomp/macos/cc"]

            def executable(path, status=0):
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(f"#!/bin/sh\nexit {status}\n")
                path.chmod(0o755)

            def ready():
                return subprocess.run(
                    ["bash", "-c", helper.group(0) + '\ndecomp_ready "$1"',
                     "stage-test", str(tree)], capture_output=True, text=True,
                    timeout=5).returncode == 0

            self.assertFalse(ready())
            executable(python)
            for tool in tools:
                self.assertFalse(ready())
                executable(tool)
            self.assertTrue(ready())
            for tool in tools:
                tool.unlink()
                self.assertFalse(ready())
                executable(tool)
            executable(python, status=1)
            self.assertFalse(ready())

    def tracked(self):
        output = subprocess.run(["git", "-C", str(ROOT), "ls-files", "-s"], check=True,
                                capture_output=True, text=True).stdout
        for line in output.splitlines():
            mode, _, _, path = line.split(maxsplit=3)
            yield mode, path

    def test_macos_shell_entry_points_are_executable(self):
        if not (ROOT / ".git").exists():
            self.skipTest("not a git checkout")
        entry_points = {
            "Build-macOS.sh", "Run-macOS.sh", "Setup-macOS.sh",
            "scripts/Prepare-DKR-Runtime-macOS.sh",
            "scripts/Prepare-Game-Source-macOS.sh",
            "scripts/apply-dependency-patches.sh",
        }
        missing = [p for mode, p in self.tracked() if p in entry_points and mode != "100755"]
        self.assertEqual(missing, [], "commit with: git update-index --chmod=+x <file>")

    def test_shell_script_references_match_on_disk_case(self):
        # macOS defaults to case-insensitive APFS, Linux CI does not.
        names = {p.name for p in (ROOT / "scripts").iterdir()}
        lower = {n.lower(): n for n in names}
        wrong = []
        for script in list(ROOT.glob("*.sh")) + list((ROOT / "scripts").glob("*.sh")):
            for ref in re.findall(r"scripts/([A-Za-z0-9_.-]+\.(?:sh|py))", script.read_text()):
                if ref not in names and ref.lower() in lower:
                    wrong.append(f"{script.name}: {ref} -> {lower[ref.lower()]}")
        self.assertEqual(wrong, [])

@unittest.skipIf(os.name == "nt", "macOS packaging and POSIX build lock")
class MacOSDevexTests(unittest.TestCase):
    def test_package_versions_are_distinct_and_overridable(self):
        import package_macos as package
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            with patch.object(package.time, "time", return_value=1800000000):
                first = package.bundle_version(root, 1)
                second = package.bundle_version(root, 1)
            self.assertLess(tuple(map(int, first.split('.'))), tuple(map(int, second.split('.'))))
            self.assertEqual(package.bundle_version(root, 1, '42.3.2'), '42.3.2')
            for invalid in ('0', '10000', '1.100.1', '1.2.100', 'beta', '-1'):
                with self.assertRaises(ValueError):
                    package.bundle_version(root, 1, invalid)
            (root / 'bin/Release').mkdir(parents=True)
            (root / 'bin/Release/dkr-release.json').write_text('{"version":"1.0.5-old","build_number":1}')
            with self.assertRaisesRegex(RuntimeError, 'differs from configured'):
                package.package(root, '1.0.5-new')

    def test_lock_rejects_concurrent_builds_and_allows_nested_prepare(self):
        import fcntl
        helper = ROOT / 'scripts/macos_build_lock.py'
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            child = root / 'child.sh'
            child.write_text('echo nested-ok\n')
            parent = root / 'parent.sh'
            parent.write_text(f'"{sys.executable}" "{helper}" "{root}" "{child}"\n')
            cmd = [sys.executable, str(helper), str(root), str(parent)]
            result = subprocess.run(cmd, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn('nested-ok', result.stdout)
            with (root / 'build/.macos-build.lock').open('r+') as held:
                fcntl.flock(held, fcntl.LOCK_EX | fcntl.LOCK_NB)
                result = subprocess.run(cmd, capture_output=True, text=True)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn('Another macOS build/prepare', result.stderr)
            parent.write_text('exit 42\n')
            self.assertEqual(subprocess.run(cmd).returncode, 42)
            parent.write_text('exit 0\n')
            self.assertEqual(subprocess.run(cmd).returncode, 0)

    def test_host_check_rejects_missing_sha1sum_and_old_gmake(self):
        helper = ROOT / 'scripts/macos-host-tools.sh'
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            def executable(name, body='exit 0'):
                p = root / name
                p.write_text('#!/bin/sh\n' + body + '\n')
                p.chmod(0o755)
            for name in ('xcodebuild', 'git', 'python3', 'cmake', 'ninja', 'c++', 'shasum', 'pcre2-config', 'sdl2-config'):
                executable(name)
            executable('uname', 'echo Darwin')
            executable('gmake', 'echo "GNU Make 3.81"')
            cmd = ['/bin/bash', '-c', f'source "{helper}"; dkr_macos_host_tools 1']
            env = dict(os.environ, PATH=str(root))
            missing = subprocess.run(cmd, env=env, capture_output=True, text=True)
            self.assertNotEqual(missing.returncode, 0)
            self.assertIn('Missing sha1sum', missing.stderr)
            executable('sha1sum')
            old_make = subprocess.run(cmd, env=env, capture_output=True, text=True)
            self.assertNotEqual(old_make.returncode, 0)
            self.assertIn('GNU Make 4', old_make.stderr)
            executable('gmake', 'echo "GNU Make 4.4"')
            executable('sha1sum', 'exit 1')
            for name in ('mktemp', 'rm'):
                (root / name).symlink_to('/usr/bin/' + name if Path('/usr/bin/' + name).exists() else '/bin/' + name)
            incompatible = subprocess.run(cmd, env=env, capture_output=True, text=True)
            self.assertNotEqual(incompatible.returncode, 0)
            self.assertIn('sha1sum must support -c --quiet', incompatible.stderr)


if __name__ == "__main__":
    unittest.main()
