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

auto main() -> int {
    std::mutex cin_mutex;
    std::condition_variable cin_cv;
    bool connecting = false;
    Connman connman;
    const auto manager = connman.manager();
    manager->onRequestInputPassphrase([&](const auto& service) -> auto {
        const auto name = service->properties().getName();

        std::string passphrase;
        {
            const std::unique_lock<std::mutex> lock(cin_mutex);
            std::cout << "Enter passphrase for " << name << " service\n";
            std::getline(std::cin, passphrase);
            connecting = false;
        }
        cin_cv.notify_one();
        return std::pair<bool, std::string>{true, passphrase};
    });
    constexpr int CIN_WAIT_TIMEOUT_SECONDS = 30;
    std::string line;
    while (true) {
        std::cout << "connmanctl> ";
        {
            std::unique_lock<std::mutex> lock(cin_mutex);
            cin_cv.wait_for(lock,
                            std::chrono::seconds(CIN_WAIT_TIMEOUT_SECONDS),
                            [&connecting] { return !connecting; });
            if (!std::getline(std::cin, line)) {
                break;  // EOF or error
            }
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

        if (cmd == "exit" || cmd == "quit") {
            break;
        }
        if (cmd == "technologies") {
            const auto techs = manager->technologies();
            if (techs.empty()) {
                std::cout << "No technologies available.\n";
            } else {
                for (const auto& tech : techs) {
                    const auto props = tech->properties();
                    std::cout << "Technology: " << props.getName() << " "
                              << tech->objPath() << "\n";
                    props.print();
                }
            }
        }
        if (cmd == "agent") {
            auto on_register = [arg](const auto success) {
                if (success) {
                    std::cout << "Agent "
                              << ((arg == "on") ? "registered" : "unregistered")
                              << "\n";
                }
            };
            if (arg == "on") {
                manager->registerAgent(manager->internalAgentPath(),
                                       on_register);
            } else if (arg == "off") {
                manager->unregisterAgent(manager->internalAgentPath(),
                                         on_register);
            } else {
                std::cout << "Usage: agent on/off\n";
            }
        } else if (cmd == "services") {
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
                        std::cout << " " << props.getName() << " "
                                  << service->objPath() << "\n";
                    }
                } else {
                    auto iterator = std::ranges::find_if(
                        services, [&arg](const auto& service) {
                            return service->objPath() == arg ||
                                   service->properties().getName() == arg;
                        });
                    if (iterator != services.end()) {
                        const auto props = (*iterator)->properties();
                        std::cout << "Service: " << props.getName() << " "
                                  << (*iterator)->objPath() << "\n";
                        props.print();
                    } else {
                        std::cout << "Service not found: " << arg << "\n";
                    }
                }
            }
        } else if (cmd == "scan") {
            if (arg.empty() ||
                (arg != "wifi" && arg != "ethernet" && arg != "bluetooth" &&
                 arg != "cellular" && arg != "vpn")) {
                std::cout << "Usage: scan <technology>\n";
                std::cout << "Available technologies: wifi, ethernet, "
                             "bluetooth, cellular, vpn\n";
                continue;
            }
            const auto techs = manager->technologies();
            for (const auto& tech : techs) {
                const auto props = tech->properties();
                const auto name = props.getName();
                if ((props.getType() == TechProperties::Type::Wifi &&
                     arg == "wifi") ||
                    (props.getType() == TechProperties::Type::Ethernet &&
                     arg == "ethernet") ||
                    (props.getType() == TechProperties::Type::Bluetooth &&
                     arg == "bluetooth") ||
                    (props.getType() == TechProperties::Type::Cellular &&
                     arg == "cellular") ||
                    (props.getType() == TechProperties::Type::Vpn &&
                     arg == "vpn")) {
                    std::cout << "Scanning " << name << "...\n";
                    tech->scan([name](bool success) {
                        if (success) {
                            std::cout << "Technology " << name
                                      << " scanned successfully.\n";
                        } else {
                            std::cout << "Failed to scan technology " << name
                                      << ".\n";
                        }
                    });
                    break;
                }
            }
        } else if (cmd == "enable" || cmd == "disable") {
            if (arg.empty() ||
                (arg != "wifi" && arg != "ethernet" && arg != "bluetooth" &&
                 arg != "cellular" && arg != "vpn")) {
                std::cout << "Usage: enable/disable <technology>\n";
                std::cout << "Available technologies: wifi, ethernet, "
                             "bluetooth, cellular, vpn\n";
                continue;
            }
            const bool enable = (cmd == "enable");
            std::cout << (enable ? "Enabling" : "Disabling")
                      << " technology: " << arg << "\n";
            const auto techs = manager->technologies();
            for (const auto& tech : techs) {
                const auto props = tech->properties();
                const auto name = props.getName();
                if ((props.getType() == TechProperties::Type::Wifi &&
                     arg == "wifi") ||
                    (props.getType() == TechProperties::Type::Ethernet &&
                     arg == "ethernet") ||
                    (props.getType() == TechProperties::Type::Bluetooth &&
                     arg == "bluetooth") ||
                    (props.getType() == TechProperties::Type::Cellular &&
                     arg == "cellular") ||
                    (props.getType() == TechProperties::Type::Vpn &&
                     arg == "vpn")) {
                    if ((!props.isPowered() && cmd == "enable") ||
                        (props.isPowered() && cmd == "disable")) {
                        std::cout << (enable ? "Enabling " : "Disabling ")
                                  << name << "...\n";
                        tech->setPowered(enable, [name, enable](bool success) {
                            if (success) {
                                std::cout << "Technology " << name
                                          << (enable ? " enabled" : " disabled")
                                          << " successfully.\n";
                            } else {
                                std::cout << "Failed to  "
                                          << (enable ? " enable" : " disable")
                                          << " technology " << name << "\n";
                            }
                        });

                    } else {
                        std::cout << "Technology " << name << " is already "
                                  << (enable ? "enabled" : "disabled") << "\n";
                    }
                }
            }
        } else if (cmd == "connect" || cmd == "disconnect") {
            if (arg.empty()) {
                std::cout << "Usage: connect/disconnect <service_name>\n";
                continue;
            }
            const bool connect = (cmd == "connect");
            const auto services = manager->services();
            auto iterator =
                std::ranges::find_if(services, [&arg](const auto& service) {
                    return service->objPath() == arg ||
                           service->properties().getName() == arg;
                });

            if (iterator != services.end()) {
                const auto name = (*iterator)->properties().getName();
                const auto state = (*iterator)->properties().getState();
                if ((state != State::Ready && state != State::Online &&
                     connect) ||
                    (state == State::Ready || state == State::Online) &&
                        !connect) {
                    std::cout << (connect ? "Connecting" : "Disconnecting")
                              << " to service: " << name << "\n";
                    const auto on_connect = [name, connect, &connecting,
                                             &cin_mutex,
                                             &cin_cv](bool success) {
                        if (success) {
                            std::cout
                                << "Service " << name
                                << (connect ? " connected" : " disconnected")
                                << " successfully.\n";
                        } else {
                            std::cout << "Failed to "
                                      << (connect ? " connect" : " disconnect")
                                      << " to service " << name << "\n";
                        }
                        {
                            const std::unique_lock<std::mutex> lock(cin_mutex);
                            connecting = false;
                        }
                        cin_cv.notify_one();
                    };
                    if (connect) {
                        (*iterator)->connect(on_connect);
                        connecting = true;
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
        } else if (cmd == "remove") {
            if (arg.empty()) {
                std::cout << "Usage: remove <service_name>\n";
                continue;
            }
            const auto services = manager->services();
            auto iterator =
                std::ranges::find_if(services, [&arg](const auto& service) {
                    return service->objPath() == arg ||
                           service->properties().getName() == arg;
                });

            if (iterator != services.end()) {
                const auto name = (*iterator)->properties().getName();
                std::cout << "Removing service: " << name << "\n";
                (*iterator)->remove([name](bool success) {
                    if (success) {
                        std::cout << "Service " << name
                                  << " removed successfully.\n";
                    } else {
                        std::cout << "Failed to remove service " << name
                                  << "\n";
                    }
                });
            } else {
                std::cout << "Service " << arg << " not available.\n";
            }
        } else {
            if (!cmd.empty()) {
                std::cout
                    << "Unknown command: " << cmd
                    << ". Available commands: technologies, services, "
                       "scan, enable, disable, connect, disconnect, remove.\n";
            }
        }
    }
    std::cout << "Exiting.\n";
    return 0;
}
