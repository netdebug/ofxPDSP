

#include "Sampler.h"

//TODO: start position with value sampled on trigger

pdsp::Sampler::Sampler(){

        addInput("trig", input_trig);
        addInput("pitch", input_pitch_mod);
        addInput("select", input_select);
        addInput("start", input_start);
        addInput("direction", input_direction);
        addOutput("signal", output);
        updateOutputNodes();  
        
        incBase = 1.0f / 11050.0f;
        sample = nullptr;
        readIndex = 0.0f;
        direction = 1.0f;
        
        positionMeter.store(0.0f);
        positionDivider = 0.001f;

        input_pitch_mod.setDefaultValue(0.0f);
        input_select.setDefaultValue(0.0f);
        input_start.setDefaultValue(0.0f);
        input_direction.setDefaultValue(1.0f);
        //input_trigger_to_start.setDefaultValue(0.0f);


        interpolatorShell.changeInterpolator(Linear);

        if(dynamicConstruction){
                prepareToPlay(globalBufferSize, globalSampleRate);
        }
}

float pdsp::Sampler::meter_position() const{
    return positionMeter.load();
}

pdsp::Patchable& pdsp::Sampler::in_trig(){
    return in("trig");
}

pdsp::Patchable& pdsp::Sampler::in_pitch(){
    return in("pitch");
}

pdsp::Patchable& pdsp::Sampler::in_select(){
    return in("select");
}

pdsp::Patchable& pdsp::Sampler::in_start(){
    return in("start");
}

pdsp::Patchable& pdsp::Sampler::in_direction(){
    return in("direction");
}

pdsp::Patchable& pdsp::Sampler::out_signal(){
    return out("signal");
}


void pdsp::Sampler::addSample(SampleBuffer* newSample){
        samples.push_back(newSample);
        if(sample==nullptr){
                sample = samples[samples.size()-1];
        }  
}

bool pdsp::Sampler::setSample(SampleBuffer* newSample, int index){
        if(index<samples.size() && index>=0){
                samples[index] = newSample;
                return true;
        }if(index == samples.size() ){
                addSample(newSample);
                return true;
        }else{
                return false;
        }
}

void pdsp::Sampler::prepareUnit( int expectedBufferSize, double sampleRate ) {
        readIndex = 0.0f;
        incBase = 1.0f / sampleRate;
        interpolatorShell.interpolator->reset();
}



void pdsp::Sampler::process(int bufferSize) noexcept {
        
        if(sample!=nullptr && sample->loaded()){
        
                int selectState;
                const float* selectBuffer = nullptr;

                int startState;
                const float* startBuffer = nullptr;

                int triggerState;
                const float* triggerBuffer = processInput(input_trig, triggerState);

                int pitchModState;
                const float* pitchModBuffer = processInput(input_pitch_mod, pitchModState  );

            
                if(pitchModState==Changed){
                        process_once<true>(pitchModBuffer);
                }


                int switcher = pitchModState + triggerState*4;
                switch ( switcher & processAudioBitMask ) {
                case audioFFFF : //false, false
                        process_audio<false, false>(pitchModBuffer, triggerBuffer, selectBuffer, selectState, startBuffer, startState, bufferSize);
                        //Logger::writeToLog("run");
                        break;
                case audioTFFF : //true, false
                        process_audio<true, false>(pitchModBuffer, triggerBuffer, selectBuffer, selectState, startBuffer, startState, bufferSize);
                        break;
                case audioFTFF : //false, true
                        process_audio<false, true>(pitchModBuffer, triggerBuffer, selectBuffer, selectState, startBuffer, startState, bufferSize);
                        break;
                case audioTTFF : //true, true
                        process_audio<true, true>(pitchModBuffer, triggerBuffer, selectBuffer, selectState, startBuffer, startState, bufferSize);
                        break;
                default:
                        break;
                }
        }else{
                setOutputToZero(output);
        }

}



template<bool pitchModChange>
void pdsp::Sampler::process_once( const float* pitchModBuffer)noexcept{
        if(pitchModChange){
                vect_calculateIncrement(&inc, pitchModBuffer, incBase * sample->fileSampleRate, 1);
                //in this way is always correct even with oversample
        }
}

template<bool pitchModAR, bool triggerAR>
void pdsp::Sampler::process_audio( const float* pitchModBuffer, const float* triggerBuffer, const float* &selectBuffer, int &selectState, const float* &startBuffer, int& startState, int bufferSize)noexcept{

        float* outputBuffer = getOutputBufferToFill(output);

        if(pitchModAR){
                //
                vect_calculateIncrement(outputBuffer, pitchModBuffer, incBase * sample->fileSampleRate, bufferSize);
                //in this way is always correct even with oversample
        }

        for(int n=0; n<bufferSize; ++n){

                if(triggerAR){
                        if(checkTrigger(triggerBuffer[n])){
                                selectSample(selectBuffer, selectState,  startBuffer, startState, n, bufferSize);
                        }
                }

                if(pitchModAR){
                        inc = outputBuffer[n];  //we have the calculated pitchs inside outputbuffer
                }

                int readIndex_int = static_cast<int>(readIndex);
                if(readIndex_int>=0 && readIndex_int < sample->length){
                        outputBuffer[n] = interpolatorShell.interpolator->interpolate(sample->buffer[sample->mono], readIndex, sample->length);
                }else{
                        outputBuffer[n] = 0.0f;
                }
                
                readIndex += inc*direction;
                 
        }
        
        positionMeter.store(readIndex*positionDivider);
}


void pdsp::Sampler::selectSample(const float* &selectBuffer, int &selectState, const float* &startBuffer, int &startState, int n, int bufferSize)noexcept{

        if(selectBuffer==nullptr){
                selectBuffer = processInput(input_select, selectState);
        }

        if(startBuffer==nullptr){
                startBuffer = processInput(input_start, startState);
        }

        //SELECT SAMPLE
        int sampleIndex;
        if (selectState==AudioRate){
                sampleIndex = static_cast<int>(selectBuffer[n]);
        }else{
                sampleIndex = static_cast<int>(selectBuffer[0]);
        }
        
        if(sampleIndex<0){
                sampleIndex=0;
        }else if(sampleIndex>=samples.size()){
                sampleIndex = samples.size() - 1 ;
        }
        
        //SET START POSITION
        sample = samples[sampleIndex];
        float start;
        if(startState==AudioRate){
                start = startBuffer[n];
        }else{
                start = startBuffer[0];
        }
        float lenFloat = static_cast<float>(sample->length);
        readIndex = start * lenFloat;
        positionDivider = 1.0f / lenFloat;

        //SET FORWARD OR REVERSE
        float directionControl = processAndGetSingleValue( input_direction, n ); 
        if(directionControl<0.0f){
            direction = -1.0f;
        }else{
            direction = 1.0f;
        }

}

void pdsp::Sampler::setInterpolatorType(Interpolator_t type){   
        interpolatorShell.changeInterpolator(type);
}
