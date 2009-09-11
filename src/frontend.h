/*
 * frontend.h - frontend widget declarations
 * This file is part of QTheoraFrontend.
 *
 * Copyright (C) 2009  Anton Novikov <an146@ya.ru>
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

#ifndef H_FRONTEND
#define H_FRONTEND

#include <QFileDialog>
#include "fileinfo.h"
#include "util.h"

class Ui_Dialog;
class QueueItem;

class Frontend : public QDialog
{
	Q_OBJECT

public:
	Frontend(QWidget* parent = 0);
	int cancel_ask(const QString &, bool);
	int queue_items() const;
	int queue_items_running() const;
	int queue_items_pending() const;
	QueueItem *queue_item(int) const;
	bool encode_audio() const;
	bool encode_video() const;

protected:
	void closeEvent(QCloseEvent *);
	QString default_extension() const;
	double cropped_aspect() const;

	void readSettings();
	void writeSettings();

protected slots:
	void transcode();
	bool cancel();
	void checkQueue();
	void updateStatus(QString statusText);
	void updateButtons();
	void checkForSomethingToEncode();
	void transcoderStarted();
	void transcoderDestroyed();

	void updateAdvancedMode();
	void outputSelected(const QString &);
	void outputChanged();
	void setDefaultOutput();
	void fixExtension();
	void selectOutput();

	void updateProgress();
	void retrieveInfo();
	void updateInfo();
	void updateAudio();
	void updateVideo(bool another_file = false);
	void updateSubtitles(bool another_file = false);
	void updateAdvanced(bool another_file = false);
	void updateMetadata(bool another_file = false);
	void fixVideoWidth();
	void fixVideoHeight();
	void xcropChanged();
	void ycropChanged();
	void videoWidthChanged();
	void videoHeightChanged();
	void updateSoftTarget();
	void resetAdjust();
	void keepPartialChanged();

private:
	pimpl_ptr<Ui_Dialog> ui;
	QFileDialog input_dlg, output_dlg, subtitles_dlg;
	bool output_auto;
	bool exitting;
	bool input_valid;
	FileInfo finfo;
};

#endif // H_FRONTEND
