import unittest
from registration import ServerTest


class ChannelModes(ServerTest):
    def login(self, nick):
        peer = self.peer()
        peer.send("PASS demo", "NICK " + nick, "USER user 0 * :Real Name")
        peer.until(" 001 ")
        return peer

    def test_combined_admission(self):
        op, member, guest = [self.login(n) for n in ("Op", "Member", "Guest")]
        op.send("JOIN #a", "MODE #a +ikl key 2")
        op.until(" MODE #a +l 2")
        guest.send("JOIN #a key")
        guest.until(" 473 ")
        for nick, peer in (("Member", member), ("Guest", guest)):
            op.send("INVITE " + nick + " #a")
            op.until(" 341 ")
            peer.until(" INVITE " + nick + " :#a")
        member.send("JOIN #a wrong")
        member.until(" 475 ")
        member.send("JOIN #a key")
        member.until(" JOIN :#a")
        guest.send("JOIN #a key")
        guest.until(" 471 ")
        op.send("MODE #a -l")
        op.until(" MODE #a -l")
        guest.send("JOIN #a key")
        guest.until(" JOIN :#a")
        guest.send("PART #a", "JOIN #a key")
        guest.until(" 473 ")
        op.send("MODE #a -ik")
        op.until(" MODE #a -k")
        guest.send("JOIN #a")
        guest.until(" JOIN :#a")

    def test_mixed_modes_and_lifecycle(self):
        op, member = self.login("Op"), self.login("Member")
        op.send("JOIN #a")
        op.until(" JOIN :#a")
        member.send("JOIN #a")
        member.until(" JOIN :#a")
        op.send("MODE #a +txkl key 3", "MODE #a +l 0", "MODE #a +l -1",
                "MODE #a +l junk", "MODE #a +l 999999999999999999999999",
                "MODE #a +o+l", "MODE #a")
        replies = op.until(" 324 Op #a +tkl key 3")
        self.assertEqual(sum(" 696 " in line for line in replies), 4)
        self.assertTrue(any(" 461 " in line for line in replies))
        self.assertTrue(any(" 472 " in line for line in replies))
        member.send("MODE #a")
        replies = member.until(" 324 Member #a +tkl * 3")
        self.assertFalse(any("key" in line for line in replies))
        op.send("MODE #a +o-l Member")
        op.until(" MODE #a -l")
        member.send("MODE #a -o+i Member")
        member.until(" 482 ")
        member.send("TOPIC #a :denied")
        member.until(" 482 ")
        op.send("KICK #a Op :self")
        op.until(" KICK #a Op :self")
        member.send("MODE #a -t")
        member.until(" 482 ")
        member.send("PART #a", "JOIN #a", "MODE #a")
        member.until(" 324 Member #a +")
        member.send("MODE #a +i")
        member.until(" MODE #a +i")


if __name__ == "__main__":
    unittest.main()
