#include <iostream>
#include <assert.h>
using namespace std;

//include SDL header
#include "SDL2-2.0.8\include\SDL.h"

//load libraries
#pragma comment(lib,"SDL2-2.0.8\\lib\\x86\\SDL2.lib")
#pragma comment(lib,"SDL2-2.0.8\\lib\\x86\\SDL2main.lib")

#pragma comment(linker,"/subsystem:console")

struct City
{
	float x, y, w, h;
};


namespace Simulation
{
	SDL_Renderer *renderer = NULL;
	int screen_width = 800;
	int screen_height = 600;

	unsigned char prev_key_state[256];
	unsigned char *keys = NULL;

	int *start_node;
	int *end_node;
	float *distance;
	const int num_cities = 5;
	City city[num_cities];

	void init()
	{
		SDL_Init(SDL_INIT_VIDEO);

		prev_key_state[256];
		keys = (unsigned char*)SDL_GetKeyboardState(NULL);

		SDL_Window *window = SDL_CreateWindow(
			"Fortnite",
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			screen_width, screen_height, SDL_WINDOW_SHOWN);

		renderer = SDL_CreateRenderer(window,
			-1, SDL_RENDERER_ACCELERATED);

		/*
		
		YOUR INIT CODE
		
		*/
		distance = (float*)malloc(sizeof(float)*num_cities);
		start_node = (int*)malloc(sizeof(int)*num_cities);
		end_node = (int*)malloc(sizeof(int)*num_cities);
	}

	void create_Random_Nodes()
	{
		for (int i = 0; i < num_cities; i++)
		{
			city[i].x = (screen_width / 2) + rand() % 100 + (-100);
			city[i].y = (screen_height / 2) + rand() % 100 + (-100);
			city[i].w = 50;
			city[i].h = 50;
		}
	}

	void retrieve_nodes()
	{
		create_Random_Nodes();
	}

	void update()
	{
		//copy previous key state
		memcpy(prev_key_state, keys, 256);

		//consume all window events, key state gets updated here automatically
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				exit(0);
			}
		}

		/*
		
		YOUR UPDATE CODE
		
		*/
		
	}

	void draw()
	{
		//set color to white
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		//clear screen with white
		SDL_RenderClear(renderer);

		for (int i = 0; i < num_cities; i++)
		{
			SDL_Rect rect;
			rect.h = city[i].h;
			rect.w = city[i].w;
			rect.x = city[i].x;
			rect.y = city[i].y;
			SDL_RenderFillRect(renderer, &rect);
		}
		

		//flip buffers
		SDL_RenderPresent(renderer);
	}

}
int main(int argc, char **argv)
{
	

	Simulation::init();


	for(;;)
	{
		Simulation::update();
		
		Simulation::draw();
	}


	return 0;
}