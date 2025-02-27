/*
 * ComputerControlClient.h - header file for the ComputerControlClient class
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

#include <QElapsedTimer>

#include "VncClientProtocol.h"
#include "VncProxyConnection.h"
#include "VncServerClient.h"
#include "VeyonServerProtocol.h"

class ComputerControlServer;

class ComputerControlClient : public VncProxyConnection
{
	Q_OBJECT
public:
	using Password = CryptoCore::PlaintextPassword;

	ComputerControlClient( ComputerControlServer* server,
						   QTcpSocket* clientSocket,
						   int vncServerPort,
						   const Password& vncServerPassword,
						   QObject* parent );
	~ComputerControlClient() override;

	bool receiveClientMessage() override;

	VncServerClient* serverClient()
	{
		return &m_serverClient;
	}

	void setMinimumFramebufferUpdateInterval(int interval);

protected:
	VncClientProtocol& clientProtocol() override
	{
		return m_clientProtocol;
	}

	VncServerProtocol& serverProtocol() override
	{
		return m_serverProtocol;
	}

private:
	ComputerControlServer* m_server;

	VncServerClient m_serverClient{};

	VeyonServerProtocol m_serverProtocol;
	VncClientProtocol m_clientProtocol;

	int m_minimumFramebufferUpdateInterval{-1};
	QElapsedTimer m_framebufferUpdateTimer;

} ;
