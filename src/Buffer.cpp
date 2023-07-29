#include <cassert>
#include <cstring>
#include "Buffer.h"

namespace rtp {

Buffer::Buffer(char* b, int nn) {
    p = bytes = b;
    nb_bytes = nn;
}

Buffer::~Buffer() {
}

Buffer* Buffer::copy() {
    Buffer* cp = new Buffer(bytes, nb_bytes);
    cp->p = p;
    return cp;
}

char* Buffer::data() {
    return bytes;
}

char* Buffer::head() {
    return p;
}

int Buffer::size() {
    return nb_bytes;
}

void Buffer::set_size(int v) {
    nb_bytes = v;
}

int Buffer::pos() {
    return (int)(p - bytes);
}

int Buffer::left() {
    return nb_bytes - (int)(p - bytes);
}

bool Buffer::empty() {
    return !bytes || (p >= bytes + nb_bytes);
}

bool Buffer::require(int required_size) {
    if (required_size < 0) {
        return false;
    }
    
    return required_size <= nb_bytes - (p - bytes);
}

void Buffer::skip(int size) {
    assert(p);
    assert(p + size >= bytes);
    assert(p + size <= bytes + nb_bytes);
    
    p += size;
}

int8_t Buffer::read_1bytes() {
    assert(require(1));
    
    return (int8_t)*p++;
}

int16_t Buffer::read_2bytes() {
    assert(require(2));
    
    int16_t value;
    char* pp = (char*)&value;
    pp[1] = *p++;
    pp[0] = *p++;
    
    return value;
}

int16_t Buffer::read_le2bytes() {
    assert(require(2));

    int16_t value;
    char* pp = (char*)&value;
    pp[0] = *p++;
    pp[1] = *p++;

    return value;
}

int32_t Buffer::read_3bytes() {
    assert(require(3));
    
    int32_t value = 0x00;
    char* pp = (char*)&value;
    pp[2] = *p++;
    pp[1] = *p++;
    pp[0] = *p++;
    
    return value;
}

int32_t Buffer::read_le3bytes() {
    assert(require(3));

    int32_t value = 0x00;
    char* pp = (char*)&value;
    pp[0] = *p++;
    pp[1] = *p++;
    pp[2] = *p++;

    return value;
}

int32_t Buffer::read_4bytes() {
    assert(require(4));
    
    int32_t value;
    char* pp = (char*)&value;
    pp[3] = *p++;
    pp[2] = *p++;
    pp[1] = *p++;
    pp[0] = *p++;
    
    return value;
}

int32_t Buffer::read_le4bytes() {
    assert(require(4));

    int32_t value;
    char* pp = (char*)&value;
    pp[0] = *p++;
    pp[1] = *p++;
    pp[2] = *p++;
    pp[3] = *p++;

    return value;
}

int64_t Buffer::read_8bytes() {
    assert(require(8));
    
    int64_t value;
    char* pp = (char*)&value;
    pp[7] = *p++;
    pp[6] = *p++;
    pp[5] = *p++;
    pp[4] = *p++;
    pp[3] = *p++;
    pp[2] = *p++;
    pp[1] = *p++;
    pp[0] = *p++;
    
    return value;
}

int64_t Buffer::read_le8bytes() {
    assert(require(8));

    int64_t value;
    char* pp = (char*)&value;
    pp[0] = *p++;
    pp[1] = *p++;
    pp[2] = *p++;
    pp[3] = *p++;
    pp[4] = *p++;
    pp[5] = *p++;
    pp[6] = *p++;
    pp[7] = *p++;

    return value;
}

std::string Buffer::read_string(int len) {
    assert(require(len));
    
    std::string value;
    value.append(p, len);
    
    p += len;
    
    return value;
}

void Buffer::read_bytes(char* data, int size) {
    assert(require(size));
    
    memcpy(data, p, size);
    
    p += size;
}

void Buffer::write_1bytes(int8_t value) {
    assert(require(1));
    
    *p++ = value;
}

void Buffer::write_2bytes(int16_t value) {
    assert(require(2));
    
    char* pp = (char*)&value;
    *p++ = pp[1];
    *p++ = pp[0];
}

void Buffer::write_le2bytes(int16_t value) {
    assert(require(2));

    char* pp = (char*)&value;
    *p++ = pp[0];
    *p++ = pp[1];
}

void Buffer::write_4bytes(int32_t value) {
    assert(require(4));
    
    char* pp = (char*)&value;
    *p++ = pp[3];
    *p++ = pp[2];
    *p++ = pp[1];
    *p++ = pp[0];
}

void Buffer::write_le4bytes(int32_t value) {
    assert(require(4));

    char* pp = (char*)&value;
    *p++ = pp[0];
    *p++ = pp[1];
    *p++ = pp[2];
    *p++ = pp[3];
}

void Buffer::write_3bytes(int32_t value) {
    assert(require(3));
    
    char* pp = (char*)&value;
    *p++ = pp[2];
    *p++ = pp[1];
    *p++ = pp[0];
}

void Buffer::write_le3bytes(int32_t value) {
    assert(require(3));

    char* pp = (char*)&value;
    *p++ = pp[0];
    *p++ = pp[1];
    *p++ = pp[2];
}

void Buffer::write_8bytes(int64_t value) {
    assert(require(8));
    
    char* pp = (char*)&value;
    *p++ = pp[7];
    *p++ = pp[6];
    *p++ = pp[5];
    *p++ = pp[4];
    *p++ = pp[3];
    *p++ = pp[2];
    *p++ = pp[1];
    *p++ = pp[0];
}

void Buffer::write_le8bytes(int64_t value) {
    assert(require(8));

    char* pp = (char*)&value;
    *p++ = pp[0];
    *p++ = pp[1];
    *p++ = pp[2];
    *p++ = pp[3];
    *p++ = pp[4];
    *p++ = pp[5];
    *p++ = pp[6];
    *p++ = pp[7];
}

void Buffer::write_string(std::string value) {
    assert(require((int)value.length()));
    
    memcpy(p, value.data(), value.length());
    p += value.length();
}

void Buffer::write_bytes(char* data, int size) {
    assert(require(size));
    
    memcpy(p, data, size);
    p += size;
}

BitBuffer::BitBuffer(Buffer* b) {
    cb = 0;
    cb_left = 0;
    stream = b;
}

BitBuffer::~BitBuffer() {
}

bool BitBuffer::empty() {
    if (cb_left) {
        return false;
    }
    return stream->empty();
}

int8_t BitBuffer::read_bit() {
    if (!cb_left) {
        assert(!stream->empty());
        cb = stream->read_1bytes();
        cb_left = 8;
    }
    
    int8_t v = (cb >> (cb_left - 1)) & 0x01;
    cb_left--;
    return v;
}

} //namespace rtp