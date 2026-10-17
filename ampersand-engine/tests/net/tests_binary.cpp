#include <gtest/gtest.h>

#include <ampersand/net/BinaryReader.hpp>
#include <ampersand/net/BinaryWriter.hpp>
#include <ampersand/net/UdpSocket.hpp>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <vector>

using ampersand::net::BinaryReader;
using ampersand::net::BinaryWriter;
using ampersand::net::NetError;

namespace {

enum class Color : std::uint8_t { Red = 0, Green = 1, Blue = 2 };

std::vector<std::byte> bytes(std::initializer_list<unsigned> values) {
    std::vector<std::byte> result;
    for (const unsigned value : values) {
        result.push_back(static_cast<std::byte>(value));
    }
    return result;
}

}  // namespace

TEST(BinaryWriter, WritesIntegersBigEndian) {
    std::vector<std::byte> buffer;
    BinaryWriter writer(buffer);
    writer.write(std::uint8_t{0x01});
    writer.write(std::uint16_t{0x0203});
    writer.write(std::uint32_t{0x04050607});
    EXPECT_EQ(buffer, bytes({0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07}));
    EXPECT_EQ(writer.size(), 7U);
}

TEST(BinaryWriter, AppendsToExistingBytes) {
    std::vector<std::byte> buffer = bytes({0xAA});
    BinaryWriter writer(buffer);
    writer.write(std::uint8_t{0xBB});
    EXPECT_EQ(buffer, bytes({0xAA, 0xBB}));
}

TEST(BinaryWriter, WritesStringWithLengthPrefix) {
    std::vector<std::byte> buffer;
    BinaryWriter writer(buffer);
    writer.writeString("RT");
    EXPECT_EQ(buffer, bytes({0x02, 'R', 'T'}));
}

TEST(BinaryWriter, RefusesStringLongerThan255Bytes) {
    std::vector<std::byte> buffer;
    BinaryWriter writer(buffer);
    EXPECT_THROW(writer.writeString(std::string(256, 'a')), NetError);
    EXPECT_NO_THROW(writer.writeString(std::string(255, 'a')));
}

TEST(BinaryReader, RoundTripsEveryType) {
    std::vector<std::byte> buffer;
    BinaryWriter writer(buffer);
    writer.write(std::uint8_t{7});
    writer.write(std::uint16_t{65000});
    writer.write(std::uint32_t{0xDEADBEEF});
    writer.write(std::uint64_t{0x0102030405060708});
    writer.writeFloat(-12.5F);
    writer.writeEnum(Color::Blue);
    writer.writeString("hello");

    BinaryReader reader(buffer);
    std::uint8_t byteValue = 0;
    std::uint16_t shortValue = 0;
    std::uint32_t intValue = 0;
    std::uint64_t longValue = 0;
    float floatValue = 0.0F;
    Color color = Color::Red;
    std::string text;
    ASSERT_TRUE(reader.read(byteValue));
    ASSERT_TRUE(reader.read(shortValue));
    ASSERT_TRUE(reader.read(intValue));
    ASSERT_TRUE(reader.read(longValue));
    ASSERT_TRUE(reader.readFloat(floatValue));
    ASSERT_TRUE(reader.readEnum(color, Color::Blue));
    ASSERT_TRUE(reader.readString(text, 16));
    EXPECT_EQ(byteValue, 7);
    EXPECT_EQ(shortValue, 65000);
    EXPECT_EQ(intValue, 0xDEADBEEF);
    EXPECT_EQ(longValue, 0x0102030405060708U);
    EXPECT_EQ(floatValue, -12.5F);
    EXPECT_EQ(color, Color::Blue);
    EXPECT_EQ(text, "hello");
    EXPECT_TRUE(reader.atEnd());
}

TEST(BinaryReader, EmptyBufferFailsEveryRead) {
    BinaryReader reader(std::span<const std::byte>{});
    std::uint8_t value = 42;
    std::string text = "untouched";
    float number = 1.0F;
    EXPECT_FALSE(reader.read(value));
    EXPECT_FALSE(reader.readString(text, 16));
    EXPECT_FALSE(reader.readFloat(number));
    EXPECT_EQ(value, 42);
    EXPECT_EQ(text, "untouched");
    EXPECT_EQ(number, 1.0F);
    EXPECT_TRUE(reader.atEnd());
}

TEST(BinaryReader, TooShortReadLeavesOutputAndOffsetUntouched) {
    const auto buffer = bytes({0x01, 0x02, 0x03});
    BinaryReader reader(buffer);
    std::uint32_t value = 99;
    EXPECT_FALSE(reader.read(value));
    EXPECT_EQ(value, 99U);
    EXPECT_EQ(reader.remaining(), 3U);

    std::uint16_t shorter = 0;
    EXPECT_TRUE(reader.read(shorter));
    EXPECT_EQ(shorter, 0x0102);
}

TEST(BinaryReader, RejectsEnumAboveLastValue) {
    const auto buffer = bytes({0x03});
    BinaryReader reader(buffer);
    Color color = Color::Green;
    EXPECT_FALSE(reader.readEnum(color, Color::Blue));
    EXPECT_EQ(color, Color::Green);
}

TEST(BinaryReader, RejectsNanAndInfinity) {
    std::vector<std::byte> buffer;
    BinaryWriter writer(buffer);
    writer.writeFloat(std::numeric_limits<float>::quiet_NaN());
    writer.writeFloat(std::numeric_limits<float>::infinity());

    BinaryReader reader(buffer);
    float value = 3.0F;
    EXPECT_FALSE(reader.readFloat(value));
    EXPECT_FALSE(reader.readFloat(value));
    EXPECT_EQ(value, 3.0F);
}

TEST(BinaryReader, RejectsStringLongerThanMaxLength) {
    const auto buffer = bytes({0x03, 'a', 'b', 'c'});
    BinaryReader reader(buffer);
    std::string text;
    EXPECT_FALSE(reader.readString(text, 2));
    EXPECT_TRUE(text.empty());
}

TEST(BinaryReader, RejectsStringLengthPastTheEnd) {
    // Claims 200 bytes but only 2 follow: must not read out of bounds.
    const auto buffer = bytes({200, 'a', 'b'});
    BinaryReader reader(buffer);
    std::string text;
    EXPECT_FALSE(reader.readString(text, 255));
    EXPECT_TRUE(text.empty());
}
