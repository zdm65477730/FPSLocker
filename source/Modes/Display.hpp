class DockedFrameskipGui : public tsl::Gui {
public:
	int y_1;
	int x_1;
	int height = 480;
	int block_width = 20;
	int block_height = 40;
	int columns = 18;
	bool block;
	uint64_t tick;
	bool state;
	DockedFrameskipGui() {
		y_1 = 0;
		x_1 = 0;
		tick = 0;
		block = false;
		state = false;
	}

	virtual tsl::elm::Element* createUI() override {
		auto frame = new tsl::elm::OverlayFrame("PluginName"_tr, "FrameskipTesterDockedFrameskipGuiOverlayFrame"_tr);
		auto list = new tsl::elm::List();
		list->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
			if (!state) {
				renderer->drawString("HowToUseDockedFrameskipGuiCustomDrawerText"_tr.c_str(), false, x, y+20, 20, renderer->a(0xFFFF));
			}
			else if (!block) {
				renderer->fillScreen(0xF000);
				renderer->drawRect(x+x_1, y+y_1, block_width, block_height, renderer->a(0xFFFF));
				renderer->drawString("ExitHintDockedFrameskipGuiCustomDrawerText"_tr.c_str(), false, x+20, y+height+20, 20, renderer->a(0xFFFF));
			}
			else {
				renderer->drawString("RenderingTooLongHintDockedFrameskipGuiCustomDrawerText"_tr.c_str(), false, x, y+20, 20, renderer->a(0xFFFF));
			}
			
		}), height+40);		
		
		frame->setContent(list);
		return frame;
	}

	// Called once every frame to handle inputs not handled by other UI elements
	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		if (!state && (keysDown & HidNpadButton_A)) {
			state = true;
			return true;
		}
		if (state && !block) {
			y_1 += block_height;
			y_1 %= height;
			if (y_1 == 0) {
				x_1 += block_width;
				x_1 %= (block_width*columns);
			}
			if (!tick) {
				tick = svcGetSystemTick();
				return false;
			}
			if ((svcGetSystemTick() - tick) > ((19200000 / *refreshRate_shared) * 2)) {
				block = true;
			}
			else tick = svcGetSystemTick();
		}
		return false;   // Return true here to singal the inputs have been consumed
	}
};

class DockedManualGui;

class DockedWizardGui : public tsl::Gui {
public:
	uint64_t tick;
	size_t i;
	char Docked_c[256] = "";
	char PressButton[40] = "";
	DockedModeRefreshRateAllowed rr;
	DockedModeRefreshRateAllowed rr_default;
	DockedAdditionalSettings as;
	uint8_t highestRefreshRate = 60;
	DockedWizardGui(uint8_t highestRefreshRate_impl) {
		if (highestRefreshRate_impl >= 70) highestRefreshRate = highestRefreshRate_impl;
		LoadDockedModeAllowedSave(rr_default, as, nullptr);
		memcpy(&rr, &rr_default, sizeof(rr));
		memset(&rr, 1, 5);
		tick = 0;
		i = 0;
	}

	virtual tsl::elm::Element* createUI() override {
		auto frame = new tsl::elm::OverlayFrame("PluginName"_tr, "DisplayUnderclockWizardDockedWizardGuiOverlayFrame"_tr);

		auto list = new tsl::elm::List();

		list->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
			snprintf(Docked_c, sizeof(Docked_c), "MenuHelpHintDockedWizardGuiCustomDrawerText"_tr.c_str());
			renderer->drawString(Docked_c, false, x, y+20, 20, renderer->a(0xFFFF));
			snprintf(PressButton, sizeof(PressButton), "StartHintDockedWizardGuiCustomDrawerText"_tr.c_str());
			renderer->drawString(PressButton, false, x, y+160, 20, renderer->a(0xFFFF));
			
		}), 200);		
		
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
		if (keysHeld & HidNpadButton_B) {
			if (R_SUCCEEDED(SaltySD_Connect())) {
				SaltySD_SetAllowedDockedRefreshRates(rr_default);
				remove("sdmc:/SaltySD/test.flag");
				svcSleepThread(100'000);
				if (tick) {
					SaltySD_SetDisplayRefreshRate(60);
					svcSleepThread(100'000);
				}
				SaltySD_Term();
			}
			tsl::goBack();
			return true;
		}
		s32 width = 0;
		s32 height = 0;
		if (R_SUCCEEDED(ommGetDefaultDisplayResolution(&width, &height))) {
			if (height != 720 && height != 1080) {
				snprintf(PressButton, sizeof(PressButton), "NotSupportAtResolutionDockedWizardGuiCustomDrawerText"_tr.c_str(), (uint16_t)height);
				return true;
			}
		}
		if (check && PluginRunning && (Shared -> pluginActive)) {
				snprintf(PressButton, sizeof(PressButton), "CloseGameFirstDockedWizardGuiCustomDrawerText"_tr.c_str());
				return true;
		}
		static u64 keyCheck = HidNpadButton_ZL;
		if ((keysHeld & HidNpadButton_X) && !tick) {
			tick = svcGetSystemTick();
			if (R_SUCCEEDED(SaltySD_Connect())) {
				SaltySD_SetAllowedDockedRefreshRates(rr);
				FILE* file = fopen("sdmc:/SaltySD/test.flag", "wb");
				if (file) fclose(file);
				svcSleepThread(100'000);
				SaltySD_SetDisplayRefreshRate(40);
				svcSleepThread(100'000);
				SaltySD_Term();
				snprintf(PressButton, sizeof(PressButton), "CheckZL40HzWorkingStatusHintDockedWizardGuiCustomDrawerText"_tr.c_str());
			}
			return true;
		}
		if (tick) {
			if (i > 3) {
				if (R_SUCCEEDED(SaltySD_Connect())) {
					SaltySD_SetAllowedDockedRefreshRates(rr);
					remove("sdmc:/SaltySD/test.flag");
					svcSleepThread(100'000);
					SaltySD_Term();
				}
				SaveDockedModeAllowedSave(rr, as);

				tsl::goBack();
				tsl::changeTo<DockedManualGui>(highestRefreshRate);
				return true;
			}
			if (svcGetSystemTick() - tick < (15 * 19200000)) {
				if (keysHeld & keyCheck) {
					rr[i] = true;
					i++;
					if (R_SUCCEEDED(SaltySD_Connect())) {
						SaltySD_SetDisplayRefreshRate(DockedModeRefreshRateAllowedValues[i]);
						svcSleepThread(100'000);
						SaltySD_Term();
					}
					if (i % 1 == 0) {
						keyCheck = HidNpadButton_X;
						snprintf(PressButton, sizeof(PressButton), "CheckXWorkingStatusHintDockedWizardGuiCustomDrawerText"_tr.c_str(), DockedModeRefreshRateAllowedValues[i]);
					}
					if (i % 3 == 0) {
						keyCheck = HidNpadButton_Y;
						snprintf(PressButton, sizeof(PressButton), "CheckYWorkingStatusHintDockedWizardGuiCustomDrawerText"_tr.c_str(), DockedModeRefreshRateAllowedValues[i]);
					}
					if (i % 2 == 0) {
						keyCheck = HidNpadButton_ZR;
						snprintf(PressButton, sizeof(PressButton), "CheckZRzWorkingStatusHintDockedWizardGuiCustomDrawerText"_tr.c_str(), DockedModeRefreshRateAllowedValues[i]);
					}
					tick = svcGetSystemTick();
					return true;
				}
			}
			else {
				rr[i] = false;
				i++;
				if (i % 1 == 0) {
					keyCheck = HidNpadButton_X;
					snprintf(PressButton, sizeof(PressButton), "CheckXWorkingStatusHintDockedWizardGuiCustomDrawerText"_tr.c_str(), DockedModeRefreshRateAllowedValues[i]);
				}
				if (i % 3 == 0) {
					keyCheck = HidNpadButton_Y;
					snprintf(PressButton, sizeof(PressButton), "CheckYWorkingStatusHintDockedWizardGuiCustomDrawerText"_tr.c_str(), DockedModeRefreshRateAllowedValues[i]);
				}
				if (i % 2 == 0) {
					keyCheck = HidNpadButton_ZR;
					snprintf(PressButton, sizeof(PressButton), "CheckZRzWorkingStatusHintDockedWizardGuiCustomDrawerText"_tr.c_str(), DockedModeRefreshRateAllowedValues[i]);
				}
				if (R_SUCCEEDED(SaltySD_Connect())) {
					SaltySD_SetDisplayRefreshRate(DockedModeRefreshRateAllowedValues[i]);
					svcSleepThread(100'000);
					SaltySD_Term();
				}
				tick = svcGetSystemTick();
			}
		}
		return false;   // Return true here to singal the inputs have been consumed
	}
};

class DockedOverWizardGui : public tsl::Gui {
public:
	uint64_t tick;
	size_t i;
	char Docked_c[384] ="";
		char PressButton[40] = "";
	DockedModeRefreshRateAllowed rr;
	DockedModeRefreshRateAllowed rr_default;
	DockedAdditionalSettings as;
	uint8_t m_maxRefreshRate;
	uint16_t delay_s = 10;
	bool block = false;
	DockedOverWizardGui(uint8_t maxRefreshRate) {
		if (maxRefreshRate > DockedModeRefreshRateAllowedValues[sizeof(DockedModeRefreshRateAllowedValues) - 1])
			maxRefreshRate = DockedModeRefreshRateAllowedValues[sizeof(DockedModeRefreshRateAllowedValues) - 1];
		m_maxRefreshRate = maxRefreshRate;
		LoadDockedModeAllowedSave(rr_default, as, nullptr);
		memcpy(&rr, &rr_default, sizeof(rr));
		tick = 0;
		i = 5;
		uint8_t times = 0;
		for (size_t x = 5; x < sizeof(DockedModeRefreshRateAllowedValues); x++) {
			if (DockedModeRefreshRateAllowedValues[i] > maxRefreshRate)
				rr[x] = false;
			else {
				rr[x] = true;
				times++;
			}
		}
		snprintf(Docked_c, sizeof(Docked_c), "PressButtonHintDockedOverWizardGuiOverlayFrameCustomDrawerText"_tr.c_str(), maxRefreshRate, delay_s * times);
	}

	virtual tsl::elm::Element* createUI() override {
		auto frame = new tsl::elm::OverlayFrame("PluginName"_tr, "DisplayOverclockWizardDockedOverWizardGuiOverlayFrame"_tr);

		auto list = new tsl::elm::List();

		list->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {

			renderer->drawString(Docked_c, false, x, y+20, 20, renderer->a(0xFFFF));
						snprintf(PressButton, sizeof(PressButton), "PressButtonDockedOverWizardGuiOverlayFrameCustomDrawerText"_tr.c_str());
			renderer->drawString(PressButton, false, x, y+260, 20, renderer->a(0xFFFF));

		}), 270);

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
			s32 width = 0;
			s32 height = 1080;
			if (tick) ommGetDefaultDisplayResolution(&width, &height);
			if (height != 1080 || mode != ApmPerformanceMode_Boost) {
				smExit();
				tsl::goBack();
				return true;
			}
		}
		smExit();
		if (keysHeld & HidNpadButton_B) {
			if (R_SUCCEEDED(SaltySD_Connect())) {
				SaltySD_SetAllowedDockedRefreshRates(rr_default);
				remove("sdmc:/SaltySD/test.flag");
				svcSleepThread(100'000);
				if (tick) {
					SaltySD_SetDisplayRefreshRate(60);
					svcSleepThread(100'000);
				}
				SaltySD_Term();
			}
			tsl::goBack();
			return true;
		}
		s32 width = 0;
		s32 height = 0;
		if (R_SUCCEEDED(ommGetDefaultDisplayResolution(&width, &height))) {
			if (height != 1080) {
				snprintf(PressButton, sizeof(PressButton), "PressButtonNotSupportAtResolutionDockedOverWizardGuiOverlayFrameCustomDrawerText"_tr.c_str(), (uint16_t)height);
				return true;
			}
		}
		if (check && PluginRunning && (Shared -> pluginActive)) {
				snprintf(PressButton, sizeof(PressButton), "PressButtonCloseGameFirstDockedOverWizardGuiOverlayFrameCustomDrawerText"_tr.c_str());
				return true;
		}
		static u64 keyCheck = HidNpadButton_ZL;
		if ((keysHeld & HidNpadButton_X) && !tick) {
			tick = svcGetSystemTick();
			if (R_SUCCEEDED(SaltySD_Connect())) {
				SaltySD_SetAllowedDockedRefreshRates(rr);
				FILE* file = fopen("sdmc:/SaltySD/test.flag", "wb");
				if (file) fclose(file);
				svcSleepThread(100'000);
				SaltySD_SetDisplayRefreshRate(DockedModeRefreshRateAllowedValues[i]);
				svcSleepThread(100'000);
				SaltySD_Term();
				snprintf(PressButton, sizeof(PressButton), "PressButtonZLConfirmFreqWorkingDockedOverWizardGuiOverlayFrameCustomDrawerText"_tr.c_str(), DockedModeRefreshRateAllowedValues[i]);
			}
			return true;
		}
		if (tick) {
			if (i > sizeof(DockedModeRefreshRateAllowedValues)) {
				if (R_SUCCEEDED(SaltySD_Connect())) {
					SaltySD_SetAllowedDockedRefreshRates(rr);
					remove("sdmc:/SaltySD/test.flag");
					svcSleepThread(100'000);
					SaltySD_SetDisplayRefreshRate(60);
					svcSleepThread(100'000);
					SaltySD_Term();
				}
				SaveDockedModeAllowedSave(rr, as);
				tsl::goBack();
				tsl::changeTo<DockedManualGui>(m_maxRefreshRate);
				return true;
			}
			if (svcGetSystemTick() - tick < (delay_s * 19200000)) {
				if (keysHeld & keyCheck) {
					rr[i] = true;
					i++;
					if (R_SUCCEEDED(SaltySD_Connect())) {
						Result rc = SaltySD_SetDisplayRefreshRate(DockedModeRefreshRateAllowedValues[i]);
						svcSleepThread(100'000);
						if (R_FAILED(rc)) {
							while(i < sizeof(DockedModeRefreshRateAllowedValues) && DockedModeRefreshRateAllowedValues[i] <= m_maxRefreshRate) {
								rr[i] = false;
								i++;
								tick = svcGetSystemTick();
								svcSleepThread(100'000);
								if (R_SUCCEEDED(SaltySD_SetDisplayRefreshRate(DockedModeRefreshRateAllowedValues[i])))
									break;
							}
							SaltySD_Term();
							return true;
						}
						else SaltySD_Term();
					}
					if (i % 1 == 0) {
						keyCheck = HidNpadButton_X;
						snprintf(PressButton, sizeof(PressButton), "PressButtonXConfirmFreqWorkingDockedOverWizardGuiOverlayFrameCustomDrawerText"_tr.c_str(), DockedModeRefreshRateAllowedValues[i]);
					}
					if (i % 3 == 0) {
						keyCheck = HidNpadButton_Y;
						snprintf(PressButton, sizeof(PressButton), "PressButtonYConfirmFreqWorkingDockedOverWizardGuiOverlayFrameCustomDrawerText"_tr.c_str(), DockedModeRefreshRateAllowedValues[i]);
					}
					if (i % 2 == 0) {
						keyCheck = HidNpadButton_ZR;
						snprintf(PressButton, sizeof(PressButton), "PressButtonZRConfirmFreqWorkingDockedOverWizardGuiOverlayFrameCustomDrawerText"_tr.c_str(), DockedModeRefreshRateAllowedValues[i]);
					}
					tick = svcGetSystemTick();
					return true;
				}
			}
			else {
				rr[i] = false;
				i++;
				if (i % 1 == 0) {
					keyCheck = HidNpadButton_X;
					snprintf(PressButton, sizeof(PressButton), "PressButtonXConfirmFreqWorkingDockedOverWizardGuiOverlayFrameCustomDrawerText"_tr.c_str(), DockedModeRefreshRateAllowedValues[i]);
				}
				if (i % 3 == 0) {
					keyCheck = HidNpadButton_Y;
					snprintf(PressButton, sizeof(PressButton), "PressButtonYConfirmFreqWorkingDockedOverWizardGuiOverlayFrameCustomDrawerText"_tr.c_str(), DockedModeRefreshRateAllowedValues[i]);
				}
				if (i % 2 == 0) {
					keyCheck = HidNpadButton_ZR;
					snprintf(PressButton, sizeof(PressButton), "PressButtonZRConfirmFreqWorkingDockedOverWizardGuiOverlayFrameCustomDrawerText"_tr.c_str(), DockedModeRefreshRateAllowedValues[i]);
				}
				if (R_SUCCEEDED(SaltySD_Connect())) {
					SaltySD_SetDisplayRefreshRate(DockedModeRefreshRateAllowedValues[i]);
					svcSleepThread(100'000);
					SaltySD_Term();
				}
				tick = svcGetSystemTick();
			}
		}
		return false;   // Return true here to singal the inputs have been consumed
	}
};

class DockedManualGui : public tsl::Gui {
public:
	uint32_t crc = 0;
	DockedModeRefreshRateAllowed rr = {0};
	DockedAdditionalSettings as;
	uint8_t maxRefreshRate = 60;
	DockedManualGui(uint8_t maxRefreshRate_impl) {
		if (maxRefreshRate_impl >= 70) maxRefreshRate = maxRefreshRate_impl;
		LoadDockedModeAllowedSave(rr, as, nullptr);
	}

	virtual tsl::elm::Element* createUI() override {
		auto frame = new tsl::elm::OverlayFrame("PluginName"_tr, "Docked1080pDisplayManualSettingsDockedManualGuiOverlayFrame"_tr);

		auto list = new tsl::elm::List();

		for (size_t i = 0; i < 4; i++) {
			char Hz[] = "120 Hz";
			snprintf(Hz, sizeof(Hz), "%d Hz", DockedModeRefreshRateAllowedValues[i]);
			auto *clickableListItem = new tsl::elm::ToggleListItem(Hz, rr[i]);
			clickableListItem->setClickListener([this, i](u64 keys) { 
				if (keys & HidNpadButton_A) {
					rr[i] = !rr[i];
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem);
		}
		for (size_t i = 5; i < sizeof(DockedModeRefreshRateAllowedValues); i++) {
			if (maxRefreshRate < DockedModeRefreshRateAllowedValues[i]) break;
			char Hz[] = "120 Hz";
			snprintf(Hz, sizeof(Hz), "%d Hz", DockedModeRefreshRateAllowedValues[i]);
			auto *clickableListItem = new tsl::elm::ToggleListItem(Hz, rr[i]);
			clickableListItem->setClickListener([this, i](u64 keys) { 
				if (keys & HidNpadButton_A) {
					rr[i] = !rr[i];
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem);	
		}

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
				tsl::goBack();
				return true;
			}
		}
		smExit();
		if (keysHeld & HidNpadButton_B) {
			if (R_SUCCEEDED(SaltySD_Connect())) {
				SaveDockedModeAllowedSave(rr, as);
				SaltySD_SetAllowedDockedRefreshRates(rr);
				svcSleepThread(100'000);
				SaltySD_Term();
			}
			tsl::goBack();
			return true;
		}
		return false;   // Return true here to singal the inputs have been consumed
	}
};

class DockedAdditionalGui : public tsl::Gui {
public:
	uint32_t crc = 0;
	DockedModeRefreshRateAllowed rr = {0};
	DockedAdditionalSettings as;
	DockedAdditionalGui() {
		LoadDockedModeAllowedSave(rr, as, nullptr);
	}

	virtual tsl::elm::Element* createUI() override {
		auto frame = new tsl::elm::OverlayFrame("PluginName"_tr, "DockedDisplayAdditionalSettingsDockedAdditionalGuiOverlayFrame"_tr);
		auto list = new tsl::elm::List();
		auto *clickableListItem4 = new tsl::elm::ToggleListItem("AllowPatchesForce60HzDockedAdditionalGuiToggleListItem"_tr, !as.dontForce60InDocked);
		clickableListItem4->setClickListener([this](u64 keys) { 
			if (keys & HidNpadButton_A) {
				as.dontForce60InDocked = !as.dontForce60InDocked;
				if (R_SUCCEEDED(SaltySD_Connect())) {
					SaltySD_SetDontForce60InDocked(as.dontForce60InDocked);
					SaltySD_Term();
				}
				SaveDockedModeAllowedSave(rr, as);
				return true;
			}
			return false;
		});
		list->addItem(clickableListItem4);
		auto *clickableListItem5 = new tsl::elm::ToggleListItem("UseLowestRefreshRateForUnmatchedDockedAdditionalGuiToggleListItem"_tr, as.fpsTargetWithoutRRMatchLowest);
		clickableListItem5->setClickListener([this](u64 keys) { 
			if (keys & HidNpadButton_A) {
				as.fpsTargetWithoutRRMatchLowest = !as.fpsTargetWithoutRRMatchLowest;
				if (R_SUCCEEDED(SaltySD_Connect())) {
					SaltySD_SetMatchLowestRR(as.fpsTargetWithoutRRMatchLowest);
					SaltySD_Term();
				}
				SaveDockedModeAllowedSave(rr, as);
				return true;
			}
			return false;
		});
		list->addItem(clickableListItem5);
		
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
				tsl::goBack();
				return true;
			}
		}
		smExit();
		return false;   // Return true here to singal the inputs have been consumed
	}
};

class DockedGui : public tsl::Gui {
private:
	char Docked_c[256] = "";
	DockedModeRefreshRateAllowed rr;
	DockedAdditionalSettings as;
	uint8_t highestRefreshRate;
	uint8_t linkRate;
public:
	DockedGui() {
		mkdir("sdmc:/SaltySD/plugins/FPSLocker/", 777);
		mkdir("sdmc:/SaltySD/plugins/FPSLocker/ExtDisplays/", 777);
		int crc32 = 0;
		LoadDockedModeAllowedSave(rr, as, &crc32);
		highestRefreshRate = 60;
		linkRate = 10;
		getDockedHighestRefreshRate(&highestRefreshRate, &linkRate);
		char linkMode[5] = "HBR";
		if (linkRate == 30) strcpy(linkMode, "HBR3");
		else if (linkRate == 20) strcpy(linkMode, "HBR2");
		else if (linkRate == 6) strcpy(linkMode, "RBR");
		else if (linkRate != 10) strcpy(linkMode, "n/d");
		snprintf(Docked_c, sizeof(Docked_c), "MaxRefreshRateDockedGuiCustomDrawerText"_tr.c_str(), highestRefreshRate, linkMode, crc32);
	}
	size_t base_height = 128;
	bool block = false;

	virtual tsl::elm::Element* createUI() override {
		auto frame = new tsl::elm::OverlayFrame("PluginName"_tr, "DockedDisplaySettingsDockedGuiOverlayFrame"_tr);
		auto list = new tsl::elm::List();
		list->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
			renderer->drawString(Docked_c, false, x, y+20, 20, renderer->a(0xFFFF));

		if (!block) {
				if (linkRate < 20) renderer->drawString("\uE14C", false, x+220, y+40, 20, renderer->a(0xF00F));
				else renderer->drawString("\uE14B", false, x+220, y+40, 20, renderer->a(0xF0F0));
			}
		}), 85);

		auto *clickableListItem1 = new tsl::elm::ListItem2("Allowed1080pRefreshRatesDockedGuiListItem"_tr);
		clickableListItem1->setClickListener([this](u64 keys) { 
			if ((keys & HidNpadButton_A) && !block) {
				tsl::changeTo<DockedManualGui>(highestRefreshRate);
				return true;
			}
			return false;
		});

		list->addItem(clickableListItem1);

		auto *clickableListItem2 = new tsl::elm::ListItem2("DisplayUnderclockWizardDockedGuiListItem"_tr);
		clickableListItem2->setClickListener([this](u64 keys) { 
			if ((keys & HidNpadButton_A) && !block) {
				tsl::changeTo<DockedWizardGui>(highestRefreshRate);
				return true;
			}
			return false;
		});

		list->addItem(clickableListItem2);

		if (highestRefreshRate >= 70.f) {
			auto *clickableListItem22 = new tsl::elm::ListItem2("1080pOverclockWizardDockedGuiListItem"_tr);
			clickableListItem22->setClickListener([this](u64 keys) { 
				if ((keys & HidNpadButton_A) && !block) {
					tsl::changeTo<DockedOverWizardGui>((uint8_t)std::round(highestRefreshRate));
					return true;
				}
				return false;
			});

			list->addItem(clickableListItem22);
		}

		auto *clickableListItem3 = new tsl::elm::ListItem2("FrameskipTesterDockedGuiListItem"_tr);
		clickableListItem3->setClickListener([this](u64 keys) { 
			if ((keys & HidNpadButton_A)) {
				tsl::changeTo<DockedFrameskipGui>();
				return true;
			}
			return false;
		});
		list->addItem(clickableListItem3);
		auto *clickableListItem4 = new tsl::elm::ListItem2("AdditionalSettingsDockedGuiListItem"_tr);
		clickableListItem4->setClickListener([this](u64 keys) { 
			if ((keys & HidNpadButton_A) && !block) {
				tsl::changeTo<DockedAdditionalGui>();
				return true;
			}
			return false;
		});
		list->addItem(clickableListItem4);
		
		frame->setContent(list);
		return frame;
	}

	virtual void update() override {
		if (!block) tsl::hlp::doWithSmSession([this]{
			if (R_SUCCEEDED(apmInitialize())) {
				ApmPerformanceMode mode = ApmPerformanceMode_Invalid;
				apmGetPerformanceMode(&mode);
				if (mode != ApmPerformanceMode_Boost ) {
					block = true;
					snprintf(Docked_c, sizeof(Docked_c), "HintUpdateDockedGuiCustomDrawerText"_tr.c_str());
				}
				apmExit();
			}
		});
	}
};

class DockedRefreshRateChangeGui : public tsl::Gui {
public:
	DockedModeRefreshRateAllowed rr;
	DockedAdditionalSettings as;
	DockedRefreshRateChangeGui () {
		LoadDockedModeAllowedSave(rr, as, nullptr);
	}

	// Called when this Gui gets loaded to create the UI
	// Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
	virtual tsl::elm::Element* createUI() override {
		// A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
		// If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
		auto frame = new tsl::elm::OverlayFrame("PluginName"_tr, "ChangeRefreshRateDockedRefreshRateChangeGuiOverlayFrame"_tr);
		// A list that can contain sub elements and handles scrolling
		auto list = new tsl::elm::List();
		for (size_t i = 0; i < sizeof(rr); i++) {
			if (rr[i] == false)
				continue;
			char Hz[] = "254 Hz";
			snprintf(Hz, sizeof(Hz), "%d Hz", DockedModeRefreshRateAllowedValues[i]);
			auto *clickableListItem = new tsl::elm::MiniListItem(Hz);
			clickableListItem->setClickListener([this, i](u64 keys) { 
				if (keys & HidNpadButton_A) {
					if (!oldSalty) {
						if (R_SUCCEEDED(SaltySD_Connect())) {
							SaltySD_SetDisplayRefreshRate(DockedModeRefreshRateAllowedValues[i]);
							SaltySD_Term();
							refreshRate_g = DockedModeRefreshRateAllowedValues[i];
						}
					}
					tsl::goBack();
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem);
		}
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
		return false;   // Return true here to singal the inputs have been consumed
	}
};

class DisplayGui : public tsl::Gui {
private:
	char refreshRate_c[32] = "";
	char oled_c[48] = "";
	bool isDocked = false;
	ApmPerformanceMode entry_mode = ApmPerformanceMode_Invalid;
public:
	DisplayGui() {
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
	size_t base_height = 128;

	virtual tsl::elm::Element* createUI() override {
		auto frame = new tsl::elm::OverlayFrame("PluginName"_tr, "DisplayGuiOverlayFrameText"_tr);
		auto list = new tsl::elm::List();
		list->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
			renderer->drawString(refreshRate_c, false, x, y+20, 20, renderer->a(0xFFFF));
			if (isOLED && !isDocked) {
				snprintf(oled_c, sizeof(oled_c), "NotAvailableForOledHintDisplayGuiCustomDrawerText"_tr.c_str());
				renderer->drawString(oled_c, false, x, y+50, 20, renderer->a(0xFFFF));
			}
		}), 90);
		if (!displaySync) {
			if (entry_mode == ApmPerformanceMode_Normal) {
				auto *clickableListItem = new tsl::elm::ListItem2("IncreaseRefreshRateDisplayGuiListItem"_tr);
				clickableListItem->setClickListener([this](u64 keys) { 
					if ((keys & HidNpadButton_A) && (!isOLED || isDocked)) {
						if ((refreshRate_g >= 40) && (refreshRate_g < 60)) {
							if (R_SUCCEEDED(SaltySD_Connect())) {
								refreshRate_g += 5;
								SaltySD_SetDisplayRefreshRate(refreshRate_g);
								SaltySD_Term();
								if (Shared) (Shared -> displaySync) = refreshRate_g ? true : false;
							}
						}
						return true;
					}
					return false;
				});
				list->addItem(clickableListItem);
				auto *clickableListItem2 = new tsl::elm::ListItem2("DecreaseRefreshRateDisplayGuiListItem"_tr);
				clickableListItem2->setClickListener([this](u64 keys) { 
					if ((keys & HidNpadButton_A) && (!isOLED || isDocked)) {
						if (refreshRate_g > 40) {
							if (R_SUCCEEDED(SaltySD_Connect())) {
								refreshRate_g -= 5;
								SaltySD_SetDisplayRefreshRate(refreshRate_g);
								if (Shared) (Shared -> displaySync) = refreshRate_g ? true : false;
								SaltySD_Term();
							}
						}
						return true;
					}
					return false;
				});
				list->addItem(clickableListItem2);
			}
			else if (entry_mode == ApmPerformanceMode_Boost) {
				auto *clickableListItem2 = new tsl::elm::ListItem2("ChangeRefreshRateDisplayGuiListItem"_tr);
				clickableListItem2->setClickListener([](u64 keys) { 
					if (keys & HidNpadButton_A) {
						tsl::changeTo<DockedRefreshRateChangeGui>();
						return true;
					}
					return false;
				});	
				list->addItem(clickableListItem2);	
			}
		}

		if (!oldSalty) {
			list->addItem(new tsl::elm::CategoryHeader("MatchRefreshRateDisplayGuiCategoryHeader"_tr, true));
			auto *clickableListItem3 = new tsl::elm::ToggleListItem("DisplaySyncDisplayGuiToggleListItem"_tr, displaySync);
			clickableListItem3->setClickListener([this](u64 keys) { 
				if (keys & HidNpadButton_A) {
					if (R_SUCCEEDED(SaltySD_Connect())) {
						SaltySD_SetDisplaySync(!displaySync);
						svcSleepThread(100'000);
						if (!isOLED || entry_mode == ApmPerformanceMode_Boost) {
							u64 PID = 0;
							Result rc = pmdmntGetApplicationProcessId(&PID);
							if (R_SUCCEEDED(rc) && Shared) {
								if (!displaySync == true && (Shared -> FPSlocked) < 40) {
									SaltySD_SetDisplayRefreshRate(60);
									(Shared -> displaySync) = false;
								}
								else if (!displaySync == true) {
									SaltySD_SetDisplayRefreshRate((Shared -> FPSlocked));
									(Shared -> displaySync) = (Shared -> FPSlocked) ? true : false;
								}
								else {
									(Shared -> displaySync) = false;
								}
							}
							else if (!displaySync == true && (R_FAILED(rc) || !PluginRunning)) {
								SaltySD_SetDisplayRefreshRate(60);
							}
						}
						SaltySD_Term();
						displaySync = !displaySync;
					}
					tsl::goBack();
					tsl::changeTo<DisplayGui>();
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem3);
			if (!isLite) {
				auto *clickableListItem4 = new tsl::elm::ListItem2("DockedSettingsDisplayGuiToggleListItem"_tr);
				clickableListItem4->setClickListener([this](u64 keys) { 
					if ((keys & HidNpadButton_A)) {
						tsl::changeTo<DockedGui>();
						return true;
					}
					return false;
				});
				list->addItem(clickableListItem4);
			}
		}
		
		frame->setContent(list);
		return frame;
	}

	virtual void update() override {
		refreshRate_g = *refreshRate_shared;
		snprintf(refreshRate_c, sizeof(refreshRate_c), "UpdateDisplayRefreshRateDisplayGuiCustomDrawerText"_tr.c_str(), refreshRate_g);
	}

	// Called once every frame to handle inputs not handled by other UI elements
	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		if (!isLite) {
			smInitialize();
			if (R_SUCCEEDED(apmInitialize())) {
				ApmPerformanceMode mode = ApmPerformanceMode_Invalid;
				apmGetPerformanceMode(&mode);
				apmExit();
				if (mode != entry_mode) {
					smExit();
					tsl::goBack();
					tsl::changeTo<DisplayGui>();
					return true;
				}
			}
			smExit();
		}
		return false;   // Return true here to singal the inputs have been consumed
	}
};

class WarningDisplayGui : public tsl::Gui {
private:
	uint8_t refreshRate = 0;
	std::string Warning = "WarningWarningDisplayGuiCustomDrawerText"_tr.c_str();
public:
	WarningDisplayGui() {}
	size_t base_height = 128;
	virtual tsl::elm::Element* createUI() override {
		auto frame = new tsl::elm::OverlayFrame("PluginName"_tr, "DisplaySettingWarningWarningDisplayGuiOverlayFrame"_tr);
		auto list = new tsl::elm::List();
		list->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
			renderer->drawString(Warning.c_str(), false, x, y+20, 20, renderer->a(0xFFFF));
		}), 200);
		auto *clickableListItem1 = new tsl::elm::ListItem2("DeclineWarningDisplayGuiListItem"_tr);
		clickableListItem1->setClickListener([this](u64 keys) { 
			if (keys & HidNpadButton_A) {
				tsl::goBack();
				return true;
			}
			return false;
		});
		list->addItem(clickableListItem1);
		auto *clickableListItem2 = new tsl::elm::ListItem2("AcceptWarningDisplayGuiListItem"_tr);
		clickableListItem2->setClickListener([this](u64 keys) { 
			if ((keys & HidNpadButton_A)) {
				tsl::goBack();
				tsl::changeTo<DisplayGui>();
				return true;
			}
			return false;
		});
		list->addItem(clickableListItem2);
		
		frame->setContent(list);
		return frame;
	}
};