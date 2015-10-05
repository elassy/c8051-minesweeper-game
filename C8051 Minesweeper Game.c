/*	Author: Youssef Elasser

	This program implements minesweeper using 9 BILEDs,
	two push buttons, and a slide switch. A potentiometer is used to set 
	the blink rate of the LEDs as a player is selecting which LED to press 
*/

#include <c8051_SDCC.h> // include files. This file is available online
#include <stdio.h>
#include <stdlib.h>

//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------
void Port_Init(void);		//Port Initialization
void ADC_Init(void); 		//ADC Initialization
unsigned char read_AD_input(unsigned char n); //Convert Analog to Digital
void Interrupt_Init(void); 	//Initialize interrupts
void Timer_Init(void);     	//Initialize Timer 0 
void Timer0_ISR(void) __interrupt 1;
unsigned char random(void); //Generate a random number
void game_play(int mine_pos_x, int mine_pos_y); //Controls game play
void debounce_wait(void); //function to insert wait for debounce of PBs
void Blink_BILED(int BILED_num, int mine_pos_x, int mine_pos_y);
void BILED_switch(int BILED_num, int BILED_state); //switches specified BILED's
												   //colors	

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

int counts;				// overflow tracking variable
int debounce_count;     // overflow variable for bouncing in PBs
int biled_count; 		// overflow variable for biled_blinking
int revealed_count;     // number of BILEDs revealed
int random_pos_x, random_pos_y; // position for mine
int LED_blink_period; // LED blink rate period in terms of counts
unsigned char digital_value; // stores the output of ADC conversion
signed char gameboard[3][3]; // 2D array to represent BILEDs
unsigned char mine_pos_num;  // stores mine position as a number from 0-8
unsigned char end = 1;   // boolean to represent if game is over or not

__sbit __at 0xB3 BILED00_A;	// BiLED00_A at Port 3.3 
__sbit __at 0xB4 BILED00_B;	// BiLED00_B at Port 3.4  
__sbit __at 0xA1 BILED01_A;	// BiLED01_A at Port 2.1 
__sbit __at 0xA2 BILED01_B;	// BiLED01_B at Port 2.2 
__sbit __at 0xA3 BILED02_A;	// BiLED02_A at Port 2.3 
__sbit __at 0xA4 BILED02_B;	// BiLED02_B at Port 2.4 
__sbit __at 0xA5 BILED10_A;	// BiLED10_A at Port 2.5 
__sbit __at 0xA6 BILED10_B;	// BiLED10_B at Port 2.6 
__sbit __at 0x91 BILED11_A;	// BiLED11_A at Port 1.1 
__sbit __at 0x92 BILED11_B;	// BiLED11_B at Port 1.2 
__sbit __at 0x93 BILED12_A;	// BiLED12_A at Port 1.3 
__sbit __at 0x84 BILED12_B;	// BiLED12_B at Port 0.4 
__sbit __at 0x94 BILED20_A;	// BiLED20_A at Port 1.4 
__sbit __at 0x95 BILED20_B;	// BiLED20_B at Port 1.5
__sbit __at 0x85 BILED21_A; // BiLED21_A at Port 1.6
__sbit __at 0x86 BILED21_B; // BiLED21_B at Port 1.7
__sbit __at 0x82 BILED22_A; // BiLED22_A at Port 0.2
__sbit __at 0x83 BILED22_B; // BiLED22_B at Port 0.3
__sbit __at 0x90 POT;       // Potentiometer at Port 1.0
__sbit __at 0xA0 SS0;       // Slide Switch 0 at Port 2.0
__sbit __at 0xB6 SS1;       // Slide Switch 1 at Port 3.6
__sbit __at 0xB7 BZR;       // Buzzer at Port 3.7
__sbit __at 0xB5 LED1;      // LED1 at Port 3.5
__sbit __at 0xB1 PB_RVL;    // PB_RVL at Port 3.1 
__sbit __at 0xB0 PB_ADV; 	// PB_ADV at Port 3.0

//*****************************************************************************

void main(void)
{
	int i,j;

	Sys_Init();      // System Initialization
	putchar(' ');    // the quote fonts may not copy correctly into SiLabs IDE
	ADC_Init();
	Port_Init();
	Interrupt_Init();
	Timer_Init();    // Initialize Timer 0 

	// Game play happens in the while(1) loop
    while (1) 
    {
    	if (SS0) // start slide switch is turned on
    	{
			// Initialize every value of the gameboard array to 0
			for (i = 0; i < 3; i++)
			{
				for (j = 0; j < 3; j++)
				{
					gameboard[i][j] = 0;
				}
			}

			printf("Slide Switch is on! \r\n");
    		// Creates two random numbers from 0-2 to generate mine position
			random_pos_x = random();
			random_pos_y = random();

			for (i = 0; i < 9; i++)
			{
				if (end)
				{	
					BILED_switch(i,0);
				}			
			}
			
			end = 0;
			BZR = 0; // Buzzer is off
			while (1)
			{
				game_play(random_pos_x, random_pos_y);
				if (end) break;
			}

			end = 1;
    	}
		else
		{
			printf("Slide Switch is off! \r\n");
		}
    }
}

//*****************************************************************************

void Port_Init(void)
{
	// Port 0 masking
	P0MDOUT |= 0x7C;

	// Port 1 masking
	P1MDIN &= ~0x01;
	P1MDOUT |= 0xFE; 
	P1MDOUT &= 0xFE;
	P1 |= ~0xFE;

	// Port 2 masking
	P2MDOUT |= 0x7E;
	P2MDOUT &= 0xFE;
	P2 |= ~0xFE;

	// Port 3 masking
	P3MDOUT |= 0xB8;
	P3MDOUT &= 0xBC;
	P3 |= ~0xBC;
}

//*****************************************************************************

void ADC_Init(void)
{
	REF0CN = 0x03;
	ADC1CN = 0x80;
	ADC1CF |= 0x01;
}

//*****************************************************************************

unsigned char read_AD_input(unsigned char n)
{
	AMX1SL = n;
	ADC1CN = ADC1CN & ~0x20;
	ADC1CN = ADC1CN | 0x10;

	while ((ADC1CN & 0x20) == 0x00);

	return ADC1;
}

//*****************************************************************************

void Interrupt_Init(void)
{
	IE |= 0x82;      // enable Timer0 Interrupt request and global interrupts
}

//*****************************************************************************

void Timer_Init(void)
{
	CKCON |= 0x08;  // Timer0 uses SYSCLK as source
	TMOD &= 0xF0;   // clear the 4 least significant bits
	TMOD |= 0x01;   // Timer0 in mode 1
	TR0 = 0;         // Stop Timer0
	TL0 = 0;         // Clear low byte of register T0
	TH0 = 0;         // Clear high byte of register T0
}

//*****************************************************************************

void Timer0_ISR(void) __interrupt 1
{
	// interrupt code to increment counter variables
	counts++;
	debounce_count++;
	biled_count++;
}

//*****************************************************************************

unsigned char random(void)
{
	return (rand()%3); // returns either 0, 1, or 2
}

//*****************************************************************************

void game_play(int mine_pos_x, int mine_pos_y)
{
	int i;
	revealed_count = 0;
	biled_count = 0;
	gameboard[mine_pos_x][mine_pos_y] = -1;
	
	digital_value = read_AD_input(0);

	LED_blink_period = (int)((((digital_value/255.0)*0.95)+.05)*337.5);

	// start the timer and set counts to 0
	counts = 0;
	TR0 = 1;
 	
 	printf("Game play is starting.\r\n");
 	
 	while (revealed_count < 8)
 	{
 		for (i = 0; i < 9; i++)
 		{
 			if (gameboard[i/3][i%3] > 0)
 				continue;

 			Blink_BILED(i, mine_pos_x, mine_pos_y);
			if (revealed_count >= 8 ) break;
 		}
 	}

 	if (revealed_count == 8)
 	{
 		printf("You win!\r\n");
		printf("Total Game time: %d seconds\r\n", (int)(counts/337.5));
		end = 1;
 	} 	
}

//*****************************************************************************

void debounce_wait(void)
{
	debounce_count = 0;
	while (debounce_count < 63); //accounts for mechanical bouncing
}

//*****************************************************************************

void Blink_BILED(int BILED_num, int mine_pos_x, int mine_pos_y)
{
	unsigned char revealed = 0;

	while (1)
	{
		// Blinks BILED alternating red/green depending on blink period 
		// set by potentiometer
		BILED_switch(BILED_num, 1);
		while (biled_count < LED_blink_period/2 && (PB_RVL && PB_ADV))	
			debounce_wait();
		
		biled_count = 0;

		BILED_switch(BILED_num, 2);
		while (biled_count < LED_blink_period/2 && (PB_RVL && PB_ADV))
			debounce_wait();

		biled_count = 0;

		// If the advance button is pressed, turn current BILED off
		// and break, moving to the next BILED
		if (!PB_ADV)
		{
			debounce_wait();
			if (!revealed)
				BILED_switch(BILED_num, 0);
			break;
		}

		if (!PB_RVL)
		{
			debounce_wait();
			BILED_switch(BILED_num, 0);
			// Revealed the mine...game is lost!
			if (gameboard[BILED_num/3][BILED_num%3] == -1)
			{
				printf("You lose!\r\n");
				printf("Total Game time: %d seconds\r\n", (int)(counts/337.5));

				revealed_count = 9;
				end = 1;
				counts = 0;
				biled_count = 0;

				while (counts < 1350)
				{
					BZR = 1;
					BILED_switch(BILED_num, 1);
					while (biled_count < LED_blink_period/2);
					BILED_switch(BILED_num, 0);
					while (biled_count < LED_blink_period);
					biled_count = 0;
				}

				BZR = 0;
				break;
			}
			
			// Logical check to see if spot is adjacent to mine
			if ((BILED_num/3 == mine_pos_x-1 && BILED_num%3 == mine_pos_y-1) ||
				(BILED_num/3 == mine_pos_x-1 && BILED_num%3 == mine_pos_y) ||
				(BILED_num/3 == mine_pos_x-1 && BILED_num%3 == mine_pos_y+1) ||
				(BILED_num/3 == mine_pos_x && BILED_num%3 == mine_pos_y-1) ||
				(BILED_num/3 == mine_pos_x && BILED_num%3 == mine_pos_y+1) ||
				(BILED_num/3 == mine_pos_x+1 && BILED_num%3 == mine_pos_y-1) ||
				(BILED_num/3 == mine_pos_x+1 && BILED_num%3 == mine_pos_y) ||
				(BILED_num/3 == mine_pos_x+1 && BILED_num%3 == mine_pos_y+1))
			{
				BILED_switch(BILED_num, 1);
				gameboard[BILED_num/3][BILED_num%3] = 1;
				revealed_count++;
			}
			else
			{
				BILED_switch(BILED_num, 2);
				gameboard[BILED_num/3][BILED_num%3] = 2;
				revealed_count++;
			}
	
			break;
			revealed = 1;
		}

		// game is paused if the slide switch is turned off, timer is paused
		// as well
		while (!SS0)
		{
			TR0 = 0;
		}

		TR0 = 1;
	}
}

//*****************************************************************************

void BILED_switch(int BILED_num, int BILED_state)
{
	if (BILED_num == 0)
	{
		if (BILED_state == 0)
		{
			BILED00_A = 1; // turn BILED off 
			BILED00_B = 1;
		}
		if (BILED_state == 1)
		{
			BILED00_A = 0; // turn BILED red
			BILED00_B = 1;
		}
		if (BILED_state == 2)
		{
			BILED00_A = 1; // turn BILED green
			BILED00_B = 0;
		}
	}

	if (BILED_num == 1)
	{
		if (BILED_state == 0)
		{
			BILED01_A = 1; // turn BILED off 
			BILED01_B = 1;
		}
		if (BILED_state == 1)
		{
			BILED01_A = 0; // turn BILED red
			BILED01_B = 1;
		}
		if (BILED_state == 2)
		{
			BILED01_A = 1; // turn BILED green
			BILED01_B = 0;
		}
	}

	if (BILED_num == 2)
	{
		if (BILED_state == 0)
		{
			BILED02_A = 1; // turn BILED off 
			BILED02_B = 1;
		}
		if (BILED_state == 1)
		{
			BILED02_A = 0; // turn BILED red
			BILED02_B = 1;
		}
		if (BILED_state == 2)
		{
			BILED02_A = 1; // turn BILED green
			BILED02_B = 0;
		}
	}

	if (BILED_num == 3)
	{
		if (BILED_state == 0)
		{
			BILED10_A = 1; // turn BILED off 
			BILED10_B = 1;
		}
		if (BILED_state == 1)
		{
			BILED10_A = 0; // turn BILED red
			BILED10_B = 1;
		}
		if (BILED_state == 2)
		{
			BILED10_A = 1; // turn BILED green
			BILED10_B = 0;
		}
	}

	if (BILED_num == 4)
	{
		if (BILED_state == 0)
		{
			BILED11_A = 1; // turn BILED off 
			BILED11_B = 1;
		}
		if (BILED_state == 1)
		{
			BILED11_A = 0; // turn BILED red
			BILED11_B = 1;
		}
		if (BILED_state == 2)
		{
			BILED11_A = 1; // turn BILED green
			BILED11_B = 0;
		}
	}

	if (BILED_num == 5)
	{
		if (BILED_state == 0)
		{
			BILED12_A = 1; // turn BILED off 
			BILED12_B = 1;
		}
		if (BILED_state == 1)
		{
			BILED12_A = 0; // turn BILED red
			BILED12_B = 1;
		}
		if (BILED_state == 2)
		{
			BILED12_A = 1; // turn BILED green
			BILED12_B = 0;
		}
	}

	if (BILED_num == 6)
	{
		if (BILED_state == 0)
		{
			BILED20_A = 1; // turn BILED off 
			BILED20_B = 1;
		}
		if (BILED_state == 1)
		{
			BILED20_A = 0; // turn BILED red
			BILED20_B = 1;
		}
		if (BILED_state == 2)
		{
			BILED20_A = 1; // turn BILED green
			BILED20_B = 0;
		}
	}

	if (BILED_num == 7)
	{
		if (BILED_state == 0)
		{
			BILED21_A = 1; // turn BILED off 
			BILED21_B = 1;
		}
		if (BILED_state == 1)
		{
			BILED21_A = 0; // turn BILED red
			BILED21_B = 1;
		}
		if (BILED_state == 2)
		{
			BILED21_A = 1; // turn BILED green
			BILED21_B = 0;
		}
	}

	if (BILED_num == 8)
	{
		if (BILED_state == 0)
		{
			BILED22_A = 1; // turn BILED off 
			BILED22_B = 1;
		}
		if (BILED_state == 1)
		{
			BILED22_A = 0; // turn BILED red
			BILED22_B = 1;
		}
		if (BILED_state == 2)
		{
			BILED22_A = 1; // turn BILED green
			BILED22_B = 0;
		}
	}
}
