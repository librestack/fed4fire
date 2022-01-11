/* SPDX-License-Identifier: MIT
 *
 * lwmon packet handling: functions to assemble, send, receive and disassemble
 * packets containing monitoring data
 *
 * this file is part of LWMON
 *
 * Copyright (c) 2010-2022 Claudio Calvelli <clc@librecast.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __LWMON_PACKET_H__
#define __LWMON_PACKET_H__ 1

#include <stdint.h>

/* a packet contains some identification information and a sequence of
 * data items; each data item has a name, an optional parameter (a string)
 * and a number of integer or fixed-point values */

/* size of a packet when sent over UDP; this is small enough to be sent
 * without fragmentation on a 6in4 tunnel */
#define LWMON_PACKET 1280

/* maximum number of data items per packet */
#define LWMON_ITEMS 255

/* maximum number of values per data item */
#define LWMON_VALUES 255

/* maximum length of item name and parameter */
#define LWMON_NAMELEN 255

/* a packet can include one or more commands to recipient (who gets to decide
 * whether to run them); this is the current list */
typedef enum {
    lwmon_command_measure    = 0x0001,  /* re-run measurements */
    lwmon_command_reopen     = 0x0002,  /* re-open output files */
    lwmon_command_terminate  = 0x0004,  /* re-open output files */
    lwmon_command_none       = 0        /* default: do nothing */
} lwmon_command_t;

/* structure containing a packet in memory */
typedef struct {
    int length;
    int header_len;
    int data_count;
    lwmon_command_t command;
    int cmdptr;
    int itemptr[LWMON_ITEMS];
    unsigned char data[LWMON_PACKET];
} lwmon_packet_t;

/* structure containing packet identification information; this is
 * normally stored in the binary packet data */
typedef struct {
    time_t timestamp;
    const char * host;  /* not 0-terminated, see hostlen */
    int hostlen;
    int id;             /* id number helps detecting duplicates */
    int seq;            /* sequence number within a block of packets */
    int checksum;
} lwmon_packetid_t;

/* structure describing the type of a data item; the number identifies the
 * type within the binary packet data, and must be between 1 and 65533, with
 * 0 reserved for "unknown type", 65534 for logs and 65535 for commands */
typedef struct {
    int number;
    const char * name;
    const char * parmname;
    int min_values;
    int max_values;
    struct {
	const char * name;
	const char * zero;
	int scale;
    } values[LWMON_VALUES];
} lwmon_datatype_t;

/* array of known types, and a function to find the type by name */
extern const lwmon_datatype_t lwmon_datatypes[];
const lwmon_datatype_t * lwmon_datatype(const char *);

/* structure containing a single data item from a packet */
typedef struct {
    const char * name; /* not 0-terminated, see namelen */
    int namelen;
    const char * parm; /* not 0-terminated, see parmlen */
    int parmlen;
    const lwmon_datatype_t * type; /* NULL if unknown / generic */
    int n_values;
    int64_t values[LWMON_VALUES];
} lwmon_dataitem_t;

/* set default host name for packets - if this is not called, it will
 * default to the value returned by gethostname(); the data is copied
 * to a new buffer */
void lwmon_packet_hostname(const char *);

/* initialise packet and fill host name, timestamp and ID in */
void lwmon_initpacket(lwmon_packet_t *);

/* like initpacket, but leaves some fields unchanged to indicate that
 * the packets are part of the same transmission */
void lwmon_reinitpacket(lwmon_packet_t *);

/* receives packet from a socket - the packet does not need to be initialised
 * and any data already present will be discarded; return 1 for OK, 0 for end
 * of file/stream, and -1 for error (setting errno); as a special case, if
 * a duplicate packet is received, it returns -2 with errno set to ENOMSG;
 * the last 2 arguments are the same as the last 2 arguments to recvfrom */
int lwmon_receivepacket(int, lwmon_packet_t *, struct sockaddr *, socklen_t *);

/* reads packet from file, the packet does not need to be initialised and
 * any data already present will be discarded; return 1 for OK, 0 for end
 * of file and -1 for error (setting errno) */
int lwmon_readpacket(int, lwmon_packet_t *);

/* add data to a packet, if there is space; return 1 if OK, 0 if no space,
 * -1 if data can never fit in packet because it is too big */
int lwmon_add_data(lwmon_packet_t *, const lwmon_dataitem_t *);

/* add commands to a packet, if there is space: return 1 if OK, 0 if
 * no space; a command can always fit in a new packet, so unlike
 * lwmon_add_data() this function never returns -1 */
int lwmon_add_command(lwmon_packet_t *, lwmon_command_t);

/* add a log entry to a packet, if there is space: returns 1 if OK, 0 if
 * no space, and -1 if there can never be space in the packet because
 * the text won't fit */
int lwmon_add_log(lwmon_packet_t *, const char *, const char *);

/* get a data item from the packet; returns 1 if OK, 2 if the item is a log
 * item, 0 if the item is not present, -1 if there is an error decoding the
 * data (setting errno) */
int lwmon_get_data(const lwmon_packet_t *, int, lwmon_dataitem_t *);

/* close packet by filling in checksum and length; after this call, the
 * packet can be sent using lwmon_sendpacket */
void lwmon_closepacket(lwmon_packet_t *);

/* produces a human-readable representation of packet */
void lwmon_printpacket(FILE *, const lwmon_packet_t *, int);

/* calls a function for each value in a packet */
int lwmon_packetdata(const lwmon_packet_t *, void *,
		     int (*f)(const lwmon_packetid_t *,
			      const lwmon_dataitem_t *, int, void *));

/* send packet on a socket, in packed binary form; caller must first
 * call lwmon_closepacket(); return 0 if OK, -1 if error (setting errno) */
int lwmon_sendpacket(int, const lwmon_packet_t *);

/* get packet identification */
void lwmon_packetid(const lwmon_packet_t *, lwmon_packetid_t *);

#endif /* __LWMON_PACKET_H__ */
