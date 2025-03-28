#include "HMA.h"
#include <dpp/dpp.h>
#include <locale>

using json = nlohmann::json;

const std::string DATA_FILE = "animes.json";
const std::string BOT_PREFIX = "!";

static json load_data() {
	std::ifstream file(DATA_FILE);
	if (!file) return json::object();
	json data;
	file >> data;
	return data; 
}

static void save_data(const json & data) {
	std::ofstream file(DATA_FILE);
	file << data.dump(4);
}

static void add_anime(const std::string& user_id, const std::string& anime_name) {
	json data = load_data();
	data[user_id].push_back(anime_name);
	save_data(data);
}

static bool remove_anime(const std::string& user_id, const std::string& anime_name) {
	json data = load_data();
	
	if (data.contains(user_id)) {
		auto& list = data[user_id];
		auto it = std::find(list.begin(), list.end(), anime_name);

		if (it != list.end()) {
			list.erase(it);
			save_data(data);
			return true;
		}
	}
	return false;
}

static std::string get_user_list(const std::string& user_id) {
	json data = load_data();
	
	if (!data.contains(user_id) || data[user_id].empty())
		return "Hmmm... parece que a lista de animes de <@" + user_id + "> está vazia 😞. ミク光線放射";

	std::string anime_list = "**Lista de animes de <@" + user_id + ">**\n";
	
	for (const auto& anime : data[user_id]) {
		anime_list += "- " + anime.get<std::string>() + "\n";
	}

	return anime_list;
}

static std::string get_all_lists() {
	json data = load_data();

	if (data.empty()) return "Nenhuma lista encontrada 😞. ミク光線放射";

	std::string anime_list = "**Listas de animes:**\n";
	
	for (auto & [user, animes] : data.items()) {
		anime_list += "<@" + user + "> Está assistindo:\n";
		
		for (const auto & anime : animes) {
			anime_list += "- " + anime.get<std::string>() + "\n";
		}
		anime_list += "\n";
	}
	return anime_list;
}

static std::vector<std::string> split(std::string& str) {
	std::vector<std::string> parts;
	std::stringstream ss(str);
	std::string part;

	while (ss >> part) {
		parts.push_back(part);
	}

	return parts;
}

static std::pair<std::string, std::string> split_once(const std::string& message) {
	size_t space_pos = message.find(" ");
	if (space_pos != std::string::npos) {
		return {
			message.substr(0, space_pos),
			message.substr(space_pos + 1)
		};
	}
	return { message, "" };
}

int main()
{
	uint64_t intents = dpp::i_default_intents | dpp::i_message_content;
	size_t len;
	char* value;
	std::string BOT_TOKEN = "token";

	const errno_t result = _dupenv_s(&value, &len, "BOT_TOKEN");

	if (result == 0 && value != nullptr) {
		BOT_TOKEN = value;
		free(value);
	}

	std::locale::global(std::locale("pt-BR.UTF-8"));
	dpp::cluster bot(BOT_TOKEN, intents);

	bot.on_log(dpp::utility::cout_logger());

	bot.on_slashcommand([&bot] (const dpp::slashcommand_t& event) {
		std::string user_id = std::to_string(event.command.usr.id);

		if (event.command.get_command_name() == "adicionar") {
			std::string anime_name = std::get<std::string>(event.get_parameter("anime"));
			add_anime(user_id, anime_name);
			event.reply("Anime **" + anime_name + "** adicionado à sua lista!");
		}

		if (event.command.get_command_name() == "remover") {
			std::string anime_name = std::get<std::string>(event.get_parameter("anime"));
			if (remove_anime(user_id, anime_name))
				event.reply("Anime **" + anime_name + "** removido da sua lista!");
			else
				event.reply("Anime não encontrado na sua lista");
		}

		if (event.command.get_command_name() == "lista") {
	
			try {
				dpp::snowflake mentioned_user = std::get<dpp::snowflake>(event.get_parameter("de"));
				user_id = std::to_string(mentioned_user);
			} catch (const std::exception &) {

			}
			
 			event.reply(get_user_list(user_id));

		};

		if (event.command.get_command_name() == "listas") {
			event.reply(get_all_lists());
		}
	});

	bot.on_message_create([&bot] (const dpp::message_create_t& event) {
		if (event.msg.author.is_bot()) return;

		std::string message = event.msg.content;
		std::string user_id = std::to_string(event.msg.author.id);

		if (message.rfind(BOT_PREFIX, 0) == 0) {
			auto [command, argument] = split_once(message.substr(BOT_PREFIX.size()));

			if (command == "lista") {
				if (!argument.empty()) 
					user_id = argument.substr(2, 18);

				event.reply(get_user_list(user_id));
			} else if (command == "listas") {
				event.reply(get_all_lists());
			} else if (command == "adicionar") {
				if (argument.empty()) {
					event.reply("Por favor coloque o nome do anime usando o comando ```!adicionar nome do anime```");
				}
				else {
					add_anime(user_id, argument);
					event.reply("Anime **" + argument + "** adicionado à sua lista!");
				}
			} else if (!argument.empty()) {

				if (remove_anime(user_id, argument)) {
					event.reply("Anime **" + argument + "** removido da sua lista!");
				}
				else {
					event.reply("Anime não encontrado na sua lista");
				}
			} 
		}
	});

	bot.on_ready([&bot] (const dpp::ready_t& event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			dpp::slashcommand add("adicionar", "Adiciona um anime à sua lista", bot.me.id);
			dpp::slashcommand remove("remover", "Remove um anime da sua lista", bot.me.id);
			dpp::slashcommand list("lista", "Mostra sua própria lista", bot.me.id);
			dpp::slashcommand lists("listas", "Mostra todas as listas", bot.me.id);

			add.add_option(dpp::command_option(dpp::co_string, "anime", "Nome do anime", true));
			remove.add_option(dpp::command_option(dpp::co_string, "anime", "Nome do anime", true));
			list.add_option(
				dpp::command_option(dpp::co_user, "de", "Mostra a lista do usuário mencionado.")
			);

			bot.global_bulk_command_create({ add, remove, list, lists });
		}
	});

	bot.start(dpp::st_wait);

	return 0;
}
