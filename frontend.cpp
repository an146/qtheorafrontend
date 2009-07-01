/*
 * frontend.cpp - frontend widget for invoking ffmpeg2theora
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

#include <cmath> // std::floor
#include <Qt>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDebug>
#include "frontend.h"

static const char *input_filter = "Video files (*.avi *.mpg *.flv);;Any files (*.*)";

Frontend::Frontend(QWidget* parent)
	: QDialog(parent),
	input_dlg(this, "Select the input file", QString(), input_filter),
	output_dlg(this, "Select the output file", QString(), "*.ogv"),
	output_auto(true),
	exitting(false),
	input_valid(false)
{
	ui.setupUi(this);
	transcoder = new Transcoder(this);

	connect(ui.input_select, SIGNAL(released()), &input_dlg, SLOT(exec()));
	connect(ui.output_select, SIGNAL(released()), &output_dlg, SLOT(exec()));
	connect(&input_dlg, SIGNAL(fileSelected(QString)), this, SLOT(inputSelected(QString)));
	connect(&output_dlg, SIGNAL(fileSelected(QString)), this, SLOT(outputSelected(QString)));
	input_dlg.setFileMode(QFileDialog::ExistingFile);
	output_dlg.setAcceptMode(QFileDialog::AcceptSave);
	output_dlg.setFileMode(QFileDialog::AnyFile);
	output_dlg.setDefaultSuffix("ogv");

	connect(ui.input, SIGNAL(textChanged(QString)), this, SLOT(setDefaultOutput()));
	connect(ui.input, SIGNAL(textChanged(QString)), this, SLOT(updateInfo()));
	connect(ui.input, SIGNAL(textChanged(QString)), this, SLOT(updateButtons()));
	connect(ui.output, SIGNAL(textChanged(QString)), this, SLOT(updateButtons()));
	connect(ui.output, SIGNAL(textEdited(QString)), this, SLOT(outputEdited()));
	connect(ui.transcode, SIGNAL(released()), this, SLOT(transcode()));
	connect(ui.cancel, SIGNAL(released()), this, SLOT(cancel()));
	connect(transcoder, SIGNAL(started()), this, SLOT(updateButtons()));
	connect(transcoder, SIGNAL(finished()), this, SLOT(updateButtons()));
	connect(transcoder, SIGNAL(statusUpdate(QString)),
		this, SLOT(updateStatus(QString)));
	connect(ui.partial, SIGNAL(stateChanged(int)), this, SLOT(partialStateChanged()));
	connect(ui.partial_start, SIGNAL(valueChanged(double)), ui.partial_end, SLOT(setMinimum(double)));
	connect(ui.partial_end, SIGNAL(valueChanged(double)), ui.partial_start, SLOT(setMaximum(double)));
	updateButtons();
}

int Frontend::cancel_ask(const QString &reason, bool cancel_button)
{
	QMessageBox::StandardButtons buttons = QMessageBox::Save | QMessageBox::Discard;
	if (cancel_button)
		buttons |= QMessageBox::Cancel;
	return QMessageBox::question(
		this,
		"qtheorafrontend",
		reason + "Do you want to keep the partially encoded file?",
		buttons,
		QMessageBox::Discard
	);
}

void Frontend::closeEvent(QCloseEvent *event)
{
	if (!transcoder->isRunning())
		event->accept();
	else {
		if (cancel())
			exitting = true;
		event->ignore();
	}
}

void Frontend::transcode()
{
	QStringList ea;
	if (ui.partial->isChecked()) {
		ea = ea << "--starttime" << QString::number(ui.partial_start->value());
		ea = ea << "--endtime" << QString::number(ui.partial_end->value());
	}
	if (ui.sync->isChecked())
		ea = ea << "--sync";
	if (ui.no_skeleton->isChecked())
		ea = ea << "--no-skeleton";
	transcoder->start(ui.input->text(), ui.output->text(), ea);
}

bool Frontend::cancel()
{
	bool keep = false;

	switch (cancel_ask("You are going to cancel the encoding. ", true)) {
	case QMessageBox::Save:
		keep = true;
	case QMessageBox::Discard:
		transcoder->stop(keep);
		return true;
	}
	return false;
}

void Frontend::updateStatus(QString statusText)
{
	ui.status->setText(statusText);
}

void Frontend::inputSelected(const QString &s)
{
	ui.input->setText(s);
}

void Frontend::outputSelected(const QString &s)
{
	ui.output->setText(s);
}

void Frontend::updateButtons()
{
	bool running = transcoder->isRunning();
	bool missing_data = ui.input->text().isEmpty() || ui.output->text().isEmpty();
	bool can_start = !running && !missing_data && input_valid;
	ui.transcode->setEnabled(can_start);
	ui.transcode->setDefault(can_start);
	ui.cancel->setEnabled(running);
	ui.partial->setEnabled(input_valid && duration > 0);
	partialStateChanged();
	if (exitting)
		close();
}

void Frontend::outputEdited()
{
	output_auto = ui.output->text().isEmpty();
}

void Frontend::setDefaultOutput()
{
	if (!output_auto)
		return;

	QString s = ui.input->text();
	if (s.isEmpty())
		return;

	QFileInfo f(s);
	QString name = f.completeBaseName() + ".ogv";
	ui.output->setText(f.dir().filePath(name));
}

#define FIELDS \
	FIELD(duration, time) \
	FIELD(bitrate, bitrate) \
	FIELD(size, file_size) \
\
	FIELD(video_codec, as_is) \
	FIELD(video_bitrate, bitrate) \
	FIELD(pixel_format, as_is) \
	FIELD(height, as_is) \
	FIELD(width, as_is) \
	FIELD(framerate, as_is) \
	FIELD(pixel_aspect_ratio, as_is) \
	FIELD(display_aspect_ratio, as_is) \
\
	FIELD(audio_codec, as_is) \
	FIELD(audio_bitrate, bitrate) \
	FIELD(samplerate, as_is) \
	FIELD(channels, as_is)

void Frontend::clearInfo()
{
#define FIELD(f, p) ui. info_##f ->setText("");
	FIELDS
#undef FIELD
}

#define BUF_SIZE 256

static QString untail(const QString &s, char c)
{
	QString ret = s.trimmed();
	if (!ret.isEmpty() && ret[ret.size() - 1] == c)
		ret.chop(1);
	return ret;
}

static QString unquote(const QString &s)
{
	QString ret = s.trimmed();
	if (!ret.isEmpty() && ret[0] == '"')
		ret.remove(0, 1);
	return untail(ret, '"');
}

static QString present_as_is(const QString &s)
{
	return s;
}

static QString present_time(const QString &s)
{
	bool ok;
	double dseconds = s.toDouble(&ok);
	if (!ok)
		return "";
	else
		return Frontend::time2string(dseconds);
}

static QString present_file_size(const QString &s)
{
	bool ok;
	long long bytes = s.toLongLong(&ok);
	if (!ok || bytes < 0)
		return s;
	long long kb = bytes / 1024;
	long long mb = kb / 1024;
	double gb = mb / 1024.0;
	char buf[BUF_SIZE];
	if (gb >= 1)
		snprintf(buf, BUF_SIZE, "%.1f GB", gb);
	else if (mb >= 1)
		snprintf(buf, BUF_SIZE, "%lli MB", mb);
	else if (kb >= 1)
		snprintf(buf, BUF_SIZE, "%lli KB", kb);
	else
		snprintf(buf, BUF_SIZE, "%lli bytes", bytes);
	return buf;
}

static QString present_bitrate(const QString &s)
{
	bool ok;
	double bitrate = s.toDouble(&ok);
	if (!ok || bitrate < 0)
		return s;

	char buf[BUF_SIZE];
	snprintf(buf, BUF_SIZE, "%.2f kbit/s", bitrate);
	return buf;
}

void Frontend::updateInfo()
{
	clearInfo();
	duration = -1;
	ui.partial->setCheckState(Qt::Unchecked);
	QString input = ui.input->text();
	if (input.isEmpty())
		return;

	QProcess proc;
	proc.start(transcoder->ffmpeg2theora(), QStringList() << "--info" << input);
	if (proc.waitForStarted()) {
		char buf[BUF_SIZE] = "";
		proc.waitForReadyRead();
		while (proc.readLine(buf, BUF_SIZE) > 0) {
			QStringList sl = untail(buf, ',').split(": ");
			if (sl.size() < 2)
				continue;
			QString key = unquote(sl[0]);
			QString value = unquote(sl[1]);
			if (key == "duration") {
				bool ok;
				duration = value.toDouble(&ok);
				if (ok && duration >= 0) {
					ui.partial_start->setMinimum(0);
					ui.partial_start->setMaximum(duration);
					ui.partial_start->setValue(0);
					ui.partial_end->setMinimum(0);
					ui.partial_end->setMaximum(duration);
					ui.partial_end->setValue(duration);
				} else
					duration = -1;
			}
#define FIELD(f, p) \
			if (key == #f) \
				ui. info_##f ->setText(present_##p(value));
			FIELDS
#undef FIELD
			proc.waitForReadyRead();
		}
		proc.waitForFinished();
		input_valid = proc.exitCode() == 0 && proc.exitStatus() == QProcess::NormalExit;
	} else {
		updateStatus("Info retrieval failed to start");
		input_valid = false;
	}
}

void Frontend::partialStateChanged()
{
	ui.partial_start->setEnabled(ui.partial->checkState());
	ui.partial_end->setEnabled(ui.partial->checkState());
}

QString Frontend::time2string(double t, int decimals)
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
	snprintf(buf, BUF_SIZE, "%lli:%.02lli:%.02lli", hours, minutes % 60, seconds % 60);

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

