/*
 * WindowsNetworkFunctions.h - declaration of WindowsNetworkFunctions class
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

#pragma once

#include "PlatformNetworkFunctions.h"

#include <winbase.h>

// clazy:exclude=copyable-polymorphic

class WindowsNetworkFunctions : public PlatformNetworkFunctions
{
public:
	WindowsNetworkFunctions();

	PingResult ping(const QString& hostAddress) override;
	bool configureFirewallException( const QString& applicationPath, const QString& description, bool enabled ) override;

	bool configureSocketKeepalive( Socket socket, bool enabled, int idleTime, int interval, int probes ) override;

	static constexpr auto WindowsFirewallServiceError = HRESULT(0x800706D9);

private:
	bool pingIPv4Address(const QString& hostAddress, PingResult* result);
	bool pingIPv6Address(const QString& hostAddress, PingResult* result);
	bool pingViaUtility(const QString& hostAddress, PingResult* result);

};
