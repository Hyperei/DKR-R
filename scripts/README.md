# Repository scripts

Use these names for new scripts:

- Shell: lowercase kebab-case, such as `macos-host-tools.sh`.
- Python: lowercase snake_case, such as `package_macos.py` (also usable as module names).
- PowerShell: retain the existing Verb-Noun command style, such as `Prepare-Game-Source.ps1`.

Older public commands such as `Build-macOS.sh`, `Setup-macOS.sh`, and
`Prepare-DKR-Runtime-macOS.sh` retain their names so existing documentation,
automation, and user commands keep working. Historical names are mixed; they
are not the convention for new shell or Python scripts.

Shared shell helpers live in `scripts/lib/` and use the `dkr_` function prefix.
Sourcing a helper should only define functions: do not change shell options,
working directory, environment, or install packages at source time. Keep helpers
compatible with the Bash 3.2 shipped with macOS and return failures to the caller.
Build entrypoints enable `set -euo pipefail` themselves.

`lib/apple-build.sh` provides shared macOS and Xcode checks, required-tool checks,
and positive-integer parallel job selection. Apple build entrypoints and the
macOS host prerequisite helper share these functions. Job selection prefers an
explicit argument, then `CMAKE_BUILD_PARALLEL_LEVEL`, then the logical CPU count;
if CPU detection fails it uses one job. Platform-specific SDK and packaging
checks remain with the build that needs them.

Run `shfmt -w -i 2` on edited shell scripts.
