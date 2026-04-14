#include "Bot.hpp"

#include "Network.hpp"
#include "Http.hpp"
#include "WebSocket.hpp"
#include "PawnDispatcher.hpp"
#include "User.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include "Guild.hpp"
#include "Embed.hpp"
#include "Logger.hpp"
#include "utils.hpp"

#include <json.hpp>
#include <fmt/format.h>

#include <map>


void ThisBot::TriggerTypingIndicator(Channel_t const &channel)
{
	Network::Get()->Http().Post(fmt::format("/channels/{:s}/typing", channel->GetId()), "");
}

void ThisBot::SetNickname(Guild_t const &guild, std::string const &nickname)
{
	json data = {
		{ "nick", nickname },
	};

	std::string json_str;
	if (!utils::TryDumpJson(data, json_str))
	{
		Logger::Get()->Log(samplog_LogLevel::ERROR, "can't serialize JSON: {}", json_str);
		return;
	}

	Network::Get()->Http().Patch(
		fmt::format("/guilds/{:s}/members/@me/nick", guild->GetId()), json_str);
}

bool ThisBot::CreatePrivateChannel(User_t const &user, pawn_cb::Callback_t &&callback)
{
	json data = {
		{ "recipient_id", user->GetId() },
	};

	std::string json_str;
	if (!utils::TryDumpJson(data, json_str))
	{
		Logger::Get()->Log(samplog_LogLevel::ERROR, "can't serialize JSON: {}", json_str);
		return false;
	}

	Network::Get()->Http().Post("/users/@me/channels", json_str,
		[this, callback](Http::Response r)
	{
		Logger::Get()->Log(samplog_LogLevel::DEBUG,
			"DM channel create response: status {}; body: {}; add: {}",
			r.status, r.body, r.additional_data);
		if (r.status / 100 == 2) // success
		{
			auto const channel_id = ChannelManager::Get()->AddChannel(json::parse(r.body));
			if (channel_id == INVALID_CHANNEL_ID)
				return;

			if (callback)
			{
				PawnDispatcher::Get()->Dispatch([=]()
				{
					m_CreatedChannelId = channel_id;
					callback->Execute();
					m_CreatedChannelId = INVALID_CHANNEL_ID;
				});
			}
		}
	});

	return true;
}

bool ThisBot::SendUserMessage(User_t const &user, std::string &&message, pawn_cb::Callback_t &&callback)
{
	json channel_data = {
		{ "recipient_id", user->GetId() },
	};

	std::string channel_json_str;
	if (!utils::TryDumpJson(channel_data, channel_json_str))
	{
		Logger::Get()->Log(samplog_LogLevel::ERROR, "can't serialize JSON: {}", channel_json_str);
		return false;
	}

	Network::Get()->Http().Post("/users/@me/channels", channel_json_str,
		[this, callback, message = std::move(message)](Http::Response channel_response) mutable
	{
		Logger::Get()->Log(samplog_LogLevel::DEBUG,
			"DM channel create response: status {}; body: {}; add: {}",
			channel_response.status, channel_response.body, channel_response.additional_data);
		if (channel_response.status / 100 != 2)
			return;

		auto const channel_id = ChannelManager::Get()->AddChannel(json::parse(channel_response.body));
		if (channel_id == INVALID_CHANNEL_ID)
			return;

		Channel_t const &channel = ChannelManager::Get()->FindChannel(channel_id);
		if (!channel)
			return;

		json message_data = {
			{ "content", std::move(message) }
		};

		std::string message_json_str;
		if (!utils::TryDumpJson(message_data, message_json_str))
		{
			Logger::Get()->Log(samplog_LogLevel::ERROR, "can't serialize JSON: {}", message_json_str);
			return;
		}

		Http::ResponseCb_t response_cb;
		if (callback)
		{
			response_cb = [this, callback, channel_id](Http::Response message_response)
			{
				Logger::Get()->Log(samplog_LogLevel::DEBUG,
					"DM message create response: status {}; body: {}; add: {}",
					message_response.status, message_response.body, message_response.additional_data);
				if (message_response.status / 100 != 2)
					return;

				auto msg_json = json::parse(message_response.body);
				PawnDispatcher::Get()->Dispatch([this, callback, channel_id, msg_json]() mutable
				{
					auto msg = MessageManager::Get()->Create(msg_json);
					if (msg == INVALID_MESSAGE_ID)
						return;

					m_CreatedChannelId = channel_id;
					MessageManager::Get()->SetCreatedMessageId(msg);
					callback->Execute();
					if (!MessageManager::Get()->Find(msg)->Persistent())
					{
						MessageManager::Get()->Delete(msg);
					}
					MessageManager::Get()->SetCreatedMessageId(INVALID_MESSAGE_ID);
					m_CreatedChannelId = INVALID_CHANNEL_ID;
				});
			};
		}

		Network::Get()->Http().Post(fmt::format("/channels/{:s}/messages", channel->GetId()), message_json_str,
			std::move(response_cb));
	});

	return true;
}

bool ThisBot::SendUserEmbedMessage(User_t const &user, Embed_t const &embed, std::string &&message,
	pawn_cb::Callback_t &&callback)
{
	json channel_data = {
		{ "recipient_id", user->GetId() },
	};

	std::string channel_json_str;
	if (!utils::TryDumpJson(channel_data, channel_json_str))
	{
		Logger::Get()->Log(samplog_LogLevel::ERROR, "can't serialize JSON: {}", channel_json_str);
		return false;
	}

	json message_data = {
		{ "content", std::move(message) },
		{ "embeds", { json::object() } }
	};

	message_data["embeds"][0] = json::object({
		{ "title", embed->GetTitle() },
		{ "description", embed->GetDescription() },
		{ "url", embed->GetUrl() },
		{ "timestamp", embed->GetTimestamp() },
		{ "color", embed->GetColor() },
		{ "footer", {
			{ "text", embed->GetFooterText() },
			{ "icon_url", embed->GetFooterIconUrl() },
		} },
		{ "thumbnail", json::object() },
		{ "image", json::object() }
	});

	if (!embed->GetThumbnailUrl().empty())
	{
		message_data["embeds"][0]["thumbnail"]["url"] = embed->GetThumbnailUrl();
	}

	if (!embed->GetImageUrl().empty())
	{
		message_data["embeds"][0]["image"]["url"] = embed->GetImageUrl();
	}

	if (embed->GetFields().size())
	{
		json field_array = json::array();
		for (auto const &field : embed->GetFields())
		{
			field_array.push_back({
				{ "name", field._name },
				{ "value", field._value },
				{ "inline", field._inline_ }
			});
		}
		message_data["embeds"][0]["fields"] = field_array;
	}

	std::string message_json_str;
	if (!utils::TryDumpJson(message_data, message_json_str))
	{
		Logger::Get()->Log(samplog_LogLevel::ERROR, "can't serialize JSON: {}", message_json_str);
		return false;
	}

	Network::Get()->Http().Post("/users/@me/channels", channel_json_str,
		[this, callback, message_json_str = std::move(message_json_str)](Http::Response channel_response) mutable
	{
		Logger::Get()->Log(samplog_LogLevel::DEBUG,
			"DM channel create response: status {}; body: {}; add: {}",
			channel_response.status, channel_response.body, channel_response.additional_data);
		if (channel_response.status / 100 != 2)
			return;

		auto const channel_id = ChannelManager::Get()->AddChannel(json::parse(channel_response.body));
		if (channel_id == INVALID_CHANNEL_ID)
			return;

		Channel_t const &channel = ChannelManager::Get()->FindChannel(channel_id);
		if (!channel)
			return;

		Http::ResponseCb_t response_cb;
		if (callback)
		{
			response_cb = [this, callback, channel_id](Http::Response message_response)
			{
				Logger::Get()->Log(samplog_LogLevel::DEBUG,
					"DM embed message create response: status {}; body: {}; add: {}",
					message_response.status, message_response.body, message_response.additional_data);
				if (message_response.status / 100 != 2)
					return;

				auto msg_json = json::parse(message_response.body);
				PawnDispatcher::Get()->Dispatch([this, callback, channel_id, msg_json]() mutable
				{
					auto msg = MessageManager::Get()->Create(msg_json);
					if (msg == INVALID_MESSAGE_ID)
						return;

					m_CreatedChannelId = channel_id;
					MessageManager::Get()->SetCreatedMessageId(msg);
					callback->Execute();
					if (!MessageManager::Get()->Find(msg)->Persistent())
					{
						MessageManager::Get()->Delete(msg);
					}
					MessageManager::Get()->SetCreatedMessageId(INVALID_MESSAGE_ID);
					m_CreatedChannelId = INVALID_CHANNEL_ID;
				});
			};
		}

		Network::Get()->Http().Post(fmt::format("/channels/{:s}/messages", channel->GetId()),
			message_json_str, std::move(response_cb));
	});

	return true;
}

std::string const &GetPresenceStatusString(ThisBot::PresenceStatus status)
{
	static const std::map<ThisBot::PresenceStatus, std::string> mapping{
		{ ThisBot::PresenceStatus::ONLINE, "online" },
		{ ThisBot::PresenceStatus::DO_NOT_DISTURB, "dnd" },
		{ ThisBot::PresenceStatus::IDLE, "idle" },
		{ ThisBot::PresenceStatus::INVISIBLE, "invisible" },
		{ ThisBot::PresenceStatus::OFFLINE, "offline" },
	};
	static std::string invalid;

	auto it = mapping.find(status);
	if (it == mapping.end())
		return invalid;

	return it->second;
}

bool ThisBot::SetPresenceStatus(PresenceStatus status)
{
	auto const &status_str = GetPresenceStatusString(status);
	if (status_str.empty())
		return false; // invalid status passed

	m_PresenceStatus = status;
	Network::Get()->WebSocket().UpdateStatus(status_str, m_ActivityName);
	return true;
}

void ThisBot::SetActivity(std::string const &name)
{
	m_ActivityName = name;

	Network::Get()->WebSocket().UpdateStatus(
		GetPresenceStatusString(m_PresenceStatus), m_ActivityName);
}
