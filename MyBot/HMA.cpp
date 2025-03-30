#include "HMA.h"
#include <dpp/dpp.h>
#include <locale>

using json = nlohmann::json;

const std::string DATA_FILE = "animes.json";

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

static std::string trim(const std::string& str) {
	auto start = std::find_if(str.begin(), str.end(), [](unsigned char ch) {
		return !std::isspace(ch);
	});

	auto end = std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
	}).base();

	return (start < end ? std::string(start, end) : "");
}

static bool is_multiple_anime(const std::string anime_str) {
	return anime_str.find(";") != std::string::npos;
}

static std::string add_animes(const std::string& user_id, const std::string& anime_str) {
	json animes = load_data();
	std::stringstream ss(anime_str);
	std::string anime;
	std::vector<std::string> added_animes;

	while (std::getline(ss, anime, ';')) {
		anime = trim(anime);
		animes[user_id].push_back(anime);
		added_animes.push_back(anime);
	}

	std::string response = "Anime **" + added_animes[0] + "** adicionado à sua lista!";

	if (is_multiple_anime(anime_str)) {
		response = "Animes adicionados à sua lista:\n";
		for (const auto& anime : added_animes) {
			response += "- **" + anime + "**\n";
		}
	} 

	save_data(animes);
	return response;
}

static std::string remove_anime(const std::string& user_id, const std::string& anime_name) {
	json data = load_data();
	
	if (data.contains(user_id)) {
		auto& list = data[user_id];
		auto it = std::find(list.begin(), list.end(), anime_name);

		if (it != list.end()) {
			list.erase(it);
			save_data(data);
			return "Anime **" + anime_name + "** removido da sua lista!";
		}
	}

	return "Anime não encontrado na sua lista";
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
	
	for (auto& [user, animes] : data.items()) {
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
			std::string anime_names = std::get<std::string>(event.get_parameter("animes"));
			event.reply(add_animes(user_id, anime_names));
		}

		if (event.command.get_command_name() == "remover") {
			std::string anime_name = std::get<std::string>(event.get_parameter("anime"));
			event.reply(remove_anime(user_id, anime_name));
		}

		if (event.command.get_command_name() == "lista") {
	
			try {
				dpp::snowflake mentioned_user = std::get<dpp::snowflake>(event.get_parameter("de"));
				user_id = std::to_string(mentioned_user);
			} catch (const std::exception&) {

			}
			
 			event.reply(get_user_list(user_id));

		};

		if (event.command.get_command_name() == "listas") {
			event.reply(get_all_lists());
		}
	});

	bot.on_ready([&bot](const dpp::ready_t& event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			dpp::slashcommand add("adicionar", "Adiciona um ou mais animes à sua lista", bot.me.id);
			dpp::slashcommand remove("remover", "Remove um anime da sua lista", bot.me.id);
			dpp::slashcommand list("lista", "Mostra sua própria lista", bot.me.id);
			dpp::slashcommand lists("listas", "Mostra todas as listas", bot.me.id);

			remove.add_option(dpp::command_option(dpp::co_string, "anime", "Nome do anime", true));
			add.add_option(dpp::command_option(
				dpp::co_string, "animes", "Nome dos animes separados por ; (ponto e vírgula)", true
			));

			list.add_option(dpp::command_option(dpp::co_user, "de", "Mostra a lista do usuário mencionado."));

			bot.global_bulk_command_create({ add, remove, list, lists });
		}
	});

	bot.start(dpp::st_wait);

	return 0;
}
