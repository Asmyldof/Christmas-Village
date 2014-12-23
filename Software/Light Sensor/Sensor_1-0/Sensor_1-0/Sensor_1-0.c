/*
 * Sensor_1_0.c
 *
 * Created: 22-12-2014 13:36:16
 *  Author: Robert
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>


/* internal functions: */
inline static uint8_t	AddValueToStack(uint8_t Value);
inline static void StartADC();

/*
 * In the hardware in this project the values are as follows:
 * Completely dark room: 87mV   (4.4bit)
 * Absolute minimum value: 81mV   (4.1bit)
 * Gloomy room: 140mV   (7.1 bit)
 * Low lighting: 400mV    (20.4 bit)
 * Well lit room: 780mV    (39.8 bit)
 * Brightly lit room: 950mV    (48.4 bit)
 * Sunny room: 1905mV    (97.2 bit)
 */

#define		SENSOR_ADCVAL_DARK				7  // At which value will it start triggering night mode
#define		SENSOR_ADCVAL_MAXIMUM			68  // At which value will it go to maximum brightness
#define		SENSOR_ADCVAL_MINIMUM			5  // At which value will it go to minimum brightness

#define		PWM_OUT_AUTO_MINIMUM			128 //AUTO Mode minimum output PWM value
#define		PWM_OUT_AUTO_MAXIMUM			255 //AUTO Mode maximum output PWM value

// the system:
//  temp = [sensor value] - [minimum sensor value]
//  if(temp < 0) temp = 0;
//  temp = temp << [shift up by]
//  temp = temp + [pwm out minimum]
//  if( temp > [pwm out maximum] ) temp = [pwm out maximum];
//  <pwm output> = temp
#define		PWM_SENSOR_SHIFTUP_BY			1	

#define		POTENTIOMETER_DISABLE_ALL_AUTO	250		// above this level the sensor does nothing
#define		POTENTIOMETER_MANUALDIM_MAX		192		// above this level the lights are full on
#define		POTENTIOMETER_MANUALDIM_MIN		64		// 
#define		POTENTIOMETER_AUTO_SEPERATION	5		// the split value in the AUTO range

#define		PWM_OUT_SHIFTUP_BY				1


#define		ADC_SENSOR_STACKSIZE			15

#define		ADC_PRESCALER					3 // Default 1MHz gives 125kHz ADC clock.
/*
ADC Clock should be between 50kHz and 200kHz for best operation
 0 - CkIO / 2
 1 - CkIO / 2
 2 - CkIO / 4
 3 - CkIO / 8
 4 - CkIO / 16
 5 - CkIO / 32
 6 - CkIO / 64
 7 - CkIO / 128
 ** Any higher numbers will be masked down to the above series (i.e. 8 = 0, 9 = 1, etc) **
*/

#define		WATCHDOG_PRESACLE			1
/*
The ADC is started by the watchdog time-out, higher frequency gives better potentiometer response
but lower levels of filtering out noise on the sensor:
0 - 16ms
1 - 32ms
2 - 64ms
3 - 0.125s
4 - 0.25s
5 - 0.5s
6 - 1s
7 - 2s
32 - 4s
33 - 8s

Value is masked, so mistakes are protected from causing unpredictable behaviour, beyond
a strange, unexpected time-out
*/




/*
 * In the hardware setup of this project:
 * LED PWM Output = PB0 / OCR0A
 * NIGHTMODE Pin = PB3 / Reset
 * Sensor = PB1 / ADC1
 * Poti = PB2 / ADC2
 */
#define		OCR0_OUTPUTREGISTER				OCR0AL
#define		TCCR0A_VALUE					0b10000001
#define		TCCR0B_VALUE					0b00001001 // Set to prescaler 1, fast PWM 8bit


#define		PORTB_OUT_PWMPIN				(1 << PORTB0)
#define		PORTB_OUT_NIGHTMODE				(1 << PORTB3)
#define		PINB_IN_POTENTIOMETER			(1 << PINB2)
#define		PINB_IN_SENSOR					(1 << PINB1)
#define		ADC_DIDR_POTENTIOMETER			(1 << ADC2D)
#define		ADC_DIDR_SENSOR					(1 << ADC1D)
#define		ADC_ADMUX_SENSOR				0x01
#define		ADC_ADMUX_POTENTIOMETER			0x02



uint8_t		SensorStack[ADC_SENSOR_STACKSIZE];
uint8_t		PotentiometerVal;


/* automated define, no touhy! */
#if (WATCHDOG_PRESACLE > 33)
#warning 'Watchdog prescaler set to a bad value (too high), setting to 8seconds'
	#undef WATCHDOG_PRESACLE
	#define WATCHDOG_PRESACLE 33
#endif


int main(void)
{
    // during development nightmode output is disabled, to avoid triggering reset:
	// TODO: uncomment when reset disable will be set:
	DDRB = (PORTB_OUT_PWMPIN | PORTB_OUT_NIGHTMODE);
	PORTB = 0x00;
	
	// Disable the input logic on the ADC channels:
	DIDR0 = (ADC_DIDR_POTENTIOMETER | ADC_DIDR_SENSOR);
	
	ADMUX = ADC_ADMUX_POTENTIOMETER;
	// ADCSRA is set in StartConversion()
	
	// enable Watchdog interrupt and mask the prescaler:
	WDTCSR = (1<<WDIE)|(WATCHDOG_PRESACLE & 0x27);
	
	OCR0AH = 0;
	OCR0AL = 0;
	OCR0BH = 0;
	OCR0BL = 0;
	
	TCCR0A = TCCR0A_VALUE;
	TCCR0B = TCCR0B_VALUE;
	
	//enable all interrupts:
	sei();
	
	while(1)
    {
        //TODO:: Please write your application code 
    }
}


ISR(WDT_vect)
{
	WDTCSR |= (1<<WDIE);
	StartADC();
}


ISR(ADC_vect)
{
	uint8_t		temp;
	uint8_t		avg;

	temp = ADCL;
	
	if( ADMUX == ADC_ADMUX_SENSOR )
	{
		if( temp < SENSOR_ADCVAL_MINIMUM )
			temp = SENSOR_ADCVAL_MINIMUM;
		
		avg = AddValueToStack(temp);
		temp = avg;
		
		if( temp > SENSOR_ADCVAL_MAXIMUM )
			temp = SENSOR_ADCVAL_MAXIMUM;
		
		temp -= SENSOR_ADCVAL_MINIMUM;

		temp = temp << PWM_SENSOR_SHIFTUP_BY;
		
		temp += PWM_OUT_AUTO_MINIMUM;

		if( temp > PWM_OUT_AUTO_MAXIMUM )
			temp = PWM_OUT_AUTO_MAXIMUM;
		
		if( PotentiometerVal < POTENTIOMETER_MANUALDIM_MIN )
		{
			OCR0_OUTPUTREGISTER = temp;
		
			if( PotentiometerVal > POTENTIOMETER_AUTO_SEPERATION )
			{
				if( avg < SENSOR_ADCVAL_DARK )
				{
					PORTB |= PORTB_OUT_NIGHTMODE;
				}
				else
				{
					PORTB &= ~PORTB_OUT_NIGHTMODE;
				}
			}
			else
			{
				PORTB &= ~PORTB_OUT_NIGHTMODE;
			}
		}
		else if( PotentiometerVal < POTENTIOMETER_DISABLE_ALL_AUTO )
		{
			if( avg < SENSOR_ADCVAL_DARK )
			{
				PORTB |= PORTB_OUT_NIGHTMODE;
			}
			else
			{
				PORTB &= ~PORTB_OUT_NIGHTMODE;
			}
		}
		else
		{
			PORTB &= ~PORTB_OUT_NIGHTMODE;
		}
	
		ADMUX = ADC_ADMUX_POTENTIOMETER;
	}
	else
	{
		PotentiometerVal = temp;
		
		if( PotentiometerVal < POTENTIOMETER_MANUALDIM_MAX )
		{
			if( PotentiometerVal >= POTENTIOMETER_MANUALDIM_MIN )
			{ /* we are manually dimming */
				temp = PotentiometerVal - POTENTIOMETER_MANUALDIM_MIN;
				temp = temp << PWM_OUT_SHIFTUP_BY;
				OCR0_OUTPUTREGISTER = temp;
			}
		}
		else
		{
			OCR0_OUTPUTREGISTER = 255;
		}
		
		ADMUX = ADC_ADMUX_SENSOR;
	}
}


/*
 *  Puts a value onto the sensor stack, calculates the stack average and returns it.
 */
inline static uint8_t	AddValueToStack(uint8_t Value) 
{
	uint16_t	temp = Value;
	
	for( uint8_t itt = 0; itt < ADC_SENSOR_STACKSIZE; itt++ )
	{
		temp += SensorStack[itt];
		if( itt == (ADC_SENSOR_STACKSIZE - 1) )
			SensorStack[itt] = Value;
		else
			SensorStack[itt] = SensorStack[itt + 1];			
	}
	
	return (uint8_t) (temp / (ADC_SENSOR_STACKSIZE+1));
}

/*
 * Starts the ADC for a one time run
 */
inline static void StartADC()
{
	ADCSRA = ((1<<ADEN)|(1<<ADSC)|(1<<ADIE)|(ADC_PRESCALER & 0x07));
}