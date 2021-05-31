EESchema Schematic File Version 2
LIBS:SynthMod
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:PowerSupply-cache
EELAYER 25 0
EELAYER END
$Descr A 11000 8500
encoding utf-8
Sheet 1 3
Title "Dual Power Supply"
Date "2016-03-09"
Rev ""
Comp "Jim Patchell"
Comment1 "patchell@cox.net"
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Sheet
S 1600 1550 1000 1050
U 56E26CF4
F0 "Positive Regulator" 60
F1 "PosReg.sch" 60
F2 "VIN" I L 1600 1750 60 
F3 "VOUT" O R 2600 1750 60 
$EndSheet
$Sheet
S 1600 3200 1450 1350
U 56E270E4
F0 "Negative Regulator" 60
F1 "NegReg.sch" 60
F2 "VIN" I L 1600 3400 60 
F3 "Vref" I L 1600 3550 60 
F4 "VOUT" O R 3050 3400 60 
$EndSheet
$EndSCHEMATC
