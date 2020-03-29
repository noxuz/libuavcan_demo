/* Demo of libuavcan v1 media driver layer for the NXP S32K14x family
 * of aumototive-grade MCU's, running CANFD at 4Mbit/s in data phase
 *
 * Description:
 * A frame object is transferred between two S32K142 evaluation boards
 * (EVB's) called nodes, NODE_A kickstarts the transmission of a 64-byte
 * payload size frame initially of zeroes, and NODE_B receives it and
 * interprets the last 64 bits of it as a uint64 number, then adds 1 to it
 * and transmit it back to NODE_A, which repeats the process; in an
 * oscilloscope the payload's last 64 bits are viewed as a count of the
 * times a node has received the frame bouncing between the nodes, and
 * each 1000 times a node has received it, toggles a green LED from the
 * board.
 *
 * Time for a single frame for transmission, reception and retransmission:
 * 440us, to toggle the LED it takes 0.88 seconds, (2000 frames transfer),
 * to reach 16^5 transfers it takes 7 minutes, to reach the 32nd bit
 * 21.87 days and to overflow the 64-bit wide count, 160,860 centuries.
 *
 * Instructions: Import the project to S32DS, change the macro
 * to NODE_A or NODE_B to build for one of the boards and flash.
 */

/* Include media layer driver for NXP S32K MCU */
#include "s32k_libuavcan.hpp"

/* Choose for which board of the demo to target, NODE_A or NODE_B */
#define NODE_A

/* Function that takes the last 8 bytes from the payload, interprets them as a uint64 and adds 1 */
void payload_bounceADD(std::uint8_t* rx_payload)
{
     /* Reinterpret the first 4 bytes from the last 8 btes of the payload as the 
      * least significat 32-bits from the whole 64-bit variable */
     std::uint64_t payloadLSB =
           static_cast<std::uint64_t>((rx_payload[ 56 ] << 24)   |
                                      (rx_payload[ 57 ] << 16)   |
                                      (rx_payload[ 58 ] << 8)    |
                                      (rx_payload[ 59 ] << 0));
     /* Reinterpret the first 4 bytes from the last 8 btes of the payload as the 
      * least significat 32-bits from the whole 64-bit variable */
     std::uint64_t payloadMSB =
           static_cast<std::uint64_t>((rx_payload[ 60 ] << 24)   |
                                      (rx_payload[ 61 ] << 16)   |
                                      (rx_payload[ 62 ] << 8)    |
                                      (rx_payload[ 63 ] << 0));
   
     /* Construct the 64-bit number from the bytes harvested from theframe's payload */
     std::uint64_t fullNumber = (std::uint64_t)((std::uint64_t)(payloadLSB << 32) | payloadMSB);

     /* Add 1 */
     fullNumber = fullNUmber++;

     /* Fill-up the payload with the previous number, placing its byte in its place for transmission */
     for(std::uint8_t i = 0; i < 8; i++)
     {
         rx_payload[63-i] = (std::uint64_t)((std::uint64_t)( fullNumber & (std::uint64_t)(0xFF << (8*i)) ) >> (8*i));
     }
}

void greenLED_init(void)
{

    PCC->PCCn[PCC_PORTD_INDEX] |= PCC_PCCn_CGC_MASK;   /* Enable clock for PORTD */
    PORTD->PCR[16] = PORT_PCR_MUX(1);                  /* Port D16: MUX = GPIO              */
    PTD->PDDR |= 1<<16;                                /* Port D16: Data direction = output  */

}

#if defined(NODE_A)
/* ID for the current UAVCAN node */
constexpr std::uint32_t Node_ID = 0xC0C0A;
/* ID of the frame to transmit */
constexpr std::uint32_t demo_FrameID = 0xC0FFE;

#elif defined(NODE_B)
/* ID and for the current UAVCAN node */
constexpr std::uint32_t Node_ID = 0xC0FFE;
/* ID of the frame to transmit */
constexpr std::uint32_t demo_FrameID = 0xC0C0A;
#endif

constexpr std::uint32_t Node_Mask = 0xFFFFF;   /* All care bits mask for frame filtering */
constexpr std::size_t Node_Filters_Count = 1u; /* Number of ID's that the node will filter in */
constexpr std::size_t Node_Frame_Count = 1u;   /* Frames transmitted each time */
constexpr std::size_t First_Instance = 1u;     /* Interface instance used in this demo */

/* Size of the payload in bytes of the frame to be transmitted */
constexpr std::uint16_t payload_length = libuavcan::media::S32K_InterfaceGroup::FrameType::MTUBytes;

int main()
{

/* Frame's Data Length Code in function of it's payload length in bytes */
libuavcan::media::CAN::FrameDLC demo_DLC = libuavcan::media::S32K_InterfaceGroup::FrameType::lengthToDlc(payload_length);

/* 64-byte payload that will be exchanged between the nodes */
std::uint8_t demo_payload[payload_length];

/* Initial value of the frame's payload */
std::fill(demo_payload,demo_payload+payload_length,0);

/* Instantiate factory object */
libuavcan::media::S32K_InterfaceManager demo_Manager;

/* Create pointer to Interface object */
libuavcan::media::S32K_InterfaceGroup* demo_InterfacePtr;

/* Create a frame that will reach NODE_B ID */
libuavcan::media::S32K_InterfaceGroup::FrameType bouncing_frame_obj(demo_FrameID,demo_payload,demo_DLC);

/* Array of frames to transmit (current implementation supports 1) */
libuavcan::media::S32K_InterfaceGroup::FrameType bouncing_frame[Node_Frame_Count] = {bouncing_frame_obj};

/* Instantiate the filter object that the current node will apply to receiving frames */
libuavcan::media::S32K_InterfaceGroup::FrameType::Filter demo_Filter(Node_ID,Node_Mask);

std::uint32_t rx_msg_count = 0;

/* Status variable for sequence control */
libuavcan::Result status;

/* Initialize the node with the previously defined filtering using factory method */
status = demo_Manager.startInterfaceGroup(&demo_Filter,Node_Filters_Count,demo_InterfacePtr);

greenLED_init();

/* Node A kickstarts */
#ifdef NODE_A
    std::size_t frames_wrote = 0;
   if ( libuavcan::isSuccess(status) )
    {
        demo_InterfacePtr->write(First_Instance,bouncing_frame,Node_Frame_Count,frames_wrote);
    }
#endif

/* Super-loop for retransmission of the frame */
for(;;)
{
    std::size_t frames_read = 0;

    if ( libuavcan::isSuccess(status) )
    {
       status = demo_InterfacePtr->read(First_Instance, bouncing_frame, frames_read);
    }

    /* frames_read should be 1 from he previous read method if a frame was received */
    if(frames_read)
    {
         /* Increment receive msg counter */
         rx_msg_count++;

         if ( rx_msg_count == 1000 )
         {
             PTD->PTOR |= 1<<16;    /* Toggle green LED*/
             rx_msg_count = 0;      /* Reset th counter of received frames */
         }

         std::size_t frames_wrote;

         /* Swap the frame's ID for returning it back to the sender */
         bouncing_frame[0].id = demo_FrameID;

         /* The frame is sent back bt with the payload  */
         payload_bounceADD(bouncing_frame[0].data);

         /* Perform transmission */
         if ( libuavcan::isSuccess(status) )
         {
             status = demo_InterfacePtr->write(First_Instance, bouncing_frame, Node_Frame_Count, frames_wrote);
         }
    }
}

}
