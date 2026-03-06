# 1. Standard CMake Setup
cmake_minimum_required(VERSION 3.20)
project(MyVSTHost VERSION 0.0.1)

# 2. Add the JUCE framework to your project
add_subdirectory(JUCE)

# 3. Create the App
# We use a GUI app so the VSTs have the required OS message loop.
juce_add_gui_app(MyVSTHost
    PRODUCT_NAME "My VST Host"
    VERSION 0.0.1)

# 4. Add your C++ source files
target_sources(MyVSTHost
    PRIVATE
        Source/main.cpp)

# 5. Link the JUCE Modules
# This is the most important step! We tell CMake to include the specific 
# JUCE libraries needed to talk to soundcards and host VST plugins.
target_link_libraries(MyVSTHost
    PRIVATE
        juce::juce_core
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors # <- REQUIRED for AudioPluginFormatManager (VST Loading)
        juce::juce_audio_utils
)

# 6. Enable VST3 Hosting
# This specific command tells JUCE to compile the VST3 hosting code into your app.
target_compile_definitions(MyVSTHost
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_PLUGINHOST_VST3=1 # <- REQUIRED to load .vst3 files
)