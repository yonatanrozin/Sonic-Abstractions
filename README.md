# Sonic-Abstractions
A process of temporary translation from 3D shape data to a sonic wavetable, proposing an alternative definition of sound which is both strictly representational and artificial.

## Setup
- ```git clone``` this repo into a desired location on your local hard drive

### Arduino turntable setup
- These instructions use a NEMA17-style bipolar stepper motor and a [TMC2209 Motor Driver](https://biqu.equipment/products/bigtreetech-tmc2209-stepper-motor-driver-for-3d-printer-board-vs-tmc2208). __The TMC2209 motor driver has a built in microstep of 1/8. If you swap it out for a different driver, the Arduino code will have to be slightly adjusted!__ You should change the line ```#define motorStepsPerRev``` to 200.
- Connect Arduino digital pins to driver according to the top 3 lines in the [Arduino file](https://github.com/yonatanrozin/Sonic-Abstractions/blob/main/stepper_stream_angle/stepper_stream_angle.ino). You can use any digital pins, but you'll have to update the numbers in the code.
- Upload Arduino file to board

## Run
- Run ```bin/Sonic Abstractions Physical.exe```
