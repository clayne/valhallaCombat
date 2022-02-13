#include "animEventHandler.h"
#include "attackHandler.h"
#include "blockHandler.h"
//all credits to Bingle
namespace anno
{
	const char* attackStop = "attackStop";
	const char* hitFrame = "HitFrame";
	const char* preHitFrame = "preHitFrame";
	const char* atkWin_Skysa = "SkySA_AttackWinStart";
	const char* block_start = "blockStartOut";
	const char* block_stop = "blockStop";
	const char* weaponSwing = "weaponSwing";
	const char* tkDodge = "";
	const char* dmcoStep = "";
	const char* dmcoRoll = "";
}
constexpr uint32_t hash(const char* data, size_t const size) noexcept
{
	uint32_t hash = 5381;

	for (const char* c = data; c < data + size; ++c) {
		hash = ((hash << 5) + hash) + (unsigned char)*c;
	}

	return hash;
}

constexpr uint32_t operator"" _h(const char* str, size_t size) noexcept
{
	return hash(str, size);
}

RE::BSEventNotifyControl animEventHandler::HookedProcessEvent(RE::BSAnimationGraphEvent& a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* src) {
    FnProcessEvent fn = fnHash.at(*(uint64_t*)this);
	std::string_view eventTag = a_event.tag.data();
	if (a_event.holder->IsPlayerRef()) {
		DEBUG("player event: {}", a_event.tag);
	}
	switch (hash(eventTag.data(), eventTag.size())) {
	case "preHitFrame"_h:
		DEBUG("==========prehitFrame==========");
		attackHandler::GetSingleton()->registerAtk(a_event.holder->As<RE::Actor>());
		break;
	case "attackStop"_h:
		DEBUG("==========attackstop==========");
		attackHandler::GetSingleton()->checkout(a_event.holder->As<RE::Actor>());
		break;
	case "blockStartOut"_h:
		DEBUG("===========blockStartOut===========");
		if (settings::bPerfectBlocking) {
			blockHandler::GetSingleton()->registerPerfectBlock(a_event.holder->As<RE::Actor>());
		}
		break;
	case "TKDR_DodgeStart"_h:
		DEBUG("==========TK DODGE============");
		if (settings::bTKDodgeCompatibility) {
			staminaHandler::staminaDodgeStep(a_event.holder->As<RE::Actor>());
		}
		break;
	}
    return fn ? (this->*fn)(a_event, src) : RE::BSEventNotifyControl::kContinue;
}

std::unordered_map<uint64_t, animEventHandler::FnProcessEvent> animEventHandler::fnHash;