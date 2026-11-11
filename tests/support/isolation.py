"""Owned temporary logs/process groups and reserved loopback ports for fixtures."""

from contextlib import contextmanager
import os
from pathlib import Path
import signal
import socket
import subprocess
import tempfile


class FixtureSandbox:
    def __init__(self):
        self._temporary = tempfile.TemporaryDirectory(prefix="ft-irc-fixture-")
        self.path = Path(self._temporary.name)
        self._commands = 0

    def __enter__(self):
        return self

    def __exit__(self, *_error):
        self._temporary.cleanup()

    def run(self, arguments, timeout=10):
        """Run one finite fixture command; preserve failure/timeout status."""
        self._commands += 1
        process = subprocess.Popen(arguments, cwd=self.path, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE, text=True,
                                   start_new_session=(os.name == "posix"))
        timed_out = False
        try:
            stdout, stderr = process.communicate(timeout=timeout)
        except subprocess.TimeoutExpired:
            timed_out = True
            if os.name == "posix":
                try:
                    os.killpg(process.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
            else:
                process.kill()
            stdout, stderr = process.communicate()
        (self.path / (str(self._commands) + "-stdout.log")).write_text(stdout)
        (self.path / (str(self._commands) + "-stderr.log")).write_text(stderr)
        if timed_out:
            raise subprocess.TimeoutExpired(arguments, timeout, output=stdout, stderr=stderr)
        if process.returncode:
            raise subprocess.CalledProcessError(process.returncode, arguments,
                                                output=stdout, stderr=stderr)
        return subprocess.CompletedProcess(arguments, process.returncode, stdout, stderr)


@contextmanager
def reserved_loopback_listener():
    """Let the OS select a free test port; keep it reserved until handoff."""
    listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        listener.bind(("127.0.0.1", 0))
        listener.listen(1)
        listener.settimeout(1)
        yield listener
    finally:
        listener.close()
