
#include "LFO.h"


pdsp::LFO::LFO() { patch(); }
pdsp::LFO::LFO(const pdsp::LFO &Other){ patch(); }
pdsp::LFO& pdsp::LFO::operator=(const pdsp::LFO &Other){ return *this; }


void pdsp::LFO::patch(){
    
    addModuleInput( "freq",   speed );
    addModuleInput( "retrig",  phazorFree.in_retrig()  );    
    addModuleInput( "phase_start",  phazorFree.in_phase_start() );  

    addModuleOutput( "triangle", triangle );
    addModuleOutput( "sine", sine );
    addModuleOutput( "saw", sawSlew );
    addModuleOutput( "square", squareSlew );
    addModuleOutput( "sample_and_hold", randomSnHSlew );
    addModuleOutput( "random", randomSlew);

    speed.set(0.5f);

    phazorFree  >> saw      >> sawSlew;
    phazorFree  >> square   >> squareSlew;
    phazorFree  >> sine;
    phazorFree  >> triangle;
    phazorFree.out_trig() >> rnd >> randomSlew;
    phazorFree.out_trig() >> rnd >> randomSnHSlew;   
        
    speed >> phazorFree;
    speed * 0.5f >> randomSlew.in_cutoff(); 
    
    sawSlew.set(200.0f); //200hz slew = 5ms
    squareSlew.set(200.0f); //200hz slew = 5ms
    randomSnHSlew.set(200.0f); //200hz slew = 5ms

}

pdsp::Patchable& pdsp::LFO::in_freq(){
    return in("freq");
}

pdsp::Patchable& pdsp::LFO::in_retrig(){
    return in("retrig");
}

pdsp::Patchable& pdsp::LFO::in_phase_start(){
    return in("phase_start");
}

pdsp::Patchable& pdsp::LFO::out_triangle(){
    return out("triangle");
}

pdsp::Patchable& pdsp::LFO::out_sine(){
    return out("sine");
}

pdsp::Patchable& pdsp::LFO::out_saw(){
    return out("saw");
}

pdsp::Patchable& pdsp::LFO::out_square(){
    return out("square");
}

pdsp::Patchable& pdsp::LFO::out_sample_and_hold(){
    return out("sample_and_hold");
}

pdsp::Patchable& pdsp::LFO::out_random(){
    return out("random");
}
