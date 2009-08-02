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
#include "transcoder.h"
#include "fileinfo.h"
#include "ui_dialog.h"

class Frontend : public QDialog
{
	Q_OBJECT

public:
	Frontend(QWidget* parent = 0);
	int cancel_ask(const QString &, bool);

	static QString time2string(double, int decimals = 0);

protected:
	void closeEvent(QCloseEvent *);
	bool encode_audio() const { return ui.audio_encode->isChecked(); }
	bool encode_video() const { return ui.video_encode->isChecked(); }
	QString default_extension() const;

protected slots:
	void transcode();
	bool cancel();
	void updateStatus(QString statusText);
	void updateStatus(double pos, double eta, double audio_b, double video_b, int pass);
	void finished(int reason);
	void updateButtons();
	void checkForSomethingToEncode();

	void inputSelected(const QString &);
	void outputSelected(const QString &);
	void outputChanged();
	void outputEdited();
	void partialStateChanged();
	void setDefaultOutput();
	void fixExtension();
	void selectOutput();

	void retrieveInfo();
	void updateInfo();
	void updateAudio();
	void updateVideo(bool another_file = false);
	double cropped_aspect() const;
	void fixVideoWidth();
	void fixVideoHeight();
	void xcropChanged();
	void ycropChanged();
	void videoWidthChanged();
	void videoHeightChanged();

private:
	Ui::Dialog ui;
	QFileDialog input_dlg, output_dlg;
	bool output_auto;
	bool exitting;
	bool input_valid;
	bool keep_output;
	FileInfo finfo;

	Transcoder* transcoder;
};

#endif // H_FRONTEND
