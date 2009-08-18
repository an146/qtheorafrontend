/* (C)opyright 2009 Anton Novikov
 * See LICENSE file for license details.
 *
 * queue_item.h
 */

#ifndef H_QUEUE_ITEM
#define H_QUEUE_ITEM

#include <QFrame>
#include "ui_queue_item.h"

class Transcoder;

class QueueItem : public QFrame
{
	Q_OBJECT

public:
	QueueItem(QWidget *, Transcoder *);
	~QueueItem();

protected:
	void focusInEvent(QFocusEvent *);
	void focusOutEvent(QFocusEvent *);

protected slots:
	void finished(int);
	void transcoderStarted();
	void updateButtons();
	void updateStatus(double duration, double pos, double eta, double audio_b, double video_b, int pass, QString);

private:
	Ui::QueueItem ui;
	Transcoder *transcoder;
};

#endif /* H_QUEUE_ITEM */
