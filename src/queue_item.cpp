#include <QFile>
#include <QScrollBar>
#include "fileinfo.h"
#include "queue_item.h"
#include "transcoder.h"
#include "ui_queue_item.h"
#include "util.h"

const QPalette::ColorRole normal_role = QPalette::Button;
const QPalette::ColorRole selected_role = QPalette::Base;

struct QueueItem::Impl
{
	Ui_QueueItem ui;
	FileInfo fi;
	
	Impl(const FileInfo &_fi)
		: fi(_fi)
	{
	}
};

QueueItem::QueueItem(QWidget *parent, Transcoder *t, const FileInfo &_fi)
	: QFrame(parent),
	impl(_fi),
	transcoder_(t),
	finished_ok_(false),
	progress_(0)
{
	impl->ui.setupUi(this);
	impl->ui.filenames->setText(t->input_filename() + " -> " + t->output_filename());
	impl->ui.details->setVisible(false);
	impl->ui.log->setVisible(false);
	impl->ui.progress->reset();
	setBackgroundRole(normal_role);

	connect(t, SIGNAL(started()), this, SLOT(transcoderStarted()));
	connect(t, SIGNAL(started()), this, SLOT(updateButtons()));
	connect(t, SIGNAL(finished()), this, SLOT(updateButtons()));
	connect(t, SIGNAL(finished(int)), this, SLOT(finished(int)));
	connect(t, SIGNAL(statusUpdate(double, double, double, double, double, int, QString)),
			this, SLOT(updateStatus(double, double, double, double, double, int, QString)));

	connect(impl->ui.details_toggle, SIGNAL(toggled(bool)), this, SLOT(updateButtons()));
	connect(impl->ui.log_toggle, SIGNAL(toggled(bool)), this, SLOT(updateButtons()));
	connect(impl->ui.start, SIGNAL(released()), transcoder(), SLOT(start()));
	connect(impl->ui.stop, SIGNAL(released()), transcoder(), SLOT(stop()));
	connect(impl->ui.remove, SIGNAL(released()), this, SLOT(close()));
	updateButtons();
}

QueueItem::~QueueItem()
{
	delete transcoder();
}

const FileInfo &
QueueItem::fileinfo() const
{
	return impl->fi;
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

	switch (reason) {
	case Transcoder::OK:
		if (impl->ui.progress->maximum() <= 0)
			impl->ui.progress->setMaximum(1);
		impl->ui.progress->setValue(impl->ui.progress->maximum());
		impl->ui.progress->setFormat("Done");
		finish_message = "Encoding finished successfully";
		finished_ok_ = true;
		progress_ = 1;
		break;
	case Transcoder::FAILED:
		finish_message = "Encoding failed";
		progress_ = 0;
		break;
	case Transcoder::STOPPED:
		finish_message = "Encoding stopped";
		progress_ = 0;
		break;
	}

	impl->ui.status->setText(finish_message);
}

void
QueueItem::transcoderStarted()
{
	QString cmdline;
	foreach (QString arg, QStringList() << Transcoder::ffmpeg2theora() << transcoder()->args()) {
		if (arg.contains(" "))
			cmdline += "\"" + arg + "\"";
		else
			cmdline += arg;
		cmdline += " ";
	}
	impl->ui.log->moveCursor(QTextCursor::End);
	impl->ui.log->insertHtml("<b>" + cmdline + "</b><br>");
}

static void
update_arrow(QToolButton *btn)
{
	btn->setArrowType(btn->isChecked() ? Qt::DownArrow : Qt::RightArrow);
}

void
QueueItem::updateButtons()
{
	update_arrow(impl->ui.details_toggle);
	update_arrow(impl->ui.log_toggle);

	bool r = transcoder()->isRunning();
	impl->ui.start->setEnabled(!r && !finished_ok_);
	impl->ui.stop->setEnabled(r);
	impl->ui.remove->setEnabled(!r);
}

void
QueueItem::updateStatus(double duration, double pos, double eta, double audio_b, double video_b, int pass, QString raw)
{
	QString status = "";
	status += "Position: " + time2string(pos);
	if (duration > 0)
		status += "/" + time2string(duration);
	if (audio_b > 0)
		status += "   Audio Bitrate: " + QString::number(audio_b, 'f', 0);
	if (video_b > 0)
		status += "   Video Bitrate: " + QString::number(video_b, 'f', 0);
	status += "   Time Elapsed: " + time2string(transcoder()->elapsed(), 0, false);
	
	if (duration > 0 && pos > 0 && pos < duration)
		progress_ = pos / duration;

	QString format =
		pass == 0 ? QString("First pass: ") :
		pass == 1 ? QString("Second pass: ") :
		QString();

	format += "%p%";
	if (eta > 0)
		format += QString("   ETA: ") + time2string(eta, 0, false);
	
	int ipos = (int)pos;
	int idur = (int)duration;
	impl->ui.progress->setFormat(format);
	if (ipos < idur) {
		impl->ui.progress->setMaximum(idur);
		impl->ui.progress->setValue(ipos);
	} else {
		impl->ui.progress->setMaximum(0);
		impl->ui.progress->setValue(0);
	}
	impl->ui.status->setText(status);
	
	QTextCursor cur(impl->ui.log->textCursor());
	cur.setCharFormat(QTextCharFormat());
	cur.movePosition(QTextCursor::End);
	
	QScrollBar *vs = impl->ui.log->verticalScrollBar();
	bool scroll_to_end = vs->value() == vs->maximum();
	cur.insertText(raw + "\n");
	if (scroll_to_end)
		vs->setValue(vs->maximum());
}

