"""Verify the reference client's received MODE/NAMES lines (IRSSI can override PATH)."""
import os
from pathlib import Path
import tempfile
import time
import unittest
import pexpect
from registration import ServerTest


class IrssiModes(ServerTest):
    def test_reference_client(self):
        operator = self.peer()
        operator.send("PASS demo", "NICK Owner", "USER owner 0 * :Owner", "JOIN #review",
                      "MODE #review +tl 5")
        operator.until(" MODE #review +l 5")
        with tempfile.TemporaryDirectory() as home:
            child = pexpect.spawn(os.environ.get("IRSSI", "irssi"),
                                  ["--home=" + home, "-c", "127.0.0.1", "-p", str(self.port),
                                   "-w", "demo", "-n", "Reviewer"],
                                  env=dict(os.environ, TERM="xterm"), encoding="utf-8",
                                  codec_errors="replace", dimensions=(42, 150))
            try:
                rawlog = Path(home) / "wire"
                def wait_for(expected):
                    deadline = time.monotonic() + 15
                    while time.monotonic() < deadline:
                        child.send("/rawlog save " + str(rawlog) + "\r")
                        try:
                            child.read_nonblocking(16384, timeout=.25)
                        except pexpect.TIMEOUT:
                            pass
                        time.sleep(.15)
                        if rawlog.exists() and expected in rawlog.read_text(errors="replace"):
                            return
                    self.fail("Missing irssi raw log entry " + expected)

                wait_for(" 422 Reviewer ")
                child.send("/join #review\r")
                wait_for(" 366 Reviewer #review ")
                operator.send("MODE #review +k testkey")
                operator.until(" MODE #review +k testkey")
                child.send("/quote MODE #review\r")
                wait_for(" 324 Reviewer #review +tkl * 5")
                log = rawlog.read_text()
                self.assertIn(" 001 Reviewer ", log)
                self.assertIn(" 353 Reviewer = #review :@Owner Reviewer", log)
                self.assertNotIn("testkey", log)
            finally:
                child.close(force=True)


if __name__ == "__main__":
    unittest.main()
