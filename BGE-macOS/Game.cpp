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

#include "Content.h"
#include "FPSController.h"
#include "XBoxController.h"
#include "Steerable3DController.h"
#include "Utils.h"
#include "SDL2/SDL_syswm.h"
#include "Params.h"

using namespace BGE;

shared_ptr<Game> Game::instance = nullptr;

Game::Game(void):GameComponent(true) {
	running = false;	
	// Rift
	window = nullptr;	
	srand(time(0));
	elapsed = 10000.0f;

	lastPrintPosition = glm::vec2(0,0);
	fontSize = 14;	
	frame = 0;

	fps = 0;

	broadphase = nullptr;
	dispatcher = nullptr;
	solver = nullptr;

	tag = "Game";	
}

Game::~Game(void) 
{
	SAFE_DELETE(collisionConfiguration);
	SAFE_DELETE(dispatcher);
	SAFE_DELETE(broadphase);
	SAFE_DELETE(solver);
	SAFE_DELETE(dynamicsWorld);
}

shared_ptr<Ground> Game::GetGround()
{
	return ground;
}

void Game::PrintText(string message, glm::vec2 position)
{
	messages.push_back(PrintMessage(message, position));
}

void Game::PrintVector(string message, glm::vec3 v)
{
	stringstream ss;
	ss << message << "(" << v.x << ", " << v.y << ", " << v.z << ")";
	PrintText(ss.str());
}

void Game::PrintQuat(string message, glm::quat q)
{
	stringstream ss;
	ss << message << "(" << q.x << ", " << q.y << ", " << q.z << ", " << q.w << ")";
	PrintText(ss.str());
}

void Game::PrintFloat(string message, float f)
{
	stringstream ss;
	ss << message << f;
	PrintText(ss.str());
}

void Game::PrintText(string message)
{
	if (!Params::GetBool("riftEnabled"))
	{
		messages.push_back(PrintMessage(message, lastPrintPosition));
		lastPrintPosition.y += 20;
	}
}

bool Game::Run() 
{	
	if (!PreInitialise())
	{
		return false;
	}
	if (!Initialise())
	{
		return false;
	}

	long last = SDL_GetTicks();
	while (running) {
		long now = SDL_GetTicks();
		float ellapsed = ((float)(now - last)) / 1000.0f;
		Time::deltaTime = ellapsed;
		Update();
		PreDraw();
		Draw();
		frame++;
		PostDraw();
		last = now;
	}
	Cleanup();

	return 0;
}

void Game::SetGround(shared_ptr<Ground> ground)
{
	children.remove(this->ground);
	this->ground = ground;
	Attach(ground);
}

void Game::Update() {
	// Check for messages
	fps = 1.0f / Time::deltaTime;
	PrintText("FPS: " + to_string((long long) fps));
	soundSystem->Update();

	static bool lastPressed = false;
	if (keyState[SDL_SCANCODE_I])
	{
		if (! lastPressed)
		{
			Params::SetBool("hud", !Params::GetBool("hud"));
		}
		lastPressed = true;
	}
	else
	{
		lastPressed = false;
	}

	SDL_Event event;
	if (SDL_PollEvent(&event))
	{
		// Check for the quit message
		if (event.type == SDL_QUIT)
		{
			// Quit the program
			Cleanup();
			exit(0);
		}
	}

	if (keyState[SDL_SCANCODE_ESCAPE])
	{
		Cleanup();
		exit(0);
	}

	dynamicsWorld->stepSimulation(Time::deltaTime, 100);

	GameComponent::Update();
}

void Game::PreDraw()
{
	glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	
	if (Params::Get("culling") == "none")
	{
		glDisable(GL_CULL_FACE);
	}
	else
	{
		glEnable(GL_CULL_FACE);
	}
	if (Params::Get("culling") == "back")
	{		
		glCullFace(GL_BACK);
	}
	if (Params::Get("culling") == "front")
	{
		glCullFace(GL_FRONT);
	}

	GameComponent::PreDraw();
}

void Game::PrintAll()
{
	// Printing has to be done last, so we batch up the print messages
	if (Params::GetBool("hud"))
	{
		vector<PrintMessage>::iterator it = messages.begin();
		while (it != messages.end())
		{
            Print(it->message, it->position);
            //SimplePrint(it->message, it->position);
			it++;
		}
	}
	messages.clear();
	lastPrintPosition.y = 0;
}

void Game::PostDraw()
{		
	PrintAll();
    SDL_GL_SwapWindow(window);
	GameComponent::PostDraw();
}

void Game::Cleanup () {
	GameComponent::Cleanup();
	
	SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();	
}

shared_ptr<Game> Game::Instance() {
	return instance;
}

const Uint8 * Game::GetKeyState()
{
	return keyState;
}

SDL_Window * Game::GetMainWindow()
{
	return window;
}

void Game::Draw()
{
    PrintText("Press I to toggle info");
    glViewport(0, 0, Params::GetFloat("width"), Params::GetFloat("height"));
    LineDrawer::Instance()->Draw();
    GameComponent::Draw();
}

void Game::Print(string message, glm::vec2 position)
{
    std::vector<glm::vec2> vertices;
    std::vector<glm::vec2> uvs;

    SDL_Color foregroundColor = { 255, 255, 255 };
    SDL_Color backgroundColor = { 0, 0, 0 , 0};
    SDL_Rect TextLocation;
    GLuint texture;
    GLint  colors;
    GLenum texture_format;
    SDL_Surface * surface = TTF_RenderText_Blended(font, message.c_str(), foregroundColor);
    colors = surface->format->BytesPerPixel;
    if (colors == 4) {
        if (surface->format->Rmask == 0x000000ff) {
            texture_format = GL_RGBA;
        }
        else {
            texture_format = GL_BGRA;
        }
    } else if (colors == 3) {
        if (surface->format->Rmask == 0x000000ff) {
                texture_format = GL_RGB;
        }
        else {
            texture_format = GL_BGR;
        }
    } else {
        printf("warning: the image is not truecolor..  this will probably break\n");
    }

    float x, y;
    float width = surface->w;
    float height = surface->h;

    x = position.x;
    y = (Params::GetFloat("height") - height) - position.y;


    vertices.push_back(glm::vec2(x + width,y));
    vertices.push_back(glm::vec2(x,y + height));
    vertices.push_back(glm::vec2(x,y));

    vertices.push_back(glm::vec2(x + width,y + height));
    vertices.push_back(glm::vec2(x,y + height));
    vertices.push_back(glm::vec2(x + width,y));


    uvs.push_back(glm::vec2(1, 1));
    uvs.push_back(glm::vec2(0, 0));
    uvs.push_back(glm::vec2(0, 1));

    uvs.push_back(glm::vec2(1, 0));
    uvs.push_back(glm::vec2(0, 0));
    uvs.push_back(glm::vec2(1, 1));


    GLuint programID = Content::LoadShaderPair("TextShader");
    glUseProgram(programID);

    // Set our "myTextureSampler" sampler to user Texture Unit 0
    GLuint textureSampler  = glGetUniformLocation(programID, "myTextureSampler");

    //
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, colors, surface->w, surface->h, 0, texture_format, GL_UNSIGNED_BYTE, surface->pixels);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLuint sizeID = glGetUniformLocation(programID, "screen_size");
    glUniform2f(sizeID, Params::GetFloat("width"), Params::GetFloat("height"));

    // Bind our texture in Texture Unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    // Set our "myTextureSampler" sampler to user Texture Unit 0
    glUniform1i(textureSampler, 0);

    GLuint vertexbuffer;
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec2), &vertices[0], GL_STATIC_DRAW);

    GLuint uvbuffer;
    glGenBuffers(1, &uvbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
    glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glVertexAttribPointer(
        0,                  // attribute
        2,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        0,                  // stride
        (void*)0            // array buffer offset
    );

    // 1st attribute buffer : normals
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
    glVertexAttribPointer(
        1,                                // attribute
        2,                                // size
        GL_FLOAT,                         // type
        GL_TRUE,                         // normalized?
        0,                                // stride
        (void*)0                          // array buffer offset
    );

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Draw call
    glDrawArrays(GL_TRIANGLES, 0, vertices.size() );

    glDisable(GL_BLEND);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glDeleteTextures(1, &texture);
    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &uvbuffer);
    SDL_FreeSurface(surface);
}

// NEW
void Game::SimplePrint(string message, glm::vec2 position)
{
    SDL_Color TextColor = { 100, 100, 100 };
    SDL_Rect TextLocation;
    SDL_Texture* TextTexture;
    
    // generate a surface with the given text string
    SDL_Surface* TextSurface = TTF_RenderText_Solid( font, message.c_str(), TextColor );
    
    //Create renderer for window
    renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED );
    if( renderer == NULL )
    {
        printf( "Renderer could not be created! SDL Error: %s\n", SDL_GetError() );
    }
    
    //Create texture from surface pixels
    TextTexture = SDL_CreateTextureFromSurface( renderer, TextSurface );
    
    SDL_SetRenderDrawColor( renderer, 0xFF, 0xFF, 0xFF, 0xFF );
    
    //Set rendering space and render to screen
    TextLocation = { (int)position.x, (int)position.y, TextSurface->w, TextSurface->w };
    SDL_RenderCopy( renderer, TextTexture, NULL, &TextLocation );
    SDL_RenderPresent( renderer );
    
    SDL_DestroyRenderer(renderer);
}

void Game::DeletePhysicsConstraints()
{
	while (dynamicsWorld->getNumConstraints() > 0)
	{
		btTypedConstraint* constraint = dynamicsWorld->getConstraint(0);
		dynamicsWorld->removeConstraint(constraint);
		SAFE_DELETE(constraint);
	}
}

bool Game::Initialise()
{
	return GameComponent::Initialise();
}

bool BGE::Game::PreInitialise()
{
	instance = dynamic_pointer_cast<Game>(This());
	camera = make_shared<Camera>();
	soundSystem = make_shared<SoundSystem>();
	soundSystem->Initialise();
	soundSystem->enabled = true;
	Attach(camera);

	// Setup the Physics stuff
	// Set up the collision configuration and dispatcher
	collisionConfiguration = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collisionConfiguration);

	// The world.
	btVector3 worldMin(-1000, -1000, -1000);
	btVector3 worldMax(1000, 1000, 1000);
	broadphase = new btAxisSweep3(worldMin, worldMax);
	solver = new btSequentialImpulseConstraintSolver();
	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
	dynamicsWorld->setGravity(btVector3(0, 0, 0));

	physicsFactory = make_shared<PhysicsFactory>(dynamicsWorld);

	int x = SDL_WINDOWPOS_CENTERED;
	int y = SDL_WINDOWPOS_CENTERED;
	
    if (!Params::ExistsKey("width"))
    {
        Params::SetFloat("width", 800);
        Params::SetFloat("height", 600);
    }
    shared_ptr<GameComponent> controller = make_shared<FPSController>();
    camera->Attach(controller);
    
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
		return false;
	}

	/* Turn on double buffering with a 24bit Z buffer.
	* You may need to change this to 16 or 32 for your system */
    
//    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 4 );
//    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
    
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

	Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | (Params::GetBool("fullScreen") ? SDL_WINDOW_FULLSCREEN : 0);

	int display = Params::GetFloat("display");
	SDL_Rect bounds;
	SDL_GetDisplayBounds(display, &bounds);
    
    window = SDL_CreateWindow("",
		SDL_WINDOWPOS_UNDEFINED_DISPLAY(display),
		SDL_WINDOWPOS_UNDEFINED_DISPLAY(display),
		Params::GetFloat("width"),
		Params::GetFloat("height"),
		flags);
    if( window == NULL )
    {
        printf( "Window could not be created! SDL Error: %s\n", SDL_GetError() );
    }
    
	context = SDL_GL_CreateContext(window);

	/* This makes our buffer swap syncronized with the monitor's vertical refresh */
	//SDL_GL_SetSwapInterval(1);

	keyState = SDL_GetKeyboardState(NULL);
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		char msg[2048];
		sprintf(msg, "glewInit failed with error: %s", glewGetErrorString(err));
		throw BGE::Exception(msg);
	}
    
	glViewport(0, 0, Params::GetFloat("width"), Params::GetFloat("height"));

	//// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
    
    // Dont know what this does!!!
    // But nothing works without it
    // Didnt need it b4
    // BD 11 Oct 2017
    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // initialize TTF
	if (TTF_Init() == -1)
	{
		throw BGE::Exception("Could not init TTF");
	}
    // load the text font
    font = TTF_OpenFont("/Library/Fonts/Arial.ttf", fontSize); // Open a font & set the font size
    if( font == NULL ) {
        printf( "Failed to load lazy font! SDL_ttf Error: %s\n", TTF_GetError() );
    }

	LineDrawer::Instance()->Initialise();

    cout << "OpenGL version: "  << glGetString(GL_VERSION) << endl;
    cout << "OpenGL renderer: "  << glGetString(GL_RENDERER) << endl;
    cout << "OpenGL GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	running = true;
	initialised = true;

	return true;
}

void BGE::Game::setGravity(glm::vec3 gravity)
{
	dynamicsWorld->setGravity(GLToBtVector(gravity));
}
