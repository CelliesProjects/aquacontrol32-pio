import subprocess
from typing import Optional

# This script is typically called by PlatformIO's SCons build system
try:
    Import("env")  # type: ignore
except NameError:
    env = None

def get_git_revision() -> str:
    """
    Retrieves the current git tag or hash. 
    Returns 'unknown' if git is not available or not a repository.
    """
    cmd = ["git", "describe", "--tags", "--always", "--dirty"]
    try:
        return (
            subprocess.check_output(cmd, stderr=subprocess.DEVNULL)
            .decode("utf-8")
            .strip()
        )
    except (subprocess.CalledProcessError, FileNotFoundError):
        return "v-dev"


def apply_git_define() -> None:
    """Injects the version define into the build environment."""
    version = get_git_revision()
    
    # Logic for PlatformIO environment injection
    if env:
        # We use a raw string for the value to handle nested quotes correctly
        env.Append(CPPDEFINES=[("GIT_VERSION", f'\\"{version}\\"')])
        print(f"Aquacontrol build: {version} ---")


if __name__ == "__main__" or env:
    apply_git_define()