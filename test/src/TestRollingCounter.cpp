// TestRollingCounter.cpp
// Unit tests for RollingCounter base class
//
#include <CppUTest/TestHarness.h>
#include "RollingCounter.h"
#include <ctime>
#include <cstring>

// Dummy subclass to access protected members and implement pure virtuals
class TestableRollingCounter : public RollingCounter {
public:
    using RollingCounter::calculateIndex;
    using RollingCounter::markMissedEntries;
    using RollingCounter::sumHistory;
    using RollingCounter::getLastUpdate;
    using RollingCounter::getUpdateRate;
    TestableRollingCounter(float q = DEFAULT_QUALITY_THRESHOLD) : RollingCounter(q) {}
    void hist_init(int16_t value = -1) override {}
    float getQualityThreshold() const { return qualityThreshold; }
    struct PublicHistory {
        int16_t* hist;
        size_t size;
        uint8_t updateRate;
    };
    static PublicHistory makeHistory(int16_t* h, size_t s, uint8_t r) {
        PublicHistory ph{h, s, r};
        return ph;
    }
    // Adapter to call sumHistory with PublicHistory
    float sumHistoryPublic(const PublicHistory& ph, bool *valid = nullptr, int *nbins = nullptr, float *quality = nullptr, float scale = 1.0) {
        History h{ph.hist, ph.size, ph.updateRate};
        return sumHistory(h, valid, nbins, quality, scale);
    }
};

TEST_GROUP(RollingCounterBasics) {
    TestableRollingCounter rc;
    void setup() {}
    void teardown() {}
};

TEST(RollingCounterBasics, DefaultConstructor) {
    TestableRollingCounter rc;
    DOUBLES_EQUAL(DEFAULT_QUALITY_THRESHOLD, rc.getQualityThreshold(), 0.0001);
    CHECK_EQUAL(0, rc.getLastUpdate());
    CHECK_EQUAL(ROLLING_COUNTER_UPD_RATE, rc.getUpdateRate());
}

TEST(RollingCounterBasics, CustomQualityThreshold) {
    TestableRollingCounter rc(0.5f);
    DOUBLES_EQUAL(0.5f, rc.getQualityThreshold(), 0.0001);
}

TEST(RollingCounterBasics, CalculateIndexHourly) {
    TestableRollingCounter rc;
    struct tm t = {0};
    t.tm_hour = 5;
    t.tm_min = 0;
    int idx = rc.calculateIndex(t, 60);
    CHECK_EQUAL(5, idx);
}

TEST(RollingCounterBasics, CalculateIndexSubHourly) {
    TestableRollingCounter rc;
    struct tm t = {0};
    t.tm_min = 18;
    int idx = rc.calculateIndex(t, 6);
    CHECK_EQUAL(3, idx); // 18/6 = 3
}

TEST(RollingCounterBasics, MarkMissedEntriesNoCrashOnZeroRate) {
    TestableRollingCounter rc;
    int16_t hist[4] = {1,2,3,4};
    rc.markMissedEntries(hist, 4, 0, 100, 0); // Should not crash
    // No assertion, just check for no crash
}

TEST(RollingCounterBasics, SumHistoryAllValid) {
    TestableRollingCounter rc;
    int16_t hist[4] = {1,2,3,4};
    auto h = TestableRollingCounter::makeHistory(hist, 4, 1);
    bool valid = false;
    int nbins = 0;
    float quality = 0.0f;
    float sum = rc.sumHistoryPublic(h, &valid, &nbins, &quality, 1.0f);
    CHECK_EQUAL(10, (int)sum);
    CHECK_TRUE(valid);
    CHECK_EQUAL(4, nbins);
    DOUBLES_EQUAL(1.0, quality, 0.0001);
}

TEST(RollingCounterBasics, SumHistorySomeInvalid) {
    TestableRollingCounter rc;
    int16_t hist[4] = {1,-1,3,-1};
    auto h = TestableRollingCounter::makeHistory(hist, 4, 1);
    bool valid = false;
    int nbins = 0;
    float quality = 0.0f;
    float sum = rc.sumHistoryPublic(h, &valid, &nbins, &quality, 1.0f);
    CHECK_EQUAL(4, (int)sum);
    CHECK_FALSE(valid); // Only 2/4 valid, below default threshold
    CHECK_EQUAL(2, nbins);
    DOUBLES_EQUAL(0.5, quality, 0.0001);
}

/*
 * Test markMissedEntries with out-of-bounds index
 *
 * lastUpdate at 8:00 (minute=0), timestamp at 8:30 (minute=30), rate=6, size=4.
 * The loop marks indices for 8:06 (idx=1), 8:12 (idx=2), 8:18 (idx=3), 8:24 (idx=4).
 * Index 4 is >= size=4, so the out-of-bounds warning path is triggered and skipped.
 */
TEST(RollingCounterBasics, MarkMissedEntriesOutOfBounds) {
    TestableRollingCounter rc;
    int16_t hist[4] = {1, 1, 1, 1};
    struct tm tm_t;
    time_t lastUpdate, timestamp;
    memset(&tm_t, 0, sizeof(tm_t));
    strptime("2023-07-22 8:00", "%Y-%m-%d %H:%M", &tm_t);
    tm_t.tm_isdst = -1;
    lastUpdate = mktime(&tm_t);
    strptime("2023-07-22 8:30", "%Y-%m-%d %H:%M", &tm_t);
    tm_t.tm_isdst = -1;
    timestamp = mktime(&tm_t);
    rc.markMissedEntries(hist, 4, lastUpdate, timestamp, 6);
    CHECK_EQUAL(1,  hist[0]);  // index 0 not in loop range
    CHECK_EQUAL(-1, hist[1]);  // marked at 8:06 (idx=1)
    CHECK_EQUAL(-1, hist[2]);  // marked at 8:12 (idx=2)
    CHECK_EQUAL(-1, hist[3]);  // marked at 8:18 (idx=3)
    // index 4 at 8:24 is out-of-bounds (size=4), skipped without crash
}

/*
 * Test sumHistory with updateRate == 0 (invalid)
 */
TEST(RollingCounterBasics, SumHistoryUpdateRateZero) {
    TestableRollingCounter rc;
    int16_t hist[4] = {1, 2, 3, 4};
    auto h = TestableRollingCounter::makeHistory(hist, 4, 0);
    bool valid = true;
    int nbins = 99;
    float quality = 1.0f;
    float sum = rc.sumHistoryPublic(h, &valid, &nbins, &quality, 1.0f);
    DOUBLES_EQUAL(0.0f, sum, 0.0001);
    CHECK_FALSE(valid);
    CHECK_EQUAL(0, nbins);
    DOUBLES_EQUAL(0.0f, quality, 0.0001);
}

/*
 * Test sumHistory with updateRate > 60 (invalid, falls back to effectiveBins=1)
 */
TEST(RollingCounterBasics, SumHistoryUpdateRateGT60) {
    TestableRollingCounter rc;
    int16_t hist[4] = {1, 2, 3, 4};
    auto h = TestableRollingCounter::makeHistory(hist, 4, 70);
    bool valid = false;
    int nbins = 0;
    float quality = 0.0f;
    float sum = rc.sumHistoryPublic(h, &valid, &nbins, &quality, 1.0f);
    CHECK_EQUAL(1, nbins);     // effectiveBins=1, binsToCheck=1, hist[0]=1 is valid
    DOUBLES_EQUAL(1.0f, sum, 0.0001);
}

/*
 * Test sumHistory when effectiveBins == 0 (size=0 buffer)
 * quality must be set to 0.0 in this case
 */
TEST(RollingCounterBasics, SumHistoryEffectiveBinsZero) {
    TestableRollingCounter rc;
    auto h = TestableRollingCounter::makeHistory(nullptr, 0, 6);
    bool valid = false;
    int nbins = 0;
    float quality = 1.0f;
    float sum = rc.sumHistoryPublic(h, &valid, &nbins, &quality, 1.0f);
    DOUBLES_EQUAL(0.0f, sum, 0.0001);
    CHECK_EQUAL(0, nbins);
    DOUBLES_EQUAL(0.0f, quality, 0.0001);
}
