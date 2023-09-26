#include "ofApp.h"
#include "ofxRealsense.h"
#include "ofxOpenCv.h"

//--------------------------------------------------------------
void ofApp::setup(){

	config.enable_stream(RS2_STREAM_DEPTH);
	config.enable_stream(RS2_STREAM_COLOR);

	pipe.start(config);

	vector <ofSerialDeviceInfo> deviceList = serial.getDeviceList();
	for (auto hi = deviceList.begin(); hi != deviceList.end(); hi++) {
		cout << hi->getDeviceName() << endl;
	}

	auto devices = ofSoundStreamListDevices();
	for (auto device = devices.begin(); device != devices.end(); device++) {
		cout << device->name << endl;
	}

	serial.setup(0, 9600);

	stream.setup(2, 0, 44100, 256, 2);

	

}

//--------------------------------------------------------------
void ofApp::update(){

	if (!scanning) return;

	rs2::frameset frames = align_to_depth.process(pipe.wait_for_frames());

	while (serial.available()) motorRotation = (float)serial.readByte() / 255.0 * 360.0;

	rs2::depth_frame depth = frames.get_depth_frame();
	rs2::video_frame color_frame = frames.get_color_frame();

	pc.map_to(color_frame);
	points = pc.calculate(depth);

	int width = color_frame.get_width();
	int height = color_frame.get_height();

	const rs2::vertex* ptr = points.get_vertices();
	const rs2::texture_coordinate* tex_coords = points.get_texture_coordinates();

	const unsigned char* colorStream = static_cast<const unsigned char* > (color_frame.get_data());

	int pointCount = points.size();

	unsigned long avgY = 0;

	for (int i = 0; i < pointCount; i++) {

		const int width = 1280;
		const int height = 720;

		if (ptr[i].z == 0) continue; //ignore empty data

		//get location of point relative to motor axis
		ofVec3f pos = ofVec3f(ptr[i].x, ptr[i].y, ptr[i].z) * 1000 - motorAxis;
		pos.y *= -1;
		if (pos.y < 0 || pos.z > 0) continue; //ignore points below wavetable + on far half of turntable

		float dist = ofVec2f(pos.x, pos.z).distance(ofVec2f());
		if (dist > tableRadius) continue; //ignore points off of turntable

		int x = tex_coords[i].u * width;
		int y = tex_coords[i].v * height;

		if (y < 0 || y >= height) continue;
		if (x < 0 || x >= width) continue;

		int colorLocation = (y * width + x) * 3;
		ofColor color(colorStream[colorLocation], colorStream[colorLocation + 1], colorStream[colorLocation + 2]);

		//rotate around axis according to motor's current rotation
		pos.rotate(motorRotation, ofVec3f(0, 1, 0));
		
		bitmap3D[(int)pos.y][(int)(pos.x + tableRadius)][(int)(pos.z + tableRadius)] = 1;

		int azimuth = fmod(atan2(pos.z, pos.x) * 180 / PI + 180, 360);
		mesh_wavetable[pos.y][azimuth].point = pos;
		mesh_wavetable[pos.y][azimuth].color = color;
		mesh_wavetable[pos.y][azimuth].dist = dist;
		
	}

	if (motorRotation == 360) {
		scanning = false;
		motorRotation = 0;
	}

}

//--------------------------------------------------------------
void ofApp::draw(){

	

	if (bitmapMode) {

		ofDisableDepthTest();

		img.draw(0, 0);

		for (int b = 0; b < contours.nBlobs; b++) {
			ofxCvBlob blob = contours.blobs.at(b);
			blob.draw();
		}

		for (auto line : lines) {
			ofPushMatrix();
			ofTranslate(0, 300);

			line.draw();
			ofPopMatrix();
		}

	}


	else {

		ofEnableDepthTest();
		glPointSize(5);
		compositeScan.clear();

		int scanHeight = mesh_wavetable.size();

		int w = ofGetWidth();
		int h = ofGetHeight();

		for (int y = 0; y < scanHeight; y++) {
			for (int x = 0; x < 360; x++) {

				if (mesh_wavetable[y][x].dist == 0) continue;
				compositeScan.addVertex(mesh_wavetable[y][x].point);
				compositeScan.addColor(mesh_wavetable[y][x].color);
			
			}
		}


		ofPushMatrix();
		cam.begin();
		compositeScan.drawVertices();
		cam.end();
		ofPopMatrix();
	}
	
}

void ofApp::audioOut(float* output, int bufferSize, int nChannels) {

	int waveCount = size(waves);

	for (auto &buf : waves) {
		auto bufOut = buf.getOutput(bufferSize);

		for (int i = 0; i < bufferSize; i++) {
			output[i * 2] += bufOut.at(i) / (float)waveCount;
			output[i * 2 + 1] += bufOut.at(i) / (float)waveCount;
		}
	}

}


void ofApp::scanSlice(int s) {
	lines.clear();

	ofPixels pixels;
	pixels.allocate(tableRadius * 2, tableRadius * 2, OF_IMAGE_GRAYSCALE);
	for (int x = 0; x < tableRadius * 2; x++) {
		for (int z = 0; z < tableRadius * 2; z++) {
			auto color = bitmap3D[s][x][z] * 255;
			pixels.setColor(x, z, color);
		}
	}
	
	img.setFromPixels(pixels);
	img.blurGaussian(5);
	contours.findContours(img, 50, 50000, size(waves), false, true);

	for (int b = 0; b < size(waves); b++) {

		if (b >= contours.nBlobs) {
			waves[b].clear();
			continue;
		}
		ofxCvBlob blob = contours.blobs.at(b);

		ofVec3f center = blob.centroid;
		float freq = 200.0 * pow(1.05946, int(center.y/12.5));
		cout << freq;

		ofPolyline blobLine; //visual line vertex data
		vector<float> blobWave; //wave envelope sample data

		//add a sample/vertex to wave/line for each point in blob
		for (int p = 0; p < blob.nPts; p++) {
			ofVec3f pt = blob.pts.at(p);
			//using distance between point and blob centroid
			float sampVal = pow(pt.distance(center) / tableRadius, 1);

			blobLine.addVertex(ofVec3f(p, sampVal));
			blobWave.push_back(sampVal);
		}
		lines.push_back(blobLine);
		waves[b].fillWithSineWave(freq);
		waves[b].setEnvelope(blobWave, (float)blob.nPts/200.0f);
	}
	
	
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	if (key == 8) { // backspace
		mesh_wavetable.clear();
		for (int x = 0; x < tableRadius*2; x++) {
			for (int y = 0; y < tableRadius*2; y++) {
				for (int z = 0; z < tableRadius*2; z++) {
					bitmap3D[x][y][z] = false;
				}
			}
		}
	}
	else if (key == 47) { // forward-slash
		bitmapMode = !bitmapMode;
	}
	else if (key == 13) { // enter/return
		unsigned char out = 100;
		serial.writeByte(out);

		scanning = true;
	}
	else if (key == 57357) { // up arrow
		waveNumber++;
		scanSlice(waveNumber);
		cout << waveNumber << endl;
	}
	else if (key == 57359) { //down arrow
		if (waveNumber > 0) waveNumber--;
		scanSlice(waveNumber);
		cout << waveNumber << endl;
	}
	else if (key != 0) cout << key << endl;

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
	/*waves[0].clear();*/
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
