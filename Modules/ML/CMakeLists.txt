hell_add_library(ML SOURCES src/MLP.cc DEPENDS HellCore)
hell_add_plugin(ML SOURCES plugins/MLPScoreProducer.cc DEPENDS HellML)
