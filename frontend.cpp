/*
 * frontend.cpp - frontend widget for invoking ffmpeg2theora
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

#include <QMessageBox>
#include <QCloseEvent>
#include "frontend.h"

static const char *input_filter = "Video files (*.avi *.mpg *.flv);;Any files (*.*)";

Frontend::Frontend(QWidget* parent)
	: QDialog(parent),
	input_dlg(this, "Select the input file", QString(), input_filter),
	output_dlg(this, "Select the output file", QString(), "*.ogv"),
	output_auto(true),
	exitting(false)
{
	ui.setupUi(this);
	transcoder = new Transcoder(this);

	connect(ui.input_select, SIGNAL(released()), &input_dlg, SLOT(exec()));
	connect(ui.output_select, SIGNAL(released()), &output_dlg, SLOT(exec()));
	connect(&input_dlg, SIGNAL(fileSelected(QString)), this, SLOT(inputSelected(QString)));
	connect(&output_dlg, SIGNAL(fileSelected(QString)), this, SLOT(outputSelected(QString)));
	input_dlg.setFileMode(QFileDialog::ExistingFile);
	output_dlg.setAcceptMode(QFileDialog::AcceptSave);
	output_dlg.setFileMode(QFileDialog::AnyFile);
	output_dlg.setDefaultSuffix("ogv");

	connect(ui.input, SIGNAL(textChanged(QString)), this, SLOT(setDefaultOutput()));
	connect(ui.input, SIGNAL(textChanged(QString)), this, SLOT(updateButtons()));
	connect(ui.output, SIGNAL(textChanged(QString)), this, SLOT(updateButtons()));
	connect(ui.output, SIGNAL(textEdited(QString)), this, SLOT(outputEdited()));
	connect(ui.transcode, SIGNAL(released()), this, SLOT(transcode()));
	connect(ui.cancel, SIGNAL(released()), this, SLOT(cancel()));
	connect(transcoder, SIGNAL(started()), this, SLOT(updateButtons()));
	connect(transcoder, SIGNAL(finished()), this, SLOT(updateButtons()));
	connect(transcoder, SIGNAL(statusUpdate(QString)),
		this, SLOT(updateStatus(QString)));
	updateButtons();
}

int Frontend::cancel_ask(const QString &reason, bool cancel_button)
{
	QMessageBox::StandardButtons buttons = QMessageBox::Save | QMessageBox::Discard;
	if (cancel_button)
		buttons |= QMessageBox::Cancel;
	return QMessageBox::question(
		this,
		"qtheorafrontend",
		reason + "Do you want to keep the partially encoded file?",
		buttons,
		QMessageBox::Discard
	);
}

void Frontend::closeEvent(QCloseEvent *event)
{
	if (!transcoder->isRunning())
		event->accept();
	else {
		if (cancel())
			exitting = true;
		event->ignore();
	}
}

void Frontend::transcode()
{
	transcoder->start(ui.input->text(), ui.output->text());
}

bool Frontend::cancel()
{
	bool keep = false;

	switch (cancel_ask("You are going to cancel the encoding. ", true)) {
	case QMessageBox::Save:
		keep = true;
	case QMessageBox::Discard:
		transcoder->stop(keep);
		return true;
	}
	return false;
}

void Frontend::updateStatus(QString statusText)
{
	ui.status->setText(statusText);
}

void Frontend::inputSelected(const QString &s)
{
	ui.input->setText(s);
	setDefaultOutput();
}

void Frontend::outputSelected(const QString &s)
{
	ui.output->setText(s);
}

void Frontend::updateButtons()
{
	bool running = transcoder->isRunning();
	bool missing_data = ui.input->text().isEmpty() || ui.output->text().isEmpty();
	bool can_start = !running && !missing_data;
	ui.transcode->setEnabled(can_start);
	ui.transcode->setDefault(can_start);
	ui.cancel->setEnabled(running);
	if (exitting)
		close();
}

void Frontend::outputEdited()
{
	output_auto = ui.output->text().isEmpty();
}

void Frontend::setDefaultOutput()
{
	if (!output_auto)
		return;

	QString s = ui.input->text();
	if (s.isEmpty())
		return;

	QFileInfo f(s);
	QString name = f.completeBaseName() + ".ogv";
	ui.output->setText(f.dir().filePath(name));
}
