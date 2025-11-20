#pragma once
#include <CC_Application.h>
#include <CC_Entry.h>

class Game : public Cc::Application
{
public:
	Game();
	~Game();

	void Run() override;
};