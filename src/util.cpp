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

#define BUF_SIZE 256

QString
time2string(double t, int decimals, bool colons)
{
	if (t < 0)
		return "";
	double precision = 1.0;
	for (int i = 0; i < decimals; i++)
		precision /= 10;
	t += precision / 2;

	long long seconds = (long long)t;
	long long minutes = seconds / 60;
	long long hours = minutes / 60;

	char buf[BUF_SIZE];
	if (colons)
		snprintf(buf, BUF_SIZE, "%lli:%.02lli:%.02lli", hours, minutes % 60, seconds % 60);
	else if (hours > 0)
		snprintf(buf, BUF_SIZE, "%llih %llim %llis", hours, minutes % 60, seconds % 60);
	else if (minutes > 0)
		snprintf(buf, BUF_SIZE, "%llim %llis", minutes % 60, seconds % 60);
	else if (seconds > 0)
		snprintf(buf, BUF_SIZE, "%llis", seconds % 60);
	else
		snprintf(buf, BUF_SIZE, "0s");

	double s = t - seconds;
	for (int i = 0; i < decimals; i++)
		s *= 10;
	int x = (int)s;
	while (x % 10 == 0 && decimals > 0) {
		x /= 10;
		decimals--;
	}
	if (decimals > 0) {
		char fmt[BUF_SIZE], buf1[BUF_SIZE];
		sprintf(fmt, ".%%.0%ii", decimals);
		sprintf(buf1, fmt, x);
		strcat(buf, buf1);
	}
	return buf;
}

