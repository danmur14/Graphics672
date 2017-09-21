// ModelView.c++ - a basic combined Model and View for OpenGL

#include <iostream>

#include "ModelView.h"
#include "Controller.h"
#include "ShaderIF.h"

double ModelView::mcRegionOfInterest[6] = { -1.0, 1.0, -1.0, 1.0, -1.0, 1.0 };
bool ModelView::aspectRatioPreservationEnabled = true;
vec4 ModelView::colors[6] = {
	{1.0, 0.0, 0.0, 1.0},
	{0.0, 1.0, 0.0, 1.0},
	{0.0, 0.0, 1.0, 1.0},
	{1.0, 0.0, 1.0, 1.0},
	{1.0, 1.0, 0.0, 1.0},
	{0.0, 1.0, 1.0, 1.0}
}; // list of possible colors

ModelView::ModelView(ShaderIF* sIF, vec2* curveCoordinates, int numPoints, int colorMode) : shaderIF(sIF)
{
	nPoints = numPoints;
	color = colorMode;
	initModelGeometry(curveCoordinates);
}

ModelView::~ModelView()
{
	if (vao[0] > 0) // hasn't already been deleted
	{
		glDeleteBuffers(1, vbo);
		glDeleteVertexArrays(1, vao);
		vao[0] = vbo[0] = 0;
	}
}

void ModelView::compute2DScaleTrans(float* scaleTransF) // CLASS METHOD
{
	double xmin = mcRegionOfInterest[0];
	double xmax = mcRegionOfInterest[1];
	double ymin = mcRegionOfInterest[2];
	double ymax = mcRegionOfInterest[3];

	if (aspectRatioPreservationEnabled)
	{
		// preserve aspect ratio. Make "region of interest" wider or taller to
		// match the Controller's viewport aspect ratio.
		double vAR = Controller::getCurrentController()->getViewportAspectRatio();
		matchAspectRatio(xmin, xmax, ymin, ymax, vAR);
	}

    // We are only concerned with the xy extents for now, hence we will
    // ignore mcRegionOfInterest[4] and mcRegionOfInterest[5].
    // Map the overall limits to the -1..+1 range expected by the OpenGL engine:
	double scaleTrans[4];
	linearMap(xmin, xmax, -1.0, 1.0, scaleTrans[0], scaleTrans[1]);
	linearMap(ymin, ymax, -1.0, 1.0, scaleTrans[2], scaleTrans[3]);
	for (int i=0 ; i<4 ; i++)
		scaleTransF[i] = static_cast<float>(scaleTrans[i]);
}

// xyzLimits: {mcXmin, mcXmax, mcYmin, mcYmax, mcZmin, mcZmax}
void ModelView::getMCBoundingBox(double* xyzLimits) const
{
	//  (-1 .. +1 is OK for z direction for 2D models)
	xyzLimits[0] = xmi; xyzLimits[1] = xma;
	xyzLimits[2] = ymi; xyzLimits[3] = yma;
	xyzLimits[4] = -1.0; xyzLimits[5] =  1.0;
}

bool ModelView::handleCommand(unsigned char anASCIIChar, double ldsX, double ldsY)
{
	return true;
}

// linearMap determines the scale and translate parameters needed in
// order to map a value, f (fromMin <= f <= fromMax) to its corresponding
// value, t (toMin <= t <= toMax). Specifically: t = scale*f + trans.
void ModelView::linearMap(double fromMin, double fromMax, double toMin, double toMax,
					  double& scale, double& trans) // CLASS METHOD
{
	scale = (toMax - toMin) / (fromMax - fromMin);
	trans = toMin - scale*fromMin;
}

void ModelView::matchAspectRatio(double& xmin, double& xmax,
        double& ymin, double& ymax, double vAR)
{

	double wHeight = ymax - ymin;
	double wWidth = xmax - xmin;
	double wAR = wHeight / wWidth;
	if (wAR > vAR)
	{
		// make window wider
		wWidth = wHeight / vAR;
		double xmid = 0.5 * (xmin + xmax);
		xmin = xmid - 0.5*wWidth;
		xmax = xmid + 0.5*wWidth;
	}
	else
	{
		// make window taller
		wHeight = wWidth * vAR;
		double ymid = 0.5 * (ymin + ymax);
		ymin = ymid - 0.5*wHeight;
		ymax = ymid + 0.5*wHeight;
	}
}

void ModelView::render() const
{
	// save the current GLSL program in use
	GLint pgm;
	glGetIntegerv(GL_CURRENT_PROGRAM, &pgm);

	// use shader
	glUseProgram(shaderIF->getShaderPgmID());

	// calculate scaleTrans
	float scaleTrans[4];
	compute2DScaleTrans(scaleTrans);
	glUniform4fv(shaderIF->ppuLoc("scaleTrans"), 1, scaleTrans);

	// pass color to fragment shader, must be used to change program
	glUniform4fv(shaderIF->ppuLoc("colorMode"), 1, colors[color]);

	// bind curve VAO
	glBindVertexArray(vao[0]);

	// draw curve
	glDrawArrays(GL_LINE_STRIP, 0, nPoints);

	// restore the previous program
	glUseProgram(pgm);
}

void ModelView::setMCRegionOfInterest(double xyz[6])
{
	for (int i=0 ; i<6 ; i++)
		mcRegionOfInterest[i] = xyz[i];
}

void ModelView::initModelGeometry(vec2* curveCoordinates)
{
	// Create the VAO and VBO
	glGenVertexArrays(1, vao);
	glGenBuffers(1, vbo);

	// Initialize them
	glBindVertexArray(vao[0]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);

	// Determine and remember the min/max coordinates
	xmi = xma = curveCoordinates[0][0];
	ymi = yma = curveCoordinates[0][1];
	for (int i = 1; i < nPoints; i++)
	{
		if (curveCoordinates[i][0] < xmi)
			xmi = curveCoordinates[i][0];
		else if (curveCoordinates[i][0] > xma)
			xma = curveCoordinates[i][0];
		if (curveCoordinates[i][1] < ymi)
			ymi = curveCoordinates[i][1];
		else if (curveCoordinates[i][1] > yma)
			yma = curveCoordinates[i][1];
	}

	// Allocate space for AND send data to GPU
	int numBytesInBuffer = nPoints * sizeof(vec2);
	glBufferData(GL_ARRAY_BUFFER, numBytesInBuffer, curveCoordinates, GL_STATIC_DRAW);
	glVertexAttribPointer(shaderIF->pvaLoc("mcPosition"), 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(shaderIF->pvaLoc("mcPosition"));
}
