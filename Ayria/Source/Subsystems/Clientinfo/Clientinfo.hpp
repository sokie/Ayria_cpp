/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-09-28
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

namespace Clientinfo
{
    struct Networkclient
    {
        uint32_t ClientID;
        uint32_t IPAddress;
        std::wstring Username;
    };
    struct Networkpool
    {
        uint16_t Groupport;
        uint16_t Maxmembers;
        uint32_t Groupaddress;
        std::vector<Networkclient> Members;
    };
    struct Matchmakingsession : Networkpool
    {
        union
        {
            uint8_t Flags;
            struct
            {
                uint8_t
                    isHost : 1,
                    isPublic : 1,
                    isServer : 1,
                    isRunning : 1,
                    Compressedinfo : 1,
                    RESERVED1 : 1,
                    RESERVED2 : 1,
                    RESERVED3 : 1;
            };
        };
        uint32_t Platform;
        std::wstring Servername;
        std::string Sessiondata;
    };

    struct Ayriaclient
    {
        uint32_t ClientID;
        char Locale[8];
        char Username[20];
    };

    // For modularity.
    namespace Internal
    {
        void UpdateLocalclient();
        void InitLocalclient();
    }

    // Wrappers for the subsystems.
    inline void doFrame()
    {
        Internal::UpdateLocalclient();
    }
    inline void Initialize()
    {
        Internal::InitLocalclient();

        // Default network groups.
        Auxiliary::Joinmessagegroup(Auxiliary::Generalport);
        Auxiliary::Joinmessagegroup(Auxiliary::Pluginsport);
        Auxiliary::Joinmessagegroup(Auxiliary::Matchmakeport);
        Auxiliary::Joinmessagegroup(Auxiliary::Fileshareport);
    }

    // Exports in JSON.
    namespace API
    {
        extern "C" EXPORT_ATTR const char *__cdecl getLocalclient();
        extern "C" EXPORT_ATTR const char *__cdecl getNetworkclients();
    }
}
