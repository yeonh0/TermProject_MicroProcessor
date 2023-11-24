/*
 * main.c       UART transmission simple example
 * 2016 Mar 17 O Romero - Initial version
 * 2016 Oct 31 SM: Clocks adjusted for 160 MHz SPLL
 *
 */

//#include "S32K144.h" /* include peripheral declarations S32K144 */
#include "device_registers.h"           // Device header
#include "clocks_and_modes.h"
#include "LPUART.h"
#include "ADC.h"

char data=0;
void PORT_init (void) {
  PCC->PCCn[PCC_PORTC_INDEX ]|=PCC_PCCn_CGC_MASK; /* Enable clock for PORTC */
  PORTC->PCR[6]|=PORT_PCR_MUX(2);           /* Port C6: MUX = ALT2,UART1 TX */
  PORTC->PCR[7]|=PORT_PCR_MUX(2);           /* Port C7: MUX = ALT2,UART1 RX */
	
	PCC->PCCn[PCC_PORTD_INDEX ]|=PCC_PCCn_CGC_MASK; /* Enable clock for PORTD */
	
	PTD->PDDR |= 1<<15|1<<16|1<<0;	/* Data Direction = output */
	PTC->PDDR &= ~((unsigned int)1<<12);
	PORTC->PCR[12] = PORT_PCR_MUX(1);
  PORTD->PCR[15]|=PORT_PCR_MUX(1);           /* Port D15: MUX = GPIO */
  PORTD->PCR[16]|=PORT_PCR_MUX(1);           /* Port D16: MUX = GPIO */
	PORTD->PCR[0]|=PORT_PCR_MUX(1);           /* Port D0: MUX = GPIO */
}

void WDOG_disable (void){
  WDOG->CNT=0xD928C520;     /* Unlock watchdog */
  WDOG->TOVAL=0x0000FFFF;   /* Maximum timeout value */
  WDOG->CS = 0x00002100;    /* Disable watchdog */
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
	
	PORT_init();           /* Configure ports */
  LPUART1_init();        /* Initialize LPUART @ 9600*/
  LPUART1_transmit_string("Running LPUART example 1\n\r");     /* Transmit char string */
  LPUART1_transmit_string("Input character to echo...\n\r"); /* Transmit char string */
	
	PTD->PSOR |= 1<<15|1<<16|1<<0; /* turn off all LEDs */
	
	int click_flag = 0;
	int btn_flag = 0;

  for(;;) {
		
		// ADC code
		convertAdcChan(12);
		while(adc_complete()==0){}            /* Wait for conversion complete flag 	*/
		adcResultInMv = read_adc_chx();       /* Get channel's conversion results in mv */
		
		if (adcResultInMv > 3750) {           /* If result > 3.75V 		*/	
			if(click_flag != 1) LPUART1_transmit_string("11111\n\r");
			click_flag = 1;
		}
		else if (adcResultInMv > 2500) {      /* If result > 2.50V 		*/
		  if(click_flag != 2) LPUART1_transmit_string("22222\n\r");
			click_flag = 2;
		}
		else if (adcResultInMv >1250) {       /* If result > 1.25V 		*/
		  if(click_flag != 3) LPUART1_transmit_string("33333\n\r");
			click_flag = 3;
		}
		else {
		  if(click_flag != 4) LPUART1_transmit_string("44444\n\r");
			click_flag = 4;
		}
		
		// Button & LED code
	  if(PTC->PDIR &(1<<12)){ 	// if button on
			if(btn_flag == 0){
			btn_flag = 1;
			LPUART1_transmit_string("Clicked!!\n\r"); /* Transmit char string */
		}
		PTD->PCOR |= 1<<0;
	}
		else {
			if(btn_flag == 1) {
				LPUART1_transmit_string("Free!!\n\r");
				btn_flag = 0;
			}
			PTD->PSOR |= 1<<0;
		}
		
		if (LPUART1_available()) {
        // ?? ?? ??
        data = LPUART1_receive_char();
        LPUART1_transmit_char(data);
        // ??? ???? ?? ?? ??
        switch(data) {
            case '1':
                PTD->PCOR |= 1<<15;
                PTD->PSOR |= 1<<16|1<<0;
						LPUART1_transmit_string(" : Red Light On!!\n\r");
                break;
            
            case '2':
                PTD->PCOR |= 1<<16;
                PTD->PSOR |= 1<<15|1<<0;
						LPUART1_transmit_string(" : Green Light On!!\n\r");
                break;
            
            case '3':
                PTD->PSOR |= 1<<15|1<<16;
						LPUART1_transmit_string(" : Light Off!!\n\r");
                break;
        }
    }
		
  }
}


