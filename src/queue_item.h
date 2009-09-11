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
class FileInfo;

class QueueItem : public QFrame
{
	Q_OBJECT

public:
	QueueItem(QWidget *, Transcoder *, const FileInfo &);
	~QueueItem();
	double progress() const { return progress_; }
	Transcoder *transcoder() { return transcoder_; }
	const FileInfo &fileinfo() const;

protected:
	void focusInEvent(QFocusEvent *);
	void focusOutEvent(QFocusEvent *);

protected slots:
	void finished(int);
	void transcoderStarted();
	void updateButtons();
	void updateStatus(double duration, double pos, double eta, double audio_b, double video_b, int pass, QString);

private:
	struct Impl;
	pimpl_ptr<Impl> impl;
	
	Transcoder *transcoder_;
	bool finished_ok_;
	double progress_;
};

#endif /* H_QUEUE_ITEM */
