// Last Update:2019-03-25 18:39:41
/**
 * @file dump.h
 * @brief 
 * @author felix
 * @version 0.1.00
 * @date 2019-03-25
 */
// Last Update:2019-03-14 10:01:55
/**
 * @file flv-dump.h
 * @brief 
 * @author felix
 * @version 0.1.00
 * @date 2019-03-09
 */

#ifndef FLV_DUMP_H
#define FLV_DUMP_H

#define RED                  "\e[0;31m"
#define NONE                 "\e[0m"
#define GREEN                "\e[0;32m"

#define BASIC() printf("[ %s %s() +%d ] ", __FILE__, __FUNCTION__, __LINE__ )
#define LOGI(args...) BASIC();printf(args)
#define LOGE(args...) printf(RED"[ %s %s() +%d ] ", __FILE__, __FUNCTION__, __LINE__ );printf(args);printf(NONE)
#define LOGD(args...) LOGI(args) 
#define LOGW(args...) LOGI(args) 
#define LOGT(args...) printf(GREEN"[ %s %s() +%d ] ", __FILE__, __FUNCTION__, __LINE__ );printf(args);printf(NONE)

#define DUMPBUF( buf, size ) DbgDumpBuf( __LINE__, __FUNCTION__, buf, size ) 

#define NALU_TYPE_SLICE     1
#define NALU_TYPE_DPA       2
#define NALU_TYPE_DPB       3
#define NALU_TYPE_DPC       4
#define NALU_TYPE_IDR       5
#define NALU_TYPE_SEI       6
#define NALU_TYPE_SPS       7
#define NALU_TYPE_PPS       8
#define NALU_TYPE_AUD       9
#define NALU_TYPE_EOSEQ     10
#define NALU_TYPE_EOSTREAM  11
#define NALU_TYPE_FILL      12


#define ASSERT( cond ) if ( !(cond) ) { \
    LOGE("assert "#cond" failed\n"); \
    exit(1);\
}

#   define AV_RB32(x)                           \
    ((((const uint8_t*)(x))[0] << 24) |         \
     (((const uint8_t*)(x))[1] << 16) |         \
     (((const uint8_t*)(x))[2] <<  8) |         \
     ((const uint8_t*)(x))[3])

#   define AV_RB16(x)                           \
    ((((const uint8_t*)(x))[0] << 8) |         \
     (((const uint8_t*)(x))[1] ))

void PushAVData( unsigned char *buf, int buf_size, int isvideo, int64_t timestamp, int composition_time, char iskey  );
void PushFinish();
void DbgDumpBuf( int line, const char *func, unsigned char *buf, int size );
void DumpNalUnit( uint8_t *data, int datasize );

#endif  /*FLV-DUMP_H*/

