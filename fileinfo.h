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

#ifndef H_FILEINFO
#define H_FILEINFO

#include <QList>
#include <QString>

struct StreamInfo
{
	int id;
};

struct AudioStreamInfo : public StreamInfo
{
	QString audio_codec;
	double audio_bitrate;
	int samplerate;
	int channels;

	AudioStreamInfo(): audio_bitrate(-1), samplerate(-1), channels(-1) { }
};

struct VideoStreamInfo : public StreamInfo
{
	QString video_codec;
	double video_bitrate;
	QString pixel_format;
	int height;
	int width;
	QString framerate;
	QString pixel_aspect_ratio;
	QString display_aspect_ratio;

	VideoStreamInfo(): video_bitrate(-1), height(-1), width(-1) { }
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
