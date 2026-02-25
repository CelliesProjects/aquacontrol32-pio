from typing import Any
import subprocess


try:
    Import("env")  # type: ignore[name-defined]
except NameError:
    env: Any = None


def get_git_version() -> str:
    try:
        return (
            subprocess.check_output(
                ["git", "describe", "--tags", "--always"],
                stderr=subprocess.DEVNULL,
                shell=False,  # explicit, safe
            )
            .decode()
            .strip()
        )
    except Exception:
        return "dev"


git_version = get_git_version()

if env is not None:
    env.Append(
        CPPDEFINES=[("GIT_VERSION", f'\\"{git_version}\\"')]
    )