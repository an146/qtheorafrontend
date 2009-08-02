/*
 * transcoder.h - transcoder thread declarations
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

#ifndef H_TRANSCODER
#define H_TRANSCODER

#include <QThread>
#include <QProcess>
#include <QMutex>

class Frontend;

class Transcoder : public QThread
{
	Q_OBJECT

public:
	explicit Transcoder(Frontend *);
	void start(const QString &input, const QString &output, const QStringList & = QStringList());
	static QString ffmpeg2theora();

	QString input_filename() { return input_filename_; }
	QString output_filename() { return output_filename_; }

	enum {
		OK,
		FAILED,
		STOPPED
	};
public slots:
	void stop();

signals:
	void statusUpdate(QString status);
	void statusUpdate(double pos, double eta, double audio_b, double video_b, int pass);
	void finished(int reason);

protected:
	void run();
	void processLine(const QString &);

protected slots:
	void readyRead();
	void procFinished(int, QProcess::ExitStatus);

signals:
	void terminate();

private:
	QString input_filename_;
	QString output_filename_;
	QProcess proc_;
	Frontend *frontend_;
	QStringList extra_args_;
	bool stopping_;

	double position_;
	double eta_;
	double audio_b_;
	double video_b_;
	int pass_;
};

#endif // H_TRANSCODER
