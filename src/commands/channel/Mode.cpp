#include "commands/channel/Mode.hpp"

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
void Mode::execute(ClientId actor, const ClientIdentity &identity, const Parameters &params)
{
    Channel *channel = memberChannel(actor, identity, params[0]);
    if (!channel)
        return;

    std::vector<ModeStep> plan = planModeChanges(params);
    for (size_t i = 0; i < plan.size(); ++i)
    {
        if (plan[i].error == 472)
            numeric(actor, identity, 472, Parameters(1, std::string(1, plan[i].flag)), "Unknown mode flag");
        else if (plan[i].error == 461)
            numeric(actor, identity, 461, Parameters(1, "MODE"), "Not enough parameters");
    }
}
}
