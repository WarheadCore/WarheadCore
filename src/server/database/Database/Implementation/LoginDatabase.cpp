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

#include "LoginDatabase.h"

void LoginDatabaseConnection::DoPrepareStatements()
{
    if (!m_reconnecting)
        m_stmts.resize(MAX_LOGINDATABASE_STATEMENTS);

    PrepareStatement(LOGIN_SEL_REALMLIST, "SELECT id, name, address, localAddress, localSubnetMask, port, icon, flag, timezone, allowedSecurityLevel, population, gamebuild FROM realmlist WHERE flag <> 3 ORDER BY name", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_DEL_EXPIRED_IP_BANS, "DELETE FROM ip_banned WHERE unbandate<>bandate AND unbandate<=UNIX_TIMESTAMP()", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_UPD_EXPIRED_ACCOUNT_BANS, "UPDATE account_banned SET active = 0 WHERE active = 1 AND unbandate<>bandate AND unbandate<=UNIX_TIMESTAMP()", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_IP_BANNED, "SELECT * FROM ip_banned WHERE ip = ?", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_INS_IP_AUTO_BANNED, "INSERT INTO ip_banned (ip, bandate, unbandate, bannedby, banreason) VALUES (?, UNIX_TIMESTAMP(), UNIX_TIMESTAMP()+?, 'Trinity realmd', 'Failed login autoban')", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_SEL_IP_BANNED_ALL, "SELECT ip, bandate, unbandate, bannedby, banreason FROM ip_banned WHERE (bandate = unbandate OR unbandate > UNIX_TIMESTAMP()) ORDER BY unbandate", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_IP_BANNED_BY_IP, "SELECT ip, bandate, unbandate, bannedby, banreason FROM ip_banned WHERE (bandate = unbandate OR unbandate > UNIX_TIMESTAMP()) AND ip LIKE CONCAT('%%', ?, '%%') ORDER BY unbandate", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_ACCOUNT_BANNED, "SELECT bandate, unbandate FROM account_banned WHERE id = ? AND active = 1", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_ACCOUNT_BANNED_ALL, "SELECT account.id, username FROM account, account_banned WHERE account.id = account_banned.id AND active = 1 GROUP BY account.id", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_ACCOUNT_BANNED_BY_USERNAME, "SELECT account.id, username FROM account, account_banned WHERE account.id = account_banned.id AND active = 1 AND username LIKE CONCAT('%%', ?, '%%') GROUP BY account.id", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_INS_ACCOUNT_AUTO_BANNED, "INSERT INTO account_banned VALUES (?, UNIX_TIMESTAMP(), UNIX_TIMESTAMP()+?, 'Trinity realmd', 'Failed login autoban', 1)", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_DEL_ACCOUNT_BANNED, "DELETE FROM account_banned WHERE id = ?", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_SEL_SESSIONKEY, "SELECT a.sessionkey, a.id, aa.gmlevel  FROM account a LEFT JOIN account_access aa ON (a.id = aa.id) WHERE username = ?", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_UPD_VS, "UPDATE account SET v = ?, s = ? WHERE username = ?", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_UPD_LOGONPROOF, "UPDATE account SET sessionkey = ?, last_ip = ?, last_login = NOW(), locale = ?, failed_logins = 0, os = ? WHERE username = ?", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_LOGONCHALLENGE, "SELECT a.sha_pass_hash, a.id, a.locked, a.lock_country, a.last_ip, aa.gmlevel, a.v, a.s, a.token_key FROM account a LEFT JOIN account_access aa ON (a.id = aa.id) WHERE a.username = ?", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_LOGON_COUNTRY, "SELECT country FROM ip2nation WHERE ip < ? ORDER BY ip DESC LIMIT 0,1", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_UPD_FAILEDLOGINS, "UPDATE account SET failed_logins = failed_logins + 1 WHERE username = ?", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_SEL_FAILEDLOGINS, "SELECT id, failed_logins FROM account WHERE username = ?", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_ACCOUNT_ID_BY_NAME, "SELECT id FROM account WHERE username = ?", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_ACCOUNT_LIST_BY_NAME, "SELECT id, username FROM account WHERE username = ?", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_ACCOUNT_INFO_BY_NAME, "SELECT id, sessionkey, last_ip, locked, lock_country, expansion, locale, recruiter, os, totaltime FROM account WHERE username = ?", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_ACCOUNT_LIST_BY_EMAIL, "SELECT id, username FROM account WHERE email = ?", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_NUM_CHARS_ON_REALM, "SELECT numchars FROM realmcharacters WHERE realmid = ? AND acctid= ?", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_ACCOUNT_BY_IP, "SELECT id, username FROM account WHERE last_ip = ?", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_ACCOUNT_BY_ID, "SELECT 1 FROM account WHERE id = ?", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_INS_IP_BANNED, "INSERT INTO ip_banned (ip, bandate, unbandate, bannedby, banreason) VALUES (?, UNIX_TIMESTAMP(), UNIX_TIMESTAMP()+?, ?, ?)", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_DEL_IP_NOT_BANNED, "DELETE FROM ip_banned WHERE ip = ?", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_INS_ACCOUNT_BANNED, "INSERT INTO account_banned VALUES (?, UNIX_TIMESTAMP(), UNIX_TIMESTAMP()+?, ?, ?, 1)", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_UPD_ACCOUNT_NOT_BANNED, "UPDATE account_banned SET active = 0 WHERE id = ? AND active != 0", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_DEL_REALM_CHARACTERS_BY_REALM, "DELETE FROM realmcharacters WHERE acctid = ? AND realmid = ?", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_DEL_REALM_CHARACTERS, "DELETE FROM realmcharacters WHERE acctid = ?", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_INS_REALM_CHARACTERS, "INSERT INTO realmcharacters (numchars, acctid, realmid) VALUES (?, ?, ?)", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_SEL_SUM_REALM_CHARACTERS, "SELECT SUM(numchars) FROM realmcharacters WHERE acctid = ?", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_INS_ACCOUNT, "INSERT INTO account(username, sha_pass_hash, expansion, joindate) VALUES(?, ?, ?, NOW())", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_INS_REALM_CHARACTERS_INIT, "INSERT INTO realmcharacters (realmid, acctid, numchars) SELECT realmlist.id, account.id, 0 FROM realmlist, account LEFT JOIN realmcharacters ON acctid=account.id WHERE acctid IS NULL", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_UPD_EXPANSION, "UPDATE account SET expansion = ? WHERE id = ?", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_UPD_ACCOUNT_LOCK, "UPDATE account SET locked = ? WHERE id = ?", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_UPD_ACCOUNT_LOCK_CONTRY, "UPDATE account SET lock_country = ? WHERE id = ?", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_UPD_USERNAME, "UPDATE account SET v = 0, s = 0, username = ?, sha_pass_hash = ? WHERE id = ?", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_UPD_PASSWORD, "UPDATE account SET v = 0, s = 0, sha_pass_hash = ? WHERE id = ?", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_UPD_LAST_IP, "UPDATE account SET last_ip = ? WHERE username = ?", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_UPD_LAST_ATTEMPT_IP, "UPDATE account SET last_attempt_ip = ? WHERE username = ?", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_UPD_ACCOUNT_ONLINE, "UPDATE account SET online = ? WHERE id = ?", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_UPD_UPTIME_PLAYERS, "UPDATE uptime SET uptime = ?, maxplayers = ? WHERE realmid = ? AND starttime = ?", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_DEL_OLD_LOGS, "DELETE FROM logs WHERE (time + ?) < ?", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_DEL_ACCOUNT_ACCESS, "DELETE FROM account_access WHERE id = ?", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_DEL_ACCOUNT_ACCESS_BY_REALM, "DELETE FROM account_access WHERE id = ? AND (RealmID = ? OR RealmID = -1)", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_INS_ACCOUNT_ACCESS, "INSERT INTO account_access (id,gmlevel,RealmID) VALUES (?, ?, ?)", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_GET_ACCOUNT_ID_BY_USERNAME, "SELECT id FROM account WHERE username = ?", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_GET_ACCOUNT_ACCESS_GMLEVEL, "SELECT gmlevel FROM account_access WHERE id = ?", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_GET_GMLEVEL_BY_REALMID, "SELECT gmlevel FROM account_access WHERE id = ? AND (RealmID = ? OR RealmID = -1)", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_GET_USERNAME_BY_ID, "SELECT username FROM account WHERE id = ?", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_CHECK_PASSWORD, "SELECT 1 FROM account WHERE id = ? AND sha_pass_hash = ?", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_CHECK_PASSWORD_BY_NAME, "SELECT 1 FROM account WHERE username = ? AND sha_pass_hash = ?", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_PINFO, "SELECT a.username, aa.gmlevel, a.email, a.reg_mail, a.last_ip, DATE_FORMAT(a.last_login, '%Y-%m-%d %T'), a.failed_logins, a.locked, a.OS FROM account a LEFT JOIN account_access aa ON (a.id = aa.id AND (aa.RealmID = ? OR aa.RealmID = -1)) WHERE a.id = ?", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_PINFO_BANS, "SELECT unbandate, bandate = unbandate, bannedby, banreason FROM account_banned WHERE id = ? AND active ORDER BY bandate ASC LIMIT 1", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_GM_ACCOUNTS, "SELECT a.username, aa.gmlevel FROM account a, account_access aa WHERE a.id=aa.id AND aa.gmlevel >= ? AND (aa.realmid = -1 OR aa.realmid = ?)", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_ACCOUNT_INFO, "SELECT a.username, a.last_ip, aa.gmlevel, a.expansion FROM account a LEFT JOIN account_access aa ON (a.id = aa.id) WHERE a.id = ? ORDER BY a.last_ip", CONNECTION_SYNCH); // Only used in ".account onlinelist" command
    PrepareStatement(LOGIN_SEL_ACCOUNT_ACCESS_GMLEVEL_TEST, "SELECT 1 FROM account_access WHERE id = ? AND gmlevel > ?", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_ACCOUNT_ACCESS, "SELECT a.id, aa.gmlevel, aa.RealmID FROM account a LEFT JOIN account_access aa ON (a.id = aa.id) WHERE a.username = ?", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_ACCOUNT_RECRUITER, "SELECT 1 FROM account WHERE recruiter = ?", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_BANS, "SELECT 1 FROM account_banned WHERE id = ? AND active = 1 UNION SELECT 1 FROM ip_banned WHERE ip = ?", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_ACCOUNT_WHOIS, "SELECT username, email, last_ip FROM account WHERE id = ?", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_LAST_ATTEMPT_IP, "SELECT last_attempt_ip FROM account WHERE id = ?", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_LAST_IP, "SELECT last_ip FROM account WHERE id = ?", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_REALMLIST_SECURITY_LEVEL, "SELECT allowedSecurityLevel from realmlist WHERE id = ?", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_DEL_ACCOUNT, "DELETE FROM account WHERE id = ?", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_SEL_IP2NATION_COUNTRY, "SELECT c.country FROM ip2nationCountries c, ip2nation i WHERE i.ip < ? AND c.code = i.country ORDER BY i.ip DESC LIMIT 0,1", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_AUTOBROADCAST, "SELECT id, weight, text FROM autobroadcast WHERE realmid = ? OR realmid = -1", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_INS_ACCOUNT_MUTE, "INSERT INTO `account_muted` (`accountid`, `mutedate`, `mutetime`, `mutedby`, `mutereason`, `active`) VALUES (?, ?, ?, ?, ?, 1)", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_UPD_ACCOUNT_MUTE, "UPDATE `account_muted` SET `mutedate` = ?, `mutetime` = ? WHERE `accountid` = ?", CONNECTION_ASYNC);
    PrepareStatement(LOGIN_SEL_ACCOUNT_MUTE_INFO, "SELECT `mutedate`, `mutetime`, `mutereason`, `mutedby` FROM `account_muted` WHERE `accountid` = ? ORDER BY `mutedate` ASC", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_SEL_ACCOUNT_MUTE, "SELECT `mutedate`, `mutetime` FROM `account_muted` WHERE `accountid` = ? AND `active` = 1 AND UNIX_TIMESTAMP() <= `mutedate` ORDER BY `mutedate` DESC", CONNECTION_SYNCH);
    PrepareStatement(LOGIN_UPD_ACCOUNT_MUTE_EXPIRED, "UPDATE `account_muted` SET `active` = 0 WHERE `active` = 1 AND `accountid` = ? AND UNIX_TIMESTAMP() >= `mutedate`", CONNECTION_ASYNC);

    // 0: uint32, 1: uint32, 2: uint8, 3: uint32, 4: string // Complete name: "Login_Insert_AccountLoginDeLete_IP_Logging"
    PrepareStatement(LOGIN_INS_ALDL_IP_LOGGING, "INSERT INTO logs_ip_actions (account_id,character_guid,type,ip,systemnote,unixtime,time) VALUES (?, ?, ?, (SELECT last_ip FROM account WHERE id = ?), ?, unix_timestamp(NOW()), NOW())", CONNECTION_ASYNC);
    // 0: uint32, 1: uint32, 2: uint8, 3: uint32, 4: string // Complete name: "Login_Insert_FailedAccountLogin_IP_Logging"
    PrepareStatement(LOGIN_INS_FACL_IP_LOGGING, "INSERT INTO logs_ip_actions (account_id,character_guid,type,ip,systemnote,unixtime,time) VALUES (?, ?, ?, (SELECT last_attempt_ip FROM account WHERE id = ?), ?, unix_timestamp(NOW()), NOW())", CONNECTION_ASYNC);
    // 0: uint32, 1: uint32, 2: uint8, 3: string, 4: string // Complete name: "Login_Insert_CharacterDelete_IP_Logging"
    PrepareStatement(LOGIN_INS_CHAR_IP_LOGGING, "INSERT INTO logs_ip_actions (account_id,character_guid,type,ip,systemnote,unixtime,time) VALUES (?, ?, ?, ?, ?, unix_timestamp(NOW()), NOW())", CONNECTION_ASYNC);
    // 0: string, 1: string, 2: string                      // Complete name: "Login_Insert_Failed_Account_Login_due_password_IP_Logging"
    PrepareStatement(LOGIN_INS_FALP_IP_LOGGING, "INSERT INTO logs_ip_actions (account_id,character_guid,type,ip,systemnote,unixtime,time) VALUES ((SELECT id FROM account WHERE username = ?), 0, 1, ?, ?, unix_timestamp(NOW()), NOW())", CONNECTION_ASYNC);
}
