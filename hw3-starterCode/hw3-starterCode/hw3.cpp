/* **************************
 * CSCI 420
 * Assignment 3 Raytracer
 * Name: Jiaqi Zuo
 * *************************
*/

#ifdef WIN32
#include <windows.h>
#endif

#if defined(WIN32) || defined(linux)
#include <GL/gl.h>
#include <GL/glut.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#define strcasecmp _stricmp
#endif

#include <imageIO.h>
#include <glm/glm.hpp>
#include <vector>
#include <iostream>
#include <math.h> 

#define MAX_TRIANGLES 20000
#define MAX_SPHERES 100
#define MAX_LIGHTS 100

char* filename = NULL;

//different display modes
#define MODE_DISPLAY 1
#define MODE_JPEG 2

int mode = MODE_DISPLAY;

//you may want to make these smaller for debugging purposes
#define WIDTH 640
#define HEIGHT 480

//the field of view of camera
#define fov 60.0
#define PI 3.14159265

using namespace std;

unsigned char buffer[HEIGHT][WIDTH][3];

struct Vertex {
	double position[3];
	double color_diffuse[3];
	double color_specular[3];
	double normal[3];
	double shininess;
};

struct Triangle {
	Vertex v[3];
};

struct Sphere {
	double position[3];
	double color_diffuse[3];
	double color_specular[3];
	double shininess;
	double radius;
};

struct Light {
	double position[3];
	double color[3];
};

class Color {
public:
	double r, g, b;

	Color() : r(double(0.0)), g(double(0.0)), b(double(0.0)) {}
	Color(double rr, double gg, double bb) : r(rr), g(gg), b(bb) {}

	// addtion operator with clamping
	Color& operator += (const Color& c) {
		r += c.r;
		r = r > 1.0 ? 1.0 : r < 0.0 ? 0.0 : r;

		g += c.g;
		g = g > 1.0 ? 1.0 : g < 0.0 ? 0.0 : g;

		b += c.b;
		b = b > 1.0 ? 1.0 : b < 0.0 ? 0.0 : b;

		return *this;
	}
	Color& operator /= (const double& d) {
		r /= d; g /= d; b /= d;
		return *this;
	}
};

// reference: Scratchapixel
class Vec3
{
public:
	double x, y, z;

	Vec3() : x(double(0.0)), y(double(0.0)), z(double(0.0)) {}
	Vec3(double xx, double yy, double zz) : x(xx), y(yy), z(zz) {}

	Vec3 operator + (const Vec3& v) const
	{
		return Vec3(x + v.x, y + v.y, z + v.z);
	}
	Vec3 operator - (const Vec3& v) const
	{
		return Vec3(x - v.x, y - v.y, z - v.z);
	}
	Vec3 operator * (const double& r) const
	{
		return Vec3(x * r, y * r, z * r);
	}

	Vec3 operator * (const Vec3& v) const
	{
		return Vec3(x * v.x, y * v.y, z * v.z);
	}

	Vec3 operator - () const
	{
		return Vec3(-x, -y, -z);
	}

	Vec3 crossProduct(const Vec3& v) const
	{
		return Vec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
	}
	double dotProduct(const Vec3& v) const
	{
		return x * v.x + y * v.y + z * v.z;
	}
	double norm() const
	{
		return (x * x + y * y + z * z);
	}
	double length() const // length of the vector
	{
		return sqrt(norm());
	}
	Vec3& normalize()
	{
		double n = norm();
		n = sqrt(n);
		if (n > 0) {
			double factor = 1 / n;
			x *= factor, y *= factor, z *= factor;
		}

		return *this;
	}
};

class Ray {
public:
	Vec3 origin;
	Vec3 direction;

	Ray(Vec3& origin, Vec3& direction) : origin(origin), direction(direction) {}

	// check if ray intersects sphere
	bool intersectSphere(const Sphere& sphere, Vec3& intersection) {
		Vec3 center;
		center.x = sphere.position[0]; center.y = sphere.position[1];  center.z = sphere.position[2];
		Vec3 d = origin - center; // distance

		// delta = b^2 - 4c
		double b = 2 * direction.dotProduct(d);
		double c = d.dotProduct(d) - pow(sphere.radius, 2);
		double delta = pow(b, 2) - 4 * c;
		// no intersection
		if (delta < 0) return false;
		// calculate roots
		double t0, t1;
		t0 = (-b - sqrt(delta)) / 2;
		t1 = (-b + sqrt(delta)) / 2;
		// let t0 be the smaller one
		if (t0 >= 0 || t1 >= 0) {
			if (t0 >= 0 && t1 >= 0) t0 = min(t0, t1);
			else if (t0 < 0 && t1 >= 0) t0 = t1;
			intersection = origin + (direction * t0);
			return true;
		}
		return false;
	}
	// check if ray intersects triangle
	bool intersectTriangle(Triangle& triangle, Vec3& intersection) {
		Vec3 A, B, C;
		A.x = triangle.v[0].position[0]; A.y = triangle.v[0].position[1]; A.z = triangle.v[0].position[2];
		B.x = triangle.v[1].position[0]; B.y = triangle.v[1].position[1]; B.z = triangle.v[1].position[2];
		C.x = triangle.v[2].position[0]; C.y = triangle.v[2].position[1]; C.z = triangle.v[2].position[2];

		// compute plane's normal
		Vec3 AB = B - A; Vec3 AC = C - A;
		Vec3 N = ((B - A).crossProduct(C - A));
		double denom = N.dotProduct(N);

		double NdotDir = N.dotProduct(direction);
		if (abs(NdotDir) < DBL_EPSILON)  return false;  // they are parallel
		// compute d parameter
		double d = -(N.dotProduct(A));
		// compute t
		double t = -(N.dotProduct(origin) + d) / NdotDir;
		if (t < 0) return false; // the triangle is behind the ray
		// compute intersection point
		intersection = origin + (direction * t);
		// inside-outside test
		// reference: Scratchapixel: Ray Tracing: Rendering a Triangle
		Vec3 crossVal = (B - A).crossProduct(intersection - A);
		if (crossVal.dotProduct(N) < 0) return false; // intersection on the right side
		crossVal = (C - B).crossProduct(intersection - B);
		if (crossVal.dotProduct(N) < 0) return false; // intersection on the right side
		crossVal = (A - C).crossProduct(intersection - C);
		if (crossVal.dotProduct(N) < 0) return false; // intersection on the right side

		return true;
	}
};

Triangle triangles[MAX_TRIANGLES];
Sphere spheres[MAX_SPHERES];
Light lights[MAX_LIGHTS];
double ambient_light[3];

int num_triangles = 0;
int num_spheres = 0;
int num_lights = 0;

bool antiAliasing = false;
bool softShadows = false;
bool reflection = false;
int iterations = 0;
vector<Light> light_points;
Color generateColor(Ray& ray, int iteration);

void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b);
void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b);
void plot_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b);

// Phong Shading for sphere
Color spherePhongShading(Sphere& sphere, Vec3& rayDirection, Light& light, Vec3& intersection, int iteration) {
	Vec3 objPos, lightPos;
	objPos.x = sphere.position[0]; objPos.y = sphere.position[1]; objPos.z = sphere.position[2];
	lightPos.x = light.position[0]; lightPos.y = light.position[1]; lightPos.z = light.position[2];

	Vec3 normal = (intersection - objPos).normalize();
	Vec3 lightDir = (lightPos - intersection).normalize();
	double LdotN = lightDir.dotProduct(normal);
	LdotN = LdotN > 1.0 ? 1.0 : LdotN < 0.0 ? 0.0 : LdotN; // clmap result between [0,1]

	Vec3 reflectRay = (normal * LdotN * 2.0 - lightDir);
	reflectRay.normalize();
	Vec3 V = (-intersection).normalize();
	double RdotV = reflectRay.dotProduct(V);
	RdotV = RdotV > 1.0 ? 1.0 : RdotV < 0.0 ? 0.0 : RdotV; // clmap result between [0,1]

	// I = lightColor * (kd * (L dot N) + ks * (R dot V) ^ sh)
	Color currColor;
	currColor.r = light.color[0] * (sphere.color_diffuse[0] * LdotN + (sphere.color_specular[0] * pow(RdotV, sphere.shininess)));
	currColor.g = light.color[1] * (sphere.color_diffuse[1] * LdotN + (sphere.color_specular[1] * pow(RdotV, sphere.shininess)));
	currColor.b = light.color[2] * (sphere.color_diffuse[2] * LdotN + (sphere.color_specular[2] * pow(RdotV, sphere.shininess)));

	// recursive reflection on sphere
	// reflection angle = 2 (l•n)n – l
	if (iteration < iterations) {
		Color finalColor;
		Vec3 tempDir = Vec3(rayDirection.x, rayDirection.y, rayDirection.z);
		// For incoming/outgoing ray negate l 
		tempDir = -tempDir;
		double lightDotNor = tempDir.dotProduct(normal);
		double reflectionAngle = lightDotNor * (double)2.0;
		Vec3 angle = normal * reflectionAngle;
		angle = angle - tempDir;
		// apply offset to intersection to avoid z-buffering fighting
		Ray reflectRay = Ray(intersection + angle * 0.0001, angle);
		// calculte the color of reflection ray
		Color reflectColor = generateColor(reflectRay, iteration + 1);
		if (softShadows) reflectColor /= (double)15; // reducing the wight of reflection color
		finalColor += reflectColor;
		// (1 - ks) * localPhongColor + ks * colorOfReflectedRay
		// reducing ks
		finalColor.r = (1 - sphere.color_specular[0]) * currColor.r + sphere.color_specular[0] * 0.1 * finalColor.r;
		finalColor.g = (1 - sphere.color_specular[1]) * currColor.g + sphere.color_specular[1] * 0.1 * finalColor.g;
		finalColor.b = (1 - sphere.color_specular[2]) * currColor.b + sphere.color_specular[2] * 0.1 * finalColor.b;
		return finalColor;
	}

	return currColor;
}

// Phong shading for triangle
Color trianglePhongShading(Triangle& triangle, Vec3& rayDirection, Light& light, Vec3& intersection, int iteration) {
	Vec3 A, B, C;
	A.x = triangle.v[0].position[0]; A.y = triangle.v[0].position[1]; A.z = triangle.v[0].position[2];
	B.x = triangle.v[1].position[0]; B.y = triangle.v[1].position[1]; B.z = triangle.v[1].position[2];
	C.x = triangle.v[2].position[0]; C.y = triangle.v[2].position[1]; C.z = triangle.v[2].position[2];

	// calculating the area of triangle
	// 1/2 * (noraml of triangle)
	Vec3 triABC = (B - A).crossProduct(C - A);
	Vec3 triBCD = (C - B).crossProduct(intersection - B);
	Vec3 triCAD = (A - C).crossProduct(intersection - C);
	double areaABC = 0.5 * triABC.length();
	double areaBCI = 0.5 * triBCD.length();
	double areaCAI = 0.5 * triCAD.length();

	double alpha = areaBCI / areaABC;
	double beta = areaCAI / areaABC;
	double gamma = 1.0 - alpha - beta;

	// interpolate normal
	Vec3 nA, nB, nC;
	nA.x = triangle.v[0].normal[0]; nA.y = triangle.v[0].normal[1]; nA.z = triangle.v[0].normal[2];
	nB.x = triangle.v[1].normal[0]; nB.y = triangle.v[1].normal[1]; nB.z = triangle.v[1].normal[2];
	nC.x = triangle.v[2].normal[0]; nC.y = triangle.v[2].normal[1]; nC.z = triangle.v[2].normal[2];
	Vec3 normal = nA * alpha + nB * beta + nC * gamma;
	normal.normalize();

	// interpolate kd
	Color kd;
	kd.r = alpha * triangle.v[0].color_diffuse[0] + beta * triangle.v[1].color_diffuse[0] + gamma * triangle.v[2].color_diffuse[0];
	kd.g = alpha * triangle.v[0].color_diffuse[1] + beta * triangle.v[1].color_diffuse[1] + gamma * triangle.v[2].color_diffuse[1];
	kd.b = alpha * triangle.v[0].color_diffuse[2] + beta * triangle.v[1].color_diffuse[2] + gamma * triangle.v[2].color_diffuse[2];

	// interpolate ks
	Color ks;
	ks.r = alpha * triangle.v[0].color_specular[0] + beta * triangle.v[1].color_specular[0] + gamma * triangle.v[2].color_specular[0];
	ks.g = alpha * triangle.v[0].color_specular[1] + beta * triangle.v[1].color_specular[1] + gamma * triangle.v[2].color_specular[1];
	ks.b = alpha * triangle.v[0].color_specular[2] + beta * triangle.v[1].color_specular[2] + gamma * triangle.v[2].color_specular[2];

	Vec3 lightPos;
	lightPos.x = light.position[0]; lightPos.y = light.position[1]; lightPos.z = light.position[2];
	Vec3 lightDir = (lightPos - intersection).normalize();
	double LdotN = lightDir.dotProduct(normal);
	LdotN = LdotN > 1.0 ? 1.0 : LdotN < 0.0 ? 0.0 : LdotN; // clamp the result between [0,1]

	Vec3 reflectRay = (normal * LdotN * 2.0 - lightDir);
	reflectRay.normalize();
	Vec3 V = (-intersection).normalize();
	double RdotV = reflectRay.dotProduct(V);
	RdotV = RdotV > 1.0 ? 1.0 : RdotV < 0.0 ? 0.0 : RdotV; // clamp the result between [0,1]

	double sAlpha = alpha * triangle.v[0].shininess + beta * triangle.v[1].shininess + gamma * triangle.v[2].shininess;

	// I = lightColor * (kd * (L dot N) + ks * (R dot V) ^ sh)  
	Color currColor;
	currColor.r = light.color[0] * (kd.r * LdotN + (ks.r * pow(RdotV, sAlpha)));
	currColor.g = light.color[1] * (kd.g * LdotN + (ks.g * pow(RdotV, sAlpha)));
	currColor.b = light.color[2] * (kd.b * LdotN + (ks.b * pow(RdotV, sAlpha)));

	// recursive reflection on triangle
	// reflection angle = 2 (l•n)n – l
	if (iteration < iterations) {
		Color finalColor;
		Vec3 tempDir = Vec3(rayDirection.x, rayDirection.y, rayDirection.z);
		tempDir = -tempDir;
		double lightDotNor = tempDir.dotProduct(normal);
		double reflectionAngle = lightDotNor * (double)2.0;
		Vec3 angle = normal * reflectionAngle;
		angle = angle - tempDir;
		// apply offset to intersection to avoid z-buffering fighting
		Ray reflectRay = Ray(intersection + angle * 0.0001, angle);
		// calculte the color of reflection ray
		Color reflectColor = generateColor(reflectRay, iteration + 1);
		if (softShadows) reflectColor /= (double)15; // reducing the weight of reflection color
		finalColor += reflectColor;
		// (1 - ks) * localPhongColor + ks * colorOfReflectedRay
		// reducing the ks
		finalColor.r = (1 - ks.r) * currColor.r + ks.r * 0.1 * finalColor.r;
		finalColor.g = (1 - ks.g) * currColor.g + ks.g * 0.1 * finalColor.g;
		finalColor.b = (1 - ks.b) * currColor.b + ks.b * 0.1 * finalColor.b;
		return finalColor;
	}
	return currColor;
}

// detect sphere intersections 
void detectSphereShadow(Ray& ray, Color& pixelColor, double& maxDist, int iteration) {
	// iterating through all spheres
	for (int i = 0; i < num_spheres; i++) {
		// initiliazing the intersection
		Vec3 intersection = Vec3(0, 0, maxDist);
		// check if ray hits the object
		if (ray.intersectSphere(spheres[i], intersection) && intersection.z > maxDist) {
			pixelColor = Color(0.0, 0.0, 0.0); // set default to black
			// ray casting from light source
			for (int j = 0; j < light_points.size(); j++) {
				bool inShadow = false; // set default to false
				Vec3 lightPos = Vec3(light_points[j].position[0], light_points[j].position[1], light_points[j].position[2]);
				Vec3 rayDir = (lightPos - intersection).normalize();
				Ray shadowRay(intersection, rayDir);
				// check if shadow ray intersects sphere
				for (int k = 0; k < num_spheres; k++) {
					Vec3 shadowIntersect = Vec3(0, 0, 0);
					if (shadowRay.intersectSphere(spheres[k], shadowIntersect) && i != k) {
						Vec3 temp = shadowIntersect - intersection;
						double toSphereDist = temp.length(); // distance from shadow to shpere
						temp = lightPos - intersection;
						double toLightDist = temp.length(); // distance from shadow to light
						if (toSphereDist < toLightDist) {
							inShadow = true;
							break;
						}
					}
				}
				// check if shadow ray intersects triangle
				for (int k = 0; k < num_triangles; k++) {
					Vec3 shadowIntersect = Vec3(0, 0, 0);
					if (shadowRay.intersectTriangle(triangles[k], shadowIntersect)) {
						Vec3 temp1 = shadowIntersect - intersection;
						double toTriangleDist = temp1.length(); // distance from shadow to triangle
						temp1 = lightPos - intersection;
						double toLightDist = temp1.length(); // distance from shadow to light
						if (toTriangleDist < toLightDist) {
							inShadow = true;
							break;
						}
					}
				}
				// applying Phong shading if it is not in the shadow
				if (inShadow == false) {
					pixelColor += spherePhongShading(spheres[i], ray.direction, light_points[j], intersection, iteration);
				}
			}
			// update the maximun distance
			maxDist = intersection.z;
		}
	}

}

// detect triangle intersections
void detectTriangleShadow(Ray& ray, Color& pixelColor, double& maxDist, int iteration) {
	// iterate through all tirangles 
	for (int i = 0; i < num_triangles; i++) {
		// initiliazing the intersection
		Vec3 intersection = Vec3(0, 0, maxDist);
		// check if ray hits the object
		if (ray.intersectTriangle(triangles[i], intersection) && intersection.z > maxDist) {
			pixelColor = Color(0.0, 0.0, 0.0); // set default to black
			// ray casting from light source
			for (int l = 0; l < light_points.size(); l++) {
				bool inShadow = false; // set default to false
				Vec3 lightPos = Vec3(light_points[l].position[0], light_points[l].position[1], light_points[l].position[2]);
				Vec3 rayDir = (lightPos - intersection).normalize();
				Ray shadowRay(intersection, rayDir);
				// Check Every Shadow of Spheres
				for (int k = 0; k < num_spheres; k++) {
					Vec3 shadowIntersect = Vec3(0, 0, 0);
					if (shadowRay.intersectSphere(spheres[k], shadowIntersect)) {
						Vec3 temp = shadowIntersect - intersection;
						double toSphereDist = temp.length(); // distance from shadow to shpere
						temp = lightPos - intersection;
						double toLightDist = temp.length(); // distance from shadow to light
						if (toSphereDist < toLightDist) {
							inShadow = true;
							break;
						}
					}
				}
				// Check Every Shadow of Other Triangles
				for (int k = 0; k < num_triangles; k++) {
					Vec3 shadowIntersect = Vec3(0, 0, 0);
					if (shadowRay.intersectTriangle(triangles[k], shadowIntersect) && i != k) {
						Vec3 temp1 = shadowIntersect - intersection;
						double toTriangleDist = temp1.length(); // distance from shadow to triangle
						temp1 = lightPos - intersection;
						double toLightDist = temp1.length(); // distance from shadow to light
						if (toTriangleDist < toLightDist) {
							inShadow = true;
							break;
						}
					}
				}
				// applying Phong shading if it is not in the shadow
				if (inShadow == false) {
					pixelColor += trianglePhongShading(triangles[i], ray.direction, light_points[l], intersection, iteration);
				}
			}
			// update the maximun distance
			maxDist = intersection.z;
		}
	}
}

// reference: https://mathworld.wolfram.com/SpherePointPicking.html
void generateLight(Light lights[]) {
	for (int i = 0; i < num_lights; i++) {
		int numSpots = 20;
		for (int j = 0; j < numSpots; j++) {
			// generate random numbers between [0,1]
			double r1 = ((double)rand() / (double)RAND_MAX);
			double r2 = ((double)rand() / (double)RAND_MAX);
			double r3 = ((double)rand() / (double)RAND_MAX);
			double theta = r1 * 2.0 * 180.0 / PI;
			double phi = acos(r2 * 2 - 1.0) * 180.0 / PI;
			r3 = cbrt(r3);
			Vec3 lightDir;
			lightDir.x = r3 * sin(phi) * cos(theta);
			lightDir.y = r3 * sin(phi) * sin(theta);
			lightDir.z = r3 * cos(phi);
			lightDir = lightDir * 0.1;
			Light currLight;
			currLight.position[0] = lights[i].position[0] + lightDir.x;
			currLight.position[1] = lights[i].position[1] + lightDir.y;
			currLight.position[2] = lights[i].position[2] + lightDir.z;
			for (int indx = 0; indx < 3; indx++) {
				currLight.color[indx] = lights[i].color[indx] / numSpots;
			}
			light_points.push_back(currLight);
		}
	}
}

// generate colors
Color generateColor(Ray& ray, int iteration) {
	Color rayColor(1.0, 1.0, 1.0); // initiate color to white

	double maxDist = -1e8; // in negative z direction

	// detect all the intersects for spheres and triangles
	detectSphereShadow(ray, rayColor, maxDist, iteration);
	detectTriangleShadow(ray, rayColor, maxDist, iteration);

	return rayColor;
}

// generate ray from camera
Ray generateRay(double x, double y) {
	double a = (double)WIDTH / HEIGHT;
	double tanFov = tan((fov / 2.0) * PI / 180.0);

	Vec3 origin, direction; // camera position origin in (0,0,0)
	direction.x = (2.0 * (x + 0.5) / (double)WIDTH - 1) * a * tanFov;
	direction.y = (2.0 * (y + 0.5) / (double)HEIGHT - 1) * tanFov;
	direction.z = -1;

	direction.normalize();

	return Ray(origin, direction);
}

//MODIFY THIS FUNCTION
void draw_scene() {
	double offSet = 0.25; // offset for anti-aliasing
	if (softShadows == false) {
		for (int i = 0; i < num_lights; i++) light_points.push_back(lights[i]);
	}
	else generateLight(lights);
	vector<Color> colors;
	for (unsigned int x = 0; x < WIDTH; x++) {
		glPointSize(2.0);
		glBegin(GL_POINTS);
		for (unsigned int y = 0; y < HEIGHT; y++) {
			Color color(0, 0, 0);
			if (antiAliasing) {
				// apply anti-aliasing 
				// addjust the coordinates
				Ray ray1 = generateRay(x - offSet, y - offSet);
				Color color1 = generateColor(ray1, 0); colors.push_back(color1);
				Ray ray2 = generateRay(x - offSet, y + offSet);
				Color color2 = generateColor(ray2, 0); colors.push_back(color2);
				Ray ray3 = generateRay(x + offSet, y - offSet);
				Color color3 = generateColor(ray3, 0); colors.push_back(color3);
				Ray ray4 = generateRay(x + offSet, y + offSet);
				Color color4 = generateColor(ray4, 0); colors.push_back(color4);
				for (int i = 0; i < 4; i++) {
					color.r += colors[i].r;
					color.g += colors[i].g;
					color.b += colors[i].b;
				}
				colors.clear();
				// average colors
				color.r /= 4; color.g /= 4; color.b /= 4;
			}
			else {
				// no anti-aliasing applied
				Ray ray = generateRay(x, y);
				color = generateColor(ray, 0);

			}
			Color ka = Color(ambient_light[0], ambient_light[1], ambient_light[2]);
			color += ka;
			plot_pixel(x, y, color.r * 255, color.g * 255, color.b * 255);

		}
		glEnd();
		glFlush();
	}
	if (antiAliasing) std::cout << "anti-Aliasing implemented" << std::endl;
	if (softShadows) std::cout << "soft shodows implemented" << std::endl;
	if (reflection) std::cout << "recursive reflection implemented" << std::endl;
	printf("Done!\n");
	std::fflush(stdout);
}

void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
	glColor3f(((float)r) / 255.0f, ((float)g) / 255.0f, ((float)b) / 255.0f);
	glVertex2i(x, y);
}

void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
	buffer[y][x][0] = r;
	buffer[y][x][1] = g;
	buffer[y][x][2] = b;
}

void plot_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
	plot_pixel_display(x, y, r, g, b);
	if (mode == MODE_JPEG)
		plot_pixel_jpeg(x, y, r, g, b);
}

void save_jpg() {
	printf("Saving JPEG file: %s\n", filename);

	ImageIO img(WIDTH, HEIGHT, 3, &buffer[0][0][0]);
	if (img.save(filename, ImageIO::FORMAT_JPEG) != ImageIO::OK)
		printf("Error in Saving\n");
	else
		printf("File saved Successfully\n");
}

void parse_check(const char* expected, char* found) {
	if (strcasecmp(expected, found)) {
		printf("Expected '%s ' found '%s '\n", expected, found);
		printf("Parse error, abnormal abortion\n");
		exit(0);
	}
}

void parse_doubles(FILE* file, const char* check, double p[3]) {
	char str[100];
	fscanf(file, "%s", str);
	parse_check(check, str);
	fscanf(file, "%lf %lf %lf", &p[0], &p[1], &p[2]);
	printf("%s %lf %lf %lf\n", check, p[0], p[1], p[2]);
}

void parse_rad(FILE* file, double* r) {
	char str[100];
	fscanf(file, "%s", str);
	parse_check("rad:", str);
	fscanf(file, "%lf", r);
	printf("rad: %f\n", *r);
}

void parse_shi(FILE* file, double* shi) {
	char s[100];
	fscanf(file, "%s", s);
	parse_check("shi:", s);
	fscanf(file, "%lf", shi);
	printf("shi: %f\n", *shi);
}

int loadScene(char* argv) {
	FILE* file = fopen(argv, "r");
	int number_of_objects;
	char type[50];
	Triangle t;
	Sphere s;
	Light l;
	fscanf(file, "%i", &number_of_objects);

	printf("number of objects: %i\n", number_of_objects);

	parse_doubles(file, "amb:", ambient_light);

	for (int i = 0; i < number_of_objects; i++) {
		fscanf(file, "%s\n", type);
		printf("%s\n", type);
		if (strcasecmp(type, "triangle") == 0) {
			printf("found triangle\n");
			for (int j = 0; j < 3; j++) {
				parse_doubles(file, "pos:", t.v[j].position);
				parse_doubles(file, "nor:", t.v[j].normal);
				parse_doubles(file, "dif:", t.v[j].color_diffuse);
				parse_doubles(file, "spe:", t.v[j].color_specular);
				parse_shi(file, &t.v[j].shininess);
			}

			if (num_triangles == MAX_TRIANGLES) {
				printf("too many triangles, you should increase MAX_TRIANGLES!\n");
				exit(0);
			}
			triangles[num_triangles++] = t;
		}
		else if (strcasecmp(type, "sphere") == 0) {
			printf("found sphere\n");

			parse_doubles(file, "pos:", s.position);
			parse_rad(file, &s.radius);
			parse_doubles(file, "dif:", s.color_diffuse);
			parse_doubles(file, "spe:", s.color_specular);
			parse_shi(file, &s.shininess);

			if (num_spheres == MAX_SPHERES) {
				printf("too many spheres, you should increase MAX_SPHERES!\n");
				exit(0);
			}
			spheres[num_spheres++] = s;
		}
		else if (strcasecmp(type, "light") == 0) {
			printf("found light\n");
			parse_doubles(file, "pos:", l.position);
			parse_doubles(file, "col:", l.color);

			if (num_lights == MAX_LIGHTS) {
				printf("too many lights, you should increase MAX_LIGHTS!\n");
				exit(0);
			}
			lights[num_lights++] = l;
		}
		else {
			printf("unknown type in scene description:\n%s\n", type);
			exit(0);
		}
	}
	return 0;
}

void display() {
}

void init() {
	glMatrixMode(GL_PROJECTION);
	glOrtho(0, WIDTH, 0, HEIGHT, 1, -1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
}

void idle() {
	//hack to make it only draw once
	static int once = 0;
	if (!once)
	{
		draw_scene();
		if (mode == MODE_JPEG)
			save_jpg();
	}
	once = 1;
}

int main(int argc, char** argv) {
	if ((argc < 2) || (argc > 6)) {
		printf("Usage: %s <input scenefile> [output jpegname]\n", argv[0]);
		exit(0);
	}

	if (argc >= 3) {
		mode = MODE_JPEG;
		filename = argv[2];
	}
	else if (argc == 2) {
		mode = MODE_DISPLAY;
	}
	if (argc >= 4) {
		if (tolower(*argv[3]) == 'y') {
			antiAliasing = true;
			std::cout << "anti-Alising implemented" << std::endl;
		}
		else antiAliasing = false;
	}
	if (argc >= 5) {
		if (tolower(*argv[4]) == 'y') {
			softShadows = true;
			std::cout << "soft shadows implemented" << std::endl;
		}
		else softShadows = false;
	}
	if (argc == 6) {
		if (tolower(*argv[5]) == 'y') {
			reflection = true;
			iterations = 3;
			std::cout << "recursive reflection implemented" << std::endl;
		}
		else iterations = 0;
	}

	glutInit(&argc, argv);
	loadScene(argv[1]);
	glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(WIDTH, HEIGHT);
	int window = glutCreateWindow("Ray Tracer");
#ifdef __APPLE__
	// This is needed on recent Mac OS X versions to correctly display the window.
	glutReshapeWindow(WIDTH - 1, HEIGHT - 1);
#endif
	glutDisplayFunc(display);
	glutIdleFunc(idle);
	init();
	glutMainLoop();
}
