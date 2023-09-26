#pragma once

#include "ofMain.h"
#include "ofxRealsense.h"
#include "ofxOpenCV.h"
#include "SineEnvelope.h"


class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		void audioOut(float* output, int bufferSize, int nChannels);
		void scanSlice(int s);

		ofEasyCam cam;

		ofSoundStream stream;
		unsigned long sampTotal = 0;

		rs2::pointcloud pc;
		rs2::points points;
		rs2::pipeline pipe;
		rs2::config config;

		rs2::temporal_filter temporalFilter;
		rs2::spatial_filter spatialFilter;
		rs2::colorizer colorizer;

		rs2::align align_to_depth = rs2::align(RS2_STREAM_DEPTH);
		rs2::align align_to_color = rs2::align(RS2_STREAM_COLOR);

		ofMesh currentPoints;

		ofVec3f motorAxis = ofVec3f(.02, .16, .53) * 1000;

		static const int tableRadius = 200; //in mm

		float motorRotation;
		bool scanning = false;

		ofSerial serial;

		struct point_sample {
			ofVec3f point;
			ofColor color;
			float dist = 0.0;
		};

		map<int, point_sample[360]> mesh_wavetable;

		//how to automatically get dimensions from tableRadius*2?
		char bitmap3D[400][400][400] = { 0 };

		ofxCvGrayscaleImage img;
		ofxCvContourFinder contours;
		
		ofMesh compositeScan;

		bool bitmapMode = false;

		int waveNumber = 0;

		int phase = 0;

		SineEnvelope waves[5] = { 
			SineEnvelope(stream),
			SineEnvelope(stream),
			SineEnvelope(stream),
			SineEnvelope(stream),
			SineEnvelope(stream) 
		};

		vector<ofPolyline> lines;
};
