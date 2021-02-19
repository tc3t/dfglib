#pragma once

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(colour) {

namespace srjw
{
    /* A colour system is defined by the CIE x and y coordinates of
   its three primary illuminants and the x and y coordinates of
   the white point. */
    struct colourSystem {
        const char* name;     	    	    /* Colour system name */
        double xRed, yRed,	    	    /* Red x, y */
               xGreen, yGreen,  	    /* Green x, y */
               xBlue, yBlue,    	    /* Blue x, y */
               xWhite, yWhite,  	    /* White point x, y */
           gamma;   	    	    /* Gamma correction for system */
    };

    /* White point chromaticities. */

    

    /*  Gamma of nonlinear correction.

    See Charles Poynton's ColorFAQ Item 45 and GammaFAQ Item 6 at:
    
       http://www.poynton.com/ColorFAQ.html
       http://www.poynton.com/GammaFAQ.html
 
*/
    const int GAMMA_REC709	= 0; /* Rec. 709 */

    #define IlluminantC     0.3101, 0.3162	    	/* For NTSC television */
    #define IlluminantD65   0.3127, 0.3291	    	/* For EBU and SMPTE */
    #define IlluminantE 	0.33333333, 0.33333333  /* CIE equal-energy illuminant */

    const colourSystem colourSystems[] = 
    {
                           /* Name                  xRed    yRed    xGreen  yGreen  xBlue  yBlue    White point        Gamma   */
        /*NTSCsystem  =*/  { "NTSC",               0.67,   0.33,   0.21,   0.71,   0.14,   0.08,   IlluminantC,    GAMMA_REC709 },
        /*EBUsystem   =*/  { "EBU (PAL/SECAM)",    0.64,   0.33,   0.29,   0.60,   0.15,   0.06,   IlluminantD65,  GAMMA_REC709 },
        /*SMPTEsystem =*/  { "SMPTE",              0.630,  0.340,  0.310,  0.595,  0.155,  0.070,  IlluminantD65,  GAMMA_REC709 },
        /*HDTVsystem  =*/  { "HDTV",               0.670,  0.330,  0.210,  0.710,  0.150,  0.060,  IlluminantD65,  GAMMA_REC709 },
        /*CIEsystem   =*/  { "CIE",                0.7355, 0.2645, 0.2658, 0.7243, 0.1669, 0.0085, IlluminantE,    GAMMA_REC709 },
        /*Rec709system =*/ { "CIE REC 709",        0.64,   0.33,   0.30,   0.60,   0.15,   0.06,   IlluminantD65,  GAMMA_REC709 }
    };

    #undef IlluminantC
    #undef IlluminantD65
    #undef IlluminantE

    extern const double cie_colour_match[81][3];

    inline constexpr double cieColourMatchTableStepInNm()    {return 5;}
    inline constexpr double cieColourMatchTableMinimumInNm() {return 380;}
    inline constexpr double cieColourMatchTableMaximumInNm() {return 780;}

    /*  	    	    	    UPVP_TO_XY
    Given 1976 coordinates u', v', determine 1931 chromaticities x, y
    */
    void upvp_to_xy(double up, double vp, double& xc, double& yc);

    /*  	    	    	    XY_TO_UPVP
    Given 1931 chromaticities x, y, determine 1976 coordinates u', v'
    */
    void xy_to_upvp(double xc, double yc, double& up, double& vp);

    /*                             XYZ_TO_RGB

    Given an additive tricolour system CS, defined by the CIE x
    and y chromaticities of its three primaries (z is derived
    trivially as 1-(x+y)), and a desired chromaticity (XC, YC,
    ZC) in CIE space, determine the contribution of each
    primary in a linear combination which sums to the desired
    chromaticity.  If the  requested chromaticity falls outside
    the Maxwell  triangle (colour gamut) formed by the three
    primaries, one of the r, g, or b weights will be negative. 

    Caller can use constrain_rgb() to desaturate an
    outside-gamut colour to the closest representation within
    the available gamut and/or norm_rgb to normalise the RGB
    components so the largest nonzero component has value 1.
*/
    void xyz_to_rgb(const colourSystem& cs,
                    double xc, double yc, double zc,
                    double& r, double& g, double& b);

    void xyzToRgbWithConstrainAndNorm(const ColourSystem& cs,
                    double xc, double yc, double zc,
                    double& r, double& g, double& b);

    /*                          CONSTRAIN_RGB

    If the requested RGB shade contains a negative weight for
    one of the primaries, it lies outside the colour gamut 
    accessible from the given triple of primaries.  Desaturate
    it by adding white, equal quantities of R, G, and B, enough
    to make RGB all positive.  The function returns 1 if the
    components were modified, zero otherwise.
    
    */
    int constrain_rgb(double& r, double& g, double& b);

    /*  	    	    	    NORM_RGB

    Normalise RGB components so the most intense (unless all
    are zero) has a value of 1.  
    */
    void norm_rgb(double& r, double& g, double& b);

    void xyz_to_rgb(ColourSystem cs,
                    double xc, double yc, double zc,
                    double& r, double& g, double& b);

    /*                            INSIDE_GAMUT

     Test whether a requested colour is within the gamut
     achievable with the primaries of the current colour
     system.  This amounts simply to testing whether all the
     primary weights are non-negative. */
    int inside_gamut(double r, double g, double b);

    /*                          GAMMA_CORRECT_RGB

    Transform linear RGB values to nonlinear RGB values. Rec.
    709 is ITU-R Recommendation BT. 709 (1990) ``Basic
    Parameter Values for the HDTV Standard for the Studio and
    for International Programme Exchange'', formerly CCIR Rec.
    709. For details see
    
       http://www.poynton.com/ColorFAQ.html
       http://www.poynton.com/GammaFAQ.html
*/
    void gamma_correct(const colourSystem& cs, double& c);

    void gamma_correct_rgb(const colourSystem& cs, double& r, double& g, double& b);

    // Evaluates cie colour match table at floating index.
    // Note: This is low level function that does not check arguments.
    double evaluateCieColourMatchAtFloatIndexNc(const double index, const int channel);

    /*                          SPECTRUM_TO_XYZ

    Calculate the CIE X, Y, and Z coordinates corresponding to
    a light source with spectral distribution given by  the
    function SPEC_INTENS, which is called with a series of
    wavelengths between 380 and 780 nm (the argument is 
    expressed in meters), which returns emittance at  that
    wavelength in arbitrary units.  The chromaticity
    coordinates of the spectrum are returned in the x, y, and z
    arguments which respect the identity:

            x + y + z = 1.
    */
    template <class WlToIntensityFunc_T>
    void spectrum_to_xyz(WlToIntensityFunc_T spec_intens,
                         double& x, double& y, double& z)
    {
        int i;
        double lambda, X = 0, Y = 0, Z = 0, XYZ;

        for (i = 0, lambda = 380; lambda < 780.1; i++, lambda += 5)
        {
            double Me;

            Me = spec_intens(lambda);
            X += Me * srjw::cie_colour_match[i][0];
            Y += Me * srjw::cie_colour_match[i][1];
            Z += Me * srjw::cie_colour_match[i][2];
        }
        XYZ = (X + Y + Z);
        x = X / XYZ;
        y = Y / XYZ;
        z = Z / XYZ;
    }
}

}}
