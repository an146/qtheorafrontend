/*
 * frontend.h - frontend widget declarations
 * This file is part of QTheoraFrontend.
 *
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

#ifndef H_FRONTEND
#define H_FRONTEND

#include <QFileDialog>
#include "transcoder.h"
#include "ui_dialog.h"

class Frontend : public QDialog
{
	Q_OBJECT

public:
	Frontend(QWidget* parent = 0);

public slots:
	void transcode();
	void updateStatus(QString statusText);
	void inputSelected(const QString &);
	void outputSelected(const QString &);
	void setDefaultOutput();

private:
	Ui::Dialog ui;
	QFileDialog input_dlg, output_dlg;

	Transcoder* transcoder;
};

#endif // H_FRONTEND
