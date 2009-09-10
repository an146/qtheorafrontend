/* (C)opyright 2009 Anton Novikov
 * See LICENSE file for license details.
 *
 * queue_item.h
 */

#ifndef H_QUEUE_ITEM
#define H_QUEUE_ITEM

#include <QFrame>
#include "util.h"

class Transcoder;
class Ui_QueueItem;

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
	pimpl_ptr<Ui_QueueItem> ui;
	Transcoder *transcoder;
	bool finished_ok;
};

#endif /* H_QUEUE_ITEM */
