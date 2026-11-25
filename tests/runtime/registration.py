"""Exercise the built server over TCP, including auth order and shared channel state."""
import itertools
import socket
import subprocess
import time
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


class Peer:
    def __init__(self, port):
        self.connection = socket.create_connection(("127.0.0.1", port), timeout=3)
        self.reader = self.connection.makefile("rb")

    def send(self, *lines):
        self.connection.sendall(("\r\n".join(lines) + "\r\n").encode())

    def until(self, fragment):
        seen = []
        for _ in range(40):
            line = self.reader.readline()
            if not line:
                break
            seen.append(line.decode().rstrip("\r\n"))
            if fragment in seen[-1]:
                return seen
        raise AssertionError((fragment, seen))

    def close(self):
        self.reader.close()
        self.connection.close()


class ServerTest(unittest.TestCase):
    def setUp(self):
        with socket.socket() as probe:
            probe.bind(("127.0.0.1", 0))
            self.port = probe.getsockname()[1]
        self.server = subprocess.Popen([ROOT / "ircserv", str(self.port), "demo"],
                                       stdout=subprocess.DEVNULL)
        self.addCleanup(self.stop)
        for _ in range(100):
            if self.server.poll() is not None:
                self.fail("server exited at startup")
            try:
                probe = Peer(self.port)
                probe.close()
                break
            except ConnectionRefusedError:
                time.sleep(.02)

    def stop(self):
        self.server.terminate()
        self.server.wait(timeout=3)

    def peer(self):
        peer = Peer(self.port)
        self.addCleanup(peer.close)
        return peer


class Registration(ServerTest):
    def test_order_and_welcome(self):
        for n, order in enumerate(itertools.permutations(range(3))):
            peer = self.peer()
            nick = "Guest" + str(n)
            commands = ["PASS demo", "NICK " + nick, "USER u 0 * :Real Name"]
            peer.send(*(commands[i] for i in order))
            self.assertEqual(len(peer.until(" 001 " + nick + " ")), 1)
            peer.send("USER x 0 * :Repeat", "JOIN #room", "MODE #room")
            replies = peer.until(" 324 " + nick + " #room +")
            self.assertTrue(any(" 462 " in line for line in replies))
            self.assertFalse(any(" 001 " in line for line in replies))

    def test_missing_credentials(self):
        peer = self.peer()
        peer.send("CAP LS 302")
        peer.until(" CAP * LS :")
        peer.send("PASS wrong", "NICK Bob", "USER u 0 * :Bob", "JOIN #test",
                  "MODE #test", "NICK")
        replies = peer.until(" 431 ")
        self.assertFalse(any(" 001 " in line or " JOIN " in line or " 324 " in line for line in replies))
        peer.send("PASS demo", "JOIN #test", "MODE #test")
        peer.until(" 001 Bob ")
        peer.until(" 324 Bob #test +")


if __name__ == "__main__":
    unittest.main()
