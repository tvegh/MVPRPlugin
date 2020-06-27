#include "MVPRPlugin.h"
#pragma comment(lib, "pluginsdk.lib")
#include "bakkesmod\wrappers\includes.h"
#include <iomanip>
#include <sstream>

using namespace std;

BAKKESMOD_PLUGIN(MVPRPlugin, "MVPR Plugin", "1.0", PLUGINTYPE_FREEPLAY)

//do i need to declare all of these up here? probably not. oh well.
float goals = 0;
float assists = 0;
float saves = 0;
float shots = 0;
float mvpr = 0.0;
float omvpr = 0.0;
float t0mvpr = 0.0;
float t1mvpr = 0.0;
float t0count = 0.0;
float t1count = 0.0;
float userX = 0.0;
float userY = 0.0;
int backgroundX = 0;
int backgroundY = 0;
int backgroundWidth = 0;
int backgroundHeight = 0;
string playerName = "";
string playerNameShort = "";
string mvprString = "";
string omvprString = "";
float playerTeam = 0;
int playercount = 0;
bool isInOnlineGame = false;
//I wanted to use a map here to hold all three of these values but
//I DONT KNOW HOW CPP WORKS SO IM MAKING 3 DIFFERENT VECTORS. LEAVE ME ALONE.
vector<string> players;
vector<float> mvprs;
vector<float> teams;



void MVPRPlugin::onLoad() {
	cvarManager->registerCvar("enableBool", "1", "toggle plugin");
	cvarManager->registerCvar("alwaysOn", "0", "toggle always on");
	cvarManager->registerCvar("omvprBool", "0", "toggle OMVPR");
	cvarManager->registerCvar("bgOpacity", "173", "set background opacity");
	cvarManager->registerCvar("bgWidth", "300", "set background width");
	cvarManager->registerCvar("bgHeight", "250", "set background width");
	cvarManager->registerCvar("bgX", "87.0", "X Position %");
	cvarManager->registerCvar("bgY", "0.0", "Y Position %");
	gameWrapper->HookEvent("Function TAGame.GFxData_GameEvent_TA.OnOpenScoreboard", std::bind(&MVPRPlugin::openScoreboard, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GFxData_GameEvent_TA.OnCloseScoreboard", std::bind(&MVPRPlugin::closeScoreboard, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function ProjectX.GRI_X.EventGameStarted", std::bind(&MVPRPlugin::gameStart, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded", std::bind(&MVPRPlugin::gameEnd, this, std::placeholders::_1));
	//im hooking into the loading screen event to unregister drawables whenever you go to the loading screen. in theory, i dont need to hook into
	//exit to main menu or play freeplay map, as they will both take you to the loading screen, but i figure better safe than sorry
	gameWrapper->HookEvent("Function ProjectX.GFxShell_X.ExitToMainMenu", std::bind(&MVPRPlugin::unregister, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GFxData_Training_TA.PlayFreeplayMap", std::bind(&MVPRPlugin::unregister, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.LoadingScreen_TA.GetProtipInputType", std::bind(&MVPRPlugin::unregister, this, std::placeholders::_1));
}
void MVPRPlugin::onUnload() {
	gameWrapper->UnhookEvent("Function TAGame.GFxData_GameEvent_TA.OnOpenScoreboard");
	gameWrapper->UnhookEvent("Function TAGame.GFxData_GameEvent_TA.OnCloseScoreboard");
	gameWrapper->UnhookEvent("Function ProjectX.GRI_X.EventGameStarted");
	gameWrapper->UnhookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded");
	gameWrapper->UnhookEvent("Function ProjectX.GFxShell_X.ExitToMainMenu");
	gameWrapper->UnhookEvent("Function TAGame.GFxData_Training_TA.PlayFreeplayMap");
	gameWrapper->UnhookEvent("Function TAGame.LoadingScreen_TA.GetProtipInputType");
}

void MVPRPlugin::openScoreboard(std::string eventName) {
	//only show drawables if plugin is enabled and "always on" is false
	if (cvarManager->getCvar("enableBool").getBoolValue() && !cvarManager->getCvar("alwaysOn").getBoolValue()) {
		gameWrapper->RegisterDrawable(std::bind(&MVPRPlugin::render, this, std::placeholders::_1));
	}
}

void MVPRPlugin::closeScoreboard(std::string eventName) {
	//only unregister drawables if plugin is enabled and "always on" is false
	if (cvarManager->getCvar("enableBool").getBoolValue() && !cvarManager->getCvar("alwaysOn").getBoolValue()) {
		gameWrapper->UnregisterDrawables();
	}
}

void MVPRPlugin::gameStart(std::string eventName) {
	//only show drawables if plugin is enabled and "always on" is true
	//if "always on" is false, then unregister drawables, so they dont stay on screen when game starts
	if (cvarManager->getCvar("enableBool").getBoolValue() && cvarManager->getCvar("alwaysOn").getBoolValue()) {
		gameWrapper->RegisterDrawable(std::bind(&MVPRPlugin::render, this, std::placeholders::_1));
	}
	else if (cvarManager->getCvar("enableBool").getBoolValue() && !cvarManager->getCvar("alwaysOn").getBoolValue()) {
		gameWrapper->UnregisterDrawables();
	}
}
void MVPRPlugin::gameEnd(std::string eventName) {
	//only show drawables if plugin is enabled and "always on" is false
	//this shows the mvpr at the end screen, if you had "always on" set to false
	if (cvarManager->getCvar("enableBool").getBoolValue() && !cvarManager->getCvar("alwaysOn").getBoolValue()) {
		gameWrapper->RegisterDrawable(std::bind(&MVPRPlugin::render, this, std::placeholders::_1));
	}
}

void MVPRPlugin::unregister(std::string eventName) {
	//unregister drawables for multiple different events, but only if the plugin is enabled in the first place.
	if (cvarManager->getCvar("enableBool").getBoolValue()) {
		gameWrapper->UnregisterDrawables();
	}
}

void MVPRPlugin::render(CanvasWrapper canvas) {
	//only works when in game. this stops the game from crashing when trying to use scoreboard in training or other areas
	isInOnlineGame = gameWrapper->IsInOnlineGame() || gameWrapper->IsSpectatingInOnlineGame();
	if (isInOnlineGame) {
		ArrayWrapper<PriWrapper> pris = gameWrapper->GetOnlineGame().GetPRIs();

		//get X and Y as a percentage of the screen size
		//userX and userY will be between 0-1
		//get user defined width and height
		userX = cvarManager->getCvar("bgX").getFloatValue() / 100;
		userY = cvarManager->getCvar("bgY").getFloatValue() / 100;
		backgroundX = static_cast<int>(userX * canvas.GetSize().X);
		backgroundY = static_cast<int>(userY * canvas.GetSize().Y);
		backgroundWidth = cvarManager->getCvar("bgWidth").getIntValue();
		backgroundHeight = cvarManager->getCvar("bgHeight").getIntValue();

		//set up background and MVPR title.
		//bgopacity is set by user in settings
		//if OMVPR is true, then make title "MVPR/OMVPR"
		canvas.SetColor(0, 0, 0, cvarManager->getCvar("bgOpacity").getIntValue());
		canvas.DrawRect(Vector2{ backgroundX, backgroundY }, Vector2{ backgroundX + backgroundWidth, backgroundY + backgroundHeight });
		canvas.SetPosition(Vector2{ backgroundX + 15, backgroundY + 15 });
		canvas.SetColor(255, 255, 255, 255);
		if (cvarManager->getCvar("omvprBool").getBoolValue()) {
			canvas.DrawString("MVPR/OMVPR", 2, 2);
		}
		else {
			canvas.DrawString("MVPR", 2, 2);
		}

		//init values
		t0mvpr = 0.0;
		t1mvpr = 0.0;
		t0count = 0.0;
		t1count = 0.0;
		playercount = 0;
		players.clear();
		mvprs.clear();
		teams.clear();

		//for the first loop, we gather player data and calculate individual and team MVPR
		//only gather data if player is on team 0 or 1. if they are not, then they are not playing, only spectating, therefore we do not need their data.
		for (int i = 0; i < pris.Count(); i++) {
			playerTeam = pris.Get(i).GetTeamNum();
			if (playerTeam == 0 || playerTeam == 1) {
				playercount++;
				playerName = pris.Get(i).GetPlayerName().ToString();
				goals = pris.Get(i).GetMatchGoals();
				assists = pris.Get(i).GetMatchAssists();
				saves = pris.Get(i).GetMatchSaves();
				shots = pris.Get(i).GetMatchShots();
				mvpr = (goals * 1) + (assists * 0.75) + (saves * 0.6) + (shots / 3);
				if (playerTeam == 0) {
					t0mvpr += mvpr;
					t0count++;
				}
				else {
					t1mvpr += mvpr;
					t1count++;
				}
				players.push_back(playerName);
				mvprs.push_back(mvpr);
				teams.push_back(playerTeam);
			}
		}

		t0mvpr = t0mvpr / t0count;
		t1mvpr = t1mvpr / t1count;

		//loop through playercount. show player names and mvpr/omvpr (if omvpr is set to true)
		for (int i = 0; i < playercount; i++) {
			playerName = players[i];
			mvpr = mvprs[i];
			playerTeam = teams[i];

			stringstream ss;
			ss << fixed << setprecision(2) << mvpr;
			mvprString = "";
			mvprString = ss.str();

			stringstream sss;
			if (playerTeam == 0) {
				sss << fixed << setprecision(2) << t1mvpr;
			}
			else {
				sss << fixed << setprecision(2) << t0mvpr;
			}
			omvprString = "";
			omvprString = sss.str();
			canvas.SetPosition(Vector2{ backgroundX + 15, backgroundY + ((i * 20) + 50) });

			if (playerName.length() > 13) {
				playerNameShort = playerName.substr(0, 12) + "...";
			}
			else {
				playerNameShort = playerName;
			}

			if (cvarManager->getCvar("omvprBool").getBoolValue()) {

				canvas.DrawString(playerNameShort + ": " + mvprString + "/" + omvprString, 1.5, 1.5);
			}
			else {
				canvas.DrawString(playerName + ": " + mvprString, 1.5, 1.5);
			}
		}
	}
}