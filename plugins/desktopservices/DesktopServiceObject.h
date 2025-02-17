/*
 * DesktopServiceObject.h - data class representing a desktop service object
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

#include <QMetaType>
#include <QString>
#include <QUuid>

class QJsonObject;

// clazy:excludeall=rule-of-three
class DesktopServiceObject
{
public:
	using Uid = QUuid;
	using Name = QString;

	enum class Type
	{
		None,
		Application,
		Website,
	} ;

	DesktopServiceObject( const DesktopServiceObject& other );
	explicit DesktopServiceObject( Type type = Type::None,
						  const Name& name = {},
						  const QString& path = {},
						  Uid uid = Uid() );
	explicit DesktopServiceObject( const QJsonObject& jsonObject );

	bool operator==( const DesktopServiceObject& other ) const;

	const Uid& uid() const
	{
		return m_uid;
	}

	Uid parentUid() const
	{
		return Uid();
	}

	Type type() const
	{
		return m_type;
	}

	const Name& name() const
	{
		return m_name;
	}

	const QString& path() const
	{
		return m_path;
	}

	QJsonObject toJson() const;

private:
	Type m_type;
	QString m_name;
	QString m_path;
	Uid m_uid;

};

Q_DECLARE_METATYPE(DesktopServiceObject::Type)
