// Last Update:2019-03-27 12:09:45
/**
 * @file ts_parse.c
 * @brief 
 * @author felix
 * @version 0.1.00
 * @date 2019-03-26
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "si.h"
#include "dump.h"
#include "ts_parse.h"

#define DUMP_PES 1

static inline int64_t ff_parse_pes_pts(const uint8_t *buf) {
    return (int64_t)(*buf & 0x0e) << 29 |
            (AV_RB16(buf+1) >> 1) << 15 |
             AV_RB16(buf+3) >> 1;
}

static int pat_parse( u8 *ts_data, u8 payload_unit_start_indicator, ts_info_t *ts )
{
    PAT_SECT_HEADER *pat = NULL;
    int i = 0;
    u16 section_length = 0;
    u16 program_number = 0;
    u16 program_map_pid = 0;
    u8 *save = ts_data;

    printf("[ PAT ]\n");
    if ( payload_unit_start_indicator ) {
        printf("\tpointer_field : %d\n", *ts_data++ );
    }
    pat = (PAT_SECT_HEADER *)ts_data;
    section_length = pat->section_length_hi << 8 | pat->section_length_lo;
    printf("\ttable_id : 0x%x\n", pat->table_id );
    printf("\tsection_length : %d\n", section_length );
    printf("\tcurrnet_next_indicator : %d\n", pat->current_next_indicator );
    printf("\ttransport_stream_id : %d\n", pat->transport_stream_id_hi << 8 | pat->transport_stream_id_lo );
    printf("\tsection_number : %d\n", pat->section_number );
    printf("\tlast_section_number : %d\n", pat->last_section_number );
    ts_data += sizeof( PAT_SECT_HEADER );
    for ( i=0; i<(section_length - 4 - 5)/4; i++ ) {// -4 : crc length /4 : program_number + programm_map_pid
        program_number = ts_data[0]<<8 | ts_data[1];
        printf("\t[ pmt%d ]\n", i );
        printf("\t\tprogram_number : 0x%x\n", program_number );
        ts_data += 2;
        program_map_pid = (ts_data[0] & 0x1f) << 8 | ts_data[1];
        printf("\t\tprogram_map_pid : 0x%x\n", program_map_pid );
        ts_data += 2;
        ts->pmt_pid = program_map_pid;
    }
    ts_data += 4;// crc length

    return (ts_data - save);
}

static int pmt_parse(u8 *ts_data, u8 payload_unit_start_indicator, ts_info_t *ts)
{
    PMT_SECT_HEADER *pmt = NULL;
    int i = 0;
    u16 section_length = 0;
    u16 program_number = 0;
    u16 program_map_pid = 0;
    u8 *save = ts_data;
    u8 *start = NULL;
    u16 program_info_length = 0;
    u16 es_loop_length = 0;
    u16 elementary_pid = 0;
    u16 es_info_length = 0;
    u8 stream_type = 0;

    printf("[ PMT ]\n");
    if ( payload_unit_start_indicator ) {
        printf("\tpointer_field : %d\n", *ts_data++ );
    }
    pmt = (PMT_SECT_HEADER *)ts_data;
    section_length = pmt->section_length_hi << 8 | pmt->section_length_lo;
    program_info_length = pmt->program_info_length_hi << 8 | pmt->program_info_length_lo;
    printf("\ttable_id : 0x%x\n", pmt->table_id );
    printf("\tsection_length : %d\n", section_length );
    printf("\tprogram_number : 0x%x\n", pmt->program_number_hi<<8 | pmt->program_number_lo );
    printf("\tcurrnet_next_indicator : %d\n", pmt->current_next_indicator );
    printf("\tsection_number : %d\n", pmt->section_number );
    printf("\tlast_section_number : %d\n", pmt->last_section_number );
    printf("\tPCR_PID : 0x%x\n", pmt->pcr_pid_hi << 8 | pmt->pcr_pid_lo );
    printf("\tprogram_info_length : %d\n", program_info_length );
    ts_data += sizeof( PMT_SECT_HEADER ) + program_info_length;
    start = ts_data;
    es_loop_length = section_length - 9 - 5 - program_info_length;
    while ( (ts_data - start) < es_loop_length ) {
        printf("\t[ es%d ]\n", i );
        stream_type = *ts_data++;
        printf("\t\tstream_type : 0x%x\n", stream_type );
        elementary_pid = (ts_data[0] & 0x1f)<<8 | ts_data[1];
        printf("\t\telementary_pid : 0x%x\n", elementary_pid);
        if ( stream_type == 0x1b ) {
            ts->video_pid = elementary_pid;
        } else if ( stream_type == 0x0f ) {
            ts->audio_pid = elementary_pid;
        } else {
            // TODO 
            // suuport more stream_type
            LOGE("unspported stream_type\n");
            exit(0);
        }
        ts_data += 2;
        es_info_length = (ts_data[0] & 0x0f)<<8 | ts_data[1];
        printf("\t\tes_info_length : %d\n", es_info_length);
        ts_data += 2 + es_info_length;
        i++;
    }
    ts_data += 4;// crc length

    return (ts_data - save);
}

static int pes_parse(u8 *ts_data, int len, u16 pid, u8 payload_unit_start_indicator, ts_info_t *ts)
{
    u8 *save = ts_data;
    static int pes_cnt = 0;
    static int audio_cnt = 0, video_cnt = 0;
    static u16 pes_packet_length = 0;
    optional_pes_header_t *opt_pes_hdr = NULL;
    int64_t pts = 0;
    static int pes_first_pkt_len = 0;
    static char *video_ptr = NULL, *video_ptr_save = NULL;
    static char *audio_ptr = NULL, *audio_ptr_save = NULL;
    static int pes_header_len = 0;

#define VIDEO_BUF_SIZE 65536
#define AUDIO_BUF_SIZE 65536

    if ( !video_ptr ) {
        video_ptr = (char *)malloc( VIDEO_BUF_SIZE );
        if ( !video_ptr )
            return -1;
        memset( video_ptr, 0, VIDEO_BUF_SIZE);
        video_ptr_save = video_ptr;
    }

    if ( !audio_ptr ) {
        audio_ptr = (char *)malloc( AUDIO_BUF_SIZE );
        if ( !audio_ptr ) 
            return -1;
        memset( audio_ptr, 0, AUDIO_BUF_SIZE );
        audio_ptr_save = audio_ptr;
    }

//    DUMPBUF( ts_data, 184 );
    if ( payload_unit_start_indicator ) {
        if ( ts_data[0] != 0x00 
             || ts_data[1] != 0x00 
             || ts_data[2] != 0x01 ) {
            LOGE("check pes start code prefix fail\n");
            exit(0);
        }

        if ( (video_ptr - video_ptr_save) > 0 ) {
            if ( video_ptr - video_ptr_save != (pes_packet_length - (pes_header_len))) {
                LOGE("check pes total size error, pes_packet_length = %d, pes_header_len = %d,"
                     "video_ptr - video_ptr_save = %d\n", pes_packet_length, pes_header_len, video_ptr - video_ptr_save);
                exit(0);
            }
            if ( video_ptr_save[0] != 0x00 
                 || video_ptr_save[1] != 0x00
                 || video_ptr_save[2] != 0x00
                 || video_ptr_save[3] != 0x01 ) {
                LOGE("check start code of video pes error\n");
                exit(0);
            }
            PushAVData( video_ptr_save, video_ptr - video_ptr_save, 1, pts, 0, video_ptr_save[4]&0x1f == 0x05 );
            video_ptr = video_ptr_save;
        }

        if ( (audio_ptr - audio_ptr_save) > 0 ) {
            if ( audio_ptr - audio_ptr_save != (pes_packet_length - pes_header_len)) {
                LOGE("check pes total size error, pes_packet_length = %d, pes_header_len = %d,"
                     "audio_ptr - audio_ptr_save = %d\n", pes_packet_length, pes_header_len, audio_ptr - audio_ptr_save );
                exit(0);
            }
            PushAVData( audio_ptr_save, audio_ptr - audio_ptr_save, 0, pts, 0, 0 );
            audio_ptr = audio_ptr_save;
        }
        
        ts_data += 3;
        printf("\t[ pes%03d ]\n", pes_cnt++ );
        printf("\t\tstream_id : 0x%x\n", *ts_data++ );
        pes_packet_length = ts_data[0]<<8 | ts_data[1];
        printf("\t\tpes_packet_length : %d\n", pes_packet_length );
        ts_data += 2;
        // TODO 
        // judge stream_id whether parse optional_pes_header
        opt_pes_hdr = (optional_pes_header_t *)ts_data;
        printf("\t\tPTS_DTS_flag : %d\n", opt_pes_hdr->PTS_DTS_flag );
        printf("\t\tPES_header_data_length : %d\n", opt_pes_hdr->PES_header_data_length );
        ts_data += sizeof(optional_pes_header_t);
        if ( opt_pes_hdr->PTS_DTS_flag ) {
            pts = ff_parse_pes_pts( ts_data );
            printf("\t\tpts : %ld\n", pts );
        }
        ts_data += opt_pes_hdr->PES_header_data_length;
        pes_header_len = ts_data - save -4 -2;// 4 : pkt start code prefix + stream_id 2 : pes_packet_length
        pes_first_pkt_len = len - (ts_data - save );
        if ( pid == ts->video_pid ) {
            memcpy( video_ptr, ts_data, pes_first_pkt_len );
            video_ptr += pes_first_pkt_len;
        } else if ( pid == ts->audio_pid ) {
            memcpy( audio_ptr, ts_data, pes_first_pkt_len );
            audio_ptr += pes_first_pkt_len;
        }
    } else {
#if DUMP_PES
        printf("\t[ pes%03d ]\n", pes_cnt++ );
        printf("\t\tlength : %d\n", len );
#endif
        if ( pid == ts->video_pid ) {
#if DUMP_PES
            printf("\t\tvideo count : %03d\n", video_cnt);
            printf("\t\ttype : video(h264)\n" );
#endif
            memcpy( video_ptr, ts_data, len );
            video_ptr += len;
            video_cnt++;
        } else if ( pid == ts->audio_pid ) {
#if DUMP_PES
            printf("\t\taudio count : %03d\n", audio_cnt);
            printf("\t\ttype : audio(aac)\n" );
#endif
            memcpy( audio_ptr, ts_data, len );
            audio_ptr += len;
            audio_cnt++;
        } else {
            LOGE("check stream type error\n");
            exit(0);
        }
    }

    return 0;
}

static void show_ts_packet_header( u16 pkt_idx,
                            u16 pid,
                            u8 payload_unit_start_indicator,
                            u8 adaptation_field_control,
                            u8 continuity_counter )
{
    printf("=========================================\n");
    printf("packet index : %d\n", pkt_idx );
    if ( pid == 0 ) {
        printf("pid : PAT\n");
    } else {
        printf("pid : 0x%x\n", pid );
    }
    printf("payload_unit_start_indicator : %d\n", payload_unit_start_indicator );
    printf("adaptation_field_control : %d\n", adaptation_field_control );
    printf("continuity_counter : %d\n", continuity_counter );
}

static int adp_field_parse( u8 *ts_data )
{
    adp_field_t *adp = (adp_field_t *)ts_data;

    if (  !adp ) {
        return -1;
    }

    if ( !adp->adaption_field_length ) {
        DUMPBUF( ts_data, 16 );
        LOGE("check adaption_field_length fail\n");
        exit(0);
    }

    printf("    [ adaption field ]\n");
    printf("        adaption_field_length : %d\n", adp->adaption_field_length );
    printf("        PCR_flag : %d\n", adp->PCR_flag );

    return (adp->adaption_field_length + 1);
}

int ts_parse( u8 *ts_data, u32 packet_nr )
{
    u32       i;
    u16       pid;
    u8*       p;
    u8        found_pat = 0;
    u8        payload_unit_start_indicator = 0;
    u8        adaptation_field_control = 0;
    u8        continuity_counter = 0;
    u32       ret = 0;
    static    ts_info_t *ts = NULL; 
    u8        *save = NULL;
    
    ts = (ts_info_t *)malloc( sizeof(ts_info_t) );
    if(!ts_data || !packet_nr || !ts )
        return 0;

    memset( ts, 0, sizeof(ts_info_t) );
    
    for( i = 0; i < packet_nr; i ++){
        p = ts_data + i * 188;// every time pointer to next ts packet
        save = p;
        //DUMPBUF( p, 188 );
        /* ts packet header : 4bytes */
        pid = ((PACKET_HEADER*)p)->pid_hi * 256 + ((PACKET_HEADER*)p)->pid_lo;
        payload_unit_start_indicator = packet_payload_unit_start_indicator(p);
        adaptation_field_control = packet_adaptation_field_control(p);
        continuity_counter = packet_continuity_counter(p);
#if !DUMP_PES
        if ( payload_unit_start_indicator || ((ts->video_pid && (ts->video_pid != pid)) 
             && (ts->audio_pid && (ts->audio_pid != pid) ))) {
#endif
            show_ts_packet_header( i, pid, payload_unit_start_indicator, adaptation_field_control, continuity_counter );
#if !DUMP_PES
        }
#endif
        p += 4;// ts packet header : 4bytes
        /* adaption field */
        if ( adaptation_field_control == 2 || adaptation_field_control == 3 ) {
            ret = adp_field_parse( p );
            if ( ret < 0 ) {
                LOGE("check adaption_field_length error\n");
                exit(0);
            }
            p += ret;
        }

        /* payload */
        if ( adaptation_field_control == 1 || adaptation_field_control == 3 ) {
            if ( pid == 0 ) {// pat
                ret = pat_parse( p, payload_unit_start_indicator, ts );
                if ( ret < 0 ) {
                    LOGE("parse pat error\n");
                    exit(0);
                }
                p += ret;
            } else {
                if ( pid == ts->pmt_pid ) {
                    ret = pmt_parse( p, payload_unit_start_indicator, ts );
                    if ( ret < 0 ) {
                        LOGE("parse pmt error\n");
                        exit(0);
                    }
                    p += ret;
                } else if ( pid == ts->video_pid || pid == ts->audio_pid ) {
                   ret = pes_parse( p, 188 - (p-save), pid, payload_unit_start_indicator, ts );
                    if ( ret < 0 ) {
                        LOGE("parse pet error\n");
                        exit(0);
                    }
                    p += ret;
                } else {
                    LOGE("unknow pid : 0x%x\n", pid );
                    exit(0);
                }
            }
        }
    }
    return 0;
}

