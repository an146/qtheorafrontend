/*
 * transcoder.h - transcoder thread declarations
 * This file is part of QTheoraFrontend.
 *
 * Copyright (C) 2009  Denver Gingerich <denver@ossguy.com>
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

#ifndef H_TRANSCODER
#define H_TRANSCODER

#include <QThread>

class Transcoder : public QThread
{
	Q_OBJECT

public:
	explicit Transcoder(QObject* parent = 0);
	void set_filenames(const QString &input, const QString &output);

signals:
	void statusUpdate(QString status);

protected:
	void run();

private:
	QString input_filename;
	QString output_filename;
};

#endif // H_TRANSCODER
