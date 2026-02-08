# Kinetic Clock

**A mechanical reinterpretation of the classic 7-segment digital clock using 3D-printed servo-driven segments.**

![Kinetic Clock Project](https://github.com/user-attachments/assets/700a77c6-e80a-44d7-ac20-5e9436f11dc5)

### 2 months later...

![Kinetic Clock Project front](https://github.com/user-attachments/assets/b48bde54-1429-4b01-8a69-73d9434bc8bd)

![Kinetic Clock Project back](https://github.com/user-attachments/assets/a01bc7a2-b913-4124-be9c-ac0fb6765c09)

### final:

https://github.com/user-attachments/assets/aa5084f3-80f1-49cf-9d3b-b7fa938c8dbd



## Project Overview

The kinetic-clock is an innovative mechanical version of the classic digital clock that combines traditional digital clock aesthetics with modern mechanical engineering principles.

Inspired by instructables, this project reimagines a standard 7-segment digital display by replacing the LED segments with 3D-printed components that are physically moved by servo motors. Each segment can pop in and out to display the current time, creating a unique kinetic visual experience.

### Key Features

- **3D-printed segments** that physically move to display time
- **Servo motors** controlling individual segments
- **Real-time data** connects to wifi, relays that time onto the built-in RTC clock, and then the time is broken down and parsed into the different segments
- **Mechanical engineering** combined with digital display design
- **Custom control system** for coordinating segment movement

## How It Works

The system uses servo motors to actuate 3D-printed segments that form digits in a 7-segment display format. Unlike traditional LED displays, each segment physically moves in and out based on which digits need to be shown. A control system reads the current time and commands the appropriate servos to position the segments correctly.

## Technical Details

- **Actuation**: Servo motors (one per segment across multiple digits)
- **Display Format**: 7-segment display with mechanical segments
- **Time Source**: Real-time data integration
- **Material**: 3D-printed components
- **Controller**: Arduino-based system

## Project Status

clock currently synchronizes with real-time data and displays time.
