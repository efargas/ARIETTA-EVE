/*

* Copyright (c) Future Technology Devices International 2014

THIS SOFTWARE IS PROVIDED BY FUTURE TECHNOLOGY DEVICES INTERNATIONAL LIMITED "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
FUTURE TECHNOLOGY DEVICES INTERNATIONAL LIMITED BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES LOSS OF USE, DATA, OR PROFITS OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

FTDI DRIVERS MAY BE USED ONLY IN CONJUNCTION WITH PRODUCTS BASED ON FTDI PARTS.

FTDI DRIVERS MAY BE DISTRIBUTED IN ANY FORM AS LONG AS LICENSE INFORMATION IS NOT MODIFIED.

IF A CUSTOM VENDOR ID AND/OR PRODUCT ID OR DESCRIPTION STRING ARE USED, IT IS THE
RESPONSIBILITY OF THE PRODUCT MANUFACTURER TO MAINTAIN ANY CHANGES AND SUBSEQUENT WHQL
RE-CERTIFICATION AS A RESULT OF MAKING THESE CHANGES.
 */


#include "FT_Platform.h"
#ifdef MSVC_PLATFORM
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <windows.h>
#include <io.h>
#include <direct.h>
#include <time.h>
#endif

#define REPORT_FRAMES   0
#define SUBDIV  4
#define YY      (480 / SUBDIV)

/* Global variables for display resolution to support various display panels */
/* Default is WQVGA - 480x272 */
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

ft_float_t transform_m,transform_c;
ft_float_t m_min = 13.0/65536;

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
   if (Ft_DlBuffer_Index> 0)
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

ft_void_t Ft_BootupConfig()
{

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

	/* Access address 0 to wake up the FT800 */
	Ft_Gpu_HostCommand(phost,FT_GPU_ACTIVE_M);  

    Ft_Gpu_Hal_Wr8(phost, REG_GPIO_DIR,0x80 | Ft_Gpu_Hal_Rd8(phost,REG_GPIO_DIR));
    Ft_Gpu_Hal_Wr8(phost, REG_GPIO,0x080 | Ft_Gpu_Hal_Rd8(phost,REG_GPIO));
	
	{
		ft_uint8_t chipid;
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


    /* Touch configuration - configure the resistance value to 1200 - this value is specific to customer requirement and derived by experiment */
    Ft_Gpu_Hal_Wr16(phost, REG_TOUCH_RZTHRESH,1200);
    Ft_Gpu_Hal_Wr8(phost, REG_GPIO_DIR,0xff);
    Ft_Gpu_Hal_Wr8(phost, REG_GPIO,0x0ff);
}

/* Boot up for FT800 */
/* Initial boot up DL - make the back ground green color */
const ft_uint8_t FT_DLCODE_BOOTUP[12] = 
{
  255,255,255,2,//GPU instruction CLEAR_COLOR_RGB
  7,0,0,38, //GPU instruction CLEAR
  0,0,0,0,  //GPU instruction DISPLAY
};

ft_void_t Info()
{
  ft_uint16_t dloffset = 0,z;
  Ft_CmdBuffer_Index = 0;
  Ft_Gpu_CoCmd_Dlstart(phost); 
  Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
  Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
  Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,FT_DispHeight/2,28,OPT_CENTERX|OPT_CENTERY,"Please tap on a dot");
  Ft_Gpu_CoCmd_Calibrate(phost,0);
  Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
  Ft_Gpu_CoCmd_Swap(phost);
  Ft_App_Flush_Co_Buffer(phost);
  Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
}
ft_void_t read_extended(ft_int16_t sx[5], ft_int16_t sy[5])
{
  ft_uint32_t sxy0, sxyA, sxyB, sxyC;
  sxy0 = Ft_Gpu_Hal_Rd32(phost,REG_CTOUCH_TOUCH0_XY);
  sxyA = Ft_Gpu_Hal_Rd32(phost,REG_CTOUCH_TOUCH1_XY);
  sxyB = Ft_Gpu_Hal_Rd32(phost,REG_CTOUCH_TOUCH2_XY);
  sxyC = Ft_Gpu_Hal_Rd32(phost,REG_CTOUCH_TOUCH3_XY);

  sx[0] = sxy0 >> 16;
  sy[0] = sxy0;
  sx[1] = sxyA >> 16;
  sy[1] = sxyA;
  sx[2] = sxyB >> 16;
  sy[2] = sxyB;
  sx[3] = sxyC >> 16;
  sy[3] = sxyC;

  sx[4] = Ft_Gpu_Hal_Rd16(phost,REG_CTOUCH_TOUCH4_X);
  sy[4] = Ft_Gpu_Hal_Rd16(phost,REG_CTOUCH_TOUCH4_Y);
}

ft_void_t set(ft_int32_t x0, ft_int16_t y0,
           ft_int32_t x1, ft_int16_t y1) {
    ft_int32_t xd = x1 - x0;
    ft_int16_t yd = y1 - y0;
    transform_m = yd / (ft_float_t)xd;
    if (transform_m < m_min)
       transform_m = m_min;
    transform_c = y0 - transform_m * x0;
  }
ft_void_t sset(ft_int32_t x0, ft_int16_t y0) 
{
    transform_c = (ft_float_t)y0 - transform_m * x0;
}
ft_int16_t m2s(ft_int32_t x) 
{
    return (ft_int16_t)(transform_m * x + transform_c);
}
ft_int32_t s2m(ft_int16_t y) 
{
    return (ft_int32_t)(y - transform_c) / transform_m;
}

FT_PROGMEM ft_prog_uint16_t sintab[257] = {
0, 402, 804, 1206, 1608, 2010, 2412, 2813, 3215, 3617, 4018, 4419, 4821, 5221, 5622, 6023, 6423, 6823, 7223, 7622, 8022, 8421, 8819, 9218, 9615, 10013, 10410, 10807, 11203, 11599, 11995, 12390, 12785, 13179, 13573, 13966, 14358, 14750, 15142, 15533, 15923, 16313, 16702, 17091, 17479, 17866, 18252, 18638, 19023, 19408, 19791, 20174, 20557, 20938, 21319, 21699, 22078, 22456, 22833, 23210, 23585, 23960, 24334, 24707, 25079, 25450, 25820, 26189, 26557, 26924, 27290, 27655, 28019, 28382, 28744, 29105, 29465, 29823, 30181, 30537, 30892, 31247, 31599, 31951, 32302, 32651, 32999, 33346, 33691, 34035, 34378, 34720, 35061, 35400, 35737, 36074, 36409, 36742, 37075, 37406, 37735, 38063, 38390, 38715, 39039, 39361, 39682, 40001, 40319, 40635, 40950, 41263, 41574, 41885, 42193, 42500, 42805, 43109, 43411, 43711, 44010, 44307, 44603, 44896, 45189, 45479, 45768, 46055, 46340, 46623, 46905, 47185, 47463, 47739, 48014, 48287, 48558, 48827, 49094, 49360, 49623, 49885, 50145, 50403, 50659, 50913, 51165, 51415, 51664, 51910, 52155, 52397, 52638, 52876, 53113, 53347, 53580, 53810, 54039, 54265, 54490, 54712, 54933, 55151, 55367, 55581, 55793, 56003, 56211, 56416, 56620, 56821, 57021, 57218, 57413, 57606, 57796, 57985, 58171, 58355, 58537, 58717, 58894, 59069, 59242, 59413, 59582, 59748, 59912, 60074, 60234, 60391, 60546, 60699, 60849, 60997, 61143, 61287, 61428, 61567, 61704, 61838, 61970, 62100, 62227, 62352, 62474, 62595, 62713, 62828, 62941, 63052, 63161, 63267, 63370, 63472, 63570, 63667, 63761, 63853, 63942, 64029, 64114, 64196, 64275, 64353, 64427, 64500, 64570, 64637, 64702, 64765, 64825, 64883, 64938, 64991, 65042, 65090, 65135, 65178, 65219, 65257, 65293, 65326, 65357, 65385, 65411, 65435, 65456, 65474, 65490, 65504, 65515, 65523, 65530, 65533, 65535
};

ft_int16_t rsin(ft_int16_t r, ft_uint16_t th) 
{
  ft_int32_t th4 ; ft_uint16_t s;ft_int16_t p;
  th >>= 6; // angle 0-123
  // return int(r * sin((2 * M_PI) * th / 1024.));
  th4 = th & 511;
  if (th4 & 256)
    th4 = 512 - th4; // 256->256 257->255, etc
  s = ft_pgm_read_byte_near(sintab + th4);
  p = ((ft_uint32_t)s * r) >> 16;
  if (th & 512)
    p = -p;
  return p;
}

ft_void_t plot()
{
	ft_int32_t mm[2],m,j;
	ft_uint8_t fadeout,h,i;
	ft_int16_t pixels_per_div,x,clock_r,x1;
	ft_int32_t y[YY + 1];
	ft_uint32_t x32 ;
	ft_uint16_t x2, options;

	Ft_App_WrCoCmd_Buffer(phost,STENCIL_OP(ZERO, ZERO));
	Ft_Gpu_CoCmd_Gradient(phost,0, 0, 0x202020, 0, 0x11f, 0x107fff);

	mm[0] = s2m(0);
	mm[1] = s2m(480);	 
	pixels_per_div = m2s(0x4000) - m2s(0);

	fadeout = min(255, max(0, (pixels_per_div - 32) * 16));  

	Ft_App_WrCoCmd_Buffer(phost, LINE_WIDTH(max(8, pixels_per_div >> 2)));
	for (m = mm[0] & ~0x3fff; m <= mm[1]; m += 0x4000) 
	{
		x = m2s(m);
		if ((-60 <= x) && (x <= 512)) 
		{
		   h = 3 * (7 & (m >> 14));

		  Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(0,0,0));
		  Ft_App_WrCoCmd_Buffer(phost,COLOR_A(((h == 0) ? 192 : 64)));
		  Ft_App_WrCoCmd_Buffer(phost, BEGIN(LINES));
		  Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(x*16,0));
		  Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(x*16,272*16));

		  if (fadeout) 
		  {
			x -= 1;
			Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(0xd0,0xd0,0xd0));
			Ft_App_WrCoCmd_Buffer(phost,COLOR_A(fadeout));
			Ft_Gpu_CoCmd_Number(phost,x, 0, 26, OPT_RIGHTX | 2, h);
			Ft_Gpu_CoCmd_Text(phost,x, 0, 26, 0, ":00");
		  }
		}
	}
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));

	for (i = 0; i < (YY + 1); i++) 
	{
		x32 = s2m(SUBDIV * i);
		x2 = (ft_uint16_t)x32 + rsin(7117, x32);
		y[i] = 130 * 16 + rsin(1200, (217 * x32) >> 8) + rsin(700, 3 * x2);
	}

	Ft_App_WrCoCmd_Buffer(phost,STENCIL_OP(INCR, INCR));
	Ft_App_WrCoCmd_Buffer(phost, BEGIN(EDGE_STRIP_B));
	for (j = 0; j < (YY + 1); j++)
	{
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(16 * SUBDIV * j, y[j]));
	}
	Ft_App_WrCoCmd_Buffer(phost,STENCIL_FUNC(EQUAL, 1, 255));
	Ft_App_WrCoCmd_Buffer(phost,STENCIL_OP(KEEP, KEEP));
	Ft_Gpu_CoCmd_Gradient(phost,0, 0, 0xf1b608, 0, 220, 0x98473a);

	Ft_App_WrCoCmd_Buffer(phost,STENCIL_FUNC(ALWAYS, 1, 255));
	Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(0xE0,0xE0,0xE0));
	Ft_App_WrCoCmd_Buffer(phost, LINE_WIDTH(24));
	Ft_App_WrCoCmd_Buffer(phost, BEGIN(LINE_STRIP));

	for (j = 0; j < (YY + 1); j++)
	{
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(16 * SUBDIV * j, y[j]));
	}

	clock_r = min(24, pixels_per_div >> 2);

	if (clock_r > 4) 
	{
		Ft_App_WrCoCmd_Buffer(phost,COLOR_A(200));
		Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(0xff,0xff,0xff));
		options = OPT_NOSECS | OPT_FLAT;
		if (clock_r < 10)
		  options |= OPT_NOTICKS;
		for (m = mm[0] & ~0x3fff; m <= mm[1]; m += 0x4000) 
		{
		  x1 = m2s(m);
		  h = 3 * (3 & (m >> 14));
		  if(x1 >= -1024)
			Ft_Gpu_CoCmd_Clock(phost,x1, 270 - 24, clock_r, options, h, 0, 0, 0);
		}
	}
}

#if defined(MSVC_PLATFORM) || defined(ARIETTA)
/* Main entry point */
ft_int32_t main(ft_int32_t argc,ft_char8_t *argv[])
#endif
#if defined(ARDUINO_PLATFORM) || defined(MSVC_FT800EMU)
ft_void_t setup()
#endif
{
	char *path = "..\\..\\..\\Test";
	ft_int16_t sx[5], sy[5];
	ft_int32_t f,j;
	static ft_int32_t _f;
	ft_int16_t cx = 0, cy = 0;
	static ft_bool_t down[2]={0,0};
	static ft_int32_t m[2]={0UL,0UL};

	ft_uint8_t n=0,i=0;
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

    Ft_BootupConfig();
	printf("reg_touch_rz =0x%x ", Ft_Gpu_Hal_Rd16(phost, REG_TOUCH_RZ));
	printf("reg_touch_rzthresh =0x%x ", Ft_Gpu_Hal_Rd32(phost, REG_TOUCH_RZTHRESH));
	printf("reg_touch_tag_xy=0x%x",Ft_Gpu_Hal_Rd32(phost, REG_TOUCH_TAG_XY));
	printf("reg_touch_tag=0x%x",Ft_Gpu_Hal_Rd32(phost, REG_TOUCH_TAG));

	Info();
	set(0, 0, 0x10000, 480);
	Ft_Gpu_Hal_Wr8(phost,REG_CTOUCH_EXTENDED, CTOUCH_MODE_EXTENDED);

	while(1)
	{
		if (REPORT_FRAMES) {
			f = Ft_Gpu_Hal_Rd16(phost,REG_FRAMES);
			
			printf("Error : %d" , f - _f);
			_f = f;
		}
		read_extended(sx, sy);

		for (i = 0; i < 2; i++) 
		{
			if (sx[i] > -10 && !down[i]) 
			{
				down[i] = 1;
				m[i] = s2m(sx[i]);
			}
			if (sx[i] < -10)
				down[i] = 0;
		}
		if (down[0] && down[1]) 
		{
			if (m[0] != m[1])
				set(m[0], sx[0], m[1], sx[1]);      
		}
		else if (down[0] && !down[1])
			sset(m[0], sx[0]);
		else if (!down[0] && down[1])
			sset(m[1], sx[1]);

		Ft_App_WrCoCmd_Buffer(phost, CMD_DLSTART);
		plot();

		// display touches
		if (0) 
		{
			Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(0xff,0xff,0xff));
			Ft_App_WrCoCmd_Buffer(phost, LINE_WIDTH(8));
			Ft_App_WrCoCmd_Buffer(phost, BEGIN(LINES));
			for (i = 0; i < 2; i++) 
			{
				if (sx[i] > -10) 
				{
					Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(sx[i], 0,0,0));
					Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(sx[i], 272,0,0));
				}
			}
		}

		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

		Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
		Ft_Gpu_CoCmd_Swap(phost);
		Ft_Gpu_CoCmd_LoadIdentity(phost);
		Ft_App_Flush_Co_Buffer(phost);		
	}

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













