# ToneBoxFPGA

Goal: To create and explore FPGA/MCU based guitar effects solutions, creating something that is cheaper, technically advanced, and great-sounding without buying off the shelf guitar gear. 

## Big Picture Product:

An FPGA based multi-effects pedal. Users can plug in their guitars and start playing, no computer required, selecting presets with physical pedal-like buttons for different effects such as distortion, delay, reverb. 

Key features:

- configurability at home with a computer
    - users download profiles from an online community
    - users can tune the parameters of the design which are uploaded to the FPGA as weights
- Live experience identical to a single pedal
    - 5V power supply cable
    - 1/4 inch cable in and out between guitar and amp
    - stomp pedals for switching profiles/effects
- Effects modeler
    - supply the pedal with clean and modified signal, FPGA will recreate that effect and store it as a preset, allowing users to “steal” pedals

![Kria KV260 FPGA](Audio%20Project/image.png)

Kria KV260 FPGA

## Inspirations

Line 6 Multi Effects Pedal: “100 pedals in a box.” A hardware/software which can recreate the transformations from classic guitar pedals and combinations of pedals, saving them to presets and allowing guitarists to change sounds live. 

Key Features:

- Faithful recreations of classic and sought after vintage pedals and effects
- Easy switching of profiles with stomp buttons like real guitar pedals
- Users configure the pedal at home, live is plug and play
- setup is equivalent to a single effects pedal
- [a demo from Smashing Pumpkins’ Billy Corgan](https://www.youtube.com/watch?v=oUweXAt5OF4&t=181s)

![line_6_99_060_2705_pod_go_wireless_pedalboard_1630122.jpg](Audio%20Project/line_6_99_060_2705_pod_go_wireless_pedalboard_1630122.jpg)

Guitar Plugins: A software which simulates the eq characteristics of popular and sought after amplifiers. 

Key Features:

- models even tone knobs, so the gain knob on the modeled amp has the same curve and sweet spots as the real amplifier
- presets and intuitive UI, allows users to set a preset and understand how that tone is created
- models multiple amplifiers in series
- Only $200 for amplifiers and sounds that cost upwards of $10k

![Neural DSP John Mayer plugin](Audio%20Project/UI-Archetype-John-Mayer-X.jpg)

Neural DSP John Mayer plugin

Amp Profiler/modeler: A hardware/software which models the sounds of classic amplifiers, not just what they theoretically sound like, but even when mic’d up and in a large room. 

Key features

- classic and iconic sounds packaged into one small module, with presets that allow users to quickly get started with sounds from classic records/players
- customizability, allows users to change and experiment the settings of classic amps
- expensive, costing over $1k

![Kemper Profiler 600w PowerHead](Audio%20Project/KemperProfiler1.jpg.webp)

Kemper Profiler 600w PowerHead

Wireless Expression Pedal: A hardware which wirelessly sends midi packets to a computer which can assign that to any eq or setting in a plugin, allowing it to control volume, gain, treble, reverb. 

Key features:

- wireless for no wires and messy setup
- latency isn’t as much of an issue because its an expression pedal and doesn’t control the notes, but should be tested in hand
- completely configurable with a DAW to control any signal you want, customizable

![Boss EV-1-WL Wireless MIDI expression pedal](Audio%20Project/shopping.jpeg)

Boss EV-1-WL Wireless MIDI expression pedal
