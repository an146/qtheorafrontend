#include <QFile>
#include <QScrollBar>
#include "queue_item.h"
#include "transcoder.h"
#include "ui_queue_item.h"
#include "util.h"

const QPalette::ColorRole normal_role = QPalette::Button;
const QPalette::ColorRole selected_role = QPalette::Base;

QueueItem::QueueItem(QWidget *parent, Transcoder *t)
	: QFrame(parent),
	transcoder(t),
	finished_ok(false)
{
	ui->setupUi(this);
	ui->filenames->setText(t->input_filename() + " -> " + t->output_filename());
	ui->details->setVisible(false);
	ui->log->setVisible(false);
	ui->progress->reset();
	setBackgroundRole(normal_role);

	connect(t, SIGNAL(started()), this, SLOT(transcoderStarted()));
	connect(t, SIGNAL(started()), this, SLOT(updateButtons()));
	connect(t, SIGNAL(finished()), this, SLOT(updateButtons()));
	connect(t, SIGNAL(finished(int)), this, SLOT(finished(int)));
	connect(t, SIGNAL(statusUpdate(double, double, double, double, double, int, QString)),
			this, SLOT(updateStatus(double, double, double, double, double, int, QString)));

	connect(ui->details_toggle, SIGNAL(toggled(bool)), this, SLOT(updateButtons()));
	connect(ui->log_toggle, SIGNAL(toggled(bool)), this, SLOT(updateButtons()));
	connect(ui->start, SIGNAL(released()), transcoder, SLOT(start()));
	connect(ui->stop, SIGNAL(released()), transcoder, SLOT(stop()));
	connect(ui->remove, SIGNAL(released()), this, SLOT(close()));
	updateButtons();
}

QueueItem::~QueueItem()
{
	delete transcoder;
}

void
QueueItem::focusInEvent(QFocusEvent *ev)
{
	setBackgroundRole(selected_role);
	return QFrame::focusInEvent(ev);
}
	
void
QueueItem::focusOutEvent(QFocusEvent *ev)
{
	setBackgroundRole(normal_role);
	return QFrame::focusOutEvent(ev);
}

void
QueueItem::finished(int reason)
{
	QString finish_message;
	Transcoder *transcoder = qobject_cast<Transcoder *>(sender());
	if (transcoder == NULL)
		return;

	switch (reason) {
	case Transcoder::OK:
		if (ui->progress->maximum() > 0)
			ui->progress->setValue(ui->progress->maximum());
		else {
			ui->progress->setMaximum(1);
			ui->progress->reset();
		}
		finish_message = "Encoding finished successfully";
		finished_ok = true;
		break;
	case Transcoder::FAILED:
		finish_message = "Encoding failed";
		break;
	case Transcoder::STOPPED:
		finish_message = "Encoding stopped";
		break;
	}

	ui->status->setText(finish_message);
}

void
QueueItem::transcoderStarted()
{
	QString cmdline;
	foreach (QString arg, QStringList() << Transcoder::ffmpeg2theora() << transcoder->args()) {
		if (arg.contains(" "))
			cmdline += "\"" + arg + "\"";
		else
			cmdline += arg;
		cmdline += " ";
	}
	ui->log->moveCursor(QTextCursor::End);
	ui->log->insertHtml("<b>" + cmdline + "</b><br>");
}

static void
update_arrow(QToolButton *btn)
{
	btn->setArrowType(btn->isChecked() ? Qt::DownArrow : Qt::RightArrow);
}

void
QueueItem::updateButtons()
{
	update_arrow(ui->details_toggle);
	update_arrow(ui->log_toggle);

	bool r = transcoder->isRunning();
	ui->start->setEnabled(!r && !finished_ok);
	ui->stop->setEnabled(r);
	ui->remove->setEnabled(!r);
}

void
QueueItem::updateStatus(double duration, double pos, double eta, double audio_b, double video_b, int pass, QString raw)
{
	Transcoder *transcoder = qobject_cast<Transcoder *>(sender());
	if (transcoder == NULL)
		return;

	QString status = "";
	status += "Position: " + time2string(pos);
	if (duration > 0)
		status += "/" + time2string(duration);
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
	
	int ipos = (int)pos;
	int idur = (int)duration;
	ui->progress->setFormat(format);
	if (ipos < idur) {
		ui->progress->setMaximum(idur);
		ui->progress->setValue(ipos);
	} else {
		ui->progress->setMaximum(0);
		ui->progress->setValue(0);
	}
	ui->status->setText(status);
	
	QTextCursor cur(ui->log->textCursor());
	cur.setCharFormat(QTextCharFormat());
	cur.movePosition(QTextCursor::End);
	
	QScrollBar *vs = ui->log->verticalScrollBar();
	bool scroll_to_end = vs->value() == vs->maximum();
	cur.insertText(raw + "\n");
	if (scroll_to_end)
		vs->setValue(vs->maximum());
}

