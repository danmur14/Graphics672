// project1.c++

#include "GLFWController.h"
#include "ShaderIF.h"
#include "ModelView.h"

#include <fstream>
#include <istream>
#include <math.h>

int main(int argc, char* argv[])
{
	GLFWController c(argv[0]);
	c.reportVersions(std::cout);

	ShaderIF* sIF = new ShaderIF("shaders/project1.vsh", "shaders/project1.fsh"); // initialize shader

	std::ifstream data; 
	float constants[8]; // 0-3 a, 4-7 b
	float tmin, tmax;
	int nPoints;
	int colorMode = 0; // determines color in modelview
	data.open("data.txt"); // open data.txt file with data

	while(data >> constants[0]) // read until end of file
	{
		// fill array with constant values and get tmin, tmax, nPoints
		for (int i = 1; i < 8; i++)
		{
			data >> constants[i];
		}
		data >> tmin;
		data >> tmax;
		data >> nPoints;

		// create array for curve coordinates and calculate dt
		vec2 curveCoordinates[nPoints];
		float dt = (tmax - tmin) / (nPoints - 1);

		// calculate coordinates and fill array
		for (int i = 0; i < nPoints; i++)
		{
			float t = tmin + i * dt;
			float xt = (constants[0] + (constants[1] * t) + (constants[2] * pow(t, 2)) + (constants[3] * pow(t, 3)));
			float yt = (constants[4] + (constants[5] * t) + (constants[6] * pow(t, 2)) + (constants[7] * pow(t, 3)));
			curveCoordinates[i][0] = xt;
			curveCoordinates[i][1] = yt;
		}

		c.addModel( new ModelView(sIF, curveCoordinates, nPoints, colorMode) ); // add model
		colorMode++; // next model has next color
	}

	data.close();

	// initialize 2D viewing information:
	// Get the overall scene bounding box in Model Coordinates:
	double xyz[6]; // xyz limits, even though this is 2D
	c.getOverallMCBoundingBox(xyz);
	// Tell class ModelView we initially want to see the whole scene:
	ModelView::setMCRegionOfInterest(xyz);

	glClearColor(1.0, 1.0, 1.0, 1.0); // white bg

	c.run();

	delete sIF;

	return 0;
}
