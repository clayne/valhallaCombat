#include "ValhallaCombat.hpp"
#include "stunHandler.h"
#include "hitProcessor.h"
#include <functional>
#include <iostream>
void stunHandler::update() {
	auto it = stunRegenQueue.begin();
	while (it != stunRegenQueue.end()) {
		auto actor = it->first;
		if (!actor || !actor->currentProcess || !actor->currentProcess->InHighProcess()) {
			actorStunMap.erase(actor);
			it = stunRegenQueue.erase(it); continue;
		}
		if (!actor->IsInCombat()) {
			if (it->second <= 0) {
				//start regenerating stun from actorStunMap.
				if (actorStunMap.find(actor) == actorStunMap.end()) {
					it = stunRegenQueue.erase(it); continue;
				}
				else {
					if (actorStunMap.find(actor)->second.second < actorStunMap.find(actor)->second.first) {
						actorStunMap.find(actor)->second.second += 
							*Utils::g_deltaTimeRealTime * 1 / 10 * actorStunMap.find(actor)->second.first;
					}
					else {
						it = stunRegenQueue.erase(it); continue;
					}
				}
			}
			else {
				it->second -= *Utils::g_deltaTimeRealTime;
			}
		}
		++it;
	}
}

void stunHandler::initTrueHUDStunMeter() {
	if (ValhallaCombat::GetSingleton()->g_trueHUD
		->RequestSpecialResourceBarsControl(SKSE::GetPluginHandle()) != TRUEHUD_API::APIResult::AlreadyTaken) {
		INFO("TrueHUD special bar request success.");
		if (
			ValhallaCombat::GetSingleton()->g_trueHUD
			->RegisterSpecialResourceFunctions(SKSE::GetPluginHandle(), getStun, getMaxStun, true) == TRUEHUD_API::APIResult::OK) {
			INFO("TrueHUD special bar init success.");
		}
	}
}

void stunHandler::releaseTrueHUDStunMeter() {
	if (ValhallaCombat::GetSingleton()->g_trueHUD
		->ReleaseSpecialResourceBarControl(SKSE::GetPluginHandle()) == TRUEHUD_API::APIResult::OK) {
		INFO("TrueHUD special bar release success.");
	}
}

float stunHandler::getMaxStun(RE::Actor* actor) {
	float stun = (actor->GetPermanentActorValue(RE::ActorValue::kStamina)
		+ actor->GetPermanentActorValue(RE::ActorValue::kHealth)) / 2;
	//DEBUG("Calculated {}'s max stun: {}.", actor->GetName(), stun);
	return stun;
}

float stunHandler::getStun(RE::Actor* actor) {
	DEBUG("Getting {}'s stun.", actor->GetName());
	auto actorStunMap = stunHandler::GetSingleton()->actorStunMap;
	auto it = actorStunMap.find(actor);
	if (it != actorStunMap.end()) {
		return it->second.second;
	}
	else {
		//DEBUG("Unable to find actor on the stun map, tracking actor and returning the actor's permanent stun.");
		stunHandler::GetSingleton()->trackStun(actor);
		return getStun(actor);
	}

}

void stunHandler::damageStun(RE::Actor* actor, float damage) {
	//DEBUG("Damaging {}'s stun by {} points.", actor->GetName(), damage);
	auto it = actorStunMap.find(actor);
	if (it == actorStunMap.end()) {
		//DEBUG("{} not found on the stun map, tracking actor.", actor->GetName());
		trackStun(actor);
		damageStun(actor, damage);
		return;
	}
	it->second.second -= damage;
	stunRegenQueue.emplace(actor, 3); //3 seconds cooldown to regenerate stun.
	//DEBUG("{}'s stun damaged to {}", actor->GetName(), it->second.second);
	/*if (it->second.second <= 0) {
		DEBUG("bleed out {}!", actor->GetName());
		actor->SetGraphVariableBool("IsBleedingOut", true);
		actor->NotifyAnimationGraph("bleedOutStart");
		actor->SetGraphVariableBool("IsBleedingOut", true);
	}*/
}

void stunHandler::knockDown(RE::Actor* aggressor, RE::Actor* victim) {

}

void stunHandler::calculateStunDamage(
	STUNSOURCE stunSource, RE::TESObjectWEAP* weapon, RE::Actor* aggressor, RE::Actor* victim, float baseDamage) {
	DEBUG("Calculating stun damage");
	//TODO:see if I can optimize it a bit more
	if (!settings::bStunToggle) { //stun damage will not be applied with stun turned off.
		return;
	}
	switch (stunSource) {
	case STUNSOURCE::parry:
		damageStun(victim, baseDamage * settings::fStunParryMult); break;
	case STUNSOURCE::powerBash:
		damageStun(victim, aggressor->GetActorValue(RE::ActorValue::kBlock) * settings::fStunPowerBashMult);
		break;
	case STUNSOURCE::bash:
		damageStun(victim, aggressor->GetActorValue(RE::ActorValue::kBlock) * settings::fStunBashMult);
		break;
	case STUNSOURCE::powerAttack:
		baseDamage *= settings::fStunPowerAttackMult;
	case STUNSOURCE::lightAttack:
		if (!weapon) {
			baseDamage *= settings::fStunUnarmedMult;
		}
		else {
			switch (weapon->GetWeaponType()) {
			case RE::WEAPON_TYPE::kHandToHandMelee: baseDamage *= settings::fStunUnarmedMult; break;
			case RE::WEAPON_TYPE::kOneHandDagger: baseDamage *= settings::fStunDaggerMult; break;
			case RE::WEAPON_TYPE::kOneHandSword: baseDamage *= settings::fStunSwordMult; break;
			case RE::WEAPON_TYPE::kOneHandAxe: baseDamage *= settings::fStunWarAxeMult; break;
			case RE::WEAPON_TYPE::kOneHandMace: baseDamage *= settings::fStunMaceMult; break;
			case RE::WEAPON_TYPE::kTwoHandAxe: baseDamage *= settings::fStun2HBluntMult; break;
			case RE::WEAPON_TYPE::kTwoHandSword: baseDamage *= settings::fStunGreatSwordMult; break;
			}
		}
		damageStun(victim, baseDamage);
		break;
	}
}

void stunHandler::houseKeeping() {
	DEBUG("housekeeping...");
	stunRegenQueue.clear();
	DEBUG("1");
	auto it = actorStunMap.begin();
	while (it != actorStunMap.end()) {
		auto actor = it->first;
		if (!actor || !actor->currentProcess || !actor->currentProcess->InHighProcess()) {
			DEBUG("actor not longer exist, cleaning up from stun map...");
			it = actorStunMap.erase(it); continue;
		}
		auto newMax = getMaxStun(actor); 		
		it->second.first = newMax; //refresh max stun.
		if (newMax > it->second.second) {
			stunRegenQueue.emplace(actor, 0); //if the new max is bigger, start regenerating
		}
		else {
			it->second.second = newMax; //if the new max is smaller/equal, lower the og value
		}
		it++;
	}
	DEBUG("housekeeping finished.");
}

void stunHandler::refreshStun() {
	stunRegenQueue.clear();
	actorStunMap.clear();
}

/*Bunch of abstracted utilities.*/
#pragma region stunUtils
void stunHandler::trackStun(RE::Actor* actor) {
	float maxStun = getMaxStun(actor);
	actorStunMap.emplace(actor, std::pair<float, float>(maxStun, maxStun));
	DEBUG("Start tracking {}'s stun. Max Stun: {}.", actor->GetName(), maxStun);
};
void stunHandler::untrackStun(RE::Actor* actor) {
	actorStunMap.erase(actor);
}
void stunHandler::resetStun(RE::Actor* actor) {
	DEBUG("Resetting {}'s stun.", actor->GetName());
	auto it = actorStunMap.find(actor);
	if (it != actorStunMap.end()) {
		it->second.second = it->second.first;
	}
	else {
		DEBUG("Erorr: {} not found in actor stun map, and thus cannot be reset", actor->GetName());
	}
}
#pragma endregion