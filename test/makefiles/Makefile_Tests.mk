COMPONENT_NAME=RainGauge

SRC_FILES = \
  $(PROJECT_SRC_DIR)/RainGauge.cpp \
  $(PROJECT_SRC_DIR)/Lightning.cpp

MOCKS_SRC_DIRS = \
  $(UNITTEST_ROOT)/mocks

TEST_SRC_FILES = \
  $(UNITTEST_SRC_DIR)/TestRainGauge.cpp \
  $(UNITTEST_SRC_DIR)/TestLightning.cpp
  #$(UNITTEST_SRC_DIR)/TestRainGaugeReal.cpp
  
include $(CPPUTEST_MAKFILE_INFRA)
