#pragma once

#include "ofMain.h"

class SineEnvelope {

public:
	int sampRate = 0;
	float frequency = 0;

	std::mutex* waveLock;

	//vectors to hold wave + envelope data
	vector<float> wave{0};
	vector<float> envelope{1.0f};
	float smoothedEnvelopeValue = 0; //interpolated envelope value to prevent popping
	float smoothedOutputValue = 0;

	//phase values + percentage-based phase values for wave and envelope
	int phase = 0;
	float perPhase = 0;
	int envPhase = 0;
	float envPerPhase = 0;

	//fills wave with sine wave of frequency f (given current sampling rate)
	bool fillWithSineWave(float f) {

		std::lock_guard<std::mutex> lock(*waveLock);
		if (f <= 20) return false;
		wave.clear();

		int length = (float)sampRate / f;
		for (int s = 0; s < length; s++) {
			wave.push_back(sin(s * 2 * PI / length));
		}
		frequency = f;
		int newPhase = perPhase * (float)wave.size();
		phase = newPhase;
	}
	
	//sets envelope values and duration
	bool setEnvelope(vector<float> input, float s) {
		std::lock_guard<std::mutex> lock(*waveLock);
		if (s <= 0) return false;

		int nearestValIndex = 0;
		float nearestValDiff = 0;
		//find index of sample closest to current envelope value
		for (int i = 0; i < input.size(); i++) {
			//get difference between sample value and current env value
			float diff = abs(input[i] - smoothedEnvelopeValue);

			if (nearestValDiff == 0) { 
				nearestValDiff = diff; 
				continue; 
			}
			else if (diff < nearestValDiff) {
				nearestValDiff = diff;
				nearestValIndex = i;
			}
		}

		/*cout << smoothedEnvelopeValue << ',' << nearestValDiff << ',' <<
			input[nearestValIndex] << endl;*/

		envelope.clear();
		int size = input.size();
		int targetSize = s * sampRate;
		
		//for each needed sample:
		for (float i = 0; i < targetSize; i++) { 
			int iTarget = i / (float)targetSize * (float)size;
			float val = input.at(iTarget);
			envelope.push_back(val);
		}
		envPhase = nearestValIndex;
	}

	bool setSawtoothEnvelope(float s) {
		if (s <= 0) return false;
		envelope.clear();

		int targetSize = s * sampRate;

		for (int i = 0; i < targetSize; i++) { //for each needed sample:
			envelope.push_back((float)i/(float)targetSize);
		}
	}

	//get next n samps of wave modulated by envelope
	vector<float> getOutput(int n) {
		std::lock_guard<std::mutex> lock(*waveLock);

		vector<float> out;
		int s = wave.size();
		int es = envelope.size();

		for (int i = 0; i < n; i++) {

			//current sample indexes from wave + envelope given phases + lengths
			int targetSamp = (phase + i) % s;
			int envTargetSamp = (envPhase + i) % es;

			//interpolation of envelope value
			smoothedEnvelopeValue = smoothedEnvelopeValue * .99 + 
				envelope.at(envTargetSamp) * .01;
			//smoothedEnvelopeValue = envelope.at(envTargetSamp);

			float sampVal = wave.at(targetSamp) * smoothedEnvelopeValue;
			smoothedOutputValue = smoothedOutputValue * .9 + sampVal * .1;

			out.push_back(smoothedOutputValue);
		}
		//increment phases by output sample count, wrap around wavelengths
		phase = (phase + n) % s;
		envPhase = (envPhase + n) % es;

		perPhase = s == 0 ? 0 : (float)phase / (float)s;
		envPerPhase = es == 0 ? 0 : (float)envPhase / (float)es;

		return out;
	}

	void clear() {
		std::lock_guard<std::mutex> lock(*waveLock);
		wave.clear();
		wave.push_back(0);
		envelope.clear();
		envelope.push_back(1);
	}

	int sampLength() {
		return wave.size();
	}
	
	SineEnvelope(ofSoundStream s) {
		sampRate = s.getSampleRate();
		waveLock = new std::mutex();
	}

	//create a sine wave with frequency f
	SineEnvelope(ofSoundStream s, float f) {
		sampRate = s.getSampleRate();
		waveLock = new std::mutex();
		fillWithSineWave(f);
	}

	//create sine wave with frequency f, modulated by 0-1 envelope of duration d
	SineEnvelope(ofSoundStream s, float f, float d) {
		sampRate = s.getSampleRate();
		waveLock = new std::mutex();
		fillWithSineWave(f);
		setSawtoothEnvelope(d);
	}

};

