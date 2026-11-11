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

void setNicknameThenLookupFindsTheSameClient()
{
	irc::ClientRegistry registry;
	irc::ClientId found;

	registry.create(1);
	registry.setNickname(1, "bob");

	CHECK(registry.clientIdForNickname("bob", found) == true);
	CHECK(found == 1UL);
}

void lookupOnUnknownNicknameFailsWithoutTouchingOut()
{
	irc::ClientRegistry registry;
	irc::ClientId found = 999;

	bool success = registry.clientIdForNickname("ghost", found);

	CHECK(success == false);
	CHECK(found == 999UL);
}

void lookupOnUnknownNicknameDoesNotInsertAFakeClient()
{
	irc::ClientRegistry registry;
	irc::ClientId found;

	registry.clientIdForNickname("ghost", found);
	registry.create(1);
	registry.setNickname(1, "ghost");

	CHECK(registry.clientIdForNickname("ghost", found) == true);
	CHECK(found == 1UL);
}

void setNicknameOnAnUncreatedClientDoesNotInsertAFakeClient()
{
	irc::ClientRegistry registry;
	irc::ClientRecord out;

	registry.setNickname(999, "ghost");

	CHECK(registry.find(999, out) == false);
}

void differentSpellingVariantsResolveToTheSameClient()
{
	irc::ClientRegistry registry;
	irc::ClientId foundUpper;
	irc::ClientId foundBracket;

	registry.create(1);
	registry.setNickname(1, "Bob[x]");

	CHECK(registry.clientIdForNickname("BOB[X]", foundUpper) == true);
	CHECK(registry.clientIdForNickname("bob{x}", foundBracket) == true);
	CHECK(foundUpper == 1UL);
	CHECK(foundBracket == 1UL);
}

void twoClientsWithDifferentNicknamesDoNotCollide()
{
	irc::ClientRegistry registry;
	irc::ClientId foundFirst;
	irc::ClientId foundSecond;

	registry.create(1);
	registry.create(2);
	registry.setNickname(1, "bob");
	registry.setNickname(2, "baz");

	CHECK(registry.clientIdForNickname("bob", foundFirst) == true);
	CHECK(registry.clientIdForNickname("baz", foundSecond) == true);
	CHECK(foundFirst == 1UL);
	CHECK(foundSecond == 2UL);
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
	failures += runTests("setNicknameThenLookupFindsTheSameClient",
						  setNicknameThenLookupFindsTheSameClient);
	failures += runTests("lookupOnUnknownNicknameFailsWithoutTouchingOut",
						  lookupOnUnknownNicknameFailsWithoutTouchingOut);
	failures += runTests("lookupOnUnknownNicknameDoesNotInsertAFakeClient",
						  lookupOnUnknownNicknameDoesNotInsertAFakeClient);
	failures += runTests("setNicknameOnAnUncreatedClientDoesNotInsertAFakeClient",
						  setNicknameOnAnUncreatedClientDoesNotInsertAFakeClient);
	failures += runTests("differentSpellingVariantsResolveToTheSameClient",
						  differentSpellingVariantsResolveToTheSameClient);
	failures += runTests("twoClientsWithDifferentNicknamesDoNotCollide",
						  twoClientsWithDifferentNicknamesDoNotCollide);

	return failures;
}
