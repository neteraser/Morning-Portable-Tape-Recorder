README.

(c) Copyright don Eugene Valery Khomchenko 2026, Y-Workshop, #yworkshopru, Morning Blessing, Morning UWFA.

Feel free to check out, use and applicate any part of this free and opensource project.

The history of the recording world is great.

* v.0.3 -- initial release.

* v.0.4 -- added basic functions.

In this version of the recorder, the global QFX regulator is introduced. You can regulate the depth of the inner recorder's processing and effects you apply a little.

HDD rewrite mode can be switched ON and OFF because not all the studio-technicians love to put an HDD to a needle like a tape.

Remastering effect is auto-offsetting the remastered track using a BEEP sound in the beginning of the record, and mixing it with the clean (unmastered) track in the QFX proportion.

Added support for a long device list.

Bugfixing.

* v.0.5

The Recorder has introduced it's own audio library utilizing a little simpler workaround with the DirectSound which satisfies the needs of the application a little better than the standard BASS Library due to it's narrower specialization.

You can choose the library to be used in the settings of the app.

Also some Audio Render / Rerender function has been implemented for the first time using Windows File Mapping method.

This application is becoming a professional audio recorder comparable to such app as the Fender Studio app, and it will become stable.

* v.0.5.1

Added Reverb & Superfi functions. The Morning Reverb effect is based on the classic 8x4 SoX Library reverb effect, which was extended to 16x8 reverb tails. Superfi function is the dance of our recorder, just making the whole sound a little smoother.

Bugfixing, the recorder's error is better.

* v.0.5.2

Introducing the Debug Screen on which you can know whether your recorder is a troll or joush in music, stable one or blinking one, this depends on the quality of your studio's setup.

* v.0.5.3 

Version 0.5.3 is introducing the first recording stabilizer, which is making less or more HDD rewriters depending on the state of your recorder. If your recorder is a troll, it makes less HDD rewrites, if it is a joush it makes more HDD rewrites.

Bugfixing, and this is a pretty stable release.

* v.0.6.0

Added in the version 0.6.0 is selection of a region inside the recorded track. Now you can cut in, cut out and mute the track.

Also introduced is a very important part of the original Columbia/RCA Records editor which is the AutoNote filter correcting the tempo of a track by automatically catching notes in your record and adjusting their length. You can check this does work nice.

* v.0.7.0

Versions 0.7.0 is adding spectral view and editing:
1. Mr. Tonor spectral screen is available by calling F2. You can save the summary as a PNG image.
2. Simplest noice reduction effect based on the spectral substraction using CatmullRom1D and InverseCatmullRom1D functions.
3. Added is a standalone Render button working with the center frequecy at 8kHz (cinematographic render).
4. Now you can import your own track from a WAV file if you want to color it in the editor, to do so you gotta empty the existing track.

Small bugfixes, but this is it so far.

* v.0.7.1

Noise Reduction effect was improved, the mathematical task is solved.

This version also includes the AudioFFT library and is drawing the FFT spectrum on the Tonor screen, hence why it is now the contemporary Tonor screen.

* v.0.8.0

Major GUI update, now we're using a little of Win32 API's dialog boxes, but the MFC classes are still switched off, and this is still a true studio app, which is rendering itself through the Direct3D interface.

Enjoy!

*v.0.9.0

Version v.0.9.0 intoduces the MorningFX DLL Extension interface, allowing you to create your own effects as DLLs for this application in one touch. The interface is very simple, very transparent, it's kind of DarkBasic level, and this is the best way to start programming audioeffects.

We're still debugging the app, it's not yet the release.

v.0.9.1

Another MFX is provided to check the plugin interface does work as expected. Today it'a Signal Generation Plugin, generating 6 frequencies I use for debugging tube amplifier for electric guitar.

The next minor releases v.0.9.1 --> v.1.0.0 will be extensive bug fixes, performace enhancements etc.

We're preparing the release of some Legacy Application to be held exactly the way I've designed it for a long period of time.

v.0.9.2

Neater graphics, some GFXs (fireworks), bugfixes, performance fixes.

We're preparing the release.