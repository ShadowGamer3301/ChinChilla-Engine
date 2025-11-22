#include "Game.h"

Game::Game()
{
}

Game::~Game()
{
}

void Game::Run()
{
	GetGraphics()->CompileShader("V_Default.hlsl", "P_Default.hlsl");
	GetGraphics()->LoadModel("blista.fbx");

	while (GetWindow()->UpdateWindow())
	{
		GetGraphics()->DrawFrame();
	}
}

Cc::Application* Cc::NewApplicationInterface(std::vector<const char*>& v_args)
{
	return new Game();
}