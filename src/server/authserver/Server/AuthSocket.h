/*
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _AUTHSOCKET_H
#define _AUTHSOCKET_H

#include "Common.h"
#include "BigNumber.h"
#include "RealmSocket.h"
#include "SRP6/SRP6.h"
#include <mutex>

class ACE_INET_Addr;
struct Realm;

enum eStatus
{
    STATUS_CHALLENGE,
    STATUS_LOGON_PROOF,
    STATUS_RECON_PROOF,
    STATUS_PATCH,      // unused
    STATUS_AUTHED,
    STATUS_CLOSED
};

// Handle login commands
class AuthSocket: public RealmSocket::Session
{
public:
    const static int s_BYTE_SIZE = 32;

    AuthSocket(RealmSocket& socket);
    virtual ~AuthSocket(void);

    virtual void OnRead(void);
    virtual void OnAccept(void);
    virtual void OnClose(void);

    static ACE_INET_Addr const& GetAddressForClient(Realm const& realm, ACE_INET_Addr const& clientAddr);

    bool _HandleLogonChallenge();
    bool _HandleLogonProof();
    bool _HandleReconnectChallenge();
    bool _HandleReconnectProof();
    bool _HandleRealmList();
    void SetVSFields(const std::string& rI);

    //data transfer handle for patch
    bool _HandleXferResume();
    bool _HandleXferCancel();
    bool _HandleXferAccept();

    FILE* pPatch;
    std::mutex patcherLock;

private:
    RealmSocket& socket_;
    RealmSocket& socket(void) { return socket_; }

    std::array<uint8, 40> sessionKey;
    std::array<uint8, 16> _reconnectProof;

    eStatus _status;

    std::string _login;
    std::string _tokenKey;

    // Since GetLocaleByName() is _NOT_ bijective, we have to store the locale as a string. Otherwise we can't differ
    // between enUS and enGB, which is important for the patch system
    std::string _localizationName;
    std::string _os;
    uint16 _build;
    uint8 _expversion;
    AccountTypes _accountSecurityLevel;
};

#endif
