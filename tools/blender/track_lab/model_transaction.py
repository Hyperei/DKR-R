"""Recoverable multi-model ID rewrites; no Blender or ROM dependency.

Every replacement and original is durably staged beside its target before
the first atomic replace. A failed commit restores earlier targets. Backups
are retained and named in the error if restoration itself fails. A killed
process also leaves the original .dkr-model-backup-* files for recovery.
"""
import os
import tempfile
from pathlib import Path


def _stage(path, data, prefix):
    descriptor, name = tempfile.mkstemp(prefix=prefix, dir=str(path.parent))
    try:
        with os.fdopen(descriptor, "wb") as handle:
            handle.write(data)
            handle.flush()
            os.fsync(handle.fileno())
    except BaseException:
        os.unlink(name)
        raise
    return name


def rewrite_models(payloads):
    """Commit (path, validated_original, replacement) records or restore them.

    Refuse changes since validation instead of overwriting an author's edit.
    PNGs and unrelated files are never opened or removed.
    """
    staged, committed, retained = [], [], set()
    try:
        for filename, original, replacement in payloads:
            path = Path(filename)
            if path.is_symlink() or path.read_bytes() != original:
                raise OSError("model changed since validation: " + str(path))
            backup = _stage(path, original, ".dkr-model-backup-")
            staged.append([path, original, backup, None])
            staged[-1][3] = _stage(path, replacement, ".dkr-model-pending-")
        # Nothing has been replaced yet; verify the complete set once more.
        for path, original, _backup, _pending in staged:
            if path.is_symlink() or path.read_bytes() != original:
                raise OSError("model changed since validation: " + str(path))
        for record in staged:
            os.replace(record[3], record[0])
            committed.append(record)
    except BaseException as error:
        for path, _original, backup, _pending in reversed(committed):
            try:
                os.replace(backup, path)
            except OSError:
                retained.add(backup)
        if retained:
            raise OSError("Model rewrite failed; original recovery files retained: "
                          + ", ".join(sorted(retained))) from error
        raise
    finally:
        for _path, _original, backup, pending in staged:
            for filename in (backup, pending):
                if filename and filename not in retained:
                    try:
                        os.unlink(filename)
                    except OSError:
                        pass  # A stale recovery file is preferable to data loss.
