#include "../include/simulator.h"

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SIX ROUTINES *********/


int nextseqnum,expectedseqnum,msg_buf_start,msg_buf_end;
int A_SENT_PACKET;

struct msg msg_buffer[5000];
struct pkt sent_buffer;

#define A 0
#define B 1

/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
	if(A_SENT_PACKET==0)
        {
                struct pkt send_packet;
                memset(&send_packet,0,sizeof(send_packet));

                send_packet.seqnum=nextseqnum;
		for (int i = 0; i < 20;i++)
		{
			send_packet.payload[i] = message.data[i];
		}	
//	strcpy(send_packet.payload,message.data);
		//printf ("<before :%d>\r\n",send_packet.checksum);
                calc_checksum(&send_packet);
		A_SENT_PACKET=1;
		starttimer(A,10);
		tolayer3(A,send_packet);
		
		sent_buffer=send_packet;
        }
        else
        {
		msg_buffer[msg_buf_end]=message;
                msg_buf_end++;
        }
	return;
}

void calc_checksum(struct pkt *send_packet)
{

        int checksum;
        checksum = send_packet->seqnum;
        checksum = checksum + send_packet->acknum;

        for (int i = 0; i <20; i++)
                checksum = checksum + (int)(send_packet->payload[i]);

        send_packet->checksum = checksum;
}






/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
	if(is_corrupted(packet))
		return;
	
        if(nextseqnum==packet.acknum)
        {
                stoptimer(A);
		if(nextseqnum==1)
			nextseqnum=0;
		else
			nextseqnum=1;


                if(msg_buf_start<msg_buf_end)	
                {
                        struct pkt send_packet;
                        memset(&send_packet,0,sizeof(send_packet));

                        send_packet.seqnum=nextseqnum;

			A_SENT_PACKET=1;
        		for (int i= 0 ; i < 20; i++)
			{
				send_packet.payload[i] = msg_buffer[msg_buf_start].data[i];
			}
			starttimer(A,10);
			calc_checksum(&send_packet);
		//	printf ("<after  :%d>\r\n",send_packet.checksum);
			tolayer3(A,send_packet);
                        sent_buffer=send_packet;
                        msg_buf_start++;
                }
		else
		{
			A_SENT_PACKET=0;
        	}
	}
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	tolayer3(A,sent_buffer);
	starttimer(A,10);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
        nextseqnum=0;
	A_SENT_PACKET=0;
	memset(&msg_buffer,0,sizeof(msg_buffer));
	memset(&sent_buffer,0,sizeof(sent_buffer));	
	msg_buf_start=0;
	msg_buf_end=0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
	if(is_corrupted(packet))
		return;

        struct pkt ack_packet;
        memset(&ack_packet,0,sizeof(ack_packet));

	ack_packet.acknum=packet.seqnum;
//	printf ("before:%d\r\n",ack_packet.checksum);
        calc_checksum(&ack_packet);
//        printf ("after:%d\r\n",ack_packet.checksum);                
        tolayer3(B,ack_packet);
        
	if(expectedseqnum==packet.seqnum)
        {
                tolayer5(B,packet.payload);
               
		if(expectedseqnum==1)
			expectedseqnum=0;
		else
			expectedseqnum=1;
        }
}


int is_corrupted(struct pkt rec_packet)
{
        int new_checksum = rec_packet.seqnum;
        new_checksum += rec_packet.acknum;

        for (int i = 0; i <20; i++)
                new_checksum += (int)rec_packet.payload[i];

        if(new_checksum==rec_packet.checksum)
                return 0;
        else
                return 1;

}



/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
        expectedseqnum=0;
}
