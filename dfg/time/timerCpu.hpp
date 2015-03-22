#pragma once

#include "../dfgDefs.hpp"
#include <boost/timer.hpp>

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(time) {

// TODO: Use better resolution, see boost/timer/timer.hpp
// Related code: boost::chrono::high_resolution_clock, std::chrono::high_resolution_clock.
class DFG_CLASS_NAME(TimerCpu)
{
public:

	DFG_CLASS_NAME(TimerCpu)() {} /*m_timer starts automatically*/

	double elapsedWallSeconds() {return m_timer.elapsed();}

public:
	boost::timer m_timer;
};

}} // module time
