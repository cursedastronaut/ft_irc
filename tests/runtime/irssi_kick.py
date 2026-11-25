"""irssi must finish channel sync and stay usable after a KICK."""
import os
import tempfile
import time
import unittest
from pathlib import Path

import pexpect
from registration import ServerTest


class IrssiKick(ServerTest):
    def test_self_kick_then_operator_kick(self):
        with tempfile.TemporaryDirectory() as home:
            client = pexpect.spawn(os.environ.get("IRSSI", "irssi"), [
                "--home=" + home, "-c", "127.0.0.1", "-p", str(self.port),
                "-w", "demo", "-n", "bob_"], env=dict(os.environ, TERM="xterm"),
                encoding="utf-8", codec_errors="replace", dimensions=(40, 140))
            self.addCleanup(client.close, True)
            path = Path(home) / "raw"
            def send(command):
                client.send(command + "\r")

            def wait_for(fragment, count=1):
                deadline = time.monotonic() + 30
                log = ""
                while time.monotonic() < deadline:
                    send("/rawlog save " + str(path))
                    try:
                        client.read_nonblocking(32768, timeout=.2)
                    except pexpect.TIMEOUT:
                        pass
                    time.sleep(.2)
                    if path.exists():
                        log = path.read_text(errors="replace")
                        if log.count(fragment) >= count:
                            return
                self.fail("Missing %r occurrence %d in raw log:\n%s" % (fragment, count, log))

            wait_for(" 422 bob_ ")
            send("/join #g")
            wait_for(" 352 bob_ #g ")
            wait_for(" 315 bob_ #g ")
            wait_for(" 368 bob_ #g ")
            send("/kick bob_")
            wait_for(" KICK #g bob_ :", 2)  # outgoing command and incoming event
            send("/quote PING :after-self")
            wait_for(" PONG ircserv :after-self")
            send("/join #g")
            wait_for(" 353 bob_ = #g :@bob_", 2)
            wait_for(" 368 bob_ #g ", 2)

            owner = self.peer()
            owner.send("PASS demo", "NICK Owner", "USER owner 0 * :Owner", "JOIN #other")
            owner.until(" JOIN :#other")
            send("/join #other")
            wait_for(" 368 bob_ #other ")
            owner.send("KICK #other bob_ :bye")
            wait_for(" KICK #other bob_ :bye")
            send("/join #other")
            wait_for(" 366 bob_ #other ", 2)
            send("/quote PING :after-peer")
            wait_for(" PONG ircserv :after-peer")


if __name__ == "__main__":
    unittest.main()
