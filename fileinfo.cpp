/*
 * fileinfo.cpp - file info retrieval
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

#include <QProcess>
#include <stdexcept>
#include "fileinfo.h"
#include "transcoder.h"

#define FIELDS \
	FILE_FIELD(duration, .toDouble()) \
	FILE_FIELD(bitrate, .toDouble()) \
	FILE_FIELD(size, .toLongLong()) \
\
	STREAM_FIELD(codec, ) \
	STREAM_FIELD(bitrate, .toDouble()) \
	STREAM_FIELD(id, .toInt()) \
\
	VSTREAM_FIELD(pixel_format, ) \
	VSTREAM_FIELD(height, .toInt()) \
	VSTREAM_FIELD(width, .toInt()) \
	VSTREAM_FIELD(framerate, ) \
	VSTREAM_FIELD(pixel_aspect_ratio, ) \
	VSTREAM_FIELD(display_aspect_ratio, ) \
\
	ASTREAM_FIELD(samplerate, .toInt()) \
	ASTREAM_FIELD(channels, .toInt()) \

static QString
untail(const QString &s, char c)
{
	QString ret = s.trimmed();
	if (!ret.isEmpty() && ret[ret.size() - 1] == c)
		ret.chop(1);
	return ret;
}

static QString
unquote(const QString &s)
{
	QString ret = s.trimmed();
	if (!ret.isEmpty() && ret[0] == '"')
		ret.remove(0, 1);
	return untail(ret, '"');
}

#define BUF_SIZE 256

enum StreamType {
	NONE,
	AUDIO,
	VIDEO
};

struct State {
	StreamInfo *stream;
	StreamType stream_type;
	AudioStreamInfo *astream;
	VideoStreamInfo *vstream;

	State(): stream(NULL), stream_type(NONE), astream(NULL), vstream(NULL) { }
};

static void
process_field(FileInfo *fi, State *s, const QString &key, const QString &value)
{

	if (key == "audio")
		s->stream_type = AUDIO;
	else if (key == "video")
		s->stream_type = VIDEO;
	else if (key == "codec") {
		switch (s->stream_type) {
		case AUDIO:
			fi->audio_streams.push_back(AudioStreamInfo());
			s->stream = s->astream = &fi->audio_streams.back();
			break;
		case VIDEO:
			fi->video_streams.push_back(VideoStreamInfo());
			s->stream = s->vstream = &fi->video_streams.back();
			break;
		default:
			break;
		}
	} else if (key == "audio_codec") {
		process_field(fi, s, "audio", "");
		process_field(fi, s, "codec", value);
	} else if (key == "video_codec") {
		process_field(fi, s, "video", "");
		process_field(fi, s, "codec", value);
	}

#define FIELD(field, conv, obj) \
	if (key == #field && obj != NULL) \
		obj->field = value conv;
#define FILE_FIELD(f, c) FIELD(f, c, fi)
#define STREAM_FIELD(f, c) FIELD(f, c, s->stream)
#define ASTREAM_FIELD(f, c) FIELD(f, c, s->astream)
#define VSTREAM_FIELD(f, c) FIELD(f, c, s->vstream)
	FIELDS
#undef FIELD
}

void
FileInfo::retrieve(const QString &filename)
{
	duration = -1;
	bitrate = -1;
	size = -1;
	audio_streams.clear();
	video_streams.clear();

	QProcess proc;
	proc.start(Transcoder::ffmpeg2theora(), QStringList() << "--info" << filename);
	if (!proc.waitForStarted())
		throw std::runtime_error("Info retrieval failed to start");

	State state;
	char buf[BUF_SIZE] = "";
	proc.waitForReadyRead();
	while (proc.readLine(buf, BUF_SIZE) > 0) {
		QStringList sl = untail(buf, ',').split(": ");
		if (sl.size() < 2)
			continue;
		QString key = unquote(sl[0]);
		QString value = unquote(sl[1]);

		process_field(this, &state, key, value);
		proc.waitForReadyRead();
	}
	proc.setReadChannel(QProcess::StandardError);
	proc.waitForReadyRead();
	/*
	while (proc.readLine(buf, BUF_SIZE) > 0)
		updateStatus(buf);
	*/
	proc.waitForFinished();
	if (proc.exitCode() != 0 || proc.exitStatus() != QProcess::NormalExit)
		throw std::runtime_error("Invalid input file");
}
