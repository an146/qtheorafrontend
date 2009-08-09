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
#include "transcoder.h"
#include "frontend.h"
#include "util.h"

Transcoder::Transcoder(Frontend *f)
	: QThread(NULL), frontend_(f)
{
	qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
	proc_.moveToThread(this);
	connect(&proc_, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(procFinished(int, QProcess::ExitStatus)));
	connect(&proc_, SIGNAL(readyReadStandardOutput()), this, SLOT(readyRead()));
	connect(&proc_, SIGNAL(readyReadStandardError()), this, SLOT(readyRead()));
	connect(this, SIGNAL(terminate()), &proc_, SLOT(kill()));
}

void
Transcoder::start(const QString &input, const QString &output, const QStringList &ea)
{
	if (!isRunning()) {
		input_filename_ = input;
		output_filename_ = output;
		start_time_ = QDateTime::currentDateTime();
		extra_args_ = ea;
		QThread::start();
	}
}

void
Transcoder::stop()
{
	stopping_ = true;
	if (isRunning())
		emit terminate();
}

QString
Transcoder::ffmpeg2theora()
{
	static QString ffmpeg2theora_;
	if (ffmpeg2theora_.isEmpty()) {
		ffmpeg2theora_ = "ffmpeg2theora";
		QString suffix = QFileInfo(QCoreApplication::applicationFilePath()).suffix();
		if (!suffix.isEmpty())
			ffmpeg2theora_ += "." + suffix;
		QString bundled = QDir(QCoreApplication::applicationDirPath()).filePath(ffmpeg2theora_);
		if (QFile(bundled).exists())
			ffmpeg2theora_ = bundled;
	}
	return ffmpeg2theora_;
}

double
Transcoder::elapsed() const
{
	return start_time_.secsTo(QDateTime::currentDateTime());
}

#define BUF_SIZE 256

void
Transcoder::readyRead()
{
	QProcess::ProcessChannel channel[2] = {QProcess::StandardOutput, QProcess::StandardError};
	char buf[BUF_SIZE] = "";
	for (int i = 0; i < 2; i++) {
		proc_.setReadChannel(channel[i]);
		while (proc_.canReadLine())
			if (proc_.readLine(buf, BUF_SIZE) > 0)
				processLine(QString(buf).trimmed());
	}
}

void
Transcoder::procFinished(int status, QProcess::ExitStatus qstatus)
{
	if (stopping_)
		emit finished(STOPPED);
	else {
		bool ok = status == 0 && qstatus == 0;
		emit finished(ok ? OK : FAILED);
	}
	quit();
}

void
Transcoder::run()
{
	position_ = eta_ = audio_b_ = video_b_ = -1;
	stopping_ = false;
	pass_ = extra_args_.contains("--two-pass") ? 0 : -1;
	
	proc_.start(ffmpeg2theora(), QStringList() << "--frontend"
		<< extra_args_
		<< "--output" << output_filename()
		<< input_filename());

	if (proc_.waitForStarted())
		exec();
	else
		emit statusUpdate("Encoding failed to start");
}

void
Transcoder::processLine(const QString &line)
{
	if (line.startsWith("{")) {
		QStringList sl = line.split(QRegExp("(\\{|,|\\})"), QString::SkipEmptyParts);
		for (QStringList::iterator i = sl.begin(); i != sl.end(); ++i) {
			QString key, value;
			if (!parse_json_pair(*i, &key, &value))
				continue;

			if (key == "remaining")
				eta_ = value.toDouble();
			else if (key == "audio_kbps")
				audio_b_ = value.toDouble();
			else if (key == "video_kbps")
				video_b_ = value.toDouble();
			else if (key == "position")
				position_ = value.toDouble();

			if (pass_ == 0 && (audio_b_ > 0 || video_b_ > 0))
				pass_ = 1;
		}
		emit statusUpdate(position_, eta_, audio_b_, video_b_, pass_);
	} else
		emit statusUpdate(line);
}
