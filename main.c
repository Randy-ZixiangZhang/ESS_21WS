#include "KeyboardHID.h"
#include "german_keyboardCodes.h"
#include "character_define.h"

/* Macros: */
#define LED1 P1_1
#define LED2 P1_0


/* Clock configuration */
XMC_SCU_CLOCK_CONFIG_t clock_config = {
	.syspll_config.p_div  = 2,
	.syspll_config.n_div  = 80,
	.syspll_config.k_div  = 4,
	.syspll_config.mode   = XMC_SCU_CLOCK_SYSPLL_MODE_NORMAL,
	.syspll_config.clksrc = XMC_SCU_CLOCK_SYSPLLCLKSRC_OSCHP,
	.enable_oschp         = true,
	.calibration_mode     = XMC_SCU_CLOCK_FOFI_CALIBRATION_MODE_FACTORY,
	.fsys_clksrc          = XMC_SCU_CLOCK_SYSCLKSRC_PLL,
	.fsys_clkdiv          = 1,
	.fcpu_clkdiv          = 1,
	.fccu_clkdiv          = 1,
	.fperipheral_clkdiv   = 1
};

/* Forward declaration of HID callbacks as defined by LUFA */
bool CALLBACK_HID_Device_CreateHIDReport(
							USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
							uint8_t* const ReportID,
							const uint8_t ReportType,
							void* ReportData,
							uint16_t* const ReportSize );

void CALLBACK_HID_Device_ProcessHIDReport(
							USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
							const uint8_t ReportID,
							const uint8_t ReportType,
							const void* ReportData,
							const uint16_t ReportSize );

void SystemCoreClockSetup(void);

/*
My Code here
*/

/*System Clock*/
uint32_t SysTickCounter = 0; //32bit for maximum
uint32_t SysTickOld = 0;
uint32_t SysTickNew = 0;

void initSysTick() {
	SysTick_Config(SystemCoreClock/1000);			
}	

extern void SysTick_Handler(){
	SysTickCounter++;
}




uint8_t password_GetNext(void);
int max_index(uint8_t,int);
//Global Variable

Character G_Password_Arr[84];//10~20 length of pass, last one enter
uint8_t G_numCharacter;

//Control Flag
bool G_isLastPassDone = 1;
bool G_shouldSend = 1;
int G_indStage = 0;
int G_indSubStage = 0;
#define numTotalSubStage 84

enum STATE {Idle,Sending,Create,Success,Summarize};
enum STATE G_CALLBACK_STATE = Create;


//LEDSTATE
bool G_isNUMon;
bool G_isCAPon;
bool G_hasNumOff = false;

//Time Measurement
uint32_t G_time_1substage[numTotalSubStage];
int G_IndrightCharac;

//temporary
uint8_t G_FailTime = 0;


/**
 * Main program entry point. This routine configures the hardware required by
 * the application, then enters a loop to run the application tasks in sequence.
 */
int main(void) {
	// Init LED pins for debugging and NUM/CAPS visual report
	XMC_GPIO_SetMode(LED1,XMC_GPIO_MODE_OUTPUT_PUSH_PULL);
	XMC_GPIO_SetMode(LED2,XMC_GPIO_MODE_OUTPUT_PUSH_PULL);
	USB_Init();

	// Wait until host has enumerated HID device
	for(int i = 0; i < 10e6; ++i)
		; 

	while (1) {

		HID_Device_USBTask(&Keyboard_HID_Interface);
	}
}

// Callback function called when a new HID report needs to be created
bool CALLBACK_HID_Device_CreateHIDReport(
							USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
							uint8_t* const ReportID,
							const uint8_t ReportType,
							void* ReportData,
							uint16_t* const ReportSize ) {
	USB_KeyboardReport_Data_t* report = (USB_KeyboardReport_Data_t *)ReportData;
	*ReportSize = sizeof(USB_KeyboardReport_Data_t);
	
	//Control Logic, Should we send?
	
	//Which password should I send?

	/*Control Logic give feedback to main loop
	wait for callback to finish sending or keep going
	*/
	static uint8_t characterSent = 0, 
			indexToSend = 0;


	//Determine which state
	switch(G_CALLBACK_STATE){
		case Create:
			//first stage, two characters
			if(G_indStage == 0){
				G_numCharacter = 2;
			}
			else if(G_indStage > 0){
				G_numCharacter = G_indStage;
			}
			
			if(G_indSubStage < numTotalSubStage){
				for(int i=0;i<G_numCharacter;i++){
					G_Password_Arr[i] = G_CHARAC_ARR[G_indSubStage];
					}
				G_Password_Arr[G_numCharacter] = G_ENTER;
				G_indSubStage++;
			}
			SysTickOld = SysTickCounter;

			G_CALLBACK_STATE = Sending;
		break;

		case Sending:


			if(indexToSend < (G_numCharacter+1)) {
				if(characterSent) {
					report->Modifier = 0; 
					report->Reserved = 0; 
					report->KeyCode[0] = 0; 
					characterSent = 0;
					++indexToSend; //increment of index
				} else {
					report->Modifier = G_Password_Arr[indexToSend].mod; 
					report->Reserved = 0; 
					report->KeyCode[0] = G_Password_Arr[indexToSend].key; 
					characterSent = 1;

					//wait a little bit for the console to print
					for(int i = 0; i < 10e5; ++i)
					; 
				}
			}
			else if (indexToSend == (G_numCharacter+1)){
				//now characterSent = 0
				indexToSend = 0;//reset of index
				G_CALLBACK_STATE = Idle;//so callback stops sending
			}
			
		break;

		case Idle:
			//this if only enters if LED off signal is received
			if(G_hasNumOff  & (G_FailTime < numTotalSubStage)){
				G_FailTime++;
				G_hasNumOff = false;
				SysTickNew = SysTickCounter;
				G_time_1substage[G_indSubStage-1] = SysTickNew - SysTickOld;

				//Summarize the substage
				// if (G_FailTime == (numTotalSubStage - 1)){
				// 	G_IndrightCharac = max_index(G_time_1substage,numTotalSubStage);
				// }
				G_CALLBACK_STATE = Create;
			}		
			break;
	}





	return true;
}


uint8_t password_GetNext(){



int max_index(uint32_t *a, int n)
{
	if(n <= 0) return -1;
	int i, max_i = 0;
	uint32_t max = a[0];
	for(i = 1; i < n; ++i){
		if(a[i] > max){
			max = a[i];
			max_i = i;
		}
	}
	return max_i;
}

//	return password;
}

// Called on report input. For keyboard HID devices, that's the state of the LEDs
void CALLBACK_HID_Device_ProcessHIDReport(
						USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
						const uint8_t ReportID,
						const uint8_t ReportType,
						const void* ReportData,
						const uint16_t ReportSize ) {
	uint8_t *report = (uint8_t*)ReportData;

	if(*report & HID_KEYBOARD_LED_NUMLOCK) {
		XMC_GPIO_SetOutputHigh(LED1);
		G_isNUMon = true;
	}
	else{
		XMC_GPIO_SetOutputLow(LED1);
		G_isNUMon = false;
		G_hasNumOff = true;
	}

	if(*report & HID_KEYBOARD_LED_CAPSLOCK) {
		XMC_GPIO_SetOutputHigh(LED2);
		G_isCAPon = true;
	}
	else{
		XMC_GPIO_SetOutputLow(LED2);
		G_isCAPon = false;
	}

}

void SystemCoreClockSetup(void) {
	/* Setup settings for USB clock */
	XMC_SCU_CLOCK_Init(&clock_config);

	XMC_SCU_CLOCK_EnableUsbPll();
	XMC_SCU_CLOCK_StartUsbPll(2, 64);
	XMC_SCU_CLOCK_SetUsbClockDivider(4);
	XMC_SCU_CLOCK_SetUsbClockSource(XMC_SCU_CLOCK_USBCLKSRC_USBPLL);
	XMC_SCU_CLOCK_EnableClock(XMC_SCU_CLOCK_USB);

	SystemCoreClockUpdate();
}