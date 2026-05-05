// clang-format off
#include <cstdio>
#include <cstring>
#include <readline/history.h>
#include <readline/readline.h>
// clang-format on

#include <algorithm>
#include <amarula/dbus/connman/gconnman.hpp>
#include <amarula/dbus/connman/gservice.hpp>
#include <amarula/dbus/connman/gtechnology.hpp>
#include <cctype>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <ranges>
#include <sstream>
#include <string>

using Amarula::DBus::G::Connman::Connman;
using Amarula::DBus::G::Connman::TechProperties;
using State = Amarula::DBus::G::Connman::ServProperties::State;
using TechnologyType = TechProperties::Type;

std::atomic<bool> has_message{false};
std::ostringstream message;
std::mutex message_mutex;
std::mutex hash_mutex;
std::mutex cin_mutex;
std::condition_variable cin_cv;

std::string passphrase;
std::string connecting_name;
std::atomic<bool> connecting = false;

auto event_hook() -> int {
    if (has_message.exchange(false)) {
        rl_clear_visible_line();
        {
            std::lock_guard<std::mutex> lock(message_mutex);
            std::cout << message.str();
            message.str("");
            message.clear();
        }
        rl_on_new_line();
        rl_redisplay();
    }

    if (connecting) {
        std::string prompt = "Enter passphrase for " + connecting_name + ": ";

        rl_set_prompt(prompt.c_str());
        rl_redisplay();
    }
    return 0;
}

enum class Command : std::uint8_t {
    Technologies,
    Services,
    Scan,
    Enable,
    Disable,
    Connect,
    Disconnect,
    Remove,
    Agent,
    Quit
};

inline const std::unordered_map<Command, std::string_view> command_map = {
    {Command::Technologies, "technologies"},
    {Command::Services, "services"},
    {Command::Scan, "scan"},
    {Command::Enable, "enable"},
    {Command::Disable, "disable"},
    {Command::Connect, "connect"},
    {Command::Disconnect, "disconnect"},
    {Command::Remove, "remove"},
    {Command::Agent, "agent"},
    {Command::Quit, "quit"}};

inline const std::unordered_map<TechnologyType, std::string_view> tech_map = {
    {TechnologyType::Ethernet, "ethernet"},
    {TechnologyType::Wifi, "wifi"},
    {TechnologyType::Cellular, "cellular"},
    {TechnologyType::Bluetooth, "bluetooth"},
    {TechnologyType::Vpn, "vpn"},
    {TechnologyType::Wired, "wired"},
    {TechnologyType::P2p, "p2p"},
    {TechnologyType::Gps, "gps"},
    {TechnologyType::Gadget, "gadget"}};

inline const std::vector<std::string> commands_container = [] {
    std::vector<std::string> string_vector;
    string_vector.reserve(command_map.size());

    for (const auto& [cmd, name] : command_map) {
        string_vector.emplace_back(name);
    }

    return string_vector;
}();

inline const std::vector<std::string> on_off_container{"on", "off"};
inline const std::vector<std::string> enable_disable_container{"enable",
                                                               "disable"};

inline std::vector<std::string> services_container;
inline std::vector<std::string> technologies_container;

void printContainer(const std::vector<std::string>& container) {
    for (size_t i = 0; i < container.size(); ++i) {
        std::cout << container[i];
        if (i + 1 < container.size()) {
            std::cout << ", ";
        }
    }
    std::cout << "\n";
}

enum class Mode : std::uint8_t { Command, ServiceHash, TechHash, OnOff };

inline Mode completion_mode = Mode::Command;

auto command_generator(const char* text, int state) -> char* {
    static size_t index;
    static size_t len;
    static std::vector<std::string> variable_container;

    if (state == 0) {
        index = 0;
        len = strlen(text);
        if (completion_mode == Mode::Command) {
            variable_container = commands_container;
        } else if (completion_mode == Mode::ServiceHash) {
            std::lock_guard<std::mutex> lock(hash_mutex);
            variable_container = services_container;
        } else if (completion_mode == Mode::TechHash) {
            std::lock_guard<std::mutex> lock(hash_mutex);
            variable_container = technologies_container;
        } else if (completion_mode == Mode::OnOff) {
            variable_container = on_off_container;
        }
    }

    while (index < variable_container.size()) {
        const auto cmd = variable_container[index++];

        if (cmd.compare(0, len, text) == 0) {
            return strdup(cmd.c_str());
        }
    }

    return nullptr;
}

auto completion(const char* text, int start, int end) -> char** {
    (void)start;
    (void)end;
    if (start == 0) {
        completion_mode = Mode::Command;
    } else {
        std::string line = rl_line_buffer;
        std::istringstream iss(line);
        std::string command;
        std::string arg;
        iss >> command >> arg;
        if (command == command_map.at(Command::Agent)) {
            completion_mode = Mode::OnOff;
        } else if (command == command_map.at(Command::Enable) ||
                   command == command_map.at(Command::Disable) ||
                   command == command_map.at(Command::Scan)) {
            completion_mode = Mode::TechHash;
        } else if (command == command_map.at(Command::Connect) ||
                   command == command_map.at(Command::Disconnect) ||
                   command == command_map.at(Command::Remove) ||
                   command == command_map.at(Command::Services)) {
            completion_mode = Mode::ServiceHash;
        } else {
            return nullptr;
        }
        if (!arg.empty() && rl_point > 0 && rl_point == line.size() &&
            line[rl_point - 1] == ' ') {
            return nullptr;
        }
    }

    return rl_completion_matches(text, command_generator);
}

auto main() -> int {
    rl_event_hook = event_hook;
    rl_attempted_completion_function = completion;
    Connman connman;
    const auto manager = connman.manager();
    manager->onRequestInputPassphrase([&](const auto& service) -> auto {
        const auto name = service->properties().getName();
        std::unique_lock<std::mutex> lock(cin_mutex);
        connecting = true;
        connecting_name = name;
        cin_cv.wait(lock, [] { return !connecting; });
        return std::pair<bool, std::string>{true, passphrase};
    });
    manager->onServicesChanged([](const auto& services) {
        std::lock_guard<std::mutex> lock(hash_mutex);
        services_container.clear();
        for (const auto& serv : services) {
            const auto path = serv->objPath();
            services_container.push_back(
                path.substr(path.find_last_of('/') + 1));
            const auto props = serv->properties();
            const auto name = props.getName();
            if (!name.empty()) {
                services_container.push_back(name);
            }
        }
    });
    manager->onTechnologiesChanged([](const auto& technologies) {
        std::lock_guard<std::mutex> lock(hash_mutex);
        technologies_container.clear();
        for (const auto& tech : technologies) {
            const auto props = tech->properties();
            technologies_container.push_back(
                tech_map.at(props.getType()).data());
        }
    });

    std::string line;
    while (true) {
        char* raw_line = readline("connmanctl> ");
        if (raw_line == nullptr) {
            break;
        }

        line = std::string(raw_line);
        free(raw_line);

        if (connecting) {
            {
                std::lock_guard<std::mutex> lock(cin_mutex);
                passphrase = line;
                connecting = false;
                cin_cv.notify_all();
            }
            rl_set_prompt("connmanctl> ");
            cin_cv.notify_all();
            continue;
        }

        if (!line.empty()) {
            add_history(line.c_str());
        }
        // Trim whitespace
        line.erase(line.begin(), std::ranges::find_if(line, [](int character) {
                       return std::isspace(character) == 0;
                   }));
        line.erase(std::ranges::find_if(std::ranges::reverse_view(line),
                                        [](int character) {
                                            return std::isspace(character) == 0;
                                        })
                       .base(),
                   line.end());
        std::istringstream iss(line);
        std::string cmd;
        std::string arg;
        iss >> cmd >> arg;
        auto match_service = [&arg](const auto& service) {
            const auto props = service->properties();
            const auto name = props.getName();
            const auto path = service->objPath();
            const auto last = path.substr(path.find_last_of('/') + 1);

            return last == arg || name == arg;
        };

        if (cmd == command_map.at(Command::Quit)) {
            break;
        }
        if (cmd == command_map.at(Command::Technologies)) {
            const auto techs = manager->technologies();
            if (techs.empty()) {
                std::cout << "No technologies available.\n";
            } else {
                for (const auto& tech : techs) {
                    const auto props = tech->properties();
                    std::cout << "Technology: " << props.getName() << " "
                              << tech->objPath() << "\n";
                    std::cout << props;
                }
            }
        } else if (cmd == command_map.at(Command::Agent)) {
            auto on_register = [arg](const auto success) {
                if (success) {
                    std::lock_guard<std::mutex> lock(message_mutex);
                    message
                        << "Agent "
                        << ((arg == on_off_container.at(0)) ? "registered"
                                                            : "unregistered")
                        << "\n";
                }
                has_message = true;
            };
            if (arg == "on") {
                manager->registerAgent(manager->internalAgentPath(),
                                       on_register);
            } else if (arg == "off") {
                manager->unregisterAgent(manager->internalAgentPath(),
                                         on_register);
            } else {
                std::cout << "Usage: agent " << on_off_container.at(0) << "/"
                          << on_off_container.at(1) << "\n";
            }
        } else if (cmd == command_map.at(Command::Services)) {
            const auto services = manager->services();
            if (services.empty()) {
                std::cout << "No services available.\n";
            } else {
                if (arg.empty()) {
                    for (const auto& service : services) {
                        const auto props = service->properties();
                        if (props.isFavorite()) {
                            std::cout << "*";
                        }
                        if (props.isAutoconnect()) {
                            std::cout << "A";
                        }
                        if (props.getState() == State::Online) {
                            std::cout << "O";
                        } else if (props.getState() == State::Ready) {
                            std::cout << "R";
                        }
                        const auto path = service->objPath();
                        std::cout << " " << props.getName() << " "
                                  << path.substr(path.find_last_of('/') + 1)
                                  << "\n";
                    }
                } else {
                    auto iterator =
                        std::ranges::find_if(services, match_service);
                    if (iterator != services.end()) {
                        const auto props = (*iterator)->properties();
                        std::cout << "Service: " << props.getName() << " "
                                  << (*iterator)->objPath() << "\n";
                        std::cout << props;
                    } else {
                        std::cout << "Service not found: " << arg << "\n";
                    }
                }
            }
        } else if (cmd == command_map.at(Command::Scan)) {
            {
                std::lock_guard<std::mutex> lock(hash_mutex);
                if (arg.empty() ||
                    std::find(technologies_container.begin(),
                              technologies_container.end(),
                              arg) == technologies_container.end()) {
                    std::cout << "Usage: scan <technology>\n";
                    std::cout << "Available technologies: ";
                    printContainer(technologies_container);
                    continue;
                }
            }
            const auto techs = manager->technologies();
            for (const auto& tech : techs) {
                const auto props = tech->properties();
                const auto name = props.getName();
                if (tech_map.at(props.getType()) == arg) {
                    std::cout << "Scanning " << name << "...\n";
                    tech->scan([name](bool success) {
                        {
                            std::lock_guard<std::mutex> lock(message_mutex);
                            if (success) {
                                message << "Technology " << name
                                        << " scanned successfully.\n";
                            } else {
                                message << "Failed to scan technology " << name
                                        << ".\n";
                            }
                        }
                        has_message = true;
                    });
                    break;
                }
            }
        } else if (cmd == command_map.at(Command::Enable) ||
                   cmd == command_map.at(Command::Disable)) {
            {
                std::lock_guard<std::mutex> lock(hash_mutex);
                if (arg.empty() ||
                    std::find(technologies_container.begin(),
                              technologies_container.end(),
                              arg) == technologies_container.end()) {
                    std::cout << "Usage: " << enable_disable_container[0] << "/"
                              << enable_disable_container[1]
                              << " <technology>\n";
                    std::cout << "Available technologies:";
                    printContainer(technologies_container);
                    continue;
                }
            }
            const bool enable = (cmd == enable_disable_container.at(0));
            std::cout << (enable ? "Enabling" : "Disabling")
                      << " technology: " << arg << "\n";
            const auto techs = manager->technologies();
            for (const auto& tech : techs) {
                const auto props = tech->properties();
                const auto name = props.getName();
                if (tech_map.at(props.getType()) == arg) {
                    if ((!props.isPowered() &&
                         cmd == enable_disable_container.at(0)) ||
                        (props.isPowered() &&
                         cmd == enable_disable_container.at(1))) {
                        std::cout << (enable ? "Enabling " : "Disabling ")
                                  << name << "...\n";
                        tech->setPowered(enable, [name, enable](bool success) {
                            {
                                std::lock_guard<std::mutex> lock(message_mutex);
                                if (success) {
                                    message
                                        << "Technology " << name
                                        << (enable ? " enabled" : " disabled")
                                        << " successfully.\n";
                                } else {
                                    message << "Failed to  "
                                            << (enable ? " enable" : " disable")
                                            << " technology " << name << "\n";
                                }
                            }

                            has_message = true;
                        });

                    } else {
                        std::cout << "Technology " << name << " is already "
                                  << (enable ? "enabled" : "disabled") << "\n";
                    }
                }
            }
        } else if (cmd == command_map.at(Command::Connect) ||
                   cmd == command_map.at(Command::Disconnect)) {
            if (arg.empty()) {
                std::cout << "Usage: connect/disconnect <service_name>\n";
                continue;
            }
            const bool connect = (cmd == command_map.at(Command::Connect));
            const auto services = manager->services();
            auto iterator = std::ranges::find_if(services, match_service);

            if (iterator != services.end()) {
                const auto name = (*iterator)->properties().getName();
                const auto state = (*iterator)->properties().getState();
                if ((state != State::Ready && state != State::Online &&
                     connect) ||
                    (state == State::Ready || state == State::Online) &&
                        !connect) {
                    std::cout << (connect ? "Connecting" : "Disconnecting")
                              << " to service: " << name << "\n";
                    const auto on_connect = [name, connect](bool success) {
                        std::lock_guard<std::mutex> lock_message(message_mutex);
                        if (success) {
                            message
                                << "Service " << name
                                << (connect ? " connected" : " disconnected")
                                << " successfully.\n";
                        } else {
                            message << "Failed to "
                                    << (connect ? " connect" : " disconnect")
                                    << " to service " << name << "\n";
                        }
                        has_message = true;
                    };
                    if (connect) {
                        (*iterator)->connect(on_connect);

                    } else {
                        (*iterator)->disconnect(on_connect);
                    }
                } else {
                    std::cout << "Service " << name << " is already "
                              << (connect ? " connected" : " disconnected")
                              << "\n";
                }
            } else {
                std::cout << "Service " << arg << " not available.\n";
            }
        } else if (cmd == command_map.at(Command::Remove)) {
            if (arg.empty()) {
                std::cout << "Usage: remove <service_name>\n";
                continue;
            }
            const auto services = manager->services();
            auto iterator = std::ranges::find_if(services, match_service);

            if (iterator != services.end()) {
                const auto name = (*iterator)->properties().getName();
                std::cout << "Removing service: " << name << "\n";
                (*iterator)->remove([name](bool success) {
                    {
                        std::lock_guard<std::mutex> lock(message_mutex);
                        if (success) {
                            message << "Service " << name
                                    << " removed successfully.\n";
                        } else {
                            message << "Failed to remove service " << name
                                    << "\n";
                        }
                    }
                    has_message = true;
                });
            } else {
                std::cout << "Service " << arg << " not available.\n";
            }
        } else {
            if (!cmd.empty()) {
                std::cout << "Unknown command: " << cmd
                          << ".\n Available commands: ";
                printContainer(commands_container);
            }
        }
    }
    std::cout << "Exiting.\n";
    return 0;
}
