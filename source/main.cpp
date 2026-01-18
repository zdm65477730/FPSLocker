#define TESLA_INIT_IMPL // If you have more than one file using the tesla header, only define this in the main one
#include <tesla.hpp>    // The Tesla Header
#include "MiniList.hpp"
#include "NoteHeader.hpp"
#include "List.hpp"
#include <sys/stat.h>
#include <dirent.h>
#include "SaltyNX.h"
#include "Lock.hpp"
#include "Utils.hpp"
#include <cstdlib>
#include "omm.h"

using namespace tsl;

union {
	struct {
		bool handheld: 1;
		bool docked: 1;
		unsigned int reserved: 6;
	} NX_PACKED ds;
	uint8_t general;
} displaySync;
bool isOLED = false;
bool isLite = false;
uint8_t refreshRate_g = 60;
bool oldSalty = false;

#include "Modes/Advanced.hpp"

class NoGameSub : public tsl::Gui {
public:
	uint64_t _titleid = 0;
	char _titleidc[17] = "";
	std::string _titleName = "";

	NoGameSub(uint64_t titleID, std::string titleName) {
		_titleid = titleID;
		sprintf(_titleidc, "%016lX", titleID);
		_titleName = titleName;
	}

	// Called when this Gui gets loaded to create the UI
	// Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
	virtual tsl::elm::Element* createUI() override {
		// A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
		// If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
		auto frame = new tsl::elm::OverlayFrame(_titleidc, _titleName);

		// A list that can contain sub elements and handles scrolling
		auto list = new tsl::elm::List();

		auto *clickableListItem = new tsl::elm::ListItem2("DeleteSettingsNoGameSubListItem"_tr);
		clickableListItem->setClickListener([this](u64 keys) {
			if (keys & HidNpadButton_A) {
				char path[512] = "";
				if (_titleid != 0x1234567890ABCDEF) for (size_t i = 0; i < 0x10; i++) {
					sprintf(path, "sdmc:/SaltySD/plugins/FPSLocker/%016lx.dat", _titleid + i);
					remove(path);
				}
				else {
					struct dirent *entry;
					DIR *dp;
					sprintf(path, "sdmc:/SaltySD/plugins/FPSLocker/");

					dp = opendir(path);
					if (!dp)
						return true;
					while ((entry = readdir(dp))) {
						if (entry -> d_type != DT_DIR && std::string(entry -> d_name).find(".dat") != std::string::npos) {
							sprintf(path, "sdmc:/SaltySD/plugins/FPSLocker/%s", entry->d_name);
							remove(path);
						}
					}
					closedir(dp);
				}
				return true;
			}
			return false;
		});

		list->addItem(clickableListItem);

		auto *clickableListItem2 = new tsl::elm::ListItem2("DeletePatchesNoGameSubListItem"_tr);
		clickableListItem2->setClickListener([this](u64 keys) {
			if (keys & HidNpadButton_A) {
				char folder[640] = "";
				if (_titleid != 0x1234567890ABCDEF) {
					sprintf(folder, "sdmc:/SaltySD/plugins/FPSLocker/patches/%016lx/", _titleid);

					struct dirent *entry;
					DIR *dp;

					dp = opendir(folder);
					if (!dp)
						return true;
					while ((entry = readdir(dp))) {
						if (entry -> d_type != DT_DIR && std::string(entry -> d_name).find(".bin") != std::string::npos) {
							sprintf(folder, "sdmc:/SaltySD/plugins/FPSLocker/patches/%016lx/%s", _titleid, entry -> d_name);
							remove(folder);
						}
					}
					closedir(dp);
				}
				else {
					struct dirent *entry;
					struct dirent *entry2;
					DIR *dp;
					DIR *dp2;

					sprintf(folder, "sdmc:/SaltySD/plugins/FPSLocker/patches/");
					dp = opendir(folder);
					if (!dp)
						return true;
					while ((entry = readdir(dp))) {
						if (entry -> d_type != DT_DIR)
							continue;
						sprintf(folder, "sdmc:/SaltySD/plugins/FPSLocker/patches/%s/", entry -> d_name);
						dp2 = opendir(folder);
						while ((entry2 = readdir(dp2))) {
							if (entry2 -> d_type != DT_DIR && std::string(entry2 -> d_name).find(".bin") != std::string::npos) {
								sprintf(folder, "sdmc:/SaltySD/plugins/FPSLocker/patches/%s/%s", entry -> d_name, entry2 -> d_name);
								remove(folder);
							}
						}
						closedir(dp2);
					}
					closedir(dp);
				}
				return true;
			}
			return false;
		});

		list->addItem(clickableListItem2);

		frame->setContent(list);

		return frame;
	}
};

class NoGame2 : public tsl::Gui {
public:

	Result rc = 1;
	NoGame2(Result result, u8 arg2, bool arg3) {
		rc = result;
	}

	// Called when this Gui gets loaded to create the UI
	// Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
	virtual tsl::elm::Element* createUI() override {
		// A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
		// If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
		auto frame = new tsl::elm::OverlayFrame("PluginName"_tr, VERSION);

		// A list that can contain sub elements and handles scrolling
		auto list = new tsl::elm::List();

		if (oldSalty || !SaltySD) {
			list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
				if (!SaltySD) {
					renderer->drawString("SaltyNXNotWorkingNoGame2CustomDrawerText"_tr.c_str(), false, x, y+20, 20, renderer->a(0xF33F));
				}
				else if (!plugin) {
					renderer->drawString("SaltyNXNotFoundNoGame2CustomDrawerText"_tr.c_str(), false, x, y+20, 20, renderer->a(0xF33F));
				}
				else if (!check) {
					renderer->drawString("GameNotRunningNoGame2CustomDrawerText"_tr.c_str(), false, x, y+20, 19, renderer->a(0xF33F));
				}
			}), 30);
		}

		if (R_FAILED(rc)) {
			char error[24] = "";
			sprintf(error, "ErrorNoGame2ListItem"_tr.c_str(), rc);
			auto *clickableListItem2 = new tsl::elm::ListItem2(error);
			clickableListItem2->setClickListener([](u64 keys) {
				if (keys & HidNpadButton_A) {
					return true;
				}
				return false;
			});

			list->addItem(clickableListItem2);
		}
		else {
			auto *clickableListItem3 = new tsl::elm::ListItem2("AllNoGame2ListItem"_tr);
			clickableListItem3->setClickListener([](u64 keys) {
				if (keys & HidNpadButton_A) {
					tsl::changeTo<NoGameSub>(0x1234567890ABCDEF, "EverythingNoGame2ListItemText"_tr);
					return true;
				}
				return false;
			});

			list->addItem(clickableListItem3);
			mutexLock(&TitlesAccess);
			for (size_t i = 0; i < titles.size(); i++) {
				auto *clickableListItem = new tsl::elm::ListItem2(titles[i].TitleName);
				clickableListItem->setClickListener([i](u64 keys) {
					if (keys & HidNpadButton_A) {
						tsl::changeTo<NoGameSub>(titles[i].TitleID, titles[i].TitleName);
						return true;
					}
					return false;
				});

				list->addItem(clickableListItem);
			}
			mutexUnlock(&TitlesAccess);
		}

		frame->setContent(list);

		return frame;
	}
};

#include "Modes/Display.hpp"

class NoGame : public tsl::Gui {
public:

	Result rc = 1;
	NoGame(Result result, u8 arg2, bool arg3) {
		rc = result;
	}

	// Called when this Gui gets loaded to create the UI
	// Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
	virtual tsl::elm::Element* createUI() override {
		// A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
		// If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
		auto frame = new tsl::elm::OverlayFrame("PluginName"_tr, VERSION);

		// A list that can contain sub elements and handles scrolling
		auto list = new tsl::elm::List();

		list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
			if (!SaltySD) {
				renderer->drawString("SaltyNXNotWorkingNoGameCustomDrawerText"_tr.c_str(), false, x, y+20, 20, renderer->a(0xF33F));
			}
			else if (!plugin) {
				renderer->drawString("SaltyNXNotFoundNoGameCustomDrawerText"_tr.c_str(), false, x, y+20, 20, renderer->a(0xF33F));
			}
			else if (!check) {
				renderer->drawString("GameNotRunningNoGameCustomDrawerText"_tr.c_str(), false, x, y+20, 19, renderer->a(0xF33F));
			}
		}), 30);

		auto *clickableListItem2 = new tsl::elm::ListItem2("GameListNoGameListItem"_tr);
		clickableListItem2->setClickListener([this](u64 keys) { 
			if (keys & HidNpadButton_A) {
				tsl::changeTo<NoGame2>(this -> rc, 2, true);
				return true;
			}
			return false;
		});

		list->addItem(clickableListItem2);

		auto *clickableListItem3 = new tsl::elm::ListItem2("DisplaySettingsNoGameListItem"_tr, "\uE151");
		clickableListItem3->setClickListener([](u64 keys) { 
			if (keys & HidNpadButton_A) {
				tsl::changeTo<WarningDisplayGui>();
				return true;
			}
			return false;
		});

		list->addItem(clickableListItem3);

		frame->setContent(list);

		return frame;
	}

	virtual void update() override {}

	// Called once every frame to handle inputs not handled by other UI elements
	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		return false;   // Return true here to singal the inputs have been consumed
	}
};

class DockedFPSTargetGui : public tsl::Gui {
public:
	uint8_t maxFPS = 0;
	DockedModeRefreshRateAllowed rr = {0};
	DockedAdditionalSettings as = {0};
	uint8_t selected = 0;
	float counter = 0;
	uint8_t AllowedFPSTargets[32] = {15, 20, 25, 30, 35, 40, 45, 50, 55, 60};
	uint8_t sizeofAllowedFPSTargets = 10;
	s32 height = 720;
	uint8_t highestRefreshRate = 60;
	DockedFPSTargetGui() {
		s32 width = 0;
		ommGetDefaultDisplayResolution(&width, &height);
		if (height == 1080 || height == 720) {
			LoadDockedModeAllowedSave(rr, as, nullptr, (height == 720) ? true : false);
		}
		if (height == 720 || height == 1080) for (size_t i = 5; i < sizeof(DockedModeRefreshRateAllowed); i++) {
			if (rr[i] == true) {
				AllowedFPSTargets[sizeofAllowedFPSTargets++] = DockedModeRefreshRateAllowedValues[i];
			}
		}
		if (Shared -> FPSlockedDocked) {
			for (size_t i = 0; i < sizeofAllowedFPSTargets; i++) {
				if (Shared->FPSlockedDocked == AllowedFPSTargets[i]) {
					selected = i;
					break;
				}
			}			
		}
		else if (Shared -> FPSmode == 2) selected = 3;
		else selected = 9;
	}

	// Called when this Gui gets loaded to create the UI
	// Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
	virtual tsl::elm::Element* createUI() override {
		// A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
		// If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
		auto frame = new tsl::elm::OverlayFrame("PluginName"_tr, "ChangeFPSTargetDockedFPSTargetGuiOverlayFrame"_tr);

		// A list that can contain sub elements and handles scrolling
		auto list = new tsl::elm::List();

		list->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
			for (uint8_t i = 0; i < sizeofAllowedFPSTargets; i += 1) {
				char FPS[] = "254";
				snprintf(FPS, sizeof(FPS), "%d", AllowedFPSTargets[i]);
				if (selected == i) {
					auto new_pos = renderer->drawString(FPS, false, x+40, y+60, 40, renderer->a(0x0000));
					auto offset_x = (60 - new_pos.first) / 2;
					if (AllowedFPSTargets[i] >= 100) offset_x = (80 - new_pos.first) / 2;
					float progress = (std::sin(counter) + 1) / 2;
					tsl::Color highlightColor = {   static_cast<u8>((0x2 - 0x8) * progress + 0x8),
													static_cast<u8>((0x8 - 0xF) * progress + 0xF),
													static_cast<u8>((0xC - 0xF) * progress + 0xF),
													0xF };
					renderer->drawRect(x+((80 * (i % 4)) + 20) - offset_x, y+((80*(i / 4))+5), 4, 56, a(highlightColor));
					//renderer->drawRect((x+((80 * (i % 4)) + 20) - offset_x)+4, y+((80*(i / 4))+5), 56, 4, a(highlightColor));
					//renderer->drawRect((x+((80 * (i % 4)) + 20) - offset_x)+56, y+((80*(i / 4))+5), 4, 60, a(highlightColor));
					if (AllowedFPSTargets[i] < 100) renderer->drawRect((x+((80 * (i % 4)) + 20) - offset_x), (y+((80*(i / 4))+5))+56, 56, 4, a(highlightColor));
					else renderer->drawRect((x+((80 * (i % 4)) + 20) - offset_x), (y+((80*(i / 4))+5))+56, 80, 4, a(highlightColor));
				}
				renderer->drawString(FPS, false, x+((80 * (i % 4)) + 20), y+((80*(i / 4))+50), 40, renderer->a(0xFFFF));
			}
		}), 480);

		frame->setContent(list);

		return frame;
	}

	// Called once every frame to handle inputs not handled by other UI elements
	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		smInitialize();
		if (R_SUCCEEDED(apmInitialize())) {
			ApmPerformanceMode mode = ApmPerformanceMode_Invalid;
			apmGetPerformanceMode(&mode);
			apmExit();
			if (mode != ApmPerformanceMode_Boost) {
				smExit();
				tsl::goBack();
				return true;
			}
		}
		smExit();
		s32 width_impl = 0;
		s32 height_impl = 0;
		ommGetDefaultDisplayResolution(&width_impl, &height_impl);
		if (height_impl != height) {
			tsl::goBack();
			return true;
		}
		counter += 0.1f;
		if (keysDown & HidNpadButton_Down) {
			if ((selected / 4) < (sizeofAllowedFPSTargets / 4)) 
				selected += 4;
			else selected = selected % 4;
			if (selected >= sizeofAllowedFPSTargets)
				selected = sizeofAllowedFPSTargets - 1;
			return true;
		}
		else if (keysDown & HidNpadButton_Up) {
			if ((selected / 4) > 0) 
				selected -= 4;
			else selected = ((sizeofAllowedFPSTargets / 4) * 4) + (selected % 4);
			if (selected >= sizeofAllowedFPSTargets)
				selected = sizeofAllowedFPSTargets - 1;	
			return true;
		}
		else if (keysDown & HidNpadButton_Right) {
			if (selected % 4 < 3) 
				selected += 1;
			else {
				selected -= 3;
			}
			if (selected >= sizeofAllowedFPSTargets)
				selected = (sizeofAllowedFPSTargets / 4) * 4;
			return true;
		}
		else if (keysDown & HidNpadButton_Left) {
			if (selected % 4 > 0) 
				selected -= 1;
			else {
				selected += 3;
			}
			if (selected >= sizeofAllowedFPSTargets)
				selected = sizeofAllowedFPSTargets - 1;
			return true;
		}

		if (keysDown & HidNpadButton_A) {
			(Shared -> FPSlockedDocked) = AllowedFPSTargets[selected];
			if (!oldSalty && displaySync.ds.docked) {
				if (R_SUCCEEDED(SaltySD_Connect())) {
					bool skip = false;
					SaltySD_SetDisplayRefreshRate(AllowedFPSTargets[selected]);
					for (uint8_t x = 0; x < sizeof(DockedModeRefreshRateAllowed); x++) {
						if (rr[x] == true && DockedModeRefreshRateAllowedValues[x] == AllowedFPSTargets[selected]) {
							refreshRate_g = AllowedFPSTargets[selected];
							skip = true;
						}
						else if (rr[x] == true && DockedModeRefreshRateAllowedValues[x] == (AllowedFPSTargets[selected] * 2)) {
							refreshRate_g = AllowedFPSTargets[selected] * 2;
							skip = true;
						}
						else if (rr[x] == true && DockedModeRefreshRateAllowedValues[x] == (AllowedFPSTargets[selected] * 3)) {
							refreshRate_g = AllowedFPSTargets[selected] * 3;
							skip = true;
						}
						else if (rr[x] == true && DockedModeRefreshRateAllowedValues[x] == (AllowedFPSTargets[selected] * 4)) {
							refreshRate_g = AllowedFPSTargets[selected] * 4;
							skip = true;
						}
						if (skip) break;
					}
					if (!skip) {
						uint8_t target = 60;
						for (uint8_t x = 0; x < sizeof(DockedModeRefreshRateAllowed); x++) {
							if (rr[x] == true && AllowedFPSTargets[selected] <= DockedModeRefreshRateAllowedValues[x]) {
								if (DockedModeRefreshRateAllowedValues[x] < target) target = DockedModeRefreshRateAllowedValues[x];
							}
						}
						refreshRate_g = target;
					}
					Shared->displaySync.ds.docked = refreshRate_g > 0;
				}
				SaltySD_Term();
			}
			saveSettings();
			tsl::goBack();
			return true;
		}			
		return false;   // Return true here to singal the inputs have been consumed
	}
};

bool blocked = false;

class GuiTest : public tsl::Gui {
public:
	ApmPerformanceMode entry_mode = ApmPerformanceMode_Invalid;
	bool render100Above = false;
	bool pluginRanAtBoot = false;
	GuiTest(u8 arg1, u8 arg2, bool arg3) { 

		if (isLite) entry_mode = ApmPerformanceMode_Normal;
		else {
			smInitialize();
			if (R_SUCCEEDED(apmInitialize())) {
				apmGetPerformanceMode(&entry_mode);
				apmExit();
			}
			else entry_mode = ApmPerformanceMode_Normal;
			smExit();
		}
	}

	// Called when this Gui gets loaded to create the UI
	// Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
	virtual tsl::elm::Element* createUI() override {
		// A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
		// If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
		auto frame = new tsl::elm::OverlayFrame("PluginName"_tr, VERSION);

		// A list that can contain sub elements and handles scrolling
		auto list = new tsl::elm::List();
		
		list->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
			if (!SaltySD) {
				renderer->drawString("SaltNXNotWorkingGuiTestCustomDrawerText"_tr.c_str(), false, x, y+50, 20, renderer->a(0xF33F));
			}
			else if (!plugin) {
				renderer->drawString("SaltNXNotFoundGuiTestCustomDrawerText"_tr.c_str(), false, x, y+50, 20, renderer->a(0xF33F));
			}
			else if (!check) {
				if (closed) {
					renderer->drawString("GameClosedGuiTestCustomDrawerText"_tr.c_str(), false, x, y+20, 19, renderer->a(0xF33F));
					renderer->drawString("RestartOverlayToCheckGuiTestCustomDrawerText"_tr.c_str(), false, x, y+70, 20, renderer->a(0xFFFF));
				}
				else {
					renderer->drawString("GameNotRunningGuiTestCustomDrawerText"_tr.c_str(), false, x, y+20, 19, renderer->a(0xF33F));
				}
			}
			else if (!PluginRunning) {
				renderer->drawString("GameRunningGuiTestCustomDrawerText"_tr.c_str(), false, x, y+20, 20, renderer->a(0xFFFF));
				renderer->drawString("NXFPSNotRunningGuiTestCustomDrawerText"_tr.c_str(), false, x, y+70, 20, renderer->a(0xF33F));
			}
			else if (!(Shared -> pluginActive)) {
				renderer->drawString("NXFPSRunningNoFrameProcessedGuiTestCustomDrawerText"_tr.c_str(), false, x, y+20, 20, renderer->a(0xF33F));
			}
			else {
				renderer->drawString("NXFPSRunningGuiTestCustomDrawerText"_tr.c_str(), false, x, y+20, 20, renderer->a(0xFFFF));
				if (((Shared -> API) > 0) && ((Shared -> API) <= 2))
					renderer->drawString(FPSMode_c, false, x, y+43, 20, renderer->a(0xFFFF));
				renderer->drawString(FPSTarget_c, false, x, y+86, 20, renderer->a(0xFFFF));
				if (render100Above) renderer->drawString(PFPS_c, false, x+265, y+48, 50, renderer->a(0xFFFF));
				else renderer->drawString(PFPS_c, false, x+290, y+48, 50, renderer->a(0xFFFF));
				renderer->drawString("FPSGuiTestCustomDrawerText"_tr.c_str(), false, x+320, y+70, 20, renderer->a(0xFFFF));
				if (Shared -> forceOriginalRefreshRate) renderer->drawString("PatchNowForcing60HzGuiTestCustomDrawerText"_tr.c_str(), false, x, y+129, 20, renderer->a(0xF99F));
			}
		}), 170);

		if (PluginRunning && (Shared -> pluginActive)) {
			pluginRanAtBoot = true;
			if (entry_mode == ApmPerformanceMode_Normal) {
				auto *clickableListItem = new tsl::elm::ListItem2("IncreaseFPSGuiTestListItem"_tr);
				clickableListItem->setClickListener([](u64 keys) { 
					if ((keys & HidNpadButton_A) && PluginRunning) {
						if ((Shared -> FPSmode) == 2 && !(Shared -> FPSlocked)) {
							(Shared -> FPSlocked) = 35;
						}
						else if (!(Shared -> FPSlocked)) {
							(Shared -> FPSlocked) = 60;
						}
						else if ((Shared -> FPSlocked) < isOLED ? supportedHandheldRefreshRatesOLED[sizeof(supportedHandheldRefreshRatesOLED)-1] : supportedHandheldRefreshRates[sizeof(supportedHandheldRefreshRates)-1]) {
							(Shared -> FPSlocked) += 5;
						}
						if (!oldSalty && displaySync.ds.handheld) {
							if (R_SUCCEEDED(SaltySD_Connect())) {
								bool skip = false;
								SaltySD_SetDisplayRefreshRate((Shared -> FPSlocked));
								for (uint8_t x = 0; x < sizeof(supportedHandheldRefreshRates); x++) {
									if (supportedHandheldRefreshRates[x] == (Shared -> FPSlocked)) {
										refreshRate_g = (Shared -> FPSlocked);
										skip = true;
									}
									else if (supportedHandheldRefreshRates[x] == ((Shared -> FPSlocked) * 2)) {
										refreshRate_g = (Shared -> FPSlocked) * 2;
										skip = true;
									}
									else if (supportedHandheldRefreshRates[x] == ((Shared -> FPSlocked) * 3)) {
										refreshRate_g = (Shared -> FPSlocked) * 3;
										skip = true;
									}
									else if (supportedHandheldRefreshRates[x] == ((Shared -> FPSlocked) * 4)) {
										refreshRate_g = (Shared -> FPSlocked) * 4;
										skip = true;
									}
									if (skip) break;
								}
								if (!skip) {
									refreshRate_g = 60;
								}
								Shared->displaySync.ds.handheld = refreshRate_g > 0;
								SaltySD_Term();
							}
						}
						saveSettings();
						return true;
					}
					return false;
				});

				list->addItem(clickableListItem);

				auto *clickableListItem2 = new tsl::elm::ListItem2("DecreaseFPSGuiTestListItem"_tr);
				clickableListItem2->setClickListener([](u64 keys) { 
					if ((keys & HidNpadButton_A) && PluginRunning) {
						if ((Shared -> FPSmode) < 2 && !(Shared -> FPSlocked)) {
							(Shared -> FPSlocked) = 55;
						}
						else if (!(Shared -> FPSlocked)) {
							(Shared -> FPSlocked) = 25;
						}
						else if ((Shared -> FPSlocked) > 15) {
							(Shared -> FPSlocked) -= 5;
						}
						if (!oldSalty && displaySync.ds.handheld) {
							if (R_SUCCEEDED(SaltySD_Connect())) {
								bool skip = false;
								SaltySD_SetDisplayRefreshRate((Shared -> FPSlocked));
								for (uint8_t x = 0; x < sizeof(supportedHandheldRefreshRates); x++) {
									if (supportedHandheldRefreshRates[x] == (Shared -> FPSlocked)) {
										refreshRate_g = (Shared -> FPSlocked);
										skip = true;
									}
									else if (supportedHandheldRefreshRates[x] == ((Shared -> FPSlocked) * 2)) {
										refreshRate_g = (Shared -> FPSlocked) * 2;
										skip = true;
									}
									else if (supportedHandheldRefreshRates[x] == ((Shared -> FPSlocked) * 3)) {
										refreshRate_g = (Shared -> FPSlocked) * 3;
										skip = true;
									}
									else if (supportedHandheldRefreshRates[x] == ((Shared -> FPSlocked) * 4)) {
										refreshRate_g = (Shared -> FPSlocked) * 4;
										skip = true;
									}
									if (skip) break;
								}
								if (!skip) {
									refreshRate_g = 60;
								}
								SaltySD_Term();
								Shared->displaySync.ds.handheld = refreshRate_g > 0;
							}
						}
						saveSettings();
						return true;
					}
					return false;
				});
				list->addItem(clickableListItem2);
			}
			else if (entry_mode == ApmPerformanceMode_Boost) {
				auto *clickableListItem2 = new tsl::elm::ListItem2("ChangeFPSGuiTestListItem"_tr);
				clickableListItem2->setClickListener([](u64 keys) { 
					if ((keys & HidNpadButton_A) && PluginRunning) {
						tsl::changeTo<DockedFPSTargetGui>();
						return true;
					}
					return false;
				});	
				list->addItem(clickableListItem2);			
			}

			auto *clickableListItem4 = new tsl::elm::ListItem2("DisableFPSGuiTestListItem"_tr);
			clickableListItem4->setClickListener([this](u64 keys) {
				if ((keys & HidNpadButton_A) && PluginRunning) {
					if (entry_mode == ApmPerformanceMode_Normal && (Shared -> FPSlocked)) {
						(Shared -> FPSlocked) = 0;
					}
					else if (entry_mode == ApmPerformanceMode_Boost && (Shared -> FPSlockedDocked)) {
						(Shared -> FPSlockedDocked) = 0;
					}
					if ((entry_mode == ApmPerformanceMode_Normal) ? displaySync.ds.handheld : displaySync.ds.docked) {
						if (!oldSalty && R_SUCCEEDED(SaltySD_Connect())) {
							SaltySD_SetDisplayRefreshRate(60);
							SaltySD_Term();
							if (entry_mode == ApmPerformanceMode_Normal) displaySync.ds.handheld = false;
							else if (entry_mode == ApmPerformanceMode_Boost) displaySync.ds.docked = false;
						}
					}
					saveSettings();
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem4);

			auto *clickableListItem3 = new tsl::elm::ListItem2("AdvancedSettingsGuiTestListItem"_tr);
			clickableListItem3->setClickListener([](u64 keys) { 
				if ((keys & HidNpadButton_A) && PluginRunning) {
					tsl::changeTo<AdvancedGui>();
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem3);
		}

		if (SaltySD) {
			auto *clickableListItem6 = new tsl::elm::ListItem2("DisplaySettingsGuiTestListItem"_tr, "\uE151");
			clickableListItem6->setClickListener([](u64 keys) { 
				if (keys & HidNpadButton_A) {
					tsl::changeTo<WarningDisplayGui>();
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem6);
		}

		// Add the list to the frame for it to be drawn
		frame->setContent(list);
		
		// Return the frame to have it become the top level element of this Gui
		return frame;
	}

	// Called once every frame to update values
	virtual void update() override {
		static uint8_t i = 10;

		if (PluginRunning) {
			if (i > 9) {
				switch ((Shared -> FPSmode)) {
					case 0:
						//This is usually a sign that game doesn't use interval
						sprintf(FPSMode_c, "IntervalModeUnusedUpdateGuiTestListItem"_tr.c_str());
						break;
					case 1 ... 5:
						if (std::fmod((double)refreshRate_g, (double)(Shared -> FPSmode)) != 0.0) {
							sprintf(FPSMode_c, "IntervalModeFPSUpdateFGuiTestListItem"_tr.c_str(), (Shared -> FPSmode), (double)refreshRate_g / (Shared -> FPSmode));
						}
						else sprintf(FPSMode_c, "IntervalModeFPSUpdateGuiTestListItem"_tr.c_str(), (Shared -> FPSmode), refreshRate_g / (Shared -> FPSmode));
						break;
					default:
						sprintf(FPSMode_c, "IntervalModeWrongUpdateGuiTestListItem"_tr.c_str(), (Shared -> FPSmode));
				}
				if ((entry_mode == ApmPerformanceMode_Normal) ? !(Shared -> FPSlocked) : !(Shared -> FPSlockedDocked)) {
					sprintf(FPSTarget_c, "CustomFPSDisabledUpdateGuiTestListItem"_tr.c_str());
				}
				else sprintf(FPSTarget_c, "CustomFPSUpdateGuiTestListItem"_tr.c_str(), (entry_mode == ApmPerformanceMode_Normal) ? (Shared -> FPSlocked) : (Shared -> FPSlockedDocked));
				uint8_t value = (Shared -> FPS);
				sprintf(PFPS_c, "%d", value);
				if (value >= 100) render100Above = true;
				else render100Above = false;
				i = 0;
			}
			else i++;
		}
	}

	// Called once every frame to handle inputs not handled by other UI elements
	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		refreshRate_g = *refreshRate_shared;
		if (!isLite) {
			smInitialize();
			if (R_SUCCEEDED(apmInitialize())) {
				ApmPerformanceMode mode = ApmPerformanceMode_Invalid;
				apmGetPerformanceMode(&mode);
				apmExit();
				if (mode != entry_mode) {
					smExit();
					tsl::goBack();
					tsl::changeTo<GuiTest>(0, 1, true);
					return true;
				}
			}
			smExit();
		}
		if (PluginRunning && (Shared -> pluginActive) && !pluginRanAtBoot) {
			tsl::goBack();
			tsl::changeTo<GuiTest>(0, 1, true);
			return true;
		}
		if (SaltySD && plugin && closed && !blocked) {
			blocked = true;
			tsl::goBack();
			tsl::changeTo<GuiTest>(0, 1, true);
			return true;
		}
		if (keysDown & HidNpadButton_B) {
			tsl::goBack();
			tsl::goBack();
			return true;
		}
		return false;   // Return true here to singal the inputs have been consumed
	}
};

class Dummy : public tsl::Gui {
public:
	Dummy(u8 arg1, u8 arg2, bool arg3) {}

	// Called when this Gui gets loaded to create the UI
	// Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
	virtual tsl::elm::Element* createUI() override {
		auto frame = new tsl::elm::OverlayFrame("PluginName"_tr, VERSION);
		return frame;
	}

	// Called once every frame to handle inputs not handled by other UI elements
	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		tsl::changeTo<GuiTest>(0, 1, true);
		return true;   // Return true here to singal the inputs have been consumed
	}
};

class OverlayTest : public tsl::Overlay {
public:
	// libtesla already initialized fs, hid, pl, pmdmnt, hid:sys and set:sys
	virtual void initServices() override {
		std::string jsonStr = R"(
			{
				"PluginName": "FPSLocker",

				"DeleteSettingsNoGameSubListItem": "Delete settings",
				"DeletePatchesNoGameSubListItem": "Delete patches",
				"SaltyNXNotWorkingNoGame2CustomDrawerText": "SaltyNX is not working!",
				"SaltyNXNotFoundNoGame2CustomDrawerText": "Can't detect NX-FPS plugin on sdcard!",
				"GameNotRunningNoGame2CustomDrawerText": "Game is not running!",
				"ErrorNoGame2ListItem": "Err: 0x%x",
				"AllNoGame2ListItem": "All",
				"EverythingNoGame2ListItemText": "Everything",
				"SaltyNXNotWorkingNoGameCustomDrawerText": "SaltyNX is not working!",
				"SaltyNXNotFoundNoGameCustomDrawerText": "Can't detect NX-FPS plugin on sdcard!",
				"GameNotRunningNoGameCustomDrawerText": "Game is not running!",
				"GameListNoGameListItem": "Games list",
				"DisplaySettingsNoGameListItem": "Display settings",
				"ChangeFPSTargetDockedFPSTargetGuiOverlayFrame": "Change FPS Target",
				"SaltNXNotWorkingGuiTestCustomDrawerText": "SaltyNX is not working!",
				"SaltNXNotFoundGuiTestCustomDrawerText": "Can't detect NX-FPS plugin on sdcard!",
				"GameClosedGuiTestCustomDrawerText": "Game was closed! Overlay disabled!",
				"GameNotRunningGuiTestCustomDrawerText": "Game is not running! Overlay disabled!",
				"GameRunningGuiTestCustomDrawerText": "Game is running.",
				"NXFPSNotRunningGuiTestCustomDrawerText": "NX-FPS is not running!",
				"NXFPSRunningNoFrameProcessedGuiTestCustomDrawerText": "NX-FPS is running, but no frame was processed.",
				"RestartOverlayToCheckGuiTestCustomDrawerText": "Restart overlay to check again.",
				"NXFPSRunningGuiTestCustomDrawerText": "NX-FPS is running.",
				"FPSGuiTestCustomDrawerText": "FPS",
				"PatchNowForcing60HzGuiTestCustomDrawerText": "Patch is now forcing 60 Hz.",
				"IncreaseFPSGuiTestListItem": "Increase FPS target",
				"DecreaseFPSGuiTestListItem": "Decrease FPS target",
				"ChangeFPSGuiTestListItem": "Change FPS target",
				"DisableFPSGuiTestListItem": "Disable custom FPS target",
				"AdvancedSettingsGuiTestListItem": "Advanced settings",
				"SaveSettingsGuiTestListItem": "Save settings",
				"DisplaySettingsGuiTestListItem": "Display settings",
				"IntervalModeUnusedUpdateGuiTestListItem": "Interval Mode: 0 [Unused]",
				"IntervalModeFPSUpdateFGuiTestListItem": "Interval Mode: %d [%.1f FPS]",
				"IntervalModeFPSUpdateGuiTestListItem": "Interval Mode: %d [%d FPS]",
				"IntervalModeWrongUpdateGuiTestListItem": "Interval Mode: %d [Wrong]",
				"CustomFPSDisabledUpdateGuiTestListItem": "Custom FPS Target: Disabled",
				"CustomFPSUpdateGuiTestListItem": "Custom FPS Target: %d",

				"SetBufferingSetBuffersOverlayFrame": "Set Buffering",
				"AppliedNextGameBootSetBuffersListItemCategoryHeader": "It will be applied on next game boot.",
				"DoubleSetBuffersListItem": "Double",
				"TripleForceSetBuffersListItem": "Triple [force]",
				"TripleSetBuffersListItem": "Triple",
				"QuadrupleForceSetBuffersListItem": "Quadruple [force]",
				"QuadrupleSetBuffersListItem": "Quadruple",
				"NVNWindowSyncWaitSyncModeOverlayFrame": "NVN Window Sync Wait",
				"SyncModeSyncModeOverlayFrameSubtitle": "Mode",
				"EnabledSyncModeSyncModeListItem": "Enabled",
				"OnSyncModeAdvancedGuiListItemText": "On",
				"SemiEnabledSyncModeSyncModeListItem": "Semi-Enabled",
				"SemiSyncModeAdvancedGuiListItemText": "Semi",
				"DisabledSyncModeSyncModeListItem": "Disabled",
				"OffSyncModeAdvancedGuiListItemText": "Off",
				"ConfigFileNotFoundAdvancedGuiCustomDrawerText": "Game config file not found\nTID: %016lX\nBID: %016lX",
				"ConfigErrorAdvancedGuiCustomDrawerText": "Game config error: 0x%X",
				"PatchFileNotExistAdvancedGuiCustomDrawerText": "Patch file doesn't exist.",
				"PatchFileNotExistMoreAdvancedGuiCustomDrawerText": "Patch file doesn't exist.\nUse \"Convert config to patch file\"\nto make it!",
				"NewConfigDownloadSuccessAdvancedGuiCustomDrawerText": "New config downloaded successfully.\nUse \"Convert config to patch file\"\nto make it applicable!",
				"PatchFileExistAdvancedGuiCustomDrawerText": "Patch file exists.",
				"AdvancedSettingsAdvancedGuiOverlayFrameSubTitle": "Advanced settings",
				"ConfigFileValidAdvancedGuiCustomDrawerText": "Found valid config file!",
				"NVNAdvancedGuiCategoryHeader": "GPU API Interface: NVN",
				"SyncWaitAdvancedGuiListItem": "Window Sync Wait",
				"SetBufferingAdvancedGuiListItem": "Set Buffering",
				"EGLAdvancedGuiCategoryHeader": "GPU API Interface: EGL",
				"VulkanAdvancedGuiCategoryHeader": "GPU API Interface: Vulkan",
				"PatchesAdvancedGuiCategoryHeader": "FPSLocker Patches",
				"PatchWillBeAppliedNextGameBootAdvancedGuiNoteHeader": "Remember to reboot the game after conversion!",
				"ConvertConfigToPatchFileAdvancedGuiListItem": "Convert config to patch file",
				"PatchFileCreatedSuccessAdvancedGuiListItemText": "Patch file created successfully.\nRestart the game and change\nFPS Target to apply the patch!",
				"PatchFileCreateFailedAdvancedGuiListItemText": "Error while creating patch: 0x%x",
				"DeletePatchFileAdvancedGuiListItem": "Delete patch file",
				"DeletePatchSuccessfulFileAdvancedGuiListItemText": "Patch file deleted successfully.",
				"TakesecondsAdvancedGuiListItemText": "This can take up to %d seconds.",
				"CheckOrDownloadConfigFileAdvancedGuiListItemText": "Check/download config file",
				"CheckWarehouseAdvancedGuiListItemText": "Checking Warehouse for config...",
				"MiscAdvancedGuiCategoryHeader": "Misc",
				"HaltUnfocusedGameAdvancedGuiListItemText": "Halt unfocused game",
				"PluginLoadedUpdateAdvancedGuiCustomDrawerText": "Patch was loaded to game.",
				"MasterWriteLoadedUpdateAdvancedGuiCustomDrawerText": "Master Write was loaded to game.",
				"PluginNotApplyUpdateAdvancedGuiCustomDrawerText": "Plugin didn't apply patch to game.",
				"SetOrActiveOrAvailableBuffersUpdateAdvancedGuiCustomDrawerText": "Set/Active/Available buffers: %d/%d/%d",
				"ActiveBuffersUpdateAdvancedGuiCustomDrawerText": "Active buffers: %d",
				"ConnectionTimeoutAdvancedGuiListItemText": "Connection timeout!",
				"ConfigNotAvailableAdvancedGuiListItemText": "Config is not available! RC: 0x%x",
				"ConfigNotAvailable404AdvancedGuiListItemText": "Config is not available!\nChecking Warehouse for more info...",
				"ConfigNotAvailableTimeoutAdvancedGuiListItemText": "Config is not available!\nChecking Warehouse for more info...\nTimeout! It took too long to check.",
				"ConfigNotAvailableConnectionErrorAdvancedGuiListItemText": "Config is not available!\nChecking Warehouse for more info...\nConnection error!",
				"NoNewConfigAdvancedGuiListItemText": "No new config available.",
				"InternetConnectionNotAvailableAdvancedGuiListItemText": "Internet connection not available!",
				"PatchNotNeededAdvancedGuiListItemText": "Patch is not needed for this game!",
				"NotListedInWarehouseAdvancedGuiListItemText": "This game is not listed in Warehouse!",
				"DiffVersionListedInWarehouseNoNeedPatchAdvancedGuiListItemText": "This game is listed in Warehouse,\nbut with different version.\n%s doesn't need a patch,\nyour version maybe doesn't need it too!",
				"DiffVersionListedInWarehouseNoPatchAdvancedGuiListItemText": "This game is listed in Warehouse,\nbut with different version.\n%s recommends patch,\nbut config is not available even for it!",
				"DiffVersionListedInWarehouseOtherVersionAvailableAdvancedGuiListItemText": "This game is listed in Warehouse,\nbut with different version.\n%s has config available!",
				"ListedInWarehousePatchNotAvailableAdvancedGuiListItemText": "This game is listed in Warehouse\nwith patch recommended for this\nversion, but config is not available!",
				"ConnectionErrorAdvancedGuiListItemText": "Connection error! RC: 0x%x",

				"FrameskipTesterDockedFrameskipGuiOverlayFrame": "Frameskip tester",
				"HowToUseDockedFrameskipGuiCustomDrawerText": "How to use it:\n1. Get a camera with options to manually set shutter speed and ISO.\n2. Set shutter speed to 1/10s or longer, and ISO so it's not too bright or dark (usually around 50 for 1/10s).\n3. Press \uE0E0 to continue.\n4. Take picture of display.\n5. If all blocks except for first and last are unevenly bright, your display doesn't support natively your current refresh rate and it's running at some other refresh rate.\n\nStill take into consideration that even if your display is frameskipping, it still works miles better than using lower FPS target that doesn't match your refresh rate because hardware solution is the best way to divide evenly frametimes.",
				"ExitHintDockedFrameskipGuiCustomDrawerText": "Press \uE0E1 to exit",
				"RenderingTooLongHintDockedFrameskipGuiCustomDrawerText": "Rendering takes too long!\nClose game, go to home screen,\ntry again.",
				"DisplayUnderclockWizardDockedWizardGuiOverlayFrame": "Display underclock wizard",
				"MenuHelpHintDockedWizardGuiCustomDrawerText": "This menu will go through all\nsupported refresh rates below 60 Hz:\n40, 45, 50, 55. Press button you are\nasked for to confirm that it works.\nIf nothing is pressed in 15 seconds,\nit will check next refresh rate.",
				"StartHintDockedWizardGuiCustomDrawerText": "To start press X.",
				"NotSupportAtResolutionDockedWizardGuiCustomDrawerText": "Not supported at %dp!",
				"CloseGameFirstDockedWizardGuiCustomDrawerText": "Close game first!",
				"CheckZL40HzWorkingStatusHintDockedWizardGuiCustomDrawerText": "Press ZL to confirm 40 Hz is working.",
				"CheckXWorkingStatusHintDockedWizardGuiCustomDrawerText": "Press X to confirm %d Hz is working.",
				"CheckYWorkingStatusHintDockedWizardGuiCustomDrawerText": "Press Y to confirm %d Hz is working.",
				"CheckZRzWorkingStatusHintDockedWizardGuiCustomDrawerText": "Press ZR to confirm %d Hz is working.",
				"PressButtonHintDockedOverWizardGuiOverlayFrameCustomDrawerText": "This menu will go through all\nsupported refresh rates above 60 Hz\nup to %d Hz.\n\nPress button you are asked for\nto confirm that it works.\nIf nothing is pressed in 10 seconds,\nit will check next refresh rate.\nThis can take up to %d seconds.",
				"DisplayOverclockWizardDockedOverWizardGuiOverlayFrame": "Display overclock wizard",
				"PressButtonDockedOverWizardGuiOverlayFrameCustomDrawerText": "To start press X.",
				"PressButtonNotSupportAtResolutionDockedOverWizardGuiOverlayFrameCustomDrawerText": "Not supported at %dp!",
				"PressButtonCloseGameFirstDockedOverWizardGuiOverlayFrameCustomDrawerText": "Close game first!",
				"PressButtonZLConfirmFreqWorkingDockedOverWizardGuiOverlayFrameCustomDrawerText": "Press ZL to confirm %d Hz is working.",
				"PressButtonXConfirmFreqWorkingDockedOverWizardGuiOverlayFrameCustomDrawerText": "Press X to confirm %d Hz is working.",
				"PressButtonYConfirmFreqWorkingDockedOverWizardGuiOverlayFrameCustomDrawerText": "Press Y to confirm %d Hz is working.",
				"PressButtonZRConfirmFreqWorkingDockedOverWizardGuiOverlayFrameCustomDrawerText": "Press ZR to confirm %d Hz is working.",
				"DockedDisplayManualSettingsDockedManualGuiOverlayFrame": "Docked %dp display manual settings",
				"DockedDisplayAdditionalSettingsDockedAdditionalGuiOverlayFrame": "Docked display additional settings",
				"AllowPatchesForce60HzDockedAdditionalGuiToggleListItem": "Allow patches to force 60 Hz",
				"UseLowestRefreshRateForUnmatchedDockedAdditionalGuiToggleListItem": "Use lowest refresh rate for unmatched FPS targets",
				"60HzInHomeMenuDockedAdditionalGuiToggleListItem": "60 Hz in HOME Menu",
				"MaxRefreshRateDockedGuiCustomDrawerText": "Max refresh rate available: %u Hz\nmyDP link rate: %s\nConfig ID: %08X",
				"NDDockedGuiCustomDrawerText": "n/d",
				"DockedDisplaySettingsDockedGuiOverlayFrame": "Docked display settings",
				"AllowedRefreshRatesDockedGuiListItem": "Allowed %dp refresh rates",
				"DisplayUnderclockWizardDockedGuiListItem": "Display underclock wizard",
				"OverclockWizardDockedGuiListItem": "%dp overclock wizard",
				"FrameskipTesterDockedGuiListItem": "Frameskip tester",
				"AdditionalSettingsDockedGuiListItem": "Additional settings",
				"HintUpdateDockedGuiCustomDrawerText": "You are not in docked mode.\nGo back, put your Switch to dock\nand come back.",
				"ChangeRefreshRateDockedRefreshRateChangeGuiOverlayFrame": "Change Refresh Rate",
				"DisplayGuiOverlayFrameText": "Display settings",
				"IncreaseRefreshRateDisplayGuiListItem": "Increase Refresh Rate",
				"DecreaseRefreshRateDisplayGuiListItem": "Decrease Refresh Rate",
				"ChangeRefreshRateDisplayGuiListItem": "Change Refresh Rate",
				"MatchRefreshRateDisplayGuiCategoryHeader": "Match refresh rate with FPS Target.",
				"DisplaySyncDisplayGuiToggleListItem": "Display Sync",
				"DockedDisplaySyncDisplayGuiToggleListItem": "Docked Display Sync",
				"DockedSettingsDisplayGuiToggleListItem": "Docked Settings",
				"RetroRemakeModeDisplayGuiToggleListItem": "Retro Remake Mode",
				"HandheldOnlyDockedAdditionalGuiToggleListItem": "Handheld only",
				"UpdateDisplayRefreshRateDisplayGuiCustomDrawerText": "Display Refresh Rate: %d Hz",
				"WarningWarningDisplayGuiCustomDrawerText": "THIS IS EXPERIMENTAL FUNCTION!\n\nIt can cause irreparable damage\nto your display.\n\nBy pressing Accept you are taking\nfull responsibility for anything\nthat can occur because of this tool.",
				"DisplaySettingWarningWarningDisplayGuiOverlayFrame": "Display settings warning",
				"DeclineWarningDisplayGuiListItem": "Decline",
				"AcceptWarningDisplayGuiListItem": "Accept"
			}
		)";
		std::string lanPath = std::string("sdmc:/switch/.overlays/lang/") + APPTITLE + "/";
		fsdevMountSdmc();
		tsl::hlp::doWithSmSession([&lanPath, &jsonStr]{
			tsl::tr::InitTrans(lanPath, jsonStr);
		});

		#if !defined(__SWITCH__) && !defined(__OUNCE__)
			systemtickfrequency = armGetSystemTickFreq();
		#endif
		tsl::hlp::doWithSmSession([]{
			nsInitialize();
			ommInitialize();
			setsysInitialize();
			SetSysProductModel model;
			if (R_SUCCEEDED(setsysGetProductModel(&model))) {
				if (model == SetSysProductModel_Aula) {
					isOLED = true;
				}
				else if (model == SetSysProductModel_Hoag) {
					isLite = true;
				}
			}
			setsysExit();
			if (file_exists("sdmc:/SaltySD/flags/displaysync.flag")) {
				displaySync.ds.handheld = true;
			}
			if (file_exists("sdmc:/SaltySD/flags/displaysyncdocked.flag")) {
				displaySync.ds.docked = true;
			}
			if (file_exists("sdmc:/SaltySD/flags/displaysync_outoffocus.flag")) {
				displaySyncOutOfFocus60 = true;
			}
			SaltySD = CheckPort();
			if (!SaltySD) return;

			if (R_SUCCEEDED(SaltySD_Connect())) {
				uint8_t refreshRate_temp = 0;
				if (R_FAILED(SaltySD_GetDisplayRefreshRate(&refreshRate_temp))) {
					refreshRate_g = 60;
					oldSalty = true;
				}
				else refreshRate_g = refreshRate_temp;
				svcSleepThread(100'000);
				SaltySD_Term();
			}

			if(!LoadSharedMemory()) return;
			uintptr_t base = (uintptr_t)shmemGetAddr(&_sharedmemory);
			refreshRate_shared = (uint8_t*)(base + 1);

			if (R_FAILED(pmdmntGetApplicationProcessId(&PID))) return;
			check = true;

			ptrdiff_t rel_offset = searchSharedMemoryBlock(base);
			if (rel_offset > -1) {
				Shared = (NxFpsSharedBlock*)(base + rel_offset);
			}

			if (!PluginRunning) {
				if (rel_offset > -1) {
					pminfoInitialize();
					pminfoGetProgramId(&TID, PID);
					pminfoExit();
					BID = getBID();
					sprintf(patchPath, "sdmc:/SaltySD/plugins/FPSLocker/patches/%016lX/%016lX.bin", TID, BID);
					sprintf(configPath, "sdmc:/SaltySD/plugins/FPSLocker/patches/%016lX/%016lX.yaml", TID, BID);
					sprintf(savePath, "sdmc:/SaltySD/plugins/FPSLocker/%016lX.dat", TID);

					SetBuffers_save = (Shared -> SetBuffers);
					forceSuspend_save = (Shared -> forceSuspend);
					PluginRunning = true;
					threadCreate(&t0, loopThread, NULL, NULL, 0x1000, 0x20, 0);
					threadStart(&t0);
				}		
			}
		
		});
	
	}  // Called at the start to initialize all services necessary for this Overlay
	
	virtual void exitServices() override {
		leventSignal(&threadexit);
		threadWaitForExit(&t1);
		threadClose(&t1);
		threadWaitForExit(&t0);
		threadClose(&t0);
		shmemClose(&_sharedmemory);
		nsExit();
		ommExit();
		fsdevUnmountDevice("sdmc");
	}  // Callet at the end to clean up all services previously initialized

	virtual void onShow() override {}    // Called before overlay wants to change from invisible to visible state
	
	virtual void onHide() override {}    // Called before overlay wants to change from visible to invisible state

	virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
		if (SaltySD && check && plugin) {
			return initially<Dummy>(1, 2, true);  // Initial Gui to load. It's possible to pass arguments to it's constructor like this
		}
		else {
			mutexInit(&TitlesAccess);
			threadCreate(&t1, TitlesThread, NULL, NULL, 0x1000, 0x10, -2);
			threadStart(&t1);
			if (oldSalty || !SaltySD)
				return initially<NoGame2>(0, 2, true);
			else return initially<NoGame>(0, 2, true);
		}
	}
};

int main(int argc, char **argv) {
	return tsl::loop<OverlayTest>(argc, argv);
}
