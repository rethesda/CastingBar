#include "StateController.h"

#include "States/DefaultState.h"
#include "States/RangedState.h"
#include "States/RightMagicState.h"
#include "States/LeftMagicState.h"
#include "States/VoiceState.h"

#include "CastingBar.h"

using namespace RE;


StateController::StateController(CastingBar* a_meter, PlayerCharacter* a_player) :
	actor(a_player), meter(a_meter), state(std::make_unique<DefaultState>())
{
	if (auto events = ScriptEventSourceHolder::GetSingleton()) {
		events->AddEventSink<TESObjectLoadedEvent>(this);
		events->AddEventSink<TESLoadGameEvent>(this);
	}

	actor->AddAnimationGraphEventSink(this);
}

Action StateController::AnimEventToAction(const BSFixedString& a_event)
{
	static const ActionMap actions 
	{
		{ "weaponDraw", Action::kWeaponDraw },
		{ "WeaponSheathe", Action::kWeaponSheathe },
		//{ "weaponSwing", Action::kWeaponSwingRight },
		//{ "weaponLeftSwing", Action::kWeaponSwingLeft },
		{ "attackStop", Action::kAttackStop },
		//{ "PowerAttackStop", Action::kPowerAttackStop },
		{ "Magic_Equip_Out", Action::kSpellEquip },
		{ "Magic_Unequip_Out", Action::kSpellUnequip },
		{ "MRh_Equipped_Event", Action::kSpellEquipRight },
		{ "MLh_Equipped_Event", Action::kSpellEquipLeft },
		//{ "MRh_Unequip_Out", Action::Action::kSpellUnequipRight },
		//{ "MLh_Unequip_Out", Action::Action::kSpellUnequipLEft },
		{ "BeginCastRight", Action::kSpellCastRight },
		{ "BeginCastLeft", Action::kSpellCastLeft },
		{ "MRh_SpellFire_Event", Action::kSpellFireRight },
		{ "MLh_SpellFire_Event", Action::kSpellFireLeft },
		{ "MRh_PreAimedCon_to_MRh_AimedCon", Action::kSpellPreAimedToAimed }, 
		{ "CastStop", Action::kSpellCastStop },
		{ "BeginCastVoice", Action::kVoiceCast },
		{ "Voice_SpellFire_Event", Action::kVoiceFire },
		{ "shoutStop", Action::kVoiceStop },
		{ "BowDraw", Action::kBowDraw },
		{ "BowDrawn", Action::kBowDrawn },
		{ "BowRelease", Action::kBowRelease },
		{ "bowReset", kBowReset },
	};

	auto it = actions.find(a_event.c_str());
	if (it != actions.end()) {
		return it->second;
	}

	return Action::kNone;
}

void StateController::Dispatch(Action action)
{
	auto newState = state->HandleAction(actor, action);
	if (newState) {

		state = std::move(newState);

		if (auto movie = state->GetMovie()) {
			meter->LoadMovie(movie);
		}	
	}
}

EventResult StateController::ProcessEvent(const BSAnimationGraphEvent* a_event, AnimationGraphEventSource*)
{
	//logger::trace("Event: {}", a_event->tag);

	Action action = AnimEventToAction(a_event->tag);
	if (action != Action::kNone) {
		Dispatch(action);
	}

	return EventResult::kContinue;
}

EventResult StateController::ProcessEvent(const TESObjectLoadedEvent* a_event, ObjectLoadedEventSource* a_source)
{
	if (a_event->formID == actor->formID && a_event->loaded) {

		auto tasks = SKSE::GetTaskInterface();
		tasks->AddTask([=] {
			actor->AddAnimationGraphEventSink(this);
		});
	}
	return EventResult::kContinue;
}

EventResult StateController::ProcessEvent(const TESLoadGameEvent* a_event, LoadGameEventSource*)
{
	if (!firstLoad) {
		state.reset(new DefaultState());	
	}
	firstLoad = false;

	actor->AddAnimationGraphEventSink(this);

	return EventResult::kContinue;
}

void StateController::Update()
{
	auto prog = state->GetProgress(actor);
	if (prog.has_value()) {
		meter->SetPercent(prog.value());
		return;
	}
	meter->FadeOut();
}
