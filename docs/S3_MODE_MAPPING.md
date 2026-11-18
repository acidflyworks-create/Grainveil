# S3 sound mapping and bank

S3 changes mode sound, not parameter IDs, automation names, or the UI. Existing
sessions retain parameter values, but **will sound different** because mode mappings
and the factory bank changed. Factory program names and positions 0–7 are preserved,
not their previous values. Bank v2 remains in `presets/factory-v2.csv` as a reference.

## Mode multiplier table

| Mode | Size | Density | Scatter | Reverse | Pitch randomness | Feedback | Timing base + scatter | Read anchor |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- | ---: |
| Cloud | 1.80 | 2.00 | 1.35 | 0.40 | 1.00 | 0.75 | .04 + .22s | .08 |
| EchoGrain | 0.80 | 0.40 | 0.35 | 0.60 | 0.65 | 1.40 | .01 + .05s | .85 |
| ReverseSwarm | 1.60 | 1.35 | 1.10 | 1.40 | 1.00 | 1.00 | .04 + .24s | .10 |
| IceHold | 2.40 | 1.10 | 0.65 | 0.60 | 0.35 | 0.75 | .02 + .08s | .08 |
| Fracture | 0.055 | 3.20 | 1.80 | 1.10 | 1.20 | 0.85 | .20 + .55s | .12 |
| PitchBloom | 1.30 | 1.70 | 1.15 | 0.75 | 1.40 | 0.85 | .04 + .25s | .08 |
| Smear | 3.00 | 1.30 | 0.60 | 0.60 | 0.75 | 0.70 | .008 + .04s | .06 |
| Ash | 0.65 | 1.70 | 1.50 | 1.00 | 1.50 | 0.85 | .10 + .40s | .10 |

Size multiplies milliseconds. Density first normalizes the 1–40 control; density,
scatter, reverse and pitch-randomness use endpoint-preserving bias
`x*k / (1 + (k-1)*x)`, not simple linear multiplication. The density result maps
to 3–64 *requested* overlapping grains; the voice pool is limited to 32. Timing is
a bipolar proportional jitter where `s` is biased scatter. Actual start position
uses `s^0.65` to interpolate from the mode anchor to a random point in the capture
window. Feedback multiplies the physical 0–0.95 value and is capped at 0.95.

ReverseSwarm has a 70% reverse floor, including at 50% reverse; full reverse is still
100%. Its reversed grains retain S2's short edges and add a rising body. IceHold
adds a 0.09 Hz position/pitch crawl when frozen, plus slight detune. Fracture bounds
actual grain sizes to 5–20 ms (subject to the 16-sample engine minimum at very low
sample rates). PitchBloom selects weighted fifths/octaves and other intervals at
least 75% of the time and adds random detuning; playback rate remains capped at
0.25–4×. Smear uses very low timing jitter. Ash sample-holds for eight samples
(previously four); the time and frequency character therefore varies with sample rate.

## Before → after range table

These are **nominal** size values for the unchanged 5–1200 ms user range, before
random size jitter and the shared sample clamp. They are not acoustic duration
measurements. The nominal multipliers are different from knob-normalized values.

| Mode | S2 nominal size (ms) | S3 nominal size (ms) | S2 → S3 density factor |
| --- | ---: | ---: | ---: |
| Cloud | 6.75–1620 | 9–2160 | 1.40 → 2.00 |
| EchoGrain | 4.5–1080 | 4–960 | 0.45 → 0.40 |
| ReverseSwarm | 6.25–1500 | 8–1920 | 1.15 → 1.35 |
| IceHold | 9–2160 | 12–2880 | 0.75 → 1.10 |
| Fracture | 1.4–336 | 5–20 | 1.80 → 3.20 |
| PitchBloom | 6–1440 | 6.5–1560 | 1.10 → 1.70 |
| Smear | 11–2640 | 15–3600 | 1.20 → 1.30 |
| Ash | 4.25–1020 | 3.25–780 | 0.90 → 1.70 |

In both versions actual grain size is clamped to 16 samples–2.5 seconds; S3
Fracture is additionally clamped to 5–20 ms after size jitter. S3 jitter is up to
±35% times biased scatter. Density endpoints remain 3 and 64 requested voices,
with 32 physically available, so the nominal overlap is not a guaranteed live voice
count. Capture range 50–4000 ms, pitch −24 to +24 semitones, spread and mix ranges,
and parameter IDs are unchanged. S2 ReverseSwarm's reverse floor was 75%; S3 is
70% with a stronger knob bias above that floor. S2 mode scatter affected size jitter
but the position calculation used the raw scatter; S3 applies the biased scatter to
both. S2 used shared feedback strength; S3 uses the table above.

At normalized knob noon JUCE's size and density skew yield approximately 304 ms
and 10.8 on the user controls (not 602.5 ms and 20.5). With scatter and reverse
at 50%, these become the following nominal *pre-jitter* sizes and requested overlap:

| Mode | Size ms | Requested overlap | Biased scatter | Reverse probability |
| --- | ---: | ---: | ---: | ---: |
| Cloud | 547 | 27.5 | 57% | 29% |
| EchoGrain | 243 | 10.2 | 26% | 38% |
| ReverseSwarm | 486 | 22.0 | 52% | 70% |
| IceHold | 730 | 19.4 | 39% | 38% |
| Fracture | 16.7 | 34.6 | 64% | 52% |
| PitchBloom | 395 | 25.2 | 53% | 43% |
| Smear | 912 | 21.5 | 38% | 38% |
| Ash | 198 | 25.2 | 60% | 50% |

Init is Cloud with mix 65%, size 160 ms, density 14, scatter 35%, feedback 16%,
and unity input/output gain. The 45 factory programs use −3 dB output (−6 dB for
24 extreme-intent designs), feedback ≤50%, and Freeze off so they can capture input.
These are conservative starting points, not a true-peak limiter or a promise against
clipping for arbitrary input/gain/feedback. Automated synthetic-signal checks are not
loudness matching or evidence that each mode is identifiable in two seconds on guitar
DI; those claims require listening in a DAW. Feed IceHold before engaging Freeze.
