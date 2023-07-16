> ### Developed with and for PhD candidate [Fil Botelho@Orhpeus Institute](https://orpheusinstituut.be/en/orpheus-research-centre/researchers/filipa-botelho)
> This is part of the experimental_system (**expS**) repositories that are present on this github account.

# 🎵 fbcontrol~
Two Max-MSP objects for acoustic feedback (Larsen effect) manipulation.
- [An early example in practice](https://vimeo.com/394801431)

# 🖱️ Use

- **The feedback system should have (minimum specs):**
  > 1. A free to move, unidirectional, microphone (input signal)
  > 2. Some kind of time-domain to frequency-domain transform algorithm (e.g., Fast Fourier Transform (FFT))
  > 3. Frequency-domain signal manipulation (`fbcontrolresist~`/`fbcontrolreact~`)
  > 4. ...
  > 5. Some kind of signal attenuation (compression, limiting) right before signal output
  > 6. A fixed speaker (output signal)
  > 7. Enough gain

- Inside `obj_source` you will find the `C` source code for the Max-MSP objects. You will need [Max-msp SDK](https://github.com/Cycling74/max-sdk) if you want to compile from source.
- Inside `examples` you can find two Max patches that showcase both objects (`fbcontrolresist~` and `fbcontrolreact~`)
- These objects depend on [sigmund~](https://github.com/v7b1/sigmund_64bit-version) by Miller Puckette.
  - This only provides the FFT output, so any other object will work, as long as the output follows the same pattern of:
    - `[osc, freq, amp, flag]` -> For detailed info on this see [sigmund~](https://github.com/v7b1/sigmund_64bit-version)
- You can find pre-compiled versions as releases

Simply add the three objects (`sigmund~`, `fbcontrolreact~`, `fbcontrolresist~`) to the `Packages` folder of Max. It is highly recommended 
that you run a compressor (and a limiter) at the end of your signal chain. **You are dealing with feedback, 
which can easily overpower your system and cause real damage to your hearing and equipment.**

These objects are made to work on live audio. You will need at least one source on input signal and a source of output signal in you feedback system.

Detailed info on each component/parameter can be found in the examples.

# ☮️ Keep in mind

- Code comments and some other text are in Portuguese (I didn't know about best practices back then)
- This is my first C project (and probably my first programming project ever). The code is not pretty, but it works.
- Please contact me at alumiamusic@gmail.com if you want to use this, or you are interested in the idea, but cannot understand the code/examples/etc.
I am more than happy to help

# 🕵️ Detailed description

When generating acoustic feedback in a room, if we move the microphone around, we realize that different regions of the room generate 
different feedback tones. 

We also hear that tones fight for stability, transitioning between each other as we move. 

`fbcontrolresist~` and `fbcontrolreact~` manipulate the way tones appear and disappear, by dealing with the concept of **resistance**

## Resistance

Let's suppose that a tone at a given frequency (in a given region of a room) really wants to appear: whatever tone we had before, 
vanishes very rapidly as soon as we move to that region of the room. **What would happen if we imposed some resistance to this change?**

We would actually change the acoustic characteristics of the produced feedback. We would be navigating in a completely different room
than before, because acoustic feedback is self inducing, and relies on itself to modulate its own behavior.
- If the tone grows slower, it will also feed less energy to the system (in the same time period), changing its own characteristics.

**So, how do we manipulate this characteristic?** 

## Time domain

We first need to be aware of how each tone is behaving. For this, we need real time frequency information. 

We can achieve that with some kind of time-domain to frequency-domain transformation. It has to be fast enough so that we can still work live with the sound.

Constant-Q transform would be ideal, but for simplicity we used an already functioning FFT implementation by Miller Puckette ([sigmund~](https://github.com/v7b1/sigmund_64bit-version)).

## Manipulation

We now have a deconstructed signal that, hundreds of times per second, informs us about the current amplitude of all frequencies.

If we let some time pass, we get a sense of the rates at which frequencies are changing their amplitudes. We want to manipulate
these rates in ways that keep the self-inducing and self-feeding characteristics of the original acoustic feedback.

We don't want to fix the grow/decay amplitude rates to a given value. We simply want a way to impose/remove some resistance to these
self-inducing and self-feeding properties.

This is what `fbcontrolresist~` and `fbcontrolreact~` try to achieve.


## Resynthesis

Our objects output the new state of each frequency (sinusoid) for a given interval. If this interval is small enough, we can 
use this freq/amp information to perform additive synthesis with a degree of fidelity that is way more than enough for 
acoustic feedback.

This is what the final part of the example patches does: a reconstruction of the manipulated audio with additive synthesis.

## Conclusion

We effectively read, deconstructed, manipulated and reconstructed the signal in close to real time. 

The manipulation is based on the acoustic feedback tones growth/decay rates, which are not forced to any specific value.

We are reducing/increasing these rates by a ratio that is proportional to the rate itself, creating a system
that is still completely self-inducing and free to behave as it wants (within these new more resistive, or reactive environments).
