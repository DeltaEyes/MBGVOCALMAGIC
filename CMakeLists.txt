cmake_minimum_required(VERSION 3.22)
project(MBGVOCALMAGIC VERSION 1.0.0)

# JUCEフレームワークを読み込む
add_subdirectory(JUCE)

# プラグインの基本情報を設定
juce_add_plugin(MBGVOCALMAGIC
    COMPANY_NAME "MottainaiBananaGang"
    PLUGIN_MANUFACTURER_CODE "MBG_"
    PLUGIN_CODE "VOCM"
    FORMATS VST3 Standalone
    PRODUCT_NAME "MBG VOCAL MAGIC"
    COPY_PLUGIN_AFTER_BUILD TRUE
)
juce_generate_juce_header(MBGVOCALMAGIC)

# コンパイルするC++ファイルを指定
target_sources(MBGVOCALMAGIC
    PRIVATE
        src/PluginProcessor.cpp
        src/PluginEditor.cpp
)

# おすすめの安全設定
target_compile_definitions(MBGVOCALMAGIC
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

# ★ 背景画像をバイナリデータとしてプラグイン内に埋め込む設定
juce_add_binary_data(MBGVOCALMAGIC_Resources
    SOURCES
        assets/bg_image.jpg
)

# 必要なJUCEの機能と、背景画像のリソースをリンクする
target_link_libraries(MBGVOCALMAGIC
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
        juce::juce_dsp
        MBGVOCALMAGIC_Resources
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)