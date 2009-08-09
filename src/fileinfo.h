/*
 * fileinfo.h - file info retrieval
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

#ifndef H_FILEINFO
#define H_FILEINFO

#include <QList>
#include <QString>

struct StreamInfo
{
	QString codec;
	double bitrate;
	int id;

	StreamInfo(): id(-1) { }
};

struct AudioStreamInfo : public StreamInfo
{
	int samplerate;
	int channels;

	AudioStreamInfo(): samplerate(-1), channels(-1) { }
};

struct VideoStreamInfo : public StreamInfo
{
	QString pixel_format;
	int height;
	int width;
	QString framerate;
	QString pixel_aspect_ratio;
	QString display_aspect_ratio;

	VideoStreamInfo(): height(-1), width(-1) { }
};

struct FileInfo
{
	double duration;
	double bitrate;
	long long size;
	QList<AudioStreamInfo> audio_streams;
	QList<VideoStreamInfo> video_streams;

	void retrieve(const QString &);
};

#endif // H_FILEINFO
