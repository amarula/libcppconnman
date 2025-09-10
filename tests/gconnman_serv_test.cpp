#include <gtest/gtest.h>

#include <amarula/dbus/connman/gconnman.hpp>
#include <amarula/dbus/connman/gservice.hpp>
#include <amarula/dbus/connman/gtechnology.hpp>
#include <iostream>
#include <string>
#include <utility>

using Amarula::DBus::G::Connman::Connman;

using Error = Amarula::DBus::G::Connman::ServProperties::Error;
using State = Amarula::DBus::G::Connman::ServProperties::State;
using Type = Amarula::DBus::G::Connman::TechProperties::Type;

TEST(Connman, getServs) {
    bool called = false;
    {
        Connman connman;
        const auto manager = connman.manager();
        manager->onTechnologiesChanged([&](const auto& technologies) {
            ASSERT_FALSE(technologies.empty()) << "No technologies returned";

            // Power on all technologies
            for (const auto& tech : technologies) {
                tech->onPropertyChanged([&](const auto& prop) {
                    EXPECT_TRUE(prop.isPowered())
                        << "Technology " << prop.getName()
                        << " was not powered ON";
                    std::cout << "onPropertyChanged:\n";
                    prop.print();
                });
                const auto prop = tech->properties();
                const auto name = prop.getName();
                if (!prop.isPowered()) {
                    tech->setPowered(true, [&, name](auto success) {
                        EXPECT_TRUE(success)
                            << "Set power for " << name << " did not succeed";
                    });
                }
            }
        });

        manager->onServicesChanged([&](const auto& services) {
            called = true;
            ASSERT_FALSE(services.empty());
            for (const auto& serv : services) {
                const auto props = serv->properties();
                props.print();
            }
        });
    }
    ASSERT_TRUE(called) << "TechnologiesChanged callback was never called";
}

TEST(Connman, setNameServers) {
    bool called = false;
    {
        Connman connman;
        const auto manager = connman.manager();

        manager->onServicesChanged([&](const auto& services) {
            called = true;
            ASSERT_FALSE(services.empty());
            for (const auto& serv : services) {
                const auto props = serv->properties();
                const auto name = props.getName();
                props.print();
                serv->onPropertyChanged([](const auto& properties) {
                    std::cout << "onPropertyChange:\n";
                    properties.print();
                });
                serv->setNameServers(
                    {"8.8.8.8", "4.4.4.4"}, [&, name](auto success) {
                        EXPECT_TRUE(success) << "Set setNameServers for "
                                             << name << " did not succeed";
                    });
            }
        });
    }
    ASSERT_TRUE(called) << "TechnologiesChanged callback was never called";
}

TEST(Connman, ForgetAndDisconnectService) {
    bool called = false;

    {
        Connman connman;
        const auto manager = connman.manager();

        manager->onServicesChanged([&](const auto& services) {
            called = true;
            ASSERT_FALSE(services.empty()) << "No services returned";
            for (const auto& serv : services) {
                const auto props = serv->properties();
                const auto name = props.getName();
                const auto state = props.getState();

                if (props.getError() != Error::None || props.isFavorite()) {
                    std::cout << "Removing service: " << name << '\n';
                    serv->remove([serv, name](bool success) {
                        EXPECT_TRUE(success);
                        std::cout << "Service removed: " << name << '\n';
                        serv->properties().print();
                    });
                } else if (state == State::Ready || state == State::Online) {
                    std::cout << "Disconnecting service: " << name << '\n';
                    serv->disconnect([serv](bool success) {
                        EXPECT_TRUE(success);
                        std::cout << "Service disconnected: " << serv->objPath()
                                  << '\n';
                        serv->properties().print();
                    });
                }
            }
        });
    }

    ASSERT_TRUE(called) << "ServicesChanged callback was never called";
}

TEST(Connman, ConnectWifi) {
    bool called = false;
    bool called_request_input = false;
    {
        Connman connman;
        const auto manager = connman.manager();

        manager->onRequestInputPassphrase([&](auto service) -> auto {
            called_request_input = true;
            std::cout << "Requesting input passphrase for service: "
                      << service->properties().getName() << '\n';
            const auto name = service->properties().getName();
            EXPECT_EQ(name, "connmantest");
            return std::pair<bool, std::string>{true, "amaruladev"};
        });

        manager->onServicesChanged([&](const auto& services) {
            called = true;
            ASSERT_FALSE(services.empty()) << "No services returned";

            for (const auto& serv : services) {
                const auto props = serv->properties();
                std::cout << "Service: " << props.getName() << "\n";
                const auto name = props.getName();

                if (name == "connmantest") {
                    std::cout << "Test wifi found\n";
                    std::cout << "Connecting to service: " << name << '\n';
                    serv->connect([serv](bool success) {
                        EXPECT_TRUE(success);
                        std::cout
                            << "Service connected successfully: " << success
                            << '\n';
                        serv->properties().print();
                    });
                }
            }
        });

        manager->onTechnologiesChanged([](const auto& technologies) {
            for (const auto& tech : technologies) {
                const auto props = tech->properties();
                if (props.getType() == Type::Wifi) {
                    std::cout << "Scanning wifi technology with name: "
                              << props.getName() << "\n";
                    tech->scan([](bool success) { EXPECT_TRUE(success); });
                }
            }
        });
    }

    ASSERT_TRUE(called) << "ServicesChanged callback was never called";
    ASSERT_TRUE(called_request_input) << "Did not requested user input";
}