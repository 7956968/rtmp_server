/*
MIT License

Copyright (c) 2016 ME_Kun_Han

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef RS_PROTOCOL_RTMP_H_
#define RS_PROTOCOL_RTMP_H_

#include "rs_common.h"
#include "rs_kernel_io.h"
#include "rs_kernel_buffer.h"
#include "rs_protocol_async_interface.h"
#include "uv.h"

class RtmpHandshakeC0C1 {
public:
    uint8_t version;
    uint32_t timestamp;
    uint32_t zero;
    std::string random_data;
public:
    RtmpHandshakeC0C1() = default;

    virtual ~RtmpHandshakeC0C1() = default;

public:
    int initialize();

    int initialize(std::string &buf);

    int initialize(RsBufferLittleEndian *buffer);

    std::string dump();
};

class RtmpHandshakeC2 {
public:
    uint32_t timestamp;
    uint32_t timestamp2;
    std::string random_data;
public:
    RtmpHandshakeC2() = default;

    virtual ~RtmpHandshakeC2() = default;

public:
    int initialize();

    int initialize(uint32_t ts, const std::string &rd);

    std::string dump();

};

class RtmpHandshakeS0S1 : public RtmpHandshakeC0C1 {
public:
    RtmpHandshakeS0S1() = default;

    virtual ~RtmpHandshakeS0S1() override = default;
};

class RtmpHandshakeS2 : public RtmpHandshakeC2 {
public:
    RtmpHandshakeS2() = default;

    virtual ~RtmpHandshakeS2() override = default;
};

class RtmpHandshakeAsync : public IRsAsyncMsg {
public:
    RtmpHandshakeAsync() = default;

    virtual ~RtmpHandshakeAsync() override = default;

public:
    int on_msg(std::vector<uint8_t> &buf) override;
};

class RsRtmpChunkMessage;

using RTMP_CHUNK_MESSAGES = std::vector<std::shared_ptr<RsRtmpChunkMessage>>;

class RsRtmpChunkMessage {
public:
    // chunk size
    uint32_t chunk_size;
    // basic header
    uint8_t fmt;
    uint32_t cs_id;
    // message header
    uint32_t timestamp;
    uint32_t message_length;
    uint8_t message_type_id;
    uint32_t message_stream_id;
    // type 1, 2
    uint32_t timestamp_delta;
    // extended timestamp
    uint32_t extended_timestamp;
    // chunk data
    std::string chunk_data;
public:
    RsRtmpChunkMessage() = default;

    ~RsRtmpChunkMessage() = default;

private:
    int type_0_decode(IRsReaderWriter *reader);

    int type_1_decode(IRsReaderWriter *reader);

    int type_2_decode(IRsReaderWriter *reader, uint32_t size);

    int type_3_decode(IRsReaderWriter *reader, uint32_t size);

    std::string basic_header_dump();

    int type_0_dump(std::string &buf);

    int type_1_dump(std::string &buf);

    int type_2_dump(std::string &buf);

    int type_3_dump(std::string &buf);

public:
    int initialize(IRsReaderWriter *reader, uint32_t cs, uint32_t payload_length = 0);

    int dump(std::string &buf);

public:
    static RTMP_CHUNK_MESSAGES
    create_chunk_messages(uint32_t ts, std::string msg, uint8_t msg_type,
                          uint32_t msg_stream_id, uint32_t cs);
};

class RsRtmpChunkHeaderAsync : public IRsAsyncMsg {
private:
    enum {
        rs_rtmp_chunk_header_uninitialized = 0,
        rs_rtmp_chunk_header_completed
    } _status;
    // chunk size
    uint32_t _chunk_size;
    // basic header
    uint8_t _fmt;
    uint32_t _cs_id;
    // message header
    uint32_t _timestamp;
    uint32_t _message_length;
    uint8_t _message_type_id;
    uint32_t _message_stream_id;
    // type 1, 2
    uint32_t _timestamp_delta;
    // extended timestamp
    uint32_t _extended_timestamp;
public:
    RsRtmpChunkHeaderAsync() : _status(rs_rtmp_chunk_header_uninitialized) {};

    virtual ~RsRtmpChunkHeaderAsync() override = default;

public:
    bool is_completed();

public:
    int on_msg(std::vector<uint8_t> &buf) override;
};

class RsRtmpChunkMsgAsync : public IRsAsyncMsg {
private:
    enum {
        rs_rtmp_chunk_message_uninitialized = 0,
        rs_rtmp_chunk_message_header_created,
        rs_rtmp_chunk_message_completed
    } _status;

    RsRtmpChunkHeaderAsync _header;

    // chunk data
    std::vector<uint8_t> _chunked_data;

    // for cache
    // std::vector<uint8_t> _cache_buffer;
    RsBufferLittleEndian _cache_buffer;
public:
    RsRtmpChunkMsgAsync() : _status(rs_rtmp_chunk_message_uninitialized) {};

    virtual ~RsRtmpChunkMsgAsync() override = default;

public:

    bool is_completed();

    void clear();

    int on_msg(std::vector<uint8_t> &buf) override;
};

#endif
