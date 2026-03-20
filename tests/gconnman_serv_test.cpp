#include <gtest/gtest.h>

#include <amarula/dbus/connman/gconnman.hpp>
#include <amarula/dbus/connman/gservice.hpp>
#include <amarula/dbus/connman/gtechnology.hpp>
#include <iostream>
#include <string>
#include <utility>

#include "qt_thread_bundle.hpp"

using Amarula::DBus::G::Connman::Connman;

using Error = Amarula::DBus::G::Connman::ServProperties::Error;
using State = Amarula::DBus::G::Connman::ServProperties::State;
using Type = Amarula::DBus::G::Connman::TechProperties::Type;
using ServType = Amarula::DBus::G::Connman::ServProperties::Type;

TEST(Connman, getServs) {
    bool called = false;
    {
        const QtThreadBundle qt_thread_bundle;
        const Connman connman;
        const auto manager = connman.manager();
        manager->onTechnologiesChanged([main_tid = qt_thread_bundle.main_tid,
                                        loop_tid = qt_thread_bundle.loop_tid](
                                           const auto& technologies) {
            const auto callback_tid = std::this_thread::get_id();
            EXPECT_NE(callback_tid, main_tid);
            EXPECT_NE(callback_tid, loop_tid);
            ASSERT_FALSE(technologies.empty()) << "No technologies returned";

            // Power on all technologies
            for (const auto& tech : technologies) {
                tech->onPropertyChanged([main_tid, loop_tid](const auto& prop) {
                    const auto callback_tid = std::this_thread::get_id();
                    EXPECT_NE(callback_tid, main_tid);
                    EXPECT_NE(callback_tid, loop_tid);
                    EXPECT_TRUE(prop.isPowered())
                        << "Technology " << prop.getName()
                        << " was not powered ON";
                    std::cout << "onPropertyChanged:\n";
                    prop.print();
                });
                const auto prop = tech->properties();
                const auto name = prop.getName();
                if (!prop.isPowered()) {
                    tech->setPowered(true, [main_tid, loop_tid,
                                            name](auto success) {
                        const auto callback_tid = std::this_thread::get_id();
                        EXPECT_NE(callback_tid, main_tid);
                        EXPECT_NE(callback_tid, loop_tid);
                        EXPECT_TRUE(success)
                            << "Set power for " << name << " did not succeed";
                    });
                }
            }
        });

        manager->onServicesChanged(
            [&called, main_tid = qt_thread_bundle.main_tid,
             loop_tid = qt_thread_bundle.loop_tid](const auto& services) {
                called = true;
                const auto callback_tid = std::this_thread::get_id();
                EXPECT_NE(callback_tid, main_tid);
                EXPECT_NE(callback_tid, loop_tid);
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
        const QtThreadBundle qt_thread_bundle;
        const Connman connman;
        const auto manager = connman.manager();

        manager->onServicesChanged(
            [&called, main_tid = qt_thread_bundle.main_tid,
             loop_tid = qt_thread_bundle.loop_tid](const auto& services) {
                called = true;
                const auto callback_tid = std::this_thread::get_id();
                EXPECT_NE(callback_tid, main_tid);
                EXPECT_NE(callback_tid, loop_tid);
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
                        {"8.8.8.8", "4.4.4.4"},
                        [name, main_tid, loop_tid](auto success) {
                            const auto callback_tid =
                                std::this_thread::get_id();
                            EXPECT_NE(callback_tid, main_tid);
                            EXPECT_NE(callback_tid, loop_tid);
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
        const QtThreadBundle qt_thread_bundle;
        const Connman connman;
        const auto manager = connman.manager();

        manager->onServicesChanged([&called,
                                    main_tid = qt_thread_bundle.main_tid,
                                    loop_tid = qt_thread_bundle.loop_tid](
                                       const auto& services) {
            called = true;
            const auto callback_tid = std::this_thread::get_id();
            EXPECT_NE(callback_tid, main_tid);
            EXPECT_NE(callback_tid, loop_tid);
            ASSERT_FALSE(services.empty()) << "No services returned";
            for (const auto& serv : services) {
                const auto props = serv->properties();
                const auto name = props.getName();
                const auto state = props.getState();

                if ((props.getError() != Error::None || props.isFavorite()) &&
                    props.getType() != ServType::Ethernet) {
                    std::cout << "Removing service: " << name << '\n';
                    serv->remove([serv, name, main_tid,
                                  loop_tid](bool success) {
                        const auto callback_tid = std::this_thread::get_id();
                        EXPECT_NE(callback_tid, main_tid);
                        EXPECT_NE(callback_tid, loop_tid);
                        EXPECT_TRUE(success);
                        std::cout << "Service removed: " << name << '\n';
                        serv->properties().print();
                    });
                } else if (state == State::Ready || state == State::Online) {
                    std::cout << "Disconnecting service: " << name << '\n';
                    serv->disconnect([serv, main_tid, loop_tid](bool success) {
                        const auto callback_tid = std::this_thread::get_id();
                        EXPECT_NE(callback_tid, main_tid);
                        EXPECT_NE(callback_tid, loop_tid);
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
        const QtThreadBundle qt_thread_bundle;
        const Connman connman;
        const auto manager = connman.manager();

        manager->onRequestInputPassphrase(
            [&called_request_input, main_tid = qt_thread_bundle.main_tid,
             loop_tid = qt_thread_bundle.loop_tid](auto service) -> auto {
                const auto callback_tid = std::this_thread::get_id();
                EXPECT_NE(callback_tid, main_tid);
                EXPECT_NE(callback_tid, loop_tid);
                called_request_input = true;
                std::cout << "Requesting input passphrase for service: "
                          << service->properties().getName() << '\n';
                const auto name = service->properties().getName();
                EXPECT_EQ(name, "connmantest");
                return std::pair<bool, std::string>{true, "amaruladev"};
            });

        manager->onServicesChanged([&called, manager,
                                    main_tid = qt_thread_bundle.main_tid,
                                    loop_tid = qt_thread_bundle.loop_tid](
                                       const auto& services) {
            const auto callback_tid = std::this_thread::get_id();
            EXPECT_NE(callback_tid, main_tid);
            EXPECT_NE(callback_tid, loop_tid);
            called = true;
            ASSERT_FALSE(services.empty()) << "No services returned";

            for (const auto& serv : services) {
                const auto props = serv->properties();
                std::cout << "Service: " << props.getName() << "\n";
                const auto name = props.getName();
                const auto state = props.getState();

                if (name == "connmantest") {
                    std::cout << "Test wifi found\n";

                    if (state == State::Idle) {
                        std::cout << "Connecting to service: " << name << '\n';
                        props.print();
                        serv->onPropertyChanged(
                            [main_tid, loop_tid](const auto& properties) {
                                const auto callback_tid =
                                    std::this_thread::get_id();
                                EXPECT_NE(callback_tid, main_tid);
                                EXPECT_NE(callback_tid, loop_tid);
                                std::cout << "onPropertyChange:\n";
                                properties.print();
                            });
                        manager->registerAgent(
                            manager->internalAgentPath(),
                            [serv, manager, main_tid,
                             loop_tid](const auto success) {
                                const auto callback_tid =
                                    std::this_thread::get_id();
                                EXPECT_NE(callback_tid, main_tid);
                                EXPECT_NE(callback_tid, loop_tid);
                                EXPECT_TRUE(success);
                                serv->connect([serv, manager, main_tid,
                                               loop_tid](bool success) {
                                    const auto callback_tid =
                                        std::this_thread::get_id();
                                    EXPECT_NE(callback_tid, main_tid);
                                    EXPECT_NE(callback_tid, loop_tid);
                                    EXPECT_TRUE(success);
                                    std::cout
                                        << "Service connected successfully: "
                                        << success << '\n';
                                    serv->properties().print();
                                    manager->unregisterAgent(
                                        manager->internalAgentPath());
                                });
                            });
                    }

                    break;
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
