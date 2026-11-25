#include "commands/channel/Mode.hpp"
#include <sstream>
#include <limits>

std::vector<irc::ModeStep> irc::planModeChanges(const Parameters &params)
{
    std::vector<ModeStep> plan;
    if (params.size() < 2)
        return plan;
    bool adding = true;
    size_t arg = 2;
    const std::string &flags = params[1];
    for (size_t i = 0; i < flags.size(); ++i)
    {
        char flag = flags[i];
        if (flag == '+' || flag == '-')
        {
            adding = flag == '+';
            continue;
        }
        ModeStep step;
        step.adding = adding;
        step.flag = flag;
        step.error = 0;
        if (std::string("itkol").find(flag) == std::string::npos)
            step.error = 472;
        else if (flag == 'o' || (adding && (flag == 'k' || flag == 'l')))
        {
            if (arg == params.size())
                step.error = 461;
            else
                step.argument = params[arg++];
        }
        plan.push_back(step);
        if (step.error == 461)
            break;
    }
    return plan;
}

namespace irc
{
// A limit is a positive decimal representable by the channel's size_type.
static bool parseLimit(const std::string &text, size_t &value)
{
    value = 0;
    for (size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] < '0' || text[i] > '9')
            return false;
        size_t digit = text[i] - '0';
        if (value > (std::numeric_limits<size_t>::max() - digit) / 10)
            return false;
        value = value * 10 + digit;
    }
    return value > 0;
}

void Mode::apply(Channel &room, ClientId actor, const ClientIdentity &identity, const ModeStep &step)
{
    Parameters fields(1, room.displayName());
    // Check again after each accepted step: -o can demote the caller.
    if (!room.isOperator(actor))
    {
        numeric(actor, identity, 482, fields, "You're not channel operator");
        return;
    }
    fields.push_back(std::string(step.adding ? "+" : "-") + step.flag);
    ChannelModes &m = room.modes;
    if (step.flag == 'i')
        m.inviteOnly = step.adding;
    else if (step.flag == 't')
        m.topicRestricted = step.adding;
    else if (step.flag == 'k')
    {
        if (step.adding && (step.argument.empty() || step.argument[0] == ':' ||
            step.argument.find_first_of(" ,\r\n\t") != std::string::npos ||
            step.argument.find(char(0)) != std::string::npos || step.argument.size() > 128))
        {
            numeric(actor, identity, 696, fields, "Invalid channel key");
            return;
        }
        m.key = step.adding ? step.argument : "";
        m.hasKey = step.adding;
        // Other members see that a key exists; only operators receive the key.
        if (step.adding)
            fields.push_back(step.argument);
    }
    else if (step.flag == 'l')
    {
        size_t limit = 0;
        if (step.adding && !parseLimit(step.argument, limit))
        {
            numeric(actor, identity, 696, fields, "Invalid channel limit");
            return;
        }
        m.limit = limit;
        m.hasLimit = step.adding;
        if (step.adding)
            fields.push_back(step.argument);
    }
    else if (step.flag == 'o')
    {
        ClientId target;
        ClientIdentity user;
        if (!context.identities.clientIdForNickname(step.argument, target) ||
            !context.identities.identityOf(target, user))
        {
            numeric(actor, identity, 401, Parameters(1, step.argument), "No such nick");
            return;
        }
        if (!room.containsMember(target))
        {
            Parameters missing(1, user.nickname);
            missing.push_back(room.displayName());
            numeric(actor, identity, 441, missing, "They aren't on that channel");
            return;
        }
        if (step.adding)
            room.grantOperator(target);
        else
            room.revokeOperator(target);
        fields.push_back(user.nickname);
    }
    std::string event;
    // MODE uses middle parameters only. Drop the builder's empty trailing field.
    if (buildEventLine(identity, "MODE", fields, "", event) != ReplySuccess)
        return;
    event.erase(event.size() - 2);
    std::string masked;
    if (step.flag == 'k' && step.adding)
        masked = event.substr(0, event.size() - step.argument.size()) + "*";
    for (Channel::ClientIds::const_iterator it = room.members.begin(); it != room.members.end(); ++it)
        context.output.enqueueLine(*it, !masked.empty() && !room.isOperator(*it) ? masked : event);
}

void Mode::execute(ClientId actor, const ClientIdentity &identity, const Parameters &params)
{
    Channel *channel = memberChannel(actor, identity, params[0]);
    if (!channel)
        return;

    // irssi requests the ban list after WHO. This server stores no bans.
    if (params.size() == 2 && (params[1] == "b" || params[1] == "+b"))
    {
        numeric(actor, identity, 368, Parameters(1, channel->displayName()), "End of channel ban list");
        return;
    }

    if (params.size() == 1)
    {
        const ChannelModes &modes = channel->modes;
        std::string flags = "+";
        if (modes.inviteOnly) flags += 'i';
        if (modes.topicRestricted) flags += 't';
        if (modes.hasKey) flags += 'k';
        if (modes.hasLimit) flags += 'l';
        Parameters fields(1, channel->displayName());
        fields.push_back(flags);
        if (modes.hasKey)
            fields.push_back(channel->isOperator(actor) ? modes.key : "*");
        if (modes.hasLimit)
        {
            std::ostringstream limit;
            limit << modes.limit;
            fields.push_back(limit.str());
        }
        // Numeric 324 has only middle parameters, no human-readable trailer.
        std::string line;
        if (buildNumericReply(324, identity.nickname, fields, "", line) == ReplySuccess)
            context.output.enqueueLine(actor, line.substr(0, line.size() - 2));
        return;
    }

    if (!channel->isOperator(actor))
    {
        numeric(actor, identity, 482, Parameters(1, channel->displayName()), "You're not channel operator");
        return;
    }
    std::vector<ModeStep> plan = planModeChanges(params);
    for (size_t i = 0; i < plan.size(); ++i)
    {
        if (plan[i].error == 472)
            numeric(actor, identity, 472, Parameters(1, std::string(1, plan[i].flag)), "Unknown mode flag");
        else if (plan[i].error == 461)
            numeric(actor, identity, 461, Parameters(1, "MODE"), "Not enough parameters");
        else
            apply(*channel, actor, identity, plan[i]);
    }
}
}
