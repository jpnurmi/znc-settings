/*
 * Copyright (C) 2015 J-P Nurmi
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <znc/Modules.h>
#include <znc/User.h>
#include <znc/IRCNetwork.h>
#include <znc/Chan.h>
#include <znc/znc.h>
#include <functional>

#if (VERSION_MAJOR < 1) || (VERSION_MAJOR == 1 && VERSION_MINOR < 7)
#error The admin module requires ZNC version 1.7.0 or later.
#endif

enum VarType {
	StringType, BoolType, IntType, DoubleType, ListType
};

static const char* VarTypes[] = {
	"String", "Boolean", "Integer", "Double", "List"
};

template <typename T>
struct Variable
{
	CString name;
	VarType type;
	CString description;
	std::function<CString(const T*)> getter;
	std::function<bool(T*, const CString&)> setter;
	std::function<bool(T*)> resetter;
};

template <typename T>
struct Command
{
	CString syntax;
	CString description;
	std::function<bool(T*, const CString&)> func;
};

class CAdminMod : public CModule
{
public:
	MODCONSTRUCTOR(CAdminMod)
	{
	}

	void OnModCommand(const CString& sLine) override;
	EModRet OnUserRaw(CString& sLine) override;

	CString GetInfix() const;
	void SetInfix(const CString& sInfix);

protected:
	EModRet OnUserCommand(CUser* pUser, const CString& sLine);
	EModRet OnNetworkCommand(CIRCNetwork* pNetwork, const CString& sLine);
	EModRet OnChanCommand(CChan* pChan, const CString& sLine);

private:
	template <typename C>
	void OnHelpCommand(const CString& sLine, const std::vector<C>& vCmds);
	template <typename T, typename V>
	void OnListVarsCommand(T* pObject, const CString& sLine, const std::vector<V>& vVars);
	template <typename T, typename V>
	void OnGetCommand(T* pObject, const CString& sLine, const std::vector<V>& vVars);
	template <typename T, typename V>
	void OnSetCommand(T* pObject, const CString& sLine, const std::vector<V>& vVars);
	template <typename T, typename V>
	void OnResetCommand(T* pObject, const CString& sLine, const std::vector<V>& vVars);
	template <typename T, typename C>
	void OnOtherCommand(T* pObject, const CString& sLine, const std::vector<C>& vCmds);

	template <typename C>
	CTable FilterCmdTable(const std::vector<C>& vCmds, const CString& sFilter) const;
	template <typename V>
	CTable FilterVarTable(const std::vector<V>& vVars, const CString& sFilter) const;

	void PutError(const CString& sLine, const CString& sTarget = "");
	void PutLine(const CString& sLine, const CString& sTarget = "");
	void PutTable(const CTable& Table, const CString& sTarget = "");

	CString m_sTarget;


	// TODO: expose the default constants needed by the reset methods?

	const std::vector<Variable<CZNC>> GlobalVars = {
		{
			"AnonIPLimit", IntType,
			"The limit of anonymous unidentified connections per IP.",
			[=](const CZNC* pZNC) {
				return CString(pZNC->GetAnonIPLimit());
			},
			[=](CZNC* pZNC, const CString& sVal) {
				pZNC->SetAnonIPLimit(sVal.ToUInt());
				return true;
			},
			[=](CZNC* pZNC) {
				pZNC->SetAnonIPLimit(10);
				return true;
			}
		},
		// TODO: BindHost
		{
			"ConnectDelay", IntType,
			"The number of seconds every IRC connection is delayed.",
			[=](const CZNC* pZNC) {
				return CString(pZNC->GetConnectDelay());
			},
			[=](CZNC* pZNC, const CString& sVal) {
				pZNC->SetConnectDelay(sVal.ToUInt());
				return true;
			},
			[=](CZNC* pZNC) {
				pZNC->SetConnectDelay(5);
				return true;
			}
		},
		{
			"HideVersion", BoolType,
			"Whether the version number is hidden from the web interface and CTCP VERSION replies.",
			[=](const CZNC* pZNC) {
				return CString(pZNC->GetHideVersion());
			},
			[=](CZNC* pZNC, const CString& sVal) {
				pZNC->SetHideVersion(sVal.ToBool());
				return true;
			},
			[=](CZNC* pZNC) {
				pZNC->SetHideVersion(false);
				return true;
			}
		},
		{
			"MaxBufferSize", IntType,
			"The maximum playback buffer size. Only admin users can exceed the limit.",
			[=](const CZNC* pZNC) {
				return CString(pZNC->GetMaxBufferSize());
			},
			[=](CZNC* pZNC, const CString& sVal) {
				pZNC->SetMaxBufferSize(sVal.ToUInt());
				return true;
			},
			[=](CZNC* pZNC) {
				pZNC->SetMaxBufferSize(500);
				return true;
			}
		},
		{
			"Motd", ListType,
			"The list of 'message of the day' lines that are sent to clients on connect via notice from *status.",
			[=](const CZNC* pZNC) {
				const VCString& vsMotd = pZNC->GetMotd();
				return CString("\n").Join(vsMotd.begin(), vsMotd.end());
			},
			[=](CZNC* pZNC, const CString& sVal) {
				pZNC->AddMotd(sVal);
				return true;
			},
			[=](CZNC* pZNC) {
				pZNC->ClearMotd();
				return true;
			},
		},
		// TODO: PidFile
		{
			"ProtectWebSessions", BoolType,
			"Whether IP changing during each web session is disallowed.",
			[=](const CZNC* pZNC) {
				return CString(pZNC->GetProtectWebSessions());
			},
			[=](CZNC* pZNC, const CString& sVal) {
				pZNC->SetProtectWebSessions(sVal.ToBool());
				return true;
			},
			[=](CZNC* pZNC) {
				pZNC->SetProtectWebSessions(true);
				return true;
			}
		},
		{
			"ServerThrottle", IntType,
			"The number of seconds between connect attempts to the same hostname.",
			[=](const CZNC* pZNC) {
				return CString(pZNC->GetServerThrottle());
			},
			[=](CZNC* pZNC, const CString& sVal) {
				pZNC->SetServerThrottle(sVal.ToUInt());
				return true;
			},
			[=](CZNC* pZNC) {
				pZNC->SetServerThrottle(30);
				return true;
			}
		},
		{
			"Skin", StringType,
			"The default web interface skin.",
			[=](const CZNC* pZNC) {
				return pZNC->GetSkinName();
			},
			[=](CZNC* pZNC, const CString& sVal) {
				pZNC->SetSkinName(sVal);
				return true;
			},
			[=](CZNC* pZNC) {
				pZNC->SetSkinName("");
				return true;
			}
		},
		{
			"SSLCertFile", StringType,
			"The TLS/SSL certificate file from which ZNC reads its server certificate.",
			[=](const CZNC* pZNC) {
				return pZNC->GetSSLCertFile();
			},
			[=](CZNC* pZNC, const CString& sVal) {
				pZNC->SetSSLCertFile(sVal);
				return true;
			},
			[=](CZNC* pZNC) {
				pZNC->SetSSLCertFile(pZNC->GetZNCPath() + "/znc.pem");
				return true;
			},
		},
		{
			"SSLCiphers", StringType,
			"The allowed SSL ciphers. Default value is from Mozilla's recomendations.",
			[=](const CZNC* pZNC) {
				return pZNC->GetSSLCiphers();
			},
			[=](CZNC* pZNC, const CString& sVal) {
				pZNC->SetSSLCiphers(sVal);
				return true;
			},
			[=](CZNC* pZNC) {
				pZNC->SetSSLCiphers("");
				return true;
			},
		},
		{
			"SSLProtocols", StringType,
			"The accepted SSL protocols. Available protocols are All, SSLv2, SSLv3, TLSv1, TLSv1.1 and TLSv1.2.",
			[=](const CZNC* pZNC) {
				return pZNC->GetSSLProtocols();
			},
			[=](CZNC* pZNC, const CString& sVal) {
				if (!pZNC->SetSSLProtocols(sVal)) {
					PutError("unknown protocol");
					return false;
				}
				return true;
			},
			[=](CZNC* pZNC) {
				pZNC->SetSSLProtocols("");
				return true;
			},
		},
		{
			"StatusPrefix", StringType,
			"The default prefix for status and module queries.",
			[=](const CZNC* pZNC) {
				return pZNC->GetSkinName();
			},
			[=](CZNC* pZNC, const CString& sVal) {
				pZNC->SetSkinName(sVal);
				return true;
			},
			[=](CZNC* pZNC) {
				pZNC->SetStatusPrefix("");
				return true;
			}
		},
		{
			"TrustedProxy", ListType,
			"The list of trusted proxies.",
			[=](const CZNC* pZNC) {
				const VCString& vsProxies = pZNC->GetTrustedProxies();
				return CString("\n").Join(vsProxies.begin(), vsProxies.end());
			},
			[=](CZNC* pZNC, const CString& sVal) {
				SCString ssProxies;
				sVal.Split(" ", ssProxies, false);
				for (const CString& sProxy : ssProxies)
					pZNC->AddTrustedProxy(sProxy);
				return true;
			},
			[=](CZNC* pZNC) {
				pZNC->ClearTrustedProxies();
				return true;
			}
		},
	};

	const std::vector<Variable<CUser>> UserVars = {
		{
			"Admin", BoolType,
			"Whether the user has admin rights.",
			[=](const CUser* pObject) {
				return CString(pObject->IsAdmin());
			},
			[=](CUser* pObject, const CString& sVal) {
				pObject->SetAdmin(sVal.ToBool());
				return true;
			},
			[=](CUser* pObject) {
				pObject->SetAdmin(false);
				return true;
			}
		},
		{
			"AdminInfix", StringType,
			"An infix (after the status prefix) for admin queries.",
			[=](const CUser* pObject) {
				return GetInfix();
			},
			[=](CUser* pObject, const CString& sVal) {
				SetInfix(sVal);
				return true;
			},
			[=](CUser* pObject) {
				SetInfix(GetUser()->GetStatusPrefix());
				return true;
			}
		},
		{
			"Allow", ListType,
			"The list of allowed IPs for the user. Wildcards (*) are supported.",
			[=](const CUser* pObject) {
				const SCString& ssHosts = pObject->GetAllowedHosts();
				return CString("\n").Join(ssHosts.begin(), ssHosts.end());
			},
			[=](CUser* pObject, const CString& sVal) {
				SCString ssHosts;
				sVal.Split(" ", ssHosts, false);
				for (const CString& sHost : ssHosts)
					pObject->AddAllowedHost(sHost);
				return true;
			},
			[=](CUser* pObject) {
				pObject->ClearAllowedHosts();
				return true;
			}
		},
		{
			"AltNick", StringType,
			"The default alternate nick.",
			[=](const CUser* pObject) {
				return pObject->GetAltNick();
			},
			[=](CUser* pObject, const CString& sVal) {
				pObject->SetAltNick(sVal);
				return true;
			},
			[=](CUser* pObject) {
				pObject->SetAltNick("");
				return true;
			}
		},
		{
			"AppendTimestamp", BoolType,
			"Whether timestamps are appended to buffer playback messages.",
			[=](const CUser* pObject) {
				return CString(pObject->GetTimestampAppend());
			},
			[=](CUser* pObject, const CString& sVal) {
				pObject->SetTimestampAppend(sVal.ToBool());
				return true;
			},
			[=](CUser* pObject) {
				pObject->SetTimestampAppend(false);
				return true;
			}
		},
		{
			"AutoClearChanBuffer", BoolType,
			"Whether channel buffers are automatically cleared after playback.",
			[=](const CUser* pObject) {
				return CString(pObject->AutoClearChanBuffer());
			},
			[=](CUser* pObject, const CString& sVal) {
				pObject->SetAutoClearChanBuffer(sVal.ToBool());
				return true;
			},
			[=](CUser* pObject) {
				pObject->SetAutoClearChanBuffer(true);
				return true;
			}
		},
		{
			"AutoClearQueryBuffer", BoolType,
			"Whether query buffers are automatically cleared after playback.",
			[=](const CUser* pObject) {
				return CString(pObject->AutoClearQueryBuffer());
			},
			[=](CUser* pObject, const CString& sVal) {
				pObject->SetAutoClearQueryBuffer(sVal.ToBool());
				return true;
			},
			[=](CUser* pObject) {
				pObject->SetAutoClearQueryBuffer(true);
				return true;
			}
		},
		{
			"BindHost", StringType,
			"The default bind host.",
			[=](const CUser* pObject) {
				return pObject->GetBindHost();
			},
			[=](CUser* pObject, const CString& sVal) {
				if (!GetUser()->IsAdmin() && GetUser()->DenySetBindHost()) {
					PutError("access denied");
					return false;
				}
				pObject->SetBindHost(sVal);
				return true;
			},
			[=](CUser* pObject) {
				pObject->SetBindHost("");
				return true;
			}
		},
		{
			"ChanBufferSize", IntType,
			"The maximum amount of lines stored for each channel playback buffer.",
			[=](const CUser* pObject) {
				return CString(pObject->GetChanBufferSize());
			},
			[=](CUser* pObject, const CString& sVal) {
				if (!pObject->SetChanBufferSize(sVal.ToUInt(), GetUser()->IsAdmin())) {
					PutError("exceeded limit " + CString(CZNC::Get().GetMaxBufferSize()));
					return false;
				}
				return true;
			},
			[=](CUser* pObject) {
				pObject->SetChanBufferSize(50);
				return true;
			},
		},
		{
			"ChanModes", StringType,
			"The default modes ZNC sets when joining an empty channel.",
			[=](const CUser* pObject) {
				return pObject->GetDefaultChanModes();
			},
			[=](CUser* pObject, const CString& sVal) {
				pObject->SetDefaultChanModes(sVal);
				return true;
			},
			[=](CUser* pObject) {
				pObject->SetDefaultChanModes("");
				return true;
			}
		},
	#ifdef HAVE_ICU
		{
			"ClientEncoding", StringType,
			"The default client encoding.",
			[=](const CUser* pObject) {
				return pObject->GetClientEncoding();
			},
			[=](CUser* pObject, const CString& sVal) {
				pObject->SetClientEncoding(sVal);
				return true;
			},
			[=](CUser* pObject) {
				pObject->SetClientEncoding("");
				return true;
			}
		},
	#endif
		{
			"CTCPReply", ListType,
			"A list of CTCP request-reply-pairs. Syntax: <request> <reply>.",
			[=](const CUser* pObject) {
				VCString vsReplies;
				for (const auto& it : pObject->GetCTCPReplies())
					vsReplies.push_back(it.first + " " + it.second);
				return CString("\n").Join(vsReplies.begin(), vsReplies.end());
			},
			[=](CUser* pObject, const CString& sVal) {
				CString sRequest = sVal.Token(0);
				CString sReply = sVal.Token(1, true);
				if (sReply.empty()) {
					if (!pObject->DelCTCPReply(sRequest.AsUpper())) {
						PutError("unable to remove");
						return false;
					}
				} else {
					if (!pObject->AddCTCPReply(sRequest, sReply)) {
						PutError("unable to add");
						return false;
					}
				}
				return true;
			},
			[=](CUser* pObject) {
				MCString mReplies = pObject->GetCTCPReplies();
				for (const auto& it : mReplies)
					pObject->DelCTCPReply(it.first);
				return true;
			}
		},
		{
			"DCCBindHost", StringType,
			"An optional bindhost for DCC connections.",
			[=](const CUser* pObject) {
				return pObject->GetDCCBindHost();
			},
			[=](CUser* pObject, const CString& sVal) {
				if (!GetUser()->IsAdmin() && GetUser()->DenySetBindHost()) {
					PutError("access denied");
					return false;
				}
				pObject->SetDCCBindHost(sVal);
				return true;
			},
			[=](CUser* pObject) {
				pObject->SetDCCBindHost("");
				return true;
			}
		},
		{
			"DenyLoadMod", BoolType,
			"Whether the user is denied access to load modules.",
			[=](const CUser* pObject) {
				return CString(pObject->DenyLoadMod());
			},
			[=](CUser* pObject, const CString& sVal) {
				if (!GetUser()->IsAdmin()) {
					PutError("access denied");
					return false;
				}
				pObject->SetDenyLoadMod(sVal.ToBool());
				return true;
			},
			[=](CUser* pObject) {
				if (!GetUser()->IsAdmin()) {
					PutError("access denied");
					return false;
				}
				pObject->SetDenyLoadMod(false);
				return true;
			}
		},
		{
			"DenySetBindHost", BoolType,
			"Whether the user is denied access to set a bind host.",
			[=](const CUser* pObject) {
				return CString(pObject->DenySetBindHost());
			},
			[=](CUser* pObject, const CString& sVal) {
				if (!GetUser()->IsAdmin()) {
					PutError("access denied");
					return false;
				}
				pObject->SetDenySetBindHost(sVal.ToBool());
				return true;
			},
			[=](CUser* pObject) {
				if (!GetUser()->IsAdmin()) {
					PutError("access denied");
					return false;
				}
				pObject->SetDenySetBindHost(false);
				return true;
			}
		},
		{
			"Ident", StringType,
			"The default ident.",
			[=](const CUser* pObject) {
				return pObject->GetIdent();
			},
			[=](CUser* pObject, const CString& sVal) {
				pObject->SetIdent(sVal);
				return true;
			},
			nullptr
		},
		{
			"JoinTries", IntType,
			"The amount of times channels are attempted to join in case of a failure.",
			[=](const CUser* pObject) {
				return CString(pObject->JoinTries());
			},
			[=](CUser* pObject, const CString& sVal) {
				pObject->SetJoinTries(sVal.ToUInt());
				return true;
			},
			[=](CUser* pObject) {
				pObject->SetJoinTries(10);
				return true;
			}
		},
		{
			"MaxJoins", IntType,
			"The maximum number of channels ZNC joins at once.",
			[=](const CUser* pObject) {
				return CString(pObject->MaxJoins());
			},
			[=](CUser* pObject, const CString& sVal) {
				pObject->SetMaxJoins(sVal.ToUInt());
				return true;
			},
			[=](CUser* pObject) {
				pObject->SetMaxJoins(0);
				return true;
			}
		},
		{
			"MaxNetworks", IntType,
			"The maximum number of networks the user is allowed to have.",
			[=](const CUser* pObject) {
				return CString(pObject->MaxNetworks());
			},
			[=](CUser* pObject, const CString& sVal) {
				if (!GetUser()->IsAdmin()) {
					PutError("access denied");
					return false;
				}
				pObject->SetMaxNetworks(sVal.ToUInt());
				return true;
			},
			[=](CUser* pObject) {
				if (!GetUser()->IsAdmin()) {
					PutError("access denied");
					return false;
				}
				pObject->SetMaxNetworks(1);
				return true;
			}
		},
		{
			"MaxQueryBuffers", IntType,
			"The maximum number of query buffers that are stored.",
			[=](const CUser* pObject) {
				return CString(pObject->MaxQueryBuffers());
			},
			[=](CUser* pObject, const CString& sVal) {
				pObject->SetMaxQueryBuffers(sVal.ToUInt());
				return true;
			},
			[=](CUser* pObject) {
				pObject->SetMaxQueryBuffers(50);
				return true;
			}
		},
		{
			"MultiClients", BoolType,
			"Whether multiple clients are allowed to connect simultaneously.",
			[=](const CUser* pObject) {
				return CString(pObject->MultiClients());
			},
			[=](CUser* pObject, const CString& sVal) {
				pObject->SetMultiClients(sVal.ToBool());
				return true;
			},
			[=](CUser* pObject) {
				pObject->SetMultiClients(true);
				return true;
			}
		},
		{
			"Nick", StringType,
			"The default primary nick.",
			[=](const CUser* pObject) {
				return pObject->GetNick();
			},
			[=](CUser* pObject, const CString& sVal) {
				pObject->SetNick(sVal);
				return true;
			},
			[=](CUser* pObject) {
				pObject->SetNick("");
				return true;
			}
		},
		{
			"PrependTimestamp", BoolType,
			"Whether timestamps are prepended to buffer playback messages.",
			[=](const CUser* pObject) {
				return CString(pObject->GetTimestampPrepend());
			},
			[=](CUser* pObject, const CString& sVal) {
				pObject->SetTimestampPrepend(sVal.ToBool());
				return true;
			},
			[=](CUser* pObject) {
				pObject->SetTimestampPrepend(true);
				return true;
			}
		},
		{
			"Password", StringType,
			"",
			[=](const CUser* pObject) {
				return CString(".", pObject->GetPass().size());
			},
			[=](CUser* pObject, const CString& sVal) {
				const CString sSalt = CUtils::GetSalt();
				const CString sHash = CUser::SaltedHash(sVal, sSalt);
				pObject->SetPass(sHash, CUser::HASH_DEFAULT, sSalt);
				return true;
			},
			nullptr
		},
		{
			"QueryBufferSize", IntType,
			"The maximum amount of lines stored for each query playback buffer.",
			[=](const CUser* pObject) {
				return CString(pObject->GetQueryBufferSize());
			},
			[=](CUser* pObject, const CString& sVal) {
				if (!pObject->SetQueryBufferSize(sVal.ToUInt(), GetUser()->IsAdmin())) {
					PutError("exceeded limit " + CString(CZNC::Get().GetMaxBufferSize()));
					return false;
				}
				return true;
			},
			[=](CUser* pObject) {
				pObject->SetQueryBufferSize(50);
				return true;
			}
		},
		{
			"QuitMsg", StringType,
			"The default quit message ZNC uses when disconnecting or shutting down.",
			[=](const CUser* pObject) {
				return pObject->GetQuitMsg();
			},
			[=](CUser* pObject, const CString& sVal) {
				pObject->SetQuitMsg(sVal);
				return true;
			},
			[=](CUser* pObject) {
				pObject->SetQuitMsg("");
				return true;
			}
		},
		{
			"RealName", StringType,
			"The default real name.",
			[=](const CUser* pObject) {
				return pObject->GetRealName();
			},
			[=](CUser* pObject, const CString& sVal) {
				pObject->SetRealName(sVal);
				return true;
			},
			[=](CUser* pObject) {
				pObject->SetRealName("");
				return true;
			}
		},
		{
			"Skin", StringType,
			"The web interface skin.",
			[=](const CUser* pObject) {
				return pObject->GetSkinName();
			},
			[=](CUser* pObject, const CString& sVal) {
				pObject->SetSkinName(sVal);
				return true;
			},
			[=](CUser* pObject) {
				pObject->SetSkinName("");
				return true;
			}
		},
		{
			"StatusPrefix", StringType,
			"The prefix for status and module queries.",
			[=](const CUser* pObject) {
				return pObject->GetStatusPrefix();
			},
			[=](CUser* pObject, const CString& sVal) {
				pObject->SetStatusPrefix(sVal);
				return true;
			},
			[=](CUser* pObject) {
				pObject->SetStatusPrefix("*");
				return true;
			}
		},
		{
			"TimestampFormat", StringType,
			"The format of the timestamps used in buffer playback messages.",
			[=](const CUser* pObject) {
				return pObject->GetTimestampFormat();
			},
			[=](CUser* pObject, const CString& sVal) {
				pObject->SetTimestampFormat(sVal);
				return true;
			},
			[=](CUser* pObject) {
				pObject->SetTimestampFormat("[%H:%M:%S]");
				return true;
			}
		},
		{
			"Timezone", StringType,
			"The timezone used for timestamps in buffer playback messages.",
			[=](const CUser* pObject) {
				return pObject->GetTimezone();
			},
			[=](CUser* pObject, const CString& sVal) {
				pObject->SetTimezone(sVal);
				return true;
			},
			[=](CUser* pObject) {
				pObject->SetTimezone("");
				return true;
			}
		},
	};

	const std::vector<Variable<CIRCNetwork>> NetworkVars = {
		{
			"AltNick", StringType,
			"An optional network specific alternate nick used if the primary nick is reserved.",
			[=](const CIRCNetwork* pObject) {
				return pObject->GetAltNick();
			},
			[=](CIRCNetwork* pObject, const CString& sVal) {
				pObject->SetAltNick(sVal);
				return true;
			},
			[=](CIRCNetwork* pObject) {
				pObject->SetAltNick("");
				return true;
			}
		},
		{
			"BindHost", StringType,
			"An optional network specific bind host.",
			[=](const CIRCNetwork* pObject) {
				return pObject->GetBindHost();
			},
			[=](CIRCNetwork* pObject, const CString& sVal) {
				if (!GetUser()->IsAdmin() && GetUser()->DenySetBindHost()) {
					PutError("access denied");
					return false;
				}
				pObject->SetBindHost(sVal);
				return true;
			},
			[=](CIRCNetwork* pObject) {
				pObject->SetBindHost("");
				return true;
			}
		},
	#ifdef HAVE_ICU
		{
			"Encoding", StringType,
			"An optional network specific client encoding.",
			[=](const CIRCNetwork* pObject) {
				return pObject->GetEncoding();
			},
			[=](CIRCNetwork* pObject, const CString& sVal) {
				pObject->SetEncoding(sVal);
				return true;
			},
			[=](CIRCNetwork* pObject) {
				pObject->SetEncoding("");
				return true;
			}
		},
	#endif
		{
			"FloodBurst", IntType,
			"The maximum amount of lines ZNC sends at once.",
			[=](const CIRCNetwork* pObject) {
				return CString(pObject->GetFloodBurst());
			},
			[=](CIRCNetwork* pObject, const CString& sVal) {
				pObject->SetFloodBurst(sVal.ToUShort());
				return true;
			},
			[=](CIRCNetwork* pObject) {
				pObject->SetFloodBurst(4);
				return true;
			}
		},
		{
			"FloodRate", DoubleType,
			"The number of lines per second ZNC sends after reaching the FloodBurst limit.",
			[=](const CIRCNetwork* pObject) {
				return CString(pObject->GetFloodRate());
			},
			[=](CIRCNetwork* pObject, const CString& sVal) {
				pObject->SetFloodRate(sVal.ToDouble());
				return true;
			},
			[=](CIRCNetwork* pObject) {
				pObject->SetFloodRate(1);
				return true;
			}
		},
		{
			"Ident", StringType,
			"An optional network specific ident.",
			[=](const CIRCNetwork* pObject) {
				return pObject->GetIdent();
			},
			[=](CIRCNetwork* pObject, const CString& sVal) {
				pObject->SetIdent(sVal);
				return true;
			},
			[=](CIRCNetwork* pObject) {
				pObject->SetIdent("");
				return true;
			}
		},
		// TODO: IRCConnectEnabled?
		{
			"JoinDelay", IntType,
			"The delay in seconds, until channels are joined after getting connected.",
			[=](const CIRCNetwork* pObject) {
				return CString(pObject->GetJoinDelay());
			},
			[=](CIRCNetwork* pObject, const CString& sVal) {
				pObject->SetJoinDelay(sVal.ToUShort());
				return true;
			},
			[=](CIRCNetwork* pObject) {
				pObject->SetJoinDelay(0);
				return true;
			}
		},
		{
			"Nick", StringType,
			"An optional network specific primary nick.",
			[=](const CIRCNetwork* pObject) {
				return pObject->GetNick();
			},
			[=](CIRCNetwork* pObject, const CString& sVal) {
				pObject->SetNick(sVal);
				return true;
			},
			[=](CIRCNetwork* pObject) {
				pObject->SetNick("");
				return true;
			}
		},
		{
			"QuitMsg", StringType,
			"An optional network specific quit message ZNC uses when disconnecting or shutting down.",
			[=](const CIRCNetwork* pObject) {
				return pObject->GetQuitMsg();
			},
			[=](CIRCNetwork* pObject, const CString& sVal) {
				pObject->SetQuitMsg(sVal);
				return true;
			},
			[=](CIRCNetwork* pObject) {
				pObject->SetQuitMsg("");
				return true;
			}
		},
		{
			"RealName", StringType,
			"An optional network specific real name.",
			[=](const CIRCNetwork* pObject) {
				return pObject->GetRealName();
			},
			[=](CIRCNetwork* pObject, const CString& sVal) {
				pObject->SetRealName(sVal);
				return true;
			},
			[=](CIRCNetwork* pObject) {
				pObject->SetRealName("");
				return true;
			}
		},
		{
			"TrustedServerFingerprint", ListType,
			"The list of trusted server fingerprints.",
			[=](const CIRCNetwork* pObject) {
				const SCString& sFP = pObject->GetTrustedFingerprints();
				return CString("\n").Join(sFP.begin(), sFP.end());
			},
			[=](CIRCNetwork* pObject, const CString& sVal) {
				pObject->AddTrustedFingerprint(sVal);
				return true;
			},
			[=](CIRCNetwork* pObject) {
				pObject->ClearTrustedFingerprints();
				return true;
			}
		},
	};

	const std::vector<Variable<CChan>> ChanVars = {
		{
			"AutoClearChanBuffer", BoolType,
			"Whether the channel buffer is automatically cleared after playback.",
			[=](const CChan* pObject) {
				CString sVal(pObject->AutoClearChanBuffer());
				if (!pObject->HasAutoClearChanBufferSet())
					sVal += " (default)";
				return sVal;
			},
			[=](CChan* pObject, const CString& sVal) {
				pObject->SetAutoClearChanBuffer(sVal.ToBool());
				return true;
			},
			[=](CChan* pObject) {
				pObject->ResetAutoClearChanBuffer();
				return true;
			}
		},
		{
			"Buffer", IntType,
			"The maximum amount of lines stored for the channel specific playback buffer.",
			[=](const CChan* pObject) {
				CString sVal(pObject->GetBufferCount());
				if (!pObject->HasBufferCountSet())
					sVal += " (default)";
				return sVal;
			},
			[=](CChan* pObject, const CString& sVal) {
				if (!pObject->SetBufferCount(sVal.ToUInt(), GetUser()->IsAdmin())) {
					PutError("exceeded limit " + CString(CZNC::Get().GetMaxBufferSize()));
					return false;
				}
				return true;
			},
			[=](CChan* pObject) {
				pObject->ResetBufferCount();
				return true;
			}
		},
		{
			"Detached", BoolType,
			"Whether the channel is detached.",
			[=](const CChan* pObject) {
				return CString(pObject->IsDetached());
			},
			[=](CChan* pObject, const CString& sVal) {
				bool b = sVal.ToBool();
				if (b != pObject->IsDetached()) {
					if (b)
						pObject->DetachUser();
					else
						pObject->AttachUser();
				}
				return true;
			},
			[=](CChan* pObject) {
				if (pObject->IsDetached())
					pObject->AttachUser();
				return true;
			}
		},
		{
			"Disabled", BoolType,
			"Whether the channel is disabled.",
			[=](const CChan* pObject) {
				return CString(pObject->IsDisabled());
			},
			[=](CChan* pObject, const CString& sVal) {
				bool b = sVal.ToBool();
				if (b != pObject->IsDisabled()) {
					if (b)
						pObject->Disable();
					else
						pObject->Enable();
				}
				return true;
			},
			[=](CChan* pObject) {
				if (pObject->IsDisabled())
					pObject->Enable();
				return true;
			}
		},
		{
			"InConfig", BoolType,
			"Whether the channel is stored in the config file.",
			[=](const CChan* pObject) {
				return CString(pObject->InConfig());
			},
			[=](CChan* pObject, const CString& sVal) {
				pObject->SetInConfig(sVal.ToBool());
				return true;
			},
			nullptr
		},
		{
			"Key", StringType,
			"An optional channel key.",
			[=](const CChan* pObject) {
				return pObject->GetKey();
			},
			[=](CChan* pObject, const CString& sVal) {
				pObject->SetKey(sVal);
				return true;
			},
			[=](CChan* pObject) {
				pObject->SetKey("");
				return true;
			}
		},
		{
			"Modes", StringType,
			"An optional set of default channel modes ZNC sets when joining an empty channel.",
			[=](const CChan* pObject) {
				return pObject->GetDefaultModes();
			},
			[=](CChan* pObject, const CString& sVal) {
				pObject->SetDefaultModes(sVal);
				return true;
			},
			[=](CChan* pObject) {
				pObject->SetDefaultModes("");
				return true;
			}
		},
	};

	const std::vector<Command<CZNC>> GlobalCmds = {
		{
			"AddUser <username> <password>",
			"Adds a new user.",
			[=](CZNC* pObject, const CString& sArgs) {
				if (!GetUser()->IsAdmin()) {
					PutError("access denied");
					return false;
				}

				const CString sUsername = sArgs.Token(0);
				const CString sPassword = sArgs.Token(1);
				if (sPassword.empty())
					return false;

				if (pObject->FindUser(sUsername)) {
					PutError("already exists");
					return false;
				}

				CUser* pUser = new CUser(sUsername);
				CString sSalt = CUtils::GetSalt();
				pUser->SetPass(CUser::SaltedHash(sPassword, sSalt), CUser::HASH_DEFAULT, sSalt);

				CString sError;
				if (!CZNC::Get().AddUser(pUser, sError)) {
					PutError(sError);
					delete pUser;
					return false;
				}

				return true;
			}
		},
		{
			"DelUser <username>",
			"Deletes a user.",
			[=](CZNC* pObject, const CString& sArgs) {
				if (!GetUser()->IsAdmin()) {
					PutError("access denied");
					return false;
				}

				const CString sUsername = sArgs.Token(0);
				if (sUsername.empty())
					return false;

				CUser *pUser = CZNC::Get().FindUser(sUsername);
				if (!pUser) {
					PutError("doesn't exist");
					return false;
				}

				if (pUser == GetUser()) {
					PutError("access denied");
					return false;
				}

				CZNC::Get().DeleteUser(pUser->GetUserName());
				return true;
			}
		},
	};

	const std::vector<Command<CUser>> UserCmds = {
	};

	const std::vector<Command<CIRCNetwork>> NetworkCmds = {
	};

	const std::vector<Command<CChan>> ChanCmds = {
	};
};


CString CAdminMod::GetInfix() const
{
	CString sInfix = GetNV("infix");
	if (sInfix.empty())
		sInfix = GetUser()->GetStatusPrefix();
	return sInfix;
}

void CAdminMod::SetInfix(const CString& sInfix)
{
	SetNV("infix", sInfix);
}

void CAdminMod::OnModCommand(const CString& sLine)
{
	const CString sCmd = sLine.Token(0);

	m_sTarget = GetModName();

	if (!GetUser()->IsAdmin() && (sCmd.Equals("Set") || sCmd.Equals("Reset"))) {
		PutError("access denied.");
		return;
	}

	if (sCmd.Equals("Help")) {
		const CString sFilter = sLine.Token(1);

		const CTable Table = FilterCmdTable(GlobalCmds, sFilter);
		if (!Table.empty())
			PutModule(Table);
		else if (!sFilter.empty())
			PutModule("No matches for '" + sFilter + "'");

		const CString sPfx = GetUser()->GetStatusPrefix() + GetInfix();

		if (sFilter.empty()) {
			PutModule("To access settings of the current user or network, open a query");
			PutModule("with " + sPfx + "user or " + sPfx + "network, respectively.");
			PutModule("-----");
			PutModule("- user settings: /msg " + sPfx + "user help");
			PutModule("- network settings: /msg " + sPfx + "network help");
			PutModule("-----");
			PutModule("To access settings of a different user (admins only) or a specific");
			PutModule("network, open a query with " + sPfx + "target, where target is the name of");
			PutModule("the user or network. The same applies to channel specific settings.");
			PutModule("-----");
			PutModule("- user settings: /msg " + sPfx + "somebody help");
			PutModule("- network settings: /msg " + sPfx + "freenode help");
			PutModule("- channel settings: /msg " + sPfx + "#znc help");
			PutModule("-----");
			PutModule("It is also possible to access the network settings of a different");
			PutModule("user (admins only), or the channel settings of a different network.");
			PutModule("Combine a user, network and channel name separated by a forward");
			PutModule("slash ('/') character.");
			PutModule("-----");
			PutModule("Advanced examples:");
			PutModule("- network settings of another user: /msg " + sPfx + "somebody/freenode help");
			PutModule("- channel settings of another network: /msg " + sPfx + "freenode/#znc help");
			PutModule("- channel settings of another network of another user: /msg " + sPfx + "somebody/freenode/#znc help");
		}
	} else if (sCmd.Equals("ListVars")) {
		OnListVarsCommand(&CZNC::Get(), sLine, GlobalVars);
	} else if (sCmd.Equals("Get")) {
		OnGetCommand(&CZNC::Get(), sLine, GlobalVars);
	} else if (sCmd.Equals("Set")) {
		OnSetCommand(&CZNC::Get(), sLine, GlobalVars);
	} else if (sCmd.Equals("Reset")) {
		OnResetCommand(&CZNC::Get(), sLine, GlobalVars);
	} else {
		OnOtherCommand(&CZNC::Get(), sLine, GlobalCmds);
	}
}

CModule::EModRet CAdminMod::OnUserRaw(CString& sLine)
{
	CString sCopy = sLine;
	if (sCopy.StartsWith("@"))
		sCopy = sCopy.Token(1, true);
	if (sCopy.StartsWith(":"))
		sCopy = sCopy.Token(1, true);

	const CString sCmd = sCopy.Token(0);

	if (sCmd.Equals("ZNC") || sCmd.Equals("PRIVMSG")) {
		CString sTarget = sCopy.Token(1);
		if (sTarget.TrimPrefix(GetUser()->GetStatusPrefix() + GetInfix())) {
			const CString sRest = sCopy.Token(2, true).TrimPrefix_n(":");
			const CString sPfx = GetInfix();

			m_sTarget = GetInfix() + sTarget;

			// <user>
			if (sTarget.Equals("user"))
				return OnUserCommand(GetUser(), sRest);
			if (CUser* pUser = CZNC::Get().FindUser(sTarget))
				return OnUserCommand(pUser, sRest);

			// <network>
			if (sTarget.Equals("network") && GetNetwork())
				return OnNetworkCommand(GetNetwork(), sRest);
			if (CIRCNetwork* pNetwork = GetUser()->FindNetwork(sTarget))
				return OnNetworkCommand(pNetwork, sRest);

			// <#chan>
			if (CChan* pChan = GetNetwork() ? GetNetwork()->FindChan(sTarget) : nullptr)
				return OnChanCommand(pChan, sRest);

			VCString vsParts;
			sTarget.Split("/", vsParts, false);
			if (vsParts.size() == 2) {
				// <user/network>
				if (CUser* pUser = CZNC::Get().FindUser(vsParts[0])) {
					if (CIRCNetwork* pNetwork = pUser->FindNetwork(vsParts[1])) {
						return OnNetworkCommand(pNetwork, sRest);
					} else {
						// <user/#chan>
						if (pUser == GetUser()) {
							if (CIRCNetwork* pUserNetwork = GetNetwork()) {
								if (CChan* pChan = pUserNetwork->FindChan(vsParts[1]))
									return OnChanCommand(pChan, sRest);
							}
						}
						if (pUser->GetNetworks().size() == 1) {
							if (CIRCNetwork* pFirstNetwork = pUser->GetNetworks().front()) {
								if (CChan* pChan = pFirstNetwork->FindChan(vsParts[1]))
									return OnChanCommand(pChan, sRest);
							}
						}
					}
					PutError("unknown (or ambiguous) network or channel");
					return HALT;
				}
				// <network/#chan>
				if (CIRCNetwork* pNetwork = GetUser()->FindNetwork(vsParts[0])) {
					if (CChan* pChan = pNetwork->FindChan(vsParts[1])) {
						return OnChanCommand(pChan, sRest);
					} else {
						PutError("unknown channel");
						return HALT;
					}
				}
			} else if (vsParts.size() == 3) {
				// <user/network/#chan>
				if (CUser* pUser = CZNC::Get().FindUser(vsParts[0])) {
					if (CIRCNetwork* pNetwork = pUser->FindNetwork(vsParts[1])) {
						if (CChan* pChan = pNetwork->FindChan(vsParts[2])) {
							return OnChanCommand(pChan, sRest);
						} else {
							PutError("unknown channel");
							return HALT;
						}
					} else {
						PutError("unknown network");
						return HALT;
					}
				}
			}
		}
	}
	return CONTINUE;
}

CModule::EModRet CAdminMod::OnUserCommand(CUser* pUser, const CString& sLine)
{
	const CString sCmd = sLine.Token(0);

	if (pUser != GetUser() && !GetUser()->IsAdmin()) {
		PutError("access denied");
		return HALT;
	}

	if (sCmd.Equals("Help"))
		OnHelpCommand(sLine, UserCmds);
	else if (sCmd.Equals("ListVars"))
		OnListVarsCommand(pUser, sLine, UserVars);
	else if (sCmd.Equals("Get"))
		OnGetCommand(pUser, sLine, UserVars);
	else if (sCmd.Equals("Set"))
		OnSetCommand(pUser, sLine, UserVars);
	else if (sCmd.Equals("Reset"))
		OnResetCommand(pUser, sLine, UserVars);
	else
		OnOtherCommand(pUser, sLine, UserCmds);

	return HALT;
}

CModule::EModRet CAdminMod::OnNetworkCommand(CIRCNetwork* pNetwork, const CString& sLine)
{
	const CString sCmd = sLine.Token(0);

	if (pNetwork->GetUser() != GetUser() && !GetUser()->IsAdmin()) {
		PutError("access denied");
		return HALT;
	}

	if (sCmd.Equals("Help"))
		OnHelpCommand(sLine, NetworkCmds);
	else if (sCmd.Equals("ListVars"))
		OnListVarsCommand(pNetwork, sLine, NetworkVars);
	else if (sCmd.Equals("Get"))
		OnGetCommand(pNetwork, sLine, NetworkVars);
	else if (sCmd.Equals("Set"))
		OnSetCommand(pNetwork, sLine, NetworkVars);
	else if (sCmd.Equals("Reset"))
		OnResetCommand(pNetwork, sLine, NetworkVars);
	else
		OnOtherCommand(pNetwork, sLine, NetworkCmds);

	return HALT;
}

CModule::EModRet CAdminMod::OnChanCommand(CChan* pChan, const CString& sLine)
{
	const CString sCmd = sLine.Token(0);

	if (pChan->GetNetwork()->GetUser() != GetUser() && !GetUser()->IsAdmin()) {
		PutError("access denied");
		return HALT;
	}

	if (sCmd.Equals("Help"))
		OnHelpCommand(sLine, ChanCmds);
	else if (sCmd.Equals("ListVars"))
		OnListVarsCommand(pChan, sLine, ChanVars);
	else if (sCmd.Equals("Get"))
		OnGetCommand(pChan, sLine, ChanVars);
	else if (sCmd.Equals("Set"))
		OnSetCommand(pChan, sLine, ChanVars);
	else if (sCmd.Equals("Reset"))
		OnResetCommand(pChan, sLine, ChanVars);
	else
		OnOtherCommand(pChan, sLine, ChanCmds);

	return HALT;
}

template <typename C>
void CAdminMod::OnHelpCommand(const CString& sLine, const std::vector<C>& vCmds)
{
	const CString sFilter = sLine.Token(1);

	const CTable Table = FilterCmdTable(vCmds, sFilter);
	if (!Table.empty())
		PutTable(Table);
	else
		PutLine("No matches for '" + sFilter + "'");
}

template <typename T, typename V>
void CAdminMod::OnListVarsCommand(T* pObject, const CString& sLine, const std::vector<V>& vVars)
{
	const CString sFilter = sLine.Token(1);

	const CTable Table = FilterVarTable(vVars, sFilter);
	if (!Table.empty())
		PutTable(Table);
	else
		PutError("unknown variable");
}

template <typename T, typename V>
void CAdminMod::OnGetCommand(T* pObject, const CString& sLine, const std::vector<V>& vVars)
{
	const CString sVar = sLine.Token(1);

	if (sVar.empty()) {
		PutLine("Usage: Get <variable>");
		return;
	}

	bool bFound = false;
	for (const auto& Var : vVars) {
		if (Var.name.WildCmp(sVar, CString::CaseInsensitive)) {
			VCString vsValues;
			Var.getter(pObject).Split("\n", vsValues, false);
			if (vsValues.empty()) {
				PutLine(Var.name + " = ");
			} else {
				for (const CString& s : vsValues)
					PutLine(Var.name + " = " + s);
			}
			bFound = true;
		}
	}

	if (!bFound)
		PutError("unknown variable");
}

template <typename T, typename V>
void CAdminMod::OnSetCommand(T* pObject, const CString& sLine, const std::vector<V>& vVars)
{
	const CString sVar = sLine.Token(1);
	const CString sVal = sLine.Token(2, true);

	if (sVar.empty() || sVal.empty()) {
		PutLine("Usage: Set <variable> <value>");
		return;
	}

	bool bFound = false;
	for (const auto& Var : vVars) {
		if (Var.name.WildCmp(sVar, CString::CaseInsensitive)) {
			if (Var.setter(pObject, sVal)) {
				VCString vsValues;
				Var.getter(pObject).Split("\n", vsValues, false);
				if (vsValues.empty()) {
					PutLine(Var.name + " = ");
				} else {
					for (const CString& s : vsValues)
						PutLine(Var.name + " = " + s);
				}
			}
			bFound = true;
		}
	}

	if (!bFound)
		PutError("unknown variable");
}

template <typename T, typename V>
void CAdminMod::OnResetCommand(T* pObject, const CString& sLine, const std::vector<V>& vVars)
{
	const CString sVar = sLine.Token(1);

	if (sVar.empty()) {
		PutLine("Usage: Reset <variable>");
		return;
	}

	bool bFound = false;
	for (const auto& Var : vVars) {
		if (Var.name.WildCmp(sVar, CString::CaseInsensitive)) {
			if (!Var.resetter) {
				PutError("reset not supported");
			} else if (Var.resetter(pObject)) {
				VCString vsValues;
				Var.getter(pObject).Split("\n", vsValues, false);
				if (vsValues.empty()) {
					PutLine(Var.name + " = ");
				} else {
					for (const CString& s : vsValues)
						PutLine(Var.name + " = " + s);
				}
			}
			bFound = true;
		}
	}

	if (!bFound)
		PutError("unknown variable");
}

template <typename T, typename C>
void CAdminMod::OnOtherCommand(T* pObject, const CString& sLine, const std::vector<C>& vCmds)
{
	const CString sCmd = sLine.Token(0);
	const CString sArgs = sLine.Token(1, true);

	for (const auto& Cmd : vCmds) {
		if (Cmd.syntax.Token(0).Equals(sCmd)) {
			if (Cmd.func(pObject, sArgs))
				PutLine("Ok");
			return;
		}
	}

	PutError("unknown command");
}

template <typename C>
CTable CAdminMod::FilterCmdTable(const std::vector<C>& vCmds, const CString& sFilter) const
{
	CTable Table;
	Table.AddColumn("Command");
	Table.AddColumn("Description");

	if (sFilter.empty() || CString("Get").WildCmp(sFilter, CString::CaseInsensitive)) {
		Table.AddRow();
		Table.SetCell("Command", "Get <variable>");
		Table.SetCell("Description", "Gets the value of a variable.");
	}

	if (sFilter.empty() || CString("Help").WildCmp(sFilter, CString::CaseInsensitive)) {
		Table.AddRow();
		Table.SetCell("Command", "Help [filter]");
		Table.SetCell("Description", "Generates this output.");
	}

	if (sFilter.empty() || CString("ListVars").WildCmp(sFilter, CString::CaseInsensitive)) {
		Table.AddRow();
		Table.SetCell("Command", "ListVars [filter]");
		Table.SetCell("Description", "Lists available variables filtered by name or type.");
	}

	if (sFilter.empty() || CString("Set").WildCmp(sFilter, CString::CaseInsensitive)) {
		Table.AddRow();
		Table.SetCell("Command", "Set <variable> <value>");
		Table.SetCell("Description", "Sets the value of a variable.");
	}

	if (sFilter.empty() || CString("Reset").WildCmp(sFilter, CString::CaseInsensitive)) {
		Table.AddRow();
		Table.SetCell("Command", "Reset <variable> <value>");
		Table.SetCell("Description", "Resets the value(s) of a variable.");
	}

	for (const auto& Cmd : vCmds) {
		const CString sCmd =  Cmd.syntax.Token(0);
		if (sFilter.empty() || sCmd.StartsWith(sFilter) || sCmd.WildCmp(sFilter, CString::CaseInsensitive)) {
			Table.AddRow();
			Table.SetCell("Command", Cmd.syntax);
			Table.SetCell("Description", Cmd.description);
		}
	}

	return Table;
}

template <typename V>
CTable CAdminMod::FilterVarTable(const std::vector<V>& vVars, const CString& sFilter) const
{
	CTable Table;
	Table.AddColumn("Variable");
	Table.AddColumn("Description");

	for (const auto& Var : vVars) {
		CString sType(VarTypes[Var.type]);
		if (sFilter.empty() || sType.Equals(sFilter) || Var.name.StartsWith(sFilter) || Var.name.WildCmp(sFilter, CString::CaseInsensitive)) {
			Table.AddRow();
			Table.SetCell("Variable", Var.name + " (" + sType + ")");
			Table.SetCell("Description", Var.description);
		}
	}

	return Table;
}

void CAdminMod::PutError(const CString& sError, const CString& sTarget)
{
	PutLine("Error: " + sError, sTarget);
}

void CAdminMod::PutLine(const CString& sLine, const CString& sTarget)
{
	const CString sTgt = sTarget.empty() ? (m_sTarget.empty() ? GetModName() : m_sTarget) : sTarget;

	if (CClient* pClient = GetClient())
		pClient->PutModule(sTgt, sLine);
	else if (CIRCNetwork* pNetwork = GetNetwork())
		pNetwork->PutModule(sTgt, sLine);
	else if (CUser* pUser = GetUser())
		pUser->PutModule(sTgt, sLine);
}

void CAdminMod::PutTable(const CTable& Table, const CString& sTarget)
{
	CString sLine;
	unsigned int i = 0;
	while (Table.GetLine(i++, sLine))
		PutLine(sLine, sTarget);
}

template<> void TModInfo<CAdminMod>(CModInfo& Info) {
}

USERMODULEDEFS(CAdminMod, "Administer ZNC conveniently through IRC.")
