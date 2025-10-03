#include <gtest/gtest.h>

#include <amarula/log.hpp>
#include <iostream>
#include <sstream>
#include <string>

TEST(Logger, EnableThenDisable) {
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

    Amarula::Log::enable(true);
    EXPECT_TRUE(Amarula::Log::isEnabled());

    LCM_LOG("first message");
    LCM_LOG("second " << "message");
    std::cout.flush();

    const std::string out1 = buffer.str();
    EXPECT_NE(out1.find("first message"), std::string::npos);
    EXPECT_NE(out1.find("second message"), std::string::npos);

    Amarula::Log::enable(false);
    buffer.str("");
    buffer.clear();

    LCM_LOG("should not appear");
    std::cout.flush();

    EXPECT_TRUE(buffer.str().empty());
    std::cout.rdbuf(old);
}
