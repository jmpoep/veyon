/*
 * WindowsNetworkFunctions.cpp - implementation of WindowsNetworkFunctions class
 *
 * Copyright (c) 2017-2025 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#define INITGUID
#include <winsock2.h>
#include <windows.h>
#include <netfw.h>
#include <mstcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>

#include <QHostAddress>
#include <QProcess>

#include "HostAddress.h"
#include "WindowsCoreFunctions.h"
#include "WindowsNetworkFunctions.h"


static HRESULT WindowsFirewallInitialize2( INetFwPolicy2** fwPolicy2 )
{
	HRESULT hr = S_OK;

	// Create an instance of the firewall settings manager.
	hr = CoCreateInstance( CLSID_NetFwPolicy2, nullptr, CLSCTX_INPROC_SERVER,
							IID_INetFwPolicy2, reinterpret_cast<void**>( fwPolicy2 ) );
	if( FAILED( hr ) )
	{
		vCritical() << "CoCreateInstance() returned" << hr;
	}

	return hr;
}


static void WindowsFirewallCleanup2( INetFwPolicy2* fwPolicy2 )
{
	if( fwPolicy2 != nullptr )
	{
		fwPolicy2->Release();
	}
}



static HRESULT WindowsFirewallAddApp2( INetFwPolicy2* fwPolicy2,
									   const wchar_t* fwApplicationPath,
									   const wchar_t* fwName )
{
	HRESULT hr = S_OK;
	BSTR fwBstrRuleName = nullptr;
	BSTR fwBstrApplicationPath = nullptr;
	BSTR fwBstrRuleDescription = nullptr;
	BSTR fwBstrRuleGrouping = nullptr;

	INetFwRules *pFwRules = nullptr;
	INetFwRule *pFwRule = nullptr;

	// Retrieve INetFwRules
	hr = fwPolicy2->get_Rules( &pFwRules );
	if( FAILED( hr ) )
	{
		vCritical() << "get_Rules() returned" << hr;
		goto cleanup;
	}

	// Create an instance of an authorized application.
	hr = CoCreateInstance( CLSID_NetFwRule, nullptr,
							CLSCTX_INPROC_SERVER,
							IID_INetFwRule, reinterpret_cast<void**>( &pFwRule ) );
	if( FAILED( hr ) )
	{
		vCritical() << "CoCreateInstance() returned" << hr;
		goto cleanup;
	}

	fwBstrRuleName = SysAllocString( fwName );
	fwBstrApplicationPath = SysAllocString( fwApplicationPath );
	fwBstrRuleDescription = SysAllocString( fwName );
	fwBstrRuleGrouping = SysAllocString( fwName );

	// Set the rule
	pFwRule->put_Name( fwBstrRuleName );
	pFwRule->put_ApplicationName( fwBstrApplicationPath );
	pFwRule->put_Description( fwBstrRuleDescription );
	pFwRule->put_Grouping( fwBstrRuleGrouping );

	pFwRule->put_Action( NET_FW_ACTION_ALLOW );
	pFwRule->put_Enabled( VARIANT_TRUE );
	pFwRule->put_Protocol( NET_FW_IP_PROTOCOL_TCP );
	pFwRule->put_Profiles( NET_FW_PROFILE2_ALL );


	// Add the application to the collection.
	hr = pFwRules->Add( pFwRule );
	if( FAILED( hr ) )
	{
		vCritical() << "Add() returned" << hr;
		goto cleanup;
	}

cleanup:
	// Free the BSTRs.
	SysFreeString( fwBstrRuleName );
	SysFreeString( fwBstrApplicationPath );
	SysFreeString( fwBstrRuleDescription );
	SysFreeString( fwBstrRuleGrouping );

	if( pFwRule != nullptr )
	{
		pFwRule->Release();
	}

	if( pFwRules != nullptr )
	{
		pFwRules->Release();
	}

	return hr;
}



static HRESULT WindowsFirewallRemoveApp2( INetFwPolicy2 * fwPolicy2,
										  const wchar_t* fwName )
{
	HRESULT hr = S_OK;
	BSTR fwBstrRuleName = SysAllocString( fwName );

	INetFwRules *pFwRules = nullptr;

	// Retrieve INetFwRules
	hr = fwPolicy2->get_Rules( &pFwRules );
	if( FAILED( hr ) )
	{
		vCritical() << "get_Rules() returned" << hr;
		goto cleanup;
	}

	// Remove rule
	hr = pFwRules->Remove( fwBstrRuleName );
	if( FAILED( hr ) )
	{
		vCritical() << "Remove() returned" << hr;
		goto cleanup;
	}

cleanup:
	// Free the BSTRs.
	SysFreeString( fwBstrRuleName );

	if( pFwRules != nullptr )
	{
		pFwRules->Release();
	}

	return hr;
}



static bool configureFirewallException( INetFwPolicy2* fwPolicy2, const wchar_t* fwApplicationPath, const wchar_t* fwName, bool enabled )
{
	// always remove firewall exception first
	WindowsFirewallRemoveApp2( fwPolicy2, fwName );

	if( enabled )
	{
		// add service to the list of authorized applications
		const auto hr = WindowsFirewallAddApp2( fwPolicy2, fwApplicationPath, fwName );
		if( hr != S_OK )
		{
			// failed because firewall service not running / disabled?
			if( hr == WindowsNetworkFunctions::WindowsFirewallServiceError )
			{
				// then assume this is intended, log a warning and
				// pretend everything went well
				vWarning() << "Windows Firewall service not running or disabled - can't add or remove firewall exception!";
				return true;
			}

			vCritical() << "WindowsFirewallAddApp2() returned" << hr;
			return false;
		}
	}

	return true;
}



WindowsNetworkFunctions::WindowsNetworkFunctions() : PlatformNetworkFunctions()
{
	WSADATA wsadata;
	const auto error = WSAStartup( MAKEWORD(2,0), &wsadata );
	if( error != S_OK )
	{
		vCritical() << "failed to initialize WinSock:" << error;
	}
}



WindowsNetworkFunctions::PingResult WindowsNetworkFunctions::ping(const QString& hostAddress)
{
	auto result = PingResult::Unknown;

	const auto ipAddress = HostAddress(hostAddress).convert(HostAddress::Type::IpAddress);
	if (ipAddress.isEmpty() == false)
	{
		const auto addressProtocol = QHostAddress(ipAddress).protocol();

		if (addressProtocol == QAbstractSocket::IPv4Protocol && pingIPv4Address(ipAddress, &result))
		{
			return result;
		}

		if (addressProtocol == QAbstractSocket::IPv6Protocol && pingIPv6Address(ipAddress, &result))
		{
			return result;
		}
	}

	if (pingViaUtility(hostAddress, &result))
	{
		return result;
	}

	return PingResult::Unknown;
}



bool WindowsNetworkFunctions::configureFirewallException( const QString& applicationPath, const QString& description, bool enabled )
{
	HRESULT hr = S_OK;

	// initialize COM
	HRESULT comInit = CoInitializeEx( nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE );

	// Ignore RPC_E_CHANGED_MODE; this just means that COM has already been
	// initialized with a different mode. Since we don't care what the mode is,
	// we'll just use the existing mode.
	if( comInit != RPC_E_CHANGED_MODE )
	{
		hr = comInit;
		if( FAILED( hr ) )
		{
			vCritical() << "CoInitializeEx() returned" << hr;
			return false;
		}
	}

	// retrieve current firewall profile
	INetFwPolicy2 *fwPolicy2 = nullptr;
	hr = WindowsFirewallInitialize2( &fwPolicy2 );
	if( FAILED( hr ) )
	{
		vCritical() << "WindowsFirewallInitialize2() returned" << hr;
		return false;
	}

	bool result = ::configureFirewallException( fwPolicy2,
												WindowsCoreFunctions::toConstWCharArray( applicationPath ),
												WindowsCoreFunctions::toConstWCharArray( description ),
												enabled );

	WindowsFirewallCleanup2( fwPolicy2 );

	// Uninitialize COM.
	if( SUCCEEDED( comInit ) )
	{
		CoUninitialize();
	}

	return result;
}



bool WindowsNetworkFunctions::configureSocketKeepalive( Socket socket, bool enabled, int idleTime, int interval, int probes )
{
	Q_UNUSED(probes)

	DWORD optval;
	int optlen = sizeof(optval);

	optval = enabled ? 1 : 0;
	if( setsockopt( socket, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char *>( &optval ), optlen ) != 0 )
	{
		int error = WSAGetLastError();
		vWarning() << "could not set SO_KEEPALIVE" << error;
		return false;
	}

	tcp_keepalive keepalive{};
	keepalive.onoff = enabled ? 1 : 0;
	keepalive.keepalivetime = static_cast<u_long>( idleTime );
	keepalive.keepaliveinterval = static_cast<u_long>( interval );

	DWORD bytesReturned = 0;

	if( WSAIoctl( socket, SIO_KEEPALIVE_VALS, &keepalive, sizeof(keepalive), nullptr, 0,
				  &bytesReturned, nullptr, nullptr ) != 0 )
	{
		int error = WSAGetLastError();
		vWarning() << "could not set keepalive parameters" << error;
		return false;
	}

	return true;
}



bool WindowsNetworkFunctions::pingIPv4Address(const QString& hostAddress, PingResult* result)
{
	if( result == nullptr )
	{
		return false;
	}

	*result = PingResult::Unknown;

	const IPAddr ipAddress = inet_addr(hostAddress.toLatin1().constData());
	if( ipAddress == INADDR_NONE )
	{
		return false;
	}

	const auto icmpHandle = IcmpCreateFile();
	if( icmpHandle == INVALID_HANDLE_VALUE )
	{
		IcmpCloseHandle(icmpHandle);
		return false;
	}

	std::array<char, 6> sendData{'V', 'e', 'y', 'o', 'n', 0};
	std::array<char, sizeof(ICMP_ECHO_REPLY) + sendData.size()> replyBuffer;

	const auto success = IcmpSendEcho(icmpHandle, ipAddress, sendData.data(), sendData.size(),
									   nullptr, replyBuffer.data(), replyBuffer.size(), PingTimeout) > 0 &&
						 reinterpret_cast<ICMP_ECHO_REPLY *>(replyBuffer.data())->Status == IP_SUCCESS;

	const auto error = GetLastError();

	IcmpCloseHandle(icmpHandle);

	if( success )
	{
		*result = PingResult::ReplyReceived;
		return true;
	}

	if (error == IP_REQ_TIMED_OUT)
	{
		*result = PingResult::TimedOut;
		return true;
	}

	return false;
}



bool WindowsNetworkFunctions::pingIPv6Address(const QString& hostAddress, PingResult* result)
{
	if( result == nullptr )
	{
		return false;
	}

	*result = PingResult::Unknown;

	SOCKADDR_IN6 icmp6LocalAddr{};
	icmp6LocalAddr.sin6_addr = in6addr_any;
	icmp6LocalAddr.sin6_family = AF_INET6;

	SOCKADDR_IN6 icmp6RemoteAddr{};
	struct addrinfo hints{};
	struct addrinfo *res = nullptr;

	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;

	if( getaddrinfo(hostAddress.toLatin1().constData(), nullptr, &hints, &res) != 0 )
	{
		return false;
	}

	auto resalloc = res;

	while( res != nullptr )
	{
		if( res->ai_family == AF_INET6 )
		{
			memcpy( &icmp6RemoteAddr, res->ai_addr, res->ai_addrlen );
			icmp6RemoteAddr.sin6_family = AF_INET6;
			break;
		}

		res = res->ai_next;
	}

	freeaddrinfo(resalloc);

	if( icmp6RemoteAddr.sin6_family != AF_INET6 )
	{
		return false;
	}

	const auto icmpFile = Icmp6CreateFile();
	if( icmpFile == INVALID_HANDLE_VALUE )
	{
		return false;
	}

	using ICMPV6_ECHO_REPLY = struct {
		IPV6_ADDRESS_EX Address;
		ULONG Status;
		unsigned int RoundTripTime;
	};

	std::array<char, 6> sendData{'V', 'e', 'y', 'o', 'n', 0};
	std::array<char, sizeof(ICMPV6_ECHO_REPLY) + sendData.size()> replyBuffer;

	const auto success = Icmp6SendEcho2(icmpFile, nullptr, nullptr, nullptr,
										 &icmp6LocalAddr,
										 &icmp6RemoteAddr,
										 sendData.data(), sendData.size(),
										 nullptr, replyBuffer.data(), replyBuffer.size(), PingTimeout) > 0 &&
						 reinterpret_cast<ICMPV6_ECHO_REPLY *>(replyBuffer.data())->Status == IP_SUCCESS;

	const auto error = GetLastError();

	IcmpCloseHandle(icmpFile);

	if( success )
	{
		*result = PingResult::ReplyReceived;
		return true;
	}

	if (error == IP_REQ_TIMED_OUT)
	{
		*result = PingResult::TimedOut;
		return true;
	}

	return false;
}



bool WindowsNetworkFunctions::pingViaUtility(const QString& hostAddress, PingResult* result)
{
	if (result == nullptr)
	{
		return false;
	}

	*result = PingResult::Unknown;

	const QStringList pingArguments = {
		QStringLiteral("-n"), QStringLiteral("1"),
		QStringLiteral("-w"), QString::number(PingTimeout),
		hostAddress
	};

	QProcess pingProcess;
	pingProcess.start(QStringLiteral("ping"), pingArguments);
	if (pingProcess.waitForStarted(PingProcessTimeout))
	{
		if (pingProcess.waitForFinished(PingProcessTimeout))
		{
			if (QString::fromUtf8(pingProcess.readAll()).split(QLatin1Char('\n')).filter(QStringLiteral("=")).size() >= 2)
			{
				*result = PingResult::ReplyReceived;
			}
			else if (pingProcess.exitCode() == 1)
			{
				*result = PingResult::NameResolutionFailed;
			}
			else
			{
				*result = PingResult::TimedOut;
			}
		}
		else
		{
			*result = PingResult::NameResolutionFailed;
		}
		return true;
	}

	return false
}
