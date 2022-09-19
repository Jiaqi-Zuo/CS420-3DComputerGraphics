/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields with Shaders.
  C++ starter code

  Student username: Jiaqi Zuo
*/

#include "basicPipelineProgram.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "openGLHeader.h"
#include "glutHeader.h"

#include <iostream>
#include <cstring>
#include <vector>


#if defined(WIN32) || defined(_WIN32)
#ifdef _DEBUG
#pragma comment(lib, "glew32d.lib")
#else
#pragma comment(lib, "glew32.lib")
#endif
#endif

#if defined(WIN32) || defined(_WIN32)
char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

using namespace std;

int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

typedef enum { P_POINTS, LINES, TRIANGLES, SMOOTH, WIREFRAME_ONTOP } RENDER_OPTION;
RENDER_OPTION renderOption = P_POINTS;

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework I";

ImageIO* heightmapImage;

GLuint triVertexBuffer, triColorVertexBuffer;
GLuint triVertexArray;
int sizeTri;

GLuint ptsVBO, ptsColorVBO;
GLuint ptsVAO;
GLuint linesVBO, linesColorVBO;
GLuint linesVAO;
GLuint triVBO, triColorVBO, triIBO;
GLuint triVAO;
GLuint pLeftVBO, pRightVBO, pDownVBO, pUpVBO, pCenterVBO, pColorVBO;
GLuint smoothVAO;
GLuint wireTopVBO, wireTopColorVBO;
GLuint wireTopVAO;

float scale = 1.0;
float height = 1.0;

OpenGLMatrix matrix;
BasicPipelineProgram* pipelineProgram;

glm::vec3 triangle[3] = {
  glm::vec3(0, 0, 0),
  glm::vec3(0, 1, 0),
  glm::vec3(1, 0, 0)
};

glm::vec4 color[3] = {
  {0, 0, 1, 1},
  {1, 0, 0, 1},
  {0, 1, 0, 1},
};

std::vector<float> pts_position;
std::vector<float> pts_color;
std::vector<float> lines_position;
std::vector<float> lines_color;
std::vector<float> tri_position;
std::vector<float> tri_color;
std::vector<int> tri_indices;
std::vector<float> pL_position, pR_position, pD_position, pU_position;
std::vector<float> wireTop_color;

int numShots = 1;
bool animation = false;

// write a screenshot to the specified filename
void saveScreenshot(const char* filename)
{
	unsigned char* screenshotData = new unsigned char[windowWidth * windowHeight * 3];
	glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

	ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

	if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
		cout << "File " << filename << " saved successfully." << endl;
	else cout << "Failed to save file " << filename << '.' << endl;

	delete[] screenshotData;
}

void displayFunc()
{
	// render some stuff...
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	matrix.SetMatrixMode(OpenGLMatrix::ModelView);
	matrix.LoadIdentity();
	// adjusting the view
	float eyez = 0;
	if (heightmapImage->getHeight() <= 220) eyez = 128;
	else if (heightmapImage->getHeight() <= 500) eyez = 256;
	else if (heightmapImage->getHeight() <= 700) eyez = 512;
	else eyez = 624;
	matrix.LookAt(128, 256, eyez, 0, 0, 0, 0, 1, 0);
	
	//Matrix.Rotate(angle, vx, vy, vz)
	matrix.Rotate(landRotate[0], 1.0f, 0.0f, 0.0f);
	matrix.Rotate(landRotate[1], 0.0f, 1.0f, 0.0f);
	matrix.Rotate(landRotate[2], 0.0f, 0.0f, 1.0f);
	//Matrix.Translate(dx, dy, dz)
	matrix.Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
	//Matrix.Scale(sx, sy, sz)
	matrix.Scale(landScale[0], landScale[1], landScale[2]);

	float m[16];
	matrix.SetMatrixMode(OpenGLMatrix::ModelView);
	matrix.GetMatrix(m);

	float p[16];
	matrix.SetMatrixMode(OpenGLMatrix::Projection);
	matrix.GetMatrix(p);
	//
	// bind shader
	pipelineProgram->Bind();

	// set variable

	pipelineProgram->SetModelViewMatrix(m);
	pipelineProgram->SetProjectionMatrix(p);
	// use VAO

	switch (renderOption)
	{
	case P_POINTS:
		// use points VAO
		glBindVertexArray(ptsVAO);
		glDrawArrays(GL_POINTS, 0, pts_position.size() / 3);
		break;

	case LINES:
		// use lines VAO
		glBindVertexArray(linesVAO);
		glDrawArrays(GL_LINES, 0, lines_position.size() / 3);
		break;

	case TRIANGLES:
		// use triangles VAO
		glBindVertexArray(triVAO);
		glDrawElements(GL_TRIANGLES, tri_indices.size(), GL_UNSIGNED_INT, (void*)0);
		break;

	case SMOOTH:
		// use smooth mode VAO
		glBindVertexArray(smoothVAO);
		glDrawElements(GL_TRIANGLES, tri_indices.size(), GL_UNSIGNED_INT, (void*)0);
		break;

	case WIREFRAME_ONTOP:
		// use wireframe on tope of solid triangles VAO
		// use glPolygonOffset to avoid z-buffer fighting
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1.0f, 1.0f);
		// draw wireframe
		glBindVertexArray(wireTopVAO);
		glDrawArrays(GL_LINES, 0, lines_position.size() / 3);
		// dissable polygon offset fill
		glDisable(GL_POLYGON_OFFSET_FILL);
		// draw solid triangles
		glBindVertexArray(triVAO);
		glDrawElements(GL_TRIANGLES, tri_indices.size(), GL_UNSIGNED_INT, (void*)0);
		break;
	}

	// unbind VAO
	glBindVertexArray(0);
	// sawp the buffers 
	glutSwapBuffers();
}

void pushColor(vector<float>& colors, float R, float G, float B) {
	// store color value into matrics
	colors.push_back(R);
	colors.push_back(G);
	colors.push_back(B);
	colors.push_back(1.0);
}

void idleFunc()
{
	// Animation
	if (animation == true) {
		GLint loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "mode");
		if (numShots <= 300) {
			// First 50 shots in points mode
			if (numShots <= 50) {
				renderOption = P_POINTS;
				glUniform1i(loc, 0);
				landRotate[1] += 5.0f;
			}
			// Next 50 shots in lines mode
			else if (numShots <= 100) {
				renderOption = LINES;
				glUniform1i(loc, 0);
				if (numShots <= 75) {
					landTranslate[0] += 1.0f;
				}
				else if (numShots <= 85) {
					landTranslate[1] += 1.0f;
				}
				else {
					landTranslate[2] += 1.0f;
				}
			}
			// Next 50 shots in triangles mode
			else if (numShots <= 150) {
				renderOption = TRIANGLES;
				glUniform1i(loc, 0);
				if (numShots <= 125) {
					landTranslate[2] += 2.0f;
				}
				else if (numShots <= 135) {
					landTranslate[1] -= 5.0f;
				}
				else {
					landTranslate[0] -= 10.0f;
				}
			}
			// Next 50 shots in smooth mode
			else if (numShots <= 200) {
				renderOption = SMOOTH;
				glUniform1i(loc, 1);
				if (numShots <= 175) {
					landScale[0] += 0.03f;
				}
				else {
					landScale[1] += 0.03f;
				}
			}
			// Next 50 shots in wireframe on top of triangles mode
			else if (numShots <= 250) {
				renderOption = WIREFRAME_ONTOP;
				glUniform1i(loc, 0);
				if (numShots <= 225) {
					landScale[2] += 0.02f;
				}
				else {
					landTranslate[0] += 5.0f;
				}
			}
			// The final 50 shots
			else if (numShots <= 265) {
				renderOption = P_POINTS;
				glUniform1i(loc, 0);
				landScale[2] += 0.01f;
				landRotate[0] += 4.0f;
			}
			else if (numShots <= 280) {
				renderOption = LINES;
				glUniform1i(loc, 0);
				landRotate[1] -= 4.0f;
			}
			else {
				renderOption = TRIANGLES;
				glUniform1i(loc, 0);
				landRotate[2] += 1.0f;
				landTranslate[1] -= 5.0f;
			}
			// store the screenshots in certain folder
			char fileName[5];
			sprintf(fileName, "%03d", numShots);
			saveScreenshot(("./screenshots/" + std::string(fileName) + ".jpg").c_str());
			numShots += 1;
		}
	}
	glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
	glViewport(0, 0, w, h);

	matrix.SetMatrixMode(OpenGLMatrix::Projection);
	matrix.LoadIdentity();
	matrix.Perspective(60.0f, (float)w / (float)h, 0.01f, 1000.0f);
	matrix.SetMatrixMode(OpenGLMatrix::ModelView);
}

void mouseMotionDragFunc(int x, int y)
{
	// mouse has moved and one of the mouse buttons is pressed (dragging)

	// the change in mouse position since the last invocation of this function
	int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

	switch (controlState)
	{
		// translate the landscape
	case TRANSLATE:
		if (leftMouseButton)
		{
			// control x,y translation via the left mouse button
			landTranslate[0] += mousePosDelta[0] * 0.01f;
			landTranslate[1] -= mousePosDelta[1] * 0.01f;
		}
		if (middleMouseButton)
		{
			// control z translation via the middle mouse button
			landTranslate[2] += mousePosDelta[1] * 0.01f;
		}
		break;

		// rotate the landscape
	case ROTATE:
		if (leftMouseButton)
		{
			// control x,y rotation via the left mouse button
			landRotate[0] += mousePosDelta[1];
			landRotate[1] += mousePosDelta[0];
		}
		if (middleMouseButton)
		{
			// control z rotation via the middle mouse button
			landRotate[2] += mousePosDelta[1];
		}
		break;

		// scale the landscape
	case SCALE:
		if (leftMouseButton)
		{
			// control x,y scaling via the left mouse button
			landScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
			landScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
		}
		if (middleMouseButton)
		{
			// control z scaling via the middle mouse button
			landScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
		}
		break;
	}

	// store the new mouse position
	mousePos[0] = x;
	mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
	// mouse has moved
	// store the new mouse position
	mousePos[0] = x;
	mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
	// a mouse button has has been pressed or depressed

	// keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables
	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		leftMouseButton = (state == GLUT_DOWN);
		break;

	case GLUT_MIDDLE_BUTTON:
		middleMouseButton = (state == GLUT_DOWN);
		break;

	case GLUT_RIGHT_BUTTON:
		rightMouseButton = (state == GLUT_DOWN);
		break;
	}

	// keep track of whether CTRL and SHIFT keys are pressed
	switch (glutGetModifiers())
	{
	case GLUT_ACTIVE_CTRL:
		controlState = TRANSLATE;
		break;

	case GLUT_ACTIVE_SHIFT:
		controlState = SCALE;
		break;

		// if CTRL and SHIFT are not pressed, we are in rotate mode
	default:
		controlState = ROTATE;
		break;
	}

	// store the new mouse position
	mousePos[0] = x;
	mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
	GLint loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "mode");
	switch (key)
	{
	case 27: // ESC key
		exit(0); // exit the program
		break;

	case ' ':
		cout << "You pressed the spacebar." << endl;
		break;

	case 'x':
		// take a screenshot
		saveScreenshot("screenshot.jpg");
		break;

	case '1':
		// points mode
		renderOption = P_POINTS;
		glUniform1i(loc, 0);
		break;

	case '2':
		// lines mode
		renderOption = LINES;
		glUniform1i(loc, 0);
		break;

	case '3':
		// triangles mode
		renderOption = TRIANGLES;
		glUniform1i(loc, 0);
		break;

	case '4':
		// smoothened mode
		renderOption = SMOOTH;
		glUniform1i(loc, 1);
		break;

	case '5':
		// wireframe on top of triangles mode
		renderOption = WIREFRAME_ONTOP;
		glUniform1i(loc, 0);
		break;

	case 'A':
		// to start the animation
		animation = true;
		break;

	// translation keys control
	case 'q':
		// positive x direction
		landTranslate[0] += 0.5f;
		break;
	case 'w':
		// negative x direction
		landTranslate[0] -= 0.5f;
		break;
	case 'e':
		// positive y direction
		landTranslate[1] += 0.5f;
		break;
	case 'r':
		// negative y direction
		landTranslate[1] -= 0.5f;
		break;
	case 't':
		// positive z direction
		landTranslate[2] += 0.5f;
		break;
	case 'y':
		// negative z direction
		landTranslate[2] -= 0.5f;
		break;

	// rotation keys control
	case 'a':
		landRotate[0] += 0.5f;
		break;
	case 's':
		landRotate[0] -= 0.5f;
		break;
	case 'd':
		landRotate[1] += 0.5f;
		break;
	case 'f':
		landRotate[1] -= 0.5f;
		break;
	case 'g':
		landRotate[2] += 0.5f;
		break;
	case 'h':
		landRotate[2] -= 0.5f;
		break;
	}
}

void getImageInfo0() {
	// generate vertices for points mode, lines mode, and triangles mode
	float imgHeight = heightmapImage->getHeight();
	float imgWidth = heightmapImage->getWidth();

	// center the image
	float imgW_h = imgWidth / 2.0f;
	float imgH_h = imgHeight / 2.0f;
	// store the oreder for the IBO
	int index = 0;
	scale = (float)(0.02 * (float)(imgWidth / 100.0f));
	for (int i = -imgH_h; i < (imgH_h - 1); i++) {
		for (int j = -imgW_h; j < (imgW_h - 1); j++) {
			height = heightmapImage->getPixel(i + imgH_h, j + imgW_h, 0);
			float color = height / 255.0f;

			pts_position.push_back((float)i); //v.x
			pts_position.push_back(height * scale); //v.y
			pts_position.push_back(-(float)j); //v.z
			pushColor(pts_color, color, color, color);

			// v2(i+1, j)  v3(i+1, j+1)
			//         -----
			//         |\  |
			//         | \ |
			// v0(i,j) |__\| v1(i, j + 1)
			// we only read four points for lines mode and triangles mode

			// vertex (i, j)
			height = heightmapImage->getPixel(i + imgH_h, j + imgW_h, 0);
			color = height / 255.0f;

			lines_position.push_back((float)i);
			lines_position.push_back(height * scale);
			lines_position.push_back(-(float)j);
			pushColor(lines_color, color, color, color);
			// set color to mauve of the wireframe on top of solid triangles mode 
			// (213, 184, 255) => (0.83, 0.72, 1.0);
			pushColor(wireTop_color, 0.83f, 0.72f, 1.0f);

			tri_position.push_back((float)i);
			tri_position.push_back(height * scale);
			tri_position.push_back(-(float)j);
			pushColor(tri_color, color, color, color);

			// vertex (i, j+1)
			height = heightmapImage->getPixel(i + imgH_h, j + 1 + imgW_h, 0);
			color = height / 255.0f;

			lines_position.push_back((float)i);
			lines_position.push_back(height * scale);
			lines_position.push_back(-(float)(j + 1));
			pushColor(lines_color, color, color, color);
			pushColor(wireTop_color, 0.83f, 0.72f, 1.0f);

			tri_position.push_back((float)i);
			tri_position.push_back(height * scale);
			tri_position.push_back(-(float)(j + 1));
			pushColor(tri_color, color, color, color);

			// vertex (i, j)
			// repeat the smae vertex for vertical line for lines mode
			height = heightmapImage->getPixel(i + imgH_h, j + imgW_h, 0);
			color = height / 255.0f;

			lines_position.push_back((float)i);
			lines_position.push_back(height * scale);
			lines_position.push_back(-(float)j);
			pushColor(lines_color, color, color, color);
			pushColor(wireTop_color, 0.83f, 0.72f, 1.0f);

			// vertex (i+1, j)
			height = heightmapImage->getPixel(i + 1 + imgH_h, j + imgW_h, 0);
			color = height / 255.0f;

			lines_position.push_back((float)(i + 1));
			lines_position.push_back(height * scale);
			lines_position.push_back(-(float)j);
			pushColor(lines_color, color, color, color);
			pushColor(wireTop_color, 0.83f, 0.72f, 1.0f);

			tri_position.push_back((float)(i + 1));
			tri_position.push_back(height * scale);
			tri_position.push_back(-(float)j);
			pushColor(tri_color, color, color, color);

			// vertex (i+1, j+1)
			height = heightmapImage->getPixel(i + 1 + imgH_h, j + 1 + imgW_h, 0);
			color = height / 255.0f;

			tri_position.push_back((float)(i + 1));
			tri_position.push_back(height * scale);
			tri_position.push_back(-(float)(j + 1));
			pushColor(tri_color, color, color, color);

			// [0,1,2] [2,1,3] .....
			// use four vertices to render two triangels
			tri_indices.push_back(index);
			tri_indices.push_back(index + 1);
			tri_indices.push_back(index + 2);
			tri_indices.push_back(index + 2);
			tri_indices.push_back(index + 1);
			tri_indices.push_back(index + 3);
			// update the starting index
			index += 4;
		}
	}
}

void getImageInfo1() {
	// generate vertices for smoothened mode
	float imgHeight = heightmapImage->getHeight();
	float imgWidth = heightmapImage->getWidth();

	scale = 0.02 * (imgWidth / 100.0f);
	
	// center the image
	float halfWidth = imgWidth / 2.0f;
	float halfHeight = imgHeight / 2.0f;

	for (int i = -halfHeight; i < (halfHeight - 1); i++) {
		for (int j = -halfWidth; j < (halfWidth-1); j++) {
			//(i+1, j)   (i+1, j+1)
			//       -----
			//       |\  |
			//       | \ |
			// (i,j) |__\| (i, j + 1)

			// *** vertex(i,j) ***
			// left vertex (i, j - 1)
			if (j == -halfWidth) {
				// when lfet vertex is off the image, pLeft = pRight
				height = heightmapImage->getPixel(i + halfHeight, j + 1 + halfWidth, 0);
				height *= scale;
			}
			else {
				height = heightmapImage->getPixel(i + halfHeight, j - 1 + halfWidth, 0);
				height *= scale;
			}
			pL_position.push_back((float)i);
			pL_position.push_back(height);
			pL_position.push_back(-(float)(j - 1));

			// right vertex (i, j + 1)
			height = heightmapImage->getPixel(i + halfHeight, j + 1 + halfWidth, 0);
			height *= scale;
			pR_position.push_back((float)i);
			pR_position.push_back(height);
			pR_position.push_back(-(float)(j + 1));
			// down vertex (i-1, j)
			if (i == -halfHeight) {
				// when down vertex is off the image, pDown = pUp
				height = heightmapImage->getPixel(i + 1 + halfHeight, j + halfWidth, 0);
				height *= scale;
			}
			else {
				height = heightmapImage->getPixel(i - 1 + halfHeight, j + halfWidth, 0);
				height *= scale;
			}
			pD_position.push_back((float)(i - 1));
			pD_position.push_back(height);
			pD_position.push_back(-(float)(j));
			// up vertex (i+1, j)
			height = heightmapImage->getPixel(i + 1 + halfHeight, j + halfWidth, 0);
			height *= scale;
			pU_position.push_back((float)(i + 1));
			pU_position.push_back(height);
			pU_position.push_back(-(float)(j));

			// *** vertex (i, j+1) ***
			// left vertex (i, j-1) => (i, j)
			height = heightmapImage->getPixel(i + halfHeight, j + halfWidth, 0);
			height *= scale;
			pL_position.push_back((float)i);
			pL_position.push_back(height);
			pL_position.push_back(-(float)j);
			// right vertex (i, j+1) => (i, j+2)
			if (j == (halfWidth - 2)) {
				// when right vertex is off th image, pRight = pLeft
				height = heightmapImage->getPixel(i + halfHeight, j + halfWidth, 0);
				height *= scale;
			}
			else {
				height = heightmapImage->getPixel(i + halfHeight, j + 2 + halfWidth, 0);
				height *= scale;
			}
			pR_position.push_back((float)i);
			pR_position.push_back(height);
			pR_position.push_back(-(float)(j + 2));
			// down vertex (i-1, j) => (i-1, j+1)
			if (i == -halfHeight) {
				// when down vertex is off the image, pDown = pUp
				height = heightmapImage->getPixel(i + 1 + halfHeight, j + 1 + halfWidth, 0);
				height *= scale;
			}
			else {
				height = heightmapImage->getPixel(i - 1 + halfHeight, j + 1 + halfWidth, 0);
				height *= scale;
			}
			pD_position.push_back((float)(i - 1));
			pD_position.push_back(height);
			pD_position.push_back(-(float)(j + 1));
			// up vertex (i+1, j) => (i+1, j+1)
			height = heightmapImage->getPixel(i + 1 + halfHeight, j + 1 + halfWidth, 0);
			height *= scale;
			pU_position.push_back((float)(i + 1));
			pU_position.push_back(height);
			pU_position.push_back(-(float)(j + 1));

			// *** vertex (i+1, j) ***
			// left vertex (i, j-1) => (i+1, j-1)
			if (j == -halfWidth) {
				// when left vertex is off the image, pLeft = pRight
				height = heightmapImage->getPixel(i + 1 + halfHeight, j + 1 + halfWidth, 0);
				height *= scale;
			}
			else {
				height = heightmapImage->getPixel(i + 1 + halfHeight, j - 1 + halfWidth, 0);
				height *= scale;
			}
			pL_position.push_back((float)(i + 1));
			pL_position.push_back(height);
			pL_position.push_back(-(float)(j - 1));
			// right vertex (i, j + 1) => (i+1,j+1)
			height = heightmapImage->getPixel(i + 1 + halfHeight, j + 1 + halfWidth, 0);
			height *= scale;
			pR_position.push_back((float)(i + 1));
			pR_position.push_back(height);
			pR_position.push_back(-(float)(j + 1));
			// down vertex (i-1, j) => (i, j)
			height = heightmapImage->getPixel(i + halfHeight, j + halfWidth, 0);
			height *= scale;
			pD_position.push_back((float)(i));
			pD_position.push_back(height);
			pD_position.push_back(-(float)(j));
			// up vertex (i+1, j) => (i+2, j)
			if (i == (halfHeight - 2)) {
				// when up vertex is off the image, pUp == pDown
				height = heightmapImage->getPixel(i + halfHeight, j + halfWidth, 0);
				height *= scale;
			}
			else {
				height = heightmapImage->getPixel(i + 2 + halfHeight, j + halfWidth, 0);
				height *= scale;
			}
			pU_position.push_back((float)(i + 2));
			pU_position.push_back(height);
			pU_position.push_back(-(float)(j));
	
			// *** vertex (i+1, j+1) ***
			// pLeft (i, j-1) => (i+1, j)
			height = heightmapImage->getPixel(i + 1 + halfHeight, j + halfWidth, 0);
			height *= scale;
			pL_position.push_back((float)(i + 1));
			pL_position.push_back(height);
			pL_position.push_back(-(float)j);
			// pRight (i, j+1) => (i+1, j+2)
			if (j == halfWidth - 2) {
				// when righe vertex is off the image, pRight = pLeft
				height = heightmapImage->getPixel(i + 1 + halfHeight, j + halfWidth, 0);
				height *= scale;
			}
			else {
				height = heightmapImage->getPixel(i + 1 + halfHeight, j + 2 + halfWidth, 0);
				height *= scale;
			}
			pR_position.push_back((float)(i + 1));
			pR_position.push_back(height);
			pR_position.push_back(-(float)(j + 2));
			// pDown (i-1, j) => (i, j+1)
			height = heightmapImage->getPixel(i + halfHeight, j + 1 + halfWidth, 0);
			height *= scale;
			pD_position.push_back((float)(i));
			pD_position.push_back(height);
			pD_position.push_back(-(float)(j + 1));
			// pUp (i+1, j) => (i+2, j+1)
			if (i == (halfHeight - 2)) {
				// when up vertex is off the image, pUp = pDown
				height = heightmapImage->getPixel(i + halfHeight, j + 1 + halfWidth, 0);
				height *= scale;
			}
			else {
				height = heightmapImage->getPixel(i + 2 + halfHeight, j + 1 + halfWidth, 0);
				height *= scale;
			}
			pU_position.push_back((float)(i + 2));
			pU_position.push_back(height);
			pU_position.push_back(-(float)(j + 1));
		}
	}
}

void VBO_VAO_mode0()
{
	//set VBO for points mode
	glGenBuffers(1, &ptsVBO);
	glBindBuffer(GL_ARRAY_BUFFER, ptsVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * pts_position.size(), &pts_position[0], GL_STATIC_DRAW);

	glGenBuffers(1, &ptsColorVBO);
	glBindBuffer(GL_ARRAY_BUFFER, ptsColorVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * pts_color.size(), &pts_color[0], GL_STATIC_DRAW);

	//set VAO for points mode
	glGenVertexArrays(1, &ptsVAO);
	glBindVertexArray(ptsVAO);

	glBindBuffer(GL_ARRAY_BUFFER, ptsVBO);
	GLuint loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	glBindBuffer(GL_ARRAY_BUFFER, ptsColorVBO);
	loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	glBindVertexArray(0);

	//set VBO for lines mode
	glGenBuffers(1, &linesVBO);
	glBindBuffer(GL_ARRAY_BUFFER, linesVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * lines_position.size(), &lines_position[0], GL_STATIC_DRAW);

	glGenBuffers(1, &linesColorVBO);
	glBindBuffer(GL_ARRAY_BUFFER, linesColorVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * lines_color.size(), &lines_color[0], GL_STATIC_DRAW);

	//set VAO for lines mode
	glGenVertexArrays(1, &linesVAO);
	glBindVertexArray(linesVAO);

	glBindBuffer(GL_ARRAY_BUFFER, linesVBO);
	loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	glBindBuffer(GL_ARRAY_BUFFER, linesColorVBO);
	loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	glBindVertexArray(0);

	//set VBO for wireframe on top of triangles mode
	glGenBuffers(1, &wireTopVBO);
	glBindBuffer(GL_ARRAY_BUFFER, wireTopVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * lines_position.size(), &lines_position[0], GL_STATIC_DRAW);

	glGenBuffers(1, &wireTopColorVBO);
	glBindBuffer(GL_ARRAY_BUFFER, wireTopColorVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * wireTop_color.size(), &wireTop_color[0], GL_STATIC_DRAW);

	//set VAO for wireframe on top of triangels mode
	glGenVertexArrays(1, &wireTopVAO);
	glBindVertexArray(wireTopVAO);

	glBindBuffer(GL_ARRAY_BUFFER, wireTopVBO);
	loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	glBindBuffer(GL_ARRAY_BUFFER, wireTopColorVBO);
	loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	glBindVertexArray(0);

	//set VBO for triangles mode
	glGenBuffers(1, &triVBO);
	glBindBuffer(GL_ARRAY_BUFFER, triVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * tri_position.size(), &tri_position[0], GL_STATIC_DRAW);

	glGenBuffers(1, &triColorVBO);
	glBindBuffer(GL_ARRAY_BUFFER, triColorVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * tri_color.size(), &tri_color[0], GL_STATIC_DRAW);

	//set VAO for triangles mode
	glGenVertexArrays(1, &triVAO);
	glBindVertexArray(triVAO);

	glBindBuffer(GL_ARRAY_BUFFER, triVBO);
	loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	glBindBuffer(GL_ARRAY_BUFFER, triColorVBO);
	loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	// bing the IBO
	glGenBuffers(1, &triIBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, tri_indices.size() * sizeof(unsigned int), &tri_indices[0], GL_STATIC_DRAW);

	glBindVertexArray(0);
}

void VBO_VAO_mode1() {
	// set VBOs and VAOs for smoothened mode
	// pLeft
	glGenBuffers(1, &pLeftVBO);
	glBindBuffer(GL_ARRAY_BUFFER, pLeftVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * pL_position.size(), &pL_position[0], GL_STATIC_DRAW);

	glGenVertexArrays(1, &smoothVAO);
	glBindVertexArray(smoothVAO);
	glBindBuffer(GL_ARRAY_BUFFER, pLeftVBO);
	GLuint loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "pLeft");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	// pRight
	glGenBuffers(1, &pRightVBO);
	glBindBuffer(GL_ARRAY_BUFFER, pRightVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * pR_position.size(), &pR_position[0], GL_STATIC_DRAW);

	loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "pRight");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	// pDown
	glGenBuffers(1, &pDownVBO);
	glBindBuffer(GL_ARRAY_BUFFER, pDownVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * pD_position.size(), &pD_position[0], GL_STATIC_DRAW);

	loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "pDown");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	// pUp
	glGenBuffers(1, &pUpVBO);
	glBindBuffer(GL_ARRAY_BUFFER, pUpVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * pU_position.size(), &pU_position[0], GL_STATIC_DRAW);

	loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "pUp");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	// pCenter
	glGenBuffers(1, &pCenterVBO);
	glBindBuffer(GL_ARRAY_BUFFER, pCenterVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * tri_position.size(), &tri_position[0], GL_STATIC_DRAW);

	loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	glGenBuffers(1, &pColorVBO);
	glBindBuffer(GL_ARRAY_BUFFER, pColorVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * tri_color.size(), &tri_color[0], GL_STATIC_DRAW);

	loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	// bind the IBO
	glGenBuffers(1, &triIBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, tri_indices.size() * sizeof(unsigned int), &tri_indices[0], GL_STATIC_DRAW);

	glBindVertexArray(0);
}

void initScene(int argc, char* argv[])
{
	// load the image from a jpeg disk file to main memory
	heightmapImage = new ImageIO();
	if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
	{
		cout << "Error reading image " << argv[1] << "." << endl;
		exit(EXIT_FAILURE);
	}

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	// modify the following code accordingly

	// read image and generate vertices
	getImageInfo0();
	getImageInfo1();

	pipelineProgram = new BasicPipelineProgram;
	int ret = pipelineProgram->Init(shaderBasePath);
	if (ret != 0) abort();

	pipelineProgram->Bind();

	// set VBOs and VAOs
	VBO_VAO_mode0();
	VBO_VAO_mode1();

	glEnable(GL_DEPTH_TEST);

	sizeTri = 3;

	std::cout << "GL error: " << glGetError() << std::endl;
}

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		cout << "The arguments are incorrect." << endl;
		cout << "usage: ./hw1 <heightmap file>" << endl;
		exit(EXIT_FAILURE);
	}

	cout << "Initializing GLUT..." << endl;
	glutInit(&argc, argv);

	cout << "Initializing OpenGL..." << endl;

#ifdef __APPLE__
	glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#else
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#endif

	glutInitWindowSize(windowWidth, windowHeight);
	glutInitWindowPosition(0, 0);
	glutCreateWindow(windowTitle);

	cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
	cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
	cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

#ifdef __APPLE__
	// This is needed on recent Mac OS X versions to correctly display the window.
	glutReshapeWindow(windowWidth - 1, windowHeight - 1);
#endif

	// tells glut to use a particular display function to redraw 
	glutDisplayFunc(displayFunc);
	// perform animation inside idleFunc
	glutIdleFunc(idleFunc);
	// callback for mouse drags
	glutMotionFunc(mouseMotionDragFunc);
	// callback for idle mouse movement
	glutPassiveMotionFunc(mouseMotionFunc);
	// callback for mouse button changes
	glutMouseFunc(mouseButtonFunc);
	// callback for resizing the window
	glutReshapeFunc(reshapeFunc);
	// callback for pressing the keys on the keyboard
	glutKeyboardFunc(keyboardFunc);

	// init glew
#ifdef __APPLE__
  // nothing is needed on Apple
#else
  // Windows, Linux
	GLint result = glewInit();
	if (result != GLEW_OK)
	{
		cout << "error: " << glewGetErrorString(result) << endl;
		exit(EXIT_FAILURE);
	}
#endif

	// do initialization
	initScene(argc, argv);

	// sink forever into the glut loop
	glutMainLoop();
}


