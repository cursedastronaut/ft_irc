#include "Test.hpp"
#include "commands/auth/NickHandler.hpp"
#include "protocol/ClientRegistry.hpp"

namespace
{

class RecordingTransport : public transport
{
	public:
		ClientIdentity returnIdentity(irc::ClientId id) const
		{
			(void)id;
			return ClientIdentity();
		}

		void requestDisconnection(irc::ClientId id, const std::string &reason,
								   bool afterFlush)
		{
			(void)id;
			(void)reason;
			(void)afterFlush;
		}

		enqueueResult enqueueLine(irc::ClientId id, const std::string &line)
		{
			ids.push_back(id);
			lines.push_back(line);
			return Queued;
		}

		std::vector<irc::ClientId> ids;
		std::vector<std::string> lines;
};

std::vector<std::string> one(const std::string &value)
{
	std::vector<std::string> params;

	params.push_back(value);
	return params;
}

// ---- PROTO-17: validator -------------------------------------------------

void validatorRejectsEmpty()
{
	CHECK(irc::isValidNickname("") == false);
}

void validatorRejectsBadFirstCharacter()
{
	CHECK(irc::isValidNickname("1bob") == false);
	CHECK(irc::isValidNickname("-bob") == false);
}

void validatorRejectsForbiddenCharacters()
{
	CHECK(irc::isValidNickname("bo b") == false);
	CHECK(irc::isValidNickname("bo:b") == false);
	CHECK(irc::isValidNickname("bo,b") == false);
	CHECK(irc::isValidNickname("bo#b") == false);
	CHECK(irc::isValidNickname("bo\x80") == false);
}

void validatorLengthBoundIsNine()
{
	CHECK(irc::isValidNickname("abcdefghi") == true);
	CHECK(irc::isValidNickname("abcdefghij") == false);
	CHECK(irc::isValidNickname("a") == true);
}

void validatorAcceptsSpecialsDigitsAndDash()
{
	CHECK(irc::isValidNickname("[bob]") == true);
	CHECK(irc::isValidNickname("a\\b") == true);
	CHECK(irc::isValidNickname("_x") == true);
	CHECK(irc::isValidNickname("`^{|}") == true);
	CHECK(irc::isValidNickname("b0b-2") == true);
}

// ---- 431 / 432 -----------------------------------------------------------

void bareNickReplies431()
{
	irc::ClientRegistry registry;
	RecordingTransport net;
	irc::NickHandler handler(registry, net);
	std::vector<std::string> none;
	irc::ClientRecord out;

	registry.create(1);
	handler.handle(1, none);

	CHECK(net.lines.size() == 1);
	CHECK(net.ids[0] == 1);
	CHECK(net.lines[0] == ":ircserv 431 * :No nickname given");
	CHECK(registry.find(1, out) && out.nicknameSet == false);
}

void emptyNickReplies431()
{
	irc::ClientRegistry registry;
	RecordingTransport net;
	irc::NickHandler handler(registry, net);

	registry.create(1);
	handler.handle(1, one(""));

	CHECK(net.lines.size() == 1);
	CHECK(net.lines[0] == ":ircserv 431 * :No nickname given");
}

void invalidNickReplies432AndKeepsCurrentNick()
{
	irc::ClientRegistry registry;
	RecordingTransport net;
	irc::NickHandler handler(registry, net);
	irc::ClientRecord out;
	irc::ClientId owner = 0;

	registry.create(1);
	handler.handle(1, one("bob"));
	handler.handle(1, one("1bad"));

	CHECK(net.lines.size() == 1);
	CHECK(net.lines[0] == ":ircserv 432 bob 1bad :Erroneous nickname");
	CHECK(registry.find(1, out) && out.nickname == "bob");
	CHECK(registry.clientIdForNickname("bob", owner) && owner == 1);
	CHECK(registry.clientIdForNickname("1bad", owner) == false);
}

// ---- PROTO-18: collision -------------------------------------------------

void caseVariantCollisionReplies433AndLeavesBothIntact()
{
	irc::ClientRegistry registry;
	RecordingTransport net;
	irc::NickHandler handler(registry, net);
	irc::ClientRecord out;
	irc::ClientId owner = 0;

	registry.create(1);
	registry.create(2);
	handler.handle(1, one("Bob"));
	handler.handle(2, one("alice"));
	handler.handle(2, one("BOB"));

	CHECK(net.lines.size() == 1);
	CHECK(net.ids[0] == 2);
	CHECK(net.lines[0] == ":ircserv 433 alice BOB :Nickname is already in use");
	CHECK(registry.find(1, out) && out.nickname == "Bob");
	CHECK(registry.find(2, out) && out.nickname == "alice");
	CHECK(registry.clientIdForNickname("bob", owner) && owner == 1);
	CHECK(registry.clientIdForNickname("alice", owner) && owner == 2);
}

void collisionBeforeFirstNickUsesStar()
{
	irc::ClientRegistry registry;
	RecordingTransport net;
	irc::NickHandler handler(registry, net);
	irc::ClientRecord out;

	registry.create(1);
	registry.create(2);
	handler.handle(1, one("bob"));
	handler.handle(2, one("bob"));

	CHECK(net.lines.size() == 1);
	CHECK(net.lines[0] == ":ircserv 433 * bob :Nickname is already in use");
	CHECK(registry.find(2, out) && out.nicknameSet == false);
}

void ownCaseChangeIsNotACollision()
{
	irc::ClientRegistry registry;
	RecordingTransport net;
	irc::NickHandler handler(registry, net);
	irc::ClientRecord out;
	irc::ClientId owner = 0;

	registry.create(1);
	handler.handle(1, one("bob"));
	handler.handle(1, one("BOB"));

	CHECK(net.lines.empty());
	CHECK(registry.find(1, out) && out.nickname == "BOB");
	CHECK(registry.clientIdForNickname("bob", owner) && owner == 1);
}

// ---- PROTO-19: record and index stay in agreement ------------------------

void renameFreesTheOldNickname()
{
	irc::ClientRegistry registry;
	RecordingTransport net;
	irc::NickHandler handler(registry, net);
	irc::ClientRecord out;
	irc::ClientId owner = 0;

	registry.create(1);
	registry.create(2);
	handler.handle(1, one("a"));
	handler.handle(1, one("b"));

	CHECK(registry.clientIdForNickname("a", owner) == false);
	CHECK(registry.clientIdForNickname("b", owner) && owner == 1);
	CHECK(registry.find(1, out) && out.nickname == "b" && out.nicknameSet);

	handler.handle(2, one("a"));
	CHECK(net.lines.empty());
	CHECK(registry.clientIdForNickname("a", owner) && owner == 2);
}

void removeFreesTheNickname()
{
	irc::ClientRegistry registry;
	RecordingTransport net;
	irc::NickHandler handler(registry, net);
	irc::ClientId owner = 0;

	registry.create(1);
	handler.handle(1, one("bob"));
	registry.remove(1);

	CHECK(registry.clientIdForNickname("bob", owner) == false);
}

void unknownActorIsIgnored()
{
	irc::ClientRegistry registry;
	RecordingTransport net;
	irc::NickHandler handler(registry, net);
	irc::ClientRecord out;
	irc::ClientId owner = 0;

	handler.handle(999, one("ghost"));

	CHECK(net.lines.empty());
	CHECK(registry.find(999, out) == false);
	CHECK(registry.clientIdForNickname("ghost", owner) == false);
}

}

int main()
{
	int failures = 0;

	failures += runTests("validatorRejectsEmpty", validatorRejectsEmpty);
	failures += runTests("validatorRejectsBadFirstCharacter",
						  validatorRejectsBadFirstCharacter);
	failures += runTests("validatorRejectsForbiddenCharacters",
						  validatorRejectsForbiddenCharacters);
	failures += runTests("validatorLengthBoundIsNine", validatorLengthBoundIsNine);
	failures += runTests("validatorAcceptsSpecialsDigitsAndDash",
						  validatorAcceptsSpecialsDigitsAndDash);
	failures += runTests("bareNickReplies431", bareNickReplies431);
	failures += runTests("emptyNickReplies431", emptyNickReplies431);
	failures += runTests("invalidNickReplies432AndKeepsCurrentNick",
						  invalidNickReplies432AndKeepsCurrentNick);
	failures += runTests("caseVariantCollisionReplies433AndLeavesBothIntact",
						  caseVariantCollisionReplies433AndLeavesBothIntact);
	failures += runTests("collisionBeforeFirstNickUsesStar",
						  collisionBeforeFirstNickUsesStar);
	failures += runTests("ownCaseChangeIsNotACollision", ownCaseChangeIsNotACollision);
	failures += runTests("renameFreesTheOldNickname", renameFreesTheOldNickname);
	failures += runTests("removeFreesTheNickname", removeFreesTheNickname);
	failures += runTests("unknownActorIsIgnored", unknownActorIsIgnored);

	return failures;
}
