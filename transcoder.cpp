/*
 * transcoder.cpp - transcoder thread implementation
 * This file is part of QTheoraFrontend.
 *
 * Copyright (C) 2009  Anton Novikov <an146@ya.ru>
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

#include <QProcess>
#include <QMetaType>
#include <QFile>
#include <QMessageBox>
#include <QTimer>
#include "transcoder.h"
#include "frontend.h"

Transcoder::Transcoder(Frontend *f)
	: QThread(NULL), frontend(f)
{
	qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
	proc.moveToThread(this);
	connect(&proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(procFinished(int, QProcess::ExitStatus)));
	connect(&proc, SIGNAL(readyReadStandardOutput()), this, SLOT(readyRead()));
	connect(&proc, SIGNAL(readyReadStandardError()), this, SLOT(readyRead()));
	connect(this, SIGNAL(terminate()), &proc, SLOT(kill()));

	ffmpeg2theora_ = "ffmpeg2theora";
	QString suffix = QFileInfo(QCoreApplication::applicationFilePath()).suffix();
	if (!suffix.isEmpty())
		ffmpeg2theora_ += "." + suffix;
	QString bundled = QDir(QCoreApplication::applicationDirPath()).filePath(ffmpeg2theora_);
	if (QFile(bundled).exists())
		ffmpeg2theora_ = bundled;
}

void Transcoder::start(const QString &input, const QString &output, const QStringList &ea)
{
	if (!isRunning()) {
		input_filename = input;
		output_filename = output;
		extra_args = ea;
		QThread::start();
	}
}

void Transcoder::stop(bool keep)
{
	if (isRunning()) {
		keep_output = keep;
		finish_message = keep ? QString("Encoding cancelled. Partial result kept") :
		                        QString("Encoding cancelled. Partial result deleted");
		emit terminate();
	}
}

#define BUF_SIZE 256

void Transcoder::readyRead()
{
	QProcess::ProcessChannel channel[2] = {QProcess::StandardOutput, QProcess::StandardError};
	char buf[BUF_SIZE] = "";
	for (int i = 0; i < 2; i++) {
		proc.setReadChannel(channel[i]);
		while (proc.canReadLine())
			if (proc.readLine(buf, BUF_SIZE) > 0)
				emit statusUpdate(QString(buf).trimmed());
	}
}

void Transcoder::procFinished(int status, QProcess::ExitStatus qstatus)
{
	if (finish_message.isEmpty()) {
		/* the process died without our help */
		bool ok = status == 0 && qstatus == 0;
		if (ok) {
			keep_output = true;
			finish_message = "Encoding finished successfully";
			QMessageBox::information(frontend, "qtheorafeontend", finish_message);
		}
		else {
			keep_output = frontend->cancel_ask("Encoding failed. ", false) == QMessageBox::Save;
			finish_message = keep_output ?
				QString("Encoding failed. Partial file kept") :
				QString("Encoding failed. Partial file deleted");
		}
	}
	if (!keep_output)
		QFile(output_filename).remove();
	emit statusUpdate(finish_message);
	quit();
}

void Transcoder::run()
{
	finish_message = "";
	keep_output = true;
	proc.start(ffmpeg2theora(), QStringList() << "--frontend"
		<< extra_args
		<< "--output" << output_filename
		<< input_filename);

	if (proc.waitForStarted())
		exec();
	else
		emit statusUpdate("Encoding failed to start");
}
