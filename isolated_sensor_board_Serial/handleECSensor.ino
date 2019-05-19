/**
   capacitor based TDS measurement
   pin CapPos ----- 330 ohm resistor ----+------------+
                                         |            |
                                  10-47 nF cap     EC probe or
                                         |         resistor (for simulation)
   pin CapNeg ---------------------------+            |
                                                      |
   pin ECpin ------ 330 ohm resistor -----------------+

   So, what's going on here?
   EC - electic conductivity - is the reciprocal of the resistance of the liquid.
   So we have to measure the resistance, but this can not be done directly as running
   a DC current through an ionic liquid just doesn't work, as we get electrolysis and
   the migration of ions to the respective electrodes.

   So this routing is using the pins of the microprocessor and a capacitor to produce a
   high frequency AC current (at least 1 kHz, best 3 kHz - based on the pulse length, but the
   pulses come at intervals). Alternating the direction of the current in these
   short pulses prevents the problems mentioned above. Maximum frequency should be about
   500 kHz (i.e. pulse length about 1 microsecond).

   To get the needed timing resolution, especially for higher EC values, we measure
   clock pulses rather than using micros().

   Then to get the resistance it is not possible to measure the voltage over the
   EC probe (the normal way of measuring electrical resistance) as this drops with
   the capacitor discharging. Instead we measure the time it takes for the cap to
   discharge enough for the voltage on the pin to drop so much, that the input
   flips from High to Low state. This time taken is a direct measure of the
   resistance encountered (the cap and the EC probe form an RC circuit) in the
   system, and that's what we need to know.

   Now the working of this technique.
   Stage 1: charge the cap full through pin CapPos.
   Stage 2: let the cap drain through the EC probe, measure the time it takes from
   flipping the pins until CapPos drops LOW.
   Stage 3: charge the cap full with opposite charge.
   Stage 4: let the cap drain through the EC probe, for the same period of time as
   was measured in Stage 2 (as compensation).
   Cap is a small capacitor, in this system we use 10-47 nF but with other probes a
   larger or smaller value can be required (the original research this is based
   upon used a 3.3 nF cap). Resistor R1 is there to protect pin CapPos and
   CapNeg from being overloaded when the cap is charged up, resistor R2
   protects ECpin from too high currents caused by very high EC or shorting the
   probe.

   Pins set to input are assumed to have infinite impedance, leaking is not taken into
   account. The specs of NodeMCU give some 66 MOhm for impedance, several orders of
   magnitude above the typical 1-100 kOhm resistance encountered by the EC probe.

   Original research this is based upon:
   https://hal.inria.fr/file/index/docid/635652/filename/TDS_Logger_RJP2011.pdf

*/

volatile uint32_t dischargeCycles;
const uint8_t oversamplingRate = 6;
const uint8_t clockspeed = 16;                // 16 MHz processor.
const uint8_t ECOversampling = 6; // 64x oversampling.
            
const uint8_t CHARGEDELAY = 40;               // Time in microseconds it takes for the cap to charge; at least 5x RC.
                                              // 22 nF & 330R resistor RC = 7.25 us, times 5 = 36.3 us.
const uint8_t DISCHARGEDELAY = 15;            // Discharge the cap before flipping polarity. Better for the pins. 2xRC.
                                              
const uint16_t EC_TIMEOUT = 2000;             // Timeout for the EC measurement in microseconds.
                                              // 2 ms half cycle --> 250 Hz.
const float ALPHA = 0.02;                     // Temperature compensation factor.

void initECSensor() {
  TCCR1A = 0;                                 // Clear timer/counter control register 1A
  TCCR1B = 0;                                 // Clear timer control register
  TCCR1B |= (1 << CS10);                      // Set timer to no prescaler
  EIMSK &= ~(1 << INT0);                      // External Interrupt Mask Register - EIMSK - is for enabling INT[6;3:0] interrupts,
  //                                          // INT0 is disabled to avoid false interrupts when manipulating EICRA
  EICRA |= (1 << ISC01);                      // External Interrupt Control Register A - EICRA - defines the interrupt edge profile,
  //                                          // here configured to trigger on falling edge
}
//

uint32_t readECSensor() {

  uint32_t totalCycles = 0;
  bool timeout = false;
  EIMSK |= (1 << INT0);                       // Enable INT0.
  for (uint16_t i = 0; i < (1 << oversamplingRate); i++) {

    ///////////////////////////////////////////////////////////////////////
    // Stage 1: charge the cap, positive cycle.
    // CAPPOS: output, high.
    // CAPNEG: output, low.
    // EC: input.
    DDRD |= (1 << CAPPOS);                    // CAPPOS output.
    DDRD |= (1 << CAPNEG);                    // CAPNEG output.
    DDRD &= ~(1 << ECPIN);                    // ECPIN input.
    PORTD |= (1 << CAPPOS);                   // CAPPOS HIGH: Charge the cap.
    PORTD &= ~(1 << CAPNEG);                  // CAPNEG LOW.
    PORTD &= ~(1 << ECPIN);                   // ECPIN pull up resistor off.
    TCNT1 = 0;
    while (TCNT1 < (CHARGEDELAY * clockspeed)) {}

    ///////////////////////////////////////////////////////////////////////
    // Stage 2: measure positive discharge cycle by measuring the number of clock cycles it takes
    // for pin CAPPOS to change from HIGH to LOW.
    // CAPPOS: input.
    // CAPNEG: output, low (unchanged).
    // EC: output, low.
    dischargeCycles = 0;
    DDRD &= ~(1 << CAPPOS);                   // CAPPOS input.
    DDRD |= (1 << ECPIN);                     // ECPIN output.
    PORTD &= ~(1 << CAPPOS);                  // CAPPOS pull up resistor off.
    PORTD &= ~(1 << ECPIN);                   // ECPIN LOW.
    TCNT1 = 0;                                // Reset the timer.
    timeout = true;
    while (TCNT1 < (EC_TIMEOUT * clockspeed)) {
      if (dischargeCycles) {
        timeout = false;
        break;
      }
    }
    totalCycles += dischargeCycles;

    ///////////////////////////////////////////////////////////////////////
    // Stage 3: fully discharge the cap, prepare for negative cycle.
    // Necessary to keep total voltage within the allowed range (without these discharge cycles the voltage would jump to about +1.4*Vcc and -0.4*Vcc)
    // CAPPOS: output, low.
    // CAPNEG: output, low (unchanged).
    // EC: input.
    DDRD |= (1 << CAPPOS);                    // CAPPOS output.
    DDRD &= ~(1 << ECPIN);                    // ECPIN input.
    TCNT1 = 0;
    while (TCNT1 < (DISCHARGEDELAY * clockspeed)) {}

    ///////////////////////////////////////////////////////////////////////
    // Stage 4: charge the cap, negative cycle.
    // CAPPOS: output, low (unchanged).
    // CAPNEG: output, high.
    // EC: input (unchanged).
    PORTD |= (1 << CAPNEG);                   // CAPNEG HIGH: Charge the cap
    TCNT1 = 0;
    while (TCNT1 < (CHARGEDELAY * clockspeed)) {}

    ///////////////////////////////////////////////////////////////////////
    // Stage 5: negative discharge cycle, compensation.
    // CAPPOS: input.
    // CAPNEG: output, high (unchanged).
    // EC: output, high.
    DDRD &= ~(1 << CAPPOS);                   // CAPPOS input
    DDRD |= (1 << ECPIN);                     // ECPIN output
    PORTD |= (1 << ECPIN);                    // ECPIN HIGH

    // Delay based on dischargeCycles, making it equal in duration to the positive discharge cycle.
    TCNT1 = 0;
    while (TCNT1 < dischargeCycles) {}

    ///////////////////////////////////////////////////////////////////////
    // Stage 6: fully discharge the cap, prepare for positive cycle.
    // CAPPOS: output, high.
    // CAPNEG: ouput, high (unchanged).
    // EC: input.
    DDRD |= (1 << CAPPOS);                    // CAPPOS output.
    PORTD |= (1 << CAPPOS);                   // CAPPOS HIGH: Charge the cap.
    DDRD &= ~(1 << ECPIN);                    // ECPIN input.
    PORTD &= ~(1 << ECPIN);                   // ECPIN pull-up resistor off.
    TCNT1 = 0;
    while (TCNT1 < (DISCHARGEDELAY * clockspeed)) {}
  }

  uint32_t averageCycles;
  if (timeout) {
    averageCycles = 0;
  }
  else {
    averageCycles = (totalCycles >> oversamplingRate);
  }
  EIMSK &= ~(1 << INT0);                      // Disable INT0.

  // Disconnect the sensor.
  DDRD &= ~(1 << CAPPOS);                     // CAPPOS input.
  DDRD &= ~(1 << CAPNEG);                     // CAPNEG input.
  DDRD &= ~(1 << ECPIN);                      // ECPIN input.
  return averageCycles;
}

// ISR routine
ISR(INT0_vect) {
  dischargeCycles = TCNT1;                    // Record the value of the timer - this is the time it took for the discharge in clock cycles.
}
