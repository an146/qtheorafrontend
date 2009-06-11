/*
 * frontend.cpp - frontend widget for invoking ffmpeg2theora
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

#include "frontend.h"

Frontend::Frontend(QWidget* parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	transcoder = new Transcoder("test.flv");
	connect(ui.transcode, SIGNAL(released()), this, SLOT(transcode()));
	connect(transcoder, SIGNAL(statusUpdate(QString)),
		this, SLOT(updateStatus(QString)));
}

void Frontend::transcode()
{
	transcoder->start();
}

void Frontend::updateStatus(QString statusText)
{
	ui.status->setText(statusText);
}
