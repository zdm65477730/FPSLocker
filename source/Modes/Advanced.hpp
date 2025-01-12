class SetBuffers : public tsl::Gui {
public:
    SetBuffers() {}

    virtual tsl::elm::Element* createUI() override {
		auto frame = new tsl::elm::OverlayFrame("NVNSetBufferingSetBuffersOverlayFrame"_tr, " ");

		auto list = new tsl::elm::List();
		list->addItem(new tsl::elm::CategoryHeader("AppliedNextGameBootSetBuffersListItemCategoryHeader"_tr, false));
		list->addItem(new tsl::elm::NoteHeader("RememberSaveSettingsSetBuffersListItemNoteHeader"_tr, true, {0xF, 0x3, 0x3, 0xF}));
		auto *clickableListItem = new tsl::elm::ListItem2("DoubleSetBuffersListItem"_tr);
		clickableListItem->setClickListener([](u64 keys) { 
			if ((keys & HidNpadButton_A) && PluginRunning) {
				SetBuffers_save = 2;
				tsl::goBack();
				return true;
			}
			return false;
		});
		list->addItem(clickableListItem);

		if ((Shared -> SetActiveBuffers) == 2 && (Shared -> Buffers) == 3 && !SetBuffers_save) {
			auto *clickableListItem2 = new tsl::elm::ListItem2("TripleForceSetBuffersListItem"_tr);
			clickableListItem2->setClickListener([](u64 keys) { 
				if ((keys & HidNpadButton_A) && PluginRunning) {
					SetBuffers_save = 3;
					tsl::goBack();
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem2);
		}
		else {
			auto *clickableListItem2 = new tsl::elm::ListItem2("TripleSetBuffersListItem"_tr);
			clickableListItem2->setClickListener([](u64 keys) { 
				if ((keys & HidNpadButton_A) && PluginRunning) {
					if ((Shared -> Buffers) == 4) SetBuffers_save = 3;
					else SetBuffers_save = 0;
					tsl::goBack();
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem2);
		}
		
		if ((Shared -> Buffers) == 4) {
			if ((Shared -> SetActiveBuffers) < 4 && (Shared -> SetActiveBuffers) > 0 && (Shared -> Buffers) == 4) {
				auto *clickableListItem3 = new tsl::elm::ListItem2("QuadrupleForceSetBuffersListItem"_tr);
				clickableListItem3->setClickListener([](u64 keys) { 
					if ((keys & HidNpadButton_A) && PluginRunning) {
						SetBuffers_save = 4;
						tsl::goBack();
						return true;
					}
					return false;
				});
				list->addItem(clickableListItem3);	
			}
			else {
				auto *clickableListItem3 = new tsl::elm::ListItem2("QuadrupleSetBuffersListItem"_tr);
				clickableListItem3->setClickListener([](u64 keys) { 
					if ((keys & HidNpadButton_A) && PluginRunning) {
						SetBuffers_save = 0;
						tsl::goBack();
						return true;
					}
					return false;
				});
				list->addItem(clickableListItem3);
			}
		}

		frame->setContent(list);

        return frame;
    }
};

class SyncMode : public tsl::Gui {
public:
    SyncMode() {}

    virtual tsl::elm::Element* createUI() override {
        auto frame = new tsl::elm::OverlayFrame("NVNWindowSyncWaitSyncModeOverlayFrame"_tr, "SyncModeSyncModeOverlayFrameSubtitle"_tr);

		auto list = new tsl::elm::List();

		auto *clickableListItem = new tsl::elm::ListItem2("EnabledSyncModeSyncModeListItem"_tr);
		clickableListItem->setClickListener([](u64 keys) { 
			if ((keys & HidNpadButton_A) && PluginRunning) {
				ZeroSyncMode = "OnSyncModeAdvancedGuiListItemText"_tr.c_str();
				(Shared -> ZeroSync) = 0;
				tsl::goBack();
				tsl::goBack();
				return true;
			}
			return false;
		});
		list->addItem(clickableListItem);

		auto *clickableListItem2 = new tsl::elm::ListItem2("SemiEnabledSyncModeSyncModeListItem"_tr);
		clickableListItem2->setClickListener([](u64 keys) { 
			if ((keys & HidNpadButton_A) && PluginRunning) {
				ZeroSyncMode = "SemiSyncModeAdvancedGuiListItemText"_tr.c_str();
				(Shared -> ZeroSync) = 2;
				tsl::goBack();
				tsl::goBack();
				return true;
			}
			return false;
		});
		list->addItem(clickableListItem2);

		auto *clickableListItem3 = new tsl::elm::ListItem2("DisabledSyncModeSyncModeListItem"_tr);
		clickableListItem3->setClickListener([](u64 keys) { 
			if ((keys & HidNpadButton_A) && PluginRunning) {
				ZeroSyncMode = "OffSyncModeAdvancedGuiListItemText"_tr.c_str();
				(Shared -> ZeroSync) = 1;
				tsl::goBack();
				tsl::goBack();
				return true;
			}
			return false;
		});
		list->addItem(clickableListItem3);
		
        frame->setContent(list);

        return frame;
    }
};

class AdvancedGui : public tsl::Gui {
public:
	bool exitPossible = true;
    AdvancedGui() {
		configValid = LOCK::readConfig(&configPath[0]);
		if (R_FAILED(configValid)) {
			if (configValid == 0x202) {
				sprintf(&lockInvalid[0], "ConfigFileNotFoundAdvancedGuiCustomDrawerText"_tr.c_str(), TID, BID);
			}
			else sprintf(&lockInvalid[0], "ConfigErrorAdvancedGuiCustomDrawerText"_tr.c_str(), configValid);
		}
		else {
			patchValid = checkFile(&patchPath[0]);
			if (R_FAILED(patchValid)) {
				if (!FileDownloaded) {
					if (R_SUCCEEDED(configValid)) {
						sprintf(&patchChar[0], "PatchFileNotExistMoreAdvancedGuiCustomDrawerText"_tr.c_str());
					}
					else sprintf(&patchChar[0], "PatchFileNotExistAdvancedGuiCustomDrawerText"_tr.c_str());
				}
				else {
					sprintf(&patchChar[0], "NewConfigDownloadSuccessAdvancedGuiCustomDrawerText"_tr.c_str());
				}
			}
			else sprintf(&patchChar[0], "PatchFileExistAdvancedGuiCustomDrawerText"_tr.c_str());
		}
		switch((Shared -> ZeroSync)) {
			case 0:
				ZeroSyncMode = "OnSyncModeAdvancedGuiListItemText"_tr.c_str();
				break;
			case 1:
				ZeroSyncMode = "OffSyncModeAdvancedGuiListItemText"_tr.c_str();
				break;
			case 2:
				ZeroSyncMode = "SemiSyncModeAdvancedGuiListItemText"_tr.c_str();
		}
	}

	size_t base_height = 134;

    virtual tsl::elm::Element* createUI() override {
        auto frame = new tsl::elm::OverlayFrame("PluginName"_tr, "AdvancedSettingsAdvancedGuiOverlayFrameSubTitle"_tr);

		auto list = new tsl::elm::List();

		if ((Shared -> API)) {
			switch((Shared -> API)) {
				case 1: {
					list->addItem(new tsl::elm::CategoryHeader("NVNAdvancedGuiCategoryHeader"_tr, false));
					
					list->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
						
						renderer->drawString(&nvnBuffers[0], false, x, y+20, 20, renderer->a(0xFFFF));
							
					}), 40);

					if ((Shared -> Buffers) == 2 || (Shared -> SetBuffers) == 2 || (Shared -> ActiveBuffers) == 2) {
						auto *clickableListItem3 = new tsl::elm::MiniListItem("SyncWaitAdvancedGuiListItem"_tr, ZeroSyncMode);
						clickableListItem3->setClickListener([](u64 keys) { 
							if ((keys & HidNpadButton_A) && PluginRunning) {
								tsl::changeTo<SyncMode>();
								return true;
							}
							return false;
						});
						list->addItem(clickableListItem3);
					}
					if ((Shared -> Buffers) > 2) {
						auto *clickableListItem3 = new tsl::elm::MiniListItem("SetBufferingAdvancedGuiListItem"_tr);
						clickableListItem3->setClickListener([](u64 keys) { 
							if ((keys & HidNpadButton_A) && PluginRunning) {
								tsl::changeTo<SetBuffers>();
								return true;
							}
							return false;
						});
						list->addItem(clickableListItem3);
					}
					break;
				}
				case 2:
					list->addItem(new tsl::elm::CategoryHeader("EGLAdvancedGuiCategoryHeader"_tr, false));
					break;
				case 3:
					list->addItem(new tsl::elm::CategoryHeader("VulkanAdvancedGuiCategoryHeader"_tr, false));
			}
		}

		list->addItem(new tsl::elm::CategoryHeader("PatchesAdvancedGuiCategoryHeader"_tr, false));

		if (R_FAILED(configValid)) {
			base_height = 154;
		}

		list->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
			
			if (R_SUCCEEDED(configValid)) {
				
				renderer->drawString("ConfigFileValidAdvancedGuiCustomDrawerText"_tr.c_str(), false, x, y+20, 20, renderer->a(0xFFFF));
				renderer->drawString(&patchAppliedChar[0], false, x, y+40, 20, renderer->a(0xFFFF));
				if (R_FAILED(patchValid)) {
					renderer->drawString(&patchChar[0], false, x, y+64, 20, renderer->a(0xF99F));
				}
				else renderer->drawString(&patchChar[0], false, x, y+64, 20, renderer->a(0xFFFF));
			}
			else {
				renderer->drawString(&lockInvalid[0], false, x, y+20, 20, renderer->a(0xFFFF));
				renderer->drawString(&patchChar[0], false, x, y+84, 20, renderer->a(0xF99F));
			}
				

		}), base_height);

		if (R_SUCCEEDED(configValid)) {
			list->addItem(new tsl::elm::NoteHeader("PatchWillBeAppliedNextGameBootAdvancedGuiNoteHeader"_tr, true, {0xF, 0x3, 0x3, 0xF}));
			auto *clickableListItem = new tsl::elm::MiniListItem("ConvertConfigToPatchFileAdvancedGuiListItem"_tr);
			clickableListItem->setClickListener([](u64 keys) { 
				if ((keys & HidNpadButton_A) && PluginRunning) {
					patchValid = LOCK::createPatch(&patchPath[0]);
					if (R_SUCCEEDED(patchValid)) {
						sprintf(&patchChar[0], "PatchFileCreatedSuccessAdvancedGuiListItemText"_tr.c_str());
					}
					else sprintf(&patchChar[0], "PatchFileCreateFailedAdvancedGuiListItemText"_tr.c_str(), patchValid);
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem);

			auto *clickableListItem2 = new tsl::elm::MiniListItem("DeletePatchFileAdvancedGuiListItem"_tr);
			clickableListItem2->setClickListener([](u64 keys) { 
				if ((keys & HidNpadButton_A) && PluginRunning) {
					if (R_SUCCEEDED(patchValid)) {
						remove(&patchPath[0]);
						patchValid = 0x202;
						sprintf(&patchChar[0], "DeletePatchSuccessfulFileAdvancedGuiListItemText"_tr.c_str());
					}
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem2);
		}
		if (R_FAILED(configValid)) {
			list->addItem(new tsl::elm::NoteHeader("Take30sAdvancedGuiListItemText"_tr, true, {0xF, 0x3, 0x3, 0xF}));
		}
		auto *clickableListItem4 = new tsl::elm::MiniListItem("CheckOrDownloadConfigFileAdvancedGuiListItemText"_tr);
		clickableListItem4->setClickListener([this](u64 keys) { 
			if ((keys & HidNpadButton_A) && PluginRunning && exitPossible) {
				exitPossible = false;
				sprintf(&patchChar[0], "CheckWarehouseAdvancedGuiListItemText"_tr.c_str());
				threadCreate(&t1, downloadPatch, NULL, NULL, 0x20000, 0x3F, 3);
				threadStart(&t1);
				return true;
			}
			return false;
		});
		list->addItem(clickableListItem4);

		frame->setContent(list);

        return frame;
    }

	virtual void update() override {
		static uint8_t i = 10;

		if (PluginRunning) {
			if (i > 9) {
				if ((Shared -> patchApplied) == 1) {
					sprintf(patchAppliedChar, "PluginLoadedUpdateAdvancedGuiCustomDrawerText"_tr.c_str());
				}
				else if ((Shared -> patchApplied) == 2) {
					sprintf(patchAppliedChar, "MasterWriteLoadedUpdateAdvancedGuiCustomDrawerText"_tr.c_str());
				}
				else sprintf(patchAppliedChar, "PluginNotApplyUpdateAdvancedGuiCustomDrawerText"_tr.c_str());
				if ((Shared -> API) == 1) {
					if (((Shared -> Buffers) >= 2 && (Shared -> Buffers) <= 4)) {
						sprintf(&nvnBuffers[0], "SetOrActiveOrAvailableBuffersUpdateAdvancedGuiCustomDrawerText"_tr.c_str(), (Shared -> SetActiveBuffers), (Shared -> ActiveBuffers), (Shared -> Buffers));
					}
				}
				i = 0;
			}
			else i++;
		}
	}

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		if (exitPossible) {
			if (keysDown & HidNpadButton_B) {
				tsl::goBack();
				return true;
			}
		}
		else if (!exitPossible) {
			if (keysDown & HidNpadButton_B)
				return true;
			Result rc = error_code;
			if (rc != UINT32_MAX && rc != 0x404) {
				threadWaitForExit(&t1);
				threadClose(&t1);
				exitPossible = true;
				error_code = UINT32_MAX;
			}
			if (rc == 0x316) {
				sprintf(&patchChar[0], "ConnectionTimeoutAdvancedGuiListItemText"_tr.c_str());
			}
			else if (rc == 0x212 || rc == 0x312) {
				sprintf(&patchChar[0], "ConfigNotAvailableAdvancedGuiListItemText"_tr.c_str(), rc);
			}
			else if (rc == 0x404) {
				sprintf(&patchChar[0], "ConfigNotAvailable404AdvancedGuiListItemText"_tr.c_str());
			}
			else if (rc == 0x405) {
				sprintf(&patchChar[0], "ConfigNotAvailableTimeoutAdvancedGuiListItemText"_tr.c_str());
			}
			else if (rc == 0x406) {
				sprintf(&patchChar[0], "ConfigNotAvailableConnectionErrorAdvancedGuiListItemText"_tr.c_str());
			}
			else if (rc == 0x104) {
				sprintf(&patchChar[0], "NoNewConfigAdvancedGuiListItemText"_tr.c_str());
			}
			else if (rc == 0x412) {
				sprintf(&patchChar[0], "InternetConnectionNotAvailableAdvancedGuiListItemText"_tr.c_str());
			}
			else if (rc == 0x1001) {
				sprintf(&patchChar[0], "PatchNotNeededAdvancedGuiListItemText"_tr.c_str());
			}
			else if (rc == 0x1002) {
				sprintf(&patchChar[0], "NotListedInWarehouseAdvancedGuiListItemText"_tr.c_str());
			}
			else if (rc == 0x1003) {
				sprintf(&patchChar[0], "DiffVersionListedInWarehouseNoNeedPatchAdvancedGuiListItemText"_tr.c_str());
			}
			else if (rc == 0x1004) {
				sprintf(&patchChar[0], "DiffVersionListedInWarehouseNoPatchAdvancedGuiListItemText"_tr.c_str());
			}
			else if (rc == 0x1005) {
				sprintf(&patchChar[0], "DiffVersionListedInWarehouseOtherVersionAvailableAdvancedGuiListItemText"_tr.c_str());
			}
			else if (rc == 0x1006) {
				sprintf(&patchChar[0], "ListedInWarehousePatchNotAvailableAdvancedGuiListItemText"_tr.c_str());
			}
			else if (R_SUCCEEDED(rc)) {
				FILE* fp = fopen(patchPath, "rb");
				if (fp) {
					fclose(fp);
					remove(patchPath);
				}
				tsl::goBack();
				tsl::changeTo<AdvancedGui>();
				return true;
			}
			else if (rc != UINT32_MAX) {
				sprintf(&patchChar[0], "ConnectionErrorAdvancedGuiListItemText"_tr.c_str(), rc);
			}
		}
        return false;   // Return true here to signal the inputs have been consumed
    }
};