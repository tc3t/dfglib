#include "defs.hpp"
#include "../dfgBase.hpp"
#include "specRendJw.hpp"
#include "../math/interpolationLinear.hpp"

/*

Note: This is a modified document originally from a 3rd party source, see credits below.
-----------------------------------------------------------------------------------------

*/


/*
                Colour Rendering of Spectra

                       by John Walker
                  http://www.fourmilab.ch/
          
         Last updated: March 9, 2003

           This program is in the public domain.

    For complete information about the techniques employed in
    this program, see the World-Wide Web document:

             http://www.fourmilab.ch/documents/specrend/
         
    The xyz_to_rgb() function, which was wrong in the original
    version of this program, was corrected by:
    
        Andrew J. S. Hamilton 21 May 1999
        Andrew.Hamilton@Colorado.EDU
        http://casa.colorado.edu/~ajsh/

    who also added the gamma correction facilities and
    modified constrain_rgb() to work by desaturating the
    colour by adding white.
    
    A program which uses these functions to plot CIE
    "tongue" diagrams called "ppmcie" is included in
    the Netpbm graphics toolkit:
        http://netpbm.sourceforge.net/
    (The program was called cietoppm in earlier
    versions of Netpbm.)

*/

#include <cmath>

 /*
CIE colour matching functions xBar, yBar, and zBar for
wavelengths from 380 through 780 nanometers, every 5
nanometers.  For a wavelength lambda in this range:

    cie_colour_match[(lambda - 380) / 5][0] = xBar
    cie_colour_match[(lambda - 380) / 5][1] = yBar
    cie_colour_match[(lambda - 380) / 5][2] = zBar

To save memory, this table can be declared as floats
rather than doubles; (IEEE) float has enough 
significant bits to represent the values.
*/

const double DFG_ROOT_NS::DFG_SUB_NS_NAME(colour)::srjw::cie_colour_match[81][3] =
{
    {0.0014,0.0000,0.0065}, {0.0022,0.0001,0.0105}, {0.0042,0.0001,0.0201},
    {0.0076,0.0002,0.0362}, {0.0143,0.0004,0.0679}, {0.0232,0.0006,0.1102},
    {0.0435,0.0012,0.2074}, {0.0776,0.0022,0.3713}, {0.1344,0.0040,0.6456},
    {0.2148,0.0073,1.0391}, {0.2839,0.0116,1.3856}, {0.3285,0.0168,1.6230},
    {0.3483,0.0230,1.7471}, {0.3481,0.0298,1.7826}, {0.3362,0.0380,1.7721},
    {0.3187,0.0480,1.7441}, {0.2908,0.0600,1.6692}, {0.2511,0.0739,1.5281},
    {0.1954,0.0910,1.2876}, {0.1421,0.1126,1.0419}, {0.0956,0.1390,0.8130},
    {0.0580,0.1693,0.6162}, {0.0320,0.2080,0.4652}, {0.0147,0.2586,0.3533},
    {0.0049,0.3230,0.2720}, {0.0024,0.4073,0.2123}, {0.0093,0.5030,0.1582},
    {0.0291,0.6082,0.1117}, {0.0633,0.7100,0.0782}, {0.1096,0.7932,0.0573},
    {0.1655,0.8620,0.0422}, {0.2257,0.9149,0.0298}, {0.2904,0.9540,0.0203},
    {0.3597,0.9803,0.0134}, {0.4334,0.9950,0.0087}, {0.5121,1.0000,0.0057},
    {0.5945,0.9950,0.0039}, {0.6784,0.9786,0.0027}, {0.7621,0.9520,0.0021},
    {0.8425,0.9154,0.0018}, {0.9163,0.8700,0.0017}, {0.9786,0.8163,0.0014},
    {1.0263,0.7570,0.0011}, {1.0567,0.6949,0.0010}, {1.0622,0.6310,0.0008},
    {1.0456,0.5668,0.0006}, {1.0026,0.5030,0.0003}, {0.9384,0.4412,0.0002},
    {0.8544,0.3810,0.0002}, {0.7514,0.3210,0.0001}, {0.6424,0.2650,0.0000},
    {0.5419,0.2170,0.0000}, {0.4479,0.1750,0.0000}, {0.3608,0.1382,0.0000},
    {0.2835,0.1070,0.0000}, {0.2187,0.0816,0.0000}, {0.1649,0.0610,0.0000},
    {0.1212,0.0446,0.0000}, {0.0874,0.0320,0.0000}, {0.0636,0.0232,0.0000},
    {0.0468,0.0170,0.0000}, {0.0329,0.0119,0.0000}, {0.0227,0.0082,0.0000},
    {0.0158,0.0057,0.0000}, {0.0114,0.0041,0.0000}, {0.0081,0.0029,0.0000},
    {0.0058,0.0021,0.0000}, {0.0041,0.0015,0.0000}, {0.0029,0.0010,0.0000},
    {0.0020,0.0007,0.0000}, {0.0014,0.0005,0.0000}, {0.0010,0.0004,0.0000},
    {0.0007,0.0002,0.0000}, {0.0005,0.0002,0.0000}, {0.0003,0.0001,0.0000},
    {0.0002,0.0001,0.0000}, {0.0002,0.0001,0.0000}, {0.0001,0.0000,0.0000},
    {0.0001,0.0000,0.0000}, {0.0001,0.0000,0.0000}, {0.0000,0.0000,0.0000}
};

void DFG_ROOT_NS::DFG_SUB_NS_NAME(colour)::srjw::upvp_to_xy(double up, double vp, double& xc, double& yc)
{
    xc = (9 * up) / ((6 * up) - (16 * vp) + 12);
    yc = (4 * vp) / ((6 * up) - (16 * vp) + 12);
}

void DFG_ROOT_NS::DFG_SUB_NS_NAME(colour)::srjw::xy_to_upvp(double xc, double yc, double& up, double& vp)
{
    up = (4 * xc) / ((-2 * xc) + (12 * yc) + 3);
    vp = (9 * yc) / ((-2 * xc) + (12 * yc) + 3);
}

void DFG_ROOT_NS::DFG_SUB_NS_NAME(colour)::srjw::xyz_to_rgb(const colourSystem& cs,
                double xc, double yc, double zc,
                double& r, double& g, double& b)
{
    double xr, yr, zr, xg, yg, zg, xb, yb, zb;
    double xw, yw, zw;
    double rx, ry, rz, gx, gy, gz, bx, by, bz;
    double rw, gw, bw;

    xr = cs.xRed;    yr = cs.yRed;    zr = 1 - (xr + yr);
    xg = cs.xGreen;  yg = cs.yGreen;  zg = 1 - (xg + yg);
    xb = cs.xBlue;   yb = cs.yBlue;   zb = 1 - (xb + yb);

    xw = cs.xWhite;  yw = cs.yWhite;  zw = 1 - (xw + yw);

    /* xyz -> rgb matrix, before scaling to white. */
    
    rx = (yg * zb) - (yb * zg);  ry = (xb * zg) - (xg * zb);  rz = (xg * yb) - (xb * yg);
    gx = (yb * zr) - (yr * zb);  gy = (xr * zb) - (xb * zr);  gz = (xb * yr) - (xr * yb);
    bx = (yr * zg) - (yg * zr);  by = (xg * zr) - (xr * zg);  bz = (xr * yg) - (xg * yr);

    /* White scaling factors.
       Dividing by yw scales the white luminance to unity, as conventional. */
       
    rw = ((rx * xw) + (ry * yw) + (rz * zw)) / yw;
    gw = ((gx * xw) + (gy * yw) + (gz * zw)) / yw;
    bw = ((bx * xw) + (by * yw) + (bz * zw)) / yw;

    /* xyz -> rgb matrix, correctly scaled to white. */
    
    rx = rx / rw;  ry = ry / rw;  rz = rz / rw;
    gx = gx / gw;  gy = gy / gw;  gz = gz / gw;
    bx = bx / bw;  by = by / bw;  bz = bz / bw;

    /* rgb of the desired point */
    
    r = (rx * xc) + (ry * yc) + (rz * zc);
    g = (gx * xc) + (gy * yc) + (gz * zc);
    b = (bx * xc) + (by * yc) + (bz * zc);
}

int DFG_ROOT_NS::DFG_SUB_NS_NAME(colour)::srjw::inside_gamut(double r, double g, double b)
{
    return (r >= 0) && (g >= 0) && (b >= 0);
}

void DFG_ROOT_NS::DFG_SUB_NS_NAME(colour)::srjw::gamma_correct(const colourSystem& cs, double& c)
{
    double gamma;

    gamma = cs.gamma;

    if (gamma == GAMMA_REC709) {
    /* Rec. 709 gamma correction. */
    double cc = 0.018;
    
    if (c < cc) {
        c *= ((1.099 * pow(cc, 0.45)) - 0.099) / cc;
    } else {
        c = (1.099 * pow(c, 0.45)) - 0.099;
    }
    } else {
    /* Nonlinear colour = (Linear colour)^(1/gamma) */
    c = pow(c, 1.0 / gamma);
    }
}

void DFG_ROOT_NS::DFG_SUB_NS_NAME(colour)::srjw::gamma_correct_rgb(const colourSystem& cs, double& r, double& g, double& b)
{
    gamma_correct(cs, r);
    gamma_correct(cs, g);
    gamma_correct(cs, b);
}

void DFG_ROOT_NS::DFG_SUB_NS_NAME(colour)::srjw::xyz_to_rgb(const ColourSystem cs,
                double xc, double yc, double zc,
                double& r, double& g, double& b)
{
    xyz_to_rgb(colourSystems[cs], xc, yc, zc, r, g, b);
}

void DFG_ROOT_NS::DFG_SUB_NS_NAME(colour)::srjw::xyzToRgbWithConstrainAndNorm(const ColourSystem& cs,
                                                                                double xc, double yc, double zc,
                                                                                double& r, double& g, double& b)
{
    xyz_to_rgb(cs, xc, yc, zc, r, g, b);
    if (constrain_rgb(r, g, b))
        norm_rgb(r, g, b);  // Approximation
    else
        norm_rgb(r, g, b);
}

int DFG_ROOT_NS::DFG_SUB_NS_NAME(colour)::srjw::constrain_rgb(double& r, double& g, double& b)
{
    double w;

    /* Amount of white needed is w = - min(0, *r, *g, *b) */
    
    w = (0 < r) ? 0 : r;
    w = (w < g) ? w : g;
    w = (w < b) ? w : b;
    w = -w;

    /* Add just enough white to make r, g, b all positive. */
    
    if (w > 0) {
        r += w;  g += w; b += w;
        return 1;                     /* Colour modified to fit RGB gamut */
    }

    return 0;                         /* Colour within RGB gamut */
}

void DFG_ROOT_NS::DFG_SUB_NS_NAME(colour)::srjw::norm_rgb(double& r, double& g, double& b)
{
    const double greatest = Max(r, g, b);
    
    if (greatest > 0)
    {
        r /= greatest;
        g /= greatest;
        b /= greatest;
    }
}

double DFG_ROOT_NS::DFG_SUB_NS_NAME(colour)::srjw::evaluateCieColourMatchAtFloatIndexNc(const double index, const int channel)
{
    // Handle last item separately: in interpolation the upper index would go out of bounds.
    if (index == count(srjw::cie_colour_match) - 1)
        return lastOf(srjw::cie_colour_match)[channel];

    const auto lowerBoundIndex = floorToInteger<size_t>(index);
    const auto nextIndex = lowerBoundIndex + 1;

    

    const auto val = DFG_ROOT_NS::DFG_SUB_NS_NAME(math)::interpolationLinear(index,
                                        double(lowerBoundIndex),
                                        srjw::cie_colour_match[lowerBoundIndex][channel],
                                        double(nextIndex),
                                        srjw::cie_colour_match[nextIndex][channel]);
    return val;
}
