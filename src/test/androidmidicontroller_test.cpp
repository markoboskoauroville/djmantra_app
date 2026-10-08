#include "controllers/midi/androidmidicontroller.h"

#include <gtest/gtest.h>

#include <QSignalSpy>


class AndroidMidiControllerTest : public testing::Test {
  protected:
    QList<QList<int>> parse(const QByteArray& bytes) {
        QList<QList<int>> messages;
        m_parser.feed(
                bytes,
                [&messages](unsigned char status, unsigned char data1, unsigned char data2) {
                    messages.append({status, data1, data2});
                },
                [this](const QByteArray& sysex) { m_sysex.append(sysex); });
        return messages;
    }
    MidiStreamParser m_parser;
    QList<QByteArray> m_sysex;
};

TEST_F(AndroidMidiControllerTest, parsesNotesAndControls) {
    // PLAY pressed, released (note-on with velocity 0); jog tick -1
    const auto messages = parse(QByteArray::fromHex("91077f910700b10a7f"));
    ASSERT_EQ(3, messages.size());
    EXPECT_EQ((QList<int>{0x91, 0x07, 0x7F}), messages[0]);
    EXPECT_EQ((QList<int>{0x91, 0x07, 0x00}), messages[1]);
    EXPECT_EQ((QList<int>{0xB1, 0x0A, 0x7F}), messages[2]);
}

TEST_F(AndroidMidiControllerTest, runningStatus) {
    // A 14-bit fader: MSB and LSB with running status (as BLE MIDI often sends)
    const auto messages = parse(QByteArray::fromHex("b10040201096000000"));
    ASSERT_EQ(3, messages.size());
    EXPECT_EQ((QList<int>{0xB1, 0x00, 0x40}), messages[0]);
    EXPECT_EQ((QList<int>{0xB1, 0x20, 0x10}), messages[1]);
    EXPECT_EQ((QList<int>{0x96, 0x00, 0x00}), messages[2]);
}

TEST_F(AndroidMidiControllerTest, sysexAndRealtimeInBetween) {
    // Identity reply, then a pad
    const auto messages = parse(QByteArray::fromHex(
            "f07e7f060200014e0200210001000000f7") +
            QByteArray::fromHex("96307f"));
    ASSERT_EQ(1, messages.size());
    EXPECT_EQ((QList<int>{0x96, 0x30, 0x7F}), messages[0]);
    ASSERT_EQ(1, m_sysex.size());
    EXPECT_EQ(QByteArray::fromHex("f07e7f060200014e0200210001000000f7"), m_sysex[0]);
}

TEST_F(AndroidMidiControllerTest, messagesSplitAcrossPackets) {
    // BLE packets may cut a message in two
    auto messages = parse(QByteArray::fromHex("91"));
    EXPECT_TRUE(messages.isEmpty());
    messages = parse(QByteArray::fromHex("077f"));
    ASSERT_EQ(1, messages.size());
    EXPECT_EQ((QList<int>{0x91, 0x07, 0x7F}), messages[0]);
}
