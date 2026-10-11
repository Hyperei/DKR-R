#!/usr/bin/env python3
"""Bring each pinned dependency checkout to exactly <pinned commit + patches>.

The expected tree is computed in a throwaway git index (`git apply --cached`
applies the stacked patches in order without touching the checkout), then
compared with the checkout. Only managed patch paths that differ are rewritten, so an
already-patched checkout is left untouched (no timestamp churn, no rebuild),
and a half-applied or locally edited one is repaired without a full reset.
Untracked files the patches don't create (build output, helper symlinks) are
never touched. Unrelated tracked edits stop preparation without changing files.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path


def git(repo: Path, *args: str, index: Path | None = None) -> str:
    """stdout of a git command; stderr passes through so a patch that no
    longer applies shows git's own diagnostic."""
    env = dict(os.environ)
    if index is not None:
        env["GIT_INDEX_FILE"] = str(index)
    return subprocess.run(["git", "-C", str(repo), *args], env=env, check=True,
                          stdout=subprocess.PIPE, text=True).stdout.strip()


def patch_trees(repo: Path, patches: list[Path], expected_index: Path,
                current_index: Path) -> tuple[str, str]:
    """Compute reviewed and working trees without touching the real index."""
    git(repo, "read-tree", "HEAD", index=expected_index)
    for patch in patches:
        try:
            git(repo, "apply", "--cached", "--whitespace=nowarn", str(patch), index=expected_index)
        except subprocess.CalledProcessError:
            raise SystemExit(f"{repo.name}: {patch.name} does not apply to the pinned commit") from None
    expected = git(repo, "write-tree", index=expected_index)

    # Files the patches create are untracked in the checkout; stage the ones
    # present so they take part in the comparison (lexists: symlinks too).
    added = git(repo, "diff", "--name-only", "-z", "--no-renames", "--diff-filter=A", "HEAD", expected)
    git(repo, "read-tree", "HEAD", index=current_index)
    git(repo, "add", "-u", index=current_index)
    present = [p for p in added.split("\0") if p and os.path.lexists(repo / p)]
    if present:
        git(repo, "add", "-f", "--", *present, index=current_index)
    current = git(repo, "write-tree", index=current_index)

    return expected, current


def checked_patches(root: Path, dependency: dict) -> list[Path]:
    result = []
    for entry in dependency["patches"]:
        path = root / entry["path"]
        if hashlib.sha256(path.read_bytes()).hexdigest() != entry["sha256"]:
            raise SystemExit(f"Patch checksum mismatch: {entry['path']}")
        result.append(path)
    return result


def is_reviewed_checkout(root: Path, repo: Path, commit: str) -> bool:
    """Accept only the complete reviewed patch tree, without repairing anything.

    Unrelated untracked files and staged edits are intentionally stricter here
    than in the patch repair command: bootstrap must preserve user work.
    """
    manifest = root / "patches" / "manifest.json"
    if not manifest.exists() or git(repo, "rev-parse", "HEAD").lower() != commit.lower():
        return False
    data = json.loads(manifest.read_text(encoding="utf-8"))
    if data.get("schemaVersion") != 1:
        raise SystemExit("Unsupported patch manifest schema")
    matches = [d for d in data["dependencies"]
               if (root / d["repositoryPath"]).resolve() == repo.resolve()]
    if len(matches) != 1 or matches[0]["expectedCommit"].lower() != commit.lower():
        return False
    reviewed = checked_patches(root, matches[0])
    with tempfile.TemporaryDirectory() as scratch:
        expected, current = patch_trees(repo, reviewed, Path(scratch) / "expected",
                                        Path(scratch) / "current")
        if current != expected:
            return False
        base = git(repo, "rev-parse", "HEAD^{tree}")
        if git(repo, "write-tree") not in (base, expected):
            return False
        added = set(git(repo, "diff", "--name-only", "-z", "--no-renames",
                        "--diff-filter=A", "HEAD", expected).split("\0"))
        untracked = set(git(repo, "ls-files", "--others", "--exclude-standard", "-z").split("\0"))
        if untracked - added - {""}:
            return False
        # Tree IDs cannot reveal working-tree edits inside submodules. Accept
        # each dirty child only when its own complete reviewed stack matches;
        # RT64 and N64ModernRuntime both contain patched submodules.
        for entry in git(repo, "ls-tree", "-r", "-z", "HEAD").split("\0"):
            if not entry:
                continue
            metadata, path = entry.split("\t", 1)
            mode, _, child_commit = metadata.split()
            child = repo / path
            if mode != "160000" or not (child / ".git").exists():
                continue
            if git(child, "status", "--porcelain", "--untracked-files=all",
                   "--ignore-submodules=none") and not is_reviewed_checkout(root, child, child_commit):
                return False
        return True


def sync_dependency(repo: Path, patches: list[Path]) -> str:
    """Returns 'unchanged' or 'updated'."""
    with tempfile.TemporaryDirectory() as scratch:
        expected_index = Path(scratch) / "expected"
        expected, current = patch_trees(repo, patches, expected_index, Path(scratch) / "current")

        if current == expected:
            return "unchanged"

        changed = git(repo, "diff", "--name-only", "--no-renames", "-z", current, expected)
        paths = [p for p in changed.split("\0") if p]
        # Repair managed patch paths only. An edit elsewhere belongs to the
        # developer and must not disappear during a routine build.
        managed = set(git(repo, "diff", "--name-only", "--no-renames", "-z",
                          "HEAD", expected).split("\0"))
        unrelated = sorted(set(paths) - managed)
        if unrelated:
            raise SystemExit(f"{repo}: local edits outside the patch set; commit or stash: "
                             + ", ".join(unrelated))
        in_expected = set(git(repo, "ls-tree", "-r", "-z", "--name-only", expected,
                              "--", *paths).split("\0")) if paths else set()
        for path in paths:
            if path not in in_expected:
                (repo / path).unlink(missing_ok=True)
        restore = [p for p in paths if p in in_expected]
        if restore:
            git(repo, "checkout-index", "-f", "--", *restore, index=expected_index)
        return "updated"


def apply_manifest(root: Path, manifest_path: Path) -> int:
    data = json.loads(manifest_path.read_text(encoding="utf-8"))
    if data.get("schemaVersion") != 1:
        raise SystemExit("Unsupported patch manifest schema")

    for dependency in data["dependencies"]:
        name = dependency["name"]
        repo = root / dependency["repositoryPath"]
        # A submodule or worktree has a .git *file*, a clone a .git directory.
        if not (repo / ".git").exists():
            print(f"[SKIP] {name}: checkout not present at {repo}")
            continue
        commit = git(repo, "rev-parse", "HEAD")
        if commit != dependency["expectedCommit"]:
            raise SystemExit(f"{name} commit mismatch: expected {dependency['expectedCommit']}, got {commit}")

        patches = checked_patches(root, dependency)

        state = sync_dependency(repo, patches) if patches else "unchanged"
        print(f"[OK] {name}: {len(patches)} patch(es) {'already applied' if state == 'unchanged' else 'applied'}")
    return 0


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--root", type=Path, default=root)
    parser.add_argument("--manifest", type=Path)
    args = parser.parse_args()
    root = args.root.resolve()  # patch paths are passed to `git -C <repo>`
    return apply_manifest(root, args.manifest or root / "patches" / "manifest.json")


if __name__ == "__main__":
    sys.exit(main())
