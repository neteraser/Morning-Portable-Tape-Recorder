#include <cmath>
#include <iostream>

class RIAAFilter {
private:
    double b0, b1, b2, a1, a2;
    double z1_state, z2_state; // Delay elements for Direct Form II

public:
    RIAAFilter(double sampleRate) {
        reset(sampleRate);
        z1_state = 0.0;
        z2_state = 0.0;
    }

    void reset(double fs) {
        // RIAA time constants in seconds
        double p1 = 3180e-6; // 50.05 Hz
        double p2 = 75e-6;   // 2212 Hz
        double z1 = 318e-6;  // 500.5 Hz

        double pole1 = std::exp(-1.0 / (fs * p1));
        double pole2 = std::exp(-1.0 / (fs * p2));
        double zero1 = std::exp(-1.0 / (fs * z1));

        // Simple 1st/2nd order combination mapping
        b0 = 1.0;
        b1 = -zero1;
        b2 = 0.0;
        
        a1 = -pole1 - pole2;
        a2 = pole1 * pole2;
    }

    double process(double x) {
        // Direct Form II realization
        double w = x - a1 * z1_state - a2 * z2_state;
        double y = b0 * w + b1 * z1_state + b2 * z2_state;
        
        z2_state = z1_state;
        z1_state = w;
        
        return y;
    }
};
