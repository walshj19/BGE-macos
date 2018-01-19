#pragma once
#include "Game.h"

namespace BGE
{
	class CubeTest :
		public Game
	{
	public:
		CubeTest(void);
		~CubeTest(void);

		bool Initialise();
		void Draw();
		void Cleanup();
        
        bool Run();

	private:
		GLuint vertexbuffer;
		GLuint colorbuffer;
		GLuint programID;
	};
}
