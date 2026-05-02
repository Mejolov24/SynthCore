# SynthCore

A simple high-performance PCM synthesis engine for 32-bits CPUs.

## Thecnical aspects
* Free standing C++
Only uses two core libraries: sdtint and cmath, allowing for compilation onto almost any architecture, No hardware specific code.
Stack only, no heap memory.
* Customizable Channels and Voice amount at the preprocessor (default 32V and 16CH
* Voice looping
* Customizable Percussion channel (default 10)
* independent voice SampleData from channel
* Double buffering
Provides a helper to get and update the buffers, allowing for seamless - jitter free audio.
* Fixed-point arithmetic for fast processing on CPUs without dedicated FPU
* Pre calculated LUTs for power()
 Used at note to index speed calculations, allowing for fast and lightweight playback, costing almost no CPU usage
* Per channel parameters 
(see ChannelParameters)
