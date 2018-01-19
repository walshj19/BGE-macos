
#ifdef _WIN32
 #include <windows.h>
#endif 
#include <GL/glew.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <glm.hpp>
#include "Game.h"
#include "TriangleTest.h"
#include "TexturedCube.h"
#include "CubeTest.h"
#include "PhysicsGame1.h"
#include "ModelTest.h"
#include "SteeringGame.h"
#include "SceneGraphGame.h"
#include "Buddha.h"
#include "Game.h"
#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <stdlib.h>
#include <ctime>

#include "Lab4.h"
#include "Lab5.h"
#include "Lab3.h"
#include "Lab6.h"
#include "Lab7.h"
#include "TextRenderTest.h"

#include "Content.h"
#include "FPSController.h"
#include "XBoxController.h"
#include "Steerable3DController.h"
#include "Utils.h"
#include "SDL2/SDL_syswm.h"
#include "SceneGraphGame.h"
#include "GravityGame.h"
#include "CubeTest.h"

using namespace BGE;

int main(int argc, char *argv[])
{
    // http://www.youtube.com/watch?v=EEbVHxOkTxw
	Params::Load("default");
    
    shared_ptr<Game> game = make_shared<Lab3>();
    game->Run();

	return 0;
}
