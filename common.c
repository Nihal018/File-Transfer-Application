#include "common.h"

void packi16(uint8_t *buf, int *idx, uint16_t val)
{
    buf[(*idx)++] = (uint8_t)((val >> 8)&0xFF);
    buf[(*idx)++] = (uint8_t)(val&0xFF);
}
void packi32(uint8_t *buf, int *idx, uint32_t val)
{
    buf[(*idx)++] = (uint8_t)((val >> 24)&0xFF);
    buf[(*idx)++] = (uint8_t)((val >> 16)&0xFF);
    buf[(*idx)++] = (uint8_t)((val >> 8)&0xFF);
    buf[(*idx)++] = (uint8_t)(val&0xFF);
}

uint16_t unpacku16(uint8_t *buf, int *idx)
{
    uint16_t val = 0; 
    val |= (uint16_t)buf[(*idx)++] << 8;
    val |= (uint16_t)buf[(*idx)++];
    return val;
}
uint32_t unpacku32(uint8_t *buf, int *idx)
{
    uint32_t val = 0; 
    val |= (uint32_t)buf[(*idx)++] << 24;
    val |= (uint32_t)buf[(*idx)++] << 16;
    val |= (uint16_t)buf[(*idx)++] << 8;
    val |= (uint16_t)buf[(*idx)++];
    return val;
}

int pack_datagram(struct datagram *dg, uint8_t buf[])
{
    int i = 0;
    packi32(buf, &i, dg->seq_no);
    buf[i++] = dg->ttl;
    packi16(buf, &i, dg->payload_len);
    memcpy(&buf[i], dg->payload, dg->payload_len);
    i += dg->payload_len;
    return i;
}

void unpack_datagram(uint8_t buf[], struct datagram *dg)
{
    int i = 0;
    dg->seq_no = unpacku32(buf, &i);
    dg->ttl = buf[i++];
    dg->payload_len= unpacku16(buf, &i);
    memset(dg->payload, 0, MAX_PAYLOAD_LEN);
    // length can be wrong
    if (dg->payload_len > MAX_PAYLOAD_LEN )
        memcpy(dg->payload, &buf[i], MAX_PAYLOAD_LEN);
    else
        memcpy(dg->payload, &buf[i], dg->payload_len);

}


void init_datagram(struct datagram *dgram, uint16_t seq_no, uint8_t ttl, int payload_len)
{
    dgram->seq_no = seq_no;
    dgram->ttl = ttl;
    dgram->payload_len = payload_len;
    memset(dgram->payload, 0, payload_len);
}

