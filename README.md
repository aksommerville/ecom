# Economy of Motion

A wee platformer where you are restricted to under 13 keystrokes.
For js13k 2024, theme "TRISKAIDEKAPHOBIA".

I'm using a framework I wrote a few months ago, 
the original with more documentation is at https://github.com/aksommerville/weebpoc .
Everything is copied; no need to clone that repo to build this one.

## High Scores

- 2024-08-19 87,74: 241
- 2024-08-19 143,178: 40
- 2024-08-20: 243
- 2024-08-20: 83,75: 244
- 2024-10-09: 82,74: 247

## Further work: Build natively

It's ok to have a ridiculously fat platform layer, if it means no changes to the core.
So I want to build something that can take the existing index.html and work from there.

Audio is going to be the tricky bit.
I think the right approach is to strip Audio.js from the output completely, and implement a different synthesizer on the native end.
So during conversion of the Javascript, remove Audio.js, then have the native side provide a global class Audio.

- [x] How to define QuickJS globals? I need 'window' at least.
- - `JS_GetGlobalObject()`, `JS_SetPropertyStr()`, nothing to it.

Looking good so far. Got it bundling, executing, and receiving input.
We do indeed have a ridiculously fat platform layer: 4.4 MB for the final executable, of which 19 kB is the actual game. :P

- [x] Video
- [x] Joystick
- [x] Audio
- - [x] Different voices for song.
- - [x] Pitch bend for sound effects.
- [x] GLES2, remove GL1 (update both render and xegl)
- [ ] Is it possible to make a leaner build of QuickJS?
