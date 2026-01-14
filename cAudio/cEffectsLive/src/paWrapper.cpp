#include <iostream>

#include "paWrapper.h"


void showDevices() {
    int numDevices = Pa_GetDeviceCount();
    const PaDeviceInfo* deviceInfo;

    std::cout << "Available audio devices:\n";
    for (int i = 0; i < numDevices; ++i) {
        deviceInfo = Pa_GetDeviceInfo(i);
        std::cout << "Device " << i << ": " << deviceInfo->name << " " << deviceInfo->defaultSampleRate << "\n";
    }
}

int initPortAudio(PaStreamCallback audioCallback, PaStream* stream, paParameters* params) {
    PaStreamParameters inputParameters{};
    PaStreamParameters outputParameters{};
    int n = 0;

    PaError err = Pa_Initialize();
    if (err != paNoError) goto error;
    showDevices();
    std::cout << "Selecting device 1 for both input and output.\n";
    std::cin >> n;

    inputParameters.device = n;
    inputParameters.channelCount = 1;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency =
        Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;

    outputParameters.device = n;
    outputParameters.channelCount = 2;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency =
        Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;

    err = Pa_IsFormatSupported( &inputParameters, &outputParameters, SAMPLE_RATE );
    if( err == paFormatIsSupported ) printf( "Hooray!\n");
    else {
        std::cerr << "The specified audio format is not supported.\n";
        goto error;
    }

    err = Pa_OpenStream(
        &stream,
        &inputParameters,
        &outputParameters,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paClipOff,
        audioCallback,
        params
    );

    if (err != paNoError) goto error;

    err = Pa_StartStream(stream);
    if (err != paNoError) goto error;

    std::cout << "PortAudio stream started.\n";

    return 0;

    error:
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        stopPortAudio(stream);
        return -1;
}

int stopPortAudio(PaStream* stream) {
    PaError err = Pa_StopStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error stopping stream: " << Pa_GetErrorText(err) << std::endl;
        return -1;
    }
    err = Pa_CloseStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error closing stream: " << Pa_GetErrorText(err) << std::endl;
        return -1;
    }
    err = Pa_Terminate();
    if (err != paNoError) {
        std::cerr << "PortAudio error terminating: " << Pa_GetErrorText(err) << std::endl;
        return -1;
    }
    return 0;
}