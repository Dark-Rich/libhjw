#include "serialize.hpp"

void omq::serialize(int16_t in, unsigned char * out) {
    omq::serialize(uint16_t(in), out);
}

void omq::serialize(uint16_t in, unsigned char * out) {
    out[0] = (unsigned char)((in >> 8) & 0xff);
    out[1] = (unsigned char)(in & 0xff);
}

void omq::serialize(int32_t in, unsigned char * out) {
    omq::serialize(uint32_t(in), out);
}

void omq::serialize(uint32_t in, unsigned char * out) {
    out[0] = (unsigned char)((in >> 24) & 0xff);
    out[1] = (unsigned char)((in >> 16) & 0xff);
    out[2] = (unsigned char)((in >> 8) & 0xff);
    out[3] = (unsigned char)(in & 0xff);
}

void omq::serialize(int64_t in, unsigned char * out) {
    omq::serialize(uint64_t(in), out);
}

void omq::serialize(uint64_t in, unsigned char * out) {
    out[0] = (unsigned char)((in >> 56) & 0xff);
    out[1] = (unsigned char)((in >> 48) & 0xff);
    out[2] = (unsigned char)((in >> 40) & 0xff);
    out[3] = (unsigned char)((in >> 32) & 0xff);
    out[4] = (unsigned char)((in >> 24) & 0xff);
    out[5] = (unsigned char)((in >> 16) & 0xff);
    out[6] = (unsigned char)((in >> 8) & 0xff);
    out[7] = (unsigned char)(in & 0xff);
}

void omq::unserialize(int16_t * out, const unsigned char * in) {
    omq::unserialize((uint16_t *)out, in);
}

void omq::unserialize(uint16_t * out, const unsigned char * in) {
    *out = ((uint16_t)in[0] << 8) | ((uint16_t)in[1]);
}

void omq::unserialize(int32_t * out, const unsigned char * in) {
    omq::unserialize((uint32_t *)out, in);
}

void omq::unserialize(uint32_t * out, const unsigned char * in) {
    *out = ((uint32_t)in[0] << 24) | ((uint32_t)in[1] << 16)
        | ((uint32_t)in[2] << 8) | ((uint32_t)in[3]);
}

void omq::unserialize(int64_t * out, const unsigned char * in) {
    omq::unserialize((uint64_t *)out, in);
}

void omq::unserialize(uint64_t * out, const unsigned char * in) {
    *out = ((uint64_t)in[0] << 56) | ((uint64_t)in[1] << 48)
        | ((uint64_t)in[2] << 40) | ((uint64_t)in[3] << 32)
        | ((uint64_t)in[4] << 24) | ((uint64_t)in[5] << 16)
        | ((uint64_t)in[6] << 8) | ((uint64_t)in[7]);
}
