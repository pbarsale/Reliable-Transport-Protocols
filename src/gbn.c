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

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* called from layer 5, passed the data to be sent to other side */

int base,nextseqnum,msg_buf_start,msg_buf_end,expectedseqnum;
struct msg msg_buffer[10000];
struct pkt sent_buffer[10000];

#define PACKETDATASIZE 20;
#define A 0
#define B 1

void A_output(struct msg message)
{
	
	int win_size=getwinsize();

	if(nextseqnum<base+win_size)
	{
		struct pkt send_packet;
		memset(&send_packet,0,sizeof(send_packet));

		send_packet.seqnum=nextseqnum;
		send_packet.acknum=0;
		
		for(int i=0;i<20;i++)
			send_packet.payload[i]=message.data[i];
		
		calc_checksum(&send_packet);

		tolayer3(A,send_packet);
		
		sent_buffer[nextseqnum]=send_packet;
		if(base==nextseqnum)
			starttimer(A,20);
		
		nextseqnum++;
	}
	else
	{
		if(msg_buf_start==-1 && msg_buf_end==-1)
		{
			msg_buf_start++;
			msg_buf_end++;
			msg_buffer[msg_buf_end]=message;
		}
		else
		{
			msg_buf_end++;
			msg_buffer[msg_buf_end]=message;
		}
	}	

}

void calc_checksum(struct pkt *send_packet)
{
	
	int checksum;
    	checksum = send_packet->seqnum;
    	checksum = checksum + send_packet->acknum;

    	for (int i = 0; i <20; i++)
        	checksum = checksum + (int)(send_packet->payload[i]);
    	checksum = 0 - checksum;

    	send_packet->checksum = checksum;
}




/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
	int win_size=getwinsize();
	if(!is_corrupted(packet))
	{
		base=packet.acknum+1;
		
		while((nextseqnum<base+win_size) && (msg_buf_start<=msg_buf_end) && (msg_buf_start!=-1))
		{
			struct pkt send_packet;
                	memset(&send_packet,0,sizeof(send_packet));

                	send_packet.seqnum=nextseqnum;
                	send_packet.acknum=0;

                	for(int i=0;i<20;i++)
                        	send_packet.payload[i]=msg_buffer[msg_buf_start].data[i];

                	calc_checksum(&send_packet);

                	tolayer3(A,send_packet);

                	sent_buffer[nextseqnum]=send_packet;
                	nextseqnum++;
			
			if(msg_buf_start==msg_buf_end)
			{
				msg_buf_start=-1;
				msg_buf_end=-1;
			}
			else
				msg_buf_start++;
		}
		
                if(base==nextseqnum)
                {
                        stoptimer(A);
                }
		else
		{
			starttimer(A,20);
		}
		
	}		
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	starttimer(A,20);
	if(!(base==nextseqnum))
	{
		for(int i=base;i<nextseqnum;i++)
		{
			tolayer3(A,sent_buffer[i]);
		}	
	}	
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	memset(&sent_buffer,0,sizeof(sent_buffer));
	memset(&msg_buffer,0,sizeof(msg_buffer));
	base=0;
	nextseqnum=0;
	msg_buf_start=-1;
	msg_buf_end=-1;
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

	ack_packet.seqnum=0;
        for(int i=0;i<20;i++)
        {
            ack_packet.payload[i]='0';
        }

	if(expectedseqnum==packet.seqnum)	
	{
		ack_packet.acknum=expectedseqnum;
		calc_checksum(&ack_packet);

		tolayer3(B,ack_packet);
		tolayer5(B,packet.payload);	
		expectedseqnum++;
	}
	else 
	{
		if(expectedseqnum>0)
		{
		        ack_packet.acknum=expectedseqnum-1;
        	        calc_checksum(&ack_packet);
                	tolayer3(B,ack_packet);
		}
	}
	
}


int is_corrupted(struct pkt rec_packet)
{
	int new_checksum;
        new_checksum = rec_packet.seqnum;
        new_checksum = new_checksum + rec_packet.acknum;

        for (int i = 0; i <20; i++)
                new_checksum = new_checksum + (int)(rec_packet.payload[i]);
        
	new_checksum=0-new_checksum;

	if(new_checksum==rec_packet.checksum)
		return 0;
	else
		return 1;

}


/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	expectedseqnum=0;
}


