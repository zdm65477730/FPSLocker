#define TESLA_INIT_IMPL // If you have more than one file using the tesla header, only define this in the main one
#include <tesla.hpp>    // The Tesla Header
#include "MiniList.hpp"
#include <sys/stat.h>
#include <dirent.h>
#include "SaltyNX.h"
#include "Lock.hpp"
#include "Utils.hpp"

using namespace tsl;

#include <curl/curl.h>

bool FileDownloaded = false;
bool displaySync = false;
bool isOLED = false;
uint8_t refreshRate_g = 60;
bool oldSalty = false;
ApmPerformanceMode performanceMode = ApmPerformanceMode_Invalid;
bool isDocked = false;

Result downloadPatch() {

	static const SocketInitConfig socketInitConfig = {
		.tcp_tx_buf_size = 0x800,
		.tcp_rx_buf_size = 0x800,
		.tcp_tx_buf_max_size = 0x8000,
		.tcp_rx_buf_max_size = 0x8000,

		.udp_tx_buf_size = 0,
		.udp_rx_buf_size = 0,

		.sb_efficiency = 1,
		.bsd_service_type = BsdServiceType_Auto
	};

	smInitialize();

	nifmInitialize(NifmServiceType_System);
	u32 dummy = 0;
	NifmInternetConnectionType NifmConnectionType = (NifmInternetConnectionType)-1;
	NifmInternetConnectionStatus NifmConnectionStatus = (NifmInternetConnectionStatus)-1;
	if (R_FAILED(nifmGetInternetConnectionStatus(&NifmConnectionType, &dummy, &NifmConnectionStatus)) || NifmConnectionStatus != NifmInternetConnectionStatus_Connected) {
		nifmExit();
		smExit();
		return 0x412;
	}
	nifmExit();

	socketInitialize(&socketInitConfig);

	curl_global_init(CURL_GLOBAL_DEFAULT);
	CURL *curl = curl_easy_init();

	Result error_code = 0;

	if (curl) {
		char download_path[256] = "";
		char file_path[192] = "";
		snprintf(download_path, sizeof(download_path), "sdmc:/SaltySD/plugins/FPSLocker/patches/%016lX/", TID);

		std::filesystem::create_directories(download_path);

		snprintf(file_path, sizeof(file_path), "sdmc:/SaltySD/plugins/FPSLocker/patches/%016lX/temp.yaml", TID);

		FILE* fp = fopen(file_path, "wb+");
		if (!fp) {
			curl_easy_cleanup(curl);
			curl_global_cleanup();
			socketExit();
			smExit();
			return 0x101;
		}

		snprintf(download_path, sizeof(download_path), "https://raw.githubusercontent.com/masagrator/FPSLocker-Warehouse/v3/SaltySD/plugins/FPSLocker/patches/%016lX/%016lX.yaml", TID, BID);
		curl_easy_setopt(curl, CURLOPT_URL, download_path);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

		CURLcode res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			fclose(fp);
			remove(file_path);
			error_code = 0x200 + res;
		}
		else {
			size_t filesize = ftell(fp);
			if (filesize > 512) {
				filesize = 512;
			}
			fseek(fp, 0, SEEK_SET);
			char* buffer = (char*)calloc(1, filesize + 1);
			fread(buffer, 1, filesize, fp);
			fclose(fp);
			char BID_char[18] = "";
			snprintf(BID_char, sizeof(BID_char), " %016lX", BID);
			if (std::search(&buffer[0], &buffer[filesize], &BID_char[0], &BID_char[17]) == &buffer[filesize]) {
				remove(file_path);
				char Not_found[] = "404: Not Found";
				if (std::search(&buffer[0], &buffer[filesize], &Not_found[0], &Not_found[strlen(Not_found)]) != &buffer[filesize]) {
					error_code = 0x404;
				}
				else error_code = 0x312;
			}
			free(buffer);
		}

		if (!error_code) {
			fp = fopen(file_path, "rb");
			fseek(fp, 0, SEEK_END);
			size_t filesize1 = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			char* buffer1 = (char*)calloc(1, filesize1 + 1);
			fread(buffer1, 1, filesize1, fp);
			fclose(fp);
			fp = fopen(configPath, "rb");
			if (fp) {
				fseek(fp, 0, SEEK_END);
				size_t filesize2 = ftell(fp);
				fseek(fp, 0, SEEK_SET);
				if (filesize2 != filesize1) {
					fclose(fp);
					free(buffer1);
					FileDownloaded = true;
				}
				else {
					char* buffer2 = (char*)calloc(1, filesize2 + 1);
					fread(buffer2, 1, filesize2, fp);
					fclose(fp);
					if (memcmp(buffer1, buffer2, filesize1)) {
						FileDownloaded = true;
					}
					else {
						error_code = 0x104;
						remove(file_path);
					}
					free(buffer1);
					free(buffer2);
				}
			}
			else {
				free(buffer1);
				FileDownloaded = true;
			}
			if (!error_code) {
				remove(configPath);
				rename(file_path, configPath);
				FILE* config = fopen(configPath, "r");
				memset(&LOCK::configBuffer, 0, sizeof(LOCK::configBuffer));
				fread(&LOCK::configBuffer, 1, 32768, config);
				fclose(config);
				strcat(&LOCK::configBuffer[0], "\n");
				LOCK::tree = ryml::parse_in_place(LOCK::configBuffer);
				size_t root_id = LOCK::tree.root_id();
				if (LOCK::tree.is_map(root_id) && LOCK::tree.find_child(root_id, "Addons") != c4::yml::NONE && !LOCK::tree["Addons"].is_keyval() && LOCK::tree["Addons"].num_children() > 0) {
					for (size_t i = 0; i < LOCK::tree["Addons"].num_children(); i++) {
						std::string temp = "";
						LOCK::tree["Addons"][i] >> temp;
						std::string dpath = "https://raw.githubusercontent.com/masagrator/FPSLocker-Warehouse/v3/" + temp;
						std::string path = "sdmc:/" + temp;
						strncpy(&download_path[0], dpath.c_str(), 255);
						strncpy(&file_path[0], path.c_str(), 191);
						curl_easy_setopt(curl, CURLOPT_URL, download_path);
						FILE* fp = fopen(file_path, "wb");
						if (!fp) {
							std::filesystem::create_directories(std::filesystem::path(file_path).parent_path());
							fp = fopen(file_path, "wb");
						}
						if (fp) {
							curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
							res = curl_easy_perform(curl);
							fclose(fp);
						}
					}
				}
			}
		}

		curl_easy_cleanup(curl);
	}

	curl_global_cleanup();
	socketExit();
	smExit();
	return error_code;
}

void loopThread(void*) {
	while(threadActive) {
		if (R_FAILED(pmdmntGetApplicationProcessId(&PID))) break;
		svcSleepThread(1'000'000'000);
	}
	PluginRunning = false;
	check = false;
	closed = true;
}

class SetBuffers : public tsl::Gui {
public:
	SetBuffers() {}

	virtual tsl::elm::Element* createUI() override {
		auto frame = new tsl::elm::OverlayFrame("NVNSetBufferingSetBuffersOverlayFrame"_tr, "");

		auto list = new tsl::elm::List();
		list->addItem(new tsl::elm::CategoryHeader("AppliedNextGameBootSetBuffersListItemCategoryHeader"_tr, false));
		list->addItem(new tsl::elm::CategoryHeader("RememberSaveSettingsSetBuffersListItemCategoryHeader"_tr, true));
		auto *clickableListItem = new tsl::elm::ListItem("DoubleSetBuffersListItem"_tr);
		clickableListItem->setClickListener([](u64 keys) {
			if ((keys & HidNpadButton_A) && PluginRunning) {
				SetBuffers_save = 2;
				tsl::goBack();
				return true;
			}
			return false;
		});
		list->addItem(clickableListItem);

		if (*SetActiveBuffers_shared == 2 && *Buffers_shared == 3 && !SetBuffers_save) {
			auto *clickableListItem2 = new tsl::elm::ListItem("TripleForceSetBuffersListItem"_tr);
			clickableListItem2->setClickListener([](u64 keys) {
				if ((keys & HidNpadButton_A) && PluginRunning) {
					SetBuffers_save = 3;
					tsl::goBack();
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem2);
		} else {
			auto *clickableListItem2 = new tsl::elm::ListItem("TripleSetBuffersListItem"_tr);
			clickableListItem2->setClickListener([](u64 keys) {
				if ((keys & HidNpadButton_A) && PluginRunning) {
					if (*Buffers_shared == 4) SetBuffers_save = 3;
					else SetBuffers_save = 0;
					tsl::goBack();
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem2);
		}

		if (*Buffers_shared == 4) {
			if (*SetActiveBuffers_shared < 4 && *SetActiveBuffers_shared > 0 && *Buffers_shared == 4) {
				auto *clickableListItem3 = new tsl::elm::ListItem("QuadrupleForceSetBuffersListItem"_tr);
				clickableListItem3->setClickListener([](u64 keys) {
					if ((keys & HidNpadButton_A) && PluginRunning) {
						SetBuffers_save = 4;
						tsl::goBack();
						return true;
					}
					return false;
				});
				list->addItem(clickableListItem3);
			} else {
				auto *clickableListItem3 = new tsl::elm::ListItem("QuadrupleSetBuffersListItem"_tr);
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

		auto *clickableListItem = new tsl::elm::ListItem("EnabledSyncModeSyncModeListItem"_tr);
		clickableListItem->setClickListener([](u64 keys) {
			if ((keys & HidNpadButton_A) && PluginRunning) {
				ZeroSyncMode = "OnSyncModeAdvancedGuiListItemText"_tr.c_str();
				*ZeroSync_shared = 0;
				tsl::goBack();
				tsl::goBack();
				return true;
			}
			return false;
		});
		list->addItem(clickableListItem);

		auto *clickableListItem2 = new tsl::elm::ListItem("SemiEnabledSyncModeSyncModeListItem"_tr);
		clickableListItem2->setClickListener([](u64 keys) {
			if ((keys & HidNpadButton_A) && PluginRunning) {
				ZeroSyncMode = "SemiSyncModeAdvancedGuiListItemText"_tr.c_str();
				*ZeroSync_shared = 2;
				tsl::goBack();
				tsl::goBack();
				return true;
			}
			return false;
		});
		list->addItem(clickableListItem2);

		auto *clickableListItem3 = new tsl::elm::ListItem("DisabledSyncModeSyncModeListItem"_tr);
		clickableListItem3->setClickListener([](u64 keys) {
			if ((keys & HidNpadButton_A) && PluginRunning) {
				ZeroSyncMode = "OffSyncModeAdvancedGuiListItemText"_tr.c_str();
				*ZeroSync_shared = 1;
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
	AdvancedGui() {
		tsl::hlp::doWithSDCardHandle([] {
			configValid = LOCK::readConfig(&configPath[0]);
			if (R_FAILED(configValid)) {
				if (configValid == 0x202) {
					sprintf(&lockInvalid[0], "ConfigFileNotFoundAdvancedGuiCustomDrawerText"_tr.c_str());
				} else sprintf(&lockInvalid[0], "ConfigErrorAdvancedGuiCustomDrawerText"_tr.c_str(), configValid);
			} else {
				patchValid = checkFile(&patchPath[0]);
				if (R_FAILED(patchValid)) {
					if (!FileDownloaded) {
						sprintf(&patchChar[0], "PatchFileNotExistAdvancedGuiCustomDrawerText"_tr.c_str());
					}
					else {
						sprintf(&patchChar[0], "NewConfigDownloadSuccessAdvancedGuiCustomDrawerText"_tr.c_str());
					}
				}
				else sprintf(&patchChar[0], "PatchFileExistAdvancedGuiCustomDrawerText"_tr.c_str());
			}
			switch(*ZeroSync_shared) {
				case 0:
					ZeroSyncMode = "OnSyncModeAdvancedGuiListItemText"_tr.c_str();
					break;
				case 1:
					ZeroSyncMode = "OffSyncModeAdvancedGuiListItemText"_tr.c_str();
					break;
				case 2:
					ZeroSyncMode = "SemiSyncModeAdvancedGuiListItemText"_tr.c_str();
			}
		});
	}

	size_t base_height = 68;

	virtual tsl::elm::Element* createUI() override {
		auto frame = new tsl::elm::OverlayFrame("PluginName"_tr, "AdvancedSettingsAdvancedGuiOverlayFrameSubTitle"_tr);

		auto list = new tsl::elm::List();

		if (configValid == 0x202) {
			base_height = 128;
		} else if (R_SUCCEEDED(configValid)) {
			base_height = 108;
		} else base_height = 68;

		list->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
			if (R_SUCCEEDED(configValid)) {
				renderer->drawString("ConfigFileValidAdvancedGuiCustomDrawerText"_tr.c_str(), false, x, y+20, 20, renderer->a(0xFFFF));
				renderer->drawString(&patchChar[0], false, x, y+40, 20, renderer->a(0xFFFF));
				renderer->drawString(&patchAppliedChar[0], false, x, y+60, 20, renderer->a(0xFFFF));
				renderer->drawString(&nvnBuffers[0], false, x, y+82, 20, renderer->a(0xFFFF));
			} else {
				renderer->drawString(&lockInvalid[0], false, x, y+20, 20, renderer->a(0xFFFF));
				if (configValid == 0x202) {
					renderer->drawString(&nvnBuffers[0], false, x, y+85, 20, renderer->a(0xFFFF));
					renderer->drawString(&patchChar[0], false, x, y+105, 20, renderer->a(0xFFFF));
				} else renderer->drawString(&nvnBuffers[0], false, x, y+40, 20, renderer->a(0xFFFF));
			}
		}), base_height);

		if (*API_shared) {
			switch(*API_shared) {
				case 1: {
					list->addItem(new tsl::elm::CategoryHeader("NVNAdvancedGuiCategoryHeader"_tr, true));
					if (*Buffers_shared == 2 || *SetBuffers_shared == 2 || *ActiveBuffers_shared == 2) {
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
					if (*Buffers_shared > 2) {
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
					list->addItem(new tsl::elm::CategoryHeader("EGLAdvancedGuiCategoryHeader"_tr, true));
					break;
				case 3:
					list->addItem(new tsl::elm::CategoryHeader("VulkanAdvancedGuiCategoryHeader"_tr, true));
			}
		}

		if (R_SUCCEEDED(configValid)) {
			list->addItem(new tsl::elm::CategoryHeader("PatchWillBeAppliedNextGameBootAdvancedGuiCategoryHeader"_tr, true));
			auto *clickableListItem = new tsl::elm::MiniListItem("ConvertConfigToPatchFileAdvancedGuiListItem"_tr);
			clickableListItem->setClickListener([](u64 keys) {
				if ((keys & HidNpadButton_A) && PluginRunning) {
					tsl::hlp::doWithSDCardHandle([] {
						patchValid = LOCK::createPatch(&patchPath[0]);
					});
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
						tsl::hlp::doWithSDCardHandle([] {
							remove(&patchPath[0]);
						});
						patchValid = 0x202;
						sprintf(&patchChar[0], "DeletePatchSuccessfulFileAdvancedGuiListItemText"_tr.c_str());
					}
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem2);
		}

		list->addItem(new tsl::elm::CategoryHeader("RemoveExistPatchFileAdvancedGuiListItemText"_tr, false));
		auto *clickableListItem4 = new tsl::elm::MiniListItem("CheckOrDownloadConfigFileAdvancedGuiListItemText"_tr);
		clickableListItem4->setClickListener([](u64 keys) { 
			if ((keys & HidNpadButton_A) && PluginRunning) {
				tsl::hlp::doWithSDCardHandle([] {
					Result rc = downloadPatch();
					if (rc == 0x212 || rc == 0x312) {
						sprintf(&patchChar[0], "PatchNotAvailableAdvancedGuiListItemText"_tr.c_str(), rc);
					}
					else if (rc == 0x404) {
						sprintf(&patchChar[0], "PatchNotAvailable404AdvancedGuiListItemText"_tr.c_str());
					}
					else if (rc == 0x104) {
						sprintf(&patchChar[0], "NoNewConfigAvailableAdvancedGuiListItemText"_tr.c_str());
					}
					else if (rc == 0x412) {
						sprintf(&patchChar[0], "InternetConnectionNotAvailableAdvancedGuiListItemText"_tr.c_str());
					}
					else if (R_SUCCEEDED(rc)) {
						FILE* fp = fopen(patchPath, "rb");
						if (fp) {
							fclose(fp);
							remove(patchPath);
						}
						tsl::goBack();
						tsl::changeTo<AdvancedGui>();
					}
					else {
						sprintf(&patchChar[0], "PatchDownloadFailedAdvancedGuiListItemText"_tr.c_str(), rc);
					}
				});
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
				if (*patchApplied_shared == 1) {
					sprintf(patchAppliedChar, "PluginLoadedUpdateAdvancedGuiCustomDrawerText"_tr.c_str());
				} else if (*patchApplied_shared == 2) {
					sprintf(patchAppliedChar, "MasterWriteLoadedUpdateAdvancedGuiCustomDrawerText"_tr.c_str());
				} else sprintf(patchAppliedChar, "PluginNotApplyUpdateAdvancedGuiCustomDrawerText"_tr.c_str());
				if (*API_shared == 1) {
					if ((*Buffers_shared >= 2 && *Buffers_shared <= 4)) {
						sprintf(&nvnBuffers[0], "SetOrActiveOrAvailableBuffersUpdateAdvancedGuiCustomDrawerText"_tr.c_str(), *SetActiveBuffers_shared, *ActiveBuffers_shared, *Buffers_shared);
					}
				}
				i = 0;
			}
			else i++;
		}
	}
};

class NoGameSub : public tsl::Gui {
public:
	uint64_t _titleid = 0;
	char _titleidc[17] = "";
	std::string _titleName = "";

	NoGameSub(uint64_t titleID, std::string titleName) {
		_titleid = titleID;
		sprintf(&_titleidc[0], "%016lX", _titleid);
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

		auto *clickableListItem = new tsl::elm::ListItem("DeleteSettingsNoGameSubListItem"_tr);
		clickableListItem->setClickListener([this](u64 keys) {
			if (keys & HidNpadButton_A) {
				tsl::hlp::doWithSDCardHandle([this] {
					char path[512] = "";
					if (_titleid != 0x1234567890ABCDEF) {
						sprintf(&path[0], "sdmc:/SaltySD/plugins/FPSLocker/%016lx.dat", _titleid);
						remove(path);
					}
					else {
						sprintf(&path[0], "sdmc:/SaltySD/plugins/FPSLocker/");

						struct dirent *entry;
						DIR *dp;
						dp = opendir(path);
						if (!dp)
							return;
						while ((entry = readdir(dp))) {
							if (entry -> d_type != DT_DIR && std::string(entry -> d_name).find(".dat") != std::string::npos) {
								sprintf(&path[0], "sdmc:/SaltySD/plugins/FPSLocker/%s", entry->d_name);
								remove(path);
							}
						}
						closedir(dp);
					}
				});
				return true;
			}
			return false;
		});

		list->addItem(clickableListItem);

		auto *clickableListItem2 = new tsl::elm::ListItem("DeletePatchesNoGameSubListItem"_tr);
		clickableListItem2->setClickListener([this](u64 keys) {
			if (keys & HidNpadButton_A) {
				tsl::hlp::doWithSDCardHandle([this] {
					char folder[640] = "";
					if (_titleid != 0x1234567890ABCDEF) {
						sprintf(&folder[0], "sdmc:/SaltySD/plugins/FPSLocker/patches/%016lx/", _titleid);

						struct dirent *entry;
						DIR *dp;

						dp = opendir(folder);
						if (!dp)
							return;
						while ((entry = readdir(dp))) {
							if (entry -> d_type != DT_DIR && std::string(entry -> d_name).find(".bin") != std::string::npos) {
								sprintf(&folder[0], "sdmc:/SaltySD/plugins/FPSLocker/patches/%016lx/%s", _titleid, entry -> d_name);
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

						sprintf(&folder[0], "sdmc:/SaltySD/plugins/FPSLocker/patches/");
						dp = opendir(folder);
						if (!dp)
							return;
						while ((entry = readdir(dp))) {
							if (entry -> d_type != DT_DIR)
								continue;
							sprintf(&folder[0], "sdmc:/SaltySD/plugins/FPSLocker/patches/%s/", entry -> d_name);
							dp2 = opendir(folder);
							while ((entry2 = readdir(dp2))) {
								if (entry2 -> d_type != DT_DIR && std::string(entry2 -> d_name).find(".bin") != std::string::npos) {
									sprintf(&folder[0], "sdmc:/SaltySD/plugins/FPSLocker/patches/%s/%s", entry -> d_name, entry2 -> d_name);
									remove(folder);
								}
							}
							closedir(dp2);
						}
						closedir(dp);
					}
				});
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

		if (oldSalty || isOLED || !SaltySD) {
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
			sprintf(&error[0], "ErrorNoGame2ListItem"_tr.c_str(), rc);
			auto *clickableListItem2 = new tsl::elm::ListItem(error);
			clickableListItem2->setClickListener([](u64 keys) {
				if (keys & HidNpadButton_A) {
					return true;
				}
				return false;
			});

			list->addItem(clickableListItem2);
		}
		else {
			auto *clickableListItem3 = new tsl::elm::ListItem("AllNoGame2ListItem"_tr);
			clickableListItem3->setClickListener([](u64 keys) {
				if (keys & HidNpadButton_A) {
					tsl::changeTo<NoGameSub>(0x1234567890ABCDEF, "EverythingNoGame2ListItemText"_tr);
					return true;
				}
				return false;
			});

			list->addItem(clickableListItem3);

			for (size_t i = 0; i < titles.size(); i++) {
				auto *clickableListItem = new tsl::elm::ListItem(titles[i].TitleName);
				clickableListItem->setClickListener([i](u64 keys) {
					if (keys & HidNpadButton_A) {
						tsl::changeTo<NoGameSub>(titles[i].TitleID, titles[i].TitleName);
						return true;
					}
					return false;
				});

				list->addItem(clickableListItem);
			}
		}

		frame->setContent(list);

		return frame;
	}

	virtual void update() override {}

	// Called once every frame to handle inputs not handled by other UI elements
	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		return false;   // Return true here to singal the inputs have been consumed
	}
};

class DisplayGui : public tsl::Gui {
private:
	char refreshRate_c[32] = "";
	uint8_t refreshRate = 0;
public:
    DisplayGui() {
		apmGetPerformanceMode(&performanceMode);
		if (performanceMode == ApmPerformanceMode_Boost) {
			isDocked = true;
		}
		else if (performanceMode == ApmPerformanceMode_Normal) {
			isDocked = false;
		}
		if (!isDocked && R_SUCCEEDED(SaltySD_Connect())) {
			SaltySD_GetDisplayRefreshRate(&refreshRate);
			svcSleepThread(100'000);
			SaltySD_Term();
			refreshRate_g = refreshRate;
		}
	}

	size_t base_height = 128;

    virtual tsl::elm::Element* createUI() override {
        auto frame = new tsl::elm::OverlayFrame("PluginName"_tr, "DisplayGuiOverlayFrameText"_tr);

		auto list = new tsl::elm::List();

		list->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {

			renderer->drawString(this -> refreshRate_c, false, x, y+20, 20, renderer->a(0xFFFF));
		}), 50);

		if (!displaySync) {

			auto *clickableListItem = new tsl::elm::ListItem("IncreaseRefreshRateDisplayGuiListItem"_tr);
			clickableListItem->setClickListener([this](u64 keys) { 
				if ((keys & HidNpadButton_A) && !isDocked) {
					if ((this -> refreshRate >= 40) && (this -> refreshRate < 60)) {
						this -> refreshRate += 5;
						if (!isDocked && R_SUCCEEDED(SaltySD_Connect())) {
							SaltySD_SetDisplayRefreshRate(this -> refreshRate);
							svcSleepThread(100'000);
							SaltySD_GetDisplayRefreshRate(&(this -> refreshRate));
							if (displaySync_shared)
								*displaySync_shared = this -> refreshRate;
							SaltySD_Term();
							refreshRate_g = this -> refreshRate;
						}
					}
					return true;
				}
				return false;
			});

			list->addItem(clickableListItem);

			auto *clickableListItem2 = new tsl::elm::ListItem("DecreaseRefreshRateDisplayGuiListItem"_tr);
			clickableListItem2->setClickListener([this](u64 keys) { 
				if ((keys & HidNpadButton_A) && !isDocked) {
					if (this -> refreshRate > 40) {
						this -> refreshRate -= 5;
						if (!isDocked && R_SUCCEEDED(SaltySD_Connect())) {
							SaltySD_SetDisplayRefreshRate(this -> refreshRate);
							svcSleepThread(100'000);
							SaltySD_GetDisplayRefreshRate(&(this -> refreshRate));
							if (displaySync_shared)
								*displaySync_shared = this -> refreshRate;
							SaltySD_Term();
							refreshRate_g = this -> refreshRate;
						}
					}
					return true;
				}
				return false;
			});

			list->addItem(clickableListItem2);
		}

		if (!oldSalty) {
			list->addItem(new tsl::elm::CategoryHeader("MatchRefreshRateCategoryHeader"_tr, true));
			auto *clickableListItem3 = new tsl::elm::ToggleListItem("DisplaySyncToggleListItem"_tr, displaySync);
			clickableListItem3->setClickListener([this](u64 keys) { 
				if (keys & HidNpadButton_A) {
					if (R_SUCCEEDED(SaltySD_Connect())) {
						SaltySD_SetDisplaySync(!displaySync);
						svcSleepThread(100'000);
						u64 PID = 0;
						Result rc = pmdmntGetApplicationProcessId(&PID);
						if (!isDocked && R_SUCCEEDED(rc) && *FPSlocked_shared) {
							if (!displaySync == true && *FPSlocked_shared < 40) {
								SaltySD_SetDisplayRefreshRate(60);
								*displaySync_shared = 0;
								refreshRate_g = 0;
							}
							else if (!displaySync == true) {
								SaltySD_SetDisplayRefreshRate(*FPSlocked_shared);
								*displaySync_shared = *FPSlocked_shared;
								refreshRate_g = *FPSlocked_shared;
							}
							else {
								*displaySync_shared = 0;
								refreshRate_g = 0;
							}
						}
						else if (!isDocked && !displaySync == true && (R_FAILED(rc) || !PluginRunning)) {
							SaltySD_SetDisplayRefreshRate(60);
							refreshRate_g = 0;
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
		}

		frame->setContent(list);

        return frame;
    }

	virtual void update() override {
		apmGetPerformanceMode(&performanceMode);
		if (performanceMode == ApmPerformanceMode_Boost) {
			isDocked = true;
		}
		else if (performanceMode == ApmPerformanceMode_Normal) {
			isDocked = false;
		}
		if (!isDocked)
			snprintf(refreshRate_c, sizeof(refreshRate_c), "LCDRefreshRateUpdateDisplayGuiCustomDrawerText"_tr.c_str(), refreshRate);
		else strncpy(refreshRate_c, "NotAvailableInDockedModeUpdateDisplayGuiCustomDrawerText"_tr.c_str(), 30);
	}
};

class WarningDisplayGui : public tsl::Gui {
private:
	uint8_t refreshRate = 0;
	std::string Warning = "WarningWarningDisplayGuiCustomDrawerText"_tr.c_str();
	std::string Docked = "DockedWarningDisplayGuiCustomDrawerText"_tr.c_str();
public:
    WarningDisplayGui() {
		apmGetPerformanceMode(&performanceMode);
		if (performanceMode == ApmPerformanceMode_Boost) {
			isDocked = true;
		}
		else if (performanceMode == ApmPerformanceMode_Normal) {
			isDocked = false;
		}
	}

	size_t base_height = 128;

    virtual tsl::elm::Element* createUI() override {
        auto frame = new tsl::elm::OverlayFrame("PluginName"_tr, "DisplaySettingWarningWarningDisplayGuiOverlayFrameText"_tr);

		auto list = new tsl::elm::List();

		list->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
			if (!isDocked)
				renderer->drawString(Warning.c_str(), false, x, y+20, 20, renderer->a(0xFFFF));
			else renderer->drawString(Docked.c_str(), false, x, y+20, 20, renderer->a(0xFFFF));
		}), 200);

		auto *clickableListItem1 = new tsl::elm::ListItem("DeclineWarningDisplayGuiListItem"_tr);
		clickableListItem1->setClickListener([this](u64 keys) { 
			if (keys & HidNpadButton_A) {
				tsl::goBack();
				return true;
			}
			return false;
		});

		list->addItem(clickableListItem1);

		auto *clickableListItem2 = new tsl::elm::ListItem("AcceptWarningDisplayGuiListItem"_tr);
		clickableListItem2->setClickListener([this](u64 keys) { 
			if ((keys & HidNpadButton_A) && !isDocked) {
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

		auto *clickableListItem2 = new tsl::elm::ListItem("GameListNoGameListItem"_tr);
		clickableListItem2->setClickListener([this](u64 keys) { 
			if (keys & HidNpadButton_A) {
				tsl::changeTo<NoGame2>(this -> rc, 2, true);
				return true;
			}
			return false;
		});

		list->addItem(clickableListItem2);

		auto *clickableListItem3 = new tsl::elm::ListItem("DisplaySettingsNoGameListItem"_tr, "\uE151");
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

class GuiTest : public tsl::Gui {
public:
	GuiTest(u8 arg1, u8 arg2, bool arg3) { }

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
				renderer->drawString("SaltNXNotWorkingGuiTestCustomDrawerText"_tr.c_str(), false, x, y+50, 20, renderer->a(0xF33F));
			}
			else if (!plugin) {
				renderer->drawString("SaltNXNotFoundGuiTestCustomDrawerText"_tr.c_str(), false, x, y+50, 20, renderer->a(0xF33F));
			}
			else if (!check) {
				if (closed) {
					renderer->drawString("GameClosedGuiTestCustomDrawerText"_tr.c_str(), false, x, y+20, 19, renderer->a(0xF33F));
				}
				else {
					renderer->drawString("GameNotRunningGuiTestCustomDrawerText"_tr.c_str(), false, x, y+20, 19, renderer->a(0xF33F));
				}
			}
			else if (!PluginRunning) {
				renderer->drawString("GameRunningGuiTestCustomDrawerText"_tr.c_str(), false, x, y+20, 20, renderer->a(0xFFFF));
				renderer->drawString("NXFPSNotRunningGuiTestCustomDrawerText"_tr.c_str(), false, x, y+40, 20, renderer->a(0xF33F));
			}
			else if (!*pluginActive) {
				renderer->drawString("NXFPSRunningNoFrameProcessedGuiTestCustomDrawerText"_tr.c_str(), false, x, y+20, 20, renderer->a(0xF33F));
				renderer->drawString("RestartOverlayToCheckGuiTestCustomDrawerText"_tr.c_str(), false, x, y+50, 20, renderer->a(0xFFFF));
			}
			else {
				renderer->drawString("NXFPSRunningGuiTestCustomDrawerText"_tr.c_str(), false, x, y+20, 20, renderer->a(0xFFFF));
				if ((*API_shared > 0) && (*API_shared <= 2))
					renderer->drawString(FPSMode_c, false, x, y+40, 20, renderer->a(0xFFFF));
				renderer->drawString(FPSTarget_c, false, x, y+60, 20, renderer->a(0xFFFF));
				renderer->drawString(PFPS_c, false, x+290, y+48, 50, renderer->a(0xFFFF));
			}
		}), 90);

		if (PluginRunning && *pluginActive) {
			auto *clickableListItem = new tsl::elm::ListItem("IncreaseFPSGuiTestListItem"_tr);
			clickableListItem->setClickListener([](u64 keys) {
				if ((keys & HidNpadButton_A) && PluginRunning) {
					if (*FPSmode_shared == 2 && !*FPSlocked_shared) {
						*FPSlocked_shared = 35;
					}
					else if (!*FPSlocked_shared) {
						*FPSlocked_shared = 60;
					}
					else if (*FPSlocked_shared < 60) {
						*FPSlocked_shared += 5;
					}
					if (!isDocked && !oldSalty && displaySync) {
						if (R_SUCCEEDED(SaltySD_Connect())) {
							if (*FPSlocked_shared >= 40) {
								SaltySD_SetDisplayRefreshRate(*FPSlocked_shared);
								refreshRate_g = *FPSlocked_shared;
							}
							else {
								SaltySD_SetDisplayRefreshRate(60);
								refreshRate_g = 60;
							}
							*displaySync_shared = refreshRate_g;
							SaltySD_Term();
						}
					}
					return true;
				}
				return false;
			});

			list->addItem(clickableListItem);
			
			auto *clickableListItem2 = new tsl::elm::ListItem("DecreaseFPSGuiTestListItem"_tr);
			clickableListItem2->setClickListener([](u64 keys) {
				if ((keys & HidNpadButton_A) && PluginRunning) {
					if (*FPSmode_shared < 2 && !*FPSlocked_shared) {
						*FPSlocked_shared = 55;
					}
					else if (!*FPSlocked_shared) {
						*FPSlocked_shared = 25;
					}
					else if (*FPSlocked_shared > 15) {
						*FPSlocked_shared -= 5;
					}
					if (!isDocked && !oldSalty && displaySync) {
						if (R_SUCCEEDED(SaltySD_Connect())) {
							if (*FPSlocked_shared >= 40) {
								SaltySD_SetDisplayRefreshRate(*FPSlocked_shared);
								refreshRate_g = *FPSlocked_shared;
							}
							else {
								SaltySD_SetDisplayRefreshRate(60);
								refreshRate_g = 60;
							}
							SaltySD_Term();
							*displaySync_shared = refreshRate_g;
						}
					}
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem2);

			auto *clickableListItem4 = new tsl::elm::ListItem("DisableFPSGuiTestListItem"_tr);
			clickableListItem4->setClickListener([](u64 keys) {
				if ((keys & HidNpadButton_A) && PluginRunning) {
					if (*FPSlocked_shared) {
						*FPSlocked_shared = 0;
					}
					if (displaySync) {
						if (!oldSalty && R_SUCCEEDED(SaltySD_Connect())) {
							SaltySD_SetDisplayRefreshRate(60);
							*displaySync_shared = 0;
							SaltySD_Term();
						}
					}
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem4);

			auto *clickableListItem3 = new tsl::elm::ListItem("AdvancedSettingsGuiTestListItem"_tr);
			clickableListItem3->setClickListener([](u64 keys) {
				if ((keys & HidNpadButton_A) && PluginRunning) {
					tsl::changeTo<AdvancedGui>();
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem3);

			auto *clickableListItem5 = new tsl::elm::ListItem("SaveSettingsGuiTestListItem"_tr);
			clickableListItem5->setClickListener([](u64 keys) {
				if ((keys & HidNpadButton_A) && PluginRunning) {
					tsl::hlp::doWithSDCardHandle([] {
						if (!*FPSlocked_shared && !*ZeroSync_shared && !SetBuffers_save) {
							remove(savePath);
						}
						else {
							DIR* dir = opendir("sdmc:/SaltySD/plugins/");
							if (!dir) {
								mkdir("sdmc:/SaltySD/plugins/", 777);
							}
							else closedir(dir);
							dir = opendir("sdmc:/SaltySD/plugins/FPSLocker/");
							if (!dir) {
								mkdir("sdmc:/SaltySD/plugins/FPSLocker/", 777);
							}
							else closedir(dir);
							FILE* file = fopen(savePath, "wb");
							if (file) {
								fwrite(FPSlocked_shared, 1, 1, file);
								if (SetBuffers_save > 2 || (!SetBuffers_save && *Buffers_shared > 2)) {
									*ZeroSync_shared = 0;
								}
								fwrite(ZeroSync_shared, 1, 1, file);
								if (SetBuffers_save) {
									fwrite(&SetBuffers_save, 1, 1, file);
								}
								fclose(file);
							}
						}
					});
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem5);
		}

		if (!isOLED && SaltySD) {
			auto *clickableListItem6 = new tsl::elm::ListItem("DisplaySettingsGuiTestListItem"_tr, "\uE151");
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
				apmGetPerformanceMode(&performanceMode);
				if (performanceMode == ApmPerformanceMode_Boost) {
					isDocked = true;
					refreshRate_g = 60;
				}
				else if (performanceMode == ApmPerformanceMode_Normal) {
					isDocked = false;
				}
				switch (*FPSmode_shared) {
					case 0:
						//This is usually a sign that game doesn't use interval
						sprintf(FPSMode_c, "IntervalModeUnusedUpdateGuiTestListItem"_tr.c_str());
						break;
					case 1 ... 5:
						sprintf(FPSMode_c, "IntervalModeFPSUpdateGuiTestListItem"_tr.c_str(), *FPSmode_shared, refreshRate_g / *FPSmode_shared);
						break;
					default:
						sprintf(FPSMode_c, "IntervalModeWrongUpdateGuiTestListItem"_tr.c_str(), *FPSmode_shared);
				}
				if (!*FPSlocked_shared) {
					sprintf(FPSTarget_c, "CustomFPSDisabledUpdateGuiTestListItem"_tr.c_str());
				}
				else sprintf(FPSTarget_c, "CustomFPSUpdateGuiTestListItem"_tr.c_str(), *FPSlocked_shared);
				sprintf(PFPS_c, "%d", *FPS_shared);
				i = 0;
			}
			else i++;
		}
	}

	// Called once every frame to handle inputs not handled by other UI elements
	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		return false;   // Return true here to singal the inputs have been consumed
	}
};

class OverlayTest : public tsl::Overlay {
public:
	// libtesla already initialized fs, hid, pl, pmdmnt, hid:sys and set:sys
	virtual void initServices() override {
		std::string jsonStr = R"(
			{
				"PluginName": "FPSLocker",
				"NVNSetBufferingSetBuffersOverlayFrame": "NVN Set Buffering",
				"AppliedNextGameBootSetBuffersListItemCategoryHeader": "It will be applied on next game boot.",
				"RememberSaveSettingsSetBuffersListItemCategoryHeader": "Remember to save settings after change.",
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
				"NewConfigDownloadSuccessAdvancedGuiCustomDrawerText": "New config downloaded successfully.",
				"PatchFileExistAdvancedGuiCustomDrawerText": "Patch file exists.",
				"AdvancedSettingsAdvancedGuiOverlayFrameSubTitle": "Advanced settings",
				"ConfigFileValidAdvancedGuiCustomDrawerText": "Found valid config file!",
				"NVNAdvancedGuiCategoryHeader": "NVN",
				"SyncWaitAdvancedGuiListItem": "Window Sync Wait",
				"SetBufferingAdvancedGuiListItem": "Set Buffering",
				"EGLAdvancedGuiCategoryHeader": "EGL",
				"VulkanAdvancedGuiCategoryHeader": "Vulkan",
				"PatchWillBeAppliedNextGameBootAdvancedGuiCategoryHeader": "It will be applied on next game boot",
				"ConvertConfigToPatchFileAdvancedGuiListItem": "Convert config to patch file",
				"PatchFileCreatedSuccessAdvancedGuiListItemText": "Patch file created successfully.",
				"PatchFileCreateFailedAdvancedGuiListItemText": "Error while creating patch: 0x%x",
				"DeletePatchFileAdvancedGuiListItem": "Delete patch file",
				"DeletePatchSuccessfulFileAdvancedGuiListItemText": "Patch file deleted successfully.",
				"RemoveExistPatchFileAdvancedGuiListItemText": "If exists, it will also remove existing patch file.",
				"CheckOrDownloadConfigFileAdvancedGuiListItemText": "Check/download config file",
				"PatchNotAvailableAdvancedGuiListItemText": "Patch is not available! RC: 0x%x",
				"PatchNotAvailable404AdvancedGuiListItemText": "Patch is not available! Err 404",
				"NoNewConfigAvailableAdvancedGuiListItemText": "No new config available.",
				"InternetConnectionNotAvailableAdvancedGuiListItemText": "Internet connection not available!",
				"PatchDownloadFailedAdvancedGuiListItemText": "Patch downloading failed! RC: 0x%x",
				"PluginLoadedUpdateAdvancedGuiCustomDrawerText": "Patch was loaded to game",
				"MasterWriteLoadedUpdateAdvancedGuiCustomDrawerText": "Master Write was loaded to game",
				"PluginNotApplyUpdateAdvancedGuiCustomDrawerText": "Plugin didn't apply patch to game",
				"SetOrActiveOrAvailableBuffersUpdateAdvancedGuiCustomDrawerText": "Set/Active/Available buffers: %d/%d/%d",
				"DeleteSettingsNoGameSubListItem": "Delete settings",
				"DeletePatchesNoGameSubListItem": "Delete patches",
				"SaltyNXNotWorkingNoGame2CustomDrawerText": "SaltyNX is not working!",
				"SaltyNXNotFoundNoGame2CustomDrawerText": "Can't detect NX-FPS plugin on sdcard!",
				"GameNotRunningNoGame2CustomDrawerText": "Game is not running!",
				"ErrorNoGame2ListItem": "Err: 0x%x",
				"AllNoGame2ListItem": "All",
				"EverythingNoGame2ListItemText": "Everything",
				"DisplayGuiOverlayFrameText": "Display settings",
				"IncreaseRefreshRateDisplayGuiListItem": "Increase Refresh Rate",
				"DecreaseRefreshRateDisplayGuiListItem": "Decrease Refresh Rate",
				"MatchRefreshRateCategoryHeader": "Match refresh rate with FPS Target.",
				"DisplaySyncToggleListItem": "Display Sync",
				"LCDRefreshRateUpdateDisplayGuiCustomDrawerText": "LCD Refresh Rate: %d Hz",
				"NotAvailableInDockedModeUpdateDisplayGuiCustomDrawerText": "Not available in docked mode!",
				"WarningWarningDisplayGuiCustomDrawerText": "THIS IS EXPERIMENTAL FUNCTION!\n\nIt can cause irreparable damage\nto your display.\n\nBy pressing Accept you are taking\nfull responsibility for anything\nthat can occur because of this tool.",
				"DockedWarningDisplayGuiCustomDrawerText": "This function is not available\nin docked mode!\n\nAccept button is disabled.",
				"DisplaySettingWarningWarningDisplayGuiOverlayFrameText": "Display settings warning",
				"DeclineWarningDisplayGuiListItem": "Decline",
				"AcceptWarningDisplayGuiListItem": "Accept",
				"SaltyNXNotWorkingNoGameCustomDrawerText": "SaltyNX is not working!",
				"SaltyNXNotFoundNoGameCustomDrawerText": "Can't detect NX-FPS plugin on sdcard!",
				"GameNotRunningNoGameCustomDrawerText": "Game is not running!",
				"GameListNoGameListItem": "Games list",
				"DisplaySettingsNoGameListItem": "Display settings",
				"SaltNXNotWorkingGuiTestCustomDrawerText": "SaltyNX is not working!",
				"SaltNXNotFoundGuiTestCustomDrawerText": "Can't detect NX-FPS plugin on sdcard!",
				"GameClosedGuiTestCustomDrawerText": "Game was closed! Overlay disabled!",
				"GameNotRunningGuiTestCustomDrawerText": "Game is not running! Overlay disabled!",
				"GameRunningGuiTestCustomDrawerText": "Game is running.",
				"NXFPSNotRunningGuiTestCustomDrawerText": "NX-FPS is not running!",
				"NXFPSRunningNoFrameProcessedGuiTestCustomDrawerText": "NX-FPS is running, but no frame was processed.",
				"RestartOverlayToCheckGuiTestCustomDrawerText": "Restart overlay to check again.",
				"NXFPSRunningGuiTestCustomDrawerText": "NX-FPS is running.",
				"IncreaseFPSGuiTestListItem": "Increase FPS target",
				"DecreaseFPSGuiTestListItem": "Decrease FPS target",
				"DisableFPSGuiTestListItem": "Disable custom FPS target",
				"AdvancedSettingsGuiTestListItem": "Advanced settings",
				"SaveSettingsGuiTestListItem": "Save settings",
				"DisplaySettingsGuiTestListItem": "Display settings",
				"IntervalModeUnusedUpdateGuiTestListItem": "Interval Mode: 0 [Unused]",
				"IntervalModeFPSUpdateGuiTestListItem": "Interval Mode: %d [%d FPS]",
				"IntervalModeWrongUpdateGuiTestListItem": "Interval Mode: %d [Wrong]",
				"CustomFPSDisabledUpdateGuiTestListItem": "Custom FPS Target: Disabled",
				"CustomFPSUpdateGuiTestListItem": "Custom FPS Target: %d"
			}
		)";
		std::string lanPath = std::string("sdmc:/switch/.overlays/lang/") + APPTITLE + "/";
		fsdevMountSdmc();
		tsl::hlp::doWithSmSession([&lanPath, &jsonStr]{
			tsl::tr::InitTrans(lanPath, jsonStr);
		});
		fsdevUnmountDevice("sdmc");

		tsl::hlp::doWithSmSession([]{
			apmInitialize();
			setsysInitialize();
			SetSysProductModel model;
			if (R_SUCCEEDED(setsysGetProductModel(&model))) {
				if (model == SetSysProductModel_Aula) {
					isOLED = true;
					tsl::hlp::doWithSDCardHandle([] {
						remove("sdmc:/SaltySD/flags/displaysync.flag");
					});
				}
			}
			FILE* file = NULL;
			tsl::hlp::doWithSDCardHandle([&file] {
				file = fopen("sdmc:/SaltySD/flags/displaysync.flag", "rb");
			});
			if (file) {
				displaySync = true;
				fclose(file);
			}
			setsysExit();
			SaltySD = CheckPort();
			if (!SaltySD) return;

			if (R_SUCCEEDED(SaltySD_Connect())) {
				if (R_FAILED(SaltySD_GetDisplayRefreshRate(&refreshRate_g))) {
					refreshRate_g = 60;
					oldSalty = true;
				}
				svcSleepThread(100'000);
				SaltySD_Term();
			}

			if (R_FAILED(pmdmntGetApplicationProcessId(&PID))) return;
			check = true;
			
			if(!LoadSharedMemory()) return;

			uintptr_t base = (uintptr_t)shmemGetAddr(&_sharedmemory);
			ptrdiff_t rel_offset = searchSharedMemoryBlock(base);
			if (rel_offset > -1)
				displaySync_shared = (uint8_t*)(base + rel_offset + 59);

			if (!PluginRunning) {
				if (rel_offset > -1) {
					pminfoInitialize();
					pminfoGetProgramId(&TID, PID);
					pminfoExit();
					BID = getBID();
					sprintf(&patchPath[0], "sdmc:/SaltySD/plugins/FPSLocker/patches/%016lX/%016lX.bin", TID, BID);
					sprintf(&configPath[0], "sdmc:/SaltySD/plugins/FPSLocker/patches/%016lX/%016lX.yaml", TID, BID);
					sprintf(&savePath[0], "sdmc:/SaltySD/plugins/FPSLocker/%016lX.dat", TID);

					FPS_shared = (uint8_t*)(base + rel_offset + 4);
					pluginActive = (bool*)(base + rel_offset + 9);
					FPSlocked_shared = (uint8_t*)(base + rel_offset + 10);
					FPSmode_shared = (uint8_t*)(base + rel_offset + 11);
					ZeroSync_shared = (uint8_t*)(base + rel_offset + 12);
					patchApplied_shared = (uint8_t*)(base + rel_offset + 13);
					API_shared = (uint8_t*)(base + rel_offset + 14);
					Buffers_shared = (uint8_t*)(base + rel_offset + 55);
					SetBuffers_shared = (uint8_t*)(base + rel_offset + 56);
					ActiveBuffers_shared = (uint8_t*)(base + rel_offset + 57);
					SetActiveBuffers_shared = (uint8_t*)(base + rel_offset + 58);
					SetBuffers_save = *SetBuffers_shared;
					PluginRunning = true;
					threadCreate(&t0, loopThread, NULL, NULL, 0x1000, 0x20, 0);
					threadStart(&t0);
				}		
			}
		
		});
	
	}  // Called at the start to initialize all services necessary for this Overlay
	
	virtual void exitServices() override {
		threadActive = false;
		threadWaitForExit(&t0);
		threadClose(&t0);
		shmemClose(&_sharedmemory);
		nsExit();
		apmExit();
	}  // Callet at the end to clean up all services previously initialized

	virtual void onShow() override {}    // Called before overlay wants to change from invisible to visible state
	
	virtual void onHide() override {}    // Called before overlay wants to change from visible to invisible state

	virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
		if (SaltySD && check && plugin) {
			return initially<GuiTest>(1, 2, true);  // Initial Gui to load. It's possible to pass arguments to it's constructor like this
		}
		else {
			tsl::hlp::doWithSmSession([]{
				nsInitialize();
			});
			Result rc = getTitles(32);
			if (oldSalty || isOLED || !SaltySD)
				return initially<NoGame2>(rc, 2, true);
			else return initially<NoGame>(rc, 2, true);
		}
	}
};

int main(int argc, char **argv) {
	return tsl::loop<OverlayTest>(argc, argv);
}
