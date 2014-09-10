  
/*
#include <stdio.h>
#include <Arduino.h>
#include <EEPROM.h>
#include <SPI.h>
#include <avr/pgmspace.h>
*/
/*#include "FT_Platform.h"
#include "FT_Hal_SPI.cpp"
#include "FT_CoPro_Cmds.cpp"
*/

#include <Wire.h>

#include "FT_Platform.h"
#include "FT_Hal_I2C.h"


#define SAMAPP_DELAY_BTW_APIS (1000)
#define SAMAPP_ENABLE_DELAY() Ft_Gpu_Hal_Sleep(SAMAPP_DELAY_BTW_APIS)
#define SAMAPP_ENABLE_DELAY_VALUE(x) Ft_Gpu_Hal_Sleep(x)

/* Global variables for display resolution to support various display panels */

ft_int16_t FT_DispWidth = 480;
ft_int16_t FT_DispHeight = 272;
ft_int16_t FT_DispHCycle =  548;
ft_int16_t FT_DispHOffset = 43;
ft_int16_t FT_DispHSync0 = 0;
ft_int16_t FT_DispHSync1 = 41;
ft_int16_t FT_DispVCycle = 292;
ft_int16_t FT_DispVOffset = 12;
ft_int16_t FT_DispVSync0 = 0;
ft_int16_t FT_DispVSync1 = 10;
ft_uint8_t FT_DispPCLK = 5;
ft_char8_t FT_DispSwizzle = 0;
ft_char8_t FT_DispPCLKPol = 1;

#define DISPLAY_QVGA	

#define ON          1
#define OFF         0						 
#define Font        27					// Font Size
#define MAX_LINES   4					// Max Lines allows to Display

#define SPECIAL_FUN     251
#define BACK_SPACE	251				// Back space
#define CAPS_LOCK	252				// Caps Lock
#define NUMBER_LOCK	253				// Number Lock
#define BACK		254				// Exit 

#define LINE_STARTPOS	FT_DispWidth/50			// Start of Line
#define LINE_ENDPOS	FT_DispWidth			// max length of the line
		
/* Global used for buffer optimization */
Ft_Gpu_Hal_Context_t host,*phost;


ft_uint32_t Ft_CmdBuffer_Index;
ft_uint32_t Ft_DlBuffer_Index;

#ifdef BUFFER_OPTIMIZATION
ft_uint8_t  Ft_DlBuffer[FT_DL_SIZE];
ft_uint8_t  Ft_CmdBuffer[FT_CMD_FIFO_SIZE];
#endif

ft_void_t Ft_App_WrCoCmd_Buffer(Ft_Gpu_Hal_Context_t *phost,ft_uint32_t cmd)
{
#ifdef  BUFFER_OPTIMIZATION
   /* Copy the command instruction into buffer */
   ft_uint32_t *pBuffcmd;
   pBuffcmd =(ft_uint32_t*)&Ft_CmdBuffer[Ft_CmdBuffer_Index];
   *pBuffcmd = cmd;
#endif
#ifdef ARDUINO_PLATFORM
   Ft_Gpu_Hal_WrCmd32(phost,cmd);
#endif

   /* Increment the command index */
   Ft_CmdBuffer_Index += FT_CMD_SIZE;  
}

ft_void_t Ft_App_WrDlCmd_Buffer(Ft_Gpu_Hal_Context_t *phost,ft_uint32_t cmd)
{
#ifdef BUFFER_OPTIMIZATION  
   /* Copy the command instruction into buffer */
   ft_uint32_t *pBuffcmd;
   pBuffcmd =(ft_uint32_t*)&Ft_DlBuffer[Ft_DlBuffer_Index];
   *pBuffcmd = cmd;
#endif

#ifdef ARDUINO_PLATFORM
   Ft_Gpu_Hal_Wr32(phost,(RAM_DL+Ft_DlBuffer_Index),cmd);
#endif
   /* Increment the command index */
   Ft_DlBuffer_Index += FT_CMD_SIZE;  
}

ft_void_t Ft_App_WrCoStr_Buffer(Ft_Gpu_Hal_Context_t *phost,const ft_char8_t *s)
{
#ifdef  BUFFER_OPTIMIZATION  
  ft_uint16_t length = 0;
  length = strlen(s) + 1;//last for the null termination
  
  strcpy(&Ft_CmdBuffer[Ft_CmdBuffer_Index],s);  

  /* increment the length and align it by 4 bytes */
  Ft_CmdBuffer_Index += ((length + 3) & ~3);  
#endif  
}

ft_void_t Ft_App_Flush_DL_Buffer(Ft_Gpu_Hal_Context_t *phost)
{
#ifdef  BUFFER_OPTIMIZATION    
   if (Ft_DlBuffer_Index > 0)
     Ft_Gpu_Hal_WrMem(phost,RAM_DL,Ft_DlBuffer,Ft_DlBuffer_Index);
#endif     
   Ft_DlBuffer_Index = 0;
   
}

ft_void_t Ft_App_Flush_Co_Buffer(Ft_Gpu_Hal_Context_t *phost)
{
#ifdef  BUFFER_OPTIMIZATION    
   if (Ft_CmdBuffer_Index > 0)
     Ft_Gpu_Hal_WrCmdBuf(phost,Ft_CmdBuffer,Ft_CmdBuffer_Index);
#endif     
   Ft_CmdBuffer_Index = 0;
}


/* API to give fadeout effect by changing the display PWM from 100 till 0 */
ft_void_t SAMAPP_fadeout()
{
   ft_int32_t i;
	
	for (i = 100; i >= 0; i -= 3) 
	{
		Ft_Gpu_Hal_Wr8(phost,REG_PWM_DUTY,i);

		Ft_Gpu_Hal_Sleep(2);//sleep for 2 ms
	}
}

/* API to perform display fadein effect by changing the display PWM from 0 till 100 and finally 128 */
ft_void_t SAMAPP_fadein()
{
	ft_int32_t i;
	
	for (i = 0; i <=100 ; i += 3) 
	{
		Ft_Gpu_Hal_Wr8(phost,REG_PWM_DUTY,i);
		Ft_Gpu_Hal_Sleep(2);//sleep for 2 ms
	}
	/* Finally make the PWM 100% */
	i = 128;
	Ft_Gpu_Hal_Wr8(phost,REG_PWM_DUTY,i);
}
#ifdef SAMAPP_ENABLE_APIS_SET0

/* Optimized implementation of sin and cos table - precision is 16 bit */
FT_PROGMEM ft_prog_uint16_t sintab[] = {
0, 402, 804, 1206, 1607, 2009, 2410, 2811, 3211, 3611, 4011, 4409, 4807, 5205, 5601, 5997, 6392, 
6786, 7179, 7571, 7961, 8351, 8739, 9126, 9511, 9895, 10278, 10659, 11038, 11416, 11792, 12166, 12539,
12909, 13278, 13645, 14009, 14372, 14732, 15090, 15446, 15799, 16150, 16499, 16845, 17189, 17530, 17868,
18204, 18537, 18867, 19194, 19519, 19840, 20159, 20474, 20787, 21096, 21402, 21705, 22004, 22301, 22594, 
22883, 23169, 23452, 23731, 24006, 24278, 24546, 24811, 25072, 25329, 25582, 25831, 26077, 26318, 26556, 26789, 
27019, 27244, 27466, 27683, 27896, 28105, 28309, 28510, 28706, 28897, 29085, 29268, 29446, 29621, 29790, 29955, 
30116, 30272, 30424, 30571, 30713, 30851, 30984, 31113, 31236, 31356, 31470, 31580, 31684, 31785, 31880, 31970, 
32056, 32137, 32213, 32284, 32350, 32412, 32468, 32520, 32567, 32609, 32646, 32678, 32705, 32727, 32744, 32757, 
32764, 32767, 32764};

ft_int16_t SAMAPP_qsin(ft_uint16_t a)
{
  ft_uint8_t f;
  ft_int16_t s0,s1;

  if (a & 32768)
    return -SAMAPP_qsin(a & 32767);
  if (a & 16384)
      a = 32768 - a;
  f = a & 127;
  s0 = ft_pgm_read_word(sintab + (a >> 7));
  s1 = ft_pgm_read_word(sintab + (a >> 7) + 1);
  return (s0 + ((ft_int32_t)f * (s1 - s0) >> 7));
}

/* cos funtion */
ft_int16_t SAMAPP_qcos(ft_uint16_t a)
{
  return (SAMAPP_qsin(a + 16384));
}
#endif


/* API to check the status of previous DLSWAP and perform DLSWAP of new DL */
/* Check for the status of previous DLSWAP and if still not done wait for few ms and check again */
ft_void_t SAMAPP_GPU_DLSwap(ft_uint8_t DL_Swap_Type)
{
	ft_uint8_t Swap_Type = DLSWAP_FRAME,Swap_Done = DLSWAP_FRAME;

	if(DL_Swap_Type == DLSWAP_LINE)
	{
		Swap_Type = DLSWAP_LINE;
	}

	/* Perform a new DL swap */
	Ft_Gpu_Hal_Wr8(phost,REG_DLSWAP,Swap_Type);

	/* Wait till the swap is done */
	while(Swap_Done)
	{
		Swap_Done = Ft_Gpu_Hal_Rd8(phost,REG_DLSWAP);

		if(DLSWAP_DONE != Swap_Done)
		{
			Ft_Gpu_Hal_Sleep(10);//wait for 10ms
		}
	}	
}

/*****************************************************************************/
/* Example code to display few points at various offsets with various colors */



/* Boot up for FT800 followed by graphics primitive sample cases */
/* Initial boot up DL - make the back ground green color */

const ft_uint8_t FT_DLCODE_BOOTUP[12] = 
{
    255,255,255,2,//GPU instruction CLEAR_COLOR_RGB
    7,0,0,38, //GPU instruction CLEAR
    0,0,0,0,  //GPU instruction DISPLAY
};

/*deflated Icons*/
static ft_uint8_t home_star_icon[] = {0x78,0x9C,0xE5,0x94,0xBF,0x4E,0xC2,0x40,0x1C,0xC7,0x7F,0x2D,0x04,0x8B,0x20,0x45,0x76,0x14,0x67,0xA3,0xF1,0x0D,0x64,0x75,0xD2,0xD5,0x09,0x27,0x17,0x13,0xE1,0x0D,0xE4,0x0D,0x78,0x04,0x98,0x5D,0x30,0x26,0x0E,0x4A,0xA2,0x3E,0x82,0x0E,0x8E,0x82,0xC1,0x38,0x62,0x51,0x0C,0x0A,0x42,0x7F,0xDE,0xB5,0x77,0xB4,0x77,0x17,0x28,0x21,0x26,0x46,0xFD,0x26,0xCD,0xE5,0xD3,0x7C,0xFB,0xBB,0xFB,0xFD,0xB9,0x02,0xCC,0xA4,0xE8,0x99,0x80,0x61,0xC4,0x8A,0x9F,0xCB,0x6F,0x31,0x3B,0xE3,0x61,0x7A,0x98,0x84,0x7C,0x37,0xF6,0xFC,0xC8,0xDD,0x45,0x00,0xDD,0xBA,0xC4,0x77,0xE6,0xEE,0x40,0xEC,0x0E,0xE6,0x91,0xF1,0xD2,0x00,0x42,0x34,0x5E,0xCE,0xE5,0x08,0x16,0xA0,0x84,0x68,0x67,0xB4,0x86,0xC3,0xD5,0x26,0x2C,0x20,0x51,0x17,0xA2,0xB8,0x03,0xB0,0xFE,0x49,0xDD,0x54,0x15,0xD8,0xEE,0x73,0x37,0x95,0x9D,0xD4,0x1A,0xB7,0xA5,0x26,0xC4,0x91,0xA9,0x0B,0x06,0xEE,0x72,0xB7,0xFB,0xC5,0x16,0x80,0xE9,0xF1,0x07,0x8D,0x3F,0x15,0x5F,0x1C,0x0B,0xFC,0x0A,0x90,0xF0,0xF3,0x09,0xA9,0x90,0xC4,0xC6,0x37,0xB0,0x93,0xBF,0xE1,0x71,0xDB,0xA9,0xD7,0x41,0xAD,0x46,0xEA,0x19,0xA9,0xD5,0xCE,0x93,0xB3,0x35,0x73,0x0A,0x69,0x59,0x91,0xC3,0x0F,0x22,0x1B,0x1D,0x91,0x13,0x3D,0x91,0x73,0x43,0xF1,0x6C,0x55,0xDA,0x3A,0x4F,0xBA,0x25,0xCE,0x4F,0x04,0xF1,0xC5,0xCF,0x71,0xDA,0x3C,0xD7,0xB9,0xB2,0x48,0xB4,0x89,0x38,0x20,0x4B,0x2A,0x95,0x0C,0xD5,0xEF,0x5B,0xAD,0x96,0x45,0x8A,0x41,0x96,0x7A,0x1F,0x60,0x0D,0x7D,0x22,0x75,0x82,0x2B,0x0F,0xFB,0xCE,0x51,0x3D,0x2E,0x3A,0x21,0xF3,0x1C,0xD9,0x38,0x86,0x2C,0xC6,0x05,0xB6,0x7B,0x9A,0x8F,0x0F,0x97,0x1B,0x72,0x6F,0x1C,0xEB,0xAE,0xFF,0xDA,0x97,0x0D,0xBA,0x43,0x32,0xCA,0x66,0x34,0x3D,0x54,0xCB,0x24,0x9B,0x43,0xF2,0x70,0x3E,0x42,0xBB,0xA0,0x95,0x11,0x37,0x46,0xE1,0x4F,0x49,0xC5,0x1B,0xFC,0x3C,0x3A,0x3E,0xD1,0x65,0x0E,0x6F,0x58,0xF8,0x9E,0x5B,0xDB,0x55,0xB6,0x41,0x34,0xCB,0xBE,0xDB,0x87,0x5F,0xA9,0xD1,0x85,0x6B,0xB3,0x17,0x9C,0x61,0x0C,0x9B,0xA2,0x5D,0x61,0x10,0xED,0x2A,0x9B,0xA2,0x5D,0x61,0x10,0xED,0x2A,0x9B,0xA2,0x5D,0x61,0x10,0xED,0x2A,0x9B,0xED,0xC9,0xFC,0xDF,0x14,0x54,0x8F,0x80,0x7A,0x06,0xF5,0x23,0xA0,0x9F,0x41,0xF3,0x10,0x30,0x4F,0x41,0xF3,0x18,0x30,0xCF,0xCA,0xFC,0xFF,0x35,0xC9,0x79,0xC9,0x89,0xFA,0x33,0xD7,0x1D,0xF6,0x5E,0x84,0x5C,0x56,0x6E,0xA7,0xDA,0x1E,0xF9,0xFA,0xAB,0xF5,0x97,0xFF,0x2F,0xED,0x89,0x7E,0x29,0x9E,0xB4,0x9F,0x74,0x1E,0x69,0xDA,0xA4,0x9F,0x81,0x94,0xEF,0x4F,0xF6,0xF9,0x0B,0xF4,0x65,0x51,0x08};

PROGMEM char *info[] = {  "FT800 Clocks Application",

                          "APP to demonstrate interactive Clocks,",
                          "using Clocks, Track",
                          "& RTC control."
                       }; 

#define COUNTRIES 12

char *country[] = { "India","Singapore","New Zealand","Japan","Denmark","China","Australia","Belgium","Bahrain","Italy","Norway","Germany"};						  

typedef struct 
{
    ft_uint8_t ghr;
    ft_uint8_t gmin;
    ft_uint8_t arith;	
}t_gmtprp;

t_gmtprp gmt_prp[12] = {{5,30,'+'}, {8,0,'+'}, {12,45,'+'},{9,0,'+'},{4,0,'+'},{8,0,'+'},{10,0,'+'},{1,0,'+'},{3,0,'+'},{1,0,'+'},{1,0,'+'},{1,0,'+'}};
                            
				

static struct {
  signed short dragprev;
  int vel;      // velocity
  long base;    // screen x coordinate, in 1/16ths pixel
  long limit;
} scroller;
struct
{
	ft_uint8_t Hrs;
	ft_uint16_t Mins;
	ft_uint16_t Secs;
	ft_uint16_t mSecs;
}ist,utc;
/********API to return the assigned TAG value when penup,for the primitives/widgets******/


static ft_uint8_t sk=0;
ft_uint8_t Read_Keys()
{
  static ft_uint8_t Read_tag=0,temp_tag=0,ret_tag=0;	
  Read_tag = Ft_Gpu_Hal_Rd8(phost,REG_TOUCH_TAG);
  ret_tag = NULL;
  if(Read_tag!=NULL)								// Allow if the Key is released
  {
    if(temp_tag!=Read_tag)
    {
      temp_tag = Read_tag;	
      sk = Read_tag;										// Load the Read tag to temp variable	
    }  
  }
  else
  {
    if(temp_tag!=0)
    {
      ret_tag = temp_tag;
    }  
    sk = 0;
  }
  return ret_tag;
}
/***********************API used to SET the ICON******************************************/
/*Refer the code flow in the flowchart availble in the Application Note */

ft_void_t home_setup()
{
  /*Icon  file is deflated use J1 Command to inflate the file and write into the GRAM*/
  Ft_Gpu_Hal_WrCmd32(phost,CMD_INFLATE);
  Ft_Gpu_Hal_WrCmd32(phost,250*1024L);
  Ft_Gpu_Hal_WrCmdBuf(phost,home_star_icon,sizeof(home_star_icon));
  /*Set the Bitmap properties for the ICONS*/ 
  Ft_Gpu_CoCmd_Dlstart(phost);        // start
  Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
  Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255, 255, 255));
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(13));    // handle for background stars
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(250*1024L));      // Starting address in gram
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L4, 16, 32));  // format 
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST, REPEAT, REPEAT, 512, 512  ));
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(14));    // handle for background stars
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(250*1024L));      // Starting address in gram
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L4, 16, 32));  // format 
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 32, 32  ));
  Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
  Ft_Gpu_CoCmd_Swap(phost);
  Ft_App_Flush_Co_Buffer(phost);
  Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
}



void Ft_Play_Sound(ft_uint8_t sound,ft_uint8_t vol,ft_uint8_t midi)
{
  ft_uint16_t val = (midi << 8) | sound; 
  Ft_Gpu_Hal_Wr8(phost,REG_SOUND,val);
  Ft_Gpu_Hal_Wr8(phost,REG_PLAY,1); 
}



ft_void_t Info()
{
  ft_uint16_t dloffset = 0,z;
  Ft_CmdBuffer_Index = 0;
  

  // Touch Screen Calibration
  Ft_Gpu_CoCmd_Dlstart(phost); 
  Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
  Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
  Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,FT_DispHeight/2,26,OPT_CENTERX|OPT_CENTERY,"Please tap on a dot");
  Ft_Gpu_CoCmd_Calibrate(phost,0);
  Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
  Ft_Gpu_CoCmd_Swap(phost);
  Ft_App_Flush_Co_Buffer(phost);
  Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
  // Ftdi Logo animation  
  Ft_Gpu_CoCmd_Logo(phost);
  Ft_App_Flush_Co_Buffer(phost);
  Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
  while(0!=Ft_Gpu_Hal_Rd16(phost,REG_CMD_READ)); 
  //Copy the Displaylist from DL RAM to GRAM
  dloffset = Ft_Gpu_Hal_Rd16(phost,REG_CMD_DL);
  dloffset -=4;
  Ft_Gpu_Hal_WrCmd32(phost,CMD_MEMCPY);
  Ft_Gpu_Hal_WrCmd32(phost,100000L);
  Ft_Gpu_Hal_WrCmd32(phost,RAM_DL);
  Ft_Gpu_Hal_WrCmd32(phost,dloffset);
  do
  {
    Ft_Gpu_CoCmd_Dlstart(phost);   
    Ft_Gpu_CoCmd_Append(phost,100000L,dloffset);
    //Reset the BITMAP properties used during Logo animation
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(256));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(256));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_B(0));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_C(0));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_D(0));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(256));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_F(0));  
    //Display the information with transparent Logo using Edge Strip  
    Ft_App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());	
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(219,180,150));
    Ft_App_WrCoCmd_Buffer(phost,COLOR_A(220));
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(EDGE_STRIP_A));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(0,FT_DispHeight*16));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(FT_DispWidth*16,FT_DispHeight*16));
    Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
    Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());	
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));
   // INFORMATION 
    Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,20,28,OPT_CENTERX|OPT_CENTERY,(char*)pgm_read_word(&info[0]));
    Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,60,26,OPT_CENTERX|OPT_CENTERY,(char*)pgm_read_word(&info[1]));
    Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,90,26,OPT_CENTERX|OPT_CENTERY,(char*)pgm_read_word(&info[2]));  
    Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,120,26,OPT_CENTERX|OPT_CENTERY,(char*)pgm_read_word(&info[3]));  
    Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,FT_DispHeight-30,26,OPT_CENTERX|OPT_CENTERY,"Click to play");
    //Check if the Play key and change the color
    if(sk!='P')
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
    else
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(100,100,100));
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));   
    Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE(20*16));
    Ft_App_WrCoCmd_Buffer(phost,TAG('P'));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((FT_DispWidth/2)*16,(FT_DispHeight-60)*16));
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(180,35,35));
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2II((FT_DispWidth/2)-14,(FT_DispHeight-75),14,4));
    Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
    Ft_Gpu_CoCmd_Swap(phost);
    Ft_App_Flush_Co_Buffer(phost);
    Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
  }while(Read_Keys()!='P');
  Ft_Play_Sound(0x50,255,0xc0);
  /* wait until Play key is not pressed*/ 
}

/* Init the Scroller */
static ft_void_t scroller_init(ft_uint32_t limit)
{
  scroller.dragprev = -32768;
  scroller.vel = 0;      // velocity
  scroller.base = 0;     // screen x coordinate, in 1/16ths pixel
  scroller.limit = limit;
}

/* Run the Scroller in Horizontal linearlly*/

static ft_void_t scroller_run()
{
  ft_int32_t change;

  signed short sx = Ft_Gpu_Hal_Rd16(phost,REG_TOUCH_SCREEN_XY + 2);

  if ((sx != -32768) & (scroller.dragprev != -32768)) 
  {
    scroller.vel = (scroller.dragprev - sx) << 4;
  } 
  else
  {
    change = max(1, abs(scroller.vel) >> 5);
    if (scroller.vel < 0)
      scroller.vel += change;
    if (scroller.vel > 0)
      scroller.vel -= change;
  }
  scroller.dragprev = sx;
  scroller.base += scroller.vel;
  scroller.base = max(0, min(scroller.base, scroller.limit));
}


static ft_uint8_t clk_adj = 0;
static ft_uint16_t min_val;
ft_uint8_t temp[7];
/* data converstion for rtc*/
byte decToBcd(byte val)
{
  return ( (val/10*16) + (val%10) );
}

// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return ( (val/16*10) + (val%16) );
}

/* write the data into the RTC*/

void Rtc_puts()
{
  temp[0] = 0x80|(decToBcd(ist.Secs));
  temp[1] = decToBcd(ist.Mins);
  temp[2] = decToBcd(ist.Hrs);
  hal_rtc_i2c_write(0,(ft_uint8_t*)&temp,7); 
}

// Gets the date and time
void Rtc_gets()
{
  hal_rtc_i2c_read(0,(ft_uint8_t*)&temp,3); 
  ist.Hrs = bcdToDec(temp[2] & 0x1f); 
  ist.Mins = bcdToDec(temp[1] & 0x7f); 
  ist.Secs = bcdToDec(temp[0]& 0x7f); 
}

/* API used to compute the time from the rotary value*/
static ft_uint16_t adj_time=0;
static ft_uint32_t t_adj=0;
  
  
static ft_uint16_t Get_TagRotary_Value(ft_uint16_t Tagval,ft_uint16_t range,ft_uint16_t max_rotation,ft_uint16_t ct)		// -180 for 90 
{
  ft_int32_t trackval = 0;
  static ft_int16_t thcurr=0,prevth=0,adj=0;
  static ft_int32_t adj_val= 0;
  static ft_uint16_t retval=0; 
  trackval = Ft_Gpu_Hal_Rd32(phost,REG_TRACKER);	
  if(trackval)
  {
    thcurr = (trackval >> 16);		
    if(adj == Tagval)
    {
 	adj_val += (ft_int16_t)(thcurr - prevth);
	if (adj_val < 0)
	adj_val += (max_rotation*65536L);
	if (adj_val >= (max_rotation*65536L))
	adj_val -= (max_rotation*65536L);
        retval = (adj_val/(65536L/range));
    }	
    prevth = thcurr;
    adj = trackval & 0xff;
    clk_adj = 0;
  }
  else
  {
    if(adj!=0)   clk_adj = 1;   
    else   retval = ct;   
// current time in 16bit presscion for the adjustment    
    adj_val=((ct/60)*65536L)+((ct%60)*(65536L/range)); 
    
    adj = 0;    
  }
  return retval;
}

/* api compute the standard time for the countries*/ 

void timer(ft_uint8_t name)
{
  static ft_uint8_t temp_time = 0;
  char hrs_t[2],min_t[2],*temp;
  static ft_uint16_t temp_m=0,Hrs=0;
  static ft_uint16_t temp_msecs=0,temp_secs=0;
  
  ft_uint32_t t;
  ft_int32_t Mins;
  
  Mins = gmt_prp[name].gmin; 
  Hrs =  gmt_prp[name].ghr; 
  Mins = Mins+(Hrs*60);
  
  if(gmt_prp[name].arith=='+')
  Mins = min_val+Mins;
  if(gmt_prp[name].arith=='-')
  {
    Mins = min_val-Mins;
    if(Mins<0)
    Mins = 720-Mins;
  }    
  
  utc.Hrs = (Mins/60)%12;      
  utc.Mins = (Mins)%60;
 /* sync RTC and MCU time*/
  #ifdef RTC_PRESENT
  t = millis();
  t = t%1000L;

  if(ist.Secs!=temp_secs)
  {
    temp_secs = ist.Secs;
    temp_msecs = 1000L-t;
  }
  utc.mSecs = (t+temp_msecs)%1000L;  
  #endif
//  utc.Secs = ist.Secs;
 /* if time adjustment is occured then writes into the RTC*/ 
  if(clk_adj)
  {
     clk_adj = 0;
     ist.Mins = min_val%60;
     ist.Hrs = (min_val/60)%12;
     #ifdef RTC_PRESENT
     Rtc_puts();
     #else
     t_adj = millis();
     t_adj = (t_adj/60000L)%60;
     adj_time = ((ist.Mins)+(ist.Hrs*60))-t_adj;
     #endif
  }
}
static byte istouch()
{
  return !(Ft_Gpu_Hal_Rd16(phost,REG_TOUCH_RAW_XY & 0x8000));
}
/* Clock Function*/
ft_void_t Clocks(ft_uint8_t clksize,ft_uint8_t options)
{
  ft_uint8_t 
	      clk_s = clksize/10,per_f,n_f,name,Tag,startup_time=0;

  ft_uint16_t  dx = (clk_s*2)+(2*clksize),	
	       dy = (FT_DispHeight/2)+clksize+10,
               temp_m,Hrs;

   
  ft_uint8_t  col = FT_DispWidth/dx;
  ft_int16_t Ox,Oy,sx,drag=0,prev=0,dragth=0,i,cts=0,th,pv;
  ft_int32_t track,Mins,Velocity=0;
  per_f = col;
  
  n_f = (COUNTRIES)/per_f;
  Oy = (FT_DispHeight/2);
  scroller_init(16*(COUNTRIES*(dx)-FT_DispWidth));
  timer(0);
  do
  {
    Tag = Ft_Gpu_Hal_Rd8(phost,REG_TOUCH_TAG);
    #ifdef RTC_PRESENT
    Rtc_gets(); 
    temp_m =  (ist.Mins + (ist.Hrs*60))%720L; 
    #else
    ft_uint32_t t = millis(); 
    utc.mSecs = t%1000L;
    ist.Secs = (t/1000L)%60;
    ist.Mins = (t/60000L)%60;
    ist.Hrs = (t/3600000L)%12;
    temp_m =  ((ist.Mins)+(ist.Hrs*60));
    temp_m+=adj_time; 
    temp_m%=720;
    #endif
    
    min_val = Get_TagRotary_Value(Tag,60,12,temp_m);
   if((Ft_Gpu_Hal_Rd32(phost,REG_TRACKER)&0xff)==0)
    {
	scroller_run();

	drag = scroller.base>>4;
	cts = drag/dx; 
	dragth = drag%dx;
    }else
    {
	scroller.vel = 0;
    }
    Ft_Gpu_CoCmd_Dlstart(phost); 
    Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
    Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(1));
    Ft_App_WrCoCmd_Buffer(phost,TAG(0));
    Ft_Gpu_CoCmd_Gradient(phost,0, 135, 0x000000, 0, 272, 0x605040);
    for(i=-1;i<(per_f+2);i++)
    {
        Ox = (clksize+clk_s)+(dx*i);
  	Ox-=dragth;    
        name = (COUNTRIES+i+cts)%COUNTRIES;
        timer(name);
   // Set the bg color of the clocks     
        if(Tag==name+1) Ft_Gpu_CoCmd_BgColor(phost,0x4040a0); else 
        Ft_Gpu_CoCmd_BgColor(phost,0x101040);
   // Draw the Clocks with tracker     
        Ft_App_WrCoCmd_Buffer(phost,TAG(name+1));
        Ft_Gpu_CoCmd_Clock(phost,Ox,FT_DispHeight/2,clksize,0,utc.Hrs,utc.Mins,ist.Secs,utc.mSecs);
        Ft_Gpu_CoCmd_Track(phost,Ox,FT_DispHeight/2,1,1,name+1);
    }   
    Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(0));
   // Display the Country name 
    for(i=-1;i<(per_f+2);i++)
    {
        Ox = (clksize+clk_s)+(dx*i);
  	Ox-=dragth;    
        name = (COUNTRIES+i+cts)%COUNTRIES;
        Ft_Gpu_CoCmd_Text(phost,Ox,dy,29,OPT_CENTERX,country[name]);
    }    	
    if(scroller.vel!=0)Velocity = scroller.vel;					
    Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
    Ft_Gpu_CoCmd_Swap(phost);
    Ft_App_Flush_Co_Buffer(phost);
    Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
#if defined FT_801_ENABLE
	Ft_Gpu_Hal_Sleep(30);
#endif
  }while(1);
}


#ifdef MSVC_PLATFORM
/* Main entry point */
ft_int32_t main(ft_int32_t argc,ft_char8_t *argv[])
#endif
#ifdef ARDUINO_PLATFORM
ft_void_t setup()
#endif
{
  /* Local variables */
  ft_uint8_t chipid;

  Ft_Gpu_HalInit_t halinit;

  halinit.TotalChannelNum = 1;
  Ft_Gpu_Hal_Init(&halinit);
  host.hal_config.channel_no = 0;
#ifdef MSVC_PLATFORM_SPI
  host.hal_config.spi_clockrate_khz = 12000; //in KHz
#endif
#ifdef ARDUINO_PLATFORM_SPI
  host.hal_config.spi_clockrate_khz = 4000; //in KHz
#endif
  Ft_Gpu_Hal_Open(&host);

  phost = &host;

  /* Do a power cycle for safer side */
  Ft_Gpu_Hal_Powercycle(phost,FT_TRUE);
  Ft_Gpu_Hal_Rd16(phost,RAM_G);
  /* Set the clk to external clock */
  Ft_Gpu_HostCommand(phost,FT_GPU_EXTERNAL_OSC);  
  Ft_Gpu_Hal_Sleep(10);


  /* Switch PLL output to 48MHz */
  Ft_Gpu_HostCommand(phost,FT_GPU_PLL_48M);  
  Ft_Gpu_Hal_Sleep(10);

  /* Do a core reset for safer side */
  Ft_Gpu_HostCommand(phost,FT_GPU_CORE_RESET);
  //Ft_Gpu_CoreReset(phost);

  /* Access address 0 to wake up the FT800 */
  Ft_Gpu_HostCommand(phost,FT_GPU_ACTIVE_M);  

  {
    //Read Register ID to check if FT800 is ready. 
    chipid = Ft_Gpu_Hal_Rd8(phost, REG_ID);
    while(chipid != 0x7C)
      chipid = Ft_Gpu_Hal_Rd8(phost, REG_ID);
#ifdef MSVC_PLATFORM
    printf("VC1 register ID after wake up %x\n",chipid);
#endif
  }
  /* Configuration of LCD display */
#ifdef SAMAPP_DISPLAY_QVGA	
  /* Values specific to QVGA LCD display */
  FT_DispWidth = 320;
  FT_DispHeight = 240;
  FT_DispHCycle =  408;
  FT_DispHOffset = 70;
  FT_DispHSync0 = 0;
  FT_DispHSync1 = 10;
  FT_DispVCycle = 263;
  FT_DispVOffset = 13;
  FT_DispVSync0 = 0;
  FT_DispVSync1 = 2;
  FT_DispPCLK = 8;
  FT_DispSwizzle = 2;
  FT_DispPCLKPol = 0;
#endif

  Ft_Gpu_Hal_Wr16(phost, REG_HCYCLE, FT_DispHCycle);
  Ft_Gpu_Hal_Wr16(phost, REG_HOFFSET, FT_DispHOffset);
  Ft_Gpu_Hal_Wr16(phost, REG_HSYNC0, FT_DispHSync0);
  Ft_Gpu_Hal_Wr16(phost, REG_HSYNC1, FT_DispHSync1);
  Ft_Gpu_Hal_Wr16(phost, REG_VCYCLE, FT_DispVCycle);
  Ft_Gpu_Hal_Wr16(phost, REG_VOFFSET, FT_DispVOffset);
  Ft_Gpu_Hal_Wr16(phost, REG_VSYNC0, FT_DispVSync0);
  Ft_Gpu_Hal_Wr16(phost, REG_VSYNC1, FT_DispVSync1);
  Ft_Gpu_Hal_Wr8(phost, REG_SWIZZLE, FT_DispSwizzle);
  Ft_Gpu_Hal_Wr8(phost, REG_PCLK_POL, FT_DispPCLKPol);
  Ft_Gpu_Hal_Wr8(phost, REG_PCLK,FT_DispPCLK);//after this display is visible on the LCD
  Ft_Gpu_Hal_Wr16(phost, REG_HSIZE, FT_DispWidth);
  Ft_Gpu_Hal_Wr16(phost, REG_VSIZE, FT_DispHeight);

  /* Initially fill both ping and pong buffer */
  Ft_Gpu_Hal_Wr8(phost, REG_GPIO_DIR,0xff);
  Ft_Gpu_Hal_Wr8(phost, REG_GPIO,0x0ff);
  /* Touch configuration - configure the resistance value to 1200 - this value is specific to customer requirement and derived by experiment */
  Ft_Gpu_Hal_Wr16(phost, REG_TOUCH_RZTHRESH,1200);



  /*It is optional to clear the screen here*/
  Ft_Gpu_Hal_WrMem(phost, RAM_DL,(ft_uint8_t *)FT_DLCODE_BOOTUP,sizeof(FT_DLCODE_BOOTUP));
  Ft_Gpu_Hal_Wr8(phost, REG_DLSWAP,DLSWAP_FRAME);

  Ft_Gpu_Hal_Sleep(1000);//Show the booting up screen.
  #ifdef RTC_PRESENT
  hal_rtc_i2c_init();
  byte WriteByte = 0x03;
  hal_rtc_i2c_write(0x07, &WriteByte,1);
  hal_rtc_i2c_read(0x00, &WriteByte,1);
  if((WriteByte&0x80)==0)
  {
    WriteByte = 0x80;
    hal_rtc_i2c_write(0x00, &WriteByte,1);
  }
  #endif

  home_setup();
  Info();
  
  Clocks(80,0);   
/* Close all the opened handles */
    Ft_Gpu_Hal_Close(phost);
    Ft_Gpu_Hal_DeInit();
#ifdef MSVC_PLATFORM
	return 0;
#endif
}

void loop()
{
}


/* Nothing beyond this */












