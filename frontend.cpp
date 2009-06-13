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
	: QDialog(parent),
	input_dlg(this, "Select the input file", QString(), "*.flv"),
	output_dlg(this, "Select the output file", QString(), "*.ogv")
{
	ui.setupUi(this);
	transcoder = new Transcoder();

	connect(ui.input_select, SIGNAL(released()), &input_dlg, SLOT(exec()));
	connect(ui.output_select, SIGNAL(released()), &output_dlg, SLOT(exec()));
	connect(&input_dlg, SIGNAL(fileSelected(QString)), this, SLOT(inputSelected(QString)));
	connect(&output_dlg, SIGNAL(fileSelected(QString)), this, SLOT(outputSelected(QString)));
	input_dlg.setFileMode(QFileDialog::ExistingFile);
	output_dlg.setAcceptMode(QFileDialog::AcceptSave);
	output_dlg.setFileMode(QFileDialog::AnyFile);
	output_dlg.setDefaultSuffix("ogv");

	connect(ui.input, SIGNAL(textChanged(QString)), this, SLOT(setDefaultOutput()));
	connect(ui.input, SIGNAL(textChanged(QString)), this, SLOT(filesUpdated()));
	connect(ui.output, SIGNAL(textChanged(QString)), this, SLOT(filesUpdated()));
	connect(ui.transcode, SIGNAL(released()), this, SLOT(transcode()));
	connect(transcoder, SIGNAL(statusUpdate(QString)),
		this, SLOT(updateStatus(QString)));
	filesUpdated();
}

void Frontend::transcode()
{
	transcoder->set_filenames(ui.input->text(), ui.output->text());
	transcoder->start();
}

void Frontend::updateStatus(QString statusText)
{
	ui.status->setText(statusText);
}

void Frontend::inputSelected(const QString &s)
{
	ui.input->setText(s);

	/* if user has selected the same file,
	 * ui.input->text() doesn't change, but still
	 * he probably wants default output to be set
	 */
	setDefaultOutput();
}

void Frontend::outputSelected(const QString &s)
{
	ui.output->setText(s);
}

void Frontend::filesUpdated()
{
	bool missing_data = ui.input->text().isEmpty() || ui.output->text().isEmpty();
	ui.transcode->setEnabled(!missing_data);
	ui.transcode->setDefault(!missing_data);
}

void Frontend::setDefaultOutput()
{
	QString s = ui.input->text();
	if (s.isEmpty())
		return;

	if (s.right(4) == ".flv")
		s.chop(4);
	s += ".ogv";
	ui.output->setText(s);
}
