// TestRollingCounter.cpp
// Unit tests for RollingCounter base class
//
#include <CppUTest/TestHarness.h>
#include "RollingCounter.h"
#include <ctime>

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
