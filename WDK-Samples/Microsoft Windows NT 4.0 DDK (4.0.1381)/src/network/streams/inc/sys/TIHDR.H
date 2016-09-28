/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    tihdr.h

Abstract:

    Reverse Engineered Header for TPI.

Author:

    sampa

Revision History:

--*/

#ifndef SYS_TIHDR_INCLUDED
#define SYS_TIHDR_INCLUDED


//
// Primitive Types
//

#define T_CONN_REQ      0
#define T_CONN_RES      1
#define T_DISCON_REQ    2
#define T_DATA_REQ      3
#define T_EXDATA_REQ    4
#define T_INFO_REQ      5
#define T_BIND_REQ      6
#define T_UNBIND_REQ    7
#define T_UNITDATA_REQ  8
#define T_OPTMGMT_REQ   9
#define T_ORDREL_REQ    10

#define T_CONN_IND      11
#define T_CONN_CON      12
#define T_DISCON_IND    13
#define T_DATA_IND      14
#define T_EXDATA_IND    15
#define T_INFO_ACK      16
#define T_BIND_ACK      17
#define T_ERROR_ACK     18
#define T_OK_ACK        19
#define T_UNITDATA_IND  20
#define T_UDERROR_IND   21
#define T_OPTMGMT_ACK   22
#define T_ORDREL_IND    23
#define T_ASTAT_REQ     24
#define T_ASTAT_RES     25
#define T_SETSHDPUT_REQ 26
#define T_CONN_RES_ACK  27
#define T_CONN_RES_NACK 28


//
// Initialization Events
//

#define TE_BIND_REQ     0
#define TE_UNBIND_REQ   1
#define TE_OPTMGMT_REQ  2
#define TE_BIND_ACK     3
#define TE_OPTMGMT_ACK  4
#define TE_ERROR_ACK    5
#define TE_OK_ACK1      6
#define TE_OK_ACK2      7
#define TE_OK_ACK3      8
#define TE_OK_ACK4      9

//
// Connection Oriented Events
//

#define TE_CONN_REQ     10
#define TE_CONN_RES     11
#define TE_DISCON_REQ   12
#define TE_DATA_REQ     13
#define TE_EXDATA_REQ   14
#define TE_ORDREL_REQ   15
#define TE_CONN_IND     16
#define TE_CONN_CON     17
#define TE_DATA_IND     18
#define TE_EXDATA_IND   19
#define TE_ORDREL_IND   20
#define TE_DISCON_IND1  21
#define TE_DISCON_IND2  22
#define TE_DISCON_IND3  23
#define TE_PASS_CONN    24

//
// Connectionless events
//

#define TE_UNITDATA_REQ 25
#define TE_UNITDATA_IND 26
#define TE_UDERROR_IND  27

#define TE_NOEVENTS     28

//
// States
//

#define TS_UNBND        0
#define TS_WACK_BREQ    1
#define TS_WACK_UREQ    2
#define TS_IDLE         3
#define TS_WACK_OPTREQ  4
#define TS_WACK_CREQ    5
#define TS_WCON_CREQ    6
#define TS_WRES_CIND    7
#define TS_WACK_CRES    8
#define TS_DATA_XFER    9
#define TS_WIND_ORDREL  10
#define TS_WREQ_ORDREL  11
#define TS_WACK_DREQ6   12
#define TS_WACK_DREQ7   13
#define TS_WACK_DREQ9   14
#define TS_WACK_DREQ10  15
#define TS_WACK_DREQ11  16

#define TS_NOSTATES     17


//
// Structure Definitions for M_PROTO Messages
//

struct T_unitdata_req {
        long    PRIM_type;                      // T_UNITDATA_REQ
        long    DEST_length;
        long    DEST_offset;
        long    OPT_length;
        long    OPT_offset;
};

struct T_uderror_ind {
        long    PRIM_type;                      // T_UDERROR_IND
        long    DEST_length;
        long    DEST_offset;
        long    OPT_length;
        long    OPT_offset;
        long    ERROR_type;
};

struct T_unitdata_ind {
        long    PRIM_type;                      // T_UNITDATA_IND
        long    SRC_length;
        long    SRC_offset;
        long    OPT_length;
        long    OPT_offset;
};

struct T_optmgmt_ack {
        long    PRIM_type;                      // T_OPTMGMT_ACK
        long    MGMT_flags;
        long    OPT_length;
        long    OPT_offset;
};

struct T_discon_ind {
        long    PRIM_type;                      // T_DISCON_IND
        long    DISCON_reason;                  //   this is an NTSTATUS
        long    SEQ_number;
};

struct T_ordrel_ind {
        long    PRIM_type;                      // T_ORDREL_IND
};

struct T_conn_con {
        long    PRIM_type;                      // T_CONN_CON
        long    RES_length;
        long    RES_offset;
        long    OPT_length;
        long    OPT_offset;
};

struct T_conn_ind {
        long    PRIM_type;                      // T_CONN_IND
        long    SRC_length;
        long    SRC_offset;
        long    OPT_length;
        long    OPT_offset;
        long    SEQ_number;
};

struct T_data_ind {
        long    PRIM_type;                      // T_DATA_IND
        long    MORE_flag;
        long    reserved;
};

struct T_exdata_ind {
        long    PRIM_type;                      // T_EXDATA_IND
        long    MORE_flag;
        long    reserved;
};

struct T_info_req {
        long    PRIM_type;                      // T_INFO_REQ
};

struct T_bind_req {
        long    PRIM_type;                      // T_BIND_REQ
        long    ADDR_length;
        long    ADDR_offset;
        long    CONIND_number;
};

struct T_conn_req {
        long    PRIM_type;                      // T_CONN_REQ
        long    DEST_length;
        long    DEST_offset;
        long    OPT_length;
        long    OPT_offset;
};

struct T_conn_res {
        long    PRIM_type;                      // T_CONN_RES
        queue_t *QUEUE_ptr;
        long    SEQ_number;
        long    OPT_length;
        long    OPT_offset;
};

struct T_conn_res_ack {
        long    PRIM_type;                      // T_CONN_RES_ACK
        long    SEQ_number;
};

struct T_conn_res_nack {
        long    PRIM_type;                      // T_CONN_RES_NACK
        long    SEQ_number;
        long    TLI_error;                      // TNTERROR signifies that
        long    UNIX_error;                     //   this is an NTSTATUS
};

struct T_data_req {
        long    PRIM_type;                      // T_DATA_REQ
        long    MORE_flag;
        long    SEND_flags;
};

struct T_exdata_req {
        long    PRIM_type;                      // T_EXDATA_REQ
        long    MORE_flag;
        long    SEND_flags;
};

struct T_discon_req {
        long    PRIM_type;                      // T_DISCON_REQ
        long    SEQ_number;
};

struct T_ordrel_req {
        long    PRIM_type;                      // T_ORDREL_REQ
};

struct T_optmgmt_req {
        long    PRIM_type;                      // T_OPTMGMT_REQ
        long    MGMT_flags;
        long    OPT_length;
        long    OPT_offset;
};

struct T_setshdput_req {
        long    PRIM_type;                      // T_SETSHDPUT_REQ
        int     *SHDPUT_func;
};

struct T_info_ack {
        long    PRIM_type;                      // T_INFO_ACK
        long    TSDU_size;
        long    ETSDU_size;
        long    CDATA_size;
        long    DDATA_size;
        long    ADDR_size;
        long    OPT_size;
        long    TIDU_size;
        long    SERV_type;
        long    CURRENT_state;
                                                // Microsoft Extensions:
        long    ADDR_length;                    // current transport address
        long    ADDR_offset;                    //   of this stream
        long    ThroughPut;                     // ThroughPut in Kbytes/second
                                                // 0 if not available
        long    Delay;                          // Delay on sending a packet
                                                // 0 if not available
        long    Reliable;                       // 1 if the connection is
                                                // reliable.
                                                // 0 if not.
        unsigned char UNIQUE_addr[6];           // unique address for this host
};

struct T_bind_ack {
        long    PRIM_type;                      // T_BIND_ACK
        long    ADDR_length;
        long    ADDR_offset;
        long    CONIND_number;
};

struct T_error_ack {
        long    PRIM_type;                      // T_ERROR_ACK
        long    ERROR_prim;
        long    TLI_error;                      // TNTERROR signifies that
        long    UNIX_error;                     //   this is an NTSTATUS
};

struct T_ok_ack {
        long    PRIM_type;                      // T_OK_ACK
        long    CORRECT_prim;
};

struct T_unbind_req {
        long    PRIM_type;                      // T_UNBIND_REQ
};

struct T_astat_req {
        long    PRIM_type;                      // T_ASTAT_REQ
        long    ADDR_length;
        long    ADDR_offset;
};

struct T_astat_res {
        long    PRIM_type;                      // T_ASTAT_RES
};

union T_primitives {
        long                    type;
        struct T_unitdata_req   t_unitdata_req;
        struct T_uderror_ind    t_uderror_ind;
        struct T_unitdata_ind   t_unitdata_ind;
        struct T_optmgmt_ack    t_optmgmt_ack;
        struct T_discon_ind       t_discon_ind;
        struct T_ordrel_ind     t_ordrel_ind;
        struct T_conn_con       t_conn_con;
        struct T_conn_ind       t_conn_ind;
        struct T_data_ind       t_data_ind;
        struct T_exdata_ind     t_exdata_ind;
        struct T_info_req       t_info_req;
        struct T_bind_req       t_bind_req;
        struct T_conn_req       t_conn_req;
        struct T_conn_res       t_conn_res;
        struct T_conn_res_ack   t_conn_res_ack;
        struct T_conn_res_nack  t_conn_res_nack;
        struct T_data_req       t_data_req;
        struct T_exdata_req     t_exdata_req;
        struct T_discon_req     t_discon_req;
        struct T_ordrel_req     t_ordrel_req;
        struct T_optmgmt_req    t_optmgmt_req;
        struct T_info_ack       t_info_ack;
        struct T_bind_ack       t_bind_ack;
        struct T_error_ack      t_error_ack;
        struct T_ok_ack         t_ok_ack;
        struct T_unbind_req     t_unbind_req;
};


#define unitdata_req            t_unitdata_req
#define uderror_ind             t_uderror_ind
#define unitdata_ind            t_unitdata_ind
#define optmgm_ack              t_optmgmt_ack
#define discon_ind              t_discon_ind
#define ordrel_ind              t_ordrel_ind
#define conn_con                t_conn_con
#define conn_ind                t_conn_ind
#define data_ind                t_data_ind
#define exdata_ind              t_exdata_ind
#define info_req                t_info_req
#define bind_req                t_bind_req
#define conn_req                t_conn_req
#define conn_res                t_conn_res
#define data_req                t_data_req
#define exdata_req              t_exdata_req
#define discon_req              t_discon_req
#define ordrel_req              t_ordrel_req
#define optmgm_req              t_optmgmt_req
#define info_ack                t_info_ack
#define bind_ack                t_bind_ack
#define error_ack               t_error_ack
#define ok_ack                  t_ok_ack
#define unbind_req              t_unbind_req



#endif
