#ifndef ItlAWDLBootArg_hpp
#define ItlAWDLBootArg_hpp

#include <pexpert/pexpert.h>

/*
 * -itlawdl alone turns on everything AirDrop needs; the single flags that were once listed one by one in the
 * boot-args (-itlawdllisten, -itlawdltx, -itlawdldata, -itlawdlpromisc, -itlawdlp2p, -itlawdlaw, -itlawdlhunt,
 * -itlawdlbaaccept, -itlawdllistener, -itlawdlclient) still work for a test that wants one of them without
 * the rest.
 *
 * ForbiddenItlwm users already carry -itlawdliface (its "tell macOS the card can do AWDL" switch, the one every
 * working set of its flags contains) and -itlawdlcoexbatx (its Block Ack on transmit). Those two count as
 * -itlawdl and -itlawdllegacyba, so moving from ForbiddenItlwm to this driver keeps AirDrop switched on; the
 * other -itlawdlcoex* names are ForbiddenItlwm's internal steps and mean nothing here.
 */
// AirDrop needs the awdl0 interface this driver creates, which only the WCL targets (macOS 15, 26 and 27) have; on
// the others the listener would only cost airtime, so the switch is off whatever the boot-args say.
inline bool itlAWDLEnabled()
{
#ifdef AIRPORT_WCL
    int value = 0;
    return PE_parse_boot_argn("-itlawdl", &value, sizeof(value)) ||
        PE_parse_boot_argn("-itlawdliface", &value, sizeof(value));
#else
    return false;
#endif
}

inline bool itlAWDLBootFlag(const char *single)
{
    int value = 0;
    return itlAWDLEnabled() || PE_parse_boot_argn(single, &value, sizeof(value));
}

// llw0 (Sidecar, Continuity Camera) is off unless -itlawdlllw asks for it: AirDrop does not need it, and its creation
// path has panicked on every machine that ran it so far.
inline bool itlAWDLLlwFlag()
{
    int value = 0;
    return itlAWDLEnabled() && PE_parse_boot_argn("-itlawdlllw", &value, sizeof(value));
}

inline bool itlAWDLLegacyBaFlag()
{
    int value = 0;
    return PE_parse_boot_argn("-itlawdllegacyba", &value, sizeof(value)) ||
        PE_parse_boot_argn("-itlawdlcoexbatx", &value, sizeof(value));
}

#endif
