#include "Test.hpp"
#include "protocol/ClientRegistry.hpp"

namespace
{

void createThenFindReturnsTheRecord()
{
	irc::ClientRegistry registry;
	irc::ClientRecord out;

	registry.create(42);
	bool found = registry.find(42, out);

	CHECK(found == true);
	CHECK(out.id == 42UL);
}

void findOnUnknownIdFailsWithoutTouchingOut()
{
	irc::ClientRegistry registry;
	irc::ClientRecord out;
	out.id = 999;

	bool found = registry.find(7, out);

	CHECK(found == false);
	CHECK(out.id == 999UL);
}

void eachClientIdHasExactlyOneRecord()
{
	irc::ClientRegistry registry;
	irc::ClientRecord out;

	registry.create(1);
	registry.create(1);
	registry.remove(1);
	bool found = registry.find(1, out);

	CHECK(found == false);
}

void distinctClientsDoNotInterfereWithEachOther()
{
	irc::ClientRegistry registry;
	irc::ClientRecord firstOut;
	irc::ClientRecord secondOut;

	registry.create(1);
	registry.create(2);

	CHECK(registry.find(1, firstOut) == true);
	CHECK(registry.find(2, secondOut) == true);
	CHECK(firstOut.id == 1UL);
	CHECK(secondOut.id == 2UL);
}

void removingOneClientDoesNotAffectAnother()
{
	irc::ClientRegistry registry;
	irc::ClientRecord out;

	registry.create(1);
	registry.create(2);
	registry.remove(1);

	CHECK(registry.find(1, out) == false);
	CHECK(registry.find(2, out) == true);
}

void repeatedRemovalIsSafe()
{
	irc::ClientRegistry registry;
	irc::ClientRecord out;

	registry.create(5);
	registry.remove(5);
	registry.remove(5);
	registry.remove(5);
	bool found = registry.find(5, out);

	CHECK(found == false);
}

void removingAClientThatNeverExistedIsSafe()
{
	irc::ClientRegistry registry;
	irc::ClientRecord out;

	registry.remove(123);
	bool found = registry.find(123, out);

	CHECK(found == false);
}

void newlyCreatedRecordCannotRegister()
{
	irc::ClientRegistry registry;
	irc::ClientRecord out;

	registry.create(1);
	registry.find(1, out);

	CHECK(out.canRegister() == false);
}

void canRegisterIsFalseWhenAllFlagsAreFalse()
{
	irc::ClientRecord record;

	record.id = 1;
	record.passwordAccepted = false;
	record.nicknameSet = false;
	record.userSet = false;

	CHECK(record.canRegister() == false);
}

void canRegisterIsFalseWithOnlyNicknameSet()
{
	irc::ClientRecord record;

	record.id = 1;
	record.passwordAccepted = false;
	record.nicknameSet = true;
	record.userSet = false;

	CHECK(record.canRegister() == false);
}

void canRegisterIsFalseWithMissingPassword()
{
	irc::ClientRecord record;

	record.id = 1;
	record.passwordAccepted = false;
	record.nicknameSet = true;
	record.userSet = true;

	CHECK(record.canRegister() == false);
}

void canRegisterIsFalseWithMissingNickname()
{
	irc::ClientRecord record;

	record.id = 1;
	record.passwordAccepted = true;
	record.nicknameSet = false;
	record.userSet = true;

	CHECK(record.canRegister() == false);
}

void canRegisterIsFalseWithMissingUser()
{
	irc::ClientRecord record;

	record.id = 1;
	record.passwordAccepted = true;
	record.nicknameSet = true;
	record.userSet = false;

	CHECK(record.canRegister() == false);
}

void canRegisterIsTrueWhenAllThreeFlagsAreSet()
{
	irc::ClientRecord record;

	record.id = 1;
	record.passwordAccepted = true;
	record.nicknameSet = true;
	record.userSet = true;

	CHECK(record.canRegister() == true);
}

}

int main()
{
	int failures = 0;

	failures += runTests("createThenFindReturnsTheRecord",
						  createThenFindReturnsTheRecord);
	failures += runTests("findOnUnknownIdFailsWithoutTouchingOut",
						  findOnUnknownIdFailsWithoutTouchingOut);
	failures += runTests("eachClientIdHasExactlyOneRecord",
						  eachClientIdHasExactlyOneRecord);
	failures += runTests("distinctClientsDoNotInterfereWithEachOther",
						  distinctClientsDoNotInterfereWithEachOther);
	failures += runTests("removingOneClientDoesNotAffectAnother",
						  removingOneClientDoesNotAffectAnother);
	failures += runTests("repeatedRemovalIsSafe", repeatedRemovalIsSafe);
	failures += runTests("removingAClientThatNeverExistedIsSafe",
						  removingAClientThatNeverExistedIsSafe);
	failures += runTests("newlyCreatedRecordCannotRegister",
						  newlyCreatedRecordCannotRegister);
	failures += runTests("canRegisterIsFalseWhenAllFlagsAreFalse",
						  canRegisterIsFalseWhenAllFlagsAreFalse);
	failures += runTests("canRegisterIsFalseWithOnlyNicknameSet",
						  canRegisterIsFalseWithOnlyNicknameSet);
	failures += runTests("canRegisterIsFalseWithMissingPassword",
						  canRegisterIsFalseWithMissingPassword);
	failures += runTests("canRegisterIsFalseWithMissingNickname",
						  canRegisterIsFalseWithMissingNickname);
	failures += runTests("canRegisterIsFalseWithMissingUser",
						  canRegisterIsFalseWithMissingUser);
	failures += runTests("canRegisterIsTrueWhenAllThreeFlagsAreSet",
						  canRegisterIsTrueWhenAllThreeFlagsAreSet);

	return failures;
}
