#pragma once
#pragma comment(lib, "BakkesMod.lib")
#include "bakkesmod/plugin/bakkesmodplugin.h"

class MVPRPlugin : public BakkesMod::Plugin::BakkesModPlugin
{
public:
	virtual void onLoad();
	virtual void onUnload();
	void openScoreboard(std::string eventName);
	void closeScoreboard(std::string eventName);
	void gameStart(std::string eventName);
	void gameEnd(std::string eventName);
	void unregister(std::string eventName);
	void render(CanvasWrapper canvas);
};
