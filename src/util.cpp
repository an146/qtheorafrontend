/*
 * util.cpp - various utils
 * This file is part of QTheoraFrontend.
 *
 * Copyright (C) 2009  Anton Novikov <an146@ya.ru>
 *
 * The contents of this file can be redistributed and/or modified under the
 * terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see http://www.gnu.org/licenses/.
 *
 */

#include <QStringList>
#include "util.h"

static QString
untail(const QString &s, char c)
{
	QString ret = s.trimmed();
	if (!ret.isEmpty() && ret[ret.size() - 1] == c)
		ret.chop(1);
	return ret;
}

static QString
unquote(const QString &s)
{
	QString ret = s.trimmed();
	if (!ret.isEmpty() && ret[0] == '"')
		ret.remove(0, 1);
	return untail(ret, '"');
}

bool
parse_json_pair(QString input, QString *key, QString *value)
{
	QStringList sl = untail(input, ',').split(": ");
	if (sl.size() != 2)
		return false;
	*key = unquote(sl[0]);
	*value = unquote(sl[1]);
	return true;
}
