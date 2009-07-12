#include <QProcess>
#include <stdexcept>
#include "fileinfo.h"
#include "transcoder.h"

#define FIELDS \
	FILE_FIELD(duration, .toDouble()) \
	FILE_FIELD(bitrate, .toDouble()) \
	FILE_FIELD(size, .toLongLong()) \
\
	STREAM_FIELD(id, .toInt()) \
\
	VSTREAM_FIELD(video_codec, ) \
	VSTREAM_FIELD(video_bitrate, .toDouble()) \
	VSTREAM_FIELD(pixel_format, ) \
	VSTREAM_FIELD(height, .toInt()) \
	VSTREAM_FIELD(width, .toInt()) \
	VSTREAM_FIELD(framerate, ) \
	VSTREAM_FIELD(pixel_aspect_ratio, ) \
	VSTREAM_FIELD(display_aspect_ratio, ) \
\
	ASTREAM_FIELD(audio_codec, ) \
	ASTREAM_FIELD(audio_bitrate, .toDouble()) \
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

	char buf[BUF_SIZE] = "";
	proc.waitForReadyRead();
	StreamInfo *last_stream = NULL;
	AudioStreamInfo *last_astream = NULL;
	VideoStreamInfo *last_vstream = NULL;
	while (proc.readLine(buf, BUF_SIZE) > 0) {
		QStringList sl = untail(buf, ',').split(": ");
		if (sl.size() < 2)
			continue;
		QString key = unquote(sl[0]);
		QString value = unquote(sl[1]);
		if (key == "audio_codec") {
			audio_streams.push_back(AudioStreamInfo());
			last_stream = last_astream = &audio_streams.back();
		} else if (key == "video_codec") {
			video_streams.push_back(VideoStreamInfo());
			last_stream = last_vstream = &video_streams.back();
		}
#define FIELD(field, conv, obj) \
		if (key == #field && obj != NULL) \
			obj->field = value conv;
#define FILE_FIELD(f, c) FIELD(f, c, this)
#define STREAM_FIELD(f, c) FIELD(f, c, last_stream)
#define ASTREAM_FIELD(f, c) FIELD(f, c, last_astream)
#define VSTREAM_FIELD(f, c) FIELD(f, c, last_vstream)
		FIELDS
#undef FIELD
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
