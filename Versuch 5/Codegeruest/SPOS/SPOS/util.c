#include "util.h"
#include "atmega644constants.h"
#include "defines.h"
#include "os_core.h"
#include "os_input.h"
#include "lcd.h"

#include <avr/io.h>
#include <avr/interrupt.h>

/*! \file
 *
 *  Contains basic utility functions used all over the system.
 *
 */

Time os_coarseSystemTime = 0;

/*!
 *  ISR that counts the number of occurred Timer 0 overflows for the getSystemTime function mainly used in delayMs.
 */
ISR(TIMER0_OVF_vect) {
    os_coarseSystemTime++;
}

/*!
 *  Get precise system Time with a precision of approx 13 us (presc/f_cpu = 256/20MHz)
 */
Time getSystemTime(void) {
    /*
     * In case Interrupts are off and the overflow flag is activated we simulate the overflow interrupt.
     * The flag signalizes, that an overflow occurred. This would have been handled by the ISR immediately
     * but since the interrupts are off, the controller will wait until they come back on. However,
     * until that point yet another overflow could occur, which we would then be unable to detect.
     */
    if ((!(SREG & (1 << 7))) && (TIFR0 & (1 << TOV0))) {
        TIFR0 |= (1 << TOV0);
        os_coarseSystemTime++;
    }

    /*
     * Here comes the actual job of this routine. We take the coarse system time which has a
     * resolution of 3.3 ms and augment it with the current TCNT0 counter register, yielding
     * a resolution of ~ 13 us.
     */
    return (((os_coarseSystemTime) << 8) | (Time)TCNT0);
}

/*!
 *  Function that may be used to wait for specific time intervals.
 *  Therefore, we calculate the relative time to wait. This value is added to the current system time
 *  in order to get the destination time. Then, we wait until a temporary variable reaches this value.
 *
 *  \param ms  The time to be waited in milliseconds (max. 65535)
 */
void delayMs(uint16_t ms) {
    Time startTime = getSystemTime();

    // The CPU frequency is defined in MHz (= 1/(1 000 000 s)) but the input value is given with the unit ms.
    // So, the CPU frequency has to be divided by the factor 1000.
    Time convertedFrequencyToMsFrequency = (Time)(F_CPU / 1000ul);
    
    // The (relative) destinationTime is now determinable.
    Time destinationTime = (((Time)ms) * convertedFrequencyToMsFrequency) / ((Time)TC0_PRESCALER);

    // DestinationTime held a relative value, now we compute the absolute time, using the startTime.
    destinationTime += startTime;
    Time now;

    /*
     * We have to distinguish two cases:
     * 1.) If the startTime is smaller than the destinationTime, we can wait until the current time (= now)
     * is bigger than startTime but smaller than the destinationTime.
     * Keep waiting if the value of now is anywhere in the sections marked with +
     *    0 |----------------S+++++++++++++++D-------| max time
     *
     * 2.) If we detect an overflow (startTime > destinationTime) then we have to wait for now reaching
     * max time (the overflow) and afterwards now reaching destinationTime
     * Keep waiting if now is anywhere in the sections marked with +
     *    0 |++++++++++++++++D---------------S+++++++| max time
     */

    if (startTime <= destinationTime) {
        do {
            now = getSystemTime();
        } while ((startTime <= now) && (now < destinationTime));
    } else {
        do {
            now = getSystemTime();
        } while ((now < destinationTime) || (startTime <= now));
    }
}
