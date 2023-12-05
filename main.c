#include "device_registers.h"           // Device header
#include "clocks_and_modes.h"
#include "LPUART.h"
#include "ADC.h"
#include <stdio.h>
#include <string.h>

int lpit0_ch0_flag_counter = 0; /*< LPIT0 timeout counter */
char data=0;

void PORT_init (void) {
  PCC->PCCn[PCC_PORTC_INDEX ]|=PCC_PCCn_CGC_MASK; /* Enable clock for PORTC */
  PORTC->PCR[6]|=PORT_PCR_MUX(2);           /* Port C6: MUX = ALT2,UART1 TX */
  PORTC->PCR[7]|=PORT_PCR_MUX(2);           /* Port C7: MUX = ALT2,UART1 RX */
	
	PCC->PCCn[PCC_PORTD_INDEX ]|=PCC_PCCn_CGC_MASK; /* Enable clock for PORTD */
	
	PTD->PDDR |= 1<<15|1<<16|1<<0;	/* Data Direction = output */
	PTC->PDDR &= ~((unsigned int)1<<12&1<<13);
	PORTC->PCR[12] = PORT_PCR_MUX(1);
	PORTC->PCR[13] = PORT_PCR_MUX(1);
  PORTD->PCR[15]|=PORT_PCR_MUX(1);           /* Port D15: MUX = GPIO */
  PORTD->PCR[16]|=PORT_PCR_MUX(2);           		/* Port D16: MUX = ALT2, FTM0CH1 */
	PORTD->PCR[0]|=PORT_PCR_MUX(1);           /* Port D0: MUX = GPIO */
}

void WDOG_disable (void){
  WDOG->CNT=0xD928C520;     /* Unlock watchdog */
  WDOG->TOVAL=0x0000FFFF;   /* Maximum timeout value */
  WDOG->CS = 0x00002100;    /* Disable watchdog */
}

void FTM_init (void){

	//FTM0 clocking
	PCC->PCCn[PCC_FTM0_INDEX] &= ~PCC_PCCn_CGC_MASK;		//Ensure clk diabled for config
	PCC->PCCn[PCC_FTM0_INDEX] |= PCC_PCCn_PCS(0b010)		//Clocksrc=1, 8MHz SIRCDIV1_CLK
								| PCC_PCCn_CGC_MASK;		//Enable clock for FTM regs

//FTM0 Initialization
	FTM0->SC = FTM_SC_PWMEN1_MASK							//Enable PWM channel 1output
				|FTM_SC_PS(0);								//TOIE(timer overflow Interrupt Ena) = 0 (deafault)
															//CPWMS(Center aligned PWM Select) =0 (default, up count)
															/* CLKS (Clock source) = 0 (default, no clock; FTM disabled) 	*/
															/* PS (Prescaler factor) = 0. Prescaler = 1 					*/

	FTM0->MOD = 8000-1;									//FTM0 counter final value (used for PWM mode)
															// FTM0 Period = MOD-CNTIN+0x0001~=8000 ctr clks=4ms
															//8Mhz /1 =8MHz
	FTM0->CNTIN = FTM_CNTIN_INIT(0);


	FTM0->CONTROLS[1].CnSC |=FTM_CnSC_MSB_MASK;
	FTM0->CONTROLS[1].CnSC |=FTM_CnSC_ELSA_MASK;			/* FTM0 ch1: edge-aligned PWM, low true pulses 		*/
															/* CHIE (Chan Interrupt Ena) = 0 (default) 			*/
															/* MSB:MSA (chan Mode Select)=0b10, Edge Align PWM		*/
															/* ELSB:ELSA (chan Edge/Level Select)=0b10, low true 	*/

}

void FTM0_CH1_PWM (int i){//uint32_t i){

	FTM0->CONTROLS[1].CnV = i;//8000~0 duty; ex(7200=> Duty 0.1 / 800=>Duty 0.9)
	//start FTM0 counter with clk source = external clock (SOSCDIV1_CLK)
	FTM0->SC|=FTM_SC_CLKS(3);
}

void LPIT0_init (uint32_t delay)
{
   uint32_t timeout;

	/*!
	    * LPIT Clocking:
	    * ==============================
	    */
	  PCC->PCCn[PCC_LPIT_INDEX] = PCC_PCCn_PCS(6);    /* Clock Src = 6 (SPLL2_DIV2_CLK)*/
	  PCC->PCCn[PCC_LPIT_INDEX] |= PCC_PCCn_CGC_MASK; /* Enable clk to LPIT0 regs       */

	  /*!
	   * LPIT Initialization:
	   */
	  LPIT0->MCR |= LPIT_MCR_M_CEN_MASK;  /* DBG_EN-0: Timer chans stop in Debug mode */
	                                        /* DOZE_EN=0: Timer chans are stopped in DOZE mode */
	                                        /* SW_RST=0: SW reset does not reset timer chans, regs */
	                                        /* M_CEN=1: enable module clk (allows writing other LPIT0 regs) */

  timeout=delay* 40;
  LPIT0->TMR[0].TVAL = timeout;      /* Chan 0 Timeout period: 40M clocks */
  LPIT0->TMR[0].TCTRL |= LPIT_TMR_TCTRL_T_EN_MASK;
                                     /* T_EN=1: Timer channel is enabled */
                              /* CHAIN=0: channel chaining is disabled */
                              /* MODE=0: 32 periodic counter mode */
                              /* TSOT=0: Timer decrements immediately based on restart */
                              /* TSOI=0: Timer does not stop after timeout */
                              /* TROT=0 Timer will not reload on trigger */
                              /* TRG_SRC=0: External trigger soruce */
                              /* TRG_SEL=0: Timer chan 0 trigger source is selected*/
}

void delay_us (volatile int us){
   LPIT0_init(us);           /* Initialize PIT0 for 1 second timeout  */
   while (0 == (LPIT0->MSR & LPIT_MSR_TIF0_MASK)) {} /* Wait for LPIT0 CH0 Flag */
               lpit0_ch0_flag_counter++;         /* Increment LPIT0 timeout counter */
               LPIT0->MSR |= LPIT_MSR_TIF0_MASK; /* Clear LPIT0 timer flag 0 */
}

void uint32ToString(uint32_t value, char* result) {
    sprintf(result, "%u", value);
}

int main(void)
{
	uint32_t adcResultInMv=0;
	
  WDOG_disable();        /* Disable WDGO*/
  SOSC_init_8MHz();      /* Initialize system oscilator for 8 MHz xtal */
  SPLL_init_160MHz();    /* Initialize SPLL to 160 MHz with 8 MHz SOSC */
  NormalRUNmode_80MHz(); /* Init clocks: 80 MHz sysclk & core, 40 MHz bus, 20 MHz flash */
  SystemCoreClockUpdate();
	ADC_init();  
	FTM_init();

	
	PORT_init();           /* Configure ports */
  LPUART1_init();        /* Initialize LPUART @ 9600*/
  LPUART1_transmit_string("Running LPUART example 1\n\r");     /* Transmit char string */
  LPUART1_transmit_string("Input character to echo...\n\r"); /* Transmit char string */
	
	PTD->PSOR |= 1<<15|1<<16|1<<0; /* turn off all LEDs */

	char btn_flag = 0;
	char myspeed[10];
	int D = 0;
  for(;;) {
		
		// ADC code
		convertAdcChan(12);
		while(adc_complete()==0){}            /* Wait for conversion complete flag 	*/
		adcResultInMv = read_adc_chx();       /* Get channel's conversion results in mv */
		
			// Button 
			// PTC 12 flag : 1, PTC 13 flag : 2, no btn : 0
	  if(PTC->PDIR &(1<<12)){ 	
			btn_flag = '1';
		  PTD->PCOR |= 1<<0;
		}
		else if(PTC->PDIR & (1<<13)){
				btn_flag = '2';
				PTD->PCOR |= 1<<15;
			}
		else {
			btn_flag = '0';
			PTD->PSOR |= 1<<0|1<<15;
		}
		
		uint32ToString(adcResultInMv, myspeed);
			

    strcat(myspeed, "\n");

		LPUART1_transmit_string(myspeed);
		delay_us(60000);
  }
}


