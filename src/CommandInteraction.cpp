#include "Command.hpp"
#include "Network.hpp"
#include "PawnDispatcher.hpp"
#include "Callback.hpp"
#include "Logger.hpp"
#include "utils.hpp"
#include "Bot.hpp"
#include "Guild.hpp"
#include "CommandInteraction.hpp"
#include "Channel.hpp"
#include "User.hpp"
#include "Embed.hpp"
#include "Http.hpp"
#include <json.hpp>
#include <regex>
#include <functional>

CommandInteraction::CommandInteraction(CommandInteractionId_t id, UserId_t user,
	nlohmann::json const& interaction_json, bool acknowledged) :
	m_InteractionUser(user),
	m_Acknowledged(acknowledged)
{
	m_ID = id;
	m_IDSnowflake = interaction_json.at("id").get<std::string>();
	m_Token = interaction_json.at("token").get<std::string>();

	std::string guild_str = "";
	bool has_guild = utils::TryGetJsonValue(interaction_json, guild_str, "guild_id");
	if (has_guild && interaction_json.at("type").get<int>() != 2)
	{
		auto const& guild = GuildManager::Get()->FindGuildById(guild_str);
		if (guild)
		{
			m_Guild = guild->GetPawnId();
		}
	}
	if (interaction_json.at("type").get<int>() == 2)
	{
		std::string name = interaction_json.at("data").at("name").get<std::string>();
		if (has_guild)
		{
			auto const& guild = GuildManager::Get()->FindGuildById(guild_str);
			if (!guild)
			{
				Logger::Get()->Log(samplog_LogLevel::WARNING,
					"received command interaction for unknown guild '{}'", guild_str);
				return;
			}
			auto const& command = CommandManager::Get()->FindCommand(
				CommandManager::Get()->FindCommandIdByName(name, guild->GetPawnId()));
			if (!command || command->GetGuild() != guild->GetPawnId())
			{
				Logger::Get()->Log(samplog_LogLevel::WARNING,
					"received command interaction for command '{}' in mismatched guild '{}'", name, guild_str);
				return;
			}
			m_Guild = guild->GetPawnId();
		}
		else
		{
			auto const& command = CommandManager::Get()->FindCommand(
				CommandManager::Get()->FindCommandIdByName(name));
			if (!command)
			{
				Logger::Get()->Log(samplog_LogLevel::WARNING,
					"received command interaction for unknown command '{}'", name);
				return;
			}
		}
	}
	else
	{
		utils::TryGetJsonValue(interaction_json.at("data"), m_CustomId, "custom_id");
		if (interaction_json.at("type").get<int>() == 5)
		{
			std::function<void(json const&)> read_components = [&](json const& node)
			{
				if (!node.is_object())
				{
					return;
				}
				std::string custom_id;
				std::string value;
				if (utils::TryGetJsonValue(node, custom_id, "custom_id")
					&& utils::TryGetJsonValue(node, value, "value"))
				{
					m_ModalInputs[custom_id] = value;
				}
				auto children = node.find("components");
				if (children != node.end() && children->is_array())
				{
					for (auto const& child : *children)
					{
						read_components(child);
					}
				}
			};
			auto components = interaction_json.at("data").find("components");
			if (components != interaction_json.at("data").end() && components->is_array())
			{
				for (auto const& component : *components)
				{
					read_components(component);
				}
			}
		}
	}

	std::string channel_str;
	utils::TryGetJsonValue(interaction_json, channel_str, "channel_id");
	Channel_t const & channel = ChannelManager::Get()->FindChannelById(channel_str);
	if (!channel && !has_guild)
	{
		ChannelId_t cid = ChannelManager::Get()->AddDMChannel(interaction_json);
		Channel_t const& dm_channel = ChannelManager::Get()->FindChannel(cid);
		m_Channel = dm_channel ? dm_channel->GetPawnId() : INVALID_CHANNEL_ID;
	}
	else
	{
		m_Channel = channel ? channel->GetPawnId() : INVALID_CHANNEL_ID;
	}
	m_InteractionUser = user;
	if (interaction_json.at("type").get<int>() == 2)
	{
		ParseOptions(interaction_json, has_guild ? guild_str : "");
	}
}

void CommandInteraction::ParseOptions(nlohmann::json const& interaction_json, std::string const & guildid)
{
	CommandInteractionOption tmpoption;

	//interaction_json.at("data").at("options")[0]

	if (interaction_json.at("data").find("options") == interaction_json.at("data").end()) {
		tmpoption.m_Type = COMMAND_OPTION_TYPE::OPTION_STRING;
		tmpoption.m_Name = "";
		tmpoption.m_Value = ""; 
	}
	else {
		auto first = interaction_json.at("data").at("options")[0];
		tmpoption.m_Type = static_cast<COMMAND_OPTION_TYPE>(first.at("type").get<int>());
		tmpoption.m_Name = first.at("name").get<std::string>();
		utils::TryGetJsonValue(first, tmpoption.m_Value, "value");

		if (tmpoption.m_Type == COMMAND_OPTION_TYPE::OPTION_STRING && guildid.length()) {
			std::regex mentions("<@!\\s*(\\d+)\\s*>");
			auto mentions_begin =
				std::sregex_iterator(tmpoption.m_Value.begin(), tmpoption.m_Value.end(), mentions);
			auto mentions_end = std::sregex_iterator();

			std::string guild_str;
			utils::TryGetJsonValue(interaction_json, guild_str, "guild_id");
			for (std::sregex_iterator i = mentions_begin; i != mentions_end; ++i) {
				std::string match = (*i)[1].str();
				const User_t & user = UserManager::Get()->FindUserById(match);
				if (user->IsValid()) {
					m_Mentions.push_back(user->GetPawnId());
				}
				
			}
		}
	}
	
	CommandInteractionOption_t option = std::unique_ptr<CommandInteractionOption>(new CommandInteractionOption(std::move(tmpoption)));
	AddInteractionOption(option);
	/*
	if (interaction_json.find("options") != interaction_json.end() && interaction_json.at("options").size())
	{
		for (const auto& option_ : interaction_json.at("options").items())
		{
			ParseOptions(option_.value());
		}
	}*/
}

void CommandInteraction::AddInteractionOption(CommandInteractionOption_t & option)
{
	m_InteractionOptions.push_back(std::move(option));
}

bool CommandInteraction::Respond(json const& response)
{
	if (m_Acknowledged)
	{
		Logger::Get()->Log(samplog_LogLevel::WARNING,
			"interaction '{}' has already been acknowledged", m_IDSnowflake);
		return false;
	}

	std::string json_str;
	if (!utils::TryDumpJson(response, json_str))
	{
		Logger::Get()->Log(samplog_LogLevel::ERROR, "can't serialize interaction response: {}", json_str);
		return false;
	}

	m_Acknowledged = true;
	Network::Get()->Http().Post(
		fmt::format("/interactions/{:s}/{:s}/callback", m_IDSnowflake, m_Token), json_str);
	return true;
}

void CommandInteraction::SendEmbed(EmbedId_t embedid, const std::string message)
{
	auto& embed = EmbedManager::Get()->FindEmbed(embedid);
	json data = {
		{ "content", message },
		{ "embeds", {
			json::object()
		}}
	};

	// Add fields (if any).

	data["embeds"][0] = json::object({
			{ "title", embed->GetTitle() },
			{ "description", embed->GetDescription() },
			{ "url", embed->GetUrl() },
			{ "timestamp", embed->GetTimestamp() },
			{ "color", embed->GetColor() },
			{ "footer", {
				{"text", embed->GetFooterText()},
				{"icon_url", embed->GetFooterIconUrl()},
			} },
			{ "thumbnail", json::object() },
			{ "image", json::object() }
		});
	
	if (!embed->GetThumbnailUrl().empty())
	{
		data["embeds"][0]["thumbnail"]["url"] = embed->GetThumbnailUrl();
	}

	if (!embed->GetImageUrl().empty())
	{
		data["embeds"][0]["image"]["url"] = embed->GetImageUrl();
	}

	if (embed->GetFields().size())
	{
		json field_array = json::array();
		for (const auto& i : embed->GetFields())
		{
			field_array.push_back({
				{"name", i._name},
				{"value", i._value},
				{"inline", i._inline_}
			});
		}
		data["embeds"][0]["fields"] = field_array;
	}

	std::string json_str;
	if (!utils::TryDumpJson(data, json_str))
		Logger::Get()->Log(samplog_LogLevel::ERROR, "can't serialize JSON: {}", json_str);

	if (!m_Acknowledged)
	{
		Respond({ { "type", 4 }, { "data", data } });
		return;
	}

	/*/webhooks/{application.id}/{interaction.token}/messages/@original*/
	Network::Get()->Http().Patch(fmt::format("/webhooks/{:s}/{:s}/messages/@original", ThisBot::Get()->GetApplicationID(), m_Token), json_str);
}

void CommandInteraction::SendInteractionMessage(const std::string message)
{
	json data = {
		{ "content", message }
	};

	std::string json_str;
	if (!utils::TryDumpJson(data, json_str))
		Logger::Get()->Log(samplog_LogLevel::ERROR, "can't serialize JSON: {}", json_str);

	if (!m_Acknowledged)
	{
		Respond({ { "type", 4 }, { "data", data } });
		return;
	}

	/*/webhooks/{application.id}/{interaction.token}/messages/@original*/
	Network::Get()->Http().Patch(fmt::format("/webhooks/{:s}/{:s}/messages/@original", ThisBot::Get()->GetApplicationID(), m_Token), json_str);
}

CommandInteraction_t const & CommandInteractionManager::FindCommandInteraction(CommandInteractionId_t interaction)
{
	static CommandInteraction_t invalid_command_interaction;
	auto it = m_Interactions.find(interaction);
	if (it == m_Interactions.end())
		return invalid_command_interaction;
	return it->second;
}

CommandInteractionId_t CommandInteractionManager::AddCommandInteraction(UserId_t user,
	nlohmann::json const& interaction_json, bool acknowledged)
{
	CommandInteractionId_t id = 1;
	while (m_Interactions.find(id) != m_Interactions.end())
		++id;

	if (!m_Interactions.emplace(id, CommandInteraction_t(
		new CommandInteraction(id, user, interaction_json, acknowledged))).first->second)
	{
		Logger::Get()->Log(samplog_LogLevel::ERROR,
			"can't create command interaction: duplicate key '{}'", id);
		return INVALID_USER_ID;
	}

	Logger::Get()->Log(samplog_LogLevel::INFO, "successfully created command interaction with id '{}'", id);
	return id;
}

bool CommandInteractionManager::DeleteCommandInteraction(CommandInteractionId_t interaction)
{
	if (m_Interactions.find(interaction) == m_Interactions.end())
	{
		Logger::Get()->Log(samplog_LogLevel::WARNING, "attempted to delete command interaction with id '{}' but it does not exist", interaction);
		return false;
	}

	m_Interactions.erase(interaction);
	Logger::Get()->Log(samplog_LogLevel::INFO, "successfully deleted command interaction with id '{}'", interaction);
	return true;
}