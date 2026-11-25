"""Replay the CORE-06 cases against ircserv with independent channel state per case."""
import json
import unittest
from registration import ROOT, ServerTest

CASES = json.loads((ROOT / "tests/fixtures/channel_permissions.json").read_text())["cases"]


def exchange(peer, *commands):
    # A deliberately argumentless NICK provides an ordered end marker.
    peer.send(*commands, "NICK")
    return peer.until(" 431 ")[:-1]


class Permissions(ServerTest):
    def run_case(self, case):
        peers = {}
        for id in range(1001, 1005):
            p = peers[id] = self.peer()
            exchange(p, "PASS demo", "NICK U" + str(id), "USER u 0 * :Test User")
        for room, members in (("#alpha", (1001, 1002, 1003)), ("#beta", (1002, 1001))):
            for id in members:
                exchange(peers[id], "JOIN " + room)
        name, op, room = case["name"], case["operation"], case["channel"]
        actor, target = case["actor"], case.get("target")
        admin = peers[1001 if room == "#alpha" else 1002]
        if name.startswith("invite-") or name == "join-invite-only-denied":
            exchange(admin, "MODE " + room + " +i")
        if name == "topic-edit-member-restricted":
            exchange(admin, "MODE " + room + " +t")
        if name == "mode-query-regular-key-hidden":
            exchange(admin, "MODE " + room + " +k key")
        values = {"k": " key", "o": " U" + str(target), "l": " 5"}
        if op.startswith("MODE -"):
            flag = op[-1]
            exchange(admin, "MODE " + room + " +" + flag + values.get(flag, ""))
        for p in peers.values():
            exchange(p)

        def snapshot(channel):
            owner = peers[1001 if channel == "#alpha" else 1002]
            return exchange(owner, "NAMES " + channel, "MODE " + channel, "TOPIC " + channel)

        other = "#beta" if room == "#alpha" else "#alpha"
        before = snapshot(room)
        other_before = snapshot(other)
        if op.startswith("MODE "):
            sign = op.split()[1]
            cmd = "MODE " + room + " " + sign
            if sign.startswith("+") or sign == "-o":
                cmd += values.get(sign[1], "")
        else:
            commands = {"JOIN": "JOIN " + room, "PART": "PART " + room,
                        "PRIVMSG": "PRIVMSG " + room + " :hello",
                        "KICK": "KICK " + room + " U" + str(target),
                        "INVITE": "INVITE U" + str(target) + " " + room,
                        "MODE_QUERY": "MODE " + room, "TOPIC_QUERY": "TOPIC " + room,
                        "TOPIC": "TOPIC " + room + " :updated"}
            cmd = commands[op]
        results = {actor: exchange(peers[actor], cmd)}
        for id, peer in peers.items():
            if id != actor:
                results[id] = exchange(peer)
        recipients = []
        for id, lines in results.items():
            events = [line for line in lines if line.split()[1].isalpha()]
            if events:
                self.assertEqual(len(events), 1)
                recipients.append(id)
        self.assertEqual(sorted(recipients), sorted(case["event_recipients"]))
        code = case.get("numeric") or case.get("ack_numeric")
        if code:
            self.assertTrue(any(" %03d " % code in line for line in results[actor]), results)
        if not case["allowed"]:
            self.assertEqual(snapshot(room), before)
        self.assertEqual(snapshot(other), other_before)
        if name == "mode-query-regular-key-hidden":
            self.assertTrue(any("+k *" in line for line in results[actor]))
        if name == "invite-regular-restricted":
            self.assertTrue(any(" 473 " in line for line in exchange(peers[target], "JOIN " + room)))
        if op in ("PART", "KICK") and case["allowed"]:
            departed = actor if op == "PART" else target
            self.assertTrue(any(" 404 " in line for line in exchange(peers[departed], "PRIVMSG " + room + " :late")))
        if name == "part-outsider":
            self.assertTrue(any(" 403 " in line for line in exchange(peers[actor], "PART #missing")))
            exchange(peers[actor], "JOIN " + room)
            self.assertTrue(any(" PART " in line for line in exchange(peers[actor], "PART " + room)))


def make_case(case):
    return lambda self: self.run_case(case)


for case in CASES:
    setattr(Permissions, "test_" + case["name"], make_case(case))

if __name__ == "__main__":
    unittest.main()
