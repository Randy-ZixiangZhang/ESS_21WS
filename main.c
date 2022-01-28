
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
uint8_t password_GetNext(void);
//Global Variable

Character G_Password_Arr[84];//10~20 length of pass, last one enter

uint8_t G_PASSWORD[20];
unsigned char G_PASS_MODIFIER[20];

uint8_t G_numCharacter;

bool G_isLastPassDone = 1;
bool G_shouldSend = 1;

//LEDSTATE
bool G_isNUMon;
bool G_isCAPon;
bool G_hasNumOff = false;

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
		//the password is wrong
	if(G_hasNumOff  & (G_FailTime < 5)){
		G_isLastPassDone = true;
		G_shouldSend = true;
		G_FailTime++;
		G_hasNumOff = false;
	}
	//Create new password
	if(G_isLastPassDone){
			G_numCharacter = 84;
			for(int i=0;i<G_numCharacter;i++){
				G_Password_Arr[i] = G_CHARAC_ARR[i];
			}

			G_isLastPassDone = false;
	}


	static uint8_t characterSent = 0, 
				   indexToSend = 0;

	if(G_shouldSend){
		if(indexToSend < G_numCharacter) {
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
			}
		}
		else if (indexToSend == G_numCharacter){
			//now characterSent = 0
			indexToSend = 0;//reset of index
			G_shouldSend = 0;//so callback stops sending
		}
	}

	return true;
}


uint8_t password_GetNext(){

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