"""Validate the reusable CORE-06 specification, not unimplemented IRC handlers."""

import copy
import json
from pathlib import Path
import unittest


FIXTURE = Path(__file__).with_name("channel_permissions.json")
OPERATIONS = {"JOIN", "PART", "PRIVMSG", "KICK", "INVITE", "TOPIC",
              "TOPIC_QUERY", "MODE_QUERY"}
OPERATIONS |= {"MODE " + sign + flag for sign in "+-" for flag in "itkol"}


def validate(data):
    ids = [actor["id"] for actor in data["actors"]]
    if len(ids) != 4 or len(set(ids)) != 4 or any(type(i) is not int or i <= 0 for i in ids):
        raise ValueError("Four distinct positive stable actor IDs are required")
    actors = set(ids)
    channels = {channel["name"]: channel for channel in data["channels"]}
    if len(channels) != 2:
        raise ValueError("Two distinct channel fixtures are required")
    for channel in channels.values():
        members = set(channel["members"])
        if not members <= actors or not set(channel["operators"]) <= members:
            raise ValueError("Operators must belong to that channel")
    coverage = {operation: set() for operation in OPERATIONS}
    names = set()
    for case in data["cases"]:
        if case["name"] in names or case["operation"] not in coverage:
            raise ValueError("Cases must have unique names and a supported operation")
        names.add(case["name"])
        if case["actor"] not in actors or case["channel"] not in channels:
            raise ValueError("Unknown fixture actor/channel")
        if type(case["allowed"]) is not bool:
            raise ValueError("Permission expectation must be a boolean")
        recipients = case["event_recipients"]
        if len(set(recipients)) != len(recipients) or not set(recipients) <= actors:
            raise ValueError("Expected recipients must be unique stable actor IDs")
        if not case["allowed"]:
            if recipients or case["mutation"] != "unchanged" or not case["numeric"]:
                raise ValueError("Denied cases may only reply to the actor; no mutation/event")
        coverage[case["operation"]].add(case["allowed"])
        if case["operation"] == "PRIVMSG" and case["allowed"]:
            expected = set(channels[case["channel"]]["members"]) - {case["actor"]}
            if set(recipients) != expected:
                raise ValueError("Channel message recipients must exclude sender/outsiders")
    if any(results != {True, False} for results in coverage.values()):
        raise ValueError("Every operation needs both allowed and denied examples")


class PermissionFixtureTests(unittest.TestCase):
    def setUp(self):
        self.data = json.loads(FIXTURE.read_text())

    def test_complete_permission_specification(self):
        validate(self.data)

    def test_missing_denied_case_is_detected(self):
        self.data["cases"] = [case for case in self.data["cases"]
                              if case["operation"] != "PART" or case["allowed"]]
        with self.assertRaises(ValueError):
            validate(self.data)

    def test_bad_operator_identity_is_detected(self):
        self.data["channels"][0]["operators"] = [1004]
        with self.assertRaises(ValueError):
            validate(self.data)

    def test_denied_mutation_is_detected(self):
        case = next(case for case in self.data["cases"] if not case["allowed"])
        case["mutation"] = "removed someone"
        with self.assertRaises(ValueError):
            validate(self.data)

    def test_leaked_channel_delivery_is_detected(self):
        case = next(case for case in self.data["cases"]
                    if case["operation"] == "PRIVMSG" and case["allowed"])
        case["event_recipients"].append(1004)
        with self.assertRaises(ValueError):
            validate(self.data)

    def test_returned_fixture_can_be_copied_without_mutating_source(self):
        copied = copy.deepcopy(self.data)
        copied["actors"][0]["nickname"] = "Renamed"
        self.assertEqual(copied["actors"][0]["id"], self.data["actors"][0]["id"])
        self.assertNotEqual(copied["actors"][0]["nickname"], self.data["actors"][0]["nickname"])


if __name__ == "__main__":
    unittest.main()
