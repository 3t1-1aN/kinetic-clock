# Kinetic Clock

**A mechanical reinterpretation of the classic 7-segment digital clock using 3D-printed servo-driven segments.**

<img width="4032" height="2268" alt="PXL_20260602_212719341 PORTRAIT" src="https://github.com/user-attachments/assets/6d89f24b-ae67-43f3-8193-8448956b8ed5" />

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
