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

class Camera {
private:
	vec3 position = vec3(0, 0, 0);
	float rotation = 0.0f;
	vec3 up = vec3(0, 1, 0);
	static Camera instance;
	float fov = 90.0f;
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
	auto file = fopen("C:\\Users\\User\\Documents\\grafika\\nagyhf\\Programs\\Skeleton\\Skeleton\\pixelshader.frag", "r");

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

// Window has become invalid: Redraw
void onDisplay() {
	glClearColor(0, 0, 0, 0);     // background color
	glClear(GL_COLOR_BUFFER_BIT); // clear frame buffer

	auto lightDirection = normalize(vec3(1, -1, 1));

	// Set color to (0, 1, 0) = green
	gpuProgram.setUniform(vec3(0.3f, 0.3f, 0.9f), "skyColor");
	gpuProgram.setUniform(vec3(0.99f, 0.8f, 0.6f), "lightColor");
	gpuProgram.setUniform(1.6f, "lightIntensity");
	gpuProgram.setUniform(vec2(windowWidth, windowHeight), "resolution");
	gpuProgram.setUniform(Camera::GetConstInstance().GetFov(), "cameraFov");
	gpuProgram.setUniform(Camera::GetConstInstance().CameraToWorldMatrix(), "camToWorldMatrix");
	gpuProgram.setUniform(lightDirection, "lightDirection");
	gpuProgram.setUniform(1, "frameCount");
	gpuProgram.setUniform(20, "numRaysPerPixel");
	gpuProgram.setUniform(6, "maxRecursionDepth");

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
	if (key == 's') {
		Camera::GetInstance().MoveForward(-0.1f);
		glutPostRedisplay();
	}
	if (key == 'a') {
		Camera::GetInstance().MoveSideWays(-0.1f);
		glutPostRedisplay();
	}
	if (key == 'd') {
		Camera::GetInstance().MoveSideWays(0.1f);
		glutPostRedisplay();
	}
	if (key == 'e') {
		Camera::GetInstance().OffsetRotation(0.1f);
		glutPostRedisplay();
	}
	if (key == 'q') {
		Camera::GetInstance().OffsetRotation(-0.1f);
		glutPostRedisplay();
	}
	if (key == 'r') {
		Camera::GetInstance().MoveUp(0.1f);
		glutPostRedisplay();
	}
	if (key == 'f') {
		Camera::GetInstance().MoveUp(-0.1f);
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
