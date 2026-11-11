import socket
import subprocess
import sys
import unittest

from isolation import FixtureSandbox, reserved_loopback_listener


class IsolationTests(unittest.TestCase):
    def test_logs_and_cleanup_are_owned_by_one_fixture(self):
        with FixtureSandbox() as first, FixtureSandbox() as second:
            first_path, second_path = first.path, second.path
            self.assertNotEqual(first_path, second_path)
            result = first.run([sys.executable, "-c", "print('normal case')"])
            self.assertEqual(result.stdout.strip(), "normal case")
            self.assertEqual((first_path / "1-stdout.log").read_text(), result.stdout)
            self.assertFalse((second_path / "1-stdout.log").exists())
        self.assertFalse(first_path.exists())
        self.assertFalse(second_path.exists())

    def test_failure_does_not_become_success_and_still_cleans_up(self):
        with self.assertRaises(subprocess.CalledProcessError) as caught:
            with FixtureSandbox() as fixture:
                path = fixture.path
                fixture.run([sys.executable, "-c", "raise SystemExit(7)"])
        self.assertEqual(caught.exception.returncode, 7)
        self.assertFalse(path.exists())

    def test_hung_fixture_times_out_and_cleans_up(self):
        with self.assertRaises(subprocess.TimeoutExpired):
            with FixtureSandbox() as fixture:
                path = fixture.path
                fixture.run([sys.executable, "-c", "import time; time.sleep(60)"], timeout=0.1)
        self.assertFalse(path.exists())

    def test_port_reservation_is_unique_and_released(self):
        with reserved_loopback_listener() as first, reserved_loopback_listener() as second:
            first_address = first.getsockname()
            self.assertNotEqual(first_address, second.getsockname())
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as competitor:
                with self.assertRaises(OSError):
                    competitor.bind(first_address)
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as released:
            released.bind(first_address)


if __name__ == "__main__":
    unittest.main()
