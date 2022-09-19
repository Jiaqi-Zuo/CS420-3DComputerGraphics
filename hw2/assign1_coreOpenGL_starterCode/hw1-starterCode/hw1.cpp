/*
  CSCI 420 Computer Graphics, USC
  Assignment 2: Roller Coaster
  C++

  Student username: jiaqizuo
*/
#include "basicPipelineProgram.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "openGLHeader.h"
#include "glutHeader.h"

#include <iostream>
#include <cstring>
#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

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

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework II";

OpenGLMatrix matrix;
BasicPipelineProgram* pipelineProgram, * texturePipelineProgram;

// represents one control point along the spline 
struct Point
{
	double x;
	double y;
	double z;
};

// spline struct 
// contains how many control points the spline has, and an array of control points 
struct Spline
{
	int numControlPoints;
	Point* points;
};

// the spline array 
Spline* splines;
// total number of splines 
int numSplines;

int imageWidth, imageHeight;

std::vector<glm::vec3> splinePts, tangents, normals, binormals; // splines and TNB
std::vector<glm::vec3> leftRail, rightRail, crossBar; // rail postioins vertices
std::vector<glm::vec3> leftRailNormals, rightRailNormals, crossBarNormals; // rail normals vertices
GLuint vboLeftRail, vboLeftNormals, vboRightRail, vboRightNormals, vboBar, vboBarNormals; // rail vbos
GLuint vaoLeftRail, vaoRightRail, vaoBar; // rail vaos

// variables for texture part
GLuint vaoGround, vboGround, vaoSky, vboSky;
GLuint vboSide1, vboSide2, vboSide3, vboSide4;
GLuint vaoSide1, vaoSide2, vaoSide3, vaoSide4;
std::vector<glm::vec3> positionGround, positionSky, positionSide1, positionSide2, positionSide3, positionSide4;
std::vector<glm::vec2> texCoordGround, texCoordSky, texCoordSide1, texCoordSide2, texCoordSide3, texCoordSide4, texCoordBar;
GLuint groundTexHandle, skyTexHandle, sideTexHandle1, sideTexHandle2, sideTexHandle3, sideTexHandle4, barTexHandle;

// lookAt
glm::vec3 eye, focus, up;
int moves = 0;
int numShots = 0;
bool takeScreenShot = false;
bool animation = true;

// alphas for rendering the tube
float a = 0.01f;
float a1 = 0.02f;

// sky box dimensions
float top = 60.0f; // the sky
float bottom = -10.0f; // the ground
float d = 40.0f; // sides

//matrices for splines
glm::mat4 basis;
glm::mat3x4 control;

// basis matrix for rendering the spline
float s = 0.5f;
float basisMatrix[16] = { -s, 2.0f - s, s - 2.0f, s,
					  2.0f * s, s - 3.0f, 3.0f - 2.0f * s, -s,
					  -s, 0.0f, s, 0.0f,
					  0.0f, 1.0f, 0.0f, 0.0f };


// lighting constant
glm::vec3 lightDirection(0.0f, 1.0f, .0f); // sun at noon

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

	// adjusting the view and animate the ride
	eye = splinePts[moves] + 0.4f * normals[moves];
	focus = tangents[moves] + splinePts[moves];
	up = normals[moves];
	matrix.LookAt(eye.x, eye.y, eye.z, focus.x, focus.y, focus.z, up.x, up.y, up.z);
	//matrix.LookAt(0, 10, 0, 0, 1, 0, 0, 0, 1);

	//Matrix.Rotate(angle, vx, vy, vz)
	matrix.Rotate(landRotate[0], 1.0f, 0.0f, 0.0f);
	matrix.Rotate(landRotate[1], 0.0f, 1.0f, 0.0f);
	matrix.Rotate(landRotate[2], 0.0f, 0.0f, 1.0f);
	//Matrix.Translate(dx, dy, dz)
	matrix.Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
	//Matrix.Scale(sx, sy, sz)
	matrix.Scale(landScale[0], landScale[1], landScale[2]);

	// Matrix for ModelView
	float m[16];
	matrix.SetMatrixMode(OpenGLMatrix::ModelView);
	matrix.GetMatrix(m);

	// Matrix for Projection
	float p[16];
	matrix.SetMatrixMode(OpenGLMatrix::Projection);
	matrix.GetMatrix(p);

	// bind shader
	pipelineProgram->Bind();
	// set variables
	pipelineProgram->SetModelViewMatrix(m);
	pipelineProgram->SetProjectionMatrix(p);

	float La[4] = { 0.8f, 0.8f, 0.8f, 1.0f }; // light ambient
	float Ld[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; // light diffuse
	float Ls[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; // light specular	
	GLint h_La = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "La");
	glUniform4fv(h_La, 1, La);
	GLint h_Ld = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "Ld");
	glUniform4fv(h_Ld, 1, Ld);
	GLint h_Ls = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "Ls");
	glUniform4fv(h_Ls, 1, Ls);

	float ka[4] = { 0.25f, 0.20725f, 0.20725f, 0.922f }; // mesh ambient
	float kd[4] = { 1.0f, 0.829f, 0.829f, 0.922f }; // mesh diffuse
	float ks[4] = { 0.296648f, 0.296648f, 0.296648f, 0.922f }; // mesh specular
	GLint h_ka = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "ka");
	glUniform4fv(h_ka, 1, ka);
	GLint h_kd = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "kd");
	glUniform4fv(h_kd, 1, kd);
	GLint h_ks = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "ks");
	glUniform4fv(h_ks, 1, ks);

	// shininess
	GLint h_alpha = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "alpha");
	glUniform1f(h_alpha, 20.0f);

	// setting phong shading light directions
	glm::mat4 view(m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8], m[9], m[10], m[11], m[12], m[13], m[14], m[15]);
	float directionX = (view * glm::vec4(lightDirection, 0.0)).x;
	float directionY = (view * glm::vec4(lightDirection, 0.0)).y;
	float directionZ = (view * glm::vec4(lightDirection, 0.0)).z;
	float viewLightDirection[3] = { directionX, directionY, directionZ }; // light direction in the view space
	GLint h_viewLightDirection = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "viewLightDirection");
	glUniform3fv(h_viewLightDirection, 1, viewLightDirection);

	// set the normal Matrix
	GLint h_normalMatrix = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "normalMatrix");
	float n[16];
	matrix.SetMatrixMode(OpenGLMatrix::ModelView);
	matrix.GetNormalMatrix(n); // get normal matrix
	// upload n to the GPU
	GLboolean isRowMajor = GL_FALSE;
	glUniformMatrix4fv(h_normalMatrix, 1, isRowMajor, n);

	// draw left rail
	glBindVertexArray(vaoLeftRail);
	glDrawArrays(GL_TRIANGLES, 0, leftRail.size());
	glBindVertexArray(0);
	// draw right rail
	glBindVertexArray(vaoRightRail);
	glDrawArrays(GL_TRIANGLES, 0, rightRail.size());
	glBindVertexArray(0);

	// bind texture shader
	texturePipelineProgram->Bind();
	// set matrix variable
	texturePipelineProgram->SetModelViewMatrix(m);
	texturePipelineProgram->SetProjectionMatrix(p);

	// bind ground texture handle
	glBindTexture(GL_TEXTURE_2D, groundTexHandle);
	glBindVertexArray(vaoGround); // bind the VAO
	glDrawArrays(GL_TRIANGLES, 0, positionGround.size());
	glBindVertexArray(0); // unbind the VAO

	// bind sky texture handle
	glBindTexture(GL_TEXTURE_2D, skyTexHandle);
	glBindVertexArray(vaoSky); // bind the VAO
	glDrawArrays(GL_TRIANGLES, 0, positionSky.size());
	glBindVertexArray(0); // unbind the VAO

	// bind side texture handles
	glBindTexture(GL_TEXTURE_2D, sideTexHandle1);
	glBindVertexArray(vaoSide1); // bind the VAO
	glDrawArrays(GL_TRIANGLES, 0, positionSide1.size());
	glBindVertexArray(0); // unbind the VAO
	// side2 
	glBindTexture(GL_TEXTURE_2D, sideTexHandle2);
	glBindVertexArray(vaoSide2); // bind the VAO
	glDrawArrays(GL_TRIANGLES, 0, positionSide2.size());
	glBindVertexArray(0); // unbind the VAO
	// side3
	glBindTexture(GL_TEXTURE_2D, sideTexHandle3);
	glBindVertexArray(vaoSide3); // bind the VAO
	glDrawArrays(GL_TRIANGLES, 0, positionSide3.size());
	glBindVertexArray(0); // unbind the VAO
	// side4
	glBindTexture(GL_TEXTURE_2D, sideTexHandle4);
	glBindVertexArray(vaoSide4); // bind the VAO
	glDrawArrays(GL_TRIANGLES, 0, positionSide4.size());
	glBindVertexArray(0); // unbind the VAO

	// bind crossbar texture handle
	glBindTexture(GL_TEXTURE_2D, barTexHandle);
	glBindVertexArray(vaoBar); //bind VAO
	glDrawArrays(GL_TRIANGLES, 0, crossBar.size());
	glBindVertexArray(0); // unbind VAO


	glutSwapBuffers();
}

void idleFunc()
{
	//control the speed
	if (animation == true && moves < splinePts.size() - 2) {
		moves += 2;
		if (takeScreenShot == true && numShots < 1000 && moves % 10 == 0)
		{
			char fileName[5];
			sprintf(fileName, "%03d", numShots);
			saveScreenshot(("animation/" + std::string(fileName) + ".jpg").c_str());
			numShots += 1;
		}
	}
	else if (animation == false) {
		matrix.LookAt(eye.x, eye.y, eye.z, focus.x, focus.y, focus.z, up.x, up.y, up.z);
	}
	else moves = 0;

	glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
	glViewport(0, 0, w, h);

	matrix.SetMatrixMode(OpenGLMatrix::Projection);
	matrix.LoadIdentity();
	matrix.Perspective(60.0f, (float)w / (float)h, 0.01f, 1000.0f);

	// set the mode back to ModelView
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

	case 'A':
		// to start the screenshot taking
		takeScreenShot = !takeScreenShot;
		break;

	case 'p':
		// to start/pause the animation
		animation = !animation;
		break;
	}
}

int loadSplines(char* argv)
{
	char* cName = (char*)malloc(128 * sizeof(char));
	FILE* fileList;
	FILE* fileSpline;
	int iType, i = 0, j, iLength;

	// load the track file 
	fileList = fopen(argv, "r");
	if (fileList == NULL)
	{
		printf("can't open file\n");
		exit(1);
	}

	// stores the number of splines in a global variable 
	fscanf(fileList, "%d", &numSplines);

	splines = (Spline*)malloc(numSplines * sizeof(Spline));

	// reads through the spline files 
	for (j = 0; j < numSplines; j++)
	{
		i = 0;
		fscanf(fileList, "%s", cName);
		fileSpline = fopen(cName, "r");

		if (fileSpline == NULL)
		{
			printf("can't open file\n");
			exit(1);
		}

		// gets length for spline file
		fscanf(fileSpline, "%d %d", &iLength, &iType);

		// allocate memory for all the points
		splines[j].points = (Point*)malloc(iLength * sizeof(Point));
		splines[j].numControlPoints = iLength;

		// saves the data to the struct
		while (fscanf(fileSpline, "%lf %lf %lf",
			&splines[j].points[i].x,
			&splines[j].points[i].y,
			&splines[j].points[i].z) != EOF)
		{
			i++;
		}
	}

	free(cName);

	return 0;
}

int initTexture(const char* imageFilename, GLuint textureHandle)
{
	// read the texture image
	ImageIO img;
	ImageIO::fileFormatType imgFormat;
	ImageIO::errorType err = img.load(imageFilename, &imgFormat);

	if (err != ImageIO::OK)
	{
		printf("Loading texture from %s failed.\n", imageFilename);
		return -1;
	}

	// check that the number of bytes is a multiple of 4
	if (img.getWidth() * img.getBytesPerPixel() % 4)
	{
		printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
		return -1;
	}

	// allocate space for an array of pixels
	int width = img.getWidth();
	int height = img.getHeight();
	unsigned char* pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

	// fill the pixelsRGBA array with the image pixels
	memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
	for (int h = 0; h < height; h++)
		for (int w = 0; w < width; w++)
		{
			// assign some default byte values (for the case where img.getBytesPerPixel() < 4)
			pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
			pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
			pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
			pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque

			// set the RGBA channels, based on the loaded image
			int numChannels = img.getBytesPerPixel();
			for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
				pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
		}

	// bind the texture
	glBindTexture(GL_TEXTURE_2D, textureHandle);

	// initialize the texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

	// generate the mipmaps for this texture
	glGenerateMipmap(GL_TEXTURE_2D);

	// set the texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// query support for anisotropic texture filtering
	GLfloat fLargest;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
	printf("Max available anisotropic samples: %f\n", fLargest);
	// set anisotropic texture filtering
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

	// query for any errors
	GLenum errCode = glGetError();
	if (errCode != 0)
	{
		printf("Texture initialization error. Error code: %d.\n", errCode);
		return -1;
	}

	// de-allocate the pixel array -- it is no longer needed
	delete[] pixelsRGBA;

	return 0;
}

// helper function to fill in texture vertices
void addUVs(vector<glm::vec2>* uvs) {
	// triangle (0,1,2)
	uvs->push_back(glm::vec2(0.0f, 0.0f));
	uvs->push_back(glm::vec2(0.0f, 1.0f));
	uvs->push_back(glm::vec2(1.0f, 1.0f));
	// triangle (2,3,0)
	uvs->push_back(glm::vec2(1.0f, 1.0f));
	uvs->push_back(glm::vec2(1.0f, 0.0f));
	uvs->push_back(glm::vec2(0.0f, 0.0f));
}

void generateGround() {
	// the bottom face of the cube
	// triangle(0,1,2)
	positionGround.push_back(glm::vec3(-d, bottom, d));
	positionGround.push_back(glm::vec3(-d, bottom, -d));
	positionGround.push_back(glm::vec3(d, bottom, -d));
	// triangle(2,3,0)
	positionGround.push_back(glm::vec3(d, bottom, -d));
	positionGround.push_back(glm::vec3(d, bottom, d));
	positionGround.push_back(glm::vec3(-d, bottom, d));
	addUVs(&texCoordGround); // fill in ground texture vertices
}

void generateSkyBox() {
	//front face of the sky box
	positionSide4.push_back(glm::vec3(d, bottom, d));
	positionSide4.push_back(glm::vec3(d, top, d));
	positionSide4.push_back(glm::vec3(d, top, -d));
	positionSide4.push_back(glm::vec3(d, top, -d));
	positionSide4.push_back(glm::vec3(d, bottom, -d));
	positionSide4.push_back(glm::vec3(d, bottom, d));
	addUVs(&texCoordSide4);

	//left face of the sky box
	positionSide1.push_back(glm::vec3(-d, bottom, d));
	positionSide1.push_back(glm::vec3(-d, top, d));
	positionSide1.push_back(glm::vec3(d, top, d));
	positionSide1.push_back(glm::vec3(d, top, d));
	positionSide1.push_back(glm::vec3(d, bottom, d));
	positionSide1.push_back(glm::vec3(-d, bottom, d));
	addUVs(&texCoordSide1);

	//back face of the sky box
	positionSide2.push_back(glm::vec3(-d, bottom, d));
	positionSide2.push_back(glm::vec3(-d, top, d));
	positionSide2.push_back(glm::vec3(-d, top, -d));
	positionSide2.push_back(glm::vec3(-d, top, -d));
	positionSide2.push_back(glm::vec3(-d, bottom, -d));
	positionSide2.push_back(glm::vec3(-d, bottom, d));
	addUVs(&texCoordSide2);

	//right face of the sky box
	positionSide3.push_back(glm::vec3(-d, bottom, -d));
	positionSide3.push_back(glm::vec3(-d, top, -d));
	positionSide3.push_back(glm::vec3(d, top, -d));
	positionSide3.push_back(glm::vec3(d, top, -d));
	positionSide3.push_back(glm::vec3(d, bottom, -d));
	positionSide3.push_back(glm::vec3(-d, bottom, -d));
	addUVs(&texCoordSide3);

	//top face of the sky box
	positionSky.push_back(glm::vec3(-d, top, d));
	positionSky.push_back(glm::vec3(-d, top, -d));
	positionSky.push_back(glm::vec3(d, top, -d));
	positionSky.push_back(glm::vec3(d, top, -d));
	positionSky.push_back(glm::vec3(d, top, d));
	positionSky.push_back(glm::vec3(-d, top, d));
	addUVs(&texCoordSky);
}

void initTextureVBO_VAO()
{
	// init vbo for ground
	int posSize = positionGround.size();
	int uvSize = texCoordGround.size();
	glGenBuffers(1, &vboGround);
	glBindBuffer(GL_ARRAY_BUFFER, vboGround);
	glBufferData(GL_ARRAY_BUFFER, (sizeof(glm::vec3) * posSize) + (sizeof(glm::vec2) * uvSize), NULL,
		GL_STATIC_DRAW); // init buffer’s size, but don’t assign any data
	// upload position data
	glBufferSubData(GL_ARRAY_BUFFER, 0, posSize * sizeof(glm::vec3), &positionGround[0]);
	// upload uv data
	glBufferSubData(GL_ARRAY_BUFFER, posSize * sizeof(glm::vec3), sizeof(glm::vec2) * uvSize, &texCoordGround[0]);

	// bind vao for ground 
	glGenVertexArrays(1, &vaoGround);
	glBindVertexArray(vaoGround);
	glBindBuffer(GL_ARRAY_BUFFER, vboGround);

	GLuint loc = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "position");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	loc = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "texCoord");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, (const void*)(posSize * sizeof(glm::vec3)));
	glBindVertexArray(0);

	// upload data for sky
	posSize = positionSky.size();
	uvSize = texCoordSky.size();
	glGenBuffers(1, &vboSky);
	glBindBuffer(GL_ARRAY_BUFFER, vboSky);
	glBufferData(GL_ARRAY_BUFFER, (sizeof(glm::vec3) * posSize) + (sizeof(glm::vec2) * uvSize), NULL,
		GL_STATIC_DRAW); // init buffer’s size, but don’t assign any data
	// upload position data
	glBufferSubData(GL_ARRAY_BUFFER, 0, posSize * sizeof(glm::vec3), &positionSky[0]);
	// upload uv data
	glBufferSubData(GL_ARRAY_BUFFER, posSize * sizeof(glm::vec3), sizeof(glm::vec2) * uvSize, &texCoordSky[0]);

	// bind vao for sky
	glGenVertexArrays(1, &vaoSky);
	glBindVertexArray(vaoSky);
	glBindBuffer(GL_ARRAY_BUFFER, vboSky);

	loc = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "position");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	loc = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "texCoord");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, (const void*)(posSize * sizeof(glm::vec3)));
	glBindVertexArray(0);

	// crossbar VBO
	posSize = crossBar.size();
	uvSize = texCoordBar.size();
	glGenBuffers(1, &vboBar);
	glBindBuffer(GL_ARRAY_BUFFER, vboBar);
	glBufferData(GL_ARRAY_BUFFER, (sizeof(glm::vec3) * posSize) + (sizeof(glm::vec2) * uvSize), NULL,
		GL_STATIC_DRAW); // init buffer’s size, but don’t assign any data
	// upload position data
	glBufferSubData(GL_ARRAY_BUFFER, 0, posSize * sizeof(glm::vec3), &crossBar[0]);
	// upload uv data
	glBufferSubData(GL_ARRAY_BUFFER, posSize * sizeof(glm::vec3), sizeof(glm::vec2) * uvSize, &texCoordBar[0]);
	// crossbar VAO
	glGenVertexArrays(1, &vaoBar);
	glBindVertexArray(vaoBar); // bind the VAO
	glBindBuffer(GL_ARRAY_BUFFER, vboBar);
	loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	loc = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "texCoord");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, (const void*)(posSize * sizeof(glm::vec3)));
	glBindVertexArray(0);

	// side1 VBO
	posSize = positionSide1.size();
	uvSize = texCoordSide1.size();
	glGenBuffers(1, &vboSide1);
	glBindBuffer(GL_ARRAY_BUFFER, vboSide1);
	glBufferData(GL_ARRAY_BUFFER, (sizeof(glm::vec3) * posSize) + (sizeof(glm::vec2) * uvSize), NULL,
		GL_STATIC_DRAW);// init buffer’s size, but don’t assign any data
	// upload position data
	glBufferSubData(GL_ARRAY_BUFFER, 0, posSize * sizeof(glm::vec3), &positionSide1[0]);
	// upload uv data
	glBufferSubData(GL_ARRAY_BUFFER, posSize * sizeof(glm::vec3), sizeof(glm::vec2) * uvSize, &texCoordSide1[0]);

	// side1 vao 
	glGenVertexArrays(1, &vaoSide1);
	glBindVertexArray(vaoSide1);
	glBindBuffer(GL_ARRAY_BUFFER, vboSide1);

	loc = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "position");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	loc = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "texCoord");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, (const void*)(posSize * sizeof(glm::vec3)));
	glBindVertexArray(0);

	// side2 VBO
	posSize = positionSide2.size();
	uvSize = texCoordSide2.size();
	glGenBuffers(1, &vboSide2);
	glBindBuffer(GL_ARRAY_BUFFER, vboSide2);
	glBufferData(GL_ARRAY_BUFFER, (sizeof(glm::vec3) * posSize) + (sizeof(glm::vec2) * uvSize), NULL,
		GL_STATIC_DRAW); // init buffer’s size, but don’t assign any data
	// upload position data
	glBufferSubData(GL_ARRAY_BUFFER, 0, posSize * sizeof(glm::vec3), &positionSide2[0]);
	// upload uv data
	glBufferSubData(GL_ARRAY_BUFFER, posSize * sizeof(glm::vec3), sizeof(glm::vec2) * uvSize, &texCoordSide2[0]);

	// sdie2 VAO
	glGenVertexArrays(1, &vaoSide2);
	glBindVertexArray(vaoSide2);
	glBindBuffer(GL_ARRAY_BUFFER, vboSide2);

	loc = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "position");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	loc = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "texCoord");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, (const void*)(posSize * sizeof(glm::vec3)));
	glBindVertexArray(0);

	// side3 VBO
	posSize = positionSide3.size();
	uvSize = texCoordSide3.size();
	glGenBuffers(1, &vboSide3);
	glBindBuffer(GL_ARRAY_BUFFER, vboSide3);
	glBufferData(GL_ARRAY_BUFFER, (sizeof(glm::vec3) * posSize) + (sizeof(glm::vec2) * uvSize), NULL,
		GL_STATIC_DRAW); // init buffer’s size, but don’t assign any data
	// upload position data
	glBufferSubData(GL_ARRAY_BUFFER, 0, posSize * sizeof(glm::vec3), &positionSide3[0]);
	// upload uv data
	glBufferSubData(GL_ARRAY_BUFFER, posSize * sizeof(glm::vec3), sizeof(glm::vec2) * uvSize, &texCoordSide3[0]);

	// side3 VAO
	glGenVertexArrays(1, &vaoSide3);
	glBindVertexArray(vaoSide3);
	glBindBuffer(GL_ARRAY_BUFFER, vboSide3);

	loc = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "position");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	loc = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "texCoord");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, (const void*)(posSize * sizeof(glm::vec3)));
	glBindVertexArray(0);

	// side4 VBO
	posSize = positionSide4.size();
	uvSize = texCoordSide4.size();
	glGenBuffers(1, &vboSide4);
	glBindBuffer(GL_ARRAY_BUFFER, vboSide4);
	glBufferData(GL_ARRAY_BUFFER, (sizeof(glm::vec3) * posSize) + (sizeof(glm::vec2) * uvSize), NULL,
		GL_STATIC_DRAW); // init buffer’s size, but don’t assign any data
	// upload position data
	glBufferSubData(GL_ARRAY_BUFFER, 0, posSize * sizeof(glm::vec3), &positionSide4[0]);
	// upload uv data
	glBufferSubData(GL_ARRAY_BUFFER, posSize * sizeof(glm::vec3), sizeof(glm::vec2) * uvSize, &texCoordSide4[0]);

	// side4 VAO
	glGenVertexArrays(1, &vaoSide4);
	glBindVertexArray(vaoSide4);
	glBindBuffer(GL_ARRAY_BUFFER, vboSide4);

	loc = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "position");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	loc = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "texCoord");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, (const void*)(posSize * sizeof(glm::vec3)));
	glBindVertexArray(0);
}

void generateSplinePoints()
{
	// generate the splines, tangents, normals and binormals
	basis = transpose(glm::make_mat4x4(basisMatrix));
	glm::vec3 p, t, n, b;
	Point p1, p2, p3, p4;
	Point add1, add2, add3;
	for (int i = 0; i < numSplines; i++)
	{
		for (int j = 0; j < splines[i].numControlPoints - 3; j++)
		{
			p1 = splines[i].points[j];
			p2 = splines[i].points[j + 1];
			p3 = splines[i].points[j + 2];
			p4 = splines[i].points[j + 3];
			float controlpts[12] = { p1.x, p1.y, p1.z,
									p2.x, p2.y, p2.z,
									p3.x, p3.y, p3.z,
									p4.x, p4.y, p4.z };
			control = transpose(glm::make_mat4x3(controlpts));

			for (float u = 0.0f; u <= 1.0f; u += 0.002f)
			{
				// p(u) = [u^3 u^2 u 1] M C
				glm::vec4 U(pow(u, 3), pow(u, 2), u, 1);
				p = U * basis * control;
				splinePts.push_back(p);
				// t(u) = p'(u) = [3u^2 2u 1 0] M C
				glm::vec4 dU(3 * pow(u, 2), 2 * u, 1, 0);
				t = dU * basis * control;
				t = glm::normalize(t);
				tangents.push_back(t);
				if (normals.size() == 0) {
					// N0 = unit(T0 x V)
					n = glm::normalize(cross(t, glm::vec3(.0f, .0f, -1.0f)));
					// B0 = unit(T0 x N0)
					b = glm::normalize(cross(t, n));
				}
				else
				{
					// N1 = unit(B0 x T1) 
					n = glm::normalize(cross(b, t));
					// B1 = unit(T1 x N1)
					b = glm::normalize(cross(t, n));
				}
				normals.push_back(n);
				binormals.push_back(b);
			}
		}
	}
}

void generateRails()
{
	/*
	v0 = p0 + a * (-n0 + b0)
	v1 = p0 + a * (n0 + b0)
	v2 = p0 + a * (n0 - b0)
	v3 = p0 + a * (-n0 - b0)
	*/
	glm::vec3 vLeft[8], vRight[8], p0Left, p0Right, p1Left, p1Right, vLeftWeb[8], vRightWeb[8];
	for (int i = 0; i < splinePts.size() - 1; i++)
	{

		/* T-shaped rail:
		   (vWeb2).__. (vWeb1)
				  |  |
		   (vweb3)|  | (vweb0)    --> web
			 .___.|__|.____.
		(v2) |             | (v1)
		(v3).|_____________|.(v0) --> bottom
		*/
		// The bottom part of T-shaped rail
		p0Left = splinePts[i] - 0.15f * binormals[i]; // left rail p
		vLeft[0] = p0Left + a * (-normals[i]) + a1 * (binormals[i]);
		vLeft[1] = p0Left + a * (normals[i]) + a1 * (binormals[i]);
		vLeft[2] = p0Left + a * (normals[i]) + a1 * (-binormals[i]);
		vLeft[3] = p0Left + a * (-normals[i]) + a1 * (-binormals[i]);

		p0Right = splinePts[i] + 0.15f * binormals[i]; // right rail p
		vRight[0] = p0Right + a * (-normals[i]) + a1 * (binormals[i]);
		vRight[1] = p0Right + a * (normals[i]) + a1 * (binormals[i]);
		vRight[2] = p0Right + a * (normals[i]) + a1 * (-binormals[i]);
		vRight[3] = p0Right + a * (-normals[i]) + a1 * (-binormals[i]);

		p1Left = splinePts[i + 1] - 0.15f * binormals[i + 1]; // left rail p1
		vLeft[4] = p1Left + a * (-normals[i + 1]) + a1 * (binormals[i + 1]);
		vLeft[5] = p1Left + a * (normals[i + 1]) + a1 * (binormals[i + 1]);
		vLeft[6] = p1Left + a * (normals[i + 1]) + a1 * (-binormals[i + 1]);
		vLeft[7] = p1Left + a * (-normals[i + 1]) + a1 * (-binormals[i + 1]);

		p1Right = splinePts[i + 1] + 0.15f * binormals[i + 1]; // right rail p1 
		vRight[4] = p1Right + a * (-normals[i + 1]) + a1 * (binormals[i + 1]);
		vRight[5] = p1Right + a * (normals[i + 1]) + a1 * (binormals[i + 1]);
		vRight[6] = p1Right + a * (normals[i + 1]) + a1 * (-binormals[i + 1]);
		vRight[7] = p1Right + a * (-normals[i + 1]) + a1 * (-binormals[i + 1]);

		// The web part of T-shaped rail
		vLeftWeb[0] = vLeft[1]; vLeftWeb[0] -= 0.015f * binormals[i];
		vLeftWeb[3] = vLeft[2]; vLeftWeb[3] += 0.015f * binormals[i];
		vLeftWeb[1] = vLeftWeb[0] + 0.04f * normals[i];
		vLeftWeb[2] = vLeftWeb[3] + 0.04f * normals[i];

		vLeftWeb[4] = vLeft[5]; vLeftWeb[4] -= 0.015f * binormals[i + 1];
		vLeftWeb[7] = vLeft[6]; vLeftWeb[7] += 0.015f * binormals[i + 1];
		vLeftWeb[5] = vLeftWeb[4] + 0.04f * normals[i + 1];
		vLeftWeb[6] = vLeftWeb[7] + 0.04f * normals[i + 1];

		vRightWeb[0] = vRight[1]; vRightWeb[0] -= 0.015f * binormals[i];
		vRightWeb[3] = vRight[2]; vRightWeb[3] += 0.015f * binormals[i];
		vRightWeb[1] = vRightWeb[0] + 0.04f * normals[i];
		vRightWeb[2] = vRightWeb[3] + 0.04f * normals[i];

		vRightWeb[4] = vRight[5]; vRightWeb[4] -= 0.015f * binormals[i + 1];
		vRightWeb[7] = vRight[6]; vRightWeb[7] += 0.015f * binormals[i + 1];
		vRightWeb[5] = vRightWeb[4] + 0.04f * normals[i + 1];
		vRightWeb[6] = vRightWeb[7] + 0.04f * normals[i + 1];

		// left rail bottom part vertices
		// left face
		leftRail.push_back(vLeft[3]); leftRail.push_back(vLeft[7]); leftRail.push_back(vLeft[2]);
		leftRailNormals.push_back(-binormals[i]);
		leftRailNormals.push_back(-binormals[i + 1]);
		leftRailNormals.push_back(-binormals[i]);
		leftRail.push_back(vLeft[2]); leftRail.push_back(vLeft[7]); leftRail.push_back(vLeft[6]);
		leftRailNormals.push_back(-binormals[i]);
		leftRailNormals.push_back(-binormals[i + 1]);
		leftRailNormals.push_back(-binormals[i + 1]);
		// top face
		leftRail.push_back(vLeft[2]); leftRail.push_back(vLeft[1]); leftRail.push_back(vLeft[5]);
		leftRailNormals.push_back(normals[i]);
		leftRailNormals.push_back(normals[i]);
		leftRailNormals.push_back(normals[i + 1]);
		leftRail.push_back(vLeft[5]); leftRail.push_back(vLeft[2]); leftRail.push_back(vLeft[6]);
		leftRailNormals.push_back(normals[i + 1]);
		leftRailNormals.push_back(normals[i]);
		leftRailNormals.push_back(normals[i + 1]);
		// right face
		leftRail.push_back(vLeft[0]); leftRail.push_back(vLeft[1]); leftRail.push_back(vLeft[5]);
		leftRailNormals.push_back(binormals[i]);
		leftRailNormals.push_back(binormals[i]);
		leftRailNormals.push_back(binormals[i + 1]);
		leftRail.push_back(vLeft[5]); leftRail.push_back(vLeft[0]); leftRail.push_back(vLeft[4]);
		leftRailNormals.push_back(binormals[i + 1]);
		leftRailNormals.push_back(binormals[i]);
		leftRailNormals.push_back(binormals[i + 1]);
		// bottom face
		leftRail.push_back(vLeft[0]); leftRail.push_back(vLeft[3]); leftRail.push_back(vLeft[7]);
		leftRailNormals.push_back(-normals[i]);
		leftRailNormals.push_back(-normals[i]);
		leftRailNormals.push_back(-normals[i + 1]);
		leftRail.push_back(vLeft[7]); leftRail.push_back(vLeft[0]); leftRail.push_back(vLeft[4]);
		leftRailNormals.push_back(-normals[i + 1]);
		leftRailNormals.push_back(-normals[i]);
		leftRailNormals.push_back(-normals[i + 1]);

		// left rail web part vertices
		// left face
		leftRail.push_back(vLeftWeb[3]); leftRail.push_back(vLeftWeb[7]); leftRail.push_back(vLeftWeb[2]);
		leftRailNormals.push_back(-binormals[i]);
		leftRailNormals.push_back(-binormals[i + 1]);
		leftRailNormals.push_back(-binormals[i]);
		leftRail.push_back(vLeftWeb[2]); leftRail.push_back(vLeftWeb[7]); leftRail.push_back(vLeftWeb[6]);
		leftRailNormals.push_back(-binormals[i]);
		leftRailNormals.push_back(-binormals[i + 1]);
		leftRailNormals.push_back(-binormals[i + 1]);
		// top face
		leftRail.push_back(vLeftWeb[2]); leftRail.push_back(vLeftWeb[1]); leftRail.push_back(vLeftWeb[5]);
		leftRailNormals.push_back(normals[i]);
		leftRailNormals.push_back(normals[i]);
		leftRailNormals.push_back(normals[i + 1]);
		leftRail.push_back(vLeftWeb[5]); leftRail.push_back(vLeftWeb[2]); leftRail.push_back(vLeftWeb[6]);
		leftRailNormals.push_back(normals[i + 1]);
		leftRailNormals.push_back(normals[i]);
		leftRailNormals.push_back(normals[i + 1]);
		// right face
		leftRail.push_back(vLeftWeb[0]); leftRail.push_back(vLeftWeb[1]); leftRail.push_back(vLeftWeb[5]);
		leftRailNormals.push_back(binormals[i]);
		leftRailNormals.push_back(binormals[i]);
		leftRailNormals.push_back(binormals[i + 1]);
		leftRail.push_back(vLeftWeb[5]); leftRail.push_back(vLeftWeb[0]); leftRail.push_back(vLeftWeb[4]);
		leftRailNormals.push_back(binormals[i + 1]);
		leftRailNormals.push_back(binormals[i]);
		leftRailNormals.push_back(binormals[i + 1]);
		// bottom face
		leftRail.push_back(vLeftWeb[0]); leftRail.push_back(vLeftWeb[3]); leftRail.push_back(vLeftWeb[7]);
		leftRailNormals.push_back(-normals[i]);
		leftRailNormals.push_back(-normals[i]);
		leftRailNormals.push_back(-normals[i + 1]);
		leftRail.push_back(vLeftWeb[7]); leftRail.push_back(vLeftWeb[0]); leftRail.push_back(vLeftWeb[4]);
		leftRailNormals.push_back(-normals[i + 1]);
		leftRailNormals.push_back(-normals[i]);
		leftRailNormals.push_back(-normals[i + 1]);

		// right rail bottom part vertices
		// left face
		rightRail.push_back(vRight[3]); rightRail.push_back(vRight[7]); rightRail.push_back(vRight[2]);
		rightRailNormals.push_back(-binormals[i]);
		rightRailNormals.push_back(-binormals[i + 1]);
		rightRailNormals.push_back(-binormals[i]);
		rightRail.push_back(vRight[2]); rightRail.push_back(vRight[7]); rightRail.push_back(vRight[6]);
		rightRailNormals.push_back(-binormals[i]);
		rightRailNormals.push_back(-binormals[i + 1]);
		rightRailNormals.push_back(-binormals[i + 1]);
		// top face
		rightRail.push_back(vRight[2]); rightRail.push_back(vRight[1]); rightRail.push_back(vRight[5]);
		rightRailNormals.push_back(normals[i]);
		rightRailNormals.push_back(normals[i]);
		rightRailNormals.push_back(normals[i + 1]);
		rightRail.push_back(vRight[5]); rightRailNormals.push_back(normals[i + 1]);
		rightRail.push_back(vRight[2]); rightRailNormals.push_back(normals[i]);
		rightRail.push_back(vRight[6]); rightRailNormals.push_back(normals[i + 1]);
		// right face
		rightRail.push_back(vRight[0]); rightRail.push_back(vRight[1]); rightRail.push_back(vRight[5]);
		rightRailNormals.push_back(binormals[i]);
		rightRailNormals.push_back(binormals[i]);
		rightRailNormals.push_back(binormals[i + 1]);
		rightRail.push_back(vRight[5]); rightRail.push_back(vRight[0]); rightRail.push_back(vRight[4]);
		rightRailNormals.push_back(binormals[i + 1]);
		rightRailNormals.push_back(binormals[i]);
		rightRailNormals.push_back(binormals[i + 1]);
		// bottom face
		rightRail.push_back(vRight[0]); rightRail.push_back(vRight[3]); rightRail.push_back(vRight[7]);
		rightRailNormals.push_back(-normals[i]);
		rightRailNormals.push_back(-normals[i]);
		rightRailNormals.push_back(-normals[i + 1]);
		rightRail.push_back(vRight[7]); rightRail.push_back(vRight[0]); rightRail.push_back(vRight[4]);
		rightRailNormals.push_back(-normals[i + 1]);
		rightRailNormals.push_back(-normals[i]);
		rightRailNormals.push_back(-normals[i + 1]);

		// right rail web part vertices
		// left face
		rightRail.push_back(vRightWeb[3]); rightRail.push_back(vRightWeb[7]); rightRail.push_back(vRightWeb[2]);
		rightRailNormals.push_back(-binormals[i]);
		rightRailNormals.push_back(-binormals[i + 1]);
		rightRailNormals.push_back(-binormals[i]);
		rightRail.push_back(vRightWeb[2]); rightRail.push_back(vRightWeb[7]); rightRail.push_back(vRightWeb[6]);
		rightRailNormals.push_back(-binormals[i]);
		rightRailNormals.push_back(-binormals[i + 1]);
		rightRailNormals.push_back(-binormals[i + 1]);
		// top face
		rightRail.push_back(vRightWeb[2]); rightRail.push_back(vRightWeb[1]); rightRail.push_back(vRightWeb[5]);
		rightRailNormals.push_back(normals[i]);
		rightRailNormals.push_back(normals[i]);
		rightRailNormals.push_back(normals[i + 1]);
		rightRail.push_back(vRightWeb[5]); rightRail.push_back(vRightWeb[2]); rightRail.push_back(vRightWeb[6]);
		rightRailNormals.push_back(normals[i + 1]);
		rightRailNormals.push_back(normals[i]);
		rightRailNormals.push_back(normals[i + 1]);
		// right face
		rightRail.push_back(vRightWeb[0]); rightRail.push_back(vRightWeb[1]); rightRail.push_back(vRightWeb[5]);
		rightRailNormals.push_back(binormals[i]);
		rightRailNormals.push_back(binormals[i]);
		rightRailNormals.push_back(binormals[i + 1]);
		rightRail.push_back(vRightWeb[5]); rightRail.push_back(vRightWeb[0]); rightRail.push_back(vRightWeb[4]);
		rightRailNormals.push_back(binormals[i + 1]);
		rightRailNormals.push_back(binormals[i]);
		rightRailNormals.push_back(binormals[i + 1]);
		// bottom face
		rightRail.push_back(vRightWeb[0]); rightRail.push_back(vRightWeb[3]); rightRail.push_back(vRightWeb[7]);
		rightRailNormals.push_back(-normals[i]);
		rightRailNormals.push_back(-normals[i]);
		rightRailNormals.push_back(-normals[i + 1]);
		rightRail.push_back(vRightWeb[7]); rightRail.push_back(vRightWeb[0]); rightRail.push_back(vRightWeb[4]);
		rightRailNormals.push_back(-normals[i + 1]);
		rightRailNormals.push_back(-normals[i]);
		rightRailNormals.push_back(-normals[i + 1]);
		// crossbar vertices
		if (i % 50 == 0)
		{
			// bottom face
			// to make the crossbar wider, expanding along the tangent direction
			crossBar.push_back(vRight[3]);
			crossBar.push_back((vRight[7] + 0.06f * tangents[i + 1]));
			crossBar.push_back((vLeft[4] + 0.06f * tangents[i + 1]));
			crossBarNormals.push_back(-normals[i]);
			crossBarNormals.push_back(-normals[i + 1]);
			crossBarNormals.push_back(-normals[i + 1]);
			crossBar.push_back((vLeft[4] + 0.06f * tangents[i + 1]));
			crossBar.push_back(vLeft[0]);
			crossBar.push_back(vRight[3]);
			crossBarNormals.push_back(-normals[i + 1]);
			crossBarNormals.push_back(-normals[i]);
			crossBarNormals.push_back(-normals[i]);
			addUVs(&texCoordBar); // fill texture vertices
			// back face
			crossBar.push_back((vLeft[4] + 0.06f * tangents[i + 1]));
			crossBar.push_back((vLeft[5] + 0.06f * tangents[i + 1]));
			crossBar.push_back((vRight[6] + 0.06f * tangents[i + 1]));
			crossBar.push_back((vRight[6] + 0.06f * tangents[i + 1]));
			crossBar.push_back((vRight[7] + 0.06f * tangents[i + 1]));
			crossBar.push_back((vLeft[4] + 0.06f * tangents[i + 1]));
			for (int k = 0; i < 6; i++) crossBarNormals.push_back(tangents[i + 1]);
			addUVs(&texCoordBar); // fill texture vertices
			// top face
			crossBar.push_back(vRight[2]);
			crossBar.push_back((vRight[6] + 0.06f * tangents[i + 1]));
			crossBar.push_back((vLeft[5] + 0.06f * tangents[i + 1]));
			crossBarNormals.push_back(normals[i]);
			crossBarNormals.push_back(normals[i + 1]);
			crossBarNormals.push_back(normals[i + 1]);
			crossBar.push_back((vLeft[5] + 0.06f * tangents[i + 1]));
			crossBar.push_back(vLeft[1]);
			crossBar.push_back(vRight[2]);
			crossBarNormals.push_back(normals[i + 1]);
			crossBarNormals.push_back(normals[i]);
			crossBarNormals.push_back(normals[i]);
			addUVs(&texCoordBar); // fill texture vertices
			// front face
			crossBar.push_back(vLeft[1]);
			crossBar.push_back(vLeft[0]);
			crossBar.push_back(vRight[3]);
			crossBar.push_back(vRight[3]);
			crossBar.push_back(vRight[2]);
			crossBar.push_back(vLeft[1]);
			for (int k = 0; i < 6; i++) crossBarNormals.push_back(-tangents[i]);
			addUVs(&texCoordBar); // fill texture vertices
		}
	}
}

void initRails_VBO_VAO()
{
	// set VBO for left rail vertices
	glGenBuffers(1, &vboLeftRail);
	glBindBuffer(GL_ARRAY_BUFFER, vboLeftRail);  // bind VBO buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * leftRail.size(), leftRail.data(), GL_STATIC_DRAW);
	// set VBO for left rail normals
	glGenBuffers(1, &vboLeftNormals);
	glBindBuffer(GL_ARRAY_BUFFER, vboLeftNormals);  // bind VBO buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * leftRailNormals.size(), leftRailNormals.data(), GL_STATIC_DRAW);

	// set VAO for left rail vertices and normals
	glGenVertexArrays(1, &vaoLeftRail);
	glBindVertexArray(vaoLeftRail); // bind the VAO
	glBindBuffer(GL_ARRAY_BUFFER, vboLeftRail);
	GLuint loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	glBindBuffer(GL_ARRAY_BUFFER, vboLeftNormals);
	loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "normal");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);
	glBindVertexArray(0); // unbind the VAO

	// set VBO for right rail vertices
	glGenBuffers(1, &vboRightRail);
	glBindBuffer(GL_ARRAY_BUFFER, vboRightRail);  // bind VBO buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * rightRail.size(), rightRail.data(), GL_STATIC_DRAW);
	// set VBO for right rial normals
	glGenBuffers(1, &vboRightNormals);
	glBindBuffer(GL_ARRAY_BUFFER, vboRightNormals);  // bind VBO buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * rightRailNormals.size(), rightRailNormals.data(), GL_STATIC_DRAW);

	// set VAO for right rail vertices and normals
	glGenVertexArrays(1, &vaoRightRail);
	glBindVertexArray(vaoRightRail); // bind the VAO
	glBindBuffer(GL_ARRAY_BUFFER, vboRightRail);
	loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	glBindBuffer(GL_ARRAY_BUFFER, vboRightNormals);
	loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "normal");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);
	glBindVertexArray(0); // unbind the VAO
}

void initScene(int argc, char* argv[])
{
	// background color
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	glEnable(GL_DEPTH_TEST);

	generateSplinePoints();
	generateRails();

	// init pipeline program
	pipelineProgram = new BasicPipelineProgram;
	int ret = pipelineProgram->Init(shaderBasePath);
	if (ret != 0) abort();

	pipelineProgram->Bind();

	initRails_VBO_VAO();

	generateGround();
	generateSkyBox();

	// init texture pipeline program
	texturePipelineProgram = new BasicPipelineProgram;
	ret = texturePipelineProgram->BuildShadersFromFiles(shaderBasePath, "texture.vertexShader.glsl", "texture.fragmentShader.glsl");
	if (ret != 0) abort();

	texturePipelineProgram->Bind();

	// setting texture handles
	glGenTextures(1, &groundTexHandle);
	int code = initTexture("moon.jpg", groundTexHandle);
	if (code != 0)
	{
		printf("Error loading the texture image.\n");
		exit(EXIT_FAILURE);
	}
	glGenTextures(1, &skyTexHandle);
	code = initTexture("mars.jpg", skyTexHandle);
	if (code != 0)
	{
		printf("Error loading the texture image.\n");
		exit(EXIT_FAILURE);
	}
	glGenTextures(1, &barTexHandle);
	code = initTexture("Floor.jpg", barTexHandle);
	if (code != 0)
	{
		printf("Error loading the texture image.\n");
		exit(EXIT_FAILURE);
	}
	glGenTextures(1, &sideTexHandle1);
	code = initTexture("galaxy1.jpg", sideTexHandle1);
	if (code != 0)
	{
		printf("Error loading the texture image.\n");
		exit(EXIT_FAILURE);
	}
	glGenTextures(1, &sideTexHandle2);
	code = initTexture("galaxy2.jpg", sideTexHandle2);
	if (code != 0)
	{
		printf("Error loading the texture image.\n");
		exit(EXIT_FAILURE);
	}
	glGenTextures(1, &sideTexHandle3);
	code = initTexture("galaxy3.jpg", sideTexHandle3);
	if (code != 0)
	{
		printf("Error loading the texture image.\n");
		exit(EXIT_FAILURE);
	}
	glGenTextures(1, &sideTexHandle4);
	code = initTexture("galaxy4.jpg", sideTexHandle4);
	if (code != 0)
	{
		printf("Error loading the texture image.\n");
		exit(EXIT_FAILURE);
	}

	initTextureVBO_VAO();

	std::cout << "GL error: " << glGetError() << std::endl;

}

// Note for Windows/MS Visual Studio:
// You should set argv[1] to track.txt.
// To do this, on the "Solution Explorer",
// right click your project, choose "Properties",
// go to "Configuration Properties", click "Debug",
// then type your track file name for the "Command Arguments".
// You can also repeat this process for the "Release" configuration.

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("usage: %s <trackfile>\n", argv[0]);
		exit(0);
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

	// load the splines from the provided filename
	loadSplines(argv[1]);

	printf("Loaded %d spline(s).\n", numSplines);
	for (int i = 0; i < numSplines; i++)
		printf("Num control points in spline %d: %d.\n", i, splines[i].numControlPoints);

	// do initialization
	initScene(argc, argv);

	// sink forever into the glut loop
	glutMainLoop();

	return 0;
	}
