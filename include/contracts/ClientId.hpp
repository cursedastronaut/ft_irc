#ifndef IRC_CONTRACTS_CLIENT_ID_HPP
#define IRC_CONTRACTS_CLIENT_ID_HPP

namespace irc
{
// CORE-02 proposal: transport allocates process-unique IDs, never socket FDs.
// Team approval is still required; see docs/team-contract.md.
typedef unsigned long ClientId;
}

#endif
