/*
 * transcoder.cpp - transcoder thread implementation
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

#include "transcoder.h"
#include <QProcess>

Transcoder::Transcoder(QObject* parent)
	: QThread(parent)
{
}

void Transcoder::set_filenames(const QString &input, const QString &output)
{
	input_filename = input;
	output_filename = output;
}

#define BUF_SIZE 256

void Transcoder::run()
{
	QProcess* proc = new QProcess;

	proc->setReadChannel(QProcess::StandardError);
	proc->start("ffmpeg2theora", QStringList() << "--frontend"
		<< "--output" << output_filename
		<< input_filename);

	proc->waitForStarted();
	proc->waitForReadyRead();

	char buf[BUF_SIZE];
	while (proc->readLine(buf, BUF_SIZE) > 0) {
		if (QString(buf).startsWith("f2t")) {
			emit statusUpdate(QString(buf).trimmed());
		}
		proc->waitForReadyRead();
	}
}
