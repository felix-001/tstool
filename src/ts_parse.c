// Last Update:2019-03-26 21:18:20
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
#include "si.h"
#include "dump.h"
#include "ts_parse.h"

static int pat_parse( u8 *ts_data, u8 payload_unit_start_indicator, ts_info_t *ts )
{
    PAT_SECT_HEADER *pat = (PAT_SECT_HEADER *)ts_data;

    printf("\t[ PAT ]\n");
    printf("\t\ttable_id : %d\n", pat->table_id );
    printf("\t\tsection_length : %d\n", pat->section_length_hi << 8 | pat->section_length_lo );
    printf("\t\tcurrnet_next_indicator : %d\n", pat->current_next_indicator );
    printf("\t\ttransport_stream_id : %d\n", pat->transport_stream_id_hi << pat->transport_stream_id_lo );
    printf("\t\tsection_number : %d\n", pat->section_number );
    printf("\t\tlast_section_number : %d\n", pat->last_section_number );
    return 0;
}

static int pmt_parse()
{
    return 0;
}

static int pes_parse()
{
    return 0;
}

static void show_ts_packet_header( u16 pkt_idx,
                            u16 pid,
                            u8 payload_unit_start_indicator,
                            u8 adaptation_field_control,
                            u8 continuity_counter )
{
    printf("===============================\n");
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

    return adp->adaption_field_length;
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
    
    ts = (ts_info_t *)malloc( sizeof(ts_info_t) );
    if(!ts_data || !packet_nr || !ts )
        return 0;

    memset( ts, 0, sizeof(ts_info_t) );
    
    for( i = 0; i < packet_nr; i ++){
        p = ts_data + i * 188;// every time pointer to next ts packet
        /* ts packet header : 4bytes */
        pid = ((PACKET_HEADER*)p)->pid_hi * 256 + ((PACKET_HEADER*)p)->pid_lo;
        payload_unit_start_indicator = packet_payload_unit_start_indicator(p);
        adaptation_field_control = packet_adaptation_field_control(p);
        continuity_counter = packet_continuity_counter(p);
        show_ts_packet_header( i, pid, payload_unit_start_indicator, adaptation_field_control, continuity_counter );
        /* adaption field */
        if ( adaptation_field_control == 2 || adaptation_field_control == 3 ) {
            ret = adp_field_parse( p + 4 );// ts packet header : 4bytes
            if ( ret < 0 ) {
                LOGE("check adaption_field_length error\n");
                exit(0);
            }
            p += ret;
        }

        /* payload */
        if ( adaptation_field_control == 1 || adaptation_field_control == 3 ) {
            if ( pid == 0 ) {// pat
                pat_parse( p, payload_unit_start_indicator, ts );
            } else {
                if ( pid == ts->pmt_pid ) {
                    pmt_parse();
                } else if ( pid == ts->video_pid || pid == ts->audio_pid ) {
                    pes_parse();
                } else {
                    LOGE("unknow pid : 0x%x\n", pid );
                    exit(0);
                }
            }
        }
    }
    return 0;
}
