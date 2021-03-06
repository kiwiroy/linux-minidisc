#include "qmdtrack.h"

static QString get_himd_str(struct himd * himd, int idx)
{
    QString outstr;
    char * str;
    if(!idx)
        return QString();
    str = himd_get_string_utf8(himd, idx, NULL, NULL);
    if(!str)
        return QString();

    outstr = QString::fromUtf8(str);
    himd_free(str);
    return outstr;
}

QHiMDTrack::QHiMDTrack(struct himd * himd, unsigned int trackindex) : himd(himd), trknum(trackindex)
{
    trackslot = himd_get_trackslot(himd, trackindex, NULL);
    if(trackslot != 0)
        if(himd_get_track_info(himd, trackslot, &ti, NULL) < 0)
            trackslot = -1;
}

QHiMDTrack::~QHiMDTrack()
{
    himd = NULL;
}

unsigned int QHiMDTrack::tracknum() const
{
    return trknum;
}

QString QHiMDTrack::title() const
{
    if(trackslot != 0)
        return get_himd_str(himd, ti.title);
    else
        return QString();
}

QString QHiMDTrack::artist() const
{
    if(trackslot != 0)
        return get_himd_str(himd, ti.artist);
    else
        return QString();
}

QString QHiMDTrack::album() const
{
    if(trackslot != 0)
        return get_himd_str(himd, ti.album);
    else
        return QString();
}

QString QHiMDTrack::codecname() const
{
    if(trackslot != 0)
        return himd_get_codec_name(&ti);
    else
        return QString();
}

QTime QHiMDTrack::duration() const
{
    QTime t(0,0,0);
    if(trackslot != 0)
        return t.addSecs(ti.seconds);
    else
        return QTime();
}

QDateTime QHiMDTrack::recdate() const
{
    QDate d(0,0,0);
    QTime t(0,0,0);
    if (trackslot != 0)
    {
        t.setHMS(ti.recordingtime.tm_hour,
          ti.recordingtime.tm_min,
          ti.recordingtime.tm_sec);
        d.setDate(ti.recordingtime.tm_year+1900,
          ti.recordingtime.tm_mon+1,
          ti.recordingtime.tm_mday);
        return QDateTime(d,t);
    }
    return QDateTime();
}

bool QHiMDTrack::copyprotected() const
{
    if(trackslot != 0)
        return !himd_track_uploadable(himd, &ti);
    return true;
}

int QHiMDTrack::blockcount() const
{
    if(trackslot != 0)
        return himd_track_blocks(himd, &ti, NULL);
    else
        return 0;
}

QString QHiMDTrack::openMpegStream(struct himd_mp3stream * str) const
{
    struct himderrinfo status;
    if(himd_mp3stream_open(himd, trackslot, str, &status) < 0)
        return QString::fromUtf8(status.statusmsg);
    return QString();
}

QString QHiMDTrack::openNonMpegStream(struct himd_nonmp3stream * str) const
{
    struct himderrinfo status;
    if(himd_nonmp3stream_open(himd, trackslot, str, &status) < 0)
        return QString::fromUtf8(status.statusmsg);
    return QString();
}

QByteArray QHiMDTrack::makeEA3Header() const
{
    char header[EA3_FORMAT_HEADER_SIZE];
    make_ea3_format_header(header, &ti.codec_info);
    return QByteArray(header,EA3_FORMAT_HEADER_SIZE);
}


QNetMDTrack::QNetMDTrack(netmd_dev_handle * deviceh, minidisc * my_md, int trackindex)
{
    uint8_t g;
    struct netmd_pair const *bitrate;
    char *name, buffer[256];

    devh = deviceh;
    md = my_md;
    trkindex = trackindex;

    if(netmd_request_title(devh, trkindex, buffer, sizeof(buffer)) < 0)
    {
        trkindex = -1;
        return;  // no track with this trackindex
    }

    /* Figure out which group this track is in */
    for( g = 1; g < md->group_count; g++ )
    {
        if( (md->groups[g].start <= trkindex+1U) && (md->groups[g].finish >= trkindex+1U ))
        {
            groupstring = QString(md->groups[g].name);
            break;
        }
    }

    netmd_request_track_time(devh, trkindex, &time);
    netmd_request_track_flags(devh, trkindex, &flags);
    netmd_request_track_bitrate(devh, trkindex, &bitrate_id, &channel);

    bitrate = find_pair(bitrate_id, bitrates);

    /* Skip 'LP:' prefix... the codec type shows up in the list anyway*/
    name = strncmp( buffer, "LP:", 3 ) ? buffer : buffer+3 ;

    titlestring = QString(name);
    codecstring = QString(bitrate->name);
    blocks = 0;
}

QNetMDTrack::~QNetMDTrack()
{
    devh = NULL;
    md = NULL;
}

unsigned int QNetMDTrack::tracknum() const
{
    /* returns zero based track number, maybe this function should return a one based track number as shown in the treeview,
     * trackindex -> zero based; tracknumber -> one based
     */
    return trkindex;
}

QString QNetMDTrack::group() const
{
    if(trkindex < 0)
        return QString();

    return groupstring;
}

QString QNetMDTrack::title() const
{
    if(trkindex < 0)
        return QString();

    return titlestring;
}

QString QNetMDTrack::codecname() const
{
    if(trkindex < 0)
        return QString();

    return codecstring;
}

QTime QNetMDTrack::duration() const
{
    QTime t(0,0,0);

    if(trkindex < 0)
        return QTime();

    return t.addSecs( time.minute * 60 + time.second);
}

bool QNetMDTrack::copyprotected() const
{
    switch(flags)
    {
       case 0x00 : return false;
       case 0x03 : return true;
       default : return true;   // return true if unknown
    }
}

void QNetMDTrack::setBlocks(int cnt)
{
    blocks = cnt;
}

int QNetMDTrack::blockcount() const
{
    return blocks;
}
