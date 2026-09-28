#pragma once

#include <cctype>
#include <cstdint>
#include <string>
#include <string_view>

#include <RE/B/BSFixedString.h>
#include <RE/T/TESDataHandler.h>
#include <RE/T/TESForm.h>

namespace WRF
{
	inline std::string_view TrimIdentifier(std::string_view value)
	{
		while (!value.empty()) {
			const auto ch = static_cast<unsigned char>(value.front());
			if (std::isspace(ch) || ch == 0xEF || ch == 0xBB || ch == 0xBF) value.remove_prefix(1);
			else break;
		}
		while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) value.remove_suffix(1);
		return value;
	}

	template <class T>
	T* ResolveIdentifier(std::string_view identifier)
	{
		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler || identifier.empty()) return nullptr;

		if (const auto split = identifier.find('|'); split != std::string_view::npos) {
			const auto pluginView = TrimIdentifier(identifier.substr(0, split));
			const auto formIDView = TrimIdentifier(identifier.substr(split + 1));
			if (pluginView.empty() || formIDView.empty()) return nullptr;

			try {
				std::size_t parsed = 0;
				if (formIDView.front() == '-' || formIDView.front() == '+') return nullptr;
				const auto parsedID = std::stoull(std::string(formIDView), &parsed, 16);
				if (parsed != formIDView.size() || parsedID > UINT32_MAX) return nullptr;
				const auto rawID = static_cast<RE::TESFormID>(parsedID);

				const std::string plugin(pluginView);
				const auto* mod = dataHandler->LookupModByName(plugin);
				if (!mod) return nullptr;

				if (mod->IsLight()) {
					// Accept either the plugin-local 12-bit ID or a full FElllrrr ID.
					// Resolve by plugin name so a changed light index does not stale config.
					if (rawID > 0x0FFF && (rawID >> 24) != 0xFE) return nullptr;
					const auto localID = rawID <= 0x0FFF ? rawID : rawID & 0x0FFF;
					if (auto* form = dataHandler->LookupForm(localID, plugin)) return form->As<T>();
					return nullptr;
				}

				// Regular plugins use a 24-bit local ID. When a full FormID is supplied,
				// strip its load-order byte and resolve against the named plugin.
				if (rawID > 0x00FFFFFF && (rawID >> 24) == 0xFE) return nullptr;
				const auto localID = rawID <= 0x00FFFFFF ? rawID : rawID & 0x00FFFFFF;
				if (auto* form = dataHandler->LookupForm(localID, plugin)) return form->As<T>();
				return nullptr;
			} catch (...) {
				return nullptr;
			}
		}

		const std::string editorID(TrimIdentifier(identifier));
		if (editorID.empty()) return nullptr;
		if (auto* form = RE::TESForm::GetFormByEditorID(RE::BSFixedString(editorID))) return form->As<T>();
		return nullptr;
	}
}
