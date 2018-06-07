# VFO
Variable Frequency Oscillator for 1.0 - 30.0 mHz using Arduino to control the AD9850.

This is the Arduino program to control an evaluation board with an AD9850 chip.
Purpose: The unit is being used as a VFO for a vintage Central Electronics 20A exciter.
Why?  - There are no VFOs with a 9 mHz offset available
      - I wanted to gain experience with digital electronics

This has two modes:
1 - Ham band VFO, 20 to 160 meters for use in vintage equipment
2 - Ham band VFO for Central Electronics 20A exciter (frequencies are different than resultant frequencies)

Last update 6/29/17 - coalesce frequency change activities into one function, misc cleanups pre-blog

User controls:
a) rotary encoder changes the frequency by 1 kHz increments
b) button changes the mode
c) button changes the frequency band

Future: Add a third mode as a standard signal generator (outside of Ham bands)
