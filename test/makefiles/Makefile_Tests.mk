COMPONENT_NAME=RainGauge

SRC_FILES = \
  $(PROJECT_SRC_DIR)/RainGauge.cpp \
  $(PROJECT_SRC_DIR)/Lightning.cpp

TEST_SRC_FILES = \
  $(UNITTEST_SRC_DIR)/TestRainGauge.cpp \
  $(UNITTEST_SRC_DIR)/TestLightning.cpp \
  $(UNITTEST_SRC_DIR)/TestRainGaugeReal.cpp
  #$(UNITTEST_SRC_DIR)/TestRainGauge.c
  
include $(CPPUTEST_MAKFILE_INFRA)
