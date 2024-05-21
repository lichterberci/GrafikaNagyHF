//=============================================================================================
// Mintaprogram: Zöld háromszög. Ervenyes 2019. osztol.
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - Mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni es
// - felesleges programsorokat a beadott programban hagyni!!!!!!! 
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak 
// A keretben nem szereplo GLUT fuggvenyek tiltottak.
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : Lichter Bertalan Ede
// Neptun : MBSEKD
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================
// HIVATKOZASOK:
// https://www.inf.ufrgs.br/~oliveira/pubs_files/CVD_Simulation/CVD_Simulation.html
// =============================================================================================
#include "framework.h"

// vertex shader in GLSL: It is a Raw string (C++11) since it contains new line characters
const char * const vertexSource = R"(
	#version 330
	precision highp float;		// normal floats, makes no difference on desktop computers

	uniform mat4 MVP;			// uniform variable, the Model-View-Projection transformation matrix
	layout(location = 0) in vec2 vp;	// Varying input: vp = vertex position is expected in attrib array 0

	void main() {
		gl_Position = vec4(vp.x, vp.y, 0, 1) * MVP;		// transform vp from modeling space to normalized device space
	}
)";

// fragment shader in GLSL
//const char * const fragmentSource = R"(
//	#version 330
//	precision highp float;	// normal floats, makes no difference on desktop computers
//	
//	uniform vec3 color;		// uniform variable, the color of the primitive
//	out vec4 outColor;		// computed color of the current pixel
//
//	void main() {
//		outColor = vec4(color, 1);	// computed color is the color of the primitive
//	}
//)";

GPUProgram gpuProgram; // vertex and fragment shaders

#define MAT3TOMAT4(a, b, c, d, e, f, g, h, i) mat4((float)a, (float)b, (float)c, 0.0f, (float)d, (float)e, (float)f, 0.0f, (float)g, (float)h, (float)i, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f)

/*

CVD table:

Severity
Protanomaly
Deuteranomaly
Tritanomaly
0.0
1.000000	0.000000	-0.000000
0.000000	1.000000	0.000000
-0.000000	-0.000000	1.000000
1.000000	0.000000	-0.000000
0.000000	1.000000	0.000000
-0.000000	-0.000000	1.000000
1.000000	0.000000	-0.000000
0.000000	1.000000	0.000000
-0.000000	-0.000000	1.000000
0.1
0.856167	0.182038	-0.038205
0.029342	0.955115	0.015544
-0.002880	-0.001563	1.004443
0.866435	0.177704	-0.044139
0.049567	0.939063	0.011370
-0.003453	0.007233	0.996220
0.926670	0.092514	-0.019184
0.021191	0.964503	0.014306
0.008437	0.054813	0.936750
0.2
0.734766	0.334872	-0.069637
0.051840	0.919198	0.028963
-0.004928	-0.004209	1.009137
0.760729	0.319078	-0.079807
0.090568	0.889315	0.020117
-0.006027	0.013325	0.992702
0.895720	0.133330	-0.029050
0.029997	0.945400	0.024603
0.013027	0.104707	0.882266
0.3
0.630323	0.465641	-0.095964
0.069181	0.890046	0.040773
-0.006308	-0.007724	1.014032
0.675425	0.433850	-0.109275
0.125303	0.847755	0.026942
-0.007950	0.018572	0.989378
0.905871	0.127791	-0.033662
0.026856	0.941251	0.031893
0.013410	0.148296	0.838294
0.4
0.539009	0.579343	-0.118352
0.082546	0.866121	0.051332
-0.007136	-0.011959	1.019095
0.605511	0.528560	-0.134071
0.155318	0.812366	0.032316
-0.009376	0.023176	0.986200
0.948035	0.089490	-0.037526
0.014364	0.946792	0.038844
0.010853	0.193991	0.795156
0.5
0.458064	0.679578	-0.137642
0.092785	0.846313	0.060902
-0.007494	-0.016807	1.024301
0.547494	0.607765	-0.155259
0.181692	0.781742	0.036566
-0.010410	0.027275	0.983136
1.017277	0.027029	-0.044306
-0.006113	0.958479	0.047634
0.006379	0.248708	0.744913
0.6
0.385450	0.769005	-0.154455
0.100526	0.829802	0.069673
-0.007442	-0.022190	1.029632
0.498864	0.674741	-0.173604
0.205199	0.754872	0.039929
-0.011131	0.030969	0.980162
1.104996	-0.046633	-0.058363
-0.032137	0.971635	0.060503
0.001336	0.317922	0.680742
0.7
0.319627	0.849633	-0.169261
0.106241	0.815969	0.077790
-0.007025	-0.028051	1.035076
0.457771	0.731899	-0.189670
0.226409	0.731012	0.042579
-0.011595	0.034333	0.977261
1.193214	-0.109812	-0.083402
-0.058496	0.979410	0.079086
-0.002346	0.403492	0.598854
0.8
0.259411	0.923008	-0.182420
0.110296	0.804340	0.085364
-0.006276	-0.034346	1.040622
0.422823	0.781057	-0.203881
0.245752	0.709602	0.044646
-0.011843	0.037423	0.974421
1.257728	-0.139648	-0.118081
-0.078003	0.975409	0.102594
-0.003316	0.501214	0.502102
0.9
0.203876	0.990338	-0.194214
0.112975	0.794542	0.092483
-0.005222	-0.041043	1.046265
0.392952	0.823610	-0.216562
0.263559	0.690210	0.046232
-0.011910	0.040281	0.971630
1.278864	-0.125333	-0.153531
-0.084748	0.957674	0.127074
-0.000989	0.601151	0.399838
1.0
0.152286	1.052583	-0.204868
0.114503	0.786281	0.099216
-0.003882	-0.048116	1.051998
0.367322	0.860646	-0.227968
0.280085	0.672501	0.047413
-0.011820	0.042940	0.968881
1.255528	-0.076749	-0.178779
-0.078411	0.930809	0.147602
0.004733	0.691367	0.303900

*/

const mat4 COLOR_DEFFICIENT_VISION_MATRIX[3][11]{
	{
		MAT3TOMAT4(
			1.000000,	0.000000, - 0.000000,
			0.000000,	1.000000,	0.000000,
			- 0.000000, - 0.000000,	1.000000
		),
		MAT3TOMAT4(
			0.856167	,0.182038 ,- 0.038205,
			0.029342	,0.955115,	0.015544,
			- 0.002880 , -0.001563,	1.004443
		),
		MAT3TOMAT4(
			0.734766,	0.334872,	-0.069637,
			0.128305,	0.873762,	-0.002067,
			-0.001534,	-0.000000,	1.001534
		),
		MAT3TOMAT4(
			0.630323,	0.366620,	0.003057,
			0.069181,	0.929553,	0.001266,
			0.016349,	0.000000,	0.983651
		),
		MAT3TOMAT4(
			0.539009,	0.460134,	0.000857,
			0.082546,	0.917036,	0.000418,
			0.000000,	0.000000,	1.000000
		),
		MAT3TOMAT4(
			0.458064,	0.421234,	0.000702,
			0.092785,	0.876248,	0.030967,
			0.000000,	0.000000,	1.000000
		),
		MAT3TOMAT4(
			0.385450,	0.236000,	0.000000,
			0.100526,	0.763975,	0.135499,
			0.000000,	0.000000,	1.000000
		),
		MAT3TOMAT4(
			0.319627,	0.150183,	0.000000,
			0.106241,	0.683572,	0.210187,
			0.000000,	0.000000,	1.000000
		),
		MAT3TOMAT4(
			0.259411,	0.076992,	0.000000,
			0.110296,	0.619802,	0.269902,
			0.000000,	0.000000,	1.000000
		),
		MAT3TOMAT4(
			0.203876,	0.009662,	0.000000,
			0.112975,	0.570398,	0.316627,
			0.000000,	0.000000,	1.000000
		),
		MAT3TOMAT4(
			0.152286,	0.000000,	0.000000,
			0.114503,	0.535056,	0.350441,
			0.000000,	0.000000,	1.000000
		)
	}, 
	{
		MAT3TOMAT4(
			1.000000,	0.000000, -0.000000,
			0.000000,	1.000000,	0.000000,
			-0.000000, -0.000000,	1.000000
		),
		MAT3TOMAT4(
			0.866435,	0.177704,	-0.044139,
			0.049567,	0.939063,	0.011370,
			-0.003453,	0.007233,	0.996220
		),
		MAT3TOMAT4(
			0.760729,	0.319078,	-0.079807,
			0.090568,	0.889315,	0.020117,
			-0.006027,	0.013325,	0.992702
		),
		MAT3TOMAT4(
			0.675425,	0.433850,	-0.109275,
			0.125303,	0.847755,	0.026942,
			-0.007950,	0.018572,	0.989378
		),
		MAT3TOMAT4(
			0.605511,	0.528560,	-0.134071,
			0.155318,	0.812366,	0.032316,
			-0.009376,	0.023176,	0.986200
		),
			MAT3TOMAT4(
			0.547494,	0.607765,	-0.155259,
			0.181692,	0.781742,	0.036566,
			-0.010410,	0.027275,	0.983136
		),
		MAT3TOMAT4(
			0.498864,	0.674741,	-0.173604,
			0.205199,	0.754872,	0.039929,
			-0.011131,	0.030969,	0.980162
		),
		MAT3TOMAT4(
			0.457771,	0.731899,	-0.189670,
			0.226409,	0.731012,	0.042579,
			-0.011595,	0.034333,	0.977261
		),
		MAT3TOMAT4(
			0.422823,	0.781057,	-0.203881,
			0.245752,	0.709602,	0.044646,
			-0.011843,	0.037423,	0.974421
		),
		MAT3TOMAT4(
			0.367322,	0.860646,	-0.227968,
			0.280085,	0.672501,	0.047413,
			-0.011820,	0.042940,	0.968881
		),
		MAT3TOMAT4(
			0.319627,	0.849633,	-0.169261,
			0.106241,	0.815969,	0.077790,
			-0.007025,	-0.028051,	1.035076
		)
	},
	{
		MAT3TOMAT4(
			1.000000,	0.000000, -0.000000,
			0.000000,	1.000000,	0.000000,
			-0.000000, -0.000000,	1.000000
		),
		MAT3TOMAT4(
			0.926670,	0.092514,	-0.019184,
			0.021191,	0.964503,	0.014306,
			0.008437,	0.054813,	0.936750
		),
		MAT3TOMAT4(
			0.895720,	0.133330,	-0.029050,
			0.029997,	0.945400,	0.024603,
			0.013027,	0.148296,	0.838294
		),
		MAT3TOMAT4(
			0.905871,	0.127791,	-0.033662,
			0.026856,	0.941251,	0.031893,
			0.013410,	0.148296,	0.838294
		),
		MAT3TOMAT4(
			0.948035,	0.089490,	-0.037526,
			0.014364,	0.946792,	0.038844,
			0.010853,	0.193991,	0.795156
		),
		MAT3TOMAT4(
			1.017277,	0.027029,	-0.044306,
			-0.006113,	0.958479,	0.047634,
			0.006379,	0.248708,	0.744913
		),
		MAT3TOMAT4(
			1.104996,	-0.046633,	-0.058363,
			-0.032137,	0.971635,	0.060503,
			0.001336,	0.317922,	0.680742
		),
		MAT3TOMAT4(
			1.193214,	-0.109812,	-0.083402,
			-0.058496,	0.979410,	0.079086,
			-0.002346,	0.403492,	0.598854
		),
		MAT3TOMAT4(
			1.257728,	-0.139648,	-0.118081,
			-0.078003,	0.975409,	0.102594,
			-0.003316,	0.501214,	0.502102
		),
		MAT3TOMAT4(
			1.278864,	-0.125333,	-0.153531,
			-0.084748,	0.957674,	0.127074,
			-0.000989,	0.601151,	0.399838
		),
		MAT3TOMAT4(
			1.255528,	-0.076749,	-0.178779,
			-0.078411,	0.930809,	0.147602,
			0.004733,	0.691367,	0.303900
		)
	}
};

class Camera {
private:
	vec3 position = vec3(0, 0, 0);
	float rotation = 0.0f;
	vec3 up = vec3(0, 1, 0);
	static Camera instance;
	float fov = 90.0f;
	uint8_t cvdType = 0;
	uint8_t cvdSeverity = 0;
public:
	Camera() {}
	static Camera& GetInstance() {
		return instance;
	}

	static const Camera& GetConstInstance() {
		return instance;
	}

	mat4 CameraToWorldMatrix() const {
		return RotationMatrix(rotation, up) * TranslateMatrix(position);
	}

	void MoveForward(float distance) {
		vec4 offset = vec4(0, 0, distance, 0) * RotationMatrix(rotation, up);
		position = position + vec3(offset.x, offset.y, offset.z);
	}

	void MoveSideWays(float distance) {
		vec4 offset = vec4(distance, 0, 0, 0) * RotationMatrix(rotation, up);
		position = position + vec3(offset.x, offset.y, offset.z);
	}

	void MoveUp(float distance) {
		vec4 offset = vec4(0, distance, 0, 0) * RotationMatrix(rotation, up);
		position = position + vec3(offset.x, offset.y, offset.z);
	}

	void OffsetRotation(float angle) {
		rotation += angle;
	}

	float GetFov() const {
		return fov;
	}

	void SetFov(float fov) {
		this->fov = fov;
	}

	void LoopCvdType() {
		cvdType = (cvdType + 1) % 3;
		switch (cvdType) {
		case 0:
			printf("Protanomaly\n");
			break;
		case 1:
			printf("Deuteranomaly\n");
			break;
		case 2:
			printf("Tritanomaly\n");
			break;
		}
	}

	void IncreaseCvdSeverity() {
		if (cvdSeverity < 10) {
			cvdSeverity++;
			printf("Severity: %f\n", cvdSeverity / 10.0f);
		}
	}

	void DecreaseCvdSeverity() {
		if (cvdSeverity > 0) {
			cvdSeverity--;
			printf("Severity: %f\n", cvdSeverity / 10.0f);
		}
	}

	mat4 GetColorDeficientVisionMatrix() const {
		return COLOR_DEFFICIENT_VISION_MATRIX[cvdType][cvdSeverity];
	}

	~Camera() {}
};

Camera Camera::instance = Camera();

unsigned int vao;
unsigned int vbo;

// Initialization, create an OpenGL context
void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);

	glGenVertexArrays(1, &vao);	// get 1 vao id
	glBindVertexArray(vao);		// make it active

	glGenBuffers(1, &vbo);	// Generate 1 buffer
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	// vertex coordinates: vbo0 -> Attrib Array 0 -> vertexPosition of the vertex shader
	glBindBuffer(GL_ARRAY_BUFFER, vbo); // make it active, it is an array
	float vertexCoords[] = { -1, -1,  1, -1,  1, 1,  -1, 1 };	// two triangles forming a quad
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexCoords), vertexCoords, GL_STATIC_DRAW);	   // copy to that part of the memory which is not modified 
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);     // stride and offset: it is tightly packed

	// read fragment shader from file
	auto file = fopen("..\\Skeleton\\pixelshader.frag", "r");

	if (file == nullptr) {
		throw "Failed to open fragment shader file.";
	}

	// read the file
	
	std::string fragmentSource = "";
	char buffer[256];
	while (fgets(buffer, 256, file) != nullptr) {
		fragmentSource += buffer;
	}


	fclose(file);

	// create program for the GPU
	gpuProgram.create(vertexSource, fragmentSource.c_str(), "outColor");
	gpuProgram.Use();
}

int frameCount = 0;
int accumIter = 0;

// Window has become invalid: Redraw
void onDisplay() {
	glClearColor(0, 0, 0, 0);     // background color
	glClear(GL_COLOR_BUFFER_BIT); // clear frame buffer

	auto lightDirection = normalize(vec3(1.5, -1, 1));

	// Set color to (0, 1, 0) = green
	gpuProgram.setUniform(vec3(0.3f, 0.3f, 0.9f), "skyColor");
	gpuProgram.setUniform(vec3(0.99f, 0.8f, 0.6f), "lightColor");
	gpuProgram.setUniform(1.3f, "lightIntensity");
	gpuProgram.setUniform(vec2(windowWidth, windowHeight), "resolution");
	gpuProgram.setUniform(Camera::GetConstInstance().GetFov(), "cameraFov");
	gpuProgram.setUniform(Camera::GetConstInstance().CameraToWorldMatrix(), "camToWorldMatrix");
	gpuProgram.setUniform(lightDirection, "lightDirection");
	gpuProgram.setUniform(frameCount, "frameCount");
	gpuProgram.setUniform(25, "numRaysPerPixel");
	gpuProgram.setUniform(7, "maxRecursionDepth");
	gpuProgram.setUniform(Camera::GetConstInstance().GetColorDeficientVisionMatrix(), "colorDefficiencyMatrix");

	float MVPtransf[4][4] = { 1, 0, 0, 0,    // MVP matrix, 
							  0, 1, 0, 0,    // row-major!
							  0, 0, 1, 0,
							  0, 0, 0, 1 };

	int location = glGetUniformLocation(gpuProgram.getId(), "MVP");	// Get the GPU location of uniform variable MVP
	glUniformMatrix4fv(location, 1, GL_TRUE, &MVPtransf[0][0]);	// Load a 4x4 row-major float matrix to the specified location

	glBindVertexArray(vao);  // Draw call
	glDrawArrays(GL_TRIANGLE_FAN, 0 /*startIdx*/, 4 /*# Elements*/);

	frameCount++;


	glutSwapBuffers(); // exchange buffers for double buffering
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
	if (key == 'w') {
		Camera::GetInstance().MoveForward(0.1f);
		glutPostRedisplay();
	}
	if (key == 'W') {
		Camera::GetInstance().MoveForward(0.3f);
		glutPostRedisplay();
	}
	if (key == 's') {
		Camera::GetInstance().MoveForward(-0.1f);
		glutPostRedisplay();
	}
	if (key == 'S') {
		Camera::GetInstance().MoveForward(-0.3f);
		glutPostRedisplay();
	}
	if (key == 'a') {
		Camera::GetInstance().MoveSideWays(-0.1f);
		glutPostRedisplay();
	}
	if (key == 'A') {
		Camera::GetInstance().MoveSideWays(-0.3f);
		glutPostRedisplay();
	}
	if (key == 'd') {
		Camera::GetInstance().MoveSideWays(0.1f);
		glutPostRedisplay();
	}
	if (key == 'D') {
		Camera::GetInstance().MoveSideWays(0.3f);
		glutPostRedisplay();
	}
	if (key == 'e') {
		Camera::GetInstance().OffsetRotation(0.1f);
		glutPostRedisplay();
	}
	if (key == 'E') {
		Camera::GetInstance().OffsetRotation(0.3f);
		glutPostRedisplay();
	}
	if (key == 'q') {
		Camera::GetInstance().OffsetRotation(-0.1f);
		glutPostRedisplay();
	}
	if (key == 'Q') {
		Camera::GetInstance().OffsetRotation(-0.3f);
		glutPostRedisplay();
	}
	if (key == 'r') {
		Camera::GetInstance().MoveUp(0.1f);
		glutPostRedisplay();
	}
	if (key == 'R') {
		Camera::GetInstance().MoveUp(0.3f);
		glutPostRedisplay();
	}
	if (key == 'f') {
		Camera::GetInstance().MoveUp(-0.1f);
		glutPostRedisplay();
	}
	if (key == 'F') {
		Camera::GetInstance().MoveUp(-0.3f);
		glutPostRedisplay();
	}
	if (key == 'c') {
		Camera::GetInstance().LoopCvdType();
		glutPostRedisplay();
	}
	if (key == 'v') {
		Camera::GetInstance().IncreaseCvdSeverity();
		glutPostRedisplay();
	}
	if (key == 'b') {
		Camera::GetInstance().DecreaseCvdSeverity();
		glutPostRedisplay();
	}
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
	// this is not needed
}

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {	// pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	// Convert to normalized device space
	float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
	float cY = 1.0f - 2.0f * pY / windowHeight;

}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) { // pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	// Convert to normalized device space
	float cX = 2.0f * (float)pX / windowWidth - 1;	// flip y axis
	float cY = 1.0f - 2.0f * (float)pY / windowHeight;

	switch (state) {
	case GLUT_DOWN: 	
		break;
	case GLUT_UP: 
		break;
	default: break;
	}
}

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
	// this is not needed
}
