class SetBuffers : public tsl::Gui {
public:
    SetBuffers() {}

    virtual tsl::elm::Element* createUI() override {
		auto frame = new tsl::elm::OverlayFrame("SetBufferingSetBuffersOverlayFrame"_tr, " ");

		auto list = new tsl::elm::List();
		if (Shared->expectedSetBuffers == -1) list->addItem(new tsl::elm::NoteHeader("AppliedNextGameBootSetBuffersListItemCategoryHeader"_tr, true, {0xF, 0x3, 0x3, 0xF}));
		auto *clickableListItem = new tsl::elm::ListItem2("DoubleSetBuffersListItem"_tr);
		clickableListItem->setClickListener([](u64 keys) { 
			if ((keys & HidNpadButton_A) && PluginRunning) {
				SetBuffers_save = 2;
				if (Shared->expectedSetBuffers != -1) Shared->expectedSetBuffers = 2;
				saveSettings();
				tsl::goBack();
				tsl::goBack();
				return true;
			}
			return false;
		});
		list->addItem(clickableListItem);

		if ((Shared -> API) == 3) {
			auto *clickableListItemv1 = new tsl::elm::ListItem2("TripleSetBuffersListItem"_tr);
			clickableListItemv1->setClickListener([](u64 keys) { 
				if ((keys & HidNpadButton_A) && PluginRunning) {
					SetBuffers_save = 3;
					if (Shared->expectedSetBuffers != -1) Shared->expectedSetBuffers = 3;
					saveSettings();
					tsl::goBack();
					tsl::goBack();
					return true;
				}
				return false;
			});
			list->addItem(clickableListItemv1);

		}
		else {
			if ((Shared -> Buffers) >= 3) {
				if ((Shared -> SetActiveBuffers) > 0) {
					auto *clickableListItem2 = new tsl::elm::ListItem2("TripleForceSetBuffersListItem"_tr);
					clickableListItem2->setClickListener([](u64 keys) { 
						if ((keys & HidNpadButton_A) && PluginRunning) {
							SetBuffers_save = 3;
							if (Shared->expectedSetBuffers != -1) Shared->expectedSetBuffers = 3;
							saveSettings();
							tsl::goBack();
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
							if (Shared->expectedSetBuffers != -1) Shared->expectedSetBuffers = 3;
							saveSettings();
							tsl::goBack();
							tsl::goBack();
							return true;
						}
						return false;
					});
					list->addItem(clickableListItem2);
				}
			}

			if ((Shared -> Buffers) == 4) {
				if ((Shared -> SetActiveBuffers) > 0) {
					auto *clickableListItem3 = new tsl::elm::ListItem2("QuadrupleForceSetBuffersListItem"_tr);
					clickableListItem3->setClickListener([](u64 keys) { 
						if ((keys & HidNpadButton_A) && PluginRunning) {
							SetBuffers_save = 4;
							if (Shared->expectedSetBuffers != -1) Shared->expectedSetBuffers = 4;
							saveSettings();
							tsl::goBack();
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
							if (Shared->expectedSetBuffers != -1) Shared->expectedSetBuffers = 4;
							saveSettings();
							tsl::goBack();
							tsl::goBack();
							return true;
						}
						return false;
					});
					list->addItem(clickableListItem3);
				}
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
				saveSettings();
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
				saveSettings();
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
				saveSettings();
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
		patchChar[0] = 0;
		configValid = LOCK::readConfig(configPath);
		if (R_FAILED(configValid)) {
			if (configValid == 0x202) {
				sprintf(lockInvalid, "ConfigFileNotFoundAdvancedGuiCustomDrawerText"_tr.c_str(), TID, BID);
				size_t appControlDataSize = 0;
				s32 appContentMetaStatusSize = 0;
				NsApplicationControlData appControlData;
				NsApplicationContentMetaStatus appContentMetaStatus[2];
				if (R_SUCCEEDED(nsGetApplicationControlData(NsApplicationControlSource::NsApplicationControlSource_Storage, TID, &appControlData, sizeof(NsApplicationControlData), &appControlDataSize)) 
					&& R_SUCCEEDED(nsListApplicationContentMetaStatus(TID, 0, appContentMetaStatus, 2, &appContentMetaStatusSize))) {
						u32 index = 0;
						if (appContentMetaStatusSize == 2 && appContentMetaStatus[1].meta_type == NcmContentMetaType_Patch) index = 1;
						if (appContentMetaStatus[index].version != 0)
							snprintf(lockVersionExpected, sizeof(lockVersionExpected), "%s | v%d/%d", appControlData.nacp.display_version, appContentMetaStatus[index].version / 65536, appContentMetaStatus[index].version);
						else sprintf(lockVersionExpected, "%s | v0", appControlData.nacp.display_version);
				}
			}
			else sprintf(lockInvalid, "ConfigErrorAdvancedGuiCustomDrawerText"_tr.c_str(), configValid);
		}
		else {
			patchValid = !file_exists(patchPath);
			if (R_FAILED(patchValid)) {
				if (!FileDownloaded) {
					if (R_SUCCEEDED(configValid)) {
						sprintf(patchChar, "PatchFileNotExistMoreAdvancedGuiCustomDrawerText"_tr.c_str());
					}
					else sprintf(patchChar, "PatchFileNotExistAdvancedGuiCustomDrawerText"_tr.c_str());
				}
				else {
					sprintf(patchChar, "NewConfigDownloadSuccessAdvancedGuiCustomDrawerText"_tr.c_str());
				}
			}
			else sprintf(patchChar, "PatchFileExistAdvancedGuiCustomDrawerText"_tr.c_str());
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
						
						renderer->drawString(nvnBuffers, false, x, y+20, 20, renderer->a(0xFFFF));
							
					}), 60);

					if ((Shared -> Buffers) == 2 || (Shared -> ActiveBuffers) == 2) {
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
				case 3: {
					list->addItem(new tsl::elm::CategoryHeader("VulkanAdvancedGuiCategoryHeader"_tr, false));

					list->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {

						renderer->drawString(nvnBuffers, false, x, y+20, 20, renderer->a(0xFFFF));

					}), 40);

					if ((Shared -> Buffers) >= 2) {
						auto *clickableListItem3 = new tsl::elm::MiniListItem("SetBufferingSetBuffersOverlayFrame"_tr);
						clickableListItem3->setClickListener([](u64 keys) { 
							if ((keys & HidNpadButton_A) && PluginRunning) {
								tsl::changeTo<SetBuffers>();
								return true;
							}
							return false;
						});
						list->addItem(clickableListItem3);
					}
				}
			}
		}

		list->addItem(new tsl::elm::CategoryHeader("PatchesAdvancedGuiCategoryHeader"_tr, false));

		if (R_FAILED(configValid)) {
			base_height = 154;
		}

		list->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
			
			if (R_SUCCEEDED(configValid)) {
				
				renderer->drawString("ConfigFileValidAdvancedGuiCustomDrawerText"_tr.c_str(), false, x, y+20, 20, renderer->a(0xFFFF));
				renderer->drawString(patchAppliedChar, false, x, y+40, 20, renderer->a(0xFFFF));
				if (R_FAILED(patchValid)) {
					renderer->drawString(patchChar, false, x, y+64, 20, renderer->a(0xF99F));
				}
				else renderer->drawString(patchChar, false, x, y+64, 20, renderer->a(0xFFFF));
			}
			else {
				renderer->drawString(lockInvalid, false, x, y+20, 20, renderer->a(0xFFFF));
				if (patchChar[0] != 0)
					renderer->drawString(patchChar, false, x, y+84, 20, renderer->a(0xF99F));
				renderer->drawString(patchChar, false, x, y+84, 20, renderer->a(0xF99F));
			}
				

		}), base_height);

		if (R_SUCCEEDED(configValid)) {
			list->addItem(new tsl::elm::NoteHeader("PatchWillBeAppliedNextGameBootAdvancedGuiNoteHeader"_tr, true, {0xF, 0x3, 0x3, 0xF}));
			auto *clickableListItem = new tsl::elm::MiniListItem("ConvertConfigToPatchFileAdvancedGuiListItem"_tr);
			clickableListItem->setClickListener([](u64 keys) { 
				if ((keys & HidNpadButton_A) && PluginRunning) {
					patchValid = LOCK::createPatch(patchPath);
					if (R_SUCCEEDED(patchValid)) {
						sprintf(patchChar, "PatchFileCreatedSuccessAdvancedGuiListItemText"_tr.c_str());
					}
					else sprintf(patchChar, "PatchFileCreateFailedAdvancedGuiListItemText"_tr.c_str(), patchValid);
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem);

			auto *clickableListItem2 = new tsl::elm::MiniListItem("DeletePatchFileAdvancedGuiListItem"_tr);
			clickableListItem2->setClickListener([](u64 keys) { 
				if ((keys & HidNpadButton_A) && PluginRunning) {
					if (R_SUCCEEDED(patchValid)) {
						remove(patchPath);
						patchValid = 0x202;
						sprintf(patchChar, "DeletePatchSuccessfulFileAdvancedGuiListItemText"_tr.c_str());
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
				sprintf(patchChar, "CheckWarehouseAdvancedGuiListItemText"_tr.c_str());
				threadCreate(&t1, downloadPatch, NULL, NULL, 0x20000, 0x3F, 3);
				threadStart(&t1);
				return true;
			}
			return false;
		});
		list->addItem(clickableListItem4);

		list->addItem(new tsl::elm::CategoryHeader("MiscAdvancedGuiCategoryHeader"_tr, false));

		auto *clickableListItem5 = new tsl::elm::MiniToggleListItem("HaltUnfocusedGameAdvancedGuiListItemText"_tr, forceSuspend_save);
		clickableListItem5->setClickListener([this](u64 keys) { 
			if ((keys & HidNpadButton_A) && PluginRunning) {
				forceSuspend_save = !forceSuspend_save;
				(Shared -> forceSuspend) = forceSuspend_save;
				saveSettings();
				return true;
			}
			return false;
		});
		list->addItem(clickableListItem5);

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
				switch (Shared -> API) {
					case 1: {
						if (((Shared -> Buffers) >= 2 && (Shared -> Buffers) <= 4)) {
							sprintf(nvnBuffers, "SetOrActiveOrAvailableBuffersUpdateAdvancedGuiCustomDrawerText"_tr.c_str(), (Shared -> SetActiveBuffers), (Shared -> ActiveBuffers), (Shared -> Buffers));
						}
						break;
					}
					case 3: {
						if (((Shared -> Buffers) >= 2 && (Shared -> Buffers) <= 4)) {
							sprintf(nvnBuffers, "ActiveBuffersUpdateAdvancedGuiCustomDrawerText"_tr.c_str(), (Shared -> Buffers));
						}
						break;
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
				sprintf(patchChar, "ConnectionTimeoutAdvancedGuiListItemText"_tr.c_str());
			}
			else if (rc == 0x212 || rc == 0x312) {
				sprintf(patchChar, "ConfigNotAvailableAdvancedGuiListItemText"_tr.c_str(), rc);
			}
			else if (rc == 0x404) {
				sprintf(patchChar, "ConfigNotAvailable404AdvancedGuiListItemText"_tr.c_str());
			}
			else if (rc == 0x405) {
				sprintf(patchChar, "ConfigNotAvailableTimeoutAdvancedGuiListItemText"_tr.c_str());
			}
			else if (rc == 0x406) {
				sprintf(patchChar, "ConfigNotAvailableConnectionErrorAdvancedGuiListItemText"_tr.c_str());
			}
			else if (rc == 0x104) {
				sprintf(patchChar, "NoNewConfigAdvancedGuiListItemText"_tr.c_str());
			}
			else if (rc == 0x412) {
				sprintf(patchChar, "InternetConnectionNotAvailableAdvancedGuiListItemText"_tr.c_str());
			}
			else if (rc == 0x1001) {
				sprintf(patchChar, "PatchNotNeededAdvancedGuiListItemText"_tr.c_str());
			}
			else if (rc == 0x1002) {
				sprintf(patchChar, "NotListedInWarehouseAdvancedGuiListItemText"_tr.c_str());
			}
			else if (rc == 0x1003) {
				sprintf(patchChar, "DiffVersionListedInWarehouseNoNeedPatchAdvancedGuiListItemText"_tr.c_str(), expected_display_version);
			}
			else if (rc == 0x1004) {
				sprintf(patchChar, "DiffVersionListedInWarehouseNoPatchAdvancedGuiListItemText"_tr.c_str(), expected_display_version);
			}
			else if (rc == 0x1005) {
				sprintf(patchChar, "DiffVersionListedInWarehouseOtherVersionAvailableAdvancedGuiListItemText"_tr.c_str(), expected_display_version);
			}
			else if (rc == 0x1006) {
				sprintf(patchChar, "ListedInWarehousePatchNotAvailableAdvancedGuiListItemText"_tr.c_str());
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
				sprintf(patchChar, "ConnectionErrorAdvancedGuiListItemText"_tr.c_str(), rc);
			}
		}
        return false;   // Return true here to signal the inputs have been consumed
    }
};
