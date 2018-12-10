#pragma once

//BEAR RUN

#include "include/Engine_core.h"
#include "include/Light_core.h"
#include "include/Actor_core.h"
#include "include/Particle_core.h"
#include "NN_core.h"


namespace My_Game
{

	

	namespace Command
	{

	}

	namespace Assets
	{
		namespace Animation
		{

		}
	}

	namespace World
	{

		namespace Parameters
		{

		}

		Tileset::Tileset tileset;
		Font::Font text;

		Grid::Grid maze;

		Grid_Camera::Grid_Camera camera;

		unsigned int last_step_time = 0;


		Vec2D::Vec2D mouse_grid_point;

		float temperature = 1.0;

		float fitness = 0.0;

		Actor::Factory obstacles;
		Actor::Factory agents_main;
		Actor::Factory agents_alt;
		int n_agents = 100;
		int n_obstacles = 10;
		double min_obstacle_force = 1000;
		double max_obstacle_force = 5000;
		int max_n_samples = 20;
		int current_sample = 0;

		int state = 0;//0 prep, 1 test, 2 update
		
		double fitness_main = 0.0;
		double fitness_alt = 0.0;
		double tmp_fitness_main = 0.0;
		double tmp_fitness_alt = 0.0;
		NN::Solution::Solution solution;
	}

	void init(int screen_w, int screen_h)
	{
		srand(time(0));
		//initialize all systems and open game window
		Engine::init("hello", screen_w, screen_h);

		Font::init(&World::text, "font_tileset.txt", Engine::renderer);
		Tileset::init(&World::tileset, "map_tileset.txt", Engine::renderer);

		Actor::init(&World::agents_main, World::n_agents);
		Actor::add_Sprite(&World::agents_main, "saitama_pink_run.txt", Engine::renderer);

		Actor::init(&World::agents_alt, World::n_agents);
		Actor::add_Sprite(&World::agents_alt, "saitama_pink_run.txt", Engine::renderer);

		Actor::init(&World::obstacles, World::n_obstacles);
		Actor::add_Sprite(&World::obstacles, "saitama_pink_run.txt", Engine::renderer);

		Grid::load(&World::maze, "collision.csv");

		Grid_Camera::init(&World::camera, Engine::screen_width, Engine::screen_height);
		World::camera.world_coord.x = 0;
		World::camera.world_coord.y = 0;
		World::camera.world_coord.w = World::maze.n_cols;
		World::camera.world_coord.h = World::maze.n_rows;


		NN::Solution::init(&World::solution, 3, 3, 1, 1.0, 0.97, 5, 4.0, 0.1, 1);

	}

	void learn()
	{
		//evaluate
		World::current_sample = 0;
		World::state = 2;
		World::fitness_main = 0;
		World::fitness_alt = 0;
		World::solution.temperature.temperature = 1.0;
		World::tmp_fitness_main = 0.0;
		World::tmp_fitness_alt = 0.0;

		Engine::event_Loop();
		printf("learn\n");
		int iter = 0;
		for (;;)
		{
			float dt = (14 + 2.0*rand() / RAND_MAX)/1000.0;

			iter++;
			Grid_Camera::calibrate(&World::camera);

			if (World::state == 0)
			{
				for (int i = 0; i < World::n_agents; i++)
				{
					Actor::set_Pos(i, 4 + 40.0*rand() / RAND_MAX, World::maze.n_rows - 4, &World::agents_main);
					Actor::set_Pos(i, Actor::get_Pos(i, &World::agents_main)->x, Actor::get_Pos(i, &World::agents_main)->y, &World::agents_alt);

					Vec2D::Vec2D *v = Actor::get_Vel(i, &World::agents_main);
					*v = {};
					v = Actor::get_Vel(i, &World::agents_alt);
					*v = {};
				}

				Actor::set_Pos(0, World::maze.n_cols - 4, World::maze.n_rows - 4, &World::obstacles);

				Vec2D::Vec2D *v = Actor::get_Vel(0, &World::obstacles);
				*v = {};

				double fx = World::min_obstacle_force + (World::max_obstacle_force - World::min_obstacle_force)*rand() / RAND_MAX;
				Vec2D::Vec2D force = { -fx,0.0 };
				Actor::add_Force(0, &World::obstacles, &force);


				World::state = 1;
			
			}
			else if (World::state == 1)
			{
				Actor::update_Vel(0, &World::obstacles, dt);
				Actor::update_Pos(0, &World::obstacles, dt);

				Vec2D::Vec2D *p = Actor::get_Pos(0, &World::obstacles);
				if (p->x <= 1)
				{
					World::state = 2;
					continue;
				}

				for (int i = 0; i < World::n_agents; i++)
				{
					double input[4];


					input[0] = abs(Actor::get_Pos(i, &World::agents_main)->x - Actor::get_Pos(0, &World::obstacles)->x);
					input[1] = 0.01*(Actor::get_Vel(0, &World::obstacles)->x);
					input[2] = dt;
					double output_main[1] = { 0 };
					NN::Solution::run_Main(output_main, input, &World::solution);


					input[0] = abs(Actor::get_Pos(i, &World::agents_alt)->x - Actor::get_Pos(0, &World::obstacles)->x);
					input[1] = 0.01*(Actor::get_Vel(0, &World::obstacles)->x);
					input[2] = dt;
					double output_alt[1] = { 0 };
					NN::Solution::run_Alt(output_alt, input, &World::solution);


					if (output_main[0] >= 0.9 && (Actor::get_Pos(i, &World::agents_main)->y >= World::maze.n_rows - 4))
					{
						Vec2D::Vec2D jump = { 0.0,-1900.0 };
						Actor::add_Force(i, &World::agents_main, &jump);
					}



					if (output_alt[0] >= 0.9 && (Actor::get_Pos(i, &World::agents_alt)->y >= World::maze.n_rows - 4))
					{
						Vec2D::Vec2D jump = { 0.0,-1900.0 };
						Actor::add_Force(i, &World::agents_alt, &jump);
					}

					Vec2D::Vec2D gravity = { 0.0,80 };
					Actor::add_Force(i, &World::agents_main, &gravity);
					Actor::add_Force(i, &World::agents_alt, &gravity);

					Actor::update_Vel(i, &World::agents_main, dt);
					Actor::update_Pos(i, &World::agents_main, dt);

					Actor::update_Vel(i, &World::agents_alt, dt);
					Actor::update_Pos(i, &World::agents_alt, dt);

					Vec2D::Vec2D *pmain = Actor::get_Pos(i, &World::agents_main);
					if (pmain->y > World::maze.n_rows - 4)
					{
						Actor::undo_Pos_Update(i, &World::agents_main);
						*(Actor::get_Vel(i, &World::agents_main)) = {};
					}
					Vec2D::Vec2D *palt = Actor::get_Pos(i, &World::agents_alt);
					if (palt->y > World::maze.n_rows - 4)
					{
						Actor::undo_Pos_Update(i, &World::agents_alt);
						*(Actor::get_Vel(i, &World::agents_alt)) = {};
					}

					if (Actor::collision(0, &World::obstacles, i, &World::agents_main))
					{
						World::tmp_fitness_main -= 1.0;
					}
					if (Actor::collision(0, &World::obstacles, i, &World::agents_alt))
					{
						World::tmp_fitness_alt -= 1.0;
					}
				}

			}
			else if (World::state == 2)
			{
				World::state = 0;

				if (++World::current_sample >= World::max_n_samples)
				{
					World::current_sample = 0;
					World::fitness_main = World::tmp_fitness_main/World::max_n_samples;
					World::fitness_alt = World::tmp_fitness_alt/World::max_n_samples;
					

					NN::Solution::learn(&World::solution, World::fitness_main, World::fitness_alt);
					printf("%d %f %f\n", iter, World::fitness_main, NN::Solution::get_Temperature(&World::solution));

					World::tmp_fitness_main = 0;
					World::tmp_fitness_alt = 0;

					if (NN::Solution::get_Temperature(&World::solution) <= 0.0001)
					{
						return;
					}

				}

				
				
			}
		
		}
		//
	}

	void begin_Play(unsigned int current_time)
	{
		RGBA::RGBA text_color = { 0,0,0,255 };
		Font::set_Screen_Size(&World::text, 16, 16);
		Font::set_RGBA(&World::text, &text_color);

		for (int i = 0; i < World::n_agents; i++)
		{
			Actor::spawn(&World::agents_main, 4, current_time);
			Actor::spawn(&World::agents_alt, 4, current_time);
			*(Actor::get_Color(i, &World::agents_main)) = { 0,255,0,150 };
			*(Actor::get_Color(i, &World::agents_alt)) = { 255,0,0,150 };
		}

		Actor::spawn(&World::obstacles, 4, current_time);
		Actor::set_Flip(0, 1, &World::obstacles);

		learn();
		printf("Done\n");
	}

	void update(unsigned int current_time, float dt)
	{

		Engine::event_Loop();

		if (Input::keys[SDL_SCANCODE_SPACE])
		{
			World::fitness_main = 0;
			World::fitness_alt = 0;
			World::solution.temperature.temperature = 1.0;
			World::state = 0;
			learn();
		}

		Grid_Camera::calibrate(&World::camera);


		Vec2D::Vec2D screen_point = { Input::mouse_x ,Input::mouse_y };
		Grid_Camera::screen_to_Grid(&World::mouse_grid_point, &screen_point, &World::camera);

		if (World::state == 0)
		{
			for (int i = 0; i < World::n_agents; i++)
			{
				Actor::set_Pos(i, 4 + 40.0*rand() / RAND_MAX, World::maze.n_rows - 4, &World::agents_main);
				Actor::set_Pos(i, Actor::get_Pos(i, &World::agents_main)->x, Actor::get_Pos(i, &World::agents_main)->y, &World::agents_alt);

				Vec2D::Vec2D *v = Actor::get_Vel(i, &World::agents_main);
				*v = {};
				v = Actor::get_Vel(i, &World::agents_alt);
				*v = {};
			}

			World::tmp_fitness_main = 0.0;
			World::tmp_fitness_alt = 0.0;

			Actor::set_Pos(0, World::maze.n_cols - 4, World::maze.n_rows - 4, &World::obstacles);
			
			Vec2D::Vec2D *v = Actor::get_Vel(0, &World::obstacles);
			*v = {};

			double fx = World::min_obstacle_force + (World::max_obstacle_force - World::min_obstacle_force)*rand() / RAND_MAX;
			Vec2D::Vec2D force = { -fx,0.0 };
			Actor::add_Force(0, &World::obstacles, &force);

			
			World::state = 1;
		}
		else if (World::state == 1)
		{
			Actor::update_Vel(0, &World::obstacles, dt);
			Actor::update_Pos(0, &World::obstacles, dt);

			Vec2D::Vec2D *p = Actor::get_Pos(0, &World::obstacles);
			if (p->x <= 1)
			{
				World::state = 2;
				return;
			}

			for (int i = 0; i < World::n_agents; i++)
			{
				double input[4];
				

				input[0] = abs(Actor::get_Pos(i, &World::agents_main)->x - Actor::get_Pos(0, &World::obstacles)->x);
				input[1] = 0.01*(Actor::get_Vel(0, &World::obstacles)->x);
				input[2] = dt;
				double output_main[1] = { 0 };
				NN::Solution::run_Main(output_main, input, &World::solution);


				input[0] = abs(Actor::get_Pos(i, &World::agents_alt)->x - Actor::get_Pos(0, &World::obstacles)->x);
				input[1] = 0.01*(Actor::get_Vel(0, &World::obstacles)->x);
				input[2] = dt;
				double output_alt[1] = { 0 };
				NN::Solution::run_Alt(output_alt, input, &World::solution);


				if (output_main[0] >= 0.9 && (Actor::get_Pos(i, &World::agents_main)->y >= World::maze.n_rows - 4))
				{
					Vec2D::Vec2D jump = { 0.0,-1900.0 };
					Actor::add_Force(i, &World::agents_main, &jump);
				}

				if (output_alt[0] >= 0.9 && (Actor::get_Pos(i, &World::agents_alt)->y >= World::maze.n_rows - 4))
				{
					Vec2D::Vec2D jump = { 0.0,-1900.0 };
					Actor::add_Force(i, &World::agents_alt, &jump);
				}

				Vec2D::Vec2D gravity = { 0.0,80 };
				Actor::add_Force(i, &World::agents_main, &gravity);
				Actor::add_Force(i, &World::agents_alt, &gravity);

				Actor::update_Vel(i, &World::agents_main, dt);
				Actor::update_Pos(i, &World::agents_main, dt);

				Actor::update_Vel(i, &World::agents_alt, dt);
				Actor::update_Pos(i, &World::agents_alt, dt);

				Vec2D::Vec2D *pmain = Actor::get_Pos(i, &World::agents_main);
				if (pmain->y > World::maze.n_rows - 4)
				{
					Actor::undo_Pos_Update(i, &World::agents_main);
					*(Actor::get_Vel(i, &World::agents_main)) = {};
				}
				Vec2D::Vec2D *palt = Actor::get_Pos(i, &World::agents_alt);
				if (palt->y > World::maze.n_rows - 4)
				{
					Actor::undo_Pos_Update(i, &World::agents_alt);
					*(Actor::get_Vel(i, &World::agents_alt)) = {};
				}

				if (Actor::collision(0, &World::obstacles, i, &World::agents_main))
				{
					World::tmp_fitness_main -= 1.0;
				}
				if (Actor::collision(0, &World::obstacles, i, &World::agents_alt))
				{
					World::tmp_fitness_alt -= 1.0;
				}
			}

		}
		else if (World::state == 2)
		{
			//evaluate
			World::state = 0;
			World::fitness_main = World::tmp_fitness_main;
			World::fitness_alt = World::tmp_fitness_alt;
			//NN::Solution::learn(&World::solution, World::fitness_main, World::fitness_alt);
		}

		//
	}

	void draw(unsigned int current_time)
	{
		SDL_SetRenderDrawColor(Engine::renderer, 255, 255, 255, 255);
		SDL_RenderClear(Engine::renderer);

		RGBA::RGBA tile_tint = { 255,255,255,255 };
		Tileset::draw_Grid(&World::tileset, &World::camera, &World::maze, &tile_tint, Engine::renderer);


		Actor::draw(0, &World::obstacles, &World::camera, current_time, Engine::renderer);

		for (int i = 0; i < World::n_agents; i++)
		{
			Actor::draw(i, &World::agents_main, &World::camera, current_time, Engine::renderer);
			//Actor::draw(i, &World::agents_alt, &World::camera, current_time, Engine::renderer);
		}


		char str[256];
		sprintf(str, "main %f\nalt %f\ntemp %f", World::fitness_main, World::fitness_alt, World::solution.temperature.temperature);
		Font::set_Screen_Size(&World::text, 32, 32);
		Font::set_Screen_Pos(&World::text, 10, 10);
		Font::draw(&World::text, str, strlen(str), &World::camera, Engine::renderer);

		//flip buffers
		SDL_RenderPresent(Engine::renderer);
	}


}