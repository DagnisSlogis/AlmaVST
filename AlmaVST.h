#ifndef __ALMAVST__
#define __ALMAVST__

#include "IPlug_include_in_plug_hdr.h"
#include "Oscillator.h"

class AlmaVST : public IPlug
{
public:
	AlmaVST(IPlugInstanceInfo instanceInfo);
	~AlmaVST();

	void Reset();
	void OnParamChange(int paramIdx);
	void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

private:
	double mFrequency;
	void CreatePresets();
	Oscillator mOscillator;
};

#endif
