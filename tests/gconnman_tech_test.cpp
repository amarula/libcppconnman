#include <gtest/gtest.h>

#include <amarula/dbus/connman/gconnman.hpp>
#include <amarula/dbus/connman/gtechnology.hpp>

using Amarula::DBus::G::Connman::Connman;
using Type = Amarula::DBus::G::Connman::TechProperties::Type;

TEST(Connman, getTechs) {
    bool called = false;
    {
        Connman connman;
        const auto manager = connman.manager();

        manager->onTechnologiesChanged([&](const auto& technologies) {
            called = true;
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
                    std::cout << "Powering on technology: " << name << '\n';
                    tech->setPowered(true, [&, name](auto success) {
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
        Connman connman;
        const auto manager = connman.manager();

        manager->onTechnologiesChanged([&](const auto& technologies) {
            ASSERT_FALSE(technologies.empty()) << "No technologies returned";

            for (const auto& tech : technologies) {
                const auto props = tech->properties();
                const auto name = props.getName();
                if (props.getType() == Type::Wifi) {
                    std::cout << "Scanning technology with name: " << name
                              << "\n";
                    tech->scan([&called, name](bool success) {
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
