// Jeremy Kimotho- 30096043
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <ctime>
#include <iostream>
#include <cmath>

#include "Geometry.h"
#include "GLDebug.h"
#include "Log.h"
#include "ShaderProgram.h"
#include "Shader.h"
#include "Window.h"


/*

Struct that represents points in 3d space. z-axis stays at 0 because we work exclusively in 2d in this project.

*/
struct Point 
{
    float positions[3] = {0.f, 0.f, 0.f};
};

/*

Struct useful in collaboration with our callbacks to switch between the different types and iterations. Types are the different shapes and they range from our first shape with type code 1, to our last type with type code 4. Iterations are the different versions of each particular type with range 0 to 6. With 0 being the base case.

*/
struct Display
{
    int type = 0;
    int iteration = 0;

    // Used to save as having to compare both type and iteration to resolve if statements
    bool operator!=(Display const &other) const
    {
        if(type!=other.type||iteration!=other.iteration) return true;
        return false;
    }

};

/*

Setting up of callbacks. These callbacks are used to switch between different types and different iterations of those types. The up and down keys will switch between types when pressed, and the left and right keys will switch between iterations when pressed.

*/
class MyCallbacks : public CallbackInterface
{

public:
    MyCallbacks(ShaderProgram &shader) : shader(shader) {}

    virtual void keyCallback(int key, int scancode, int action, int mods)
    {
        if (key == GLFW_KEY_R && action == GLFW_PRESS)
        {
            shader.recompile();
        }

        // If iteration is above 0 we can decrement otherwise do not because range floor is 0
        if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
        {
            if (display.iteration > 0)
                display.iteration--;
        }

        // If iteration is below 8 we can increment otherwise do not because range ceiling is 8
        if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
        {
            if (display.iteration < 8)
                display.iteration++;
        }

        // If type is above 1 we can decrement otherwise do not because range floor is 1
        if (key == GLFW_KEY_UP && action == GLFW_PRESS)
        {
            if (display.type > 1)
                display.type--;
        }

        // If iteration is below 4 we can increment otherwise do not because range ceiling is 4
        if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
        {
            if (display.type < 4)
                display.type++;
        }
    }

    Display getState()
    {
        return display;
    }

private:
    Display display;
    ShaderProgram &shader;
};

/*

Pushes on the std vector of the provided cpuGeom the point provided.

*/
void pushPoint(Point a, CPU_Geometry& cpuGeom)
{
    cpuGeom.verts.push_back(glm::vec3(a.positions[0], a.positions[1], a.positions[2]));
}

/*

Takes three random integers as input and those integers are used to produce different colours for the lines of the square and diamond. They're then pushed onto the provided cpuGeom std vector for colours.

*/
void pushCols(int a, int b, int c, CPU_Geometry& cpuGeom)
{
    a = a % RAND_MAX;
    b = b % RAND_MAX;
    c = c % RAND_MAX;

    cpuGeom.cols.push_back(glm::vec3((float)(a) / RAND_MAX, (float)(b) / RAND_MAX, (float)(c) / RAND_MAX));
}

/*

Recursively generates points for our serpinski triangles. Starts with points a, b, c which are in center of screen and splits that larger triangle into 3 smaller ones. Process continues depending on the number of iterations currently on display. After base case has been reached, where we're at smallest triangles for that particular iteration, we then begin to save those points in std vector of cpuGeom. So we build up from the bottom of the triangle, until all levels have been drawn. 

*/
CPU_Geometry generateSierpinski(Point a, Point b, Point c, int m, CPU_Geometry& cpuGeom)
{
    Point v0, v1, v2;
    int red_rand, green_rand, blue_rand, j;

    if (m > 0)
    {
        for (j = 0; j < 2; j++)
        {
            v0.positions[j] = (a.positions[j] + b.positions[j]) * 0.5;
        }
            
        for (j = 0; j < 2; j++)
        {
            v1.positions[j] = (a.positions[j] + c.positions[j]) * 0.5;
        }
            
        for (j = 0; j < 2; j++)
        {
            v2.positions[j] = (b.positions[j] + c.positions[j]) * 0.5;
        }
    
        generateSierpinski(a, v0, v1, m - 1, cpuGeom);
        generateSierpinski(v0, b, v2, m - 1, cpuGeom);
        generateSierpinski(v1, v2, c, m - 1, cpuGeom);

    }
    else
    {
        pushPoint(a, cpuGeom);
        pushPoint(b, cpuGeom);
        pushPoint(c, cpuGeom);
    }
    
    // Setting up colour for the triangles
    red_rand = rand();
    green_rand = rand();
    blue_rand = rand();

    pushCols(red_rand, green_rand, blue_rand, cpuGeom);
    pushCols(red_rand, green_rand, blue_rand, cpuGeom);
    pushCols(red_rand, green_rand, blue_rand, cpuGeom);

    return cpuGeom;
}

/*

Takes 3 points and a cpuGeom as input. Then pushes those 3 given points into the std vector of cpuGeom. This has now formed a sequence of 3 points which will be interpreted as a triangle by GL_TRIANGLES primitive in the draw code. Also sets the colours as a gradient.

*/
CPU_Geometry drawBaseTriangle(Point a, Point b, Point c, CPU_Geometry& cpuGeom)
{
    // vertices
    cpuGeom.verts.push_back(glm::vec3(a.positions[0], a.positions[1], a.positions[2]));
    cpuGeom.verts.push_back(glm::vec3(b.positions[0], b.positions[1], b.positions[2]));
    cpuGeom.verts.push_back(glm::vec3(c.positions[0], c.positions[1], c.positions[2]));

    cpuGeom.cols.push_back(glm::vec3(0.f, 1.f, 0.f));
    cpuGeom.cols.push_back(glm::vec3(1.f, 0.f, 0.f));
    cpuGeom.cols.push_back(glm::vec3(0.f, 0.f, 1.f));

    return cpuGeom;
}

/*

Takes thedisplay structure as input. If the display type is equal to the type code of Sierpinski triangles, which is 1, then it will move into the function. Once in the function, a determination is made on whether the iteration is at the base case of 0, where we'll draw a single solid triangle, or if it is larger than 0, where we'll move to the recursive function that discovers the points to draw. In either case, returns a cpuGeom with triangle vectors in its std vector. 

*/
CPU_Geometry createSierpinksi(Display display)
{
    CPU_Geometry cpuGeom;
    
    if (display.type == 1)
    {
        Point a, b, c;

        // Set base position of triangle points. These are centered in our display window
        a.positions[0] = -0.5f;
        a.positions[1] = -0.5f;

        b.positions[0] = 0.5f;
        b.positions[1] = -0.5f;

        c.positions[0] = 0.f;
        c.positions[1] = 0.5f;

        if (display.iteration == 0)
        {
            return drawBaseTriangle(a, b, c, cpuGeom);
        }
        else
        {
            return generateSierpinski(a, b, c, display.iteration, cpuGeom);
        }
    }
    else
    {
        return cpuGeom;
    }
}

/*

Recursively draws the square and diamond starting from the bottom or lowest level and moving upwards. Does so by using 8 provided points. The first 4 points abcd form the square and the next 4 efgh forms the diamond. To find the points on the level below the current one, we take midpoints of the appropriate lines. The lines of the square and diamond also change level by level and produce a color chaning effect. This is done by making use of rand to produce 3 integers and they're then processed in pushCols to produce the different colours.

*/
CPU_Geometry generateSquareDiamond(Point a, Point b, Point c, Point d, Point e, Point f, Point g, Point h, int m, CPU_Geometry& cpuGeom)
{
    Point v0,v1,v2,v3,v4,v5,v6,v7;
    int red_rand, green_rand, blue_rand;

    if(m > 1)
    {
        v0.positions[0] = (a.positions[0] + e.positions[0]) * 0.5;
        v0.positions[1] = (a.positions[1] + h.positions[1]) * 0.5;

        v1.positions[0] = (e.positions[0] + b.positions[0]) * 0.5;
        v1.positions[1] = (b.positions[1] + f.positions[1]) * 0.5;

        v2.positions[0] = (c.positions[0] + g.positions[0]) * 0.5;
        v2.positions[1] = (f.positions[1] + c.positions[1]) * 0.5;

        v3.positions[0] = (g.positions[0] + d.positions[0]) * 0.5;
        v3.positions[1] = (d.positions[1] + h.positions[1]) * 0.5;

        v4.positions[0] = (v0.positions[0] + v1.positions[0]) * 0.5;
        v4.positions[1] = v0.positions[1];

        v5.positions[0] = v1.positions[0];
        v5.positions[1] = (v1.positions[1] + v2.positions[1]) * 0.5;

        v6.positions[0] = (v2.positions[0] + v3.positions[0]) * 0.5;
        v6.positions[1] = v2.positions[1];

        v7.positions[0] = v3.positions[0];
        v7.positions[1] = (v3.positions[1] + v0.positions[1]) * 0.5;

        generateSquareDiamond(v0,v1,v2,v3,v4,v5,v6,v7,m-1,cpuGeom);

        // Diamond 

        // New colours
        red_rand = rand();
        green_rand = rand();
        blue_rand = rand();

        // v0 -> v1
        pushPoint(v0, cpuGeom);
        pushPoint(v1, cpuGeom);

        pushCols(red_rand, green_rand, blue_rand, cpuGeom);
        pushCols(red_rand, green_rand, blue_rand, cpuGeom);

        // v1 -> v2
        pushPoint(v1, cpuGeom);
        pushPoint(v2, cpuGeom);

        pushCols(red_rand, green_rand, blue_rand, cpuGeom);
        pushCols(red_rand, green_rand, blue_rand, cpuGeom);

        // v2 -> v3
        pushPoint(v2, cpuGeom);
        pushPoint(v3, cpuGeom);

        pushCols(red_rand, green_rand, blue_rand, cpuGeom);
        pushCols(red_rand, green_rand, blue_rand, cpuGeom);

        // v3 -> v0
        pushPoint(v3, cpuGeom);
        pushPoint(v0, cpuGeom);

        pushCols(red_rand, green_rand, blue_rand, cpuGeom);
        pushCols(red_rand, green_rand, blue_rand, cpuGeom);

        // New colours
        red_rand = rand();
        green_rand = rand();
        blue_rand = rand();

        // v4 -> v5
        pushPoint(v4, cpuGeom);
        pushPoint(v5, cpuGeom);

        pushCols(red_rand, green_rand, blue_rand, cpuGeom);
        pushCols(red_rand, green_rand, blue_rand, cpuGeom);

        // v5 -> v6
        pushPoint(v5, cpuGeom);
        pushPoint(v6, cpuGeom);

        pushCols(red_rand, green_rand, blue_rand, cpuGeom);
        pushCols(red_rand, green_rand, blue_rand, cpuGeom);

        // v6 -> v7
        pushPoint(v6, cpuGeom);
        pushPoint(v7, cpuGeom);

        pushCols(red_rand, green_rand, blue_rand, cpuGeom);
        pushCols(red_rand, green_rand, blue_rand, cpuGeom);

        // v7 -> v4
        pushPoint(v7, cpuGeom);
        pushPoint(v4, cpuGeom);

        pushCols(red_rand, green_rand, blue_rand, cpuGeom);
        pushCols(red_rand, green_rand, blue_rand, cpuGeom);

        // New colours
        red_rand = rand();
        green_rand = rand();
        blue_rand = rand();
        
        // Square

        // a -> b
        pushPoint(a, cpuGeom);
        pushPoint(b, cpuGeom);

        pushCols(red_rand, green_rand, blue_rand, cpuGeom);
        pushCols(red_rand, green_rand, blue_rand, cpuGeom);

        // b -> c
        pushPoint(b, cpuGeom);
        pushPoint(c, cpuGeom);

        pushCols(red_rand, green_rand, blue_rand, cpuGeom);
        pushCols(red_rand, green_rand, blue_rand, cpuGeom);

        // c -> d
        pushPoint(c, cpuGeom);
        pushPoint(d, cpuGeom);

        pushCols(red_rand, green_rand, blue_rand, cpuGeom);
        pushCols(red_rand, green_rand, blue_rand, cpuGeom);

        // d -> a
        pushPoint(d, cpuGeom);
        pushPoint(a, cpuGeom);

        pushCols(red_rand, green_rand, blue_rand, cpuGeom);
        pushCols(red_rand, green_rand, blue_rand, cpuGeom);

        // New colours
        red_rand = rand();
        green_rand = rand();
        blue_rand = rand();

        // e -> f
        pushPoint(e, cpuGeom);
        pushPoint(f, cpuGeom);

        pushCols(red_rand, green_rand, blue_rand, cpuGeom);
        pushCols(red_rand, green_rand, blue_rand, cpuGeom);

        // f -> g
        pushPoint(f, cpuGeom);
        pushPoint(g, cpuGeom);

        pushCols(red_rand, green_rand, blue_rand, cpuGeom);
        pushCols(red_rand, green_rand, blue_rand, cpuGeom);

        // g -> h
        pushPoint(g, cpuGeom);
        pushPoint(h, cpuGeom);

        pushCols(red_rand, green_rand, blue_rand, cpuGeom);
        pushCols(red_rand, green_rand, blue_rand, cpuGeom);

        // h -> e
        pushPoint(h, cpuGeom);
        pushPoint(e, cpuGeom);

        pushCols(red_rand, green_rand, blue_rand, cpuGeom);
        pushCols(red_rand, green_rand, blue_rand, cpuGeom);

    }

    return cpuGeom;
}

/*

Used to draw the base (or first) square. Uses the same process and system as square diamond.

*/
CPU_Geometry drawBaseSquare(Point a, Point b, Point c, Point d, Point e, Point f, Point g, Point h, CPU_Geometry& cpuGeom)
{
    
    int red_rand, green_rand, blue_rand;

    // New colours
    red_rand = rand();
    green_rand = rand();
    blue_rand = rand();

    // a -> b
    pushPoint(a, cpuGeom);
    pushPoint(b, cpuGeom);

    pushCols(red_rand, green_rand, blue_rand, cpuGeom);
    pushCols(red_rand, green_rand, blue_rand, cpuGeom);

    // b -> c
    pushPoint(b, cpuGeom);
    pushPoint(c, cpuGeom);

    pushCols(red_rand, green_rand, blue_rand, cpuGeom);
    pushCols(red_rand, green_rand, blue_rand, cpuGeom);

    // c -> d
    pushPoint(c, cpuGeom);
    pushPoint(d, cpuGeom);

    pushCols(red_rand, green_rand, blue_rand, cpuGeom);
    pushCols(red_rand, green_rand, blue_rand, cpuGeom);

    // d -> a
    pushPoint(d, cpuGeom);
    pushPoint(a, cpuGeom);

    pushCols(red_rand, green_rand, blue_rand, cpuGeom);
    pushCols(red_rand, green_rand, blue_rand, cpuGeom);

    // New colours
    red_rand = rand();
    green_rand = rand();
    blue_rand = rand();

    // e -> f
    pushPoint(e, cpuGeom);
    pushPoint(f, cpuGeom);

    pushCols(red_rand, green_rand, blue_rand, cpuGeom);
    pushCols(red_rand, green_rand, blue_rand, cpuGeom);

    // f -> g
    pushPoint(f, cpuGeom);
    pushPoint(g, cpuGeom);

    pushCols(red_rand, green_rand, blue_rand, cpuGeom);
    pushCols(red_rand, green_rand, blue_rand, cpuGeom);

    // g -> h
    pushPoint(g, cpuGeom);
    pushPoint(h, cpuGeom);

    pushCols(red_rand, green_rand, blue_rand, cpuGeom);
    pushCols(red_rand, green_rand, blue_rand, cpuGeom);

    // h -> e
    pushPoint(h, cpuGeom);
    pushPoint(e, cpuGeom);

    pushCols(red_rand, green_rand, blue_rand, cpuGeom);
    pushCols(red_rand, green_rand, blue_rand, cpuGeom);

    return cpuGeom;
}

/*

Generates the base positions from where we will draw further levels of the square. Only activates if the display type is set to 2 which is the type code for this shape, the square-diamond. Depending on the iteration will either use drawBaseSquare which is a non-recursive function that draws a single square diamond, however if the display iteration is higher we then use the recursive generateSquareDiamond to produce as many of them as requested.

*/
CPU_Geometry createSquare(Display display)
{

    CPU_Geometry cpuGeom;

    if (display.type == 2)
    {
        Point a, b, c, d, e, f, g, h;

        // Set base positions of square and diamond. These are centered in display window
        a.positions[0] = -0.75f;
        a.positions[1] = -0.75f;

        b.positions[0] = 0.75f;
        b.positions[1] = -0.75f;

        c.positions[0] = 0.75f;
        c.positions[1] = 0.75f;

        d.positions[0] = -0.75f;
        d.positions[1] = 0.75f;

        e.positions[0] = 0.f;
        e.positions[1] = -0.75f;

        f.positions[0] = 0.75f;
        f.positions[1] = 0.f;

        g.positions[0] = 0.f;
        g.positions[1] = 0.75f;

        h.positions[0] = -0.75f;
        h.positions[1] = 0.f;

        if (display.iteration == 0) return drawBaseSquare(a, b, c, d, e, f, g, h, cpuGeom);
        return generateSquareDiamond(a, b, c, d, e, f, g, h, display.iteration + 1, cpuGeom);
    }
    else
    {
        return cpuGeom;
    }
}

/*

Given a point a and  b, findC finds the point c that completes the  koch snowflake. So this point c is the one that sits perpendicular to the y-plane a and b are both in. Finds this point by rotating a vector by 60 degrees and multiplying it by the magnitude the intended vector should be. The magnitudes are stored in v0.

*/
Point findC(Point a, Point b)
{
    Point v0;
    v0.positions[0] = b.positions[0] - a.positions[0];
    v0.positions[1] = b.positions[1] - a.positions[1];

    Point c;
    c.positions[0] = a.positions[0] + ((v0.positions[0] * cos(glm::radians(60.f))) - (v0.positions[1] * sin(glm::radians(60.f))));
    c.positions[1] = a.positions[1] + ((v0.positions[0] * sin(glm::radians(60.f))) + (v0.positions[1] * cos(glm::radians(60.f))));

    return c;
}

/*

This function recursively generates Koch snowflakes by taking as input two points a and b that from a line, and from that creating 3 additional points and 3 additional lines that form the equilateral triangle and two lines flanking it. When the number of iterations m have been completed, the points are then pushed onto the std vector verts of cpuGeom by pushPoint. Colours are also set for the lines by pushing random colours onto the std vector cols of cpuGeom. We then return cpuGeom with all the points added. 

*/
CPU_Geometry generateKochSnowflakes(Point a, Point b, int m, CPU_Geometry &cpuGeom)
{
    Point v0, v1, v2;
    int red_rand, green_rand, blue_rand;

    if (m > 0)
    {
        v0.positions[0] = a.positions[0] + ((b.positions[0] - a.positions[0]) * (1.f / 3.f));
        v0.positions[1] = a.positions[1] + ((b.positions[1] - a.positions[1]) * (1.f / 3.f));

        v1.positions[0] = a.positions[0] + ((b.positions[0] - a.positions[0]) * (2.f / 3.f));
        v1.positions[1] = a.positions[1] + ((b.positions[1] - a.positions[1]) * (2.f / 3.f));

        v2 = findC(v0, v1);

        // The line a -> b, produces a -> v0, v0 -> v2, v2 -> v1,  v1 -> b

        generateKochSnowflakes(a, v0, m - 1, cpuGeom);
        generateKochSnowflakes(v0, v2, m - 1, cpuGeom);
        generateKochSnowflakes(v2, v1, m - 1, cpuGeom);
        generateKochSnowflakes(v1, b, m - 1, cpuGeom);
    }
    else
    {
        pushPoint(a, cpuGeom);
        pushPoint(b, cpuGeom);

        // Setting up colour for the lines
        red_rand = rand();
        green_rand = rand();
        blue_rand = rand();

        pushCols(red_rand, green_rand, blue_rand, cpuGeom);
        pushCols(red_rand, green_rand, blue_rand, cpuGeom);

    }

    return cpuGeom;
}

/*

Sets the base positions that form the initial equilateral triangle. We then run generateKochSnowflakes on the 3 lines that make up the triangle. 

*/
CPU_Geometry createSnowflakes(Display display)
{
    CPU_Geometry cpuGeom;

    if (display.type == 3)
    {
        Point a, b, c;

        a.positions[0] = -0.5f;
        a.positions[1] = -0.5f;

        c.positions[0] = 0.5f;
        c.positions[1] = -0.5f;

        b.positions[0] = 0.f;
        b.positions[1] = 0.3660254038f;

        cpuGeom = generateKochSnowflakes(a, b, display.iteration, cpuGeom);
        cpuGeom = generateKochSnowflakes(b, c, display.iteration, cpuGeom);
        cpuGeom = generateKochSnowflakes(c, a, display.iteration, cpuGeom);

        return cpuGeom;
    }
    else
    {
        return cpuGeom;
    }

}

Point findPointA(Point a, Point b)
{
    Point v0;
    v0.positions[0] = b.positions[0] - a.positions[0];
    v0.positions[1] = b.positions[1] - a.positions[1];

    Point c;
    c.positions[0] = v0.positions[0] * cos(glm::radians(45.f)) - v0.positions[1] * sin(glm::radians(45.f));
    c.positions[1] = v0.positions[0] * sin(glm::radians(45.f)) + v0.positions[1] * cos(glm::radians(45.f));

    c.positions[0] = c.positions[0] * (1 / sqrt(2));
    c.positions[1] = c.positions[1] * (1 / sqrt(2));

    c.positions[0] += a.positions[0];
    c.positions[1] += a.positions[1];

    return c;
}

Point findPointB(Point a, Point b)
{
    Point v0;
    v0.positions[0] = b.positions[0] - a.positions[0];
    v0.positions[1] = b.positions[1] - a.positions[1];

    Point c;
    c.positions[0] = v0.positions[0] * cos(glm::radians(-45.f)) - v0.positions[1] * sin(glm::radians(-45.f));
    c.positions[1] = v0.positions[0] * sin(glm::radians(-45.f)) + v0.positions[1] * cos(glm::radians(-45.f));

    c.positions[0] = c.positions[0] * (1 / sqrt(2));
    c.positions[1] = c.positions[1] * (1 / sqrt(2));

    // c.positions[0] += 1;

    c.positions[0] += a.positions[0];
    c.positions[1] += a.positions[1];

    return c;
}

CPU_Geometry generateCurve(Point a, Point b, int m, int n, CPU_Geometry& cpuGeom)
{
    Point v0;
    int red_rand, green_rand, blue_rand;

    if(m > 0)
    {

        if(n % 2 == 0)
        {
            v0 = findPointA(b, a);
        }
        else
        {
            v0 = findPointB(b, a);
        }

        generateCurve(a, v0, m - 1, n, cpuGeom);
        n += 1;
        generateCurve(v0, b, m - 1, n, cpuGeom);
    }
    else
    {
        pushPoint(a, cpuGeom);
        pushPoint(b, cpuGeom);

        // Setting up colour for the lines
        red_rand = rand();
        green_rand = rand();
        blue_rand = rand();

        pushCols(red_rand, green_rand, blue_rand, cpuGeom);
        pushCols(red_rand, green_rand, blue_rand, cpuGeom);

    }

    return cpuGeom;
}

CPU_Geometry createCurve(Display display)
{
    CPU_Geometry cpuGeom;

    if(display.type == 4)
    {
        Point a, b, c, d;

        a.positions[0] = -0.5f;
        a.positions[1] = 0.f;

        b.positions[0] = 0.5f;
        b.positions[1] = 0.f;

        return generateCurve(a, b, display.iteration, 2, cpuGeom);
    }
    else
    {
        return cpuGeom;
    }
    
}

int main() {
	Log::debug("Starting main");

	// WINDOW
	glfwInit();
	Window window(800, 800, "CPSC 453- Assignment 1"); // can set callbacks at construction if desired

	GLDebug::enable();

	// SHADERS
	ShaderProgram shader("shaders/test.vert", "shaders/test.frag");

    // CALLBACKS
    std::shared_ptr<MyCallbacks> callbacks = std::make_shared<MyCallbacks>(shader);
    window.setCallbacks(callbacks); // can also update callbacks to new ones

    // PARAMETER FOR CALLBACKS
    Display display;
    display.type = 1;
    display.iteration = 0;

    // SEEDING RAND 
    srand(time(NULL));

    // GEOMETRY
	CPU_Geometry cpuGeomTriangle;
	GPU_Geometry gpuGeomTriangle;
    
    CPU_Geometry cpuGeomSquare;
    GPU_Geometry gpuGeomSquare;

    CPU_Geometry cpuGeomSnow;
    GPU_Geometry gpuGeomSnow;

    CPU_Geometry cpuGeomCurve;
    GPU_Geometry gpuGeomCurve;

    // RENDER LOOP
	while (!window.shouldClose()) {

		glfwPollEvents();

        if (callbacks->getState() != display)
        {
            display = callbacks->getState();

            // Serpinski Triangle
            cpuGeomTriangle.verts.clear();
            cpuGeomTriangle.cols.clear();

            cpuGeomTriangle = createSierpinksi(display);

            gpuGeomTriangle.setVerts(cpuGeomTriangle.verts);
            gpuGeomTriangle.setCols(cpuGeomTriangle.cols);

            // Squares and Diamonds
            cpuGeomSquare.verts.clear();
            cpuGeomSquare.cols.clear();

            cpuGeomSquare = createSquare(display);

            gpuGeomSquare.setVerts(cpuGeomSquare.verts);
            gpuGeomSquare.setCols(cpuGeomSquare.cols);

            // Koch snowflake
            cpuGeomSnow.verts.clear();
            cpuGeomSnow.cols.clear();

            cpuGeomSnow = createSnowflakes(display);

            gpuGeomSnow.setVerts(cpuGeomSnow.verts);
            gpuGeomSnow.setCols(cpuGeomSnow.cols);

            // Dragon curve
            cpuGeomCurve.verts.clear();
            cpuGeomCurve.cols.clear();

            cpuGeomCurve = createCurve(display);

            gpuGeomCurve.setVerts(cpuGeomCurve.verts);
            gpuGeomCurve.setCols(cpuGeomCurve.cols);
            
        }

        shader.use();
		glEnable(GL_FRAMEBUFFER_SRGB);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Serpinksi binds and draws
        gpuGeomTriangle.bind();
        glDrawArrays(GL_TRIANGLES, 0, GLsizei(cpuGeomTriangle.verts.size()));

        // Squares and Diamonds binds and draws
        gpuGeomSquare.bind();
        glDrawArrays(GL_LINES, 0, GLsizei(cpuGeomSquare.verts.size()));

        // Koch snowflake binds and draws
        gpuGeomSnow.bind();
        glDrawArrays(GL_LINES, 0, GLsizei(cpuGeomSnow.verts.size()));

        // Koch snowflake binds and draws
        gpuGeomCurve.bind();
        glDrawArrays(GL_LINES, 0, GLsizei(cpuGeomCurve.verts.size()));

        glDisable(GL_FRAMEBUFFER_SRGB); 
		window.swapBuffers();
	}

	glfwTerminate();
	return 0;
}
