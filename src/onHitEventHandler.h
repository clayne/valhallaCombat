#include "avHandler.h"
using EventResult = RE::BSEventNotifyControl;
class onHitEventHandler : public RE::BSTEventSink<RE::TESHitEvent>
{
public:
	boolean shouldHitRestoreStamina(const RE::TESHitEvent* a_event);
	virtual EventResult ProcessEvent(const RE::TESHitEvent* a_event, RE::BSTEventSource<RE::TESHitEvent>* a_eventSource);

	static bool Register()
	{
		static onHitEventHandler singleton;
		auto ScriptEventSource = RE::ScriptEventSourceHolder::GetSingleton();

		if (!ScriptEventSource) {
			ERROR("Script event source not found");
			return false;
		}

		ScriptEventSource->AddEventSink(&singleton);

		INFO("Register {}.", typeid(singleton).name());

		return true;
	}

private:
	onHitEventHandler() = default;
	~onHitEventHandler() = default;
	onHitEventHandler(const onHitEventHandler&) = delete;
	onHitEventHandler(onHitEventHandler&&) = delete;
	onHitEventHandler& operator=(const onHitEventHandler&) = delete;
	onHitEventHandler& operator=(onHitEventHandler&&) = delete;
};