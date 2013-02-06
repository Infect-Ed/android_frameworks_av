/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MPEG2_TS_EXTRACTOR_H_

#define MPEG2_TS_EXTRACTOR_H_

#include <media/stagefright/foundation/ABase.h>
#include <media/stagefright/MediaExtractor.h>
#include <utils/threads.h>
#include <utils/Vector.h>

namespace android {

struct AMessage;
struct AnotherPacketSource;
struct ATSParser;
struct DataSource;
struct MPEG2TSSource;
struct String8;
struct LiveSession;
struct TSBuffer;
//PES stream info
struct StreamInfo : public RefBase {
public:
    unsigned mStreamPID;
    unsigned mProgramPID;
    uint64_t mFirstPTS;
    uint64_t mLastPTS;
    int64_t  mDurationUs;
    off64_t  mFirstPTSOffset;
    off64_t  mLastPTSOffset;
    uint32_t mIndex;
    off64_t  mOffset;
};


struct MPEG2TSExtractor : public MediaExtractor {
    MPEG2TSExtractor(const sp<DataSource> &source);

    virtual size_t countTracks();
    virtual sp<MediaSource> getTrack(size_t index);
    virtual sp<MetaData> getTrackMetaData(size_t index, uint32_t flags);

    virtual sp<MetaData> getMetaData();

    virtual uint32_t flags() const;

    void setLiveSession(const sp<LiveSession> &liveSession);
    void seekTo(int64_t seekTimeUs);

    virtual ~MPEG2TSExtractor();

    status_t feedTSPacket(const void *data, size_t size);
    status_t parseTSToGetPID(const void *data, size_t size,
                                               unsigned& streamPID);
    status_t parseTSToGetPTS(const void *data, size_t size,
                                               unsigned streamPID, uint64_t& PTS);

    bool     isSeekable();
    void     signalSeekDiscontinuity(sp<StreamInfo> &stream);
private:

    struct SourceObjects : public RefBase {
    public:
        sp<AnotherPacketSource> impl;
        sp<StreamInfo> stream;
        bool isVideo;
    };

    friend struct MPEG2TSSource;

    mutable Mutex mLock;

    sp<DataSource> mDataSource;

    sp<ATSParser> mParser;

    Vector<sp<SourceObjects> > mSourceObjectsList;

    off64_t mOffset;

    void init();
    status_t feedMore();
    status_t findStreamDuration(const sp<StreamInfo> &stream,
            const sp<AnotherPacketSource> &impl);

    bool           mSeekable;
    off64_t        mClipSize;
    sp<TSBuffer>   mTSBuffer;

    DISALLOW_EVIL_CONSTRUCTORS(MPEG2TSExtractor);
};

bool SniffMPEG2TS(
        const sp<DataSource> &source, String8 *mimeType, float *confidence,
        sp<AMessage> *);

}  // namespace android

#endif  // MPEG2_TS_EXTRACTOR_H_
