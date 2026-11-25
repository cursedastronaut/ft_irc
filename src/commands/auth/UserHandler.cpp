#include "commands/auth/UserHandler.hpp"
#include "protocol/Reply.hpp"

void irc::UserHandler::handle(ClientId id, const std::vector<std::string> &p)
{
    ClientRecord record;
    if (!registry_.find(id, record))
        return;
    int code = 461;
    std::vector<std::string> fields(1, "USER");
    std::string text = "Invalid USER parameters";
    if (record.userSet || record.registered)
    {
        code = 462;
        fields.clear();
        text = "You may not reregister";
    }
    else if (p.size() == 4 && !p[0].empty() && !p[1].empty() && !p[2].empty() && !p[3].empty()
             && p[0].find_first_of(" !@:\r\n\t") == std::string::npos
             && p[0].find(char(0)) == std::string::npos && p[0].size() <= 64)
    {
        registry_.setUser(id, p[0], p[3]);
        return;
    }
    std::string line;
    if (buildNumericReply(code, registry_.recipientFor(id), fields, text, line) == ReplySuccess)
        net_.enqueueLine(id, line);
}
