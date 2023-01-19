COMPONENT_NAME=RainGauge

SRC_FILES = \
  $(PROJECT_SRC_DIR)/RainGauge.cpp \

TEST_SRC_FILES = \
  $(UNITTEST_SRC_DIR)/TestRainGaugeReal.c #\ 
  #$(UNITTEST_SRC_DIR)/TestRainGauge.c
  
include $(CPPUTEST_MAKFILE_INFRA)
