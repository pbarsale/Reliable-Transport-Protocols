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

int base,nextseqnum,msg_buf_start,msg_buf_end,expectedseqnum;
struct msg msg_buffer[2000];
struct ack_buf
{
	struct pkt ack_packet;
	int valid;
}ack_buffer[2000];

struct sent_buf
{
	struct pkt sent_packet;
	int got_ack;
	float timer;
}sent_buffer[2000];
	 
#define A 0
#define B 1

float get_min_time();

/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
//	printf("Inside A_output \r\n");
//	printf("Sender has message %s \r\n",message);
        int win_size=getwinsize();

        if(nextseqnum<base+win_size)
        {
//		 printf(" nextseqnum %d \r\n",nextseqnum);
                struct pkt send_packet;
                memset(&send_packet,0,sizeof(send_packet));

                send_packet.seqnum=nextseqnum;
                send_packet.acknum=0;

                for(int i=0;i<20;i++)
                        send_packet.payload[i]=message.data[i];

                calc_checksum(&send_packet);

                tolayer3(A,send_packet);
	
		sent_buffer[nextseqnum].sent_packet=send_packet;
		sent_buffer[nextseqnum].timer=get_sim_time()+30;
//		printf("Packet has sent : S: %d T: %f\r\n",nextseqnum,sent_buffer[nextseqnum].timer);
		
                if(base==nextseqnum)
                        starttimer(A,30);

                nextseqnum++;
        }
        else
        {
//		 printf("Cant sent the message, buffer it \r\n");
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
//	printf("Inside calculate checksum  \r\n");
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
//	printf("Inside A_input \r\n");
//	printf("A got packet S : %d A: %d M: %s \r\n",packet.seqnum,packet.acknum,packet.payload);
        int win_size=getwinsize();
	if(is_corrupted(packet) || (packet.acknum<base) || (packet.acknum>=base+win_size))
		return;
	
//	 printf("Packet is not corrupted \r\n");
	sent_buffer[packet.acknum].got_ack=1;
	sent_buffer[packet.acknum].timer=0;

	if(packet.acknum==base)
	{
//		printf("Base packet arrived \r\n");
		for(int i=base;i<nextseqnum;i++)
		{
			if(sent_buffer[i].got_ack!=1)
				break;
			base++;
		}
//		printf("Final value of base %d \r\n",base);
	}

	while((nextseqnum<base+win_size) && (msg_buf_start<=msg_buf_end) && (msg_buf_start!=-1))
       {
//		 printf("Send buffer messages \r\n");
		struct pkt send_packet;
                memset(&send_packet,0,sizeof(send_packet));

                send_packet.seqnum=nextseqnum;
                send_packet.acknum=0;

                for(int i=0;i<20;i++)
                	send_packet.payload[i]=msg_buffer[msg_buf_start].data[i];

               	calc_checksum(&send_packet);

                tolayer3(A,send_packet);
			
		sent_buffer[nextseqnum].sent_packet=send_packet;
		sent_buffer[nextseqnum].timer=get_sim_time()+30;
//		printf("Packet has sent : S: %d T: %f\r\n",nextseqnum,sent_buffer[nextseqnum].timer);

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
		stoptimer(A);			
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
//	printf("Inside timer interrupt \r\n");
//	printf("Timer Interrupt : get_sim_time %f \r\n",get_sim_time());
	for(int i=base;i<nextseqnum;i++)
	{
//		printf("Check for seqnum %d \r\n",base);
//		printf("Check timer for the packet %f \r\n",sent_buffer[i].timer);
		if(sent_buffer[i].got_ack==0 && sent_buffer[i].timer==get_sim_time())
		{

//			printf("Resending this packet : %d \r\n",sent_buffer[i].sent_packet.seqnum);
			tolayer3(A,sent_buffer[i].sent_packet);
			sent_buffer[i].timer=get_sim_time()+30;
//			printf("Updated timer after resending the packet %d \r\n",sent_buffer[i].timer);
			break;
		}
	}
//	printf("Out of loop\r\n");
	float min=get_min_time();
//	printf("Minimum available timer is %f : \r\n",min);
	starttimer(A,min-get_sim_time());	
}  

float get_min_time()
{
//	printf("Inside get_min_time \r\n");
	float min_time=0;
	for(int i=base;i<nextseqnum;i++)
        {
		if(sent_buffer[i].got_ack==0 && sent_buffer[i].timer>0)
		{
			if(min_time==0)
				 min_time=sent_buffer[i].timer;

			else if(sent_buffer[i].timer<min_time)
                                min_time=sent_buffer[i].timer;

//			printf("Checking for packet %d having timer %f \r\n",sent_buffer[i].sent_packet.seqnum, sent_buffer[i].timer);
//			printf("min now is %f \r\n",min_time);
		}		
	}
//	printf("Final min is %f \r\n",min_time);
	return min_time;
}
/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
//	printf("Initializing A \r\n");
        memset(&sent_buffer,0,sizeof(sent_buffer));
        memset(&msg_buffer,0,sizeof(msg_buffer));
        base=0;
        nextseqnum=0;
        msg_buf_start=-1;
        msg_buf_end=-1;
//	printf("A init done \r\n");
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
//	printf("Inside B_input \r\n");
//	printf("B got packet S: %d A: %d M: %s \r\n",packet.seqnum,packet.acknum,packet.payload);
	int win_size=getwinsize();
	
	if((is_corrupted(packet)) || (packet.seqnum>=expectedseqnum+win_size) || (packet.seqnum<expectedseqnum-win_size))
                return;
	
//	printf("Base %d \r\n",expectedseqnum);
//	printf("Packet is not corrupted and within window size \r\n");
        struct pkt ack_packet;
        memset(&ack_packet,0,sizeof(ack_packet));

        ack_packet.seqnum=0;
        for(int i=0;i<20;i++)
        {
        	ack_packet.payload[i]='0';
        }

  	ack_packet.acknum=packet.seqnum;
        calc_checksum(&ack_packet);
        tolayer3(B,ack_packet);

 	if(packet.seqnum>expectedseqnum)
	{
//		printf("Out of packet arrived \r\n");
		ack_buffer[packet.seqnum].ack_packet=packet;
		ack_buffer[packet.seqnum].valid=1;
	}   
	
	else if(packet.seqnum==expectedseqnum)
	{
//		printf("Base Packet arrived \r\n");
		int len=expectedseqnum+win_size;
		tolayer5(B,packet.payload);
		expectedseqnum++;
				
		for(int i=expectedseqnum;i<len;i++)
		{
			if(ack_buffer[i].valid==0)
				break;
			tolayer5(B,ack_buffer[i].ack_packet.payload);
                        expectedseqnum++;
		}
//		printf("new base is %d \r\n",expectedseqnum);
	}
}

int is_corrupted(struct pkt rec_packet)
{
//	printf("Inside is_corrupted \r\n");
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
//	printf("B init start \r\n");
	memset(&ack_buffer,0,sizeof(ack_buffer));     
	expectedseqnum=0;
//	printf("B init done \r\n");
}
