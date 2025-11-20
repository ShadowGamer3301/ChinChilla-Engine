#include "Game.h"

Game::Game()
{
}

Game::~Game()
{
}

void Game::Run()
{
	GetGraphics()->CompileShader("../Assets/V_Default.hlsl", "../Assets/P_Default.hlsl");

	while (GetWindow()->UpdateWindow())
	{
		GetGraphics()->DrawFrame();
	}
}

Cc::Application* Cc::NewApplicationInterface(std::vector<const char*>& v_args)
{
	return new Game();
}