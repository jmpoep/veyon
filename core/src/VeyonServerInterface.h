/*
 * VeyonServerInterface.h - interface class for VeyonServer
 *
 * Copyright (c) 2018-2025 Tobias Junghans <tobydox@veyon.io>
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

#include "VeyonCore.h"

class FeatureMessage;
class FeatureWorkerManager;
class MessageContext;

// clazy:excludeall=copyable-polymorphic

class VEYON_CORE_EXPORT VeyonServerInterface : public QObject
{
	Q_OBJECT
public:
	explicit VeyonServerInterface(QObject* parent) :
		QObject(parent)
	{
	}

	virtual ~VeyonServerInterface() = default;

	virtual FeatureWorkerManager& featureWorkerManager() = 0;
	virtual bool sendFeatureMessageReply( const MessageContext& context, const FeatureMessage& reply ) = 0;

	virtual int vncServerBasePort() const = 0;

	virtual void setMinimumFramebufferUpdateInterval(const MessageContext& context, int interval) = 0;

};
