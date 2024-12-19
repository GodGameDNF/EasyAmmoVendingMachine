#include <algorithm>  // std::shuffle
#include <chrono>     // std::chrono::system_clock
#include <cmath>      // for sin, cos
#include <cstdlib>    // for rand
#include <ctime>      // for time
#include <iostream>
#include <random>  // std::default_random_engine
#include <regex>
#include <string>
#include <vector>
#include <iomanip>  // std::hex, std::uppercase
#include <unordered_set>

using namespace RE;

PlayerCharacter* p = nullptr;
BSScript::IVirtualMachine* vm = nullptr;
TESDataHandler* DH = nullptr;

TESGlobal* gAmmoStock = nullptr;

BGSListForm* sellRoundsMult_1_5_List = nullptr;
BGSListForm* sellRoundsMult_0_5_List = nullptr;
BGSListForm* sellRoundsForcedList = nullptr;

std::unordered_set<BGSMod::Attachment::Mod*> saveMod;  // 중복 방지와 빠른 탐색을 위한 unordered_set 사용

struct FormOrInventoryObj
{
	TESForm* form{ nullptr };  // TESForm 포인터를 가리키는 포인터
	uint64_t second_arg{ 0 };  // unsigned 64비트 정수
};

bool AddItemVM(BSScript::IVirtualMachine* vm, uint32_t i, TESObjectREFR* target, FormOrInventoryObj obj, uint32_t count, bool b1)
{
	using func_t = decltype(&AddItemVM);
	REL::Relocation<func_t> func{ REL::ID(1212351) };
	return func(vm, i, target, obj, count, b1);
}

void RemoveAllItems(BSScript::IVirtualMachine* vm, uint32_t i, TESObjectREFR* del, TESObjectREFR* send, bool bMove)
{
	using func_t = decltype(&RemoveAllItems);
	REL::Relocation<func_t> func{ REL::ID(534058) };
	return func(vm, i, del, send, bMove);
}

void injectAmmo(std::monostate)
{
	TESObjectREFR* sellerChest = (TESObjectREFR*)DH->LookupForm(0x01b, "EasyAmmoVendingMachine.esp");
	if (!sellerChest) {
		return;
	}


	//logger::info("템 갯수 f4se {}", itemArray.size());

	RemoveAllItems(vm, 0, sellerChest, nullptr, false);
	std::vector<TESAmmo*> sellAmmoArray;

	BGSInventoryList* iList = p->inventoryList;
	BSTArray<BGSInventoryItem> itemArray = iList->data;
	int itemArraySize = itemArray.size();

	// 플레이어의 소지품에서 무기를 확인하고 그 무기가 사용하는 탄약을 배열에 더함
	for (int i = 0; i < itemArraySize; ++i) {
		BGSInventoryItem tempInvenItem = itemArray[i];

		TESForm* tempForm = tempInvenItem.object;

		if (tempForm && tempForm->formType == ENUM_FORM_ID::kWEAP) {
			TESObjectWEAP* weapon = (TESObjectWEAP*)tempForm;
			if (weapon->GetPlayable(weapon->GetBaseInstanceData())) {

				BGSInventoryItem tempInvenItem = itemArray[i];
				TESAmmo* targetAmmo = weapon->weaponData.ammo;

				// 스택 데이터 가져오기
				auto stack = tempInvenItem.stackData.get();

				while (stack) {
					if (stack->extra) {
						// extra 데이터를 통해 모드 정보를 탐색
						ExtraDataList* extraData = stack->extra.get();
						if (extraData) {
							// 모드 관련 데이터를 찾아봄 (예: kExtraDataMod)
							auto modData = extraData->GetByType(kObjectInstance);
							if (modData) {
								// 모드 데이터를 BGSMod::Attachment::Mod로 캐스팅
								//BGSMod::Attachment::Mod* mod = (BGSMod::Attachment::Mod*)modData;
								BGSObjectInstanceExtra* iData = (BGSObjectInstanceExtra*)modData;

								for (const auto& tMod : saveMod) {
									if (iData->HasMod(*tMod)) {
										if (!tMod)
											continue;

										BGSMod::Attachment::Mod::Data modData;
										tMod->GetData(modData);

										for (std::uint32_t i = 0; i < modData.propertyModCount; ++i) {
											BGSMod::Property::Mod* propertyMod = &modData.propertyMods[i];

											if (propertyMod) {
												if (propertyMod->op == BGSMod::Property::OP::kSet && propertyMod->type == BGSMod::Property::TYPE::kForm) {
													TESForm* tForm = propertyMod->data.form;
													if (tForm && tForm->formType == ENUM_FORM_ID::kAMMO) {
														targetAmmo = (TESAmmo*)tForm;
														break;
													}
												}
											}
										}
									}
								}
							}
						}
					}
					// 다음 스택으로 이동
					stack = stack->nextStack.get();
				}

				if (targetAmmo && std::find(sellAmmoArray.begin(), sellAmmoArray.end(), targetAmmo) == sellAmmoArray.end()) {
					sellAmmoArray.push_back(targetAmmo);
				}
			}
		}
	}

	// 강제로 팔 리스트에 들어있으면 판매목록에 끼워넣음
	if (sellRoundsForcedList && !sellRoundsForcedList->arrayOfForms.empty()) {
		for (TESForm* sellForcedAmmo : sellRoundsForcedList->arrayOfForms) {
			if (sellForcedAmmo->formType != ENUM_FORM_ID::kAMMO) {
				continue;
			}

			bool existAmmo = false;
			for (TESAmmo* ammo : sellAmmoArray) {
				if (ammo == sellForcedAmmo) {
					existAmmo = true;
					break;
				}
			}

			if (!existAmmo) {
				sellAmmoArray.push_back((TESAmmo*)sellForcedAmmo);
			}
		}
	}

	// 글로벌로 설정된 판매량
	uint32_t ammoStock = gAmmoStock->value;

	for (TESAmmo* ammo : sellAmmoArray) {
		uint32_t playerAmmoCount = 0;
		p->GetItemCount(playerAmmoCount, ammo, false);

		uint32_t sellAmmoCount = ammoStock;
		// 1.5배로 팔 리스트에 들어있으면 총알을 늘림
		if (sellRoundsMult_1_5_List && !sellRoundsMult_1_5_List->arrayOfForms.empty()) {
			for (TESForm* increaseAmmo : sellRoundsMult_1_5_List->arrayOfForms) {
				if (increaseAmmo == ammo) {
					sellAmmoCount *= 1.5;
					break;
				}
			}
		}

		// 0.5배로 팔 리스트에 있으면 총알을 줄임
		if (sellRoundsMult_0_5_List && !sellRoundsMult_0_5_List->arrayOfForms.empty()) {
			for (TESForm* decreaseAmmo : sellRoundsMult_0_5_List->arrayOfForms) {
				if (decreaseAmmo == ammo) {
					sellAmmoCount *= 0.5;
					break;
				}
			}
		}

		if (sellAmmoCount > playerAmmoCount) {
			uint32_t sellCount = sellAmmoCount - playerAmmoCount;

			FormOrInventoryObj tempObj;
			tempObj.form = ammo;
			AddItemVM(vm, 0, sellerChest, tempObj, sellCount, false);
		}
	}
}

void saveAmmoMods()
{
	auto& setAmmoModArray = DH->GetFormArray<BGSMod::Attachment::Mod>();

	int itemArraySize = setAmmoModArray.size();

	for (int i = 0; i < itemArraySize; ++i) {
		BGSMod::Attachment::Mod* tMod = setAmmoModArray[i];

		if (!tMod)
			continue;

		BGSMod::Attachment::Mod::Data modData;
		tMod->GetData(modData);

		if (modData.propertyModCount == 0) {
			continue;  // propertyModCount가 0이면 다음으로 넘어감
		}

		for (std::uint32_t i = 0; i < modData.propertyModCount; ++i) {
			BGSMod::Property::Mod* propertyMod = &modData.propertyMods[i];

			if (propertyMod) {
				if (propertyMod->op == BGSMod::Property::OP::kSet && propertyMod->type == BGSMod::Property::TYPE::kForm) {
					TESForm* tForm = propertyMod->data.form;
					if (tForm && tForm->formType == ENUM_FORM_ID::kAMMO) {
						// unordered_set으로 중복 체크 및 추가
						saveMod.insert(tMod);
					}
				}
			}
		}
	}
	//logger::info("저장한 모드 숫자{}", saveMod.size());
}

void OnF4SEMessage(F4SE::MessagingInterface::Message* msg)
{
	switch (msg->type) {
	case F4SE::MessagingInterface::kGameLoaded:
		DH = RE::TESDataHandler::GetSingleton();
		p = PlayerCharacter::GetSingleton();

		gAmmoStock = (TESGlobal*)DH->LookupForm(0x0026, "EasyAmmoVendingMachine.esp");

		sellRoundsMult_1_5_List = (BGSListForm*)DH->LookupForm(0x002B, "EasyAmmoVendingMachine.esp");
		sellRoundsMult_0_5_List = (BGSListForm*)DH->LookupForm(0x002C, "EasyAmmoVendingMachine.esp");
		sellRoundsForcedList = (BGSListForm*)DH->LookupForm(0x002D, "EasyAmmoVendingMachine.esp");

		saveAmmoMods();

		break;
	}
}

bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine* a_vm)
{
	vm = a_vm;

	//REL::IDDatabase::Offset2ID o2i;
	//logger::info("0x0x80750: {}", o2i(0x80750));

	a_vm->BindNativeMethod("EAV_Ammo_F4SE"sv, "injectAmmo"sv, injectAmmo);
	
	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info)
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= fmt::format("{}.log", Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("Global Log"s, std::move(sink));

#ifndef NDEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::trace);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%^%l%$] %v"s);

	logger::info("{} v{}", Version::PROJECT, Version::NAME);

	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::MAJOR;

	if (a_f4se->IsEditor()) {
		logger::critical("loaded in editor");
		return false;
	}

	const auto ver = a_f4se->RuntimeVersion();
	if (ver < F4SE::RUNTIME_1_10_162) {
		logger::critical("unsupported runtime v{}", ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se);

	const F4SE::PapyrusInterface* papyrus = F4SE::GetPapyrusInterface();
	if (papyrus)
		papyrus->Register(RegisterPapyrusFunctions);

	const F4SE::MessagingInterface* message = F4SE::GetMessagingInterface();
	if (message)
		message->RegisterListener(OnF4SEMessage);

	return true;
}
