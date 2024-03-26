/*==================================================================================
* COSC363 Assignment 2
*
* Jemima Gregory
*===================================================================================
*/
#include <iostream>
#include <cmath>
#include <vector>
#include <glm/glm.hpp>
#include "Sphere.h"
#include "SceneObject.h"
#include "Ray.h"
#include "Plane.h"
#include "TextureBMP.h"
#include "Sphere.h"
#include <GL/freeglut.h>
using namespace std;

const float EDIST = 40.0;
const int NUMDIV = 500;
const int MAX_STEPS = 5;
const float XMIN = -10.0;
const float XMAX = 10.0;
const float YMIN = -10.0;
const float YMAX = 10.0;
TextureBMP texture;

vector<SceneObject*> sceneObjects;

//---The most important function in a ray tracer! ---------------------------------- 
//   Computes the colour value obtained by tracing a ray and finding its 
//     closest point of intersection with objects in the scene.
//----------------------------------------------------------------------------------
glm::vec3 trace(Ray ray, int step)
{
	glm::vec3 backgroundCol(0);						//Background colour = (0,0,0)
    glm::vec3 lightPos(30, 15, -30);					//Light's position
    glm::vec3 color(0);
	SceneObject* obj;

    ray.closestPt(sceneObjects);					//Compare the ray with all objects in the scene
    if(ray.index == -1) return backgroundCol;		//no intersection
	obj = sceneObjects[ray.index];					//object on which the closest point of intersection is found


    if (ray.index == 1) //1 is the index of the celiling plane in the sceneObjects list
    {
        // Diamond pattern
        int diamondSize = 10; // the size of each diamond
        int ix = (ray.hit.x + ray.hit.z)/ diamondSize;
        int iz = ray.hit.z / diamondSize;
        int k = (ix + iz) % 2;
        glm::vec3 celing_color = k == 0 ? glm::vec3(1, 0, 1) : glm::vec3(0, 1, 1); // Alternate between diamond colours
        obj->setColor(celing_color);
    }

    if (ray.index == 0) //0 is the index of the floor plane in the sceneObjects list
    {
        // Checker pattern
        int cheqSize = 15; // the size of each square
        int ix = (ray.hit.x) / cheqSize; //moving it away from 0 for calculations
        int iz = (ray.hit.z) / cheqSize;
        int j = ix % 2;
        int k = iz % 2;
        if (ray.hit.x < 0) {

            j = !j;
        }
        int m = (j+k)%2;
        glm::vec3 color = m == 0 ? glm::vec3(0, 0, 0) : glm::vec3(0, 1, 0.3); // Alternate between colours
        obj->setColor(color);
    }

    if (ray.index == 11) //11 is the index of the sphere to be textured in the sceneObjects list
    {
        //uv mapping a sphere
        glm::vec3 origin = glm::vec3(-10.0, 10.0, -100.0);

        //unit vector from the hit point to the sphere's origin
        glm::vec3 d = glm::normalize(ray.hit - origin);

        float texcoord_u = 0.5 + (atan(d.z/d.x) / (2 * M_PI));
        float texcoord_v = 0.5 + (asin(d.y) / M_PI);

        color=texture.getColorAt(texcoord_u, texcoord_v);
        obj->setColor(color);

    }


    color = obj->lighting(lightPos,-ray.dir, ray.hit);						//Object's colour

    glm::vec3 lightVec = lightPos - ray.hit;
    Ray shadowRay(ray.hit, lightVec);
    shadowRay.closestPt(sceneObjects);
    //glm::length(lightVeForc);
    if((shadowRay.index > -1) && (shadowRay.dist < glm::length(lightVec))) {
        color = 0.2f * obj->getColor(); //0.2 = ambient scale factor
    }

    //Lighter Shadows for transparent and refractive objects
    SceneObject* shadowObj = sceneObjects[shadowRay.index];
    if (shadowObj->isTransparent() || shadowObj->isRefractive()) {
        color = 0.4f * obj->getColor();
    }

    //implementing a spotlight
    glm::vec3 spotlightPos(0, 39, 0);
    glm::vec3 spotlightAimPoint(0, -38, -200);
    glm::vec3 spotlightDir = spotlightAimPoint - spotlightPos;
    float cut_off_angle = 8.0;
    glm::vec3 spotlightVec = ray.hit - spotlightPos;

    Ray spotlightShadowRay(ray.hit, spotlightPos);
    spotlightShadowRay.closestPt(sceneObjects);

    //the dot product between the vectors
    float dotProduct = glm::dot(spotlightDir, spotlightVec);

    //magnitudes of the vectors
    float magnitude1 = glm::length(spotlightDir);
    float magnitude2 = glm::length(spotlightVec);

    //calculating the angle
    float angle = acos(dotProduct / (magnitude1 * magnitude2));

    //converting from radians to degrees
    float angleBetween = glm::degrees(angle);


    //check if angle is greater than the cut-off angle -if it is NOT in the spotlight
    if(angleBetween > cut_off_angle) {
        color *= 0.7; //dim everything thats not in the spotlight
    } else {
        color /= 0.6;
    }



    if (obj->isTransparent() && step < MAX_STEPS)
    {
        // Get the transparency coefficient
        float transparencyCoeff = obj->getTransparencyCoeff();

        // Find the closest object behind the transparent object
        //closest point (otherside of sphere)
        Ray closestRay = Ray(ray.hit, ray.dir);
        closestRay.closestPt(sceneObjects);

        //closest object outside the sphere?
        Ray secondClosestRay = Ray(closestRay.hit, ray.dir);
        secondClosestRay.closestPt(sceneObjects);

        glm::vec3 behindColor = trace(secondClosestRay, step + 1);

        color = transparencyCoeff * behindColor + (1 - transparencyCoeff) * obj->getColor();
    }



    if (obj->isRefractive() && step < MAX_STEPS)
    {
        // Get the transparency coefficient
        float refractionCoeff = obj->getRefractionCoeff();

        // Get the transparency coefficient
        float refractiveIndex = obj->getRefractiveIndex();

        //g

        glm::vec3 normalVec = obj->normal(ray.hit);
        //direction for g
        glm::vec3 closestDir = glm::refract(ray.dir,normalVec,1/refractiveIndex);

        //closest point (otherside of sphere)
        Ray closestRay(ray.hit, closestDir);
        closestRay.closestPt(sceneObjects);


        //h

        glm::vec3 secondNormalVec = -obj->normal(closestRay.hit);

        //direction for h
        glm::vec3 secondClosestDir = glm::refract(closestRay.dir,secondNormalVec,refractiveIndex);

        //closest object outside the sphere
        Ray secondClosestRay(closestRay.hit, secondClosestDir);


        glm::vec3 behindColor = trace(secondClosestRay, step + 1);

        color = refractionCoeff * behindColor + (1 - refractionCoeff) * obj->getColor();
    }

    if (obj->isReflective() && step < MAX_STEPS)
    {
        float rho = obj->getReflectionCoeff();
        glm::vec3 normalVec = obj->normal(ray.hit);
        glm::vec3 reflectedDir = glm::reflect(ray.dir, normalVec);
        Ray reflectedRay(ray.hit, reflectedDir);
        glm::vec3 reflectedColor = trace(reflectedRay, step + 1);
        color = color + (rho * reflectedColor);
    }

	return color;
}



//---The main display module -----------------------------------------------------------
// In a ray tracing application, it just displays the ray traced image by drawing
// each cell as a quad.
//---------------------------------------------------------------------------------------
void display()
{
	float xp, yp;  //grid point
	float cellX = (XMAX - XMIN) / NUMDIV;  //cell width
	float cellY = (YMAX - YMIN) / NUMDIV;  //cell height
	glm::vec3 eye(0., 0., 0.);

	glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

	glBegin(GL_QUADS);  //Each cell is a tiny quad.

	for (int i = 0; i < NUMDIV; i++)	//Scan every cell of the image plane
	{
		xp = XMIN + i * cellX;
		for (int j = 0; j < NUMDIV; j++)
		{
			yp = YMIN + j * cellY;

            //initial ray
            glm::vec3 dir(xp + 0.5 * cellX, yp + 0.5 * cellY, -EDIST);	//direction of the primary ray
            Ray ray = Ray(eye, dir);
            glm::vec3 col = trace(ray, 1);
            glColor3f(col.r, col.g, col.b);



            //generate 4 rays rather than 1
            glm::vec3 dir1(xp + 0.25 * cellX, yp + 0.25 * cellY, -EDIST);
            glm::vec3 dir2(xp + 0.75 * cellX, yp + 0.25 * cellY, -EDIST);
            glm::vec3 dir3(xp + 0.75 * cellX, yp + 0.75 * cellY, -EDIST);
            glm::vec3 dir4(xp + 0.25 * cellX, yp + 0.75 * cellY, -EDIST);

            Ray ray1 = Ray(eye, dir1);
            Ray ray2 = Ray(eye, dir2);
            Ray ray3 = Ray(eye, dir3);
            Ray ray4 = Ray(eye, dir4);

            //Trace each sub-divided ray and get the colour value
            glm::vec3 col1 = trace(ray1, 1);
            glColor3f(col1.r, col1.g, col1.b);

            glm::vec3 col2 = trace(ray2, 1);
            glColor3f(col2.r, col2.g, col2.b);

            glm::vec3 col3 = trace(ray3, 1);
            glColor3f(col3.r, col3.g, col3.b);

            glm::vec3 col4 = trace(ray4, 1);
            glColor3f(col4.r, col4.g, col4.b);

            //get the average of the colour values
            float red = (col1.r + col2.r + col3.r + col4.r)/4;
            float green = (col1.g + col2.g + col3.g + col4.g)/4;
            float blue = (col1.b + col2.b + col3.b + col4.b)/4;
            glColor3f(red, green, blue);

            //Draw each cell -using the current col (colour) value
            glVertex2f(xp, yp);                 // grid point
            glVertex2f(xp + cellX, yp);         // x_value+cell width,y_value
            glVertex2f(xp + cellX, yp + cellY); // x_value+cell width, y_value+cell height
            glVertex2f(xp, yp + cellY);         // x_value, y_value+cell height

		}
    }

    glEnd();
    glFlush();
}



//---This function initializes the scene ------------------------------------------- 
//   Specifically, it creates scene objects (spheres, planes, cones, cylinders etc)
//     and add them to the list of scene objects.
//   It also initializes the OpenGL 2D orthographc projection matrix for drawing the
//     the ray traced//    sphere1->setSpecularity();
//----------------------------------------------------------------------------------
void initialize()
{
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(XMIN, XMAX, YMIN, YMAX);
    glClearColor(0, 0, 0, 1);
    texture = TextureBMP("latlon-normal-map.bmp");

    //Floor
    Plane *plane1 = new Plane
            (glm::vec3(-40., -39, 20), //Point A
             glm::vec3(40., -39, 20), //Point B
             glm::vec3(40., -39, -300), //Point C
             glm::vec3(-40., -39, -300)); //Point D
    sceneObjects.push_back(plane1);
    plane1->setSpecularity(false); //removing the specular reflection
    //colour is set in the chequered pattern bit

    //Ceiling
    Plane *plane2 = new Plane
            (glm::vec3(-40., 39, -300), //Point A
             glm::vec3(40., 39, -300), //Point B
             glm::vec3(40., 39, 20), //Point C
             glm::vec3(-40., 39, 20)); //Point D
    sceneObjects.push_back(plane2);
    plane2->setSpecularity(false); //removing the specular reflection
    //colour is set in the diamond pattern bit

    //Left
    Plane *plane3 = new Plane
            (glm::vec3(-40., -40, 20), //Point A
             glm::vec3(-40., -40, -300), //Point B
             glm::vec3(-40., 40, -300), //Point C
             glm::vec3(-40., 40, 20)); //Point D
    plane3->setColor(glm::vec3(0, 1, 1));
    sceneObjects.push_back(plane3);
    plane3->setSpecularity(false); //removing the specular reflection

    //Right
    Plane *plane4 = new Plane
            (glm::vec3(40., -40, -300), //Point A
             glm::vec3(40., -40, 20), //Point B
             glm::vec3(40., 40, 20), //Point C
             glm::vec3(40., 40, -300)); //Point D
    plane4->setColor(glm::vec3(1, 0, 1));
    sceneObjects.push_back(plane4);
    plane4->setSpecularity(false); //removing the specular reflection

    //Front
    Plane *plane5 = new Plane
            (glm::vec3(-40., -40, -300), //Point A
             glm::vec3(40., -40, -300), //Point B
             glm::vec3(40., 40, -300), //Point C
             glm::vec3(-40., 40, -300)); //Point D
    plane5->setColor(glm::vec3(1, 0, 0.8));
    sceneObjects.push_back(plane5);
    plane5->setSpecularity(false); //removing the specular reflection

    //Back -coords occur 'the other way around' bc the plane is behind the camera, so turn around rather tahn look through
    Plane *plane6 = new Plane
            (glm::vec3(40., -40, 20), //Point A
             glm::vec3(-40., -40, 20), //Point B
             glm::vec3(-40., 40, 20), //Point C
             glm::vec3(40., 40, 20)); //Point D
    plane6->setColor(glm::vec3(0, 1, 0));
    sceneObjects.push_back(plane6);
    plane6->setSpecularity(false); //removing the specular reflection


    //Mirror -Front wall
    Plane *plane7 = new Plane
            (glm::vec3(-30., -30, -280), //Point A
             glm::vec3(30., -30, -280), //Point B
             glm::vec3(30., 30, -280), //Point C
             glm::vec3(-30., 30, -280)); //Point D
    plane7->setColor(glm::vec3(0, 0, 0));
    sceneObjects.push_back(plane7);
    plane7->setSpecularity(false); //removing the specular reflection
    plane7->setReflectivity(true, 0.9); //second param is the coefficient of reflection, Pr

    //Mirror -Back wall
    Plane *plane8 = new Plane
            (glm::vec3(30., -30, 18), //Point A
             glm::vec3(-30., -30, 18), //Point B
             glm::vec3(-30., 30, 18), //Point C
             glm::vec3(30., 30, 18)); //Point D
    plane8->setColor(glm::vec3(0, 0, 0));
    sceneObjects.push_back(plane8);
    plane8->setSpecularity(false); //removing the specular reflection
    plane8->setReflectivity(true, 0.9); //second param is the coefficient of reflection, Pr


    //reflective
    Sphere *sphere1 = new Sphere(glm::vec3(-10.0, -10.0, -100.0), 8.0);
    sphere1->setColor(glm::vec3(0, 0, 0));   //Set colour to black
    sceneObjects.push_back(sphere1);		 //Add sphere to scene objects
    sphere1->setReflectivity(true, 0.8); //second param is the coefficient of reflection, Pr

    //transparent
    Sphere *sphere2 = new Sphere(glm::vec3(10.0, -10.0, -100.0), 8.0);
    sphere2->setColor(glm::vec3(0, 0, 1));   //Set colour to blue
    sceneObjects.push_back(sphere2);		 //Add sphere to scene objects
    sphere2->setTransparency(true, 0.7); //second param is the coefficient of transparency, Pr
    sphere2->setReflectivity(true, 0.4); //second param is the coefficient of reflection, Pr

    //refractive
    Sphere *sphere3 = new Sphere(glm::vec3(10.0, 10.0, -100.0), 8.0);
    sphere3->setColor(glm::vec3(0,0,0));   //Set colour to black
    sceneObjects.push_back(sphere3);		 //Add sphere to scene objects
    sphere3->setRefractivity(true, 0.8, 1.01);
    sphere3->setReflectivity(true, 0.05);

    //textured
    Sphere *sphere4 = new Sphere(glm::vec3(-10.0, 10.0, -100.0), 8.0);
    sphere4->setColor(glm::vec3(0,0,0));   //Set colour to black
    sceneObjects.push_back(sphere4);		 //Add sphere to scene objects
    sphere4->setSpecularity(false);

}


int main(int argc, char *argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB );
    glutInitWindowSize(500, 500);
    glutInitWindowPosition(20, 20);
    glutCreateWindow("Assignment 2");

    glutDisplayFunc(display);
    initialize();

    glutMainLoop();
    return 0;
}
