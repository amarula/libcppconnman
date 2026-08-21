#include <gtest/gtest.h>

#include <amarula/dbus/connman/gconnman.hpp>
#include <amarula/dbus/connman/gtechnology.hpp>

#include "thread_bundle.hpp"

using Amarula::DBus::G::Connman::Connman;
using Type = Amarula::DBus::G::Connman::TechProperties::Type;

constexpr uint32_t WIFI_FREQ_2412_MHZ = 2412;

TEST(Connman, getTechs) {
    bool called = false;
    {
        const ThreadBundle thread_bundle;
        Connman connman;
        const auto manager = connman.manager();

        auto do_on_techs = [&called, main_tid = thread_bundle.main_tid,
                            loop_tid = thread_bundle.loop_tid](
                               const auto& technologies,
                               const bool check_thread_id = true) {
            called = true;
            if (check_thread_id) {
                const auto callback_tid = std::this_thread::get_id();
                EXPECT_NE(callback_tid, main_tid);
                EXPECT_NE(callback_tid, loop_tid);
            }
            ASSERT_FALSE(technologies.empty()) << "No technologies returned";
            for (const auto& tech : technologies) {
                const auto props = tech->properties();
                EXPECT_FALSE(props.getName().empty());
                if (props.isConnected()) {
                    EXPECT_TRUE(props.isPowered())
                        << "Technology is connected but not powered";
                }
                std::cout << props;
            }
        };

        if (manager->technologies().empty()) {
            manager->onTechnologiesChanged(do_on_techs);
        } else {
            do_on_techs(manager->technologies(), false);
        }
    }
    ASSERT_TRUE(called) << "TechnologiesChanged callback was never called";
}

TEST(Connman, PowerOnAllTechnologies) {
    bool called = false;

    {
        const ThreadBundle thread_bundle;
        Connman connman;
        const auto manager = connman.manager();

        auto do_on_techs = [&called, main_tid = thread_bundle.main_tid,
                            loop_tid = thread_bundle.loop_tid](
                               const auto& technologies,
                               const bool check_thread_id = true) {
            if (check_thread_id) {
                const auto callback_tid = std::this_thread::get_id();
                EXPECT_NE(callback_tid, main_tid);
                EXPECT_NE(callback_tid, loop_tid);
            }
            ASSERT_FALSE(technologies.empty()) << "No technologies returned";
            // Power on all technologies
            for (const auto& tech : technologies) {
                tech->onPropertyChanged([main_tid, loop_tid](const auto& prop) {
                    EXPECT_TRUE(prop.isPowered())
                        << "Technology " << prop.getName()
                        << " was not powered ON";
                    const auto callback_tid = std::this_thread::get_id();
                    EXPECT_NE(callback_tid, main_tid);
                    EXPECT_NE(callback_tid, loop_tid);
                    std::cout << "onPropertyChanged:\n";
                    std::cout << prop;
                });
                const auto prop = tech->properties();
                const auto name = prop.getName();
                if (!prop.isPowered()) {
                    std::cout << "Powering on technology: " << name << '\n';
                    tech->setPowered(true, [&called, name, main_tid,
                                            loop_tid](auto success) {
                        const auto callback_tid = std::this_thread::get_id();
                        EXPECT_NE(callback_tid, main_tid);
                        EXPECT_NE(callback_tid, loop_tid);
                        std::cout << "setPowered callback for " << name << ": "
                                  << (success ? "Success" : "Failure") << '\n';
                        EXPECT_TRUE(success)
                            << "Set power for " << name << " did not succeed";
                        ;
                        called = true;
                    });
                }
            }
        };
        if (manager->technologies().empty()) {
            manager->onTechnologiesChanged(do_on_techs);
        } else {
            do_on_techs(manager->technologies(), false);
        }
    }
    ASSERT_TRUE(called) << "setPowered callback was never called";
}

TEST(Connman, ScanWifiTechnology) {
    bool called = false;
    {
        const ThreadBundle thread_bundle;
        Connman connman;
        const auto manager = connman.manager();

        auto do_on_techs = [&called, main_tid = thread_bundle.main_tid,
                            loop_tid = thread_bundle.loop_tid](
                               const auto& technologies,
                               const bool check_thread_id = true) {
            if (check_thread_id) {
                const auto callback_tid = std::this_thread::get_id();
                EXPECT_NE(callback_tid, main_tid);
                EXPECT_NE(callback_tid, loop_tid);
            }
            ASSERT_FALSE(technologies.empty()) << "No technologies returned";

            for (const auto& tech : technologies) {
                const auto props = tech->properties();
                const auto name = props.getName();
                if (props.getType() == Type::Wifi) {
                    std::cout << "Scanning technology with name: " << name
                              << "\n";
                    tech->scan([&called, name, main_tid,
                                loop_tid](bool success) {
                        const auto callback_tid = std::this_thread::get_id();
                        EXPECT_NE(callback_tid, main_tid);
                        EXPECT_NE(callback_tid, loop_tid);
                        called = true;
                        EXPECT_TRUE(success);
                        std::cout << "Technology " << name
                                  << " scanned successfully.\n";
                    });
                }
            }
        };

        if (manager->technologies().empty()) {
            manager->onTechnologiesChanged(do_on_techs);
        } else {
            do_on_techs(manager->technologies(), false);
        }
    }
    ASSERT_TRUE(called) << "TechnologiesChanged callback was never called";
}

TEST(Connman, SetTetheringOn) {
    bool called = false;
    {
        const ThreadBundle thread_bundle;
        Connman connman;
        const auto manager = connman.manager();

        auto do_on_techs = [&called, main_tid = thread_bundle.main_tid,
                            loop_tid = thread_bundle.loop_tid](
                               const auto& technologies,
                               const bool check_thread_id = true) {
            if (check_thread_id) {
                const auto callback_tid = std::this_thread::get_id();
                EXPECT_NE(callback_tid, main_tid);
                EXPECT_NE(callback_tid, loop_tid);
            }
            ASSERT_FALSE(technologies.empty()) << "No technologies returned";

            for (const auto& tech : technologies) {
                const auto props = tech->properties();
                const auto name = props.getName();

                if (props.getType() == Type::Wifi) {  // test only wifi
                    std::cout << "Setting tethering properties for " << name
                              << "\n";
                    tech->setTetheringIdentifier(
                        "AmarulaTestSSID",
                        [name, main_tid, loop_tid](bool success) {
                            const auto callback_tid =
                                std::this_thread::get_id();
                            EXPECT_NE(callback_tid, main_tid);
                            EXPECT_NE(callback_tid, loop_tid);
                            EXPECT_TRUE(success)
                                << "Failed to set tethering identifier for "
                                << name;
                        });
                    tech->setTetheringPassphrase(
                        "AmarulaTestPassphrase",
                        [name, main_tid, loop_tid](bool success) {
                            const auto callback_tid =
                                std::this_thread::get_id();
                            EXPECT_NE(callback_tid, main_tid);
                            EXPECT_NE(callback_tid, loop_tid);
                            EXPECT_TRUE(success)
                                << "Failed to set tethering passphrase for "
                                << name;
                        });

                    tech->setTetheringFreq(
                        WIFI_FREQ_2412_MHZ,
                        [name, main_tid, loop_tid](bool success) {
                            const auto callback_tid =
                                std::this_thread::get_id();
                            EXPECT_NE(callback_tid, main_tid);
                            EXPECT_NE(callback_tid, loop_tid);
                            EXPECT_TRUE(success)
                                << "Failed to set tethering frequency for "
                                << name;
                        });
                    tech->setTethering(true, [&called, name, main_tid,
                                              loop_tid](bool success) {
                        const auto callback_tid = std::this_thread::get_id();
                        EXPECT_NE(callback_tid, main_tid);
                        EXPECT_NE(callback_tid, loop_tid);
                        EXPECT_TRUE(success)
                            << "Failed to set tethering for " << name;
                        called = true;
                    });
                }
            }
        };
        if (manager->technologies().empty()) {
            manager->onTechnologiesChanged(do_on_techs);
        } else {
            do_on_techs(manager->technologies(), false);
        }
    }
    ASSERT_TRUE(called) << "setTethering callback was never called";
}

TEST(Connman, SetTetheringOff) {
    bool called = false;
    {
        const ThreadBundle thread_bundle;
        Connman connman;
        const auto manager = connman.manager();

        auto do_on_techs = [&called, main_tid = thread_bundle.main_tid,
                            loop_tid = thread_bundle.loop_tid](
                               const auto& technologies,
                               const bool check_thread_id = true) {
            if (check_thread_id) {
                const auto callback_tid = std::this_thread::get_id();
                EXPECT_NE(callback_tid, main_tid);
                EXPECT_NE(callback_tid, loop_tid);
            }
            ASSERT_FALSE(technologies.empty()) << "No technologies returned";

            for (const auto& tech : technologies) {
                const auto props = tech->properties();
                const auto name = props.getName();

                if (props.getType() == Type::Wifi) {  // test only wifi
                    std::cout << "Disable tethering for " << name << "\n";
                    tech->setTethering(false, [&called, name, main_tid,
                                               loop_tid](bool success) {
                        const auto callback_tid = std::this_thread::get_id();
                        EXPECT_NE(callback_tid, main_tid);
                        EXPECT_NE(callback_tid, loop_tid);
                        EXPECT_TRUE(success)
                            << "Failed to unset tethering for " << name;
                        called = true;
                    });
                }
            }
        };
        if (manager->technologies().empty()) {
            manager->onTechnologiesChanged(do_on_techs);
        } else {
            do_on_techs(manager->technologies(), false);
        }
    }
    ASSERT_TRUE(called) << "setTethering callback was never called";
}

TEST(Connman, PowerOffAllTechnologies) {
    bool called = false;
    {
        const ThreadBundle thread_bundle;
        Connman connman;
        const auto manager = connman.manager();

        auto do_on_techs = [&called, main_tid = thread_bundle.main_tid,
                            loop_tid = thread_bundle.loop_tid](
                               const auto& technologies,
                               const bool check_thread_id = true) {
            if (check_thread_id) {
                const auto callback_tid = std::this_thread::get_id();
                EXPECT_NE(callback_tid, main_tid);
                EXPECT_NE(callback_tid, loop_tid);
            }
            ASSERT_FALSE(technologies.empty()) << "No technologies returned";
            // Power off all technologies
            for (const auto& tech : technologies) {
                tech->onPropertyChanged([&](const auto& prop) {
                    EXPECT_FALSE(prop.isPowered())
                        << "Technology " << prop.getName()
                        << " was not powered OFF";
                    std::cout << "onPropertyChanged:\n";
                    std::cout << prop;
                });
                const auto prop = tech->properties();
                const auto name = prop.getName();
                if (prop.isPowered()) {
                    std::cout << "Powering off technology: " << prop.getName()
                              << '\n';
                    tech->setPowered(false, [&, name](auto success) {
                        std::cout << "setPowered callback for " << name << ": "
                                  << (success ? "Success" : "Failure") << '\n';
                        EXPECT_TRUE(success)
                            << "Set power for " << name << " did not succeed";
                        called = true;
                    });
                }
            }
        };

        if (manager->technologies().empty()) {
            manager->onTechnologiesChanged(do_on_techs);
        } else {
            do_on_techs(manager->technologies(), false);
        }
    }
    ASSERT_TRUE(called) << "setPowered callback was never called";
}
