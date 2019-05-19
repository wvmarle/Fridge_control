#include <avr/sleep.h>                                // Sleep Modes (allows the processor to sleep during the ADC reading).

const uint8_t PHOversampling = 5;                     // 32x oversampling - expected by the regular library.

EMPTY_INTERRUPT (ADC_vect);

void initpHSensor() {

}

uint32_t readpHSensor() {
  uint32_t pHReading = 0;                             // Total of the ADC measurements done.
  for (uint16_t i = 0; i < (1 << PHOversampling); i++) { // Take the required number of samples.
    pHReading += getReading();                        // Read the analog input.
  }
  return pHReading;
}

// Take an ADC reading in sleep mode (ADC)
uint16_t getReading () {
  ADCSRA = bit (ADEN) | bit (ADIF);  // enable ADC, turn off any pending interrupt
  ADCSRA |= bit (ADPS0) | bit (ADPS1) | bit (ADPS2);   // prescaler of 128
  ADMUX = bit (REFS0) | bit (REFS1) | (PH_PIN & 0x07);  // AVcc

  noInterrupts ();
  set_sleep_mode (SLEEP_MODE_ADC);    // sleep during sample
  sleep_enable();

  // start the conversion
  ADCSRA |= bit (ADSC) | bit (ADIE);
  interrupts ();
  sleep_cpu ();
  sleep_disable ();

  // awake again, reading should be done, but better make sure
  // maybe the timer interrupt fired
  while (bit_is_set (ADCSRA, ADSC)) {}
  return ADC;
}
