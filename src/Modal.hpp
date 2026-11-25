#pragma once

#include "Singleton.hpp"
#include "types.hpp"

#include <string>
#include <vector>
#include <map>
#include <json.hpp>

using json = nlohmann::json;

struct ModalTextInput
{
	std::string custom_id;
	std::string label;
	std::string placeholder;
	int style;
	int min_length;
	int max_length;
	bool required;
};

class Modal
{
public:
	Modal(std::string const& custom_id, std::string const& title);

	bool AddTextInput(std::string const& custom_id, std::string const& label,
		int style, std::string const& placeholder, bool required, int min_length, int max_length);
	json ToJson() const;
	size_t GetInputCount() const
	{
		return m_Inputs.size();
	}

private:
	std::string m_CustomId;
	std::string m_Title;
	std::vector<ModalTextInput> m_Inputs;
};

class ModalManager : public Singleton<ModalManager>
{
	friend class Singleton<ModalManager>;
private:
	ModalManager() = default;
	~ModalManager() = default;

	std::map<ModalId_t, Modal_t> m_Modals;

public:
	ModalId_t AddModal(std::string const& custom_id, std::string const& title);
	bool DeleteModal(ModalId_t id);
	Modal_t const& FindModal(ModalId_t id);
};
