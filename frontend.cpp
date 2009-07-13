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

#include <stdexcept>
#include <cmath> // std::floor
#include <Qt>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDebug>
#include "frontend.h"

#define LENGTH(x) int(sizeof(x) / sizeof(*x))

static QString input_filter();

Frontend::Frontend(QWidget* parent)
	: QDialog(parent),
	input_dlg(this, "Select the input file", QString(), input_filter()),
	output_dlg(this, "Select the output file", QString(), "*.ogv"),
	output_auto(true),
	exitting(false),
	input_valid(false)
{
	ui.setupUi(this);
	transcoder = new Transcoder(this);
	connect(transcoder, SIGNAL(started()), this, SLOT(updateButtons()));
	connect(transcoder, SIGNAL(finished()), this, SLOT(updateButtons()));
	connect(transcoder, SIGNAL(statusUpdate(QString)), this, SLOT(updateStatus(QString)));

	input_dlg.setOption(QFileDialog::HideNameFilterDetails);
	input_dlg.setFileMode(QFileDialog::ExistingFile);
	output_dlg.setAcceptMode(QFileDialog::AcceptSave);
	output_dlg.setFileMode(QFileDialog::AnyFile);
	output_dlg.setDefaultSuffix("ogv");
	connect(&input_dlg, SIGNAL(fileSelected(QString)), this, SLOT(inputSelected(QString)));
	connect(&output_dlg, SIGNAL(fileSelected(QString)), this, SLOT(outputSelected(QString)));

	connect(ui.input_select, SIGNAL(released()), &input_dlg, SLOT(exec()));
	connect(ui.output_select, SIGNAL(released()), &output_dlg, SLOT(exec()));
	connect(ui.input, SIGNAL(textChanged(QString)), this, SLOT(retrieveInfo()));
	connect(ui.output, SIGNAL(textChanged(QString)), this, SLOT(updateButtons()));
	connect(ui.output, SIGNAL(textEdited(QString)), this, SLOT(outputEdited()));
	connect(ui.transcode, SIGNAL(released()), this, SLOT(transcode()));
	connect(ui.cancel, SIGNAL(released()), this, SLOT(cancel()));
	connect(ui.partial, SIGNAL(stateChanged(int)), this, SLOT(partialStateChanged()));
	connect(ui.partial_start, SIGNAL(valueChanged(double)), ui.partial_end, SLOT(setMinimum(double)));
	connect(ui.partial_end, SIGNAL(valueChanged(double)), ui.partial_start, SLOT(setMaximum(double)));
	connect(ui.no_skeleton, SIGNAL(toggled(bool)), this, SLOT(noSkeleton(bool)));

	connect(ui.info_audio_stream, SIGNAL(currentIndexChanged(int)), this, SLOT(updateInfo()));
	connect(ui.info_video_stream, SIGNAL(currentIndexChanged(int)), this, SLOT(updateInfo()));

	connect(ui.audio_encode, SIGNAL(toggled(bool)), this, SLOT(updateAudio()));
	connect(ui.audio_const_quality, SIGNAL(toggled(bool)), ui.audio_quality, SLOT(setEnabled(bool)));
	connect(ui.audio_const_bitrate, SIGNAL(toggled(bool)), ui.audio_bitrate, SLOT(setEnabled(bool)));
	retrieveInfo();
}

int
Frontend::cancel_ask(const QString &reason, bool cancel_button)
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

void
Frontend::closeEvent(QCloseEvent *event)
{
	if (!transcoder->isRunning())
		event->accept();
	else {
		if (cancel())
			exitting = true;
		event->ignore();
	}
}

void
Frontend::transcode()
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

	if (ui.audio_encode->isChecked()) {
		ea = ea << "--audiostream" << ui.audio_stream->currentText();
		ea = ea << "--channels" << ui.audio_channels->currentText();
		ea = ea << "--samplerate" << ui.audio_samplerate->currentText();
		if (ui.audio_const_quality->isChecked())
			ea = ea << "--audioquality" << QString::number(ui.audio_quality->value());
		else if (ui.audio_const_bitrate->isChecked())
			ea = ea << "--audiobitrate" << ui.audio_bitrate->currentText();
	} else {
		ea = ea << "--noaudio";
	}
	transcoder->start(ui.input->text(), ui.output->text(), ea);
}

bool
Frontend::cancel()
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

void
Frontend::updateStatus(QString statusText)
{
	ui.status->setText(statusText);
}

void
Frontend::inputSelected(const QString &s)
{
	ui.input->setText(s);
}

void
Frontend::outputSelected(const QString &s)
{
	ui.output->setText(s);
	output_auto = false;
}

void
Frontend::updateButtons()
{
	bool running = transcoder->isRunning();
	bool missing_data = ui.input->text().isEmpty() || ui.output->text().isEmpty();
	bool can_start = !running && !missing_data && input_valid;
	ui.transcode->setEnabled(can_start);
	ui.transcode->setDefault(can_start);
	ui.cancel->setEnabled(running);
	ui.partial->setEnabled(input_valid && finfo.duration > 0);
	partialStateChanged();
	if (exitting)
		close();
}

void
Frontend::outputEdited()
{
	output_auto = ui.output->text().isEmpty();
	if (ui.output->text().endsWith(".ogg"))
		ui.no_skeleton->setChecked(true);
}

/* returns a QDir that != dir */

static QDir
another_dir(const QDir &dir)
{
#define TRY(d) if (d != dir) return d
	TRY(QDir::root());
	TRY(QDir::home());
	TRY(QDir::temp());
	
	// well, shit happens
	QDir d = dir;
	d.cdUp();
	return dir;
}

/* hack: in some versions of Qt (4.5.1 at least)
 * when an existing file is selected in QFileDialog
 * and you select a non-existing file, selection
 * stays on the previous one, though filename in
 * text box is set correctly. So we need to reset
 * the selection before we select another file
 */

static void
clear_filedialog_selection(QFileDialog *dlg)
{
	QDir dir = dlg->directory();
	dlg->setDirectory(another_dir(dir));
	dlg->setDirectory(dir);
}

void
Frontend::setDefaultOutput()
{
	if (!output_auto)
		return;

	QString s = ui.input->text();
	if (s.isEmpty())
		return;

	QFileInfo f(s);
	QString ext = finfo.video_streams.empty() ?
		(ui.no_skeleton->isChecked() ? "ogg" : "oga") :
		"ogv";
	QString name = f.completeBaseName() + "." + ext;
	bool has_path = f.fileName() != f.filePath();
	QString out = has_path ? f.dir().filePath(name) : name;
	ui.output->setText(out);
	output_dlg.setDirectory(f.dir());
	clear_filedialog_selection(&output_dlg);
	output_dlg.selectFile(name);
}

#define FIELDS \
	FILE_FIELD(duration, time) \
	FILE_FIELD(bitrate, bitrate) \
	FILE_FIELD(size, file_size) \
\
	VSTREAM_FIELD(codec, as_is) \
	VSTREAM_FIELD(bitrate, bitrate) \
	VSTREAM_FIELD(pixel_format, as_is) \
	VSTREAM_FIELD(height, int) \
	VSTREAM_FIELD(width, int) \
	VSTREAM_FIELD(framerate, as_is) \
	VSTREAM_FIELD(pixel_aspect_ratio, as_is) \
	VSTREAM_FIELD(display_aspect_ratio, as_is) \
\
	ASTREAM_FIELD(codec, as_is) \
	ASTREAM_FIELD(bitrate, bitrate) \
	ASTREAM_FIELD(samplerate, int) \
	ASTREAM_FIELD(channels, int)

#define BUF_SIZE 256

static QString
present_as_is(const QString &s)
{
	return s;
}

static QString
present_time(double t)
{
	return Frontend::time2string(t);
}

static QString
present_file_size(long long bytes)
{
	if (bytes < 0)
		return "";
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

static QString
present_bitrate(double bitrate)
{
	if (bitrate < 0)
		return "";

	char buf[BUF_SIZE];
	snprintf(buf, BUF_SIZE, "%.2f kbit/s", bitrate);
	return buf;
}

static QString
present_int(int n)
{
	return QString::number(n);
}

template <class Info>
static void
get_streams(QComboBox *cb, const QList<Info> &list)
{
	cb->clear();
	cb->setEnabled(!list.empty());
	for (typename QList<Info>::const_iterator i = list.begin(); i != list.end(); ++i)
		cb->addItem(QString::number(i->id));
	cb->setCurrentIndex(0);
}

template <class Info>
static const Info *
stream(const QComboBox *cb, const QList<Info> &list)
{
	int i = cb->currentIndex();
	if (i < 0 || i >= list.size())
		return NULL;
	return &list[i];
}

void
Frontend::retrieveInfo()
{
	input_valid = false;
	finfo = FileInfo();
	ui.partial->setCheckState(Qt::Unchecked);
	QString input = ui.input->text();

	try {
		QFileInfo fi(input);
		if (input.isEmpty())
			throw std::runtime_error("");
		else if (!fi.exists())
			throw std::runtime_error("File does not exist");
		else if(!fi.isFile())
			throw std::runtime_error("Not a file");
		finfo.retrieve(input);

		ui.partial_start->setMinimum(0);
		ui.partial_start->setMaximum(finfo.duration);
		ui.partial_start->setValue(0);
		ui.partial_end->setMinimum(0);
		ui.partial_end->setMaximum(finfo.duration);
		ui.partial_end->setValue(finfo.duration);

		input_valid = true;
		updateStatus("");
	} catch (std::exception &x) {
		updateStatus(x.what());
	}
	ui.audio_encode->setEnabled(!finfo.audio_streams.empty());
	ui.audio_encode->setChecked(!finfo.audio_streams.empty());

	get_streams(ui.info_audio_stream, finfo.audio_streams);
	get_streams(ui.info_video_stream, finfo.video_streams);
	get_streams(ui.audio_stream, finfo.audio_streams);

	setDefaultOutput();
	updateButtons();
	updateInfo();
	updateAudio();
}

void
Frontend::updateInfo()
{
#define FIELD(f, uif, t, o) ui. uif ->setText((o != NULL && input_valid) ? present_##t(o->f) : QString(""));
#define FILE_FIELD(f, c) FIELD(f, info_##f, c, (&finfo))
#define ASTREAM_FIELD(f, c) FIELD(f, info_audio_##f, c, stream(ui.info_audio_stream, finfo.audio_streams))
#define VSTREAM_FIELD(f, c) FIELD(f, info_video_##f, c, stream(ui.info_video_stream, finfo.video_streams))
		FIELDS
#undef FIELD
}

static const int samplerates[] = {
	22050,
	24000,
	32000,
	44100,
	48000
};

static const int bitrates[] = {
	32,
	40,
	48,
	56,
	64,
	80,
	96,
	112,
	128,
	160
};

#define select_last(c) c->setCurrentIndex(c->count() - 1)

void
Frontend::updateAudio()
{
	int c = ui.audio_options_layout->count();
	int encode = ui.audio_encode->isChecked();
	int i;

	for (i = 0; i < c; i++) {
		QWidget *w = ui.audio_options_layout->itemAt(i)->widget();
		if (w != 0)
			w->setEnabled(encode);
	}
	ui.audio_channels->clear();
	ui.audio_samplerate->clear();
	ui.audio_bitrate->clear();
	const AudioStreamInfo *s = stream(ui.audio_stream, finfo.audio_streams);
	if (s != NULL) {
		for (i = 0; i < s->channels; i++)
			ui.audio_channels->addItem(QString::number(i + 1));
		for (i = 0; i < LENGTH(samplerates); i++)
			if (samplerates[i] <= s->samplerate)
				ui.audio_samplerate->addItem(QString::number(samplerates[i]));
		for (i = 0; i < LENGTH(bitrates); i++)
			if (bitrates[i] <= s->bitrate)
				ui.audio_bitrate->addItem(QString::number(bitrates[i]));
		select_last(ui.audio_channels);
		select_last(ui.audio_samplerate);
		select_last(ui.audio_bitrate);
	}
}

void
Frontend::partialStateChanged()
{
	ui.partial_start->setEnabled(ui.partial->checkState());
	ui.partial_end->setEnabled(ui.partial->checkState());
}

QString
Frontend::time2string(double t, int decimals)
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

static const char *extensions[] = {
	"302",
	"16sv",
	"3g2",
	"3gp",
	"4xm",
	"8svx",
	"aac",
	"ac3",
	"aif",
	"aiff",
	"amv",
	"anim",
	"apc",
	"ape",
	"asf",
	"avs",
	"bf",
	"bfi",
	"c93",
	"cak",
	"cam",
	"cdata",
	"cin",
	"cmv",
	"cpk",
	"dct",
	"divx",
	"drc",
	"dts",
	"dv",
	"dxa",
	"ffm",
	"flc",
	"fli",
	"flv",
	"flx",
	"gxf",
	"hnm",
	"ilbm",
	"iss",
	"k3g",
	"m1a",
	"m1v",
	"m2a",
	"m4a",
	"m4b",
	"m4p",
	"m4r",
	"m4v",
	"mad",
	"mka",
	"mks",
	"mkv",
	"mm",
	"mov",
	"mp+",
	"mp1",
	"mp2",
	"mp3",
	"mp4",
	"mp4",
	"mpa",
	"mpc",
	"mpeg",
	"mpg",
	"mpp",
	"mpv",
	"mtv",
	"mve",
	"mvi1",
	"mvi2",
	"mxf",
	"nsv",
	"nut",
	"nuv",
	"oga",
	"ogg",
	"ogv",
	"ps",
	"pva",
	"qcp",
	"qt",
	"rgi",
	"roq",
	"sfd",
	"skm",
	"snd",
	"son",
	"swf",
	"tgq",
	"tgv",
	"tmv",
	"ts",
	"uv",
	"uv2",
	"vb",
	"vid",
	"vob",
	"voc",
	"vp6",
	"vqe",
	"vqf",
	"vql",
	"wav",
	"wma",
	"wmv",
	"wve",

	"261",
	"aa3",
	"asf",
	"au",
	"aud",
	"film",
	"flac",
	"gsm",
	"h263",
	"h264",
	"mlp",
	"ogm",
	"oma",
	"p64",
	"pcm",
	"r3d",
	"ra",
	"rl2",
	"rm",
	"rmvb",
	"rpl",
	"seq",
	"shn",
	"smk",
	"sol",
	"str",
	"thp",
	"tta",
	"txd",
	"vmd",
	"vqa",
	"wmv3",
	"wmv9",
	"wmva",
	"wmvp",
	"wmvr",
	"wv",
	"wvc1",
	"wvp2",

	"avi",
};

static QString
input_filter()
{
	QString ext_list;
	for (int i = 0; i < LENGTH(extensions); i++) {
		ext_list += " *.";
		ext_list += extensions[i];
	}
	return QString("Video and Audio files (") + ext_list + ");;Any files (*)";
}

void
Frontend::noSkeleton(bool checked)
{
	if (checked && finfo.video_streams.empty() /* !ui.video_encode->isChecked() */) {
		QString out = ui.output->text();
		if (out.endsWith(".oga")) {
			out.chop(4);
			ui.output->setText(out + ".ogg");
		}
	}
}
