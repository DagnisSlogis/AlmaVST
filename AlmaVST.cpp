#include "AlmaVST.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmain"
#include "IPlug_include_in_plug_src.h"
#pragma clang diagnostic pop
#include "IControl.h"
#include "IKeyboardControl.h"
#include "resource.h"

#include <math.h>
#include <algorithm>

#include <functional>

const int kNumPrograms = 5;

const double parameterStep = 0.001;

enum EParams
{
	// Oscillator 1 Section:
	mOsc1Waveform = 0,
	mOsc1PitchMod,
	mOsc1Voices,
	// mOsc1Octave,
	// Oscillator 2 Section: 
	mOsc2Waveform,
	mOsc2PitchMod,
	mOsc2Voices,
	// mOsc2Octave,
	//mOscMix,
	// Filter Section:
	mFilterMode,
	mFilterCutoff,
	mFilterResonance,
	mFilterLfoAmount,
	mFilterEnvAmount,
	// LFO:
	mLFOWaveform,
	mLFOFrequency,
	// Volume Envelope:
	mVolumeEnvAttack,
	mVolumeEnvDecay,
	mVolumeEnvSustain,
	mVolumeEnvRelease,
	// Filter Envelope:
	mFilterEnvAttack,
	mFilterEnvDecay,
	mFilterEnvSustain,
	mFilterEnvRelease,
	kNumParams
};

typedef struct {
	const char* name;
	const int x;
	const int y;
	const double defaultVal;
	const double minVal;
	const double maxVal;
} parameterProperties_struct;

const parameterProperties_struct parameterProperties[kNumParams] = {
	{ "Osc 1 Waveform", 36, 36, 0, 0, 0 },
	{ "Osc 1 Pitch Mod", 206, 25, 0.0, 0.0, 1.0 },
	{ "Osc 1 Voices", 148, 56, 1, 1, 8},
//	{ "Osc 1 Octave", 100, 100, 0, 0, 0},
	{ "Osc 2 Waveform", 36, 126, 0, 0, 0 },
	{ "Osc 2 Pitch Mod", 206, 115, 0.0, 0.0, 1.0 },
	{ "Osc 2 Voices", 148, 146, 1, 1, 8},
//	{ "Osc 2 Octave", 200, 200, 0, 0, 0 },
	//{ "Osc Mix", 130, 61, 0.5, 0.0, 1.0 },
	{ "Filter Mode", 414, 125, 0, 0, 0 },
	{ "Filter Cutoff", 487, 115, 0.99, 0.0, 0.99 },
	{ "Filter Resonance", 560, 115, 0.0, 0.0, 1.0 },
	{ "Filter LFO Amount", 634, 115, 0.0, 0.0, 1.0 },
	{ "Filter Envelope Amount", 708, 115, 0.0, -1.0, 1.0 },

	{ "LFO Waveform", 192, 331, 0, 0, 0 },
	{ "LFO Frequency", 193, 382, 6.0, 0.01, 30.0 },
	// Volume
	{ "Volume Env Attack", 487, 205, 0.01, 0.01, 10.0 },
	{ "Volume Env Decay", 560, 205, 0.5, 0.01, 15.0 },
	{ "Volume Env Sustain", 634, 205, 0.1, 0.001, 1.0 },
	{ "Volume Env Release", 708, 205, 1.0, 0.01, 15.0 },

	{ "Filter Env Attack", 30, 303, 0.01, 0.01, 10.0 },
	{ "Filter Env Decay", 30, 382, 0.5, 0.01, 15.0 },
	{ "Filter Env Sustain", 106, 303, 0.1, 0.001, 1.0 },
	{ "Filter Env Release", 106, 382, 1.0, 0.01, 15.0 },
};

enum ELayout
{
	kWidth = GUI_WIDTH,
	kHeight = GUI_HEIGHT,
	kKeybX = 58,
	kKeybY = 460
};

AlmaVST::AlmaVST(IPlugInstanceInfo instanceInfo) : IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo), lastVirtualKeyboardNoteNumber(virtualKeyboardMinimumNoteNumber - 1) {
	TRACE;

	CreateParams();
	CreateGraphics();
	CreatePresets();

	mMIDIReceiver.noteOn.Connect(&voiceManager, &VoiceManager::onNoteOn);
	mMIDIReceiver.noteOff.Connect(&voiceManager, &VoiceManager::onNoteOff);
}

AlmaVST::~AlmaVST() {}

void AlmaVST::CreateParams() {
	for (int i = 0; i < kNumParams; i++) {
		IParam* param = GetParam(i);
		const parameterProperties_struct& properties = parameterProperties[i];
		switch (i) {
			// Enum Parameters:
		case mOsc1Waveform:
		case mOsc2Waveform:
			param->InitEnum(properties.name,
				Oscillator::OSCILLATOR_MODE_SAW,
				Oscillator::kNumOscillatorModes);
			// For VST3:
			param->SetDisplayText(0, properties.name);
			break;
		case mLFOWaveform:
			param->InitEnum(properties.name,
				Oscillator::OSCILLATOR_MODE_TRIANGLE,
				Oscillator::kNumOscillatorModes);
			// For VST3:
			param->SetDisplayText(0, properties.name);
			break;
		case mFilterMode:
			param->InitEnum(properties.name,
				Filter::FILTER_MODE_LOWPASS,
				Filter::kNumFilterModes);
			break;
			// Double Parameters:
		default:
			param->InitDouble(properties.name,
				properties.defaultVal,
				properties.minVal,
				properties.maxVal,
				parameterStep);
			break;
		}
	}
	GetParam(mFilterCutoff)->SetShape(2);
	GetParam(mVolumeEnvAttack)->SetShape(3);
	GetParam(mFilterEnvAttack)->SetShape(3);
	GetParam(mVolumeEnvDecay)->SetShape(3);
	GetParam(mFilterEnvDecay)->SetShape(3);
	GetParam(mVolumeEnvSustain)->SetShape(2);
	GetParam(mFilterEnvSustain)->SetShape(2);
	GetParam(mVolumeEnvRelease)->SetShape(3);
	GetParam(mFilterEnvRelease)->SetShape(3);
	for (int i = 0; i < kNumParams; i++) {
		OnParamChange(i);
	}
}

void AlmaVST::CreateGraphics() {
	// Create Graphics object
	IGraphics* pGraphics = MakeGraphics(this, kWidth, kHeight);
	pGraphics->AttachBackground(BG_ID, BG_FN);

	// Keyboard
	IBitmap whiteKeyImage = pGraphics->LoadIBitmap(WHITE_KEY_ID, WHITE_KEY_FN, 6);
	IBitmap blackKeyImage = pGraphics->LoadIBitmap(BLACK_KEY_ID, BLACK_KEY_FN);
	//                         C  C#  D	  D#  E   F   F#  G     G#    A   A#  B
	int keyCoordinates[12] = { 0, 16, 26, 42, 52, 78, 94, 104, 120, 130, 146, 156 };
	mVirtualKeyboard = new IKeyboardControl(this, kKeybX, kKeybY, virtualKeyboardMinimumNoteNumber, /* octaves: */ 4, &whiteKeyImage, &blackKeyImage, keyCoordinates);
	pGraphics->AttachControl(mVirtualKeyboard);

	// WaveForms
	IBitmap waveformBlueBitmap = pGraphics->LoadIBitmap(WAVEFORM_BLUE_ID, WAVEFORM_BLUE_FN, 5);
	IBitmap waveformRedBitmap = pGraphics->LoadIBitmap(WAVEFORM_RED_ID, WAVEFORM_RED_FN, 5);

	// FilterModes
	IBitmap filterModeBitmap = pGraphics->LoadIBitmap(FILTERMODE_ID, FILTERMODE_FN, 4);

	// KNOBS
	IBitmap knobBitmap = pGraphics->LoadIBitmap(KNOB_ID, KNOB_FN, 71);
	IBitmap knobWhiteBitmap = pGraphics->LoadIBitmap(KNOB_WHITE_ID, KNOB_WHITE_FN, 71);

	// Number knobs
	IBitmap octaveBitmap = pGraphics->LoadIBitmap(OCTAVES_ID, OCTAVES_FN, 49);
	IBitmap voicesBitmap = pGraphics->LoadIBitmap(VOICES_ID, VOICES_FN, 8);

	for (int i = 0; i < kNumParams; i++) {
		const parameterProperties_struct& properties = parameterProperties[i];
		IControl* control;
		ISwitchControl* iSwitch;
		IBitmap* graphic;
		switch (i) {
			// Switches:
		case mOsc1Waveform:
			graphic = &waveformRedBitmap;
			control = new ISwitchControl(this, properties.x, properties.y, i, graphic);
			break;
		case mOsc1Voices:
		case mOsc2Voices:
			graphic = &voicesBitmap;
			control = new IKnobMultiControl(this, properties.x, properties.y, i, graphic);
			break;
/*		case mOsc1Octave:
		case mOsc2Octave:
			graphic = &octaveBitmap;
			control = new IKnobMultiControl(this, properties.x, properties.y, i, graphic);
			break;
*/
		case mOsc2Waveform:
		case mLFOWaveform:
			graphic = &waveformBlueBitmap;
			control = new ISwitchControl(this, properties.x, properties.y, i, graphic);
			break;
		case mFilterMode:
			graphic = &filterModeBitmap;
			control = new ISwitchControl(this, properties.x, properties.y, i, graphic);
			break;
		case mOsc1PitchMod:
		case mOsc2PitchMod:
		case mLFOFrequency:
			graphic = &knobWhiteBitmap;
			control = new IKnobMultiControl(this, properties.x, properties.y, i, graphic);
			break;
			// Knobs:
		default:
			graphic = &knobBitmap;
			control = new IKnobMultiControl(this, properties.x, properties.y, i, graphic);
			break;
		}
		pGraphics->AttachControl(control);
	}
	AttachGraphics(pGraphics);
}

void AlmaVST::CreatePresets() {
}

void AlmaVST::ProcessDoubleReplacing(
	double** inputs,
	double** outputs,
	int nFrames)
{
	// Mutex is already locked for us.

	double *leftOutput = outputs[0];
	double *rightOutput = outputs[1];
	processVirtualKeyboard();
	for (int i = 0; i < nFrames; ++i) {
		mMIDIReceiver.advance();
		leftOutput[i] = rightOutput[i] = voiceManager.nextSample();
	}

	mMIDIReceiver.Flush(nFrames);
}

void AlmaVST::Reset()
{
	TRACE;
	IMutexLock lock(this);
	double sampleRate = GetSampleRate();
	voiceManager.setSampleRate(sampleRate);
}

void AlmaVST::OnParamChange(int paramIdx)
{
	IMutexLock lock(this);
	IParam* param = GetParam(paramIdx);
	if (paramIdx == mLFOWaveform) {
		voiceManager.setLFOMode(static_cast<Oscillator::OscillatorMode>(param->Int()));
	}
	else if (paramIdx == mLFOFrequency) {
		voiceManager.setLFOFrequency(param->Value());
	}
	else {
		using std::tr1::placeholders::_1;
		using std::tr1::bind;
		VoiceManager::VoiceChangerFunction changer;
		switch (paramIdx) {
		case mOsc1Waveform:
			changer = bind(&VoiceManager::setOscillatorMode,
				_1,
				1,
				static_cast<Oscillator::OscillatorMode>(param->Int()));
			break;
		case mOsc1PitchMod:
			changer = bind(&VoiceManager::setOscillatorPitchMod, _1, 1, param->Value());
			break;
		case mOsc2Waveform:
			changer = bind(&VoiceManager::setOscillatorMode, _1, 2, static_cast<Oscillator::OscillatorMode>(param->Int()));
			break;
		case mOsc2PitchMod:
			changer = bind(&VoiceManager::setOscillatorPitchMod, _1, 2, param->Value());
			break;
/*		case mOscMix:
			changer = bind(&VoiceManager::setOscillatorMix, _1, param->Value());
			break;
			// Filter Section:
			*/
		case mOsc1Voices:
			changer = bind(&VoiceManager::setOscillatorOneVoiceCount, _1, param->Value());
			break;
		case mOsc2Voices:
			changer = bind(&VoiceManager::setOscillatorTwoVoiceCount, _1, param->Value());
			break;
		case mFilterMode:
			changer = bind(&VoiceManager::setFilterMode, _1, static_cast<Filter::FilterMode>(param->Int()));
			break;
		case mFilterCutoff:
			changer = bind(&VoiceManager::setFilterCutoff, _1, param->Value());
			break;
		case mFilterResonance:
			changer = bind(&VoiceManager::setFilterResonance, _1, param->Value());
			break;
		case mFilterLfoAmount:
			changer = bind(&VoiceManager::setFilterLFOAmount, _1, param->Value());
			break;
		case mFilterEnvAmount:
			changer = bind(&VoiceManager::setFilterEnvAmount, _1, param->Value());
			break;
			// Volume Envelope:
		case mVolumeEnvAttack:
			changer = bind(&VoiceManager::setVolumeEnvelopeStageValue, _1, EnvelopeGenerator::ENVELOPE_STAGE_ATTACK, param->Value());
			break;
		case mVolumeEnvDecay:
			changer = bind(&VoiceManager::setVolumeEnvelopeStageValue, _1, EnvelopeGenerator::ENVELOPE_STAGE_DECAY, param->Value());
			break;
		case mVolumeEnvSustain:
			changer = bind(&VoiceManager::setVolumeEnvelopeStageValue, _1, EnvelopeGenerator::ENVELOPE_STAGE_SUSTAIN, param->Value());
			break;
		case mVolumeEnvRelease:
			changer = bind(&VoiceManager::setVolumeEnvelopeStageValue, _1, EnvelopeGenerator::ENVELOPE_STAGE_RELEASE, param->Value());
			break;
			// Filter Envelope:
		case mFilterEnvAttack:
			changer = bind(&VoiceManager::setFilterEnvelopeStageValue, _1, EnvelopeGenerator::ENVELOPE_STAGE_ATTACK, param->Value());
			break;
		case mFilterEnvDecay:
			changer = bind(&VoiceManager::setFilterEnvelopeStageValue, _1, EnvelopeGenerator::ENVELOPE_STAGE_DECAY, param->Value());
			break;
		case mFilterEnvSustain:
			changer = bind(&VoiceManager::setFilterEnvelopeStageValue, _1, EnvelopeGenerator::ENVELOPE_STAGE_SUSTAIN, param->Value());
			break;
		case mFilterEnvRelease:
			changer = bind(&VoiceManager::setFilterEnvelopeStageValue, _1, EnvelopeGenerator::ENVELOPE_STAGE_RELEASE, param->Value());
			break;
		}
		voiceManager.changeAllVoices(changer);
	}
}

void AlmaVST::ProcessMidiMsg(IMidiMsg* pMsg) {
	mMIDIReceiver.onMessageReceived(pMsg);
	mVirtualKeyboard->SetDirty();
}

void AlmaVST::processVirtualKeyboard() {
	IKeyboardControl* virtualKeyboard = (IKeyboardControl*)mVirtualKeyboard;
	int virtualKeyboardNoteNumber = virtualKeyboard->GetKey() + virtualKeyboardMinimumNoteNumber;

	if (lastVirtualKeyboardNoteNumber >= virtualKeyboardMinimumNoteNumber && virtualKeyboardNoteNumber != lastVirtualKeyboardNoteNumber) {
		// The note number has changed from a valid key to something else (valid key or nothing). Release the valid key:
		IMidiMsg midiMessage;
		midiMessage.MakeNoteOffMsg(lastVirtualKeyboardNoteNumber, 0);
		mMIDIReceiver.onMessageReceived(&midiMessage);
	}

	if (virtualKeyboardNoteNumber >= virtualKeyboardMinimumNoteNumber && virtualKeyboardNoteNumber != lastVirtualKeyboardNoteNumber) {
		// A valid key is pressed that wasn't pressed the previous call. Send a "note on" message to the MIDI receiver:
		IMidiMsg midiMessage;
		midiMessage.MakeNoteOnMsg(virtualKeyboardNoteNumber, virtualKeyboard->GetVelocity(), 0);
		mMIDIReceiver.onMessageReceived(&midiMessage);
	}

	lastVirtualKeyboardNoteNumber = virtualKeyboardNoteNumber;
}