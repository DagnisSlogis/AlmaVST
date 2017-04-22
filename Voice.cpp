#include "Voice.h"

double Voice::nextSample() {
	// Check if voice is active, returns muted output
	if (!isActive) return 0.0;


	// Gets next sample from bouth osc multiplied by the voices
	double oscillatorOneOutput = mOscillatorOne.nextSample() * mOscillatorOneVoiceCount;
	double oscillatorTwoOutput = mOscillatorTwo.nextSample() * mOscillatorTwoVoiceCount;
	// Sums it up
	double oscillatorSum = ((1 - mOscillatorMix) * oscillatorOneOutput) + (mOscillatorMix * oscillatorTwoOutput);

	double volumeEnvelopeValue = mVolumeEnvelope.nextSample();
	double filterEnvelopeValue = mFilterEnvelope.nextSample();

	mFilter.setCutoffMod(filterEnvelopeValue * mFilterEnvelopeAmount + mLFOValue * mFilterLFOAmount);

	mOscillatorOne.setPitchMod(mLFOValue * mOscillatorOnePitchAmount);
	mOscillatorTwo.setPitchMod(mLFOValue * mOscillatorTwoPitchAmount);

	return mFilter.process(oscillatorSum * volumeEnvelopeValue * mVelocity / 127.0);
}

void Voice::setFree() {
	isActive = false;
}

void Voice::reset() {
	mNoteNumber = -1;
	mVelocity = 0;
	mOscillatorOne.reset();
	mOscillatorTwo.reset();
	mVolumeEnvelope.reset();
	mFilterEnvelope.reset();
	mFilter.reset();
}