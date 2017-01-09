/****************************************************************************
 *    Copyright 2010 Andy Kirkham, Stellar Technologies Ltd
 *    
 *    This file is part of the Satellite Observers Workbench (SOWB).
 *
 *    SOWB is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    SOWB is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SOWB.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    $Id: main.cpp 5 2010-07-12 20:51:11Z ajk $
 *    
 ***************************************************************************/

#ifdef NEVERCOMPILETHIS

#include "mbed.h"
#include "nexstar.h"
#include "utils.h"
#include "debug.h"
#include "main.h"

DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);

/* The main place holder data structure for the Nexstar. */
NEXSTAR_DATA nexstar;

/* A timer to look for serial comms failure. */
OneMS   timeout;

/** nexstar_process(void)
 *
 * Standard module _process function.
 */
void nexstar_process(void) {
    int i, j, q;
    signed char status, type;
    
    BLAT4;
    
    /* Multiple queued requests will start automatically after the previous
       request (see the ISR for details). However, it is possible that when
       the ISR detects the completion of a request there are, at that time,
       no more pending requests in the queue. So loop over the request queue 
       and if none are in progress, initiate a request transfer to the Nexstar. */
    if (! nexstar_request_count_status(NEXSTAR_REQUEST_IN_PROGRESS)) {
        if (nexstar_request_count_status(NEXSTAR_REQUEST_PENDING)) {
            for (i = 0, q = nexstar.currentRequest; i < NEXSTAR_NUM_OF_PACKETS; i++) {
                if (nexstar.commsPackets[q].requestStatus == NEXSTAR_REQUEST_PENDING) {
                    nexstar.currentRequest = q;
                    nexstar_send_request(q);
                    i = NEXSTAR_NUM_OF_PACKETS;
                }
                else {
                    NEXSTAR_NEXT_REQUEST(q);
                }
            }
        }
    }
    
    /* look through all the comms request packets and see if any need processing. */
    /*
    for (i = 0; i < NEXSTAR_NUM_OF_PACKETS; i++) {
        status = nexstar.commsPackets[i].requestStatus;
        type   = nexstar.commsPackets[i].requestType; 
        if (status == NEXSTAR_REQUEST_TIMEOUT) {                        
            for (j = 0; j < NEXSTAR_NUM_OF_PACKETS; j++) {
                REQUEST_SET_PACKET_STATUS(j, NEXSTAR_REQUEST_IDLE); 
                nexstar_request_packet_reset(&nexstar.commsPackets[j]);
            }
            NEXSTAR_SET_STATUS(NEXSTAR_NOT_CONNECTED);
        }
        else if (status == NEXSTAR_REQUEST_COMPLETE) {
            switch (type) {
                case NEXSTAR_ECHO_COMMS:
                    nexstar_request_echo(NEXSTAR_PROCESS_REQUEST, i);
                    break;
                case NEXSTAR_ASK_IS_ALIGNED:
                    nexstar_request_get_aligned(NEXSTAR_PROCESS_REQUEST, i);
                    break;
                case NEXSTAR_ASK_ALTAZM_POSITION:
                    nexstar_request_get_altazm(NEXSTAR_PROCESS_REQUEST, i);
                    break;
            }
        }       
    }
    */
    
    if (nexstar.commsPackets[nexstar.currentRequest].requestStatus; == NEXSTAR_REQUEST_COMPLETE) {
        switch (type) {
            case NEXSTAR_ECHO_COMMS:
                nexstar_request_echo(NEXSTAR_PROCESS_REQUEST, nexstar.currentRequest);
                break;
            case NEXSTAR_ASK_IS_ALIGNED:
                nexstar_request_get_aligned(NEXSTAR_PROCESS_REQUEST, nexstar.currentRequest);
                break;
            case NEXSTAR_ASK_ALTAZM_POSITION:
                nexstar_request_get_altazm(NEXSTAR_PROCESS_REQUEST, nexstar.currentRequest);
                break;
        }
    }       
    
   /* If the Nexstar is in an unknown state attempt to ask it for
       alignment. Once we get a positive response we at least know
       a Nexstar is connected and responding. Until we have that
       knowledge there is little else we can do. */
    switch(nexstar.status) {
        case NEXSTAR_NOT_CONNECTED:
            if (! nexstar_request_count_request_type(NEXSTAR_ECHO_COMMS)) { 
                Uart2_flush_rxfifo();
                nexstar_request_echo(NEXSTAR_SEND_REQUEST, 0);                
            }
            return;            

        case NEXSTAR_CONNECTED:
        case NEXSTAR_NOT_ALIGNED:
            if (! nexstar_request_count_request_type(NEXSTAR_ASK_IS_ALIGNED)) {             
                Uart2_flush_rxfifo();
                nexstar_request_get_aligned(NEXSTAR_SEND_REQUEST, 0);                
            }
            return; 
            
        default:
            debug.printf("I don't know what to do! status is %d\r\n", nexstar.status);
            break;           
    }    

    /* As possible as often, request the Nexstar's pointing position. */
    if (! nexstar_request_count_request_type(NEXSTAR_ASK_ALTAZM_POSITION)) {
        nexstar_request_get_altazm(NEXSTAR_SEND_REQUEST, 0);
    }

    
}

/** nexstar_init(void)
 *
 * Standard module _init function. 
 */
void nexstar_init(void) {

    /* Setup the global mode of this Nexstar. */
    NEXSTAR_SET_STATUS(NEXSTAR_NOT_CONNECTED);

    /* Initialise the comms packets and buffers. */
    nexstar.currentRequest = 0;
    nexstar.availableRequest = 0;
    for (int i = 0; i < NEXSTAR_NUM_OF_PACKETS; i++) {
        nexstar_request_packet_reset(&nexstar.commsPackets[i]);
        //nexstar.commsPackets[i].requestType = 0;
        //nexstar.commsPackets[i].requestStatus = NEXSTAR_REQUEST_IDLE;
        //nexstar.commsPackets[i].bufferPointer = 0;
        //nexstar.commsPackets[i].callback = NULL;
        //memset(nexstar.commsPackets[i].buffer, 0, NEXSTAR_PACKET_BUFFER_SIZE);
    }
    
    /* Prepare the one-shot timeout timers. */
    timeout.mode(ONEMS_MODE_TIMEOUT_CALLBACK);
    timeout.attach(nexstar_timeout);

    /* Initialise the UART we use. */
    Uart2_init();
}

/** nexstar_request_packet_reset
 *
 * Return a request packet back to default state.
 *
 * @param NEXSTAR_COMMS_PACKET *p A pointer to the packet to reset.
 */
void nexstar_request_packet_reset(NEXSTAR_COMMS_PACKET *p) {
    p->requestType = 0;
    p->bufferPointer = 0;
    p->callback = NULL;
    memset(p->buffer, 0, NEXSTAR_PACKET_BUFFER_SIZE);
    p->requestStatus = NEXSTAR_REQUEST_IDLE;
}

/** nexstar_p(void)
 *
 * Get a pointer to the data structure for the Nexstar.
 * Used by the API to get data.
 */
NEXSTAR_DATA * nexstar_p(void) {
    return &nexstar;
}

/** nexstar_request_count_request_type(char requestType)
 *
 * Used to find out if any of the packets are currently marked as the supplied request type.
 *
 * @param char The status to test.
 * @return bool True otherwise false
 */
int nexstar_request_count_request_type(char requestType) {
    int count, i;
    
    for (count = 0, i = 0; i < NEXSTAR_NUM_OF_PACKETS; i++) {
        if (nexstar.commsPackets[i].requestType == requestType) {
            count++;;
        }
    }    
    return count;
}

/** nexstar_request_count_status (char status)
 *
 * Used to find out if any of the packets are currently marked as the supplied status.
 *
 * @param char The status to test.
 * @return bool True otherwise false
 */
int nexstar_request_count_status(char status) {
    int count, i;
    
    for (count = 0, i = 0; i < NEXSTAR_NUM_OF_PACKETS; i++) {
        if (nexstar.commsPackets[i].requestStatus == status) {
            count++;;
        }
    }    
    return count;
}

/** nexstar_timeout(class OneMS *q)
 *
 * A callback for the timeout timer.
 *
 * @param pointer class OneMS that invoked the callback.
 */
void nexstar_timeout(class OneMS *q) {
    q->stop();  /* Ensure we stop this timer. */
    
    /* Mark this request timed out. */
    REQUEST_SET_PACKET_STATUS(nexstar.currentRequest, NEXSTAR_REQUEST_TIMEOUT);
}

/** nexstar_request_get_altazm(int mode, unsigned char handle)
 *
 * Used to either create a request to get the current AltAzm position of the Nextsra
 * or to process a previously completed request.
 *
 * @param int mode Determine what action to create, queue request or process packet.
 * @param unsigned char handle If processing a request this is the index of the packet to be processed.
 * @return zero on failure or error, non-zero otherwise.
 */
int nexstar_request_get_altazm(int mode, unsigned char handle) {
    
    /* Create a request to get the ALT/AZM and queue it. */
    if (mode == NEXSTAR_SEND_REQUEST) {
        REQUEST_HANDLE;
        nexstar.commsPackets[handle].buffer[0] = 'Z';
        REQUEST_SET(NEXSTAR_ASK_ALTAZM_POSITION, 1);
        led3 = 1;
        return 1;
    }
    
    /* Given a completed request, parse the data and update our information. */
    if (mode == NEXSTAR_PROCESS_REQUEST) {
        if (nexstar.commsPackets[handle].requestStatus != NEXSTAR_REQUEST_COMPLETE) return 0;
        char *buffer = nexstar.commsPackets[handle].buffer;
        REQUEST_BUFFER_CHECK(9, 0);
        nexstar.currentAltRaw = hex2bin(buffer + 5, 4);
        nexstar.currentAlt    = (double)(((double)nexstar.currentAltRaw / 65536.) * 360.);
        nexstar.currentAzmRaw = hex2bin(buffer + 0, 4);
        nexstar.currentAzm    = (double)(((double)nexstar.currentAzmRaw / 65536.) * 360.);
        nexstar_request_packet_reset(&nexstar.commsPackets[handle]);
        REQUEST_SET_PACKET_STATUS(handle, NEXSTAR_REQUEST_IDLE);
        led3 = 0;
        return 1;
    }
    
    return 0;
}

/** nexstar_request_get_aligned(int mode, unsigned char handle)
 *
 * Used to either create a request to check teh Nexstar is aligned
 * or to process a previously completed request.
 *
 * @param int mode Determine what action to create, queue request or process packet.
 * @param unsigned char handle If processing a request this is the index of the packet to be processed.
 * @return zero on failure or error, non-zero otherwise.
 */
int nexstar_request_get_aligned(int mode, unsigned char handle) {
    
    /* Create a request to get the ALT/AZM and queue it. */
    if (mode == NEXSTAR_SEND_REQUEST) {
        REQUEST_HANDLE;
        nexstar.commsPackets[handle].buffer[0] = 'J';
        nexstar.commsPackets[handle].timeout = 2000;
        REQUEST_SET(NEXSTAR_ASK_IS_ALIGNED, 1);
        led2 = 1;
        return 1;
    }
    
    /* Given a completed request, parse the data and update our information. */
    if (mode == NEXSTAR_PROCESS_REQUEST) {
        if (nexstar.commsPackets[handle].requestStatus == NEXSTAR_REQUEST_TIMEOUT) {
            NEXSTAR_SET_STATUS(NEXSTAR_NOT_CONNECTED);
            nexstar_request_packet_reset(&nexstar.commsPackets[handle]);            
            led2 = 0;
            return 0;
        }
        if (nexstar.commsPackets[handle].requestStatus != NEXSTAR_REQUEST_COMPLETE) return 0;
        char *buffer = nexstar.commsPackets[handle].buffer;
        REQUEST_BUFFER_CHECK(1, 0);
        NEXSTAR_SET_STATUS((buffer[0] == 1) ? NEXSTAR_ALIGNED : NEXSTAR_NOT_ALIGNED);
        nexstar_request_packet_reset(&nexstar.commsPackets[handle]);            
        REQUEST_SET_PACKET_STATUS(handle, NEXSTAR_REQUEST_IDLE);
        led2 = 0;
        return 1;
    }
    
    return 0;
}

/** nexstar_request_echo(int mode, unsigned char handle)
 *
 * Used to either echo a data byte off the Nexstar.
 *
 * @param int mode Determine what action to create, queue request or process packet.
 * @param unsigned char handle If processing a request this is the index of the packet to be processed.
 * @return zero on failure or error, non-zero otherwise.
 */
int nexstar_request_echo(int mode, unsigned char handle) {
    
    /* Create a request to get the ALT/AZM and queue it. */
    if (mode == NEXSTAR_SEND_REQUEST) {
        REQUEST_HANDLE;
        nexstar.commsPackets[handle].buffer[0] = 'K';
        nexstar.commsPackets[handle].buffer[1] = 'k';
        nexstar.commsPackets[handle].timeout = 2000;
        REQUEST_SET(NEXSTAR_ECHO_COMMS, 2);
        led1 = 1;
        return 1;
    }
    
    /* Given a completed request, parse the data and update our information. */
    if (mode == NEXSTAR_PROCESS_REQUEST) {
    led3 = 1;
        if (nexstar.commsPackets[handle].requestStatus == NEXSTAR_REQUEST_TIMEOUT) {
            NEXSTAR_SET_STATUS(NEXSTAR_NOT_CONNECTED);
            nexstar_request_packet_reset(&nexstar.commsPackets[handle]);            
            led1 = 0;
            return 0;
        }
        if (nexstar.commsPackets[handle].requestStatus != NEXSTAR_REQUEST_COMPLETE) return 0;
        char *buffer = nexstar.commsPackets[handle].buffer;
        REQUEST_BUFFER_CHECK(1, 0);
        if (buffer[0] == 'k') debug.printf("YES! ");
        else debug.printf("NO! ");
            
        NEXSTAR_SET_STATUS((buffer[0] == 'k') ? NEXSTAR_CONNECTED : NEXSTAR_NOT_CONNECTED);
        nexstar_request_packet_reset(&nexstar.commsPackets[handle]);            
        REQUEST_SET_PACKET_STATUS(handle, NEXSTAR_REQUEST_IDLE);
        led1 = 0;
        return 1;
    }
    
    return 0;
}

/** nexstar_request_set_tracking_mode(int mode, unsigned char handle, char tracking)
 *
 * Used to set the tracking mode of the Nexstar
 *
 * @param int mode Determine what action to create, queue request or process packet.
 * @param unsigned char handle If processing a request this is the index of the packet to be processed.
 * @param char tracking The mode to send.
 * @return zero on failure or error, non-zero otherwise.
 */
int nexstar_request_set_tracking_mode(int mode, unsigned char handle, char tracking) {

    /* Create request and queue. */    
    if (mode == NEXSTAR_SEND_REQUEST) {
        REQUEST_HANDLE;
        nexstar.commsPackets[handle].buffer[0] = 'T';
        nexstar.commsPackets[handle].buffer[1] = tracking;
        REQUEST_SET(NEXSTAR_SET_TRACKING, 2);
        return 1;
    }
    
    /* Given a completed request, parse the data and update our information. */
    if (mode == NEXSTAR_PROCESS_REQUEST) {
        if (nexstar.commsPackets[handle].requestStatus != NEXSTAR_REQUEST_COMPLETE) return 0;
        char *buffer = nexstar.commsPackets[handle].buffer;
        REQUEST_BUFFER_CHECK(0, 0);
        nexstar_request_packet_reset(&nexstar.commsPackets[handle]);            
        REQUEST_SET_PACKET_STATUS(handle, NEXSTAR_REQUEST_IDLE);
        return 1;
    }
    
    return 0;
}

/** nexstar_request_get_tracking_mode(int mode, unsigned char handle)
 *
 * Used to find out what tracking mode Nexstar is in.
 *
 * @param int mode Determine what action to create, queue request or process packet.
 * @param unsigned char handle If processing a request this is the index of the packet to be processed.
 * @return zero on failure or error, non-zero otherwise.
 */
int nexstar_request_get_tracking_mode(int mode, unsigned char handle) {

    /* Create request and queue. */    
    if (mode == NEXSTAR_SEND_REQUEST) {
        REQUEST_HANDLE;
        nexstar.commsPackets[handle].buffer[0] = 't';
        REQUEST_SET(NEXSTAR_ASK_TRACKING, 1);
        return 1;
    }
    
    /* Given a completed request, parse the data and update our information. */
    if (mode == NEXSTAR_PROCESS_REQUEST) {
        if (nexstar.commsPackets[handle].requestStatus != NEXSTAR_REQUEST_COMPLETE) return 0;
        char *buffer = nexstar.commsPackets[handle].buffer;
        REQUEST_BUFFER_CHECK(1, 0);
        nexstar.trackingMode = buffer[0];
        nexstar_request_packet_reset(&nexstar.commsPackets[handle]);            
        REQUEST_SET_PACKET_STATUS(handle, NEXSTAR_REQUEST_IDLE);
        return 1;
    }
    
    return 0;
}

/** nexstar_request_set_slew(int mode, unsigned char handle, double alt, double azm)
 *
 * Used to set a variable slew rate.
 *
 * @param int mode Determine what action to create, queue request or process packet.
 * @param unsigned char handle If processing a request this is the index of the packet to be processed.
 * @param double alt The alt slew rate in degrees per second.
 * @param double azm The azm slew rate in degrees per second.
 * @return zero on failure or error, non-zero otherwise.
 */
int nexstar_request_set_slew(int mode, unsigned char handle, double alt, double azm) {
    int i;
    
    /* Create request and queue. */    
    if (mode == NEXSTAR_SEND_REQUEST) {
        REQUEST_HANDLE;
        nexstar.commsPackets[handle].buffer[0] = 'P';
        nexstar.commsPackets[handle].buffer[1] = '\x3';
        nexstar.commsPackets[handle].buffer[2] = '\x11';
        nexstar.commsPackets[handle].buffer[3] = alt > 0 ? '\x6' : '\x7';
        i = ((int)(alt * 3600.)) * 4;
        nexstar.commsPackets[handle].buffer[4] = (char)((i & 0xFF00) >> 8);
        nexstar.commsPackets[handle].buffer[5] = (char)(i & 0xFF);
        nexstar.commsPackets[handle].buffer[6] = '\0';
        nexstar.commsPackets[handle].buffer[7] = '\0';
        REQUEST_SET(NEXSTAR_SET_ALT_RATE, 8);
        
        REQUEST_HANDLE;
        nexstar.commsPackets[handle].buffer[0] = 'P';
        nexstar.commsPackets[handle].buffer[1] = '\x3';
        nexstar.commsPackets[handle].buffer[2] = '\x10';
        nexstar.commsPackets[handle].buffer[3] = azm > 0 ? '\x6' : '\x7';
        i = ((int)(azm * 3600.)) * 4;
        nexstar.commsPackets[handle].buffer[4] = (char)((i & 0xFF00) >> 8);
        nexstar.commsPackets[handle].buffer[5] = (char)(i & 0xFF);
        nexstar.commsPackets[handle].buffer[6] = '\0';
        nexstar.commsPackets[handle].buffer[7] = '\0';
        REQUEST_SET(NEXSTAR_SET_AZM_RATE, 8);
        return 1;
    }
    
    /* Given a completed request, parse the data and update our information. */
    if (mode == NEXSTAR_PROCESS_REQUEST) {
        if (nexstar.commsPackets[handle].requestStatus != NEXSTAR_REQUEST_COMPLETE) return 0;
        char *buffer = nexstar.commsPackets[handle].buffer;
        REQUEST_BUFFER_CHECK(0, 0);
        nexstar_request_packet_reset(&nexstar.commsPackets[handle]);            
        REQUEST_SET_PACKET_STATUS(handle, NEXSTAR_REQUEST_IDLE);
        return 1;
    }
    
    return 0;
}

/** nexstar_request_set_location(int mode, unsigned char handle, NEXSTAR_LOCATION *location)
 *
 * Used to set the Nexstar's location.
 *
 * @param int mode Determine what action to create, queue request or process packet.
 * @param unsigned char handle If processing a request this is the index of the packet to be processed.
 * @param NEXSTAR_LOCATION *location A pointer to a data struct holding the data.
 * @return zero on failure or error, non-zero otherwise.
 */
int nexstar_request_set_location(int mode, unsigned char handle, NEXSTAR_LOCATION *location) {

    /* Create request and queue. */    
    if (mode == NEXSTAR_SEND_REQUEST) {
        REQUEST_HANDLE;
        nexstar.commsPackets[handle].buffer[0] = 'W';
        memcpy((char *)nexstar.commsPackets[handle].buffer + 1, (char *)location, 8);
        REQUEST_SET(NEXSTAR_SET_LOCATION, 9);
        return 1;
    }
    
    /* Given a completed request, parse the data and update our information. */
    if (mode == NEXSTAR_PROCESS_REQUEST) {
        if (nexstar.commsPackets[handle].requestStatus != NEXSTAR_REQUEST_COMPLETE) return 0;
        char *buffer = nexstar.commsPackets[handle].buffer;
        REQUEST_BUFFER_CHECK(1, 0);
        nexstar.trackingMode = buffer[0];
        nexstar_request_packet_reset(&nexstar.commsPackets[handle]);            
        REQUEST_SET_PACKET_STATUS(handle, NEXSTAR_REQUEST_IDLE);
        return 1;
    }
    
    return 0;
}

/** nexstar_request_set_time(int mode, unsigned char handle, NEXSTAR_TIME *time)
 *
 * Used to set the Nexstar time.
 *
 * @param int mode Determine what action to create, queue request or process packet.
 * @param unsigned char handle If processing a request this is the index of the packet to be processed.
 * @param NEXSTAR_TIME *time A pointer to a data struct holding the data.
 * @return zero on failure or error, non-zero otherwise.
 */
int nexstar_request_set_time(int mode, unsigned char handle, NEXSTAR_TIME *time) {

    /* Create request and queue. */    
    if (mode == NEXSTAR_SEND_REQUEST) {
        REQUEST_HANDLE;
        nexstar.commsPackets[handle].buffer[0] = 'H';
        memcpy((char *)nexstar.commsPackets[handle].buffer + 1, (char *)time, 8);
        REQUEST_SET(NEXSTAR_SET_TIME, 9);
        return 1;
    }
    
    /* Given a completed request, parse the data and update our information. */
    if (mode == NEXSTAR_PROCESS_REQUEST) {
        if (nexstar.commsPackets[handle].requestStatus != NEXSTAR_REQUEST_COMPLETE) return 0;
        char *buffer = nexstar.commsPackets[handle].buffer;
        REQUEST_BUFFER_CHECK(0, 0);
        nexstar_request_packet_reset(&nexstar.commsPackets[handle]);            
        REQUEST_SET_PACKET_STATUS(handle, NEXSTAR_REQUEST_IDLE);
        return 1;
    }
    
    return 0;
}

/** nexstar_request_get_version(int mode, unsigned char handle)
 *
 * Used to set the Nexstar software version.
 *
 * @param int mode Determine what action to create, queue request or process packet.
 * @param unsigned char handle If processing a request this is the index of the packet to be processed.
 * @return zero on failure or error, non-zero otherwise.
 */
int nexstar_request_get_version(int mode, unsigned char handle) {

    /* Create request and queue. */    
    if (mode == NEXSTAR_SEND_REQUEST) {
        REQUEST_HANDLE;
        nexstar.commsPackets[handle].buffer[0] = 'V';
        REQUEST_SET(NEXSTAR_ASK_VERSION, 1);
        return 1;
    }
    
    /* Given a completed request, parse the data and update our information. */
    if (mode == NEXSTAR_PROCESS_REQUEST) {
        if (nexstar.commsPackets[handle].requestStatus != NEXSTAR_REQUEST_COMPLETE) return 0;
        char *buffer = nexstar.commsPackets[handle].buffer;
        REQUEST_BUFFER_CHECK(2, 0);
        nexstar.version = (buffer[0] << 8) | buffer[1];
        nexstar_request_packet_reset(&nexstar.commsPackets[handle]);            
        REQUEST_SET_PACKET_STATUS(handle, NEXSTAR_REQUEST_IDLE);
        return 1;
    }
    
    return 0;
}

/** nexstar_request_raw(int mode, unsigned char handle, NEXSTAR_COMMS_PACKET *q)
 *
 * Used for external callers to setup a REQUEST based on an externally defined packet structure.
 *
 * This function allows other parts of the system to make Nextstar requests that are not covered
 * by the API or just want to handle a request via a callback. 
 *
 * @param int mode Determine what action to create, queue request or process packet.
 * @param unsigned char handle If processing a request this is the index of the packet to be processed.
 * @param pointer A pointer to a predefined comms request packet to copy into our queue.
 * @return zero on failure or error, non-zero otherwise.
 */
int nexstar_request_raw(int mode, unsigned char handle, NEXSTAR_COMMS_PACKET *q) {

    /* Create request and queue. */    
    if (mode == NEXSTAR_SEND_REQUEST) {
        /* Externally handled requests must have a callback defined as a completion handler. */
        if (q->callback == NULL) {
            return 0;
        }
        REQUEST_HANDLE;
        memcpy(&nexstar.commsPackets[handle], q, sizeof(NEXSTAR_COMMS_PACKET));
        REQUEST_SET(NEXSTAR_REQUEST_RAW, 0);
        return 1;
    }
    
    /* Given a completed request, parse the data and update our information. */
    if (mode == NEXSTAR_PROCESS_REQUEST) {
        if (nexstar.commsPackets[handle].requestStatus != NEXSTAR_REQUEST_COMPLETE) return 0;
        if (nexstar.commsPackets[handle].callback != NULL) {
            (nexstar.commsPackets[handle].callback)(&nexstar.commsPackets[handle]);
        }
        nexstar_request_packet_reset(&nexstar.commsPackets[handle]);            
        REQUEST_SET_PACKET_STATUS(handle, NEXSTAR_REQUEST_IDLE);
        return 1;
    }
   
    return 0;
}


/** nexstar_send_request(int handle)
 *
 * Used to send a request packet to Nexstar via the serial port.
 *
 * @param int handle The packet to send.
 * @return zero on failure, non-zero otherwise. 
 */
int nexstar_send_request(int handle) {
    if (nexstar.commsPackets[handle].requestStatus == NEXSTAR_REQUEST_PENDING) {
        nexstar.commsPackets[handle].requestStatus = NEXSTAR_REQUEST_IN_PROGRESS;
        if (Uart2_puts(nexstar.commsPackets[handle].buffer, (int)nexstar.commsPackets[handle].txLength) != 0) {                
            memset(nexstar.commsPackets[handle].buffer, 0, NEXSTAR_PACKET_BUFFER_SIZE);
            nexstar.commsPackets[handle].bufferPointer = 0;
            // Note, we only start the timeout timer AFTER the TxFIFO goes empty, see below.
            return 1;
        }
        else {
            /* If we failed to fill the Tx FIFo then switch this request back to pending. */
            REQUEST_SET_PACKET_STATUS(handle, NEXSTAR_REQUEST_PENDING);        
            return 0;
        }
    }
    return 0;
}

/** UART2_IRQHandler(void)
 *
 * The interrupt service routine for the UART2.
 */
extern "C" void UART2_IRQHandler(void) {
    char c;
    uint32_t iir; 

    /* Get the interrupt identification register which also resets IIR. */
    iir = LPC_UART2->IIR;

    if (iir & 0x0001) {
        /* Eh? Then why did we interrupt? */
        return;
    }
       
    /* Do we have incoming data? */
    if (iir & UART_ISSET_RDA) {
        c = (char)LPC_UART2->RBR;
        //debug.printf(" %c", c & 0x7f);
        if (nexstar.commsPackets[nexstar.currentRequest].requestStatus != NEXSTAR_REQUEST_IN_PROGRESS) { 
            return;
        }
        nexstar.commsPackets[nexstar.currentRequest].buffer[nexstar.commsPackets[nexstar.currentRequest].bufferPointer] = c;
        nexstar.commsPackets[nexstar.currentRequest].bufferPointer++;
        nexstar.commsPackets[nexstar.currentRequest].bufferPointer &= (NEXSTAR_PACKET_BUFFER_SIZE-1);
        if (c == '#' || c == '\r' || c == '\n') {            
            timeout.stop();   /* Halt the timeout timer. */
            REQUEST_SET_PACKET_STATUS(nexstar.currentRequest, NEXSTAR_REQUEST_COMPLETE);
            
            /* Advance to the next request packet and if pending start it. */
            //NEXSTAR_NEXT_REQUEST(nexstar.currentRequest);
            //nexstar_send_request(nexstar.currentRequest);
            //debug.printf("\r\n");
        }
    }

    /* If the THRE goes empty AND the TxFIFO is also empty then a command
       was just sent to the Nexstar. Start the serial timeout timer so we 
       only make a timeout measurement AFTER we have sent our command. 
       That way we don't include our serial transmit time within the timeout
       specified by the timer. */
    if (iir & UART_ISSET_THRE) {
        if ((LPC_UART2->FIFOLVL & UART_ISSET_FIFOLVL_TXFULL) == 0) {
            timeout.start(nexstar.commsPackets[nexstar.currentRequest].timeout == 0 ? NEXSTAR_SERIAL_TIMEOUT : (uint32_t)nexstar.commsPackets[nexstar.currentRequest].timeout);
        }
    }  
    
    if (iir & UART_ISSET_RLS) {
        // Do nothing for now other than read the iir register and clear the interrupt.
    }
}

/** Uart2_init
 *
 * Initialise UART2 to our requirements for Nexstar.
 */
void Uart2_init (void) {

    /* Switch on UART2. */
    LPC_SC->PCONP |= UART2_ORMASK_PCONP;

    /* Set PCLK == CCLK, 96Mhz */
    LPC_SC->PCLKSEL1 |= UART2_ORMASK_PCLKSEL1;

    /* Enable the tx/rx to pins and select pin mode. */
    LPC_PINCON->PINSEL0  |= UART2_ORMASK_PINSEL0;
    LPC_PINCON->PINMODE0 |= UART2_ORMASK_PINMODE0;
       
    /* Set the divisor values for 9600 and then set 8,n,1 */
    LPC_UART2->LCR = UART2_SET_LCR_DLAB;
    LPC_UART2->DLL = UART2_SET_DLLSB;
    LPC_UART2->DLM = UART2_SET_DLMSB;
    LPC_UART2->LCR = UART2_SET_LCR;
    
    /* Enable FIFOs and then clear them. */
    LPC_UART2->FCR = UART2_SET_FCR;
    LPC_UART2->FCR = UART2_SET_FCR_CLEAR;

    /* Enable the RDA and THRE interrupts. */
    LPC_UART2->IER = UART2_SET_IER;
    
    /* Now it's time to do interrupts. */
    NVIC_SetVector(UART2_IRQn, (uint32_t)UART2_IRQHandler);
    NVIC_EnableIRQ(UART2_IRQn);    
}

/** Uart2_flush_rxfifo
 * 
 * Flush the input RX fifo to make sure it's empty.
 */
void Uart2_flush_rxfifo(void) {
    uint8_t c;
    while (LPC_UART2->LSR & 0x1) {
        c = LPC_UART2->RBR;
    }
}

/** Uart2_putc
 *
 * Put the character given into the TxFIFO.
 * By design there should always be room in 
 * the fifo, but check it to make sure.
 *
 * @param char The character to write.
 * @return int zero on failure, non-zero otherwise.
 */
int Uart2_putc(char c) {
    /* Check the TxFIFO is not full.  */
    if ((LPC_UART2->FIFOLVL & UART_ISSET_FIFOLVL_TXFULL) != 0) return 0;
    LPC_UART2->THR = (uint8_t)c;
    return -1;
}

/** Uart2_puts
 *
 * Put the string given into the TX FIFO.
 * By design there should always be room in 
 * the fifo, but check it to make sure.
 *
 * @param char *s Pointer The string to write.
 * @return int zero on failure, non-zero (number of chars written) otherwise.
 */
int Uart2_puts(char *s, int len) {
    int i;
    
    /* Check the FIFO is empty, as it should always be by design. */
    if ((LPC_UART2->FIFOLVL & UART_ISSET_FIFOLVL_TXFULL) == 0) {
        for (i = 0; i < len; i++) {
            LPC_UART2->THR = (uint8_t)s[i];
            debug.printf("%c", s[i]);
        }
        debug.printf("\r\n");
    }
    else {
        return 0;
    }
    return i;
}

#endif