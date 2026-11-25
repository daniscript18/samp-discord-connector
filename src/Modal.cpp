#include "Modal.hpp"

#include <algorithm>

Modal::Modal(std::string const& custom_id, std::string const& title) :
	m_CustomId(custom_id),
	m_Title(title)
{
}

bool Modal::AddTextInput(std::string const& custom_id, std::string const& label,
	int style, std::string const& placeholder, bool required, int min_length, int max_length)
{
	if (custom_id.empty() || custom_id.size() > 100 || label.empty() || label.size() > 45
		|| placeholder.size() > 100 || (style != 1 && style != 2)
		|| min_length < 0 || max_length < 1 || min_length > max_length || max_length > 4000
		|| m_Inputs.size() >= 5
		|| std::find_if(m_Inputs.begin(), m_Inputs.end(), [&custom_id](ModalTextInput const& input)
			{ return input.custom_id == custom_id; }) != m_Inputs.end())
	{
		return false;
	}

	m_Inputs.push_back({ custom_id, label, placeholder, style, min_length, max_length, required });
	return true;
}

json Modal::ToJson() const
{
	json components = json::array();
	for (auto const& input : m_Inputs)
	{
		json text_input = {
			{ "type", 4 },
			{ "custom_id", input.custom_id },
			{ "style", input.style },
			{ "label", input.label },
			{ "required", input.required },
			{ "min_length", input.min_length },
			{ "max_length", input.max_length }
		};
		if (!input.placeholder.empty())
		{
			text_input["placeholder"] = input.placeholder;
		}
		components.push_back({
			{ "type", 1 },
			{ "components", json::array({ text_input }) }
		});
	}

	return {
		{ "custom_id", m_CustomId },
		{ "title", m_Title },
		{ "components", components }
	};
}

ModalId_t ModalManager::AddModal(std::string const& custom_id, std::string const& title)
{
	if (custom_id.empty() || custom_id.size() > 100 || title.empty() || title.size() > 45)
	{
		return INVALID_MODAL_ID;
	}

	ModalId_t id = 1;
	while (m_Modals.find(id) != m_Modals.end())
	{
		++id;
	}

	m_Modals.emplace(id, Modal_t(new Modal(custom_id, title)));
	return id;
}

bool ModalManager::DeleteModal(ModalId_t id)
{
	return m_Modals.erase(id) != 0;
}

Modal_t const& ModalManager::FindModal(ModalId_t id)
{
	static Modal_t invalid_modal;
	auto it = m_Modals.find(id);
	return it == m_Modals.end() ? invalid_modal : it->second;
}
