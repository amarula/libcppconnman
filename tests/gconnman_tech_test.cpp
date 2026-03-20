#include <gtest/gtest.h>

#include <amarula/dbus/connman/gconnman.hpp>
#include <amarula/dbus/connman/gtechnology.hpp>

#include "qt_thread_bundle.hpp"

using Amarula::DBus::G::Connman::Connman;
using Type = Amarula::DBus::G::Connman::TechProperties::Type;

TEST(Connman, getTechs) {
    bool called = false;
    {
        const QtThreadBundle qt_thread_bundle;
        Connman connman;
        const auto manager = connman.manager();

        manager->onTechnologiesChanged(
            [&called, main_tid = qt_thread_bundle.main_tid,
             loop_tid = qt_thread_bundle.loop_tid](const auto& technologies) {
                called = true;
                const auto callback_tid = std::this_thread::get_id();
                EXPECT_NE(callback_tid, main_tid);
                EXPECT_NE(callback_tid, loop_tid);
                ASSERT_FALSE(technologies.empty());
                for (const auto& tech : technologies) {
                    const auto props = tech->properties();
                    EXPECT_FALSE(props.getName().empty());
                    if (props.isConnected()) {
                        EXPECT_TRUE(props.isPowered())
                            << "Technology is connected but not powered";
                    }
                    props.print();
                }
            });
    }
    ASSERT_TRUE(called) << "TechnologiesChanged callback was never called";
}

TEST(Connman, PowerOnAllTechnologies) {
    bool called = false;

    {
        const QtThreadBundle qt_thread_bundle;
        Connman connman;
        const auto manager = connman.manager();

        manager->onTechnologiesChanged([&called,
                                        main_tid = qt_thread_bundle.main_tid,
                                        loop_tid = qt_thread_bundle.loop_tid](
                                           const auto& technologies) {
            const auto callback_tid = std::this_thread::get_id();
            EXPECT_NE(callback_tid, main_tid);
            EXPECT_NE(callback_tid, loop_tid);
            ASSERT_FALSE(technologies.empty());
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
                    prop.print();
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
        });
    }
    ASSERT_TRUE(called) << "setPowered callback was never called";
}

TEST(Connman, ScanWifiTechnology) {
    bool called = false;
    {
        const QtThreadBundle qt_thread_bundle;
        Connman connman;
        const auto manager = connman.manager();

        manager->onTechnologiesChanged(
            [&called, main_tid = qt_thread_bundle.main_tid,
             loop_tid = qt_thread_bundle.loop_tid](const auto& technologies) {
                ASSERT_FALSE(technologies.empty())
                    << "No technologies returned";

                for (const auto& tech : technologies) {
                    const auto props = tech->properties();
                    const auto name = props.getName();
                    if (props.getType() == Type::Wifi) {
                        std::cout << "Scanning technology with name: " << name
                                  << "\n";
                        tech->scan(
                            [&called, name, main_tid, loop_tid](bool success) {
                                const auto callback_tid =
                                    std::this_thread::get_id();
                                EXPECT_NE(callback_tid, main_tid);
                                EXPECT_NE(callback_tid, loop_tid);
                                called = true;
                                EXPECT_TRUE(success);
                                std::cout << "Technology " << name
                                          << " scanned successfully.\n";
                            });
                    }
                }
            });
    }
    ASSERT_TRUE(called) << "TechnologiesChanged callback was never called";
}

TEST(Connman, PowerOffAllTechnologies) {
    bool called = false;
    {
        Connman connman;
        const auto manager = connman.manager();

        manager->onTechnologiesChanged([&](const auto& technologies) {
            ASSERT_FALSE(technologies.empty()) << "No technologies returned";

            // Power off all technologies
            for (const auto& tech : technologies) {
                tech->onPropertyChanged([&](const auto& prop) {
                    EXPECT_FALSE(prop.isPowered())
                        << "Technology " << prop.getName()
                        << " was not powered OFF";
                    std::cout << "onPropertyChanged:\n";
                    prop.print();
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
        });
    }
    ASSERT_TRUE(called) << "setPowered callback was never called";
}