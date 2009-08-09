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
#include <QMessageBox>
#include <QCloseEvent>
#include <QPlastiqueStyle>
#include "frontend.h"

#define LENGTH(x) int(sizeof(x) / sizeof(*x))

#define MIN_BITRATE 1
#define MAX_BITRATE 16778
#define MIN_KEYINT 1
#define MAX_KEYINT 2147483647
#define MIN_BDELAY 1
#define MAX_BDELAY 2147483647
#define ADJUST_SCALE 10.0

/* a year will do :) */
#define MAX_TIME (365 * 24 * 3600)

#define DEPEND(wdep, w) ui.wdep->setEnabled(ui.w->isEnabled() && ui.w->isChecked())
#define DEPEND_CONNECT(wdep, w) connect(ui.w, SIGNAL(toggled(bool)), ui.wdep, SLOT(setEnabled(bool)));

static QString input_filter();

static void
activate_layout(QWidget *widget)
{
	QWidget *parent = qobject_cast<QWidget *>(widget->parent());
	if (parent == NULL)
		return;
	activate_layout(parent);
	if (parent->layout() != NULL)
		parent->layout()->activate();
}

static void
set_label_min_size(QLabel *label, QString text)
{
	QString prev_text = label->text();
	label->setText(text);
	activate_layout(label);
	label->setMinimumSize(label->size());
	label->setText(prev_text);
}

Frontend::Frontend(QWidget* parent)
	: QDialog(parent),
	input_dlg(this, "Select the input file", QString(), input_filter()),
	output_dlg(this, "Select the output file", QString(), "*.*"),
	exitting(false),
	input_valid(false)
{
	ui.setupUi(this);
	ui.progress->setStyle(new QPlastiqueStyle());
	transcoder = new Transcoder(this);
	connect(transcoder, SIGNAL(started()), this, SLOT(updateButtons()));
	connect(transcoder, SIGNAL(finished()), this, SLOT(updateButtons()));
	connect(transcoder, SIGNAL(finished(int)), this, SLOT(finished(int)));
	connect(transcoder, SIGNAL(statusUpdate(QString)), this, SLOT(updateStatus(QString)));
	connect(transcoder, SIGNAL(statusUpdate(double, double, double, double, int)),
			this, SLOT(updateStatus(double, double, double, double, int)));

	input_dlg.setOption(QFileDialog::HideNameFilterDetails);
	input_dlg.setFileMode(QFileDialog::ExistingFile);
	output_dlg.setOption(QFileDialog::DontConfirmOverwrite);
	output_dlg.setAcceptMode(QFileDialog::AcceptSave);
	output_dlg.setFileMode(QFileDialog::AnyFile);
	connect(&input_dlg, SIGNAL(fileSelected(QString)), this, SLOT(inputSelected(QString)));
	connect(&output_dlg, SIGNAL(fileSelected(QString)), this, SLOT(outputSelected(QString)));

	connect(ui.advanced_mode, SIGNAL(toggled(bool)), this, SLOT(updateAdvancedMode()));
	connect(ui.input_select, SIGNAL(released()), &input_dlg, SLOT(exec()));
	connect(ui.output_select, SIGNAL(released()), this, SLOT(selectOutput()));
	connect(ui.input, SIGNAL(textChanged(QString)), this, SLOT(retrieveInfo()));
	connect(ui.output, SIGNAL(textChanged(QString)), this, SLOT(outputChanged()));
	connect(ui.transcode, SIGNAL(released()), this, SLOT(transcode()));
	connect(ui.cancel, SIGNAL(released()), this, SLOT(cancel()));
	connect(ui.partial_start, SIGNAL(valueChanged(double)), ui.partial_end, SLOT(setMinimum(double)));
	connect(ui.partial_end, SIGNAL(valueChanged(double)), ui.partial_start, SLOT(setMaximum(double)));
	connect(ui.no_skeleton, SIGNAL(toggled(bool)), this, SLOT(fixExtension()));
	DEPEND_CONNECT(partial_start, partial);
	DEPEND_CONNECT(partial_end, partial);

	/* Info */
	connect(ui.info_audio_stream, SIGNAL(currentIndexChanged(int)), this, SLOT(updateInfo()));
	connect(ui.info_video_stream, SIGNAL(currentIndexChanged(int)), this, SLOT(updateInfo()));

	/* Audio */
	connect(ui.audio_encode, SIGNAL(toggled(bool)), this, SLOT(updateAudio()));
	connect(ui.audio_encode, SIGNAL(toggled(bool)), this, SLOT(checkForSomethingToEncode()));
	connect(ui.audio_encode, SIGNAL(toggled(bool)), this, SLOT(updateButtons()));
	connect(ui.audio_quality, SIGNAL(valueChanged(int)), ui.audio_quality_label, SLOT(setNum(int)));
	DEPEND_CONNECT(audio_quality, audio_const_quality);
	DEPEND_CONNECT(audio_bitrate, audio_const_bitrate);
	set_label_min_size(ui.audio_quality_label, "10");

	/* Video */
	connect(ui.video_encode, SIGNAL(toggled(bool)), this, SLOT(updateVideo()));
	connect(ui.video_encode, SIGNAL(toggled(bool)), this, SLOT(checkForSomethingToEncode()));
	connect(ui.video_encode, SIGNAL(toggled(bool)), this, SLOT(updateButtons()));
	connect(ui.video_encode, SIGNAL(toggled(bool)), this, SLOT(fixExtension()));
	DEPEND_CONNECT(video_quality, video_const_quality);
	DEPEND_CONNECT(video_quality_label, video_const_quality);
	DEPEND_CONNECT(video_bitrate, video_const_bitrate);
	DEPEND_CONNECT(video_two_pass, video_const_bitrate);
	DEPEND_CONNECT(video_st, video_const_bitrate);
	connect(ui.video_const_bitrate, SIGNAL(toggled(bool)), this, SLOT(updateSoftTarget()));
	connect(ui.video_const_bitrate, SIGNAL(toggled(bool)), this, SLOT(updateAdvanced()));
	connect(ui.video_st, SIGNAL(toggled(bool)), this, SLOT(updateSoftTarget()));
	connect(ui.video_st_quality_on, SIGNAL(toggled(bool)), this, SLOT(updateSoftTarget()));
	connect(ui.video_quality, SIGNAL(valueChanged(int)), ui.video_quality_label, SLOT(setNum(int)));
	connect(ui.video_st_quality, SIGNAL(valueChanged(int)), ui.video_st_quality_label, SLOT(setNum(int)));
	connect(ui.video_crop_left, SIGNAL(valueChanged(int)), this, SLOT(xcropChanged()));
	connect(ui.video_crop_right, SIGNAL(valueChanged(int)), this, SLOT(xcropChanged()));
	connect(ui.video_crop_top, SIGNAL(valueChanged(int)), this, SLOT(ycropChanged()));
	connect(ui.video_crop_bottom, SIGNAL(valueChanged(int)), this, SLOT(ycropChanged()));
	connect(ui.video_width, SIGNAL(valueChanged(int)), this, SLOT(videoWidthChanged()));
	connect(ui.video_height, SIGNAL(valueChanged(int)), this, SLOT(videoHeightChanged()));
	connect(ui.video_keep_proportions, SIGNAL(toggled(bool)), this, SLOT(fixVideoHeight()));
	set_label_min_size(ui.video_quality_label, "10");
	set_label_min_size(ui.video_st_quality_label, "10");
	ui.video_bitrate->setValidator(new QIntValidator(MIN_BITRATE, MAX_BITRATE, ui.video_bitrate));

	/* Advanced */
	connect(ui.advanced_contrast, SIGNAL(valueChanged(int)), this, SLOT(updateAdvanced()));
	connect(ui.advanced_brightness, SIGNAL(valueChanged(int)), this, SLOT(updateAdvanced()));
	connect(ui.advanced_gamma, SIGNAL(valueChanged(int)), this, SLOT(updateAdvanced()));
	connect(ui.advanced_saturation, SIGNAL(valueChanged(int)), this, SLOT(updateAdvanced()));
	connect(ui.advanced_adjust_reset, SIGNAL(released()), this, SLOT(resetAdjust()));
	DEPEND_CONNECT(advanced_keyint_value, advanced_keyint);
	DEPEND_CONNECT(advanced_format_value, advanced_format);
	DEPEND_CONNECT(advanced_bdelay_value, advanced_bdelay);
	set_label_min_size(ui.advanced_contrast_label, "10.0");
	ui.advanced_keyint_value->setValidator(new QIntValidator(MIN_KEYINT, MAX_KEYINT, ui.advanced_keyint_value));
	ui.advanced_bdelay_value->setValidator(new QIntValidator(MIN_BDELAY, MAX_BDELAY, ui.advanced_bdelay_value));

	/* Metadata */
	connect(ui.metadata_add, SIGNAL(toggled(bool)), this, SLOT(updateMetadata()));

	retrieveInfo();
	updateAdvancedMode();
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

static QString widget_value(QSpinBox *w)       { return QString::number(w->value()); }
static QString widget_value(QDoubleSpinBox *w) { return QString::number(w->value()); }
static QString widget_value(QSlider *w)        { return QString::number(w->value()); }
static QString widget_value(QComboBox *w)      { return w->currentText(); }
static QString widget_value(QLineEdit *w)      { return w->text(); }
static QString widget_value(QLabel *w)         { return w->text(); }

void
Frontend::transcode()
{
	if (QFileInfo(ui.output->text()).exists()) {
		if (QMessageBox::warning(this, "Overwrite file?",
			"The output file already exists. Do you want to replace it?",
			QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes
		)
			return;
	}

	QStringList ea;

#define OPTION(opt) ea = ea << opt
#define OPTION_FLAG(opt, widget) \
	if (ui.widget->isEnabled() && ui.widget->isChecked()) \
		OPTION(opt)
#define OPTION_VALUE(opt, widget) \
	if (ui.widget->isEnabled()) \
		ea = ea << opt << widget_value(ui.widget)
#define OPTION_DEFVALUE(opt, widget) \
	if (ui.widget->currentIndex() > 0) \
		OPTION_VALUE(opt, widget)
#define OPTION_TEXT(opt, widget) \
	if (!ui.widget->text().isEmpty()) \
		OPTION_VALUE(opt, widget)

	OPTION_VALUE("--starttime", partial_start);
	OPTION_VALUE("--endtime", partial_end);
	OPTION_FLAG("--sync", sync);
	OPTION_FLAG("--no-skeleton", no_skeleton);

	if (ui.audio_encode->isChecked()) {
		OPTION_VALUE("--audiostream", audio_stream);
		OPTION_VALUE("--channels", audio_channels);
		OPTION_VALUE("--samplerate", audio_samplerate);
		OPTION_VALUE("--audioquality", audio_quality);
		OPTION_VALUE("--audiobitrate", audio_bitrate);
	} else
		OPTION("--noaudio");

	if (ui.video_encode->isChecked()) {
		OPTION_VALUE("--videostream", video_stream);
		OPTION_VALUE("--videoquality", video_quality);
		OPTION_VALUE("--videobitrate", video_bitrate);
		OPTION_FLAG("--two-pass", video_two_pass);
		OPTION_FLAG("--soft-target", video_st);
		OPTION_VALUE("-v", video_st_quality);

		OPTION_VALUE("--width", video_width);
		OPTION_VALUE("--height", video_height);
		OPTION_FLAG("--max-size", video_max_size);
		OPTION_DEFVALUE("--aspect", video_frame_aspect_ratio);
		OPTION_VALUE("--croptop", video_crop_top);
		OPTION_VALUE("--cropbottom", video_crop_bottom);
		OPTION_VALUE("--cropleft", video_crop_left);
		OPTION_VALUE("--cropright", video_crop_right);
		OPTION_FLAG("--optimize", video_optimize);
		OPTION_FLAG("--deinterlace", video_deinterlace);
		OPTION_DEFVALUE("--inputfps", video_input_framerate);
		OPTION_DEFVALUE("--framerate", video_output_framerate);
	} else
		OPTION("--novideo");

	OPTION_VALUE("--contrast", advanced_contrast_label);
	OPTION_VALUE("--brightness", advanced_brightness_label);
	OPTION_VALUE("--gamma", advanced_gamma_label);
	OPTION_VALUE("--saturation", advanced_saturation_label);
	OPTION_FLAG("--no-upscaling", advanced_no_upscaling);
	OPTION_VALUE("--keyint", advanced_keyint_value);
	OPTION_TEXT("--format", advanced_format_value);
	OPTION_VALUE("--buf-delay", advanced_bdelay_value);

	if (ui.metadata_add->isChecked()) {
		OPTION_TEXT("--artist", metadata_artist);
		OPTION_TEXT("--title", metadata_title);
		OPTION_TEXT("--date", metadata_date);
		OPTION_TEXT("--location", metadata_location);
		OPTION_TEXT("--organization", metadata_organization);
		OPTION_TEXT("--copyright", metadata_copyright);
		OPTION_TEXT("--license", metadata_license);
		OPTION_TEXT("--contact", metadata_contact);
	}
#undef OPTION
#undef OPTION_FLAG
#undef OPTION_VALUE
#undef OPTION_DEFVALUE
#undef OPTION_TEXT

	ui.progress->setMaximum(finfo.duration > 0 ? int(finfo.duration) : 0);
	transcoder->start(ui.input->text(), ui.output->text(), ea);
}

bool
Frontend::cancel()
{
	switch (cancel_ask("You are going to cancel the encoding. ", true)) {
	case QMessageBox::Discard:
		keep_output = false;
		transcoder->stop();
		return true;
	case QMessageBox::Save:
		keep_output = true;
		transcoder->stop();
		return true;
	default:
		return false;
	}
}

void
Frontend::updateStatus(QString statusText)
{
	ui.status->setText(statusText);
}

void
Frontend::updateStatus(double pos, double eta, double audio_b, double video_b, int pass)
{
	QString status = "";
	status += "Position: " + time2string(pos);
	if (finfo.duration > 0)
		status += "/" + time2string(finfo.duration);
	if (audio_b > 0)
		status += "   Audio Bitrate: " + QString::number(audio_b, 'f', 0);
	if (video_b > 0)
		status += "   Video Bitrate: " + QString::number(video_b, 'f', 0);
	status += "   Time Elapsed: " + time2string(transcoder->elapsed(), 0, false);

	QString format =
		pass == 0 ? QString("First pass: ") :
		pass == 1 ? QString("Second pass: ") :
		QString();

	format += "%p%";
	if (eta > 0)
		format += QString("   ETA: ") + time2string(eta, 0, false);
	ui.progress->setFormat(format);

	int ipos = (int)pos;
	if (ipos < ui.progress->maximum())
		ui.progress->setValue(ipos);
	updateStatus(status);
}

void
Frontend::finished(int reason)
{
	QString finish_message;

	switch (reason) {
	case Transcoder::OK:
		keep_output = true;
		if (ui.progress->maximum() > 0)
			ui.progress->setValue(ui.progress->maximum());
		else {
			ui.progress->setMaximum(1);
			ui.progress->reset();
		}
		finish_message = "Encoding finished successfully";
		QMessageBox::information(this, "qtheorafeontend", finish_message);
		break;
	case Transcoder::FAILED:
		keep_output = cancel_ask("Encoding failed. ", false) == QMessageBox::Save;
	case Transcoder::STOPPED:
		finish_message = keep_output ?
			QString("Encoding failed. Partial file kept") :
			QString("Encoding failed. Partial file deleted");
		break;
	}

	if (!keep_output)
		QFile(transcoder->output_filename()).remove();
	emit updateStatus(finish_message);
}

void
Frontend::updateAdvancedMode()
{
	bool adv = ui.advanced_mode->isChecked();
	ui.advanced_mode->setArrowType(adv ? Qt::DownArrow : Qt::UpArrow);
	ui.partial->setVisible(adv);
	ui.partial_start->setVisible(adv);
	ui.partial_end->setVisible(adv);
	ui.sync->setVisible(adv);
	ui.no_skeleton->setVisible(adv);
	ui.tabs->setVisible(adv);
	layout()->activate();
	resize(QSize(width(), minimumSize().height()));
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
	bool missing_data = ui.input->text().isEmpty() ||
		ui.output->text().isEmpty() ||
		ui.input->text() == ui.output->text();
	bool encode = encode_audio() || encode_video();
	bool can_start = !running && !missing_data && encode && input_valid;

	ui.output_select->setEnabled(input_valid);
	ui.output->setEnabled(input_valid);
	ui.transcode->setEnabled(can_start);
	ui.transcode->setDefault(can_start);
	ui.cancel->setEnabled(running);
	ui.partial->setEnabled(input_valid);
	ui.progress->setEnabled(running);
	if (!running) {
		ui.progress->setMaximum(100);
		ui.progress->reset();
	}
	checkForSomethingToEncode();
	DEPEND(partial_start, partial);
	DEPEND(partial_end, partial);
	if (exitting)
		close();
}

void
Frontend::checkForSomethingToEncode()
{
	if (!transcoder->isRunning() && input_valid) {
		if (!encode_audio() && !encode_video())
			updateStatus("Nothing to encode");
		else if (ui.input->text() == ui.output->text())
			updateStatus("Input and output filenames must differ");
		else
			updateStatus("");
	}
}

void
Frontend::outputChanged()
{
	if (ui.output->text().endsWith(".ogg"))
		ui.no_skeleton->setChecked(true);
	else if (ui.output->text().endsWith(".oga")) {
		ui.no_skeleton->setChecked(false);
		ui.video_encode->setChecked(false);
	} else if (ui.output->text().endsWith(".ogv")) {
		ui.no_skeleton->setChecked(false);
		if (!finfo.video_streams.empty())
			ui.video_encode->setChecked(true);
	}
	updateButtons();
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
	QString s = ui.input->text();
	if (s.isEmpty())
		return;

	QFileInfo f(s);
	QString name = f.completeBaseName() + "." + default_extension();
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

		double max_time = finfo.duration > 0 ? finfo.duration : MAX_TIME;
		ui.partial_start->setMinimum(0);
		ui.partial_start->setMaximum(max_time);
		ui.partial_start->setValue(0);
		ui.partial_end->setMinimum(0);
		ui.partial_end->setMaximum(max_time);
		ui.partial_end->setValue(finfo.duration > 0 ? finfo.duration : 0);

		input_valid = true;
		updateStatus("");
	} catch (std::exception &x) {
		updateStatus(x.what());
	}
	ui.audio_encode->setEnabled(!finfo.audio_streams.empty());
	ui.audio_encode->setChecked(!finfo.audio_streams.empty());
	ui.video_encode->setEnabled(!finfo.video_streams.empty());
	ui.video_encode->setChecked(!finfo.video_streams.empty());

	get_streams(ui.info_audio_stream, finfo.audio_streams);
	get_streams(ui.info_video_stream, finfo.video_streams);
	get_streams(ui.audio_stream, finfo.audio_streams);
	get_streams(ui.video_stream, finfo.video_streams);

	setDefaultOutput();
	updateButtons();
	updateInfo();
	updateAudio();
	updateVideo(true);
	updateMetadata(true);
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
	45,
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

/* recursively enable/disable all first-level widgets in a layout */

static void
layout_enable(QLayout *layout, bool enabled)
{
	int c = layout->count();

	for (int i = 0; i < c; i++) {
		QWidget *w = layout->itemAt(i)->widget();
		QLayout *l = layout->itemAt(i)->layout();
		if (w != 0)
			w->setEnabled(enabled);
		else if (l != 0)
			layout_enable(l, enabled);
	}
}

void
Frontend::updateAudio()
{
	int i;

	layout_enable(ui.audio_options_layout, encode_audio());
	ui.audio_channels->clear();
	ui.audio_samplerate->clear();
	ui.audio_bitrate->clear();
	const AudioStreamInfo *s = stream(ui.audio_stream, finfo.audio_streams);
	if (s != NULL) {
		for (i = 0; i < s->channels; i++)
			ui.audio_channels->addItem(QString::number(i + 1));
		for (i = 0; i < LENGTH(samplerates); i++)
			if (s->samplerate <= 0 || samplerates[i] <= s->samplerate)
				ui.audio_samplerate->addItem(QString::number(samplerates[i]));
		if (ui.audio_samplerate->count() <= 0 && s->samplerate > 0)
			ui.audio_samplerate->addItem(QString::number(s->samplerate));
		for (i = 0; i < LENGTH(bitrates); i++)
			if (s->bitrate <= 0 || bitrates[i] <= s->bitrate)
				ui.audio_bitrate->addItem(QString::number(bitrates[i]));
		select_last(ui.audio_channels);
		select_last(ui.audio_samplerate);
		select_last(ui.audio_bitrate);
	}
}

void
Frontend::updateVideo(bool another_file)
{
	layout_enable(ui.video_options_layout, encode_video());
	const VideoStreamInfo *s = stream(ui.video_stream, finfo.video_streams);
	if (another_file && s != NULL) {
		ui.video_width->setValue(s->width);
		ui.video_height->setValue(s->height);

		ui.video_crop_left->setValue(0);
		ui.video_crop_right->setValue(0);
		ui.video_crop_top->setValue(0);
		ui.video_crop_bottom->setValue(0);

		ui.video_crop_left->setMaximum(s->width);
		ui.video_crop_right->setMaximum(s->width);
		ui.video_crop_top->setMaximum(s->height);
		ui.video_crop_bottom->setMaximum(s->height);
	}
	updateAdvanced(another_file);
}

void
Frontend::updateAdvanced(bool another_file)
{
	if (another_file)
		ui.advanced_adjust->setChecked(false);
	ui.advanced_contrast_label->setText(QString().sprintf("%.1f", ui.advanced_contrast->value() / ADJUST_SCALE));
	ui.advanced_brightness_label->setText(QString().sprintf("%.1f", ui.advanced_brightness->value() / ADJUST_SCALE));
	ui.advanced_gamma_label->setText(QString().sprintf("%.1f", ui.advanced_gamma->value() / ADJUST_SCALE));
	ui.advanced_saturation_label->setText(QString().sprintf("%.1f", ui.advanced_saturation->value() / ADJUST_SCALE));
	DEPEND(advanced_adjust, video_encode);
	DEPEND(advanced_keyint, video_encode);
	DEPEND(advanced_bdelay, video_const_bitrate);
	DEPEND(advanced_bdelay_value, advanced_bdelay);
}

void
Frontend::updateMetadata(bool another_file)
{
	if (another_file)
		ui.metadata_add->setChecked(false);
	layout_enable(ui.metadata_options_layout, ui.metadata_add->isChecked());
}

QString
Frontend::time2string(double t, int decimals, bool colons)
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

static QString
output_filter(bool has_video)
{
	QString ret = "*.oga;;*.ogg";
	if (has_video)
		ret = QString("*.ogv;;") + ret;
	return ret;
}

void
Frontend::fixExtension()
{
	QString out = ui.output->text();
	if (out.endsWith(".ogv") || out.endsWith(".oga") || out.endsWith(".ogg")) {
		out.chop(4);
		ui.output->setText(out + "." + default_extension());
	}
}

void
Frontend::selectOutput()
{
	QString ext = default_extension();
	output_dlg.setDefaultSuffix(ext);
	bool no_video = input_valid && finfo.video_streams.empty();
	output_dlg.setNameFilter(output_filter(!no_video));
	output_dlg.selectNameFilter(QString("*.") + ext);
	output_dlg.selectFile(ui.output->text());
	output_dlg.exec();
}

QString
Frontend::default_extension() const
{
	return ui.no_skeleton->isChecked() ? "ogg" : (encode_video() ? "ogv" : "oga");
}

double
Frontend::cropped_aspect() const
{
	const VideoStreamInfo *s = stream(ui.video_stream, finfo.video_streams);
	double x = s->width - ui.video_crop_left->value() - ui.video_crop_right->value();
	double y = s->height - ui.video_crop_top->value() - ui.video_crop_bottom->value();
	return x / y;
}

void
Frontend::fixVideoWidth()
{
	if (ui.video_keep_proportions->isChecked())
		ui.video_width->setValue(int(ui.video_height->value() * cropped_aspect() + 0.5));
}

void
Frontend::fixVideoHeight()
{
	if (ui.video_keep_proportions->isChecked())
		ui.video_height->setValue(int(ui.video_width->value() / cropped_aspect() + 0.5));
}

void
Frontend::xcropChanged()
{
	const VideoStreamInfo *s = stream(ui.video_stream, finfo.video_streams);
	ui.video_crop_left->setMaximum(s->width - ui.video_crop_right->value() - 1);
	ui.video_crop_right->setMaximum(s->width - ui.video_crop_left->value() - 1);
	fixVideoWidth();
}

void
Frontend::ycropChanged()
{
	const VideoStreamInfo *s = stream(ui.video_stream, finfo.video_streams);
	ui.video_crop_top->setMaximum(s->height - ui.video_crop_bottom->value() - 1);
	ui.video_crop_bottom->setMaximum(s->height - ui.video_crop_top->value() - 1);
	fixVideoHeight();
}

void
Frontend::videoWidthChanged()
{
	if (ui.video_width->hasFocus())
		fixVideoHeight();
}

void
Frontend::videoHeightChanged()
{
	if (ui.video_height->hasFocus())
		fixVideoWidth();
}

void
Frontend::updateSoftTarget()
{
	DEPEND(video_st_quality_on, video_st);
	DEPEND(video_st_quality, video_st_quality_on);
	DEPEND(video_st_quality_label, video_st_quality_on);
}

void
Frontend::resetAdjust()
{
	ui.advanced_contrast->setValue(int(1.0 * ADJUST_SCALE));
	ui.advanced_brightness->setValue(int(0.0 * ADJUST_SCALE));
	ui.advanced_gamma->setValue(int(1.0 * ADJUST_SCALE));
	ui.advanced_saturation->setValue(int(1.0 * ADJUST_SCALE));
}
