#include <cmath>

double toRad(double deg) { return deg * (M_PI / 180.0); } 
double toDeg(double rad) { return rad * (180.0 / M_PI); }

double d_cos(double th) { return cos(toRad(th)); }
double d_sin(double th) { return sin(toRad(th)); }
double d_tan(double th) { return tan(toRad(th)); }
double d_atan(double th) { return toDeg(atan(th)); }
double d_acos(double th) { return toDeg(acos(th));}

double floatMod(double input, double mod) {
    double r = fmod(input, mod);   // from <cmath>
    if (r < 0) r += mod;
    return r;
}

// Adapted from https://edwilliams.org/sunrise_sunset_algorithm.htm
double suntime(double day, double month, double year, double lat, 
              double lon, double zenith, bool risingTime) {
    /*
    Zenith being sun's position at the output time. 
        Official sunrise = 90 degrees
        Civil = 96
        Nautical = 102
        Astronomical = 108

    risingTime true if we want the time of the sunrise
    False if we want time of sunset 
    */

    // Get day of the year 
    int n1 = floor(275 * month/9); 
    int n2 = floor((month + 9) / 12); 
    int n3 = (1 + floor((year - 4 * floor(year / 4) + 2) / 3 ));
    double n = n1 - (n2 * n3) + day -30; 

    // Convert longitude to hour 
    double lngHour = lon / 15; 
    double t = (risingTime) ? 
        (n + ((6- lngHour) / 24)) : 
        (n + ((18 - lngHour) / 24)); 

    // Sun's mean anomaly 
    double m = (0.9856 * t) - 3.289;

    // Sun's true longitude 
    double l = m + (1.916 * d_sin(m))
                + (0.020 * d_sin(2 * m))
                + 282.634; 
                
    l = floatMod(l, 360); 

    // Sun's right ascention
    double ra = d_atan(0.91764 * d_tan(l)); 
    ra = floatMod(ra, 360); 

    // Adjust to be in same quadrant as L 
    double lquadrant = floor(l/90) * 90; 
    double rquadrant = floor(ra/90) * 90;
    ra = ra + (lquadrant - rquadrant); 
    ra = ra / 15; 

    // Calculate sun's declination
    double sinDec = 0.39782 * d_sin(l); 
    double cosDec = toDeg(cos(asin(toRad(sinDec)))); 

    // Calculate sun's local hour angle 
    double cosH = (d_cos(zenith) - (sinDec * d_sin(lat)))
                / (cosDec * d_cos(lat)); 
    
    if ( (cosH > 1 && risingTime) || (cosH < -1 && !risingTime)) {
        // Corner case for if you're in Alaska or something 
        // and the sun never rises/sets 
        return -1.; 
    }

    double h = (risingTime) ? 
                360 - d_acos(cosH) : 
                d_acos(cosH); 
    
    h = h/15; 

    // Calculate local mean time
    t = h + ra - (0.06571 * t) - 6.622; 

    // Adjust to UTC 
    t = t - lngHour; 
    t = floatMod(t, 24); 

    return t; 
}